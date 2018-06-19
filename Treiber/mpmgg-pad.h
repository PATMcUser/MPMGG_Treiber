#ifndef _EXPANDERLIB_
#define _EXPANDERLIB_

#include <linux/slab.h>			/* kzalloc */
#include <linux/module.h>   	/* Needed by all modules */
#include <linux/kernel.h>   	/* KERN_INFO */
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/sched.h>
#include <linux/fb.h>

#include "mpmgg-xchange.h"

#define DRIVER_DESC_P	"Multi Player Multi Goal Gaming Gamepad Driver"

MODULE_AUTHOR("Patrick Schweinsberg <p.schweinsberg@student.uni-kassel.de>");
MODULE_DESCRIPTION(DRIVER_DESC_P);
MODULE_LICENSE("GPL"); 


#define POLL_RATE	  		   40 
#define	ROTATE					0
#define DEBOUNCECNT				2
#define CALL_DELAY			   10

/******************************
 *         Keymapping         *
 ******************************/
#define MPMGG_KEY_INP_UP		0x8000  //B
#define MPMGG_KEY_INP_DOWN		0x1000  //B
#define MPMGG_KEY_INP_LEFT		0x2000  //B
#define MPMGG_KEY_INP_RIGHT		0x4000  //B
#define MPMGG_KEY_INP_TL		0x0800  //B
#define MPMGG_KEY_INP_SELECT	0x0400  //B
#define MPMGG_KEY_INP_START		0x0020  //A
#define MPMGG_KEY_INP_TR		0x0010  //A
#define MPMGG_KEY_INP_A			0x0008  //A
#define MPMGG_KEY_INP_B			0x0002  //A
#define MPMGG_KEY_INP_X			0x0004  //A
#define MPMGG_KEY_INP_Y			0x0001  //A


static int mpmggpad_mask[] 	= {
	MPMGG_KEY_INP_TL, 
	MPMGG_KEY_INP_SELECT, 
	MPMGG_KEY_INP_START, 
	MPMGG_KEY_INP_TR, 
	MPMGG_KEY_INP_X, 
	MPMGG_KEY_INP_Y, 
	MPMGG_KEY_INP_A, 
	MPMGG_KEY_INP_B, 
	0 
};

static int mpmggpad_btn[]	= { 
	BTN_TOP2, 
	BTN_BASE3, 
	BTN_BASE4, 
	BTN_PINKIE, 
	BTN_TRIGGER, 
	BTN_TOP, 
	BTN_THUMB, 
	BTN_THUMB2,
	0
};

static int mpmggpad_axis[]	= { 
	ABS_X,//ABS_HAT0Y,
	ABS_Y,//ABS_HAT0X,
	0
};


//extern struct mpmgg_mutex;
	
struct mpmggpad_mod {
	struct input_dev *pad;
	struct spi_device *disp;
	char phys[64];
	struct i2c_client *mcp23017;
	struct mutex *i2c_lock;
	struct mutex *call_lock;
	struct mutex *spi_lock;
	struct mutex *spi_write_lock;
	struct timer_list timer;
	char poll;
	struct task_struct   *thread;
	int old;
	int data;
};

static int poll_Ops(void * data);
static void pollTimerHandler(unsigned long data);

int setdir_mcp23017(struct i2c_client* expander, int confreg, int value);
int write_mcp23017(struct i2c_client* expander, int reg, int value);
int gpio_read_mcp23017(struct i2c_client* expander);
int read_mcp23017(struct i2c_client* expander, int reg);


static int mpmggpad_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int mpmggpad_remove(struct i2c_client *client);
static int mpmggpad_open(struct input_dev *dev);
static void mpmggpad_close(struct input_dev *dev);


static const struct i2c_device_id mpmggpad_id[] = {
	{ PAD_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mpmggpad_id);

//I²C module struct
static struct i2c_driver mpmggpad_driver = {
	.driver ={
		.name	= PAD_NAME,
		.owner	= THIS_MODULE,
	},
	.probe	= mpmggpad_probe,
	.remove	= mpmggpad_remove,
	.id_table = mpmggpad_id,
};
//module_i2c_driver(mpmggpad_driver);

static void __exit mcp23017_module_exit(void);
static int __init mcp23017_module_init(void);

#endif /* EXPANDERLIB_H */