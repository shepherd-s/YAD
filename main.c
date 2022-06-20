#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "FCM/common.h"



int main (int argc, char *argv[]) {
    int salida = getpid();
    int max = GY91MAXBYTES;

    printf("%d\nMAX: %d\n", salida, max);

    if (0) {
        printf("%d\n MAX: %d\n", salida, max);
    }

    asm
    (
        "main_asm:\n\t"
            "ldr r0, =0x01c20000\n\t"           //base dir of ccu
            "ldr r1, =0x00a0\n\t"               //offset
            "ldr r2, =0x1\n\t"                         
            "str r2, [r0, r1]\n\t"
    );
    
} 