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
}