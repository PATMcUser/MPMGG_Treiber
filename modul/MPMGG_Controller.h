#include "mcp23017.h"
#include "mcp23017.h"

#define MAX_PADS						0x04





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