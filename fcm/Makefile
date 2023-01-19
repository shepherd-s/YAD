MAKEFLAGS += -k
PWD := $(shell pwd)
KDIR ?= "/lib/modules/$(shell uname -r)/build"
clean_files = .flight_control.ko.cmd .flight_control.mod.cmd .flight_control.mod.o.cmd .flight_control.o.cmd .Module.symvers.cmd \
.modules.order.cmd flight_control.mod flight_control.mod.c flight_control.mod.o flight_control.o Module.symvers modules.order .flight_control.o.d
obj-m += flight_control.o

all: module install insert clean_mod

module: fcm_ops.h fcm_ops.c
	 $(MAKE) -C "$(KDIR)" M="$(CURDIR)" modules

install:
	 $(MAKE) -C "$(KDIR)" M="$(CURDIR)" modules_install

insert:
	insmod flight_control.ko

clean_mod:
	rm -f $(clean_files)

clean:
	rmmod flight_control.ko;rm -f flight_control.ko
