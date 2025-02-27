obj-m += sleep.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc -g -Wall -o cat_nonblock cat_nonblock.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm cat_nonblock