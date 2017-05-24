#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/sched.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/stat.h>

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
//???#include <linux/i2c-dev.h>
#endif

#ifdef RPI
//RPI uses xxx for SPI
//RPI has HW I²C
#include <linux/i2c-dev.h>
#include "i2c_funcs.c"	/* I2C routines */
#endif


/******************************
 *       Drivers config       *
 ******************************/
#define DRIVER_DESC	"Multi Player Multi Goal Gaming Controller Driver"
#define DRIVER_NAME	"Multi Player Multi Goal Gaming Pad"

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
	u8 xtype;
} xpad_device[] = {
	{ 0x0000, 0x0000, DRIVER_DESC, 0, TYPE}
};

/* buttons */
static const signed short mpmggpad_common_btn[] = {
	BTN_A, BTN_B, BTN_X, BTN_Y,	/* game buttons */
	BTN_START, BTN_SELECT,		/* controll buttons */
	BTN_TL, BTN_TR,				/* trigger buttons */
	-1							/* terminating entry */
};
 
/* dpad buttons */
static const signed short mpmggpad_btn_pad[] = {
	BTN_TRIGGER_HAPPY1, BTN_TRIGGER_HAPPY2,	/* d-pad left, right */
	BTN_TRIGGER_HAPPY3, BTN_TRIGGER_HAPPY4,	/* d-pad up, down */
	-1										/* terminating entry */
};

/* dpad axes */
static const signed short mpmggpad_abs_pad[] = {
	ABS_HAT0X, ABS_HAT0Y,	/* d-pad axes */
	-1						/* terminating entry */
};

/* triggers axes */
static const signed short mpmggpad_abs_triggers[] = {
	ABS_Z, ABS_RZ,		/* triggers left/right */
	-1					/* terminating entry */
};


//MODULE_DEVICE_TABLE(usb, xpad_table);

/******************************
 *       Initial things       *
 ******************************/
static int __init controller_init(void)
{
    /* called on module loading */
	int err;
	printk("MPMGG_Controller_Driver: initing");
	//TODO: set SPI config
	if (err) goto fail_spi;
	//TODO: set I²C config
	printk("MPMGG_Controller_Driver: inited");
	
	return 0; /* success */
	
	fail_spi: 
		return 1;
}

static void __exit controller_exit(void)
{
    /* called on module unloading */
	printk("MPMGG_Controller_Driver: cleaned");
}

module_init(controller_init);
module_exit(controller_exit);


/******************************
 *      Driver functions      *
 ******************************/
 mpmggpad_probe
 
 mpmggpad_disconnect
 
 mpmggpad_suspend
 
 mpmggpad_resume
 
 mpmggpad_table

/******************************
 *  Driver modul definition   *
 ******************************/
module_gameport_driver(controller_drv);
static struct usb_driver mpmggpad_driver = {
	.name		= DRIVER_NAME,
	.probe		= mpmggpad_probe,
	.disconnect	= mpmggpad_disconnect,
	.suspend	= mpmggpad_suspend,
	.resume		= mpmggpad_resume,
	.id_table	= mpmggpad_table,
};

module_driver(mpmggpad_driver);
