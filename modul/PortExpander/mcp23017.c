#include "mcp23017.h"

#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <linux/mutex.h>

#define BASE_ADDR	0x20
#define ADDR_MASK	0x0F
#define ADDR_OFFSET	0x70
#define DISPLAY_BL	0x01
#define DISPLAY_DC	0x40
#defind DISPLAY_CS	0x80

#defind EXPANDER_PATH "/dev/i2c-1"

#include <linux/timer.h>
#include <linux/err.h>


static int lookup_i2c_bus_by_name(const char *bus_name);
int setdir_mcp23017( mcp23017 expander, int richtungsregister, int value);
int open_mcp23017( mcp23017 expander);

static DEFINE_MUTEX(mcp23017_lock);

/* Init des Expanders; gibt den Expander zurück
 * address: I2C-Busadresse des Bausteines (i2cdetect -y 1)
 */
mcp23017 init_mcp23017(int address){
  return init_full_mcp23017(address, EXPANDER_PATH);
}

/* Init des Expanders; gibt den Expander zurück
 * address: I2C-Busadresse des Bausteines (i2cdetect -y 1)
 * I2CBus: Pfad zum I2CBus ("/dev/i2c-1" für Bus 1)
 */
mcp23017 init_full_mcp23017(int address, char* I2CBus){
  int fd;             /* Filehandle */
  mcp23017 expander;  /* Rueckgabedaten */

  /* Structure mit Daten fuellen */
  expander.address = address;
  expander.directionA = 0x3F;
  expander.directionB = 0xFC;
  expander.outA = 0x00;
  expander.outB = 0x00;
  expander.I2CBus = I2CBus;

  // Port-Richtung (Eingabe/Ausgabe) setzen
  fd = open_mcp23017(expander);
  
  //display_CS(expander, 1);
  //display_DC(expander, 1);
  //display_BL(expander, 1);
  
  close(fd);
  return expander;
}

/* Ports Einrichten; gibt einen Fehler oder 0 zurück
 * expander: Definiert den IC
 * confreg: muss "IODIRA" oder "IODIRB" sein!
 * value: Zuordnung der Bits (Input: 1, Output: 0)
 * Bei den Eingangspins wird der Pullup-Widerstand eingeschaltet und die Logik umgekehrt
 */
int setdir_mcp23017( mcp23017 expander, int confreg, int value){
  if(confreg == IODIRA){
    write_mcp23017(expander, IODIRA, value);
    write_mcp23017(expander, GPPUA, value);
    write_mcp23017(expander, IPOLA, value);
	write_mcp23017(expander, OLATA, expander.outA );
	
	if(read_mcp23017(expander, IODIRA)==value)
	  return -5;
	if(read_mcp23017(expander, GPPUA)==value)
	  return -5;
	if(read_mcp23017(expander, IPOLA)==value)
	  return -5;
  }else if(confreg == IODIRB){
    write_mcp23017(expander, IODIRB, value);
    write_mcp23017(expander, GPPUB, value);
    write_mcp23017(expander, IPOLB, value);
	write_mcp23017(expander, OLATB, expander.outB );
	
	if(read_mcp23017(expander, IODIRB)==value)
	  return -5;
	if(read_mcp23017(expander, GPPUB)==value)
	  return -5;
	if(read_mcp23017(expander, IPOLB)==value)
	  return -5;
  }else{
    return -1;
  }
  
  return 0;
}

/* Öffnet den Bus; gibt einen Fehler oder den Filebus zurück
 * expander: Definiert den IC
 */
int open_mcp23017( mcp23017 expander){
  int fd;
  if ((fd = open(expander.I2CBus, O_RDWR)) < 0)
  {
    return -2;
  }

  /* Spezifizieren der Adresse des slave device */
  if (ioctl(fd, I2C_SLAVE, expander.address) < 0){
    return -14;
  }
  return fd;
}

/* Schreibt in ein Register des Expanders
 * expander: Definiert den IC
 * reg: Register in das geschrieben werden soll
 * value: Byte das geschrieben werden soll
 */
int write_mcp23017( mcp23017 expander, int reg, int value){
  int fd;
  mutex_lock(&mcp23017_lock);
  fd = open_mcp23017(expander);
  
  if( (i2c_smbus_write_byte_data(fd,reg,value)) < 0 ) return -2;
  
  close(fd);
  mutex_unlock(&mcp23017_lock);
  return 0;
}


/* Liest beide IO-Register
 * expander: Definiert den IC
 * gibt ausgelesenen Registerwert zurück
 */
int gpio_read_mcp23017( mcp23017 expander){
  int value,fd;
  mutex_lock(&mcp23017_lock);
  fd = open_mcp23017(expander);
  
  if( (value = i2c_smbus_read_word_data(fd, GPIOA)) < 0 ) value = -2;
  
  close(fd);
  mutex_unlock(&mcp23017_lock);
  return value;
}

/* Liest Register des Expanders
 * expander: Definiert den IC
 * reg: Register, das ausgelesen wird;
 * gibt ausgelesenen Registerwert zurück
 */
