//
// Created by ves on 20/04/2020.
//

#ifndef BOMBERMAN_CLIENT_CONNECTION_H
#define BOMBERMAN_CLIENT_CONNECTION_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

typedef struct connection_manager_struct{
    int socket_desc;
    struct addrinfo hints;
    struct addrinfo *p;
}Connection;

void initSocket(Connection* conn);


// add argument to pass port number for TCP connection
void test_connection();

#endif //BOMBERMAN_CLIENT_CONNECTION_H
