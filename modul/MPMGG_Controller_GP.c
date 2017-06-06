/*
 * Creative Labs Blaster GamePad Cobra driver for Linux
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */
#include "PortExpander/mcp23017.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/gameport.h>
#include <linux/input.h>
#include <linux/jiffies.h>

#define DRIVER_DESC	"Multi Player Multi Goal Gaming Controller Driver"
#define PORT_DRIVER_NAME	"Multi Player Multi Goal Gaming Port"
#define PAD_DRIVER_NAME	"Multi Player Multi Goal Gaming Pad"
#define DRIVER_OUT	"MPMG-Pad: "

MODULE_AUTHOR("Patrick Schweinsberg <p.schweinsberg@student.uni-kassel.de>");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL"); 

//#define COBRA_MAX_STROBE	45	/* 45 us max wait for first strobe */
//#define COBRA_LENGTH		36
#define MAX_PADS 4

/******************************
 *         Keymapping         *
 ******************************/
#define INP_KEY_UP		0x0010
#define INP_KEY_DOWN	0x0080
#define INP_KEY_LEFT	0x0020
#define INP_KEY_RIGHT	0x0040
#define INP_KEY_TL		0x0004
#define INP_KEY_SELECT	0x0008
#define INP_KEY_START	0x1000
#define INP_KEY_TR		0x2000
#define INP_KEY_A		0x0400
#define INP_KEY_B		0x0100
#define INP_KEY_X		0x0800
#define INP_KEY_Y		0x0200

static int mpmggpad_mask[] 	= {
//	INP_KEY_UP, 
//	INP_KEY_DOWN, 
//	INP_KEY_LEFT,
//	INP_KEY_RIGHT, 
	INP_KEY_START, 
	INP_KEY_SELECT, 
	INP_KEY_TL, 
	INP_KEY_TR, 
	INP_KEY_X, 
	INP_KEY_Y, 
	INP_KEY_A, 
	INP_KEY_B, 
	0 };
static int mpmggpad_btn[]	= { 
//	BTN_UP, 
//	BTN_DOWN, 
//	BTN_LEFT,
//	BTN_RIGHT, 
	BTN_START, 
	BTN_SELECT, 
	BTN_TL, 
	BTN_TR, 
	BTN_X, 
	BTN_Y, 
	BTN_A, 
	BTN_B, 
	0 };

struct mpmggpad_mod {
	struct gameport *gameport;
	struct input_dev *pad[MAX_PADS];
	struct input_dev *dev[MAX_PADS];
	int reads;
	int bads;
	char phys[MAX_PADS][32];
	char addrs[MAX_PADS];
	mcp23017 mcp23017[MAX_PADS];
	//st7735 st7735[MAX_PADS];
};

static void mpmggpad_poll(struct gameport *gameport){
	struct mpmggpad_mod *mpmggpad = gameport_get_drvdata(gameport);
	struct input_dev *pad;
	int data;
	int i,button;

	mpmggpad->reads++;
	
	//kill exists
	//if ((r = cobra_read_packet(gameport, data)) != mpmggpad->addrs[0]) {
	//	mpmggpad->bads++;
	//	return;
	//}

	for (i = 0; i < MAX_PADS; i++){
			pad = mpmggpad->pad[i];
			data = gpio_read_mcp23017( mpmggpad->mcp23017[i] );
			
			input_report_abs(pad, ABS_X, 
						((data&INP_KEY_UP) == INP_KEY_UP)		- ((data&INP_KEY_DOWN) == INP_KEY_DOWN) );
			input_report_abs(pad, ABS_Y, 
						((data&INP_KEY_RIGHT) == INP_KEY_RIGHT)	- ((data&INP_KEY_LEFT) == INP_KEY_LEFT) );

			for(button=0;mpmggpad_btn[button];button++)
				input_report_key(pad, mpmggpad_btn[button],		
						(data&mpmggpad_mask[button]) == mpmggpad_mask[button]	);

			input_sync(pad);
	}
}

static int mpmggpad_open(struct input_dev *pad){
	struct mpmggpad_mod *mpmggpad = input_get_drvdata(pad);

	gameport_start_polling(mpmggpad->gameport);
	return 0;
}

static void mpmggpad_close(struct input_dev *pad){
	struct mpmggpad_mod *mpmggpad = input_get_drvdata(pad);

	gameport_stop_polling(mpmggpad->gameport);
}

