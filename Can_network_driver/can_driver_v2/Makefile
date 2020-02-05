obj-m := can_driver_ctrl.o
can_driver_ctrl-objs := can_driver.o mcp_can.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
