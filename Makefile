CFLAGS += -Wa,-mthumb -levl -lpthread -std=gnu11 -O3 -funroll-loops
PWD := $(shell pwd)

all: yad
	make -C $(PWD)/fcm

yad: yad.c command.h command.c server_thread.h server_thread.c

.PHONY: clean
clean:
	rm -f main
	make clean -C $(PWD)/fcm