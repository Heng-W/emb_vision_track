# Build kernel modules

ifneq ($(KERNELRELEASE),)

# obj-m := <module_name>.o
obj-m:= motor.o servo.o

# <module_name>-objs := <obj_files>
#led-objs:=led/led.o
motor-objs:=motor/motor.o mach/pwm.o mach/gpio.o
servo-objs:=servo/servo.o mach/pwm.o 

else

KERNEL_DIR:=~/src/linux-3.4.39/
PWD:=$(shell pwd)
MODULES_DIR:=../build/modules/

.PHONY: modules modules_clean
modules:
	make -C $(KERNEL_DIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-
	mkdir -p $(MODULES_DIR)
	cp *.ko $(MODULES_DIR)

modules_clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
	rm -rf $(MODULES_DIR)


endif


