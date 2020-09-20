CONFIT_MODULE_SIG = n
EXTRA_CFLAGS += -O2
ifneq ($(KERNELRELEASE),)
	obj-m := udriver_kpart.o
	udriver_kpart-objs += udriver_kpart_init.o
else
	KERNELDIR ?=/usr/src/kylin-headers-4.4.58-20180615-generic
	#KERNELDIR ?=/usr/src/linux-headers-4.15.0-29-generic
	#KERNELDIR ?=/lib/modules/$(shell uname -r)/build

	PWD :=$(shell pwd)
      default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=arm64 clean
	#$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=x86_64 clean
	
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=arm64 modules  
	#$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=x86_64 modules  
endif
      clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=arm64 clean
	#$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=x86_64 modules  
