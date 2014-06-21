MODULE := leds
obj-m := $(MODULE).o
KERNELDIR := /development/mini2440/linux-2.6.32.2
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

.PHONEY: clean
clean:
	rm -f $(MODULE).mod.c  $(MODULE).o   modules.order
	rm -f $(MODULE).ko  $(MODULE).mod.o    Module.symvers

