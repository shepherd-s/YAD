CFLAGS += -Wa,-mthumb -std=gnu11 -O3 -funroll-loops
PWD := $(shell pwd)
clean_files = .flight_control.ko.cmd .flight_control.mod.cmd .flight_control.mod.o.cmd \
.flight_control.o.cmd .Module.symvers.cmd .modules.order.cmd flight_control.mod \
flight_control.mod.c flight_control.mod.o flight_control.o \
Module.symvers modules.order .flight_control.o.d .fcm_ops.o.cmd

obj-m += fcm.o
fcm-objs := flight_control.o motor_thread.o

all: module install clean

module:
	make -C "/lib/modules/$(shell uname -r)/build" M="$(PWD)" modules

install:
	make -C "/lib/modules/$(shell uname -r)/build" M="$(PWD)" modules_install
	cp fcm.ko /root/YAD
ifeq ($(shell lsmod | grep fcm | cut -d" " -f1), fcm)
	rmmod fcm.ko
endif
	insmod fcm.ko

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

	