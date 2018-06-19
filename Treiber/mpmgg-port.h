/*
 * Creative Labs Blaster GamePad Cobra driver for Linux
 */

/* nach art http://elixir.free-electrons.com/linux/latest/source/drivers/i2c/busses/i2c-xiic.c#L814
 * und https://www.kernel.org/doc/Documentation/driver-model/platform.txt
 */

#ifndef _PORTLIB_
#define _PORTLIB_

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>			/* kzalloc */
#include "mpmgg-xchange.h"

#define DRIVER_DESC	"Multi Player Multi Goal Gaming Port Controll Driver"
#define PORT_DRIVER_NAME	"mpmgg-port"

MODULE_AUTHOR("Patrick Schweinsberg <p.schweinsberg@student.uni-kassel.de>");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL"); 

#define GPIO_RESET		17
#define RESETTIME		20

struct mpmggport_mod {
	struct i2c_client **pad;
	struct mutex i2c_lock;
	struct mutex i2c_call_lock;
	struct mutex spi_lock;
	struct mutex spi_write_lock;
};


//static struct i2c_board_info __initdata board_info[] =  
static struct i2c_board_info board_info[] = {
	{
		I2C_BOARD_INFO(PAD_NAME, 0x20),
		.platform_data = NULL,
	},{
		I2C_BOARD_INFO(PAD_NAME, 0x21),
		.platform_data = NULL,
	},{
		I2C_BOARD_INFO(PAD_NAME, 0x22),
		.platform_data = NULL,
	},{
		I2C_BOARD_INFO(PAD_NAME, 0x23),
		.platform_data = NULL,
	},{
		I2C_BOARD_INFO(PAD_NAME, 0x24),
		.platform_data = NULL,
	},{
		I2C_BOARD_INFO(PAD_NAME, 0x25),
		.platform_data = NULL,
	},{
		I2C_BOARD_INFO(PAD_NAME, 0x26),
		.platform_data = NULL,
	},{
		I2C_BOARD_INFO(PAD_NAME, 0x27),
		.platform_data = NULL,
	}
};


//struct mpmggport_mod mpmggport_data = {
//	.pad={NULL,NULL,NULL,NULL}
//};


static int mpmggport_driver_probe(struct platform_device *pdev);
static int mpmggport_driver_suspend(struct device *dev);
static int mpmggport_driver_resume(struct device *dev);
static int mpmggport_driver_remove(struct platform_device *pdev);


static struct platform_device mpmggport_dev = {
	.name				=	PORT_DRIVER_NAME,
	.id 				= PLATFORM_DEVID_NONE,
//	.dev.platform_data	= &mpmggport_data,
};

static const struct dev_pm_ops mpmggport_ops = {
    .suspend	= mpmggport_driver_suspend,
    .resume		= mpmggport_driver_resume,
};

static struct platform_driver mpmggport_driver = {
    .probe      = mpmggport_driver_probe,
    .remove     = mpmggport_driver_remove,
    .driver     = {
        .name	= PORT_DRIVER_NAME,
        .owner	= THIS_MODULE,
        .pm		= &mpmggport_ops,
    },
};

static int __init mpmggport_init(void);
static void __exit mpmggport_exit(void);

module_init(mpmggport_init);
module_exit(mpmggport_exit);


#define GPIO_OUTPUT		  0
#define GPIO_INPUT		  1

#define GPIO_ON		  	  1
#define GPIO_OFF		  0

#define DEBOUNCE_TIME	150

int setGPIOFunction(int GPIO, int functionCode);
void freeGPIO(int GPIO);
void setGPIOOutputValue(int GPIO, int outputValue);


int st7735r_init_all(struct i2c_client *base, struct spi_device *spi, int *init_sequence,struct mpmgg_mutex *import);
void mcp23017_init_all(struct i2c_client *base);
void all_i2c (struct i2c_client *base, u8, u8);

#endif /* PORTLIB_H */