obj-m := hello.o
KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

.PHONEY: clean
clean:
	rm -f hello.mod.c  hello.o   modules.order
	rm -f hello.ko  hello.mod.o    Module.symvers

