#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
//#include <linux/sched.h>
//#include <linux/rcupdate.h>
//#include <linux/slab.h>
//#include <linux/stat.h>





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
 * To support event triggers  *
 *  the module needs to poll  *
 *         the events         *
 ******************************/
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>

static struct hrtimer htimer;
static ktime_t kt_periode;

static void timer_init(void){
    kt_periode = ktime_set(0, 16666); //seconds,nanoseconds: 60 fps
    hrtimer_init (& htimer, CLOCK_REALTIME, HRTIMER_MODE_REL);
    htimer.function = timer_function;
    hrtimer_start(& htimer, kt_periode, HRTIMER_MODE_REL);
}

static void timer_cleanup(void){
    hrtimer_cancel(& htimer);
}

static enum hrtimer_restart timer_function(struct hrtimer * timer){
    // TODO: Poll
	int keys = gpio_read_mcp23017(pad_device->mcp23017_dev);
	
	input_report_abs(dev, ABS_X, ((keys&INP_KEY_UP)==INP_KEY_UP) - ((keys&INP_KEY_DOWN)==INP_KEY_DOWN));
	input_report_abs(dev, ABS_Y, ((keys&INP_KEY_RIGHT)==INP_KEY_RIGHT) - ((keys&INP_KEY_LEFT)==INP_KEY_LEFT));
	//if((pad_device->keymap^keys)&INP_KEY_UP){
	input_report_key(dev, BTN_DPAD_UP, ((keys&INP_KEY_UP)==INP_KEY_UP));
	//}
	input_report_key(dev, BTN_DPAD_DOWN, ((keys&INP_KEY_DOWN)==INP_KEY_DOWN));
	input_report_key(dev, BTN_DPAD_LEFT, ((keys&INP_KEY_LEFT)==INP_KEY_LEFT));
	input_report_key(dev, BTN_DPAD_RIGHT, ((keys&INP_KEY_RIGHT)==INP_KEY_RIGHT));
	input_report_key(dev, BTN_TL,((keys&INP_KEY_TL)==INP_KEY_TL));
	input_report_key(dev, BTN_TR,((keys&INP_KEY_TR)==INP_KEY_TR));
	input_report_key(dev, BTN_SELECT,((keys&INP_KEY_SELECT)==INP_KEY_SELECT));
	input_report_key(dev, BTN_START,((keys&INP_KEY_START)==INP_KEY_START));
	input_report_key(dev, BTN_A,((keys&INP_KEY_A)==INP_KEY_A));
	input_report_key(dev, BTN_B,((keys&INP_KEY_B)==INP_KEY_B));
	input_report_key(dev, BTN_X,((keys&INP_KEY_X)==INP_KEY_X));
	input_report_key(dev, BTN_Y,((keys&INP_KEY_Y)==INP_KEY_Y));
	
	//keymap=keys;
	
    hrtimer_forward_now(timer, kt_periode);
    return HRTIMER_RESTART;
}

/******************************
 *       Drivers config       *
 ******************************/
#define DRIVER_DESC	"Multi Player Multi Goal Gaming Controller Driver"
#define DRIVER_NAME	"Multi Player Multi Goal Gaming Pad"

static const char drv_titel[] = "MPMGG_Controller_Driver";
static int major; // =250 Abhaengig von der /dev/rtc Geraetedatei
static int minor;

#define TYPE 0

MODULE_AUTHOR("Patrick Schweinsberg <p.schweinsberg@student.uni-kassel.de>");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL"); 

/******************************
 *      Drivers structs       *
 ******************************/
static const struct mpmggpad_device {
	u16 idVendor;
	u16 idProduct;
	char *name;
	u8 mapping;
	struct mcp23017 mcp23017_dev;
	//int keymap;
	struct st7735 st7735_dev;
} pad_device[] = {
	{ 0x0000, 0x0000, DRIVER_DESC, 0, null, null}
};

/* buttons */
/*static const short mpmggpad_common_btn[] = {
	BTN_A, BTN_B, BTN_X, BTN_Y,	// game buttons 
	BTN_START, BTN_SELECT,		// controll buttons 
	BTN_TL, BTN_TR,				// trigger buttons 
};*/

/******************************
 *       Initial things       *
 ******************************/
