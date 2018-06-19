#ifndef _MPMGGDISP_
#define _MPMGGDISP_

//#include <linux/slab.h>			/* kzalloc */
//#include <linux/errno.h>
//#include <linux/export.h>

//#include <linux/vmalloc.h>
#include <linux/fb.h>
#include <linux/spinlock.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include "mpmgg-xchange.h"
#include "mpmgg-st7735-control.h"



/*
#define FBTFT_NOP					   0x00
#define FBTFT_SWRESET				   0x01
#define FBTFT_RDDID					   0x04
#define FBTFT_RDDST					   0x09
#define FBTFT_CASET					   0x2A
#define FBTFT_RASET					   0x2B
#define FBTFT_RAMWR					   0x2C
*/

#define FBTFT_ONBOARD_BACKLIGHT 		  2

#define FBTFT_GPIO_NO_MATCH			 0xFFFF
#define FBTFT_GPIO_NAME_SIZE			 32
#define FBTFT_MAX_INIT_SEQUENCE      	512
#define FBTFT_GAMMA_MAX_VALUES_TOTAL 	128

#define FBTFT_OF_INIT_CMD			BIT(24)
#define FBTFT_OF_INIT_DELAY			BIT(25)

#define DEFAULT_GAMMA \
		"0F 1A 0F 18 2F 28 20 22 1F 1B 23 37 00 07 02 10\n" \
        "0F 1B 0F 17 33 2C 29 2E 30 30 39 3F 00 07 03 10"

		
struct st7735r_par; 
		
/**
 * struct st7735r_ops - FBTFT operations structure
 * @write: Writes to interface bus
 * @read: Reads from interface bus
 * @write_vmem: Writes video memory to display
 * @write_reg: Writes to controller register
 * @set_addr_win: Set the GRAM update window
 * @reset: Reset the LCD controller
 * @mkdirty: Marks display lines for update
 * @update_display: Updates the display
 * @init_display: Initializes the display
 * @blank: Blank the display (optional)
 * @request_gpios_match: Do pinname to gpio matching
 * @request_gpios: Request gpios from the kernel
 * @free_gpios: Free previously requested gpios
 * @verify_gpios: Verify that necessary gpios is present (optional)
 * @register_backlight: Used to register backlight device (optional)
 * @unregister_backlight: Unregister backlight device (optional)
 * @set_var: Configure LCD with values from variables like @rotate and @bgr
 *           (optional)
 * @set_gamma: Set Gamma curve (optional)
 *
 * Most of these operations have default functions assigned to them in
 *     st7735r_framebuffer_alloc()
 */
struct st7735r_ops {
	int (*write)(struct st7735r_par *par, void *buf, size_t len);
	int (*read)(struct st7735r_par *par, void *buf, size_t len);
	int (*write_vmem)(struct st7735r_par *par, size_t offset, size_t len);
	void (*write_register)(struct st7735r_par *par, int len, ...);

	void (*set_addr_win)(struct st7735r_par *par,
		int xs, int ys, int xe, int ye);
	void (*reset)(struct st7735r_par *par);
	void (*mkdirty)(struct fb_info *info, int from, int to);
	void (*update_display)(struct st7735r_par *par,
				unsigned start_line, unsigned end_line);
	int (*init_display)(struct st7735r_par *par);
	int (*blank)(struct st7735r_par *par, bool on);

	void (*register_backlight)(struct st7735r_par *par);
	void (*unregister_backlight)(struct st7735r_par *par);

	int (*set_var)(struct st7735r_par *par);
	int (*set_gamma)(struct st7735r_par *par, unsigned long *curves);
};


/**
 * struct st7735r_display - Describes the display properties
 * @width: Width of display in pixels
 * @height: Height of display in pixels
 * @regwidth: LCD Controller Register width in bits
 * @buswidth: Display interface bus width in bits
 * @backlight: Backlight type.
 * @st7735rops: FBTFT operations provided by driver or device (platform_data)
 * @bpp: Bits per pixel
 * @fps: Frames per second
 * @txbuflen: Size of transmit buffer
 * @init_sequence: Pointer to LCD initialization array
 * @gamma: String representation of Gamma curve(s)
 * @gamma_num: Number of Gamma curves
 * @gamma_len: Number of values per Gamma curve
 * @debug: Initial debug value
 *
 * This structure is not stored by FBTFT except for init_sequence.
 */
