#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
//#include <linux/backlight.h>
//#include "mpmgg-xchange.h"
#include "mpmgg-st7735.h"

static bool dma;
module_param(dma, bool, 0);
MODULE_PARM_DESC(dma, "Use DMA buffer");

/*
 * Copyright (C) 2013 Noralf Tronnes
 *
 * This driver is inspired by:
 *   st7735fb.c, Copyright (C) 2011, Matt Porter
 *   broadsheetfb.c, Copyright (C) 2008, Jaya Kumar
 *
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
 
/*	
void st7735r_register_backlight(struct st7735r_par *par) {
	struct backlight_device *bd;
	struct backlight_properties bl_props = { 0, };
	struct backlight_ops *bl_ops;
	
	par->fbtftops.unregister_backlight = st7735r_unregister_backlight;

	if(!par->import.display_BL)
		printk(KERN_ERR PAD_DISPLAY_NAME ": FKT_CHECK: No display_BL\n");
		return
	}

	bl_ops = devm_kzalloc(par->info->device, sizeof(struct backlight_ops),
				GFP_KERNEL);
	if (!bl_ops) {
		printk(KERN_ERR PAD_DISPLAY_NAME ": Can't register backlight ops\n");
		return;
	}

	bl_ops->get_brightness = st7735r_backlight_get_brightness;
	bl_ops->update_status = st7735r_backlight_update_status;
	
	bl_props.type = BACKLIGHT_RAW;
	bl_props.power = FB_BLANK_POWERDOWN;
	
	bl_props.state = BL_CORE_DRIVER1;
	par->import.display_BL(par->import.i2c_dev, 1);
	
	bd = backlight_device_register(
				dev_driver_string(par->info->device),
				par->info->device, par, bl_ops, &bl_props);
				
				
	if (IS_ERR(bd)) {
		printk(KERN_ERR PAD_DISPLAY_NAME ": Can't register backlight\n");
		return;
	}
	
	//par->info->bl_dev = bd;
};

void st7735r_unregister_backlight(struct st7735r_par *par) { 
	const struct backlight_ops *bl_ops;
	par->import.display_BL(par->import.i2c_dev, 0);

	if (par->info->bl_dev) {
		par->info->bl_dev->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(par->info->bl_dev);
		bl_ops = par->info->bl_dev->ops;
		backlight_device_unregister(par->info->bl_dev);
		par->info->bl_dev = NULL;
	}
};

int fbtft_backlight_update_status(struct backlight_device *bd)
{
	struct st7735r_par *par = bl_get_data(bd);
	bool polarity = !!(bd->props.state & BL_CORE_DRIVER1);

	if ((bd->props.power == FB_BLANK_UNBLANK) && (bd->props.fb_blank == FB_BLANK_UNBLANK))
		par->import.display_BL(par->import.i2c_dev, polarity);
	else
		par->import.display_BL(par->import.i2c_dev, !polarity);
	return 0;
}



EXPORT_SYMBOL(st7735r_register_backlight);
EXPORT_SYMBOL(st7735r_unregister_backlight);
*/

static spinlock_t probe_disp_lock;

static int OK = 1;

/* default init sequences */
static int st7735r_init[] = {
	
	-1, SWRESET,					//  1: Software reset     
	-2, 150,						//		150 ms delay
	-1, SLPOUT,						//  2: Sleep out & booster on
	-2, 500,						//		500 ms delay
	-1, FRMCTR1, 0x01, 0x2C, 0x2D,	//  3: Frame rate: (normal) = fosc / (1 x 2 + 40) * (LINE + 2C + 2D)
	-1, FRMCTR2, 0x01, 0x2C, 0x2D, 	//  4: Frame rate: (idle)   = fosc / (1 x 2 + 40) * (LINE + 2C + 2D)
	-1, FRMCTR3,					//  5: Frame rate control - partial mode dot inversion mode, line inversion mode
	  0x01, 0x2C, 0x2D, 
	  0x01, 0x2C, 0x2D,	
	-1, INVCTR, 0x07,				//  6: Display inversion control no inversion
	-1, PWCTR1, 0xA2, 0x02, 0x84,	//  7: Power Control: -4.6V, AUTO mode
	-1, PWCTR2, 0xC5,				//  8: Power Control: VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
	-1, PWCTR3, 0x0A, 0x00,			//  9: Power Control: OpAmp current small, Boost frequency
	-1, PWCTR4, 0x8A,0x2A,			// 10: Power Control: BCLK/2, Opamp current small & Medium low 
	-1, PWCTR5, 0x8A, 0xEE,			// 11: Power Control
	-1, VMCTR1, 0x0E,				// 12: Power Control
	-1, INVOFF,						// 13: Display inversion off
	-1, COLMOD, 0x05,				// 14: Interface pixel format
	-1, DISPON,						// 15: Display On
	-2, 100,						//		100 ms delay
	-1, NORON,						// 16: Partial off (Normal)
	-2, 10,							//		10 ms delay
	-3            
};


