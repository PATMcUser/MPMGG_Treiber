#ifndef _EXPANDERLIB_H_
#define _EXPANDERLIB_H_

/* Richtungsregister von Port A und Port B */
#define IODIRA 0x00
#define IODIRB 0x01

/* Datenregister Port A und Port B */
#define GPIOA 0x12
#define GPIOB 0x13

/* Register um Logik-Polaritaet umzustellen */
#define IPOLA 0x02
#define IPOLB 0x03

/* Interne Pull-Up-Widerstände einschalten */
#define GPPUA 0x0C
#define GPPUB 0x0D

/* Schreibe auf Latche für ausgang */
#define OLATA 0x14
#define OLATB 0x15

/* Pins am Port */
#define P1 0x01
#define P2 0x02
#define P3 0x04
#define P4 0x08
#define P5 0x10
#define P6 0x20
#define P7 0x40
#define P8 0x80


/* Struktur fuer den Expander-Datentyp */
typedef struct{
  int address;      /* I2C-Bus-Adresse des Bausteines		*/
  int directionA;   /* Datenrichtung Port A					*/
  int directionB;   /* Datenrichtung Port B					*/
  int outA;   		/* State Port A          				*/
  int outB;   		/* State Port B                			*/
  char* I2CBus;     /* I2C-Device ("/dev/i2c-1" für Bus 1)	*/
}mcp23017;

mcp23017 init_mcp23017(int address);
mcp23017 init_full_mcp23017(int address, char* I2CBus);
int write_mcp23017(mcp23017 expander, int reg, int value);
int gpio_read_mcp23017( mcp23017 expander);
int read_mcp23017( mcp23017 expander, int reg);

void display_BL( mcp23017 expander, unsigned char value);
void display_DC( mcp23017 expander, unsigned char value);
void display_CS( mcp23017 expander, unsigned char value);

int find_mcp23017(char *addres, int max);

#endif /* EXPANDERLIB_H */