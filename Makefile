#Se silencian los comandos con @
#%.o cualquier archivo que termine en .o ej regla carpeta1/%.o:carpeta1/%.c el objetivo se llamara igual
#$? pasa todos los requisitos, $< pasa los .c $@ pasa los .o
#\ para dividir lineas en variables por ejemplo
#entra en fcm y ejecuta su make para crear e insertar el módulo flight_control.ko
#después ejecuta make en el directorio principal

#*******USAGE********
# make clean to eliminate binaries and unmount module flight_control.ko
# make to build, if fails, do make clean
EXTRA_CFLAGS += -std=gnu11
PWD := $(shell pwd)

all: main
	make clean -C $(PWD)/fcm
	make -C $(PWD)/fcm
	./main

main: main.c command.h command.c

clean:
	rm -f main
	make clean -C $(PWD)/fcm