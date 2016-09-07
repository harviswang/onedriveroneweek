MODULE := version##这儿不允许有空格###
obj-m := $(MODULE).o
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

modules:
	$(MAKE) -Wall -Werror -C $(KERNELDIR) M=$(PWD) modules


.PHONEY: clean
clean:
	rm -f $(MODULE).mod.c  $(MODULE).o   modules.order
	rm -f $(MODULE).ko  $(MODULE).mod.o    Module.symvers