struct st7735r_display {
	unsigned width;
	unsigned height;
	unsigned rotate;
	unsigned regwidth;
	unsigned buswidth;
	unsigned backlight;
	struct st7735r_ops st7735rops;
	unsigned bpp;
	unsigned fps;
	int txbuflen;
	int *init_sequence;
	char *gamma;
	int gamma_num;
	int gamma_len;
};



/*
static int st7735r_init[] = {	// Initialization commands for 7735B screens
    -1, SWRESET,			//  1: Software reset
    -2, 50,					//		50 ms delay
    -1, SLPOUT,				//  2: Out of sleep mode
    -2, 500,                //		500 ms delay
    -1, COLMOD,				//  3: Set color mode:
      0x05,						//     16-bit color
    -2, 10,					//		10 ms delay
    -1, FRMCTR1,			//  4: Frame rate control:
      0x00,						//     fastest refresh
      0x06,						//     6 lines front porch
      0x03,						//     3 lines back porch
    -2, 10,					//		10 ms delay
    -1, MADCTL,				//  5: Memory access ctrl (directions):
      0x08,						//     Row addr/col addr, bottom to top refresh
    -1, DISSET5,			//  6: Display settings #5:
      0x15,						//     1 clk cycle nonoverlap, 2 cycle gate rise, 3 cycle osc equalize
      0x02,						//     Fix on VTL
    -1, INVCTR,				//  7: Display inversion control:
      0x00,						//     Line inversion
    -1, PWCTR1,				//  8: Power control:
      0x02,						//     GVDD = 4.7V
      0x70,						//     1.0uA
    -2, 10,					//		10 ms delay
    -1, PWCTR2,				//  9: Power control:
      0x05,						//     VGH = 14.7V, VGL = -7.35V
    -1, PWCTR3,				// 10: Power control:
      0x01,						//     Opamp current small
      0x02,						//     Boost frequency
    -1, VMCTR1,				// 11: Power control:
      0x3C,						//     VCOMH = 4V
      0x38,						//     VCOML = -1.1V
    -2, 10,					//		10 ms delay
    -1, PWCTR6,				// 12: Power control:
      0x11, 0x15,
    -1, GMCTRP1,			// 13: Magical unicorn dust:
      0x09, 0x16, 0x09, 0x20,	//     (seriously though, not sure what
      0x21, 0x1B, 0x13, 0x19,	//      these config values represent)
      0x17, 0x15, 0x1E, 0x2B,
      0x04, 0x05, 0x02, 0x0E,
    -1, GMCTRN1,			// 14: Sparkles and rainbows:
      0x0B, 0x14, 0x08, 0x1E,
      0x22, 0x1D, 0x18, 0x1E,
      0x1B, 0x1A, 0x24, 0x2B,
      0x06, 0x06, 0x02, 0x0F,
    -2, 10,					//		10 ms delay
    -1, CASET,				// 15: Column addr set:
      0x00, 0x02,				//     XSTART = 2
      0x00, 0x81,				//     XEND = 129
    -1, RASET,				// 16: Row addr set:
      0x00, 0x02,				//     XSTART = 1
      0x00, 0x81,				//     XEND = 160
    -1, NORON,				// 17: Normal display on
    -2, 10,					//		10 ms delay
    -1, DISPON,				// 18: Main screen turn on
    -2, 500, 				//		500 ms delay
	-3,
};                  
*/