void st7735r_reset(struct st7735r_par *par){
	printk(KERN_INFO  PAD_DISPLAY_NAME ": Software reset");
	par->st7735rops.write_register(par, 1, SWRESET);
	msleep(120);
}


/*  Gamma string format  */
#define CURVE(num, idx)  curves[num*par->gamma.num_values + idx]
static int set_gamma(struct st7735r_par *par, unsigned long *curves){
	int i,j;

	/* apply mask */
	for (i = 0; i < par->gamma.num_curves; i++)
		for (j = 0; j < par->gamma.num_values; j++)
			CURVE(i,j) &= 0b111111;

	for (i = 0; i < par->gamma.num_curves; i++)
		write_reg(par, 0xE0 + i,
			CURVE(i, 0), CURVE(i, 1), CURVE(i, 2), CURVE(i, 3),
			CURVE(i, 4), CURVE(i, 5), CURVE(i, 6), CURVE(i, 7),
			CURVE(i, 8), CURVE(i, 9), CURVE(i, 10), CURVE(i, 11),
			CURVE(i, 12), CURVE(i, 13), CURVE(i, 14), CURVE(i,15));

	return 0;
}
#undef CURVE


#define MY 							(1 << 7)
#define MX 							(1 << 6)
#define MV 							(1 << 5)

static void set_addr_win(struct st7735r_par *par, int xs, int ys, int xe, int ye){
	
	printk(KERN_INFO  PAD_DISPLAY_NAME ": (xs=%d, ys=%d, xe=%d, ye=%d)\n", xs, ys, xe, ye);
	/* Column address */
	xs+=2;
	xe+=2;
	ys+=1;
	ye+=1;
	write_reg(par, 0x2A, xs >> 8, xs & 0xFF, xe >> 8, xe & 0xFF);
	/* Row adress */
	write_reg(par, 0x2B, ys >> 8, ys & 0xFF, ye >> 8, ye & 0xFF);
	/* Memory write */
	write_reg(par, 0x2C);
}


static int set_var(struct st7735r_par *par){

	/* MADCTL - Memory data access control
	     RGB/BGR:
	     1. Mode selection pin SRGB
	        RGB H/W pin for color filter setting: 0=RGB, 1=BGR
	     2. MADCTL RGB bit
	        RGB-BGR ORDER color filter panel: 0=RGB, 1=BGR */
	switch (par->info->var.rotate) {
	case 270:
		write_reg(par, 0x36, MY | MV | (par->bgr << 3));
		break;
	case 180:
		write_reg(par, 0x36, (par->bgr << 3));
		break;
	case 90:
		write_reg(par, 0x36, MX | MV | (par->bgr << 3));
		break;
		break;
	default :
		write_reg(par, 0x36, MX | MY | (par->bgr << 3));
		break;
	}

	return 0;
}


static struct st7735r_display raw_display = { 
	.width = 128,
	.height = 160,
	.regwidth = 8,
	.buswidth = 8,
	.backlight = 1,
	.st7735rops = {
		.set_addr_win = set_addr_win,
		.set_var = set_var,
		.set_gamma = set_gamma,
	},
	.bpp = 16,
	.fps = 20,
	.rotate = 0,
	.init_sequence = st7735r_init,
	.gamma_num = 2,
	.gamma_len = 16,
	.gamma = DEFAULT_GAMMA,
};

void st7735r_update_display(struct st7735r_par *par, unsigned start_line, unsigned end_line){
	size_t offset, len;
	struct timespec ts_start, ts_end, ts_fps, ts_duration;
	long fps_ms, fps_us, duration_ms, duration_us;
	long fps, throughput;
	bool timeit = false;
	int ret = 0;

	/* Sanity checks */
	if (start_line > end_line) {
		printk(KERN_WARNING  PAD_DISPLAY_NAME ": start_line=%u is larger than end_line=%u. Shouldn't happen, will do full display update\n",
			 start_line, end_line);
		start_line = 0;
		end_line = par->info->var.yres - 1;
	}
	if (start_line > par->info->var.yres - 1 || end_line > par->info->var.yres - 1) {
		printk(KERN_WARNING  PAD_DISPLAY_NAME ": start_line=%u or end_line=%u is larger than max=%d. Shouldn't happen, will do full display update\n",
			 start_line, end_line, par->info->var.yres - 1);
		start_line = 0;
		end_line = par->info->var.yres - 1;
	}

	printk(KERN_INFO PAD_DISPLAY_NAME ":(start_line=%u, end_line=%u)\n",
		start_line, end_line);

	if (par->st7735rops.set_addr_win)
		par->st7735rops.set_addr_win(par, 0, start_line,
				par->info->var.xres-1, end_line);

	offset = start_line * par->info->fix.line_length;
	len = (end_line - start_line + 1) * par->info->fix.line_length;
	ret = par->st7735rops.write_vmem(par, offset, len);
	if (ret < 0)
		printk(KERN_ERR PAD_DISPLAY_NAME ": write_vmem failed to update display buffer\n");

	if (unlikely(timeit)) {
		getnstimeofday(&ts_end);
		if (par->update_time.tv_nsec == 0 && par->update_time.tv_sec == 0) {
			par->update_time.tv_sec = ts_start.tv_sec;
			par->update_time.tv_nsec = ts_start.tv_nsec;
		}
		ts_fps = timespec_sub(ts_start, par->update_time);
		par->update_time.tv_sec = ts_start.tv_sec;
		par->update_time.tv_nsec = ts_start.tv_nsec;
		fps_ms = (ts_fps.tv_sec * 1000) + ((ts_fps.tv_nsec / 1000000) % 1000);
		fps_us = (ts_fps.tv_nsec / 1000) % 1000;
		fps = fps_ms * 1000 + fps_us;
		fps = fps ? 1000000 / fps : 0;

		ts_duration = timespec_sub(ts_end, ts_start);
		duration_ms = (ts_duration.tv_sec * 1000) + ((ts_duration.tv_nsec / 1000000) % 1000);
		duration_us = (ts_duration.tv_nsec / 1000) % 1000;
		throughput = duration_ms * 1000 + duration_us;
		throughput = throughput ? (len * 1000) / throughput : 0;
		throughput = throughput * 1000 / 1024;

		printk(KERN_INFO  PAD_DISPLAY_NAME ": Display update: %ld kB/s (%ld.%.3ld ms), fps=%ld (%ld.%.3ld ms)\n",
			throughput, duration_ms, duration_us, fps, fps_ms, fps_us);
		par->first_update_done = true;
	}
}


