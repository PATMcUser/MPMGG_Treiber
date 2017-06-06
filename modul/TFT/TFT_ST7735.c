#include "TFT_ST7735.h"

#include <linux/fb.h>
#include <linux/spi/spi.h>

#include <linux/mutex.h>

#define TFT_PATH "spidev0.0"


static DEFINE_MUTEX(st7735_lock);

st7735 init_st7735(){
}

int (){
  mutex_lock(&st7735_lock);
  mutex_unlock(&st7735_lock);
  return 0;
}