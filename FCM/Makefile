MAKEFLAGS += -k
PWD   := $(shell pwd)
clean_files = .gy91.ko.cmd .gy91.mod.cmd .gy91.mod.o.cmd .gy91.o.cmd .Module.symvers.cmd \
.modules.order.cmd gy91.mod gy91.mod.c gy91.mod.o gy91.o Module.symvers modules.order
obj-m += gy91.o

all: module install insert clean

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install

insert:
	insmod gy91.ko

clean:
	rm -f $(clean_files)