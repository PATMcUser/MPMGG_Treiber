#ifndef _MPMGGDISP_CTRL_
#define _MPMGGDISP_CTRL_


#define NOP     0x00
#define SWRESET 0x01
#define RDDID   0x04
#define RDDST   0x09

#define SLPIN   0x10
#define SLPOUT  0x11
#define PTLON   0x12
#define NORON   0x13

#define INVOFF  0x20
#define INVON   0x21
#define DISPOFF 0x28
#define DISPON  0x29
#define CASET   0x2A
#define RASET   0x2B
#define RAMWR   0x2C
#define RAMRD   0x2E

#define PTLAR   0x30
#define COLMOD  0x3A
#define MADCTL  0x36

#define FRMCTR1 0xB1
#define FRMCTR2 0xB2
#define FRMCTR3 0xB3
#define INVCTR  0xB4
#define DISSET5 0xB6

#define PWCTR1  0xC0
#define PWCTR2  0xC1
#define PWCTR3  0xC2
#define PWCTR4  0xC3
#define PWCTR5  0xC4
#define VMCTR1  0xC5

#define RDID1   0xDA
#define RDID2   0xDB
#define RDID3   0xDC
#define RDID4   0xDD

#define PWCTR6  0xFC

#define GMCTRP1 0xE0
#define GMCTRN1 0xE1


#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

// color definitions
#define COLOR_BLACK       0x0000      /*   0,   0,   0 */
#define COLOR_NAVY        0x000F      /*   0,   0, 128 */
#define COLOR_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define COLOR_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define COLOR_MAROON      0x7800      /* 128,   0,   0 */
#define COLOR_PURPLE      0x780F      /* 128,   0, 128 */
#define COLOR_OLIVE       0x7BE0      /* 128, 128,   0 */
#define COLOR_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define COLOR_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define COLOR_BLUE        0x001F      /*   0,   0, 255 */
#define COLOR_GREEN       0x07E0      /*   0, 255,   0 */
#define COLOR_CYAN        0x07FF      /*   0, 255, 255 */
#define COLOR_RED         0xF800      /* 255,   0,   0 */
#define COLOR_MAGENTA     0xF81F      /* 255,   0, 255 */
#define COLOR_YELLOW      0xFFE0      /* 255, 255,   0 */
#define COLOR_WHITE       0xFFFF      /* 255, 255, 255 */
#define COLOR_ORANGE      0xFD20      /* 255, 165,   0 */
#define COLOR_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define COLOR_PINK        0xF81F

#endif