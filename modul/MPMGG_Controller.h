#define MAX_PADS						0x04

/******************************
 *       MCP23017 Def's       *
 ******************************/
#define MPC23017_GPIOA_MODE				0x00
#define MPC23017_GPIOB_MODE				0x01
#define MPC23017_GPIOA_PULLUPS_MODE		0x0c
#define MPC23017_GPIOB_PULLUPS_MODE		0x0d
#define MPC23017_GPIOA_READ				0x12
#define MPC23017_GPIOB_READ				0x13
#define MPC23017_BASEADDR				0x20



struct mpmgg_pad {
    struct input_dev *dev;
    enum mk_type type;
    char phys[32];
    int mcp23017addr;
    int gpio_maps[12]
};

struct mpmgg_c {
    struct mk_pad pads[MAX_PADS];
    struct timer_list timer;
    int pad_count[MK_MAX];
    int used;
    struct mutex mutex;
};

static struct mpmgg_c *mpmgg_ctrl_base;