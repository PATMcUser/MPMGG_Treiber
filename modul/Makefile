obj-m += MPMGG_Controller_GP.o

all:
	make -C /lib/modules/4.4.0-1055-raspi2/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

