obj-m := get_flag.o
KID := /home/yevzwming/code/Android/android-kernel/kernel/goldfish
CROSS_COMPILE=arm-linux-androideabi-
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld

all:
	make -C $(KID) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) M=$(shell pwd) modules

clean:
	rm -rf *.ko *.o *.mod.c *.order *.symvers