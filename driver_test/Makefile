ifneq  ($(KERNELRELEASE),)  
obj-m:=beep.o  
$(info "2nd")  
else  
#KDIR := /lib/modules/$(shell uname -r)/build  
KDIR := /opt/linux-3.4.y
PWD:=$(shell pwd)  
all:  
	$(info "1st")  
	make -C $(KDIR) M=$(PWD) modules  
	gcc test.c -o beeptest  
clean:  
	rm -f *.ko *.o *.symvers *.mod.c *.mod.o *.order  
endif  
