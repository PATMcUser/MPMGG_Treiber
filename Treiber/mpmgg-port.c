#include "mpmgg-port.h"
#include "mpmgg-pad-control.h"
#include <linux/delay.h>


static int NO_CLIENTS = MAX_PADS;
module_param(NO_CLIENTS, int, 1);
MODULE_PARM_DESC(NO_CLIENTS, "Number of pads");

//nach http://linuxseekernel.blogspot.de/2014/05/platform-device-driver-practical.html
//besser: https://www.codeproject.com/Tips/1080177/Linux-Platform-Device-Driver
static int mpmggport_driver_probe(struct platform_device *pdev){
	int i = 0, spi_s = 0;
	struct i2c_adapter		*i2c_adap = i2c_get_adapter(1);
	//struct i2c_client *pads[NO_CLIENTS];
	struct mpmggport_mod	*mpmggport;
	struct mpmgg_mutex 		*mux_platform;
	
	struct spi_master *master;
	struct spi_device *std_spi_dev = ((struct spi_device *)0);
	
	struct i2c_client *base;
	
	
	mpmggport = kmalloc(sizeof(struct mpmggport_mod), GFP_KERNEL);
	mpmggport->pad = kmalloc_array (NO_CLIENTS, sizeof(struct i2c_client *),GFP_KERNEL);

	
	mutex_init(&mpmggport->i2c_lock);
	//spin_lock_init(&mpmggport->i2c_lock);
	mutex_init(&mpmggport->spi_lock);
	//spin_lock_init(&mpmggport->spi_lock);
	mutex_init(&mpmggport->i2c_call_lock);
	mutex_init(&mpmggport->spi_write_lock);
	
	
	
	master = spi_busnum_to_master(0);
	if (!master) {
		printk(KERN_ERR PAD_NAME ": No Spi Master 0");
		goto miss;
	}
	master->num_chipselect = 10;
	std_spi_dev = spi_alloc_device(master);
	if(std_spi_dev){
		std_spi_dev->max_speed_hz = 8000000;
		//std_spi_dev->bus_num = 0;
		std_spi_dev->mode = SPI_MODE_0 | SPI_NO_CS;
		std_spi_dev->chip_select = 3;
	
		spi_add_device(std_spi_dev);
	}
	//std_spi_dev = spi_new_device(master, &st7735_board_info);
miss:	
	
	mux_platform = kmalloc_array (NO_CLIENTS, sizeof(struct mpmgg_mutex),GFP_KERNEL);
	for (i=0;i<NO_CLIENTS;i++){
		mux_platform[i].i2c_lock = &mpmggport->i2c_lock;
		mux_platform[i].spi_lock = &mpmggport->spi_lock;
		mux_platform[i].i2c_call_lock = &mpmggport->i2c_call_lock;
		mux_platform[i].spi_write_lock = &mpmggport->spi_write_lock;
		mux_platform[i].spi_dev = std_spi_dev;
		mux_platform[i].spi_cs = spi_s++;
	};

	
    if(!mpmggport)
        return -ENOMEM;
	platform_set_drvdata(pdev, mpmggport);
	

	//RESET
	printk(KERN_INFO PORT_DRIVER_NAME ": RESET\n");
	setGPIOFunction(GPIO_RESET, GPIO_OUTPUT);
	msleep(RESETTIME);
	setGPIOOutputValue(GPIO_RESET, GPIO_OFF);
	msleep(RESETTIME);
	setGPIOOutputValue(GPIO_RESET, GPIO_ON);
	
	//INIT Displays
	printk(KERN_INFO PORT_DRIVER_NAME ": HARD Display Init\n");
	base = i2c_new_dummy(i2c_adap, 0x20);
	
	mcp23017_init_all(base);
	st7735r_init_all(base, std_spi_dev, MPMGG_DISP_get_initArray(), &mux_platform[0]);
	
	i2c_unregister_device(base);
	
	// Setup device
	for(i=0;i<NO_CLIENTS;i++){
		printk(KERN_INFO PORT_DRIVER_NAME ": add type: %s , Addr: 0x%x\n",board_info[i].type, board_info[i].addr);
		board_info[i].platform_data = &mux_platform[i];
		
		mpmggport->pad[i]=i2c_new_device(i2c_adap, &(board_info[i]) );
	}
	
	return 0;
}