static int __init controller_init(void){
    /* called on module loading */
	int err;
	printk("%s: initing");
	
	// Registrieren des i2c-Busses
	if ( (err = i2c_add_driver(&rtc_driver)) ) {
		printk("%s: ERROR, can't register device\n", drv_titel);
		return err;
	}

	if ( major ) {
	      // Registieren des Treibers beim Kernel mit vorgegebener Major-Nummer
		  //TODO: minornummer?!?
	else {
		//registriert den Treiber beim Kernel, Major-Nummer wird zugewiesen
		//TODO: major registrieren
		//like: major = register_chrdev(0, rtc_name, &fops);
		if ( major > 0 ) {
			printk("%s: registriert with %d as Major\n",drv_titel, major);
			return 0;
		}
		printk("%s: Treiber konnte nicht registriert werden\n", drv_titel);
		return -EIO;
	}
	
	
	
	//TODO: set SPI config
	if (err) goto fail_spi;
	
	//TODO: set I²C config
	printk("MPMGG_Controller_Driver: inited");
	
	return 0; /* success */
	
	fail_spi: 
		return 1;
}

static void __exit controller_exit(void){
    /* called on module unloading */
    if ( i2c_del_driver(&rtc_driver) ) {
		printk("MPMGG_Controller_Driver: ERROR, can't remove driver\n", rtc_name);
		return;
	}
	printk("MPMGG_Controller_Driver: cleaned");
}

module_init(controller_init);
module_exit(controller_exit);


/******************************
 *      Driver functions      *
 ******************************/
static int mpmggpad_probe(struct platform_device *dev){
	struct mpmggpad_device *i2c;
	i2c = devm_kzalloc(&dev->dev, sizeof(*i2c), GFP_KERNEL);
	
	input_set_abs_params(dev, ABS_X, -1, 1, 0, 0);
	input_set_abs_params(dev, ABS_Y, -1, 1, 0, 0);
	input_set_capability(dev, EV_KEY, BTN_DPAD_UP);
	input_set_capability(dev, EV_KEY, BTN_DPAD_DOWN);
	input_set_capability(dev, EV_KEY, BTN_DPAD_LEFT);
	input_set_capability(dev, EV_KEY, BTN_DPAD_RIGHT);
	input_set_capability(dev, EV_KEY, BTN_A);
	input_set_capability(dev, EV_KEY, BTN_B);
	input_set_capability(dev, EV_KEY, BTN_X);
	input_set_capability(dev, EV_KEY, BTN_Y);
	input_set_capability(dev, EV_KEY, BTN_TL);
	input_set_capability(dev, EV_KEY, BTN_TR);
	input_set_capability(dev, EV_KEY, BTN_SELECT);
	input_set_capability(dev, EV_KEY, BTN_START);
    //called once, inits the devices allocates memory and ressources
    //TODO: find device address on i²c-bus
	int addr = 0;
	pad_device.mcp23017_dev = init_mcp23017(addr);
}
 
 
static int mpmggpad_disconnect(struct platform_device *dev){
	
}


/******************************
 *  Driver modul definition   *
 ******************************/
 /*
static struct usb_driver mpmggpad_driver = {
	.name		= DRIVER_NAME,
	.probe		= mpmggpad_probe,
	.disconnect	= mpmggpad_disconnect,
	.suspend	= mpmggpad_suspend,
	.resume		= mpmggpad_resume,
	.id_table	= mpmggpad_table,
};*/
/*
static struct i2c_driver mpmggpad_driver = {
      .driver = {
        .name = rtc_name,
		.owner = THIS_MODULE,
	},
	.attach_adapter = rtc_attach,
	.detach_client = rtc_detach
};
*/
static const struct of_device_id mcp23017_i2c_match[] = {
	{ .compatible = "Microchip, mcp23017-i2c" },
	{},
};

static struct platform_driver mpmggpad_driver = {
	.driver = {
		.name = DRIVER_DESC,
		.of_match_table = mcp23017_i2c_match,
	},
	.probe = mpmggpad_probe,
	.remove = mpmggpad_disconnect,
	.disconnect	= mpmggpad_disconnect,
};

MODULE_DEVICE_TABLE(of, mcp23017_i2c_match);
module_platform_driver(mpmggpad_driver);
module_driver(mpmggpad_driver);
