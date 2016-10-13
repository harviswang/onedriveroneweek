obj-m := filp_open_test.o
KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
clean:
	rm *.o *.ko *.mod.c modules.order  Module.symvers

.PHONY: clean all
