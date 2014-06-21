obj-m := hello.o
KERNELDIR := /development/mini2440/linux-2.6.32.2
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

.PHONEY: clean
clean:
	rm -f hello.mod.c  hello.o   modules.order
	rm -f hello.ko  hello.mod.o    Module.symvers

