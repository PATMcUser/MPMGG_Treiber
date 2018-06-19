#include "mpmgg-pad.h"
#include "mpmgg-pad-control.h"
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <linux/delay.h>


/*	//SPI WAY
static struct spi_board_info st7735_spi_info = {
	.modalias = PAD_DISPLAY_NAME,
	.max_speed_hz = 8000000,
	.bus_num = 0,
	.mode = SPI_MODE_0 | SPI_NO_CS,
};
*/

static spinlock_t probe_pad_lock;
static int OK = 1;
static int spi_s = 0;


static int poll_Ops(void * data){
	struct i2c_client *client;
	struct input_dev *pad;
	struct mpmggpad_mod *mpmggpad;
	int button, new_in;
	int errCnt=0, debounce=0;
	
	
	client = (struct i2c_client*) data;
	mpmggpad = i2c_get_clientdata(client);
	//printk(KERN_INFO PAD_NAME ": poll client: 0x%x", client->addr);
	pad = mpmggpad->pad;
	
	set_freezable();
	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;
		schedule();
		try_to_freeze();
		set_current_state(TASK_RUNNING);
		
		set_current_state(TASK_UNINTERRUPTIBLE);

		mutex_lock(mpmggpad->call_lock);
		//mutex_lock(&mpmggpad->call_id_lock);
retry:	
		new_in = gpio_read_mcp23017( client );
		//printk(KERN_INFO PAD_NAME ": get data; %d \n", mpmggpad->data); 
		for(debounce=0; debounce<DEBOUNCECNT; debounce++){
			ndelay(CALL_DELAY);
			new_in &= gpio_read_mcp23017( client );
		}
		
		if (client->addr==0x20)
			printk(KERN_WARNING PAD_NAME ": pad %x\t\tinput:%x\n", client->addr, new_in);
		
		if ( new_in < 0 || new_in >= 0xFFFF){
			printk(KERN_WARNING PAD_NAME ": can't read pad; %x   error:%x\n", client->addr, new_in);
			//if(10<=errCnt++){
				//printk(KERN_ERR PAD_NAME ": killing pad; %x \n", client->addr);
				//mutex_unlock(&call_lock);
				//break;
			//}
			ndelay(CALL_DELAY);
			goto retry;
			//mutex_unlock(mpmggpad->call_lock);
			//continue;
		}else{
			
			if ( __sw_hweight16(new_in & DIR_MASK) > 5 ){
				printk(KERN_WARNING PAD_NAME ": pad fault; %x   error:%x\n", client->addr, new_in);
				
				mutex_lock(mpmggpad->spi_lock);
				mutex_lock(mpmggpad->spi_write_lock);
				write_mcp23017(client, IODIRA, DIR_MASK_A);
				write_mcp23017(client, GPPUA, DIR_MASK_A);
				write_mcp23017(client, IPOLA, DIR_MASK_A);
				
				write_mcp23017(client, IODIRB, DIR_MASK_B);
				write_mcp23017(client, GPPUB, DIR_MASK_B);
				write_mcp23017(client, IPOLB, DIR_MASK_B);
				
				//write_mcp23017(client, OLATA, (DIR_MASK|new_in));
				//write_mcp23017(client, OLATB, ((DIR_MASK|new_in)>>8));
				
				mutex_unlock(mpmggpad->spi_lock);
				mutex_unlock(mpmggpad->spi_write_lock);
				goto retry;
				//mutex_unlock(mpmggpad->call_lock);
				//continue;
			}
			
			mpmggpad->old  = mpmggpad->data;
			mpmggpad->data = new_in;
		}
		
		errCnt=0;
		
		if ( mpmggpad->data != mpmggpad->old ){
	
			if(		(mpmggpad->data^mpmggpad->old) & (MPMGG_KEY_INP_UP | MPMGG_KEY_INP_DOWN)	)
				input_report_abs(pad, mpmggpad_axis[1], 
						((mpmggpad->data & MPMGG_KEY_INP_UP)?1:0) 		- ((mpmggpad->data & MPMGG_KEY_INP_DOWN)?1:0) );
			
			if(		(mpmggpad->data^mpmggpad->old) & (MPMGG_KEY_INP_RIGHT | MPMGG_KEY_INP_LEFT)	)
				input_report_abs(pad, mpmggpad_axis[0], 
						((mpmggpad->data & MPMGG_KEY_INP_RIGHT)?1:0) 	- ((mpmggpad->data & MPMGG_KEY_INP_LEFT)?1:0) );

			for(button=0;mpmggpad_btn[button];button++)
				if ( (mpmggpad->data^mpmggpad->old) & mpmggpad_mask[button] )
					input_report_key(pad,	mpmggpad_btn[button],
							(mpmggpad->data & mpmggpad_mask[button]) 	);
		
			input_sync(pad);
		}

		mutex_unlock(mpmggpad->call_lock);
	}
	return 0;
}