static int mpmggport_driver_suspend(struct device *pdev){
    //struct mpmggport_mod* mpmggport = dev_get_drvdata(pdev);
    return 0;
}

static int mpmggport_driver_resume(struct device *pdev){
    //struct mpmggport_mod* mpmggport = dev_get_drvdata(pdev);
    return 0;
}

static int mpmggport_driver_remove(struct platform_device *pdev){
	int i=0;
	struct mpmggport_mod *mpmggport;
	mpmggport = dev_get_platdata((struct device *)pdev);
	
	printk(KERN_INFO PORT_DRIVER_NAME ": RESET\n");
	setGPIOOutputValue(GPIO_RESET, GPIO_OFF);
	msleep(RESETTIME);
	setGPIOOutputValue(GPIO_RESET, GPIO_ON);
	msleep(RESETTIME);
	
	freeGPIO(GPIO_RESET);
	
	for(i=0;i<NO_CLIENTS;i++)
		if (mpmggport->pad[i]){ 
			i2c_unregister_device(mpmggport->pad[i]);
			printk(KERN_INFO PORT_DRIVER_NAME ": unregistered Dev: %d", i);
		}
	
	kzfree(mpmggport);
    printk(KERN_WARNING PORT_DRIVER_NAME ": free the kernel!!!!!");
	return 0;
}


static int __init mpmggport_init(void){
	int ret=0;
	while (MPMGG_pad_load());
    printk(KERN_INFO PORT_DRIVER_NAME ": Register");
    platform_device_register(&mpmggport_dev);
	ret = platform_driver_probe(&mpmggport_driver, mpmggport_driver_probe);
	return ret;
}


static void __exit mpmggport_exit(void){
    platform_driver_unregister(&mpmggport_driver);
    printk(KERN_INFO PORT_DRIVER_NAME ": Unregister");
}


int setGPIOFunction(int GPIO, int functionCode){
  if (!gpio_is_valid(GPIO)){
    printk(KERN_INFO "GPIO_TEST: invalid GPIO: %d\n",GPIO);
    return -ENODEV;
  }
  gpio_request(GPIO, "sysfs");				// Causes gpio to appear in /sys/class/gpio
  if(functionCode==GPIO_OUTPUT){
    gpio_direction_output(GPIO, 1);			// Set the gpio to be in output mode and on
  }else{
    gpio_direction_input(GPIO);				// Set the button GPIO to be an input
    gpio_set_debounce(GPIO, DEBOUNCE_TIME);	// Set the buttons depunce time
  }
  gpio_export(GPIO, false);					// the bool argument prevents the direction from being changed
  return 0;
}

void freeGPIO(int GPIO){
   gpio_set_value(GPIO, 0);					// Turn the GPIO off, makes it clear the device was unloaded
   gpio_unexport(GPIO);						// Unexport the GPIO
   gpio_free(GPIO);							// Free the Button GPIO
}


void setGPIOOutputValue(int GPIO, int outputValue){
   gpio_set_value(GPIO, outputValue);		// Set the physical GPIO accordingly
}


/**
 * st7735r_init_display() - Generic init_display() function
 * @par: Driver data
 *
 * Uses par->init_sequence to do the initialization
 *
 * Return: 0 if successful, negative if error
 */
