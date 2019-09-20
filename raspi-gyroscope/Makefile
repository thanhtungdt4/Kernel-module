obj-m := gyro_ctl.o
obj-m := gyro_i2c_ctl.o

KERN_DIR=/home/tungnt58/workspace/testcase/vision/linux

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	#make -C $(KERN_DIR) M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	#make -C $(KERN_DIR) M=$(PWD) clean