static void pollTimerHandler(unsigned long data){
	struct i2c_client *client = (struct i2c_client *)data;
	struct mpmggpad_mod *mpmggpad = i2c_get_clientdata(client);
	//if(mpmggpad->thread->state>0){
		//TOT
	//mpmggpad->thread = kthread_run( poll_Ops, client, "%s", client->name);
	//}else{
		//Lebt noch
		wake_up_process(mpmggpad->thread);
	//}
	if (mpmggpad->poll==1) 
		mod_timer(&mpmggpad->timer, jiffies + msecs_to_jiffies( POLL_RATE ));
}

/* Ports Einrichten; gibt einen Fehler oder 0 zurück
 * expander: Definiert den IC
 * confreg: muss "IODIRA" oder "IODIRB" sein!
 * value: Zuordnung der Bits (Input: 1, Output: 0)
 * Bei den Eingangspins wird der Pullup-Widerstand eingeschaltet und die Logik umgekehrt
 */
int setdir_mcp23017(struct i2c_client* expander, int confreg, int value){
  int err=0;	  
  
  if(confreg == IODIRA){
	err += write_mcp23017(expander, IODIRA, value);
	err += write_mcp23017(expander, GPPUA, value);
	err += write_mcp23017(expander, IPOLA, value);
	err += write_mcp23017(expander, OLATA, value);
	
	if(read_mcp23017(expander, IODIRA)!=value)
		err = -5;
	if(read_mcp23017(expander, GPPUA)!=value)
		err = -5;
	if(read_mcp23017(expander, IPOLA)!=value)
		err = -5;
  }else if(confreg == IODIRB){
	err += write_mcp23017(expander, IODIRB, value);
	err += write_mcp23017(expander, GPPUB, value);
	err += write_mcp23017(expander, IPOLB, value);
	err += write_mcp23017(expander, OLATB, value);
	
	if(read_mcp23017(expander, IODIRB)!=value)
		err =  -5;
	if(read_mcp23017(expander, GPPUB)!=value)
		err =  -5;
	if(read_mcp23017(expander, IPOLB)!=value)
		err =  -5;
  }else{
	err =  -1;
  }
  
  return err;
}

/* Schreibt in ein Register des Expanders
 * expander: Definiert den IC
 * reg: Register in das geschrieben werden soll
 * value: Byte das geschrieben werden soll
 */
int write_mcp23017(struct i2c_client* expander, int reg, int value){
  int err = 0;
  struct mpmggpad_mod *mpmggpad = i2c_get_clientdata(expander);
  mutex_lock(mpmggpad->i2c_lock);
  if( (i2c_smbus_write_byte_data(expander,reg,value)) < 0 )
		err = -2;
  i2c_smbus_read_byte_data(expander, reg);
  mutex_unlock(mpmggpad->i2c_lock);
  return err;
}


/* Liest beide IO-Register
 * expander: Definiert den IC
 * gibt ausgelesenen Registerwert zurück
 */
int gpio_read_mcp23017(struct i2c_client* expander){
  struct mpmggpad_mod *mpmggpad = i2c_get_clientdata(expander);
  int value;
  mutex_lock(mpmggpad->i2c_lock);
#if (IOCON_VAL & 0x80) == 0x80 //BANK=1
  value = i2c_smbus_read_byte_data(expander, GPIOB);
  value <<= 8;
  value |= i2c_smbus_read_byte_data(expander, GPIOA);
#else
  value = i2c_smbus_read_word_data(expander, GPIOA);
#endif
  mutex_unlock(mpmggpad->i2c_lock);
  return value;
}

