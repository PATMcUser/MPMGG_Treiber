SRCS   = m1.c m2.c
OBJS   = $(SRCS:.c=.o)

obj-m += $(OBJS)

all:
    $(MAKE) -C /lib/modules/`uname -r`/build M=$(PWD) modules

all:
    $(MAKE) -C /lib/modules/`uname -r`/build M=$(PWD) modules
all:
	make -C /lib/modules/4.4.0-1055-raspi2/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