/*------------------------------------------------------------------*/	
/* mpmgg-st7735.c */
//extern void st7735r_reset(struct st7735r_par *par);
//extern void st7735r_update_display(struct st7735r_par *par, unsigned start_line, unsigned end_line);
//extern void st7735r_mkdirty(struct fb_info *info, int y, int height);
void st7735r_deferred_io(struct fb_info *info, struct list_head *pagelist);
void st7735r_fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect);
void st7735r_fb_copyarea(struct fb_info *info, const struct fb_copyarea *area);
void st7735r_fb_imageblit(struct fb_info *info, const struct fb_image *image);
//ssize_t st7735r_fb_write(struct fb_info *info, const char __user *buf, size_t count, loff_t *ppos);
//extern unsigned int chan_to_field(unsigned chan, struct fb_bitfield *bf);
//extern int st7735r_fb_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue, unsigned transp, struct fb_info *info);
//extern int st7735r_fb_blank(int blank, struct fb_info *info);
extern struct fb_info *st7735r_framebuffer_alloc(struct st7735r_display *display, struct device *dev);
extern void st7735r_framebuffer_release(struct fb_info *info);
extern int st7735r_register_framebuffer(struct fb_info *fb_info);
extern int st7735r_unregister_framebuffer(struct fb_info *fb_info);
extern int st7735r_init_display(struct st7735r_par *par);
extern int st7735r_probe_common(struct st7735r_display *display, struct spi_device *sdev, struct platform_device *pdev);
extern int st7735r_remove_common(struct device *dev, struct fb_info *info);
//static int __init st7735r_driver_module_init(void);
//static void __exit st7735r_driver_module_exit(void);

/* mpmgg-st7735-fops.c */
//void st7735r_write_reg8_bus8(struct st7735r_par *par, int len, ...);
//int st7735r_write_vmem16_bus8(struct st7735r_par *par, size_t offset, size_t len);
extern void st7735r_write_reg8_bus8 (struct st7735r_par *par, int len, ...);
//extern void st7735r_fb_copyarea(struct fb_info *info, const struct fb_copyarea *area);
//extern void st7735r_fb_imageblit(struct fb_info *info, const struct fb_image *image);
//extern void st7735r_fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect);
extern int st7735r_write_vmem16_bus8(struct st7735r_par *par, size_t offset, size_t len);
extern ssize_t st7735r_fb_write(struct fb_info *info, const char __user *buf, size_t count, loff_t *ppos);

/* mpmgg-st7735-support.c */
//static int set_gamma(struct st7735r_par *par, unsigned long *curves);
//static int set_var(struct st7735r_par *par);
extern int st7735r_write_spi(struct st7735r_par *par, void *buf, size_t len);
extern int st7735r_read_spi(struct st7735r_par *par, void *buf, size_t len);
//static void set_addr_win(struct st7735r_par *par, int xs, int ys, int xe, int ye);
/*
int st7735r_write_gpio8_wr(struct st7735r_par *par, void *buf, size_t len);
*/
/*------------------------------------------------------------------*/
//int st7735r_write_spi(struct st7735r_par *par, void *buf, size_t len);
//int st7735r_read_spi(struct st7735r_par *par, void *buf, size_t len);
// void st7735r_set_addr_win(struct st7735r_par *par, int xs, int ys, int xe, int ye);
extern void st7735r_register_backlight(struct st7735r_par *par);
extern void st7735r_unregister_backlight(struct st7735r_par *par);


/* mpmgg-st7735-sysfs.c */
extern void st7735r_sysfs_init(struct st7735r_par *par);
extern void st7735r_sysfs_exit(struct st7735r_par *par);
extern int st7735r_gamma_parse_str(struct st7735r_par *par, unsigned long *curves, const char *str, int size);

//static int get_next_ulong(char **str_p, unsigned long *val, char *sep, int base);
//int st7735r_gamma_parse_str(struct st7735r_par *par, unsigned long *curves, const char *str, int size);
//static ssize_t sprintf_gamma(struct st7735r_par *par, unsigned long *curves, char *buf);
//static ssize_t store_gamma_curve(struct device *device, struct device_attribute *attr, const char *buf, size_t count);
//static ssize_t show_gamma_curve(struct device *device, struct device_attribute *attr, char *buf);
//void st7735r_sysfs_init(struct st7735r_par *par);
//void st7735r_sysfs_exit(struct st7735r_par *par);

