#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include "i2cbusses.h"


static int *scan_i2c_bus(int file, unsigned long funcs, int first, int last){
	int i=0, j;
	int res, result[last-first+1];

	for(j = first; j <= last; j++) {
		fflush(stdout);

			
		/* Set slave address */
		if (ioctl(file, I2C_SLAVE, j) < 0) {
			if (errno == EBUSY) {
				continue;
			} else {
				printk("Error: Could not set address to 0x%02x: %s\n", j, strerror(errno));
				return result;
			}
		}

		/* Probe this address */
		res = i2c_smbus_write_quick(file, I2C_SMBUS_WRITE);

		if (res >= 0)
			umsatz[i++]=res;
				
			
	}

	return &result;
}


int main(int argc, char *argv[]) //(int startAddr, int endAddr)
{
	int i2cbus, file;
	int res*;
	char filename[20];
	unsigned long funcs;
	

	i2cbus = lookup_i2c_bus("1");
	if (i2cbus < 0) {
		printk("Error: Could not get i2c bus");
	}

	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
	if (file < 0) {
		exit(1);
	}

	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		printk("Error: Could not get the adapter ");
		close(file);
		exit(1);
	}


	if (!(funcs & (I2C_FUNC_SMBUS_QUICK | I2C_FUNC_SMBUS_READ_BYTE))) {
		printk("Error: Bus doesn't support detection commands\n");
	}
	
	if (!(funcs & I2C_FUNC_SMBUS_QUICK))
		printk("Warning: Can't use SMBus Quick Write ");
		
	if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE))
		printk("Warning: Can't use SMBus Receive Byte ");


	res = scan_i2c_bus(file, funcs, startAddr, endAddr);

	close(file);
	
	return res;
}