void st7735r_mkdirty(struct fb_info *info, int y, int height){
	struct st7735r_par *par = info->par;
	struct fb_deferred_io *fbdefio = info->fbdefio;

	/* special case, needed ? */
	if (y == -1) {
		y = 0;
		height = info->var.yres - 1;
	}

	/* Mark display lines/area as dirty */
	spin_lock(&par->dirty_lock);
	if (y < par->dirty_lines_start)
		par->dirty_lines_start = y;
	if (y + height - 1 > par->dirty_lines_end)
		par->dirty_lines_end = y + height - 1;
	spin_unlock(&par->dirty_lock);

	/* Schedule deferred_io to update display (no-op if already on queue)*/
	schedule_delayed_work(&info->deferred_work, fbdefio->delay);
}


void st7735r_deferred_io(struct fb_info *info, struct list_head *pagelist){
	struct st7735r_par *par = info->par;
	unsigned dirty_lines_start, dirty_lines_end;
	struct page *page;
	unsigned long index;
	unsigned y_low = 0, y_high = 0;
	int count = 0;

	spin_lock(&par->dirty_lock);
	dirty_lines_start = par->dirty_lines_start;
	dirty_lines_end = par->dirty_lines_end;
	// set display line markers as clean 
	par->dirty_lines_start = par->info->var.yres - 1;
	par->dirty_lines_end = 0;
	spin_unlock(&par->dirty_lock);

	// Mark display lines as dirty 
	list_for_each_entry(page, pagelist, lru) {
		count++;
		index = page->index << PAGE_SHIFT;
		y_low = index / info->fix.line_length;
		y_high = (index + PAGE_SIZE - 1) / info->fix.line_length;
		//printk(KERN_WARNING  PAD_DISPLAY_NAME ": page->index=%lu y_low=%d y_high=%d\n", page->index, y_low, y_high);
		if (y_high > info->var.yres - 1)
			y_high = info->var.yres - 1;
		if (y_low < dirty_lines_start)
			dirty_lines_start = y_low;
		if (y_high > dirty_lines_end)
			dirty_lines_end = y_high;
	}

	par->st7735rops.update_display(info->par,
					dirty_lines_start, dirty_lines_end);
}


