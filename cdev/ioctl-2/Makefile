obj-m += ioctl-2.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc -g -Wall -o userspace-ioctl userspace-ioctl.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm userspace-ioctl