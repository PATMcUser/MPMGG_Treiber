#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
//#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include "mpmgg-st7735.h"


ssize_t st7735r_fb_write(struct fb_info *info, const char __user *buf, size_t count, loff_t *ppos){
	struct st7735r_par *par = info->par;
	ssize_t res = 0;

	printk(KERN_INFO  PAD_DISPLAY_NAME ": count=%zd, ppos=%llu\n", count, *ppos);
	res = fb_sys_write(info, buf, count, ppos);

	par->st7735rops.mkdirty(info, -1, 0);

	return res;
}
EXPORT_SYMBOL(st7735r_fb_write);

void st7735r_fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect){
	struct st7735r_par *par = info->par;

	printk(KERN_WARNING  PAD_DISPLAY_NAME ": dx=%d, dy=%d, width=%d, height=%d\n",
		rect->dx, rect->dy, rect->width, rect->height);
	sys_fillrect(info, rect);

	par->st7735rops.mkdirty(info, rect->dy, rect->height);
}

void st7735r_fb_copyarea(struct fb_info *info, const struct fb_copyarea *area){
	struct st7735r_par *par = info->par;
	sys_copyarea(info, area);


	printk(KERN_INFO  PAD_DISPLAY_NAME ": dx=%d, dy=%d, width=%d, height=%d\n",
		area->dx, area->dy, area->width, area->height);
	par->st7735rops.mkdirty(info, area->dy, area->height);
}


void st7735r_fb_imageblit(struct fb_info *info, const struct fb_image *image){
	struct st7735r_par *par = info->par;

	printk(KERN_INFO  PAD_DISPLAY_NAME ": dx=%d, dy=%d, width=%d, height=%d\n",
		image->dx, image->dy, image->width, image->height);
	sys_imageblit(info, image);

	par->st7735rops.mkdirty(info, image->dy, image->height);
}




void st7735r_write_reg8_bus8 (struct st7735r_par *par, int len, ...){                                                                             \
	va_list args;
	int i, ret;
	int offset = 0;
	u8 *buf = (u8 *)par->buf;
	
	va_start(args, len);
	
	if (par->startbyte) {
		*(u8 *)par->buf = par->startbyte;
		buf = (u8 *)(par->buf + 1);
		offset = 1;
	}
	
	*buf = ((u8)va_arg(args, unsigned int));
	
	mutex_lock(par->import.spi_lock);
	
	if (par->import.display_DC && par->import.display_CS){
		par->import.display_DC(par->import.i2c_dev , 0);
		ndelay(200);
	}else{
		printk(KERN_ERR PAD_DISPLAY_NAME ": No DC or CS to ennable\n");
		goto sendfail;
	}
	
	par->import.display_CS(par->import.i2c_dev , 0);
	ret = par->st7735rops.write(par, par->buf, sizeof(u8)+offset);
	par->import.display_CS(par->import.i2c_dev , 1);
	
	par->import.display_DC(par->import.i2c_dev , 1);
	ndelay(200);
	
	if (ret < 0) {
		va_end(args);
		printk(KERN_ERR PAD_DISPLAY_NAME ": write() failed and returned %d\n", ret);
		mutex_unlock(par->import.spi_lock);
		return;
	}
	len--;
	
	if (par->startbyte)
		*(u8 *)par->buf = par->startbyte | 0x2;
	
	if (len) {
		i = len;
		
		while (i--) {
			*buf++ = (u8)va_arg(args, unsigned int);
		}
		
		par->import.display_CS(par->import.i2c_dev , 0);
		ret = par->st7735rops.write(par, par->buf, len * (sizeof(u8)+offset));
		par->import.display_CS(par->import.i2c_dev , 1);
		/*
		while (i--) {
			*buf = (u8)va_arg(args, unsigned int);
			par->import.display_CS(par->import.i2c_dev , 0);
			ret |= par->st7735rops.write(par, par->buf, sizeof(u8)+offset);
			par->import.display_CS(par->import.i2c_dev , 1);
		}
		*/
		if (ret < 0) {
			printk(KERN_ERR PAD_DISPLAY_NAME ": write() failed and returned %d\n", ret);
		}
	}
	
	//par->import.display_CS(par->import.i2c_dev , 1);
	//ndelay(200);
sendfail:
	va_end(args);
	mutex_unlock(par->import.spi_lock);
}



/*****************************************************************************
 *
 *   int (*write_vmem)(struct st7735r_par *par);
 *
 *****************************************************************************/

/* 16 bit pixel over 8-bit databus */
int st7735r_write_vmem16_bus8(struct st7735r_par *par, size_t offset, size_t len)
{
	u16 *vmem16;
	u16 *txbuf16 = (u16 *)par->txbuf.buf;
	size_t remain;
	size_t to_copy;
	size_t tx_array_size;
	int i;
	int ret = 0;
	size_t startbyte_size = 0;

	//printk(KERN_INFO PAD_DISPLAY_NAME ": mem16_bus8\n");

	remain = len / 2;
	vmem16 = (u16 *)(par->info->screen_base + offset);
	
	mutex_lock(par->import.spi_lock);
	//if (par->import.display_DC){
	//	par->import.display_DC(par->import.i2c_dev, 1);
	// 	msleep(5);
	//}else{
	//	printk(KERN_ERR PAD_DISPLAY_NAME ": No display_DC to disable\n");
	//}
	
	if (par->import.display_CS){
		par->import.display_CS(par->import.i2c_dev, 0);
	}else{
		printk(KERN_ERR PAD_DISPLAY_NAME ": No display_CS to enable\n");
	}

	/* non buffered write */
	if (!par->txbuf.buf){
		ret =  par->st7735rops.write(par, vmem16, len);
		par->import.display_CS(par->import.i2c_dev, 1);
		mutex_unlock(par->import.spi_lock);
		return ret;
	}
	/* buffered write */
	tx_array_size = par->txbuf.len / 2;

	if (par->startbyte) {
		txbuf16 = (u16 *)(par->txbuf.buf + 1);
		tx_array_size -= 2;
		*(u8 *)(par->txbuf.buf) = par->startbyte | 0x2;
		startbyte_size = 1;
	}

	while (remain) {
		to_copy = remain > tx_array_size ? tx_array_size : remain;
		//printk(KERN_INFO PAD_DISPLAY_NAME ":    to_copy=%zu, remain=%zu\n",to_copy, remain - to_copy);

		for (i = 0; i < to_copy; i++)
			txbuf16[i] = cpu_to_be16(vmem16[i]);

		vmem16 = vmem16 + to_copy;
		ret = par->st7735rops.write(par, par->txbuf.buf,
						startbyte_size + to_copy * 2);
		if (ret < 0){
			mutex_unlock(par->import.spi_lock);
			return ret;
		}
		remain -= to_copy;
	}
	
	if (par->import.display_CS){
		par->import.display_CS(par->import.i2c_dev, 1);
		ndelay(100);
	}else{
		printk(KERN_ERR PAD_DISPLAY_NAME ": No display_CS to disable\n");
	}
	mutex_unlock(par->import.spi_lock);

	return ret;
}