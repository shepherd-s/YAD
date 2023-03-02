#include <pthread.h>
#include "common/common.h"

#define PORT 60000

extern pthread_mutex_t command_mutex;
extern char command[XBUFFER_SIZE];

void copy_to(char *from, char *to, size_t size);
void *server_init_fnptr(void *args);