/* Liest Register des Expanders
 * expander: Definiert den IC
 * reg: Register, das ausgelesen wird;
 * gibt ausgelesenen Registerwert zurück
 */
int read_mcp23017(struct i2c_client* expander, int reg){
  struct mpmggpad_mod *mpmggpad = i2c_get_clientdata(expander);
  int value;
  mutex_lock(mpmggpad->i2c_lock);
  value = i2c_smbus_read_byte_data(expander, reg);
  mutex_unlock(mpmggpad->i2c_lock);
  return value;
}


/* setzet Display-Flag für BL
 * expander: Definiert den IC;
 * gibt ausgelesenen Registerwert zurück
 */
void MPMGG_display_BL(struct i2c_client* expander, unsigned char value){
  //struct mpmggpad_mod *mpmggpad = i2c_get_clientdata(expander);
  int out = read_mcp23017(expander, OLATB);
  //mutex_lock(&mpmggpad->call_id_lock);
  out = value>0?(out | DISPLAY_BL):(out & ~DISPLAY_BL);
  write_mcp23017( expander, OLATB, out);
  //mutex_unlock(&mpmggpad->call_id_lock);
}
EXPORT_SYMBOL(MPMGG_display_BL);

/* setzet Display-Flag für DC
 * expander: Definiert den IC;
 * gibt ausgelesenen Registerwert zurück
 */
void MPMGG_display_DC(struct i2c_client* expander, unsigned char value){
  //struct mpmggpad_mod *mpmggpad = i2c_get_clientdata(expander);
  int out = read_mcp23017(expander, OLATA);
  //mutex_lock(&mpmggpad->call_id_lock);
  out = value>0?(out | DISPLAY_DC):(out & ~DISPLAY_DC);
  write_mcp23017( expander, OLATA, out);
  //mutex_unlock(&mpmggpad->call_id_lock);
}

EXPORT_SYMBOL(MPMGG_display_DC);

/* setzet Display-Flag für CS
 * expander: Definiert den IC;
 * gibt ausgelesenen Registerwert zurück
 */
void MPMGG_display_CS(struct i2c_client* expander, unsigned char value){
  //struct mpmggpad_mod *mpmggpad = i2c_get_clientdata(expander);
  int out = read_mcp23017(expander, OLATA);
  //mutex_lock(&mpmggpad->call_id_lock);
  out = value>0?(out | DISPLAY_CS):(out & ~DISPLAY_CS);
  write_mcp23017( expander, OLATA, out);
  if (value) ndelay(5);
  //mutex_unlock(&mpmggpad->call_id_lock);
}
EXPORT_SYMBOL(MPMGG_display_CS);

/* setzet Display-Flag für DC und CS
 * expander: Definiert den IC;
 * gibt ausgelesenen Registerwert zurück
 */
void MPMGG_display_DC_CS(struct i2c_client* expander, unsigned char dc, unsigned char cs){
  //struct mpmggpad_mod *mpmggpad = i2c_get_clientdata(expander);
  int out = read_mcp23017(expander, OLATA);
  //mutex_lock(&mpmggpad->call_id_lock);
  out = dc>0?(out | DISPLAY_DC):(out & ~DISPLAY_DC);
  out = cs>0?(out | DISPLAY_CS):(out & ~DISPLAY_CS);
  write_mcp23017( expander, OLATA, out);
  if (cs) ndelay(5);
  //mutex_unlock(&mpmggpad->call_id_lock);
}
EXPORT_SYMBOL(MPMGG_display_DC_CS);


