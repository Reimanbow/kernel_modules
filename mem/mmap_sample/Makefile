obj-m += mmap_sample.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc -g -Wall -o mmap_sample_user mmap_sample_user.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf mmap_sample_user