static int mpmggpad_connect(struct gameport *gameport, struct gameport_driver *drv){
	struct mpmggpad_mod *mpmggpad;
	struct input_dev *input_pad;
	int i,j;
	int err;

	mpmggpad = kzalloc(sizeof(struct mpmggpad_mod), GFP_KERNEL);
	if (!mpmggpad)
		return -ENOMEM;

	mpmggpad->gameport = gameport;

	gameport_set_drvdata(gameport, mpmggpad);

	err = gameport_open(gameport, drv, GAMEPORT_MODE_RAW);
	if (err)
		goto fail1;

	for(i = 0; i < MAX_PADS; i++){
		mpmggpad->addrs[i]=-1;
	}
	
	find_mcp23017((mpmggpad->addrs), MAX_PADS);
	
	for (i = 0; i < MAX_PADS; i++){
		if(mpmggpad->addrs[i]!=-1){
			printk(KERN_WARNING DRIVER_OUT "\n\r\tSlot: %d \n\r\tDevice: %s\n\r\tAddress: %d", i, gameport->phys, mpmggpad->addrs[i]);
			mpmggpad->mcp23017[i]=init_mcp23017(mpmggpad->addrs[i]);
		}
	}
	
	if (mpmggpad->addrs[i]==-1) {
		err = -ENODEV;
		goto fail2;
	}

	gameport_set_poll_handler(gameport, mpmggpad_poll);
	gameport_set_poll_interval(gameport, 20);

	for (i = 0; i < MAX_PADS; i++) {
		//if (~(mpmggpad->exists >> i) & 1)
		//	continue;

		mpmggpad->pad[i] = input_pad = input_allocate_device();
		if (!input_pad) {
			err = -ENOMEM;
			goto fail3;
		}

		snprintf(mpmggpad->phys[i], sizeof(mpmggpad->phys[i]),
			 "%s/input%d", gameport->phys, i);

		input_pad->name = PAD_DRIVER_NAME;
		input_pad->phys = mpmggpad->phys[i];
		input_pad->id.bustype = BUS_GAMEPORT;
		input_pad->id.vendor = GAMEPORT_ID_VENDOR_CREATIVE;
		input_pad->id.product = 0x00;
		input_pad->id.version = 0x00;
		input_pad->dev.parent = &gameport->dev;

		input_set_drvdata(input_pad, mpmggpad);

		input_pad->open = mpmggpad_open;
		input_pad->close = mpmggpad_close;

		input_pad->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
		input_set_abs_params(input_pad, ABS_X, -1, 1, 0, 0);
		input_set_abs_params(input_pad, ABS_Y, -1, 1, 0, 0);
		
		
		for(j=0;mpmggpad_btn[j];j++)
			set_bit(mpmggpad_btn[j], input_pad->keybit);

		err = input_register_device(mpmggpad->pad[i]);
		if (err)
			goto fail4;
	}

	return 0;

 fail4:	input_free_device(mpmggpad->pad[i]);
 fail3:	while (--i >= 0)
		if (mpmggpad->pad[i])
			input_unregister_device(mpmggpad->pad[i]);
 fail2:	printk(KERN_EMERG DRIVER_OUT "no Device found");
	gameport_close(gameport);
 fail1:	gameport_set_drvdata(gameport, NULL);
	kfree(mpmggpad);
	return err;
}

static void mpmggpad_disconnect(struct gameport *gameport)
{
	struct mpmggpad_mod *mpmggpad = gameport_get_drvdata(gameport);
	int i;

	for (i = 0; i < MAX_PADS; i++)
		if (mpmggpad->addrs[i] )
			input_unregister_device(mpmggpad->pad[i]);
	gameport_close(gameport);
	gameport_set_drvdata(gameport, NULL);
	kfree(mpmggpad);
}

static struct gameport_driver mpmggpad_driver = {
	.driver		= {
		.name	= PORT_DRIVER_NAME,
	},
	.description	= DRIVER_DESC,
	.connect	= mpmggpad_connect,
	.disconnect	= mpmggpad_disconnect,
};

static int __init mpmggpad_init(void)
{
	return gameport_register_driver(&mpmggpad_driver);
}

static void __exit mpmggpad_exit(void)
{
	gameport_unregister_driver(&mpmggpad_driver);
}

module_init(mpmggpad_init);
module_exit(mpmggpad_exit);