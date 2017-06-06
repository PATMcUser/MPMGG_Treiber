#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>


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


/******************************
 *    Include TFT Library     *
 ******************************/
#include "PortExpander/mcp23017.h"


/******************************
 *  Include Display Library   *
 ******************************/
#include "TFT/TFT_ST7735.h"


/******************************
 * Use the following flag for *
 *      if HW is Odroid!      *
 ******************************/
//#define ODROID


/******************************
 *  If the HW is not Odroid,  *
 * the Raspberry Pi is chosen *
 ******************************/
#ifndef ODROID
#define RPI  
#endif


/******************************
 *  Includes for HW-Platform  *
 ******************************/
#ifdef ODROID
//ODROID uses xxx for SPI
//ODROID has no HW I²C
#endif


/******************************
 *       Drivers config       *
 ******************************/
 
#define MAX_PADS 4
#define DRIVER_DESC	"Multi Player Multi Goal Gaming Controller Driver"
#define DRIVER_NAME	"Multi Player Multi Goal Gaming Pad"

static const char drv_titel[] = "MPMGG_Controller_Driver";

MODULE_AUTHOR("Patrick Schweinsberg <p.schweinsberg@student.uni-kassel.de>");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL"); 

/******************************
 *      Drivers structs       *
 ******************************/
static struct mpmggpad_config  {
	struct input_dev *dev[DB9_MAX_DEVICES];
	struct timer_list timer;
	struct pardevice *pd;
	int mode;
	int used;
	int parportno;
	struct mutex mutex;
	char phys[DB9_MAX_DEVICES][32];
	
	struct mcp23017 mcp23017_dev;
	struct st7735 st7735_dev;
};

static struct mpmggpad_config mpmggpad_cfg[MAX_PADS];

/******************************
 *       Initial things       *
 ******************************/
 static struct parport_driver mpmggpad_driver = {
	.name = DRIVER_NAME,
	.match_port = mpmggpad_attach,
	.detach = mpmggpad_detach,
	.devmodel = true,
};
 
static int __init mpmggpad_init_driver(void){
	int i;
	
	//RESET
	s_pGpioRegisters = (struct GpioRegisters *)__io_address(GPIO_BASE);
	SetGPIOFunction( RESET_PIN, 0b001);
	Reset_false();
	Reset_true();
	Reset_false();
	
	//ADD DRIVERS
	for (i = 0; i < MAX_PADS; i++) {
		/* TODO:
			Speicher allokieren für
			struct mcp23017 mcp23017_dev;
			struct st7735 st7735_dev;
			und im struct mpmggpad_config speichern
		*/
	}

	return parport_register_driver(&mpmggpad_driver);
}

static void __exit mpmggpad_exit_driver(void){
	Reset_true();
	parport_unregister_driver(&mpmggpad_driver);
}

module_init(mpmggpad_init_driver);
module_exit(mpmggpad_exit_driver);
 

static void mpmggpad_attach(struct parport *pp){
}

static void mpmggpad_detach(struct parport *port){
	int i;
	struct db9 *db9;

	for (i = 0; i < MAX_PADS; i++) {
		if (mpmggpad_cfg[i] && mpmggpad_cfg[i]->parportno == port->number)
			break;
	}

	if (i == MAX_PADS)
		return;

	db9 = mpmggpad_cfg[i];
	mpmggpad_cfg[i] = NULL;

	for (i = 0; i < min(db9_modes[db9->mode].n_pads, MAX_PADS); i++)
		input_unregister_device(db9->dev[i]);
	parport_unregister_device(db9->pd);
	kfree(db9);
}


/******************************
 *      Driver functions      *
 ******************************/

/******************************
 *  Driver modul definition   *
 ******************************/

/******************************
 *        Driver Reset        *
 ******************************/
#include<asm/io.h> 
#include<mach/platform.h> 
#include <linux/timer.h>
#include <linux/err.h>

#define RESET_PIN		17

struct GpioRegisters
{
    uint32_t GPFSEL[6];
    uint32_t Reserved1;
    uint32_t GPSET[2];
    uint32_t Reserved2;
    uint32_t GPCLR[2];
};

struct GpioRegisters *s_pGpioRegisters;

static void SetGPIOFunction(int GPIO, int functionCode)
{
    int registerIndex = GPIO / 10;
    int bit = (GPIO % 10) * 3;

    unsigned oldValue = s_pGpioRegisters-> GPFSEL[registerIndex];
    unsigned mask = 0b111 << bit;
    printk("Changing function of GPIO%d from %x to %x\n",
           GPIO,
           (oldValue >> bit) & 0b111,
           functionCode);

    s_pGpioRegisters-> GPFSEL[registerIndex] =
        (oldValue & ~mask) | ((functionCode << bit) & mask);
}

static void SetGPIOOutputValue(int GPIO, bool outputValue)
{
    if (outputValue)
        s_pGpioRegisters-> GPSET[GPIO / 32] = (1 << (GPIO % 32));
    else
        s_pGpioRegisters-> GPCLR[GPIO / 32] = (1 << (GPIO % 32));
}

static struct timer_list s_rstTimer;

static void Reset_false(){
    SetGPIOOutputValue( LedGpioPin, 1);
}

static void Reset_true(){
    SetGPIOOutputValue( LedGpioPin, 0);
    mod_timer(&s_rstTimer,
              jiffies + msecs_to_jiffies(500));
}