/* from pxafb.c */
unsigned int chan_to_field(unsigned chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

int st7735r_fb_setcolreg(unsigned regno,
			       unsigned red, unsigned green, unsigned blue,
			       unsigned transp, struct fb_info *info){
	//struct st7735r_par *par = info->par;
	unsigned val;
	int ret = 1;

	printk(KERN_INFO  PAD_DISPLAY_NAME ": (regno=%u, red=0x%X, green=0x%X, blue=0x%X, trans=0x%X)\n",
		regno, red, green, blue, transp);

	switch (info->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		if (regno < 16) {
			u32 *pal = info->pseudo_palette;

			val  = chan_to_field(red,   &info->var.red);
			val |= chan_to_field(green, &info->var.green);
			val |= chan_to_field(blue,  &info->var.blue);

			pal[regno] = val;
			ret = 0;
		}
		break;

	}
	return ret;
}

int st7735r_fb_blank(int blank, struct fb_info *info)
{
	struct st7735r_par *par = info->par;
	int ret = -EINVAL;

	printk(KERN_INFO  PAD_DISPLAY_NAME ":(blank=%d)\n",
		blank);

	if (!par->st7735rops.blank)
		return ret;

	switch (blank) {
	case FB_BLANK_POWERDOWN:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
		ret = par->st7735rops.blank(par, true);
		break;
	case FB_BLANK_UNBLANK:
		ret = par->st7735rops.blank(par, false);
		break;
	}
	return ret;
}


/**
 * st7735r_framebuffer_alloc - creates a new frame buffer info structure
 *
 * @display: pointer to structure describing the display
 * @dev: pointer to the device for this fb, this can be NULL
 *
 * Creates a new frame buffer info structure.
 *
 * Also creates and populates the following structures:
 *   info->fbops
 *   info->fbdefio
 *   info->pseudo_palette
 *   par->st7735rops
 *   par->txbuf
 *
 * Returns the new structure, or NULL if an error occurred.
 *
 */
struct fb_info *st7735r_framebuffer_alloc(struct st7735r_display *display, struct device *dev){
	struct fb_info *info;
	struct st7735r_par *par;
	struct fb_ops *fbops = NULL;
	struct fb_deferred_io *fbdefio = NULL;
	struct st7735r_platform_data *pdata = dev->platform_data;
	u8 *vmem = NULL;
	void *txbuf = NULL;
	void *buf = NULL;
	int vmem_size; //i;
	int *init_sequence = display->init_sequence;
	char *gamma = display->gamma;
	unsigned long *gamma_curves = NULL;

	/* sanity check */
	printk(KERN_ERR PAD_DISPLAY_NAME ": NEW Display\n");
	
	if (display->gamma_num * display->gamma_len > FBTFT_GAMMA_MAX_VALUES_TOTAL) {
		printk(KERN_ERR PAD_DISPLAY_NAME ": FBTFT_GAMMA_MAX_VALUES_TOTAL=%d is exceeded by %d*%d=%d\n",
			FBTFT_GAMMA_MAX_VALUES_TOTAL,
			display->gamma_num, display->gamma_len, display->gamma_num * display->gamma_len);
		return NULL;
	}

	/* defaults */
	if (!pdata) {
		printk(KERN_ERR PAD_DISPLAY_NAME ": platform data is missing\n");
		return NULL;
	}


	//display->debug |= debug;
	//st7735r_expand_debug_value(&display->debug);
		
	printk(KERN_INFO PAD_DISPLAY_NAME ": alloc vmen\n");
	vmem_size = display->width * display->height * display->bpp / 8;
	vmem = vzalloc(vmem_size);
	if (!vmem)
		goto alloc_fail;

	fbops = devm_kzalloc(dev, sizeof(struct fb_ops), GFP_KERNEL);
	if (!fbops)
		goto alloc_fail;
	fbdefio = devm_kzalloc(dev, sizeof(struct fb_deferred_io), GFP_KERNEL);
	if (!fbdefio)
		goto alloc_fail;

	buf = devm_kzalloc(dev, 128, GFP_KERNEL);
	if (!buf)
		goto alloc_fail;

	if (display->gamma_num && display->gamma_len) {
		gamma_curves = devm_kzalloc(dev, display->gamma_num * display->gamma_len * sizeof(gamma_curves[0]),
						GFP_KERNEL);
		if (!gamma_curves)
			goto alloc_fail;
	}

	printk(KERN_INFO PAD_DISPLAY_NAME ": alloc framebuffer\n");
	info = framebuffer_alloc(sizeof(struct st7735r_par), dev);
	if (!info)
		goto alloc_fail;
	
	
	printk(KERN_INFO PAD_DISPLAY_NAME ": Fill structs\n");
	info->screen_base = (u8 __force __iomem *)vmem;
	info->fbops = fbops;
	info->fbdefio = fbdefio;

	fbops->owner        =      dev->driver->owner;
	fbops->fb_read      =      fb_sys_read;
	fbops->fb_write     =      st7735r_fb_write;
	fbops->fb_fillrect  =      st7735r_fb_fillrect;
	fbops->fb_copyarea  =      st7735r_fb_copyarea;
	fbops->fb_imageblit =      st7735r_fb_imageblit;
	fbops->fb_setcolreg =      st7735r_fb_setcolreg;
	fbops->fb_blank     =      st7735r_fb_blank;

	fbdefio->delay =           HZ/display->fps;
	fbdefio->deferred_io =     st7735r_deferred_io;

	strncpy(info->fix.id, dev->driver->name, 16);
	info->fix.type =           FB_TYPE_PACKED_PIXELS;
	info->fix.visual =         FB_VISUAL_TRUECOLOR;
	info->fix.xpanstep =	   0;
	info->fix.ypanstep =	   0;
	info->fix.ywrapstep =	   0;
	info->fix.accel =          FB_ACCEL_NONE;
	info->fix.smem_len =       vmem_size;
	info->var.rotate =         display->rotate;
	
	fb_deferred_io_init(info);

	switch (display->rotate) {
		case 90:
		case 270:
			info->var.xres =           display->height;
			info->var.yres =           display->width;
			info->fix.line_length =    display->height * display->bpp /8;
			break;
		default:
			info->var.xres =           display->width;
			info->var.yres =           display->height;
			info->fix.line_length =    display->width * display->bpp /8;
	}
	
	info->var.xres_virtual =   info->var.xres;
	info->var.yres_virtual =   info->var.yres;
	info->var.bits_per_pixel = display->bpp;
	info->var.nonstd =         1;

	/* RGB565 */
	info->var.red.offset =     11;
	info->var.red.length =     5;
	info->var.green.offset =   5;
	info->var.green.length =   6;
	info->var.blue.offset =    0;
	info->var.blue.length =    5;
	info->var.transp.offset =  0;
	info->var.transp.length =  0;

	info->flags =              FBINFO_FLAG_DEFAULT | FBINFO_VIRTFB;

	par = info->par;
	par->info = info;
	par->pdata = dev->platform_data;
	
	par->buf = buf;
	spin_lock_init(&par->dirty_lock);
	par->init_sequence = init_sequence;
	par->gamma.curves = gamma_curves;
	par->gamma.num_curves = display->gamma_num;
	par->gamma.num_values = display->gamma_len;
	mutex_init(&par->gamma.lock);
	info->pseudo_palette = par->pseudo_palette;

	if (par->gamma.curves && gamma) {
		if (st7735r_gamma_parse_str(par,
			par->gamma.curves, gamma, strlen(gamma)))
			goto alloc_fail;
	}

	/* Transmit buffer */
	if (display->txbuflen == -1)
		display->txbuflen = vmem_size + 2; /* add in case startbyte is used */

#ifdef __LITTLE_ENDIAN
	if ((!display->txbuflen) && (display->bpp > 8))
		display->txbuflen = PAGE_SIZE; /* need buffer for byteswapping */
#endif

	if (display->txbuflen > 0) {
		if (dma) {
			dev->coherent_dma_mask = ~0;
			txbuf = dmam_alloc_coherent(dev, display->txbuflen, &par->txbuf.dma, GFP_DMA);
		} else {
			txbuf = devm_kzalloc(par->info->device, display->txbuflen, GFP_KERNEL);
		}
		if (!txbuf)
			goto alloc_fail;
		par->txbuf.buf = txbuf;
		par->txbuf.len = display->txbuflen;
	}
	
	printk(KERN_INFO PAD_DISPLAY_NAME ": def ops\n");

	/* default st7735r operations */
	par->st7735rops.write = st7735r_write_spi;
	par->st7735rops.read = st7735r_read_spi;
	par->st7735rops.write_vmem = st7735r_write_vmem16_bus8;
	par->st7735rops.write_register = st7735r_write_reg8_bus8;
	par->st7735rops.set_addr_win = set_addr_win;
	par->st7735rops.reset = st7735r_reset;
	par->st7735rops.mkdirty = st7735r_mkdirty;
	par->st7735rops.update_display = st7735r_update_display;
	par->st7735rops.init_display = st7735r_init_display;
	par->st7735rops.set_var= set_var;
	//if (display->backlight)
	//	par->st7735rops.register_backlight = st7735r_register_backlight;

	/* use driver provided functions */
	if (display->st7735rops.set_gamma)
		par->st7735rops.set_gamma = display->st7735rops.set_gamma;
	
	

	return info;

alloc_fail:
	vfree(vmem);
	printk(KERN_ERR PAD_DISPLAY_NAME ": FAILES TO ALLOC\n");
	msleep(2500);
	return NULL;
}
EXPORT_SYMBOL(st7735r_framebuffer_alloc);

/**
 * st7735r_framebuffer_release - frees up all memory used by the framebuffer
 *
 * @info: frame buffer info structure
 *
 */
void st7735r_framebuffer_release(struct fb_info *info){
	fb_deferred_io_cleanup(info);
	vfree(info->screen_base);
	framebuffer_release(info);
}
EXPORT_SYMBOL(st7735r_framebuffer_release);

/**
 *	st7735r_register_framebuffer - registers a tft frame buffer device
 *	@fb_info: frame buffer info structure
 *
 *  Sets SPI driverdata if needed
 *  Requests needed gpios.
 *  Initializes display
 *  Updates display.
 *	Registers a frame buffer device @fb_info.
 *
 *	Returns negative errno on error, or zero for success.
 *
 */
int st7735r_register_framebuffer(struct fb_info *fb_info){
	int ret;
	char membuf_text[50] = "";
	char spi_text[50] = "";
	char pdev_text[50] = "";
	struct st7735r_par *par = fb_info->par;
	struct spi_device *spi = par->spi;

	/* sanity checks */
	if (!par->st7735rops.init_display) {
		printk(KERN_ERR PAD_DISPLAY_NAME ": missing st7735rops.init_display()\n");
		return -EINVAL;
	}

	if (par->pdev)
		platform_set_drvdata(par->pdev, fb_info);
	else
		spi_set_drvdata(spi, fb_info);

	printk(KERN_INFO PAD_DISPLAY_NAME ": INIT_DISPLAY\n");
	ret = par->st7735rops.init_display(par);
	if (ret < 0)
		goto reg_fail;
	
	ret = par->st7735rops.set_var(par);
		if (ret < 0)
			goto reg_fail;
		
	
	/* update the entire display */
	par->st7735rops.update_display(par, 0, par->info->var.yres - 1);

	
	ret = par->st7735rops.set_gamma(par, par->gamma.curves);
	if (ret)
		goto reg_fail;

	if (par->st7735rops.register_backlight)
		par->st7735rops.register_backlight(par);

	ret = register_framebuffer(fb_info);
	if (ret < 0)
		goto reg_fail;

	st7735r_sysfs_init(par);

	if (par->txbuf.buf)
		sprintf(membuf_text, ", %d KiB %sbuffer memory",
			par->txbuf.len >> 10, par->txbuf.dma ? "DMA " : "");
	if (spi)
		sprintf(spi_text, ", spi%d.%d at %d MHz", spi->master->bus_num, spi->chip_select, spi->max_speed_hz/1000000);
	if (par->pdev)
		sprintf(pdev_text, ", devNo=%d", par->mpmgg_display->spi_cs);
	
	printk(KERN_INFO PAD_DISPLAY_NAME ": frame buffer, %dx%d, %d KiB video memory%s, fps=%lu%s%s\n",
		fb_info->var.xres, fb_info->var.yres,
		fb_info->fix.smem_len >> 10, membuf_text,
		HZ/fb_info->fbdefio->delay, spi_text, pdev_text);

	return 0;

reg_fail:
	if (par->st7735rops.unregister_backlight)
		par->st7735rops.unregister_backlight(par);
	if (par->pdev)
		platform_set_drvdata(par->pdev, NULL);
	else
		spi_set_drvdata(spi, NULL);

	return ret;
}


/**
 *	st7735r_unregister_framebuffer - releases a tft frame buffer device
 *	@fb_info: frame buffer info structure
 *
 *  Frees SPI driverdata if needed
 *  Frees gpios.
 *	Unregisters frame buffer device.
 *
 */
int st7735r_unregister_framebuffer(struct fb_info *fb_info){
	struct st7735r_par *par = fb_info->par;
	struct spi_device *spi = par->spi;
	int ret;

	if (spi)
		spi_set_drvdata(spi, NULL);
	if (par->pdev)
		platform_set_drvdata(par->pdev, NULL);
	if (par->st7735rops.unregister_backlight)
		par->st7735rops.unregister_backlight(par);
	st7735r_sysfs_exit(par);
	ret = unregister_framebuffer(fb_info);
	return ret;
}
EXPORT_SYMBOL(st7735r_unregister_framebuffer);


/**
 * st7735r_init_display() - Generic init_display() function
 * @par: Driver data
 *
 * Uses par->init_sequence to do the initialization
 *
 * Return: 0 if successful, negative if error
 */
int st7735r_init_display(struct st7735r_par *par){
	//int buf[64];
	//char msg[128];
	//char str[16];
	int i = 0;
	//int j;

	/* sanity check */
	if (!par->init_sequence) {
		printk(KERN_ERR PAD_DISPLAY_NAME ": error: init_sequence is not set\n");
		return -EINVAL;
	}

	/* make sure stop marker exists */
	for (i = 0; i < FBTFT_MAX_INIT_SEQUENCE; i++)
		if (par->init_sequence[i] == -3)
			break;
	if (i == FBTFT_MAX_INIT_SEQUENCE) {
		printk(KERN_ERR PAD_DISPLAY_NAME ": missing stop marker at end of init sequence\n");
		return -EINVAL;
	}

	/*
	par->st7735rops.reset(par);
	
	//if (par->import.display_CS)
	//	par->import.display_CS(false);
	printk(KERN_INFO PAD_DISPLAY_NAME ": reset suceeded\n");
	par->import.display_CS(par->import.i2c_dev , 0);

	i = 0;
	printk(KERN_INFO PAD_DISPLAY_NAME ": configuring display\n");
	while (i < FBTFT_MAX_INIT_SEQUENCE) {
		if (par->init_sequence[i] == -3) {
			// done
			return 0;
		}
		if (par->init_sequence[i] >= 0) {
			printk(KERN_ERR PAD_DISPLAY_NAME ": missing delimiter at position %d\n", i);
			return -EINVAL;
		}
		if (par->init_sequence[i+1] < 0) {
			printk(KERN_ERR PAD_DISPLAY_NAME ": missing value after delimiter %d at position %d\n",
				par->init_sequence[i], i);
			return -EINVAL;
		}
		switch (par->init_sequence[i]) {
		case -1:
			i++;
			// make debug message 
			strcpy(msg, "");
			j = i + 1;
			while (par->init_sequence[j] >= 0) {
				sprintf(str, "0x%02X ", par->init_sequence[j]);
				strcat(msg, str);
				j++;
			}
			//printk(KERN_WARNING  PAD_DISPLAY_NAME ": init: write(0x%02X) %s\n",
			//	par->init_sequence[i], msg);

			// Write 
			j = 0;
			while (par->init_sequence[i] >= 0) {
				if (j > 63) {
					printk(KERN_ERR PAD_DISPLAY_NAME ": Maximum register values exceeded\n");
					return -EINVAL;
				}
				buf[j++] = par->init_sequence[i++];
			}
			par->st7735rops.write_register(par, j,
				buf[0], buf[1], buf[2], buf[3],
				buf[4], buf[5], buf[6], buf[7],
				buf[8], buf[9], buf[10], buf[11],
				buf[12], buf[13], buf[14], buf[15],
				buf[16], buf[17], buf[18], buf[19],
				buf[20], buf[21], buf[22], buf[23],
				buf[24], buf[25], buf[26], buf[27],
				buf[28], buf[29], buf[30], buf[31],
				buf[32], buf[33], buf[34], buf[35],
				buf[36], buf[37], buf[38], buf[39],
				buf[40], buf[41], buf[42], buf[43],
				buf[44], buf[45], buf[46], buf[47],
				buf[48], buf[49], buf[50], buf[51],
				buf[52], buf[53], buf[54], buf[55],
				buf[56], buf[57], buf[58], buf[59],
				buf[60], buf[61], buf[62], buf[63]);
			break;
		case -2:
			i++;
			//printk(KERN_WARNING  PAD_DISPLAY_NAME ": init: mdelay(%d)\n", par->init_sequence[i]);
			//mdelay 
			msleep(par->init_sequence[i++]);
			//msleep_interruptible(par->init_sequence[i++]);
			break;
		default:
			printk(KERN_ERR PAD_DISPLAY_NAME ": unknown delimiter %d at position %d\n",
				par->init_sequence[i], i);
			return -EINVAL;
		}
	}*/
	return 0;

	printk(KERN_ERR PAD_DISPLAY_NAME ": The magic fairy swings the wand to get here.\n");
	return -EINVAL;
}
//EXPORT_SYMBOL(st7735r_init_display);

/**
 * st7735r_probe_common() - Generic device probe() helper function
 * @display: Display properties
 * @sdev: SPI device
 * @pdev: Platform device
 *
 * Allocates, initializes and registers a framebuffer
 *
 * Either @sdev or @pdev should be NULL
 *
 * Return: 0 if successful, negative if error
 */
int st7735r_probe_common(struct st7735r_display *display, struct spi_device *sdev, struct platform_device *pdev){
	struct device *dev;
	struct fb_info *info;
	struct st7735r_par *par;
	int ret;
	struct mpmgg_display *mux;
	printk(KERN_INFO PAD_DISPLAY_NAME ": Probing\n");

	if (sdev){
		dev = &sdev->dev;
	}else{	
		dev = &pdev->dev;
	}
	
	mux =(struct mpmgg_display*) dev->platform_data;

	info = st7735r_framebuffer_alloc(display, dev);
	if (!info)
		return -ENOMEM;

	par = info->par;
	par->pdev = pdev;
	if (sdev){
		par->spi = sdev;
	}else{	
	    par->spi = mux->spi_dev;
	}
	par->mpmgg_display = mux;
	
	if(mux->i2c_dev){
		par->import.i2c_dev		= mux->i2c_dev;
	}else{
		printk(KERN_ERR PAD_DISPLAY_NAME ": FKT_CHECK: No i2c_dev\n");
	}
	if(mux->spi_lock){
	par->import.spi_lock	= mux->spi_lock;
	}else{
		printk(KERN_ERR PAD_DISPLAY_NAME ": FKT_CHECK: No spi_lock\n");
	}
	if(mux->spi_write_lock){
	par->import.spi_write_lock	= mux->spi_write_lock;
	}else{
		printk(KERN_ERR PAD_DISPLAY_NAME ": FKT_CHECK: No spi_write_lock\n");
	}
	if(mux->display_DC){
	par->import.display_DC	= mux->display_DC;
	}else{
		printk(KERN_ERR PAD_DISPLAY_NAME ": FKT_CHECK: No display_DC\n");
	}
	if(mux->display_CS){
	par->import.display_CS	= mux->display_CS;
	}else{
		printk(KERN_ERR PAD_DISPLAY_NAME ": FKT_CHECK: No display_CS\n");
	}
	if(mux->display_DC_CS){
	par->import.display_DC_CS = mux->display_DC_CS;
	}else{
		printk(KERN_ERR PAD_DISPLAY_NAME ": FKT_CHECK: No display_DC_CS\n");
	}
	
	par->import.display_BL	= mux->display_BL;

	par->st7735rops.write_register = st7735r_write_reg8_bus8;
	par->st7735rops.write_vmem = st7735r_write_vmem16_bus8;

	
	printk(KERN_INFO PAD_DISPLAY_NAME ": Dev configured\n");
	/* make sure we still use the driver provided functions */
	if (display->st7735rops.write)
		par->st7735rops.write = display->st7735rops.write;
	if (display->st7735rops.read)
		par->st7735rops.read = display->st7735rops.read;
	if (display->st7735rops.write_vmem)
		par->st7735rops.write_vmem = display->st7735rops.write_vmem;
	if (display->st7735rops.write_register)
		par->st7735rops.write_register = display->st7735rops.write_register;
	if (display->st7735rops.set_addr_win)
		par->st7735rops.set_addr_win = display->st7735rops.set_addr_win;
	if (display->st7735rops.reset)
		par->st7735rops.reset = display->st7735rops.reset;
	if (display->st7735rops.mkdirty)
		par->st7735rops.mkdirty = display->st7735rops.mkdirty;
	if (display->st7735rops.update_display)
		par->st7735rops.update_display = display->st7735rops.update_display;
	
	
	if (display->st7735rops.set_var)
		par->st7735rops.set_var = display->st7735rops.set_var;
	
	par->st7735rops.set_gamma = display->st7735rops.set_gamma;
	printk(KERN_INFO PAD_DISPLAY_NAME ": Added Functions\n");
	
	/* use init_sequence if provided */
	if (par->init_sequence)
		par->st7735rops.init_display = st7735r_init_display;

	//turn display on
	printk(KERN_INFO PAD_DISPLAY_NAME ": turn on light\n");
	par->import.display_BL(par->import.i2c_dev, 1);
	
	printk(KERN_INFO PAD_DISPLAY_NAME ": Register Framebuffer\n");
	ret = st7735r_register_framebuffer(info);
	if (ret < 0)
		goto out_release;

	return 0;

out_release:
	st7735r_framebuffer_release(info);

	return ret;
}

/**
 * st7735r_remove_common() - Generic device remove() helper function
 * @dev: Device
 * @info: Framebuffer
 *
 * Unregisters and releases the framebuffer
 *
 * Return: 0 if successful, negative if error
 */
int st7735r_remove_common(struct device *dev, struct fb_info *info){
	if (!info)
		return -EINVAL;
	
	st7735r_unregister_framebuffer(info);
	st7735r_framebuffer_release(info);

	return 0;
}

static int st7735r_driver_probe_spi(struct spi_device *spi){
	int ret=0;
	spin_lock(&probe_disp_lock);
	printk(KERN_INFO PAD_DISPLAY_NAME ": probe via spi\n");
	ret = st7735r_probe_common(&raw_display, spi, NULL);
	spin_unlock(&probe_disp_lock);
	return ret;
}

static int st7735r_driver_remove_spi(struct spi_device *spi){
	struct fb_info *info = spi_get_drvdata(spi);
	return st7735r_remove_common(&spi->dev, info);
}


int *MPMGG_DISP_get_initArray(void){
	return &st7735r_init[0];
}
EXPORT_SYMBOL(MPMGG_DISP_get_initArray);

static int st7735r_driver_probe_pdev(struct platform_device *pdev){
	int ret=0;
	printk(KERN_INFO PAD_DISPLAY_NAME ": probe via platform_device\n");
	spin_lock(&probe_disp_lock);
	ret = st7735r_probe_common(&raw_display, NULL, pdev);
	spin_unlock(&probe_disp_lock);
	return ret;
}

static int st7735r_driver_remove_pdev(struct platform_device *pdev){
	struct fb_info *info = platform_get_drvdata(pdev);
	return st7735r_remove_common(&pdev->dev, info);
}        

static struct spi_driver st7735r_driver_spi_driver = {
	.driver = {
		.name   = PAD_DISPLAY_NAME,
		.owner  = THIS_MODULE,
	},
	.probe  = st7735r_driver_probe_spi,
	.remove = st7735r_driver_remove_spi
};

static struct platform_driver st7735r_driver_platform_driver = {
	.driver = {
		.name   = PAD_DISPLAY_NAME,
		.owner  = THIS_MODULE,
	},
	.probe  = st7735r_driver_probe_pdev,
	.remove = st7735r_driver_remove_pdev
}; 


int MPMGG_st7735_load(void)
{return OK;}
EXPORT_SYMBOL(MPMGG_st7735_load);


/*------------------------------------------------------------------*/
static int __init st7735r_driver_module_init(void){ 
	int ret = 0;
	printk(KERN_INFO PAD_DISPLAY_NAME ": Register\n");
	spin_lock_init(&probe_disp_lock);
	ret = spi_register_driver(&st7735r_driver_spi_driver);
	if (ret < 0) 
		return ret;
	ret = platform_driver_register(&st7735r_driver_platform_driver);
	if (ret < 0) 
		return ret;
	
	OK = 0;
	return 0;
}

static void __exit st7735r_driver_module_exit(void){
	spi_unregister_driver(&st7735r_driver_spi_driver);
	platform_driver_unregister(&st7735r_driver_platform_driver);
	printk(KERN_INFO PAD_DISPLAY_NAME ": Unregister\n");
}


module_init(st7735r_driver_module_init);
module_exit(st7735r_driver_module_exit);

/*------------------------------------------------------------------*/

MODULE_AUTHOR("Patrick Schweinsberg <p.schweinsberg@student.uni-kassel.de>");
MODULE_DESCRIPTION(DRIVER_DESC_D);
MODULE_LICENSE("GPL"); 