static int mpmggpad_probe(struct i2c_client *client, const struct i2c_device_id *id){
	int err=0, j=0;
	struct mpmggpad_mod *mpmggpad;
	struct input_dev *input_pad;
	struct i2c_adapter *adapter;
	/*	//SPI WAY
	struct spi_master	*master;
	struct spi_board_info *dsp;
	struct spi_device *output_disp;
	*/
	struct mpmgg_mutex *mux;
	struct mpmgg_display *display_platform;
	struct platform_device *dsp_plat;
	int output_disp;
	
	mux =(struct mpmgg_mutex*) client->dev.platform_data;
	
	spin_lock(&probe_pad_lock);
	mutex_lock(mux->i2c_call_lock);
	display_platform=kmalloc(sizeof(struct mpmgg_display), GFP_KERNEL);
	//kmalloc(sizeof(struct mpmgg_display), GFP_KERNEL);
	
	//if (client && client->adapter)
	adapter = client->adapter;
	
	
	//printk(KERN_INFO PAD_NAME ": Probe \n");
	printk(KERN_INFO PAD_NAME ": dev_addr: %d  on: %d - %s \n",client->addr,adapter->nr,adapter->name);
	
	
   if (!i2c_check_functionality(client->adapter, 
			I2C_FUNC_SMBUS_READ_BYTE_DATA | 
			I2C_FUNC_SMBUS_WRITE_BYTE_DATA|
			I2C_FUNC_SMBUS_READ_WORD_DATA )	)	{
      printk(KERN_ERR PAD_NAME ": needed i2c functionality is not supported\n");
	  mutex_unlock(mux->i2c_call_lock);
	  spin_unlock(&probe_pad_lock);
      return -ENODEV;
   }
	
	mpmggpad = kzalloc(sizeof(struct mpmggpad_mod), GFP_KERNEL);
	if (!mpmggpad){
	  mutex_unlock(mux->i2c_call_lock);
	  spin_unlock(&probe_pad_lock);
	  return -ENOMEM;
	}
	
	i2c_set_clientdata(client, mpmggpad);
	
	
	mpmggpad->i2c_lock	= mux->i2c_lock;
	mpmggpad->spi_lock	= mux->spi_lock;
	mpmggpad->spi_write_lock	= mux->spi_write_lock;
	//mutex_init(&mpmggpad->call_id_lock);
	
	err	+= setdir_mcp23017(client, IODIRA, DIR_MASK_A);
	if(err) printk(KERN_ERR PAD_NAME ": IODIRA not found\n");
	err	+= setdir_mcp23017(client, IODIRB, DIR_MASK_B);
	if(err) printk(KERN_ERR PAD_NAME ": IODIRB not found\n");
	
	
	mpmggpad->mcp23017	= client;
	err = read_mcp23017(client, IODIRA);
	if ( err < 0 ) {
		printk(KERN_ERR PAD_NAME ": first contact read failed");
		goto fail1;
	}else{
		//printk(KERN_INFO PAD_NAME ": first contact read OK: %x", err);
		err = 0;
	}
	
	
	//add input dev
	mpmggpad->pad		= input_pad = input_allocate_device();
	mpmggpad->poll=1;
	mpmggpad->call_lock = mux->i2c_call_lock;
	
	if (!input_pad || !mpmggpad->mcp23017 || err ) {
		err = -ENOMEM;
		goto fail1;
	}
	
	//configure Pad
	snprintf(mpmggpad->phys, sizeof(mpmggpad->phys),
			 "mpmggpad_input%d", client->addr);

	input_pad->name 	= PAD_NAME;
	input_pad->phys = mpmggpad->phys;
	input_pad->id.bustype = BUS_I2C;
	input_pad->id.vendor  = 0x00;
	input_pad->id.product = 0x01;
	input_pad->id.version = 0x01;
	input_pad->dev.parent = &client->dev;
	
	input_set_drvdata(input_pad, mpmggpad);

	input_pad->open = mpmggpad_open;
	input_pad->close = mpmggpad_close;

	//configure Data-access
	__set_bit(EV_KEY, input_pad->evbit);
	__set_bit(EV_ABS, input_pad->evbit);
	
	//printk(KERN_INFO PAD_NAME ": config Keys");
	set_bit(mpmggpad_axis[0], input_pad->absbit);
	set_bit(mpmggpad_axis[1], input_pad->absbit);
	
	input_set_abs_params(input_pad, mpmggpad_axis[0], -1, 1, 0, 0);
	input_set_abs_params(input_pad, mpmggpad_axis[1], -1, 1, 0, 0);
	
	for (j = 0; mpmggpad_btn[j]; j++)
		__set_bit(mpmggpad_btn[j], input_pad->keybit);
		
	
	err = input_register_device(input_pad);
	
	if (err)
		goto fail2;
	
	//init polling
	init_timer(&mpmggpad->timer);
	//mpmggpad->timer.data = (struct i2c_client*) client;
	mpmggpad->timer.data = (int) client;
	mpmggpad->timer.function = pollTimerHandler;
	
	mpmggpad->thread = kthread_run( poll_Ops, client, "%s",
					   client->name);
	
	mod_timer(&mpmggpad->timer, jiffies + msecs_to_jiffies( POLL_RATE));
	
	
	//add Display
	display_platform->spi_lock 		 = mpmggpad->spi_lock;
	display_platform->spi_write_lock = mpmggpad->spi_write_lock;
	display_platform->i2c_dev		 = mpmggpad->mcp23017;
	display_platform->spi_dev		 = mux->spi_dev;
	display_platform->display_BL	 = MPMGG_display_BL;
	display_platform->display_DC	 = MPMGG_display_DC;
	display_platform->display_CS	 = MPMGG_display_CS;
	display_platform->display_DC_CS	 = MPMGG_display_DC_CS;
	display_platform->spi_cs 		 = mux->spi_cs;
	
	/*
		//SPI WAY
	dsp = kmalloc(sizeof(struct spi_board_info), GFP_KERNEL);
	*dsp = st7735_spi_info;
	dsp->platform_data = (void *) display_platform;
	master = spi_busnum_to_master(0);
	if (!master) {
		printk(KERN_ERR PAD_NAME ": No Spi Master 0");
		goto fail2;
	}
	master->num_chipselect = 10;
	//dsp->chip_select = mux->spi_cs;
	output_disp = spi_new_device(master, dsp);
	mpmggpad->disp		= output_disp;
	*/
		//PLATFORM WAY
	//sprintf (name_buf, "%s%d", PAD_DISPLAY_NAME, );
	
	dsp_plat = platform_device_alloc(PAD_DISPLAY_NAME, mux->spi_cs);
	dsp_plat->dev.platform_data = (void *) display_platform;
	output_disp=platform_device_add(dsp_plat);
	//*/
	if (mpmggpad->disp || output_disp < 0) {
		printk(KERN_ERR PAD_NAME ": Can't register Display");
		//err = -ENXIO;
		//goto fail2;
	}
	
	if (err)
		goto fail2;
	
	spin_unlock(&probe_pad_lock);
	mutex_unlock(mpmggpad->call_lock);
	return 0;

fail2:	input_free_device(mpmggpad->pad);
fail1:	if (mpmggpad->pad)
			input_unregister_device(mpmggpad->pad);
	kfree(mpmggpad);
	spin_unlock(&probe_pad_lock);
	mutex_unlock(mux->i2c_call_lock);
	return err;
}


