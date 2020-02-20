#obj-m:=char_driver.o
#obj-m:=sysfs_led_driver.o
obj-m:=led_ctrl.o

obj-m:=simple_char.o
all:
#	make -C $(KERN_DIR) M=$(PWD) modules
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
#	make -C $(KERN_DIR) M=$(PWD) clean
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
