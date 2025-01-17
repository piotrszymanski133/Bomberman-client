//
// Created by Kacper on 20/04/2020.
//

#include "connection.h"

void initConnection(char *name/*, char *port*/) {
    conn = (Connection*)malloc(sizeof(Connection));

    pthread_mutex_init(&tick_lock, NULL);
    memset(&conn->hints, 0, sizeof(conn->hints));
    conn->hints.ai_family = AF_INET;
    conn->hints.ai_socktype = SOCK_STREAM;
    conn->hints.ai_flags = 0;
    conn->hints.ai_protocol = 0;

    conn->name = name;
    /*conn->port = port;*/
    conn->closeConnection = 0;
    conn->connectionEstablished = 0;
    conn->player_count = 1;
}

void connectServer() {
    pthread_create(&conn->thread, NULL, communication, NULL);
}

void *communication(void *args) {

    while (conn->closeConnection == 0) {
        // get possible IPs of server
        /*int result = getaddrinfo("serveo.net", conn->port, &conn->hints, &conn->infoptr);
        if (result) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        }

        for (conn->p = conn->infoptr; conn->p != NULL; conn->p = conn->p->ai_next) {
            conn->socket = socket(conn->p->ai_family, conn->p->ai_socktype, conn->p->ai_protocol);

            if (conn->socket == -1)
                continue;

            // trying to connect to consecutive possible addresses
            if (connect(conn->socket, conn->p->ai_addr, conn->p->ai_addrlen) != -1) {
                puts("Connected\n");
                break;      // Success
            }

            close(conn->socket);
        }*/

        // create socket
        struct sockaddr_in server;
        conn->socket = socket(AF_INET , SOCK_STREAM , 0);
        if (conn->socket == -1)
        {
            printf("Could not create socket");
        }

        server.sin_addr.s_addr = inet_addr("87.207.30.151");
        server.sin_family = AF_INET;
        server.sin_port = htons( 5000 );

        // try to connect to server
        if (connect(conn->socket, (struct sockaddr *)&server , sizeof(server)) < 0)
        {
            puts("connect error");
        }
        // none of possible addresses connected
        /*if (conn->p == NULL) {
            fprintf(stderr, "Could not find the server!\n");
        }*/
        else {
            // connection successful

            conn->connectionEstablished = 1;
            fprintf(stdout, "Connection stable.\n");

            // send name to server
            sendName();

            char buffer[2048];
            // receive information from server
            while (recv(conn->socket, buffer, sizeof(buffer), 0) > 0 && conn->closeConnection == 0) {
                //printf("%s\n", buffer);
                // decode message from server
                decodeMessage(buffer);
                // clear buffer
                memset(buffer, 0, sizeof(buffer));
            }
        }
        // connection lost or couldn't connect
        conn->connectionEstablished = 0;
        if(conn->closeConnection == 0) {
            fprintf(stderr, "Connection lost, attempting reconnect...\n");
            close(conn->socket);
            sleep(5);
        }
    }
    fprintf(stdout, "Connection to server closed.");
    close(conn->socket);
    return NULL;
}