int read_mcp23017(mcp23017 expander, int reg){
  int value,fd;
  mutex_lock(&mcp23017_lock);
  fd = open_mcp23017(expander);
  
  if( (value = i2c_smbus_read_byte_data(fd, reg)) < 0 ) value =  -1;
  
  close(fd);
  mutex_unlock(&mcp23017_lock);
  return value;
}

static struct timer_list s_rstTimer;
#define WAIT_PERIOD					16 //ms

/* setzet Display-Flag für BL
 * expander: Definiert den IC;
 * gibt ausgelesenen Registerwert zurück
 */
void display_BL( mcp23017 expander, unsigned char value){
  expander.outB = value>0?(expander.outB | DISPLAY_BL):(expander.outB & ~DISPLAY_BL);
  mod_timer(&s_waitTimer,
              jiffies + msecs_to_jiffies(WAIT_PERIOD));
  //write_mcp23017(expander, OLATB, expander.outB );
}

/* setzet Display-Flag für DC
 * expander: Definiert den IC;
 * gibt ausgelesenen Registerwert zurück
 */
void display_DC( mcp23017 expander, unsigned char value){
  expander.outA = value>0?(expander.outA | DISPLAY_DC):(expander.outA & ~DISPLAY_DC);
  //int mod_timer (	struct timer_list *  	timer,
 	unsigned long  	expires);
  mod_timer(&s_waitTimer,
              jiffies + msecs_to_jiffies(WAIT_PERIOD));
  //write_mcp23017(expander, OLATA, expander.outA );
}

/* setzet Display-Flag für CS
 * expander: Definiert den IC;
 * gibt ausgelesenen Registerwert zurück
 */
void display_CS( mcp23017 expander, unsigned char value){
  expander.outA = value>0?(expander.outA | DISPLAY_CS):(expander.outA & ~DISPLAY_CS);
  mod_timer(&s_waitTimer,
              jiffies + msecs_to_jiffies(WAIT_PERIOD));
  //write_mcp23017(expander, OLATA, expander.outA );
}

/* setzet Display-Flag für DC und CS
 * expander: Definiert den IC;
 * gibt ausgelesenen Registerwert zurück
 */
void display_DC( mcp23017 expander, unsigned char dc, unsigned char cs){
  expander.outA = dc>0?(expander.outA | DISPLAY_DC):(expander.outA & ~DISPLAY_DC);
  expander.outA = cs>0?(expander.outA | DISPLAY_CS):(expander.outA & ~DISPLAY_CS);
  mod_timer(&s_waitTimer,
              jiffies + msecs_to_jiffies(WAIT_PERIOD));
  //write_mcp23017(expander, OLATA, expander.outA );
}


int find_mcp23017(char *addres, int max){
	int no = 0;
	int res;
	int file;
	//TODO: open
	if ((file = open(EXPANDER_PATH, O_RDWR)) < 0)return -2;

	/* Spezifizieren der Adresse des slave device */
	if (ioctl(file, I2C_SLAVE, expander.address) < 0)return -14;
	
	if (file < 0) return -1;

	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		printk(KERN_EMERG 
				"Error: Could not get the adapter ");
		close(file);
		return -1;
	}

	if (!(funcs & (I2C_FUNC_SMBUS_QUICK | I2C_FUNC_SMBUS_READ_BYTE))) {
		printk(KERN_EMERG 
				"Error: Bus doesn't support detection commands\n");
	}
	
	if (!(funcs & I2C_FUNC_SMBUS_QUICK))
		printk(KERN_WARNING 
				"Warning: Can't use SMBus Quick Write ");
		
	if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE))
		printk(KERN_WARNING 
				"Warning: Can't use SMBus Receive Byte ");

	//detect
	for(j = BASE_ADDR; j <= (BASE_ADDR + ADDR_MASK) && no < max; j++) {
		/* Set slave address */
		if (ioctl(file, I2C_SLAVE, j) < 0) {
			if (errno == EBUSY) {
				continue;
			} else {
				//printk("Error: Could not set address to 0x%02x: %s\n", j, strerror(errno));
				return -1;
			}
		}

		/* Probe this address */
		res = i2c_smbus_write_quick(file, I2C_SMBUS_WRITE);

		if (res >= 0)
			addres[no++]=res;
	}
	
	return 0;
}

static int lookup_i2c_bus_by_name(const char *bus_name)
{
	struct i2c_adap *adapters;
	int i, i2cbus = -1;

	adapters = gather_i2c_busses();
	if (adapters == NULL) {
		printk(KERN_EMERG 
				"Error: Out of memory!\n");
		return -3;
	}

	/* Walk the list of i2c busses, looking for the one with the
	   right name */
	for (i = 0; adapters[i].name; i++) {
		if (strcmp(adapters[i].name, bus_name) == 0) {
			if (i2cbus >= 0) {
				printk(KERN_EMERG 
					"Error: I2C bus name is not unique!\n");
				i2cbus = -4;
				goto done;
			}
			i2cbus = adapters[i].nr;
		}
	}

	if (i2cbus == -1)
		printk(KERN_EMERG 
			"Error: I2C bus name doesn't match any "
			"bus present!\n");

done:
	free_adapters(adapters);
	return i2cbus;
}