static int mpmggpad_remove(struct i2c_client *client){
	struct mpmggpad_mod *mpmggpad = i2c_get_clientdata(client);
	input_free_device(mpmggpad->pad);
	//TODO: remove framebuffer
	del_timer_sync(&mpmggpad->timer);
	kzfree(mpmggpad);
	//i2c_unregister_device(client);
	i2c_set_clientdata(client, NULL);
	return 0;
}


static int mpmggpad_open(struct input_dev *dev){
	struct mpmggpad_mod *mpmggpad = input_get_drvdata(dev);
	mpmggpad->poll=1;
	mod_timer(&mpmggpad->timer, jiffies + msecs_to_jiffies( POLL_RATE ));
	return 0;
}

static void mpmggpad_close(struct input_dev *dev){
	struct mpmggpad_mod *mpmggpad= input_get_drvdata(dev);
	mpmggpad->poll=0;
}


int MPMGG_pad_load(){return OK;}
EXPORT_SYMBOL(MPMGG_pad_load);


static int __init mcp23017_module_init(void){
	int ret;
	printk(KERN_INFO PAD_NAME ": Register\n");
	spi_s = 0;
	spin_lock_init(&probe_pad_lock);
	while (MPMGG_st7735_load());
	ret = i2c_add_driver(&mpmggpad_driver);
	OK = 0;
	return ret;
}

static void __exit mcp23017_module_exit(void){
	i2c_del_driver(&mpmggpad_driver);
	printk(KERN_INFO PAD_NAME ": Unegister\n");
}

module_init(mcp23017_module_init);
module_exit(mcp23017_module_exit);