int st7735r_init_all(struct i2c_client *base, struct spi_device *spi, int *init_sequence, struct mpmgg_mutex *import){
	int buf[64];
	int i = 0, j, ret=0;
	struct st7735r_par *par=kmalloc(sizeof(struct st7735r_par), GFP_KERNEL);
	
	par->import.spi_write_lock=import->spi_write_lock;
	par->import.spi_lock=import->spi_lock;
	par->spi=import->spi_dev;
	par->txbuf.dma=0;

	/* sanity check */
	if (!init_sequence) {
		printk(KERN_ERR PAD_DISPLAY_NAME ": error: init_sequence is not set\n");
		return -EINVAL;
	}

	mutex_lock(import->spi_lock);
	
	/* make sure stop marker exists */
	for (i = 0; i < FBTFT_MAX_INIT_SEQUENCE; i++)
		if (init_sequence[i] == -3)
			break;
	if (i == FBTFT_MAX_INIT_SEQUENCE) {
		printk(KERN_ERR PAD_DISPLAY_NAME ": missing stop marker at end of init sequence\n");
		return -EINVAL;
	}

	i = 0;
	//printk(KERN_INFO PAD_DISPLAY_NAME ": configuring display\n");
	while (i < FBTFT_MAX_INIT_SEQUENCE) {
		if (init_sequence[i] == -3) {
			/* done */
			kzfree(par);
			mutex_unlock(import->spi_lock);
			return 0;
		}
		if (init_sequence[i] >= 0) {
			printk(KERN_ERR PAD_DISPLAY_NAME ": missing delimiter at position %d\n", i);
			mutex_unlock(import->spi_lock);	
			return -EINVAL;
		}
		if (init_sequence[i+1] < 0) {
			printk(KERN_ERR PAD_DISPLAY_NAME ": missing value after delimiter %d at position %d\n",
				init_sequence[i], i);
			mutex_unlock(import->spi_lock);	
			return -EINVAL;
		}
		
		switch (init_sequence[i]) {
		case -1:
			i++;

			/* Write */
			j = 0;
			while (init_sequence[i] >= 0) {
				if (j > 63) {
					printk(KERN_ERR PAD_DISPLAY_NAME ": Maximum register values exceeded\n");
					mutex_unlock(import->spi_lock);	
					return -EINVAL;
				}
				buf[j++] = init_sequence[i++];
			}	
			
			all_i2c(base, OLATA, 0);
			ret=st7735r_write_spi(par, buf, sizeof(u8));
			all_i2c(base, OLATA, DISPLAY_DC|DISPLAY_CS);
			
			if (ret < 0) {
				printk(KERN_ERR PAD_DISPLAY_NAME ": write() failed and returned %d\n", ret);
				goto endbreak;
			}
	
			if (--j) {
				all_i2c(base, OLATA, DISPLAY_DC);	
				ret=st7735r_write_spi(par, &buf[1], j*(sizeof(u8)));
				all_i2c(base, OLATA, DISPLAY_DC|DISPLAY_CS);
			}
endbreak:	
			break;
		case -2:
			i++;
			mdelay(init_sequence[i++]);
			break;
		default:
			printk(KERN_ERR PAD_DISPLAY_NAME ": unknown delimiter %d at position %d\n",
				init_sequence[i], i);
			mutex_unlock(import->spi_lock);	
			return -EINVAL;
		}
	}

	printk(KERN_ERR PAD_DISPLAY_NAME ": The magic fairy swings the wand to get here.\n");
	mutex_unlock(import->spi_lock);	
	return -EINVAL;
}


void mcp23017_init_all(struct i2c_client *base){
	all_i2c(base, IOCON,  IOCON_VAL);
	all_i2c(base, IODIRA, DIR_MASK_A);
	all_i2c(base, GPPUA,  DIR_MASK_A);
	all_i2c(base, IPOLA,  DIR_MASK_A);
	all_i2c(base, OLATA,  DIR_MASK_A);
	all_i2c(base, IODIRB, DIR_MASK_B);
	all_i2c(base, GPPUB,  DIR_MASK_B);
	all_i2c(base, IPOLB,  DIR_MASK_B);
	all_i2c(base, OLATB,  DIR_MASK_B);
}

void all_i2c(struct i2c_client *base, u8 value1, u8 value2){
	int i, base_addr = base->addr;
	for(i=0;i<NO_CLIENTS;i++){
		base->addr=base_addr+i;
		i2c_smbus_write_byte_data(base, value1, value2);
	}
	base->addr=base_addr;
}

