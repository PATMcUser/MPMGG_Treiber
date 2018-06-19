ifneq ($(KERNELRELEASE),)

mpmgg-st7735-y += mpmgg-st7735-main.o mpmgg-st7735-fops.o mpmgg-st7735-sysfs.o mpmgg-st7735-support.o 
obj-m += mpmgg-st7735.o 

obj-m += mpmgg-pad.o
obj-m += mpmgg-port.o 
EXTRA_CFLAGS = -O2

else
KDIR ?= /lib/modules/`uname -r`/build

default: .config
	$(MAKE) -C $(KDIR) M=$$PWD modules
	
.config:
	grep config Kconfig | cut -d' ' -f2 | sed 's@^@CONFIG_@; s@$$@=m@' > .config
	
install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install
	
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.d .*.o.d *.mod.c .tmp_versions modules.order Module.symvers .config
endif