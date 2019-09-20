
obj-m += thermal_driver.o
thermal_driver-objs := lcd.o ADC0808.o thermal-ctl.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