void decodeMessage(char *message) {
    // msg - id of message
    int msg, buff_length;
    char* buff_ptr = message;
    // loop till end of message
    while(*buff_ptr) {
        // get msg id
        sscanf(buff_ptr, "%d%n", &msg, &buff_length);
        buff_ptr += buff_length;
        switch (msg) {
            case start_msg: {
                int enemy_c = 0;
                int player_number;
                // get player count
                sscanf(buff_ptr, "%d\n%n", &conn->player_count, &buff_length);
                buff_ptr += buff_length;
                for (int i = 0; i < conn->player_count; i++) {
                    char name[100];
                    int x, y;
                    // read player number, name and starting positions
                    sscanf(buff_ptr, "%d %s %d %d\n%n", &player_number, name, &x, &y, &buff_length);
                    if (strcmp(name, conn->name) == 0) {
                        // initialise player and their bomb
                        pthread_mutex_lock(&player_lock);
                        initPlayer(board, player_number, x, y, i);
                        pthread_mutex_unlock(&player_lock);
                        initBomb(bombs[i]);
                        pthread_mutex_lock(&renderer_lock);
                        loadPlayer(window->gWindow, window->gRenderer);
                        loadBomb(bombs[i], window->gRenderer);
                        pthread_mutex_unlock(&renderer_lock);
                    } else {
                        // initialise enemy and their bomb
                        pthread_mutex_lock(&enemy_lock);
                        initEnemy(enemies[enemy_c], board, player_number, x, y, name, i);
                        pthread_mutex_unlock(&enemy_lock);
                        initBomb(bombs[i]);
                        pthread_mutex_lock(&renderer_lock);
                        loadEnemy(window->gRenderer, enemies[enemy_c], enemy_c);
                        loadBomb(bombs[i], window->gRenderer);
                        pthread_mutex_unlock(&renderer_lock);
                        enemy_c++;
                    }
                    buff_ptr += buff_length;
                }
                // start game signal
                board->startGame = 1;
            }
                break;
            case players_msg: {
                if (board->startGame == 0)
                    break;
                int playerc;
                // amount of players to check in message
                sscanf(buff_ptr, "%d\n%n", &playerc, &buff_length);
                buff_ptr += buff_length;
                for(int i = 0; i < playerc; i++) {
                    char name[100];
                    int x, y, isAlive;
                    sscanf(buff_ptr, "%s %d %d %d\n%n", name, &x, &y, &isAlive, &buff_length);
                    if (strcmp(conn->name, name) == 0) {
                        buff_ptr += buff_length;
                        pthread_mutex_lock(&player_lock);
                        player->isAlive = isAlive;
                        pthread_mutex_unlock(&player_lock);
                        continue;
                    }
                    for (int i = 0; i < conn->player_count - 1; i++) {
                        if (strcmp(name, enemies[i]->name) == 0) {
                            pthread_mutex_lock(&enemy_lock);
                            enemies[i]->isAlive = isAlive;
                            if (x != 0 && y != 0) {
                                enemies[i]->nextX = x;
                                enemies[i]->nextY = y;
                                enemies[i]->stepX = (enemies[i]->nextX - enemies[i]->x) / 6;
                                enemies[i]->stepY = (enemies[i]->nextY - enemies[i]->y) / 6;
                                enemies[i]->stepCounter = 0;
                            }
                            pthread_mutex_unlock(&enemy_lock);
                        }
                    }
                    buff_ptr += buff_length;
                }
            }
                break;
            case bombs_msg: {
                if (board->startGame == 0)
                    break;
                int bombsc, tick_number;
                sscanf(buff_ptr, "%d %d\n%n", &bombsc, &tick_number, &buff_length);
                pthread_mutex_lock(&tick_lock);
                actualTick = tick_number;
                pthread_mutex_unlock(&tick_lock);
                buff_ptr += buff_length;
                for(int i = 0; i < bombsc; i++) {
                    char name[100];
                    int tile_number, explode_tick, end_of_explosion_tick;
                    sscanf(buff_ptr, "%s %d %d %d\n%n", name, &tile_number, &explode_tick, &end_of_explosion_tick, &buff_length);
                    if (strcmp(conn->name, name) == 0) {
                        pthread_mutex_lock(&bombs_lock);
                        placeBomb((bombs[player->bombId]), board, tile_number, explode_tick, end_of_explosion_tick);
                        pthread_mutex_unlock(&bombs_lock);
                        buff_ptr += buff_length;
                        continue;
                    }
                    for (int i = 0; i < conn->player_count - 1; i++) {
                        if (strcmp(name, enemies[i]->name) == 0){
                            pthread_mutex_lock(&bombs_lock);
                            placeBomb((bombs[enemies[i]->bombId]), board, tile_number, explode_tick, end_of_explosion_tick);
                            pthread_mutex_unlock(&bombs_lock);
                        }
                    }
                    buff_ptr += buff_length;
                }
            }
                break;
            case walls_msg:
            {
                char status[121];
                for(int i = 0; i < 121; i++) {
                    sscanf(buff_ptr, "%d %n", &status[i], &buff_length);
                    buff_ptr += buff_length;
                }
                loadBreakable(status);
            }
                break;
            case destroy_msg:
            {
                int wallc;
                sscanf(buff_ptr, "%d\n%n", &wallc, &buff_length);
                buff_ptr += buff_length;
                pthread_mutex_lock(&board_lock);
                for(int i = 0; i < wallc; i++){
                    int index;
                    sscanf(buff_ptr, "%d %n", &index, &buff_length);
                    destroyBreakableIceBlock(index);
                    buff_ptr += buff_length;
                }
                pthread_mutex_unlock(&board_lock);
            }
                break;
            default:
                break;
        }
    }
}

void sendName() {
    if (conn->connectionEstablished == 1) {
        char buffer[100];
        sprintf(buffer, "%d %s", name_msg, conn->name);
        send(conn->socket, buffer, strlen(buffer), 0);
    }
}

void sendPlayerData(int x, int y, unsigned int *action_counter) {
    if (conn->connectionEstablished == 1) {
        // convert data to ANSI string
        char buffer[100];
        sprintf(buffer, "%d %s %d %d %d\n", move_msg, conn->name, x, y, (*action_counter)++);
        send(conn->socket, buffer, strlen(buffer), 0);
    }
}

void sendBombEvent(int tile) {
    if(conn->connectionEstablished == 1){
        char buffer[100];
        sprintf(buffer, "%d %s %d\n", bomb_msg, conn->name, tile);
        send(conn->socket, buffer, strlen(buffer), 0);
    }
}


void closeConnection() {
    conn->closeConnection = 1;
    //close(conn->socket);
    pthread_join(conn->thread, NULL);
}

void closeConnStruct() {
    // freeaddrinfo(conn->infoptr);
    free(conn);
    pthread_mutex_destroy(&tick_lock);
}
