#Se silencian los comandos con @
#%.o cualquier archivo que termine en .o ej regla carpeta1/%.o:carpeta1/%.c el objetivo se llamara igual
#$? pasa todos los requisitos, $< pasa los .c $@ pasa los .o
#entra en FCM y ejecuta su make para crear e insertar el módulo gy91.ko
#después ejecuta make en el directorio principal

all: fcm main exec

fcm: 
	make -C $(PWD)/FCM/

main: main.c

exec:
	./main

clean: remove fcm_clean_mod
	rm -f main

remove:
	rmmod gy91.ko

fcm_clean_mod:
	make clean_mod -C $(PWD)/FCM/
	