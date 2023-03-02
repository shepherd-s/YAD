CFLAGS += -Wa,-mthumb -levl -lpthread -std=gnu11 -O3 -funroll-loops
PWD := $(shell pwd)

all: main
	make -C $(PWD)/fcm
#	./main

main: main.c command.h command.c server_thread.h server_thread.c

test: test.c
	gcc test.c
	./a.out

.PHONY: clean
clean:
	rm -f main
	make clean -C $(PWD)/fcm