#ifndef _EXCHLIB_
#define _EXCHLIB_

#include <linux/spinlock.h>
#include <linux/spi/spi.h>
#include "mpmgg-st7735.h"

#define PAD_NAME			"mpmgg-pad"
#define PAD_DISPLAY_NAME	"mpmgg-display"
#define MAX_PADS			4	//HW-Limit is 8

struct mpmgg_mutex {
	struct mutex *i2c_lock;
	struct mutex *i2c_call_lock;
	struct mutex *spi_lock;
	struct mutex *spi_write_lock;
	int spi_cs;
	struct spi_device *spi_dev;
};

struct mpmgg_display {
	struct spi_device *spi_dev;
	struct i2c_client *i2c_dev;
	struct mutex *spi_lock;
	struct mutex *spi_write_lock;
	int spi_cs;
	void (*display_BL)(struct i2c_client*, unsigned char);
	void (*display_DC)(struct i2c_client*, unsigned char);
	void (*display_CS)(struct i2c_client*, unsigned char);
	void (*display_DC_CS)(struct i2c_client*, unsigned char, unsigned char);
};


extern void MPMGG_display_BL(struct i2c_client* expander, unsigned char value);
extern void MPMGG_display_DC(struct i2c_client* expander, unsigned char value);
extern void MPMGG_display_CS(struct i2c_client* expander, unsigned char value);
extern void MPMGG_display_DC_CS(struct i2c_client* expander, unsigned char dc, unsigned char cs);
extern int *MPMGG_DISP_get_initArray(void);

//extern int st7735r_write_spi(struct st7735r_par *par, void *buf, size_t len);

extern int MPMGG_st7735_load(void);
extern int MPMGG_pad_load(void);

#endif /* EXCHLIB_H */

