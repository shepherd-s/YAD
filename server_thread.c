/**
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2023 Shepherd <shepherdsoft@outlook.com>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "server_thread.h"

pthread_mutex_t command_mutex;
char command[XBUFFER_SIZE] = {0};

void copy_to(char *from, char *to, size_t size)
{
    for (int i = 0; i < size; i++) {
        to[i] = from[i];
    }
}

/**
* @brief Creates an UDP socket in the specified port in the
*        Internet namespace
*
* @param port the port number of the socket
*/
int make_server_socket(uint16_t port)
{
    int sock;
    struct sockaddr_in name;

    /* Create the socket. */
    sock = socket (PF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror ("socket");
        exit (EXIT_FAILURE);
    }

    /* Give the socket a name. */
    name.sin_family = AF_INET;
    name.sin_port = htons (port);
    name.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind(sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    } 

    return sock;
}

/**
* @brief Function pointer for the server thread
*/
void *server_init_fnptr(void *args)
{
    int sock;
    struct sockaddr_in clname;
    size_t size;
    int nbytes;
    char buf[XBUFFER_SIZE] = {0};

    size = sizeof (clname);
    sock = make_server_socket(PORT);

    while (1) {
        nbytes = recvfrom(sock, &buf, XBUFFER_SIZE, 0, (struct sockaddr*) &clname, &size);
        pthread_mutex_lock(&command_mutex);
        copy_to((char*) &buf, (char*) &command, XBUFFER_SIZE);
        pthread_mutex_unlock(&command_mutex);
        usleep(2000);
    }
}