/*------------------------------------------------------------------*/	


/**
 * struct st7735r_par - Main FBTFT data structure
 *
 * This structure holds all relevant data to operate the display
 *
 * See sourcefile for documentation since nested structs is not
 * supported by kernel-doc.
 *
 */
/* @spi: Set if it is a SPI device
 * @pdev: Set if it is a platform device
 * @info: Pointer to framebuffer fb_info structure
 * @pdata: Pointer to platform data
 * @ssbuf: Not used
 * @pseudo_palette: Used by fb_set_colreg()
 * @txbuf.buf: Transmit buffer
 * @txbuf.len: Transmit buffer length
 * @buf: Small buffer used when writing init data over SPI
 * @startbyte: Used by some controllers when in SPI mode.
 *             Format: 6 bit Device id + RS bit + RW bit
 * @st7735rops: FBTFT operations provided by driver or device (platform_data)
 * @dirty_lock: Protects dirty_lines_start and dirty_lines_end
 * @dirty_lines_start: Where to begin updating display
 * @dirty_lines_end: Where to end updating display
 * @gpio.reset: GPIO used to reset display
 * @gpio.dc: Data/Command signal, also known as RS
 * @gpio.rd: Read latching signal
 * @gpio.wr: Write latching signal
 * @gpio.latch: Bus latch signal, eg. 16->8 bit bus latch
 * @gpio.cs: LCD Chip Select with parallel interface bus
 * @gpio.db[16]: Parallel databus
 * @gpio.led[16]: Led control signals
 * @gpio.aux[16]: Auxillary signals, not used by core
 * @init_sequence: Pointer to LCD initialization array
 * @gamma.lock: Mutex for Gamma curve locking
 * @gamma.curves: Pointer to Gamma curve array
 * @gamma.num_values: Number of values per Gamma curve
 * @gamma.num_curves: Number of Gamma curves
 * @debug: Pointer to debug value
 * @current_debug:
 * @first_update_done: Used to only time the first display update
 * @update_time: Used to calculate 'fps' in debug output
 * @bgr: BGR mode/\n
 * @extra: Extra info needed by driver
 */
struct st7735r_par {
	struct spi_device *spi;
	struct platform_device *pdev;
	struct mpmgg_display *mpmgg_display;
	struct fb_info *info;
	struct st7735r_platform_data *pdata;
	u16 *ssbuf;
	u32 pseudo_palette[16];
	struct {
		void *buf;
		dma_addr_t dma;
		size_t len;
	} txbuf;
	u8 *buf;
	u8 startbyte;
	struct st7735r_ops st7735rops;
	spinlock_t dirty_lock;
	unsigned dirty_lines_start;
	unsigned dirty_lines_end;
	struct {
		struct i2c_client *i2c_dev;
		struct mutex *spi_lock;
		struct mutex *spi_write_lock;
		void (*display_BL)(struct i2c_client*, unsigned char);
		void (*display_DC)(struct i2c_client*, unsigned char);
		void (*display_CS)(struct i2c_client*, unsigned char);
		void (*display_DC_CS)(struct i2c_client*, unsigned char, unsigned char);
	} import;
	int *init_sequence;
	struct {
		struct mutex lock;
		unsigned long *curves;
		int num_values;
		int num_curves;
	} gamma;
	unsigned long debug;
	bool first_update_done;
	struct timespec update_time;
	bool bgr;
	void *extra;
};

#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))

#define write_reg(par, ...)                                              \
do {                                                                     \
	par->st7735rops.write_register(par, NUMARGS(__VA_ARGS__), __VA_ARGS__); \
} while (0)

        
/*------------------------------------------------------------------*/


//static int __init st7735r_driver_module_init(void);
//static void __exit st7735r_driver_module_exit(void);


#define DRIVER_DESC_D	"Multi Player Multi Goal Gaming Display Driver"

#endif