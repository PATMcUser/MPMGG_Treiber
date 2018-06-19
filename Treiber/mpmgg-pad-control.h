#define IOCON		0x0A
#define BANK_1		0x80
#define IOCON_VAL 	0x00 //0x00|BANK_1

#if (IOCON_VAL & BANK_1) == BANK_1 //BANK

/* Richtungsregister von Port A und Port B */
#define IODIRA 0x00
#define IODIRB 0x10

/* Datenregister Port A und Port B */
#define GPIOA 0x09
#define GPIOB 0x19

/* Register um Logik-Polaritaet umzustellen */
#define IPOLA 0x01
#define IPOLB 0x11

/* Interne Pull-Up-Widerstände einschalten */
#define GPPUA 0x06
#define GPPUB 0x16

/* Schreibe auf Latche für ausgang */
#define OLATA 0x0A
#define OLATB 0x1A

#else

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

#endif

/* Pins am Port */
#define P1 0x01
#define P2 0x02
#define P3 0x04
#define P4 0x08
#define P5 0x10
#define P6 0x20
#define P7 0x40
#define P8 0x80

#define ADDR_MASK	0x0F
#define DISPLAY_BL	0x01
#define DISPLAY_DC	0x40
#define DISPLAY_CS	0x80

#define DIR_MASK_A	0x3F
#define DIR_MASK_B	0xFC

#define DIR_MASK	0xFC3F