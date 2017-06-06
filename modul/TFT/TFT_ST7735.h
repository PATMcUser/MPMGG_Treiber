/* Struktur fuer den Expander-Datentyp */
struct tft
  {
  char* fb      	/* FrameBuffer des Bausteines		*/
  char* SPIBus;     /* SPI-Device ("/dev/i2c-1" für Bus 1)	*/
  };
/* Expander-Datentyp */
typedef struct tft st7735;