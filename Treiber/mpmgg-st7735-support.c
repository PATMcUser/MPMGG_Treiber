#include <linux/export.h>
#include <linux/errno.h>
#include <linux/spi/spi.h>
//#ifdef CONFIG_ARCH_BCM2708
//#include <mach/platform.h>
//#endif

#include "mpmgg-st7735.h"


int st7735r_write_spi(struct st7735r_par *par, void *buf, size_t len)
{
	int ret=0;
	struct spi_transfer t = {
		.tx_buf = buf,
		.len = len,
	};
	struct spi_message m;

	//printk(KERN_INFO  PAD_DISPLAY_NAME ":(len=%d): ", len);

	if (!par->spi) {
		printk(KERN_ERR PAD_DISPLAY_NAME ": par->spi is unexpectedly NULL\n");
		return -1;
	}

	mutex_lock(par->import.spi_write_lock);
	spi_message_init(&m);
	if (par->txbuf.dma && buf == par->txbuf.buf) {
		t.tx_dma = par->txbuf.dma;
		m.is_dma_mapped = 1;
	}
	spi_message_add_tail(&t, &m);
	ret = spi_sync(par->spi, &m);
	mutex_unlock(par->import.spi_write_lock);
	
	return ret;
	
}
EXPORT_SYMBOL(st7735r_write_spi);


int st7735r_read_spi(struct st7735r_par *par, void *buf, size_t len){
	int ret;
	u8 txbuf[32] = { 0, };
	struct spi_transfer	t = {
			.speed_hz = 2000000,
			.rx_buf		= buf,
			.len		= len,
		};
	struct spi_message	m;

	if (!par->spi) {
		printk(KERN_ERR PAD_DISPLAY_NAME ": par->spi is unexpectedly NULL\n");
		return -ENODEV;
	}

	mutex_lock(par->import.spi_write_lock);
	if (par->startbyte) {
		if (len > 32) {
			printk(KERN_ERR PAD_DISPLAY_NAME ": len=%d can't be larger than 32 when using 'startbyte'\n", len);
			return -EINVAL;
		}
		txbuf[0] = par->startbyte | 0x3;
		t.tx_buf = txbuf;
		printk(KERN_INFO PAD_DISPLAY_NAME ":(len=%d) txbuf => ", len);
	}

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	ret = spi_sync(par->spi, &m);
	printk(KERN_INFO PAD_DISPLAY_NAME ":(len=%d) buf <= ", len);
    mutex_unlock(par->import.spi_write_lock);
	
	return ret;
}
EXPORT_SYMBOL(st7735r_read_spi);

/*------------------------------------------------------------------*/
