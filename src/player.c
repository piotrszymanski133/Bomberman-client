//
// Created by pioter on 20.04.2020.
//
#include "player.h"


void initPlayer(Board *board,int player_number, int x, int y, int bomb) {
    double tilesPerSecond = 3;

    pthread_mutex_init(&player_lock, NULL);

    player = (Player*)malloc(sizeof(Player));

    player->velocity = tilesPerSecond * (double)board->length / board->size; // tile per second
    player->velX = 0;
    player->velY = 0;
    player->isAlive = 1;

    player->counter = 0;
    player->placedBomb = 0;
    player->bombId = bomb;

    player->image.w = board->length / board->size * 7 / 10;
    player->image.h = player->image.w;

    // Setting up starting position depending on player number
    if(x == 0 || y == 0)
    {
        switch(player_number){
            case 0:
                player->x = board->start_x + board->tile_length / 2;
                player->y = board->start_y + board->tile_length / 2;
                break;
            case 1:
                player->x = board->end_x - board->tile_length / 2;
                player->y = board->end_y - board->tile_length / 2;
                break;
            case 2:
                player->x = board->start_x + board->tile_length / 2;
                player->y = board->end_y - board->tile_length / 2;
                break;
            case 3:
                player->x = board->end_x - board->tile_length / 2;
                player->y = board->start_y + board->tile_length / 2;
                break;
        }
    }
    else
    {
        // player position on reconnection to server
        player->x = x;
        player->y = y;
    }
    player->image.x = (int)player->x - player->image.w / 2;
    player->image.y = (int)player->y - player->image.w / 2;
    player->current_tile = ((int)(player->y - board->start_y) / board->tile_length) * board->size
                           + ((player->x - board->start_x) / board->tile_length);
}

void getAllTiles(int x, int y){
    int corners[4];
    // left top
    corners[0] = ((y - 41 - 40 / 2) / 58) * 11
                 + ((x - 321 - 40 / 2) / 58);
    // right top
    corners[1] = ((y - 41 - 40 / 2) / 58) * 11
                 + ((x - 321 + 40 / 2 - 1) / 58);
    // right bottom
    corners[2] = ((y - 41 + 40 / 2 - 1) / 58) * 11
                 + ((x - 321 + 40 / 2 - 1) / 58);
    // left bottom
    corners[3] = ((y - 41 + 40 / 2 - 1) / 58) * 11
                 + ((x - 321 - 40 / 2) / 58);

}

void loadPlayer(SDL_Window *window, SDL_Renderer *renderer)
{
    SDL_Surface *surface = IMG_Load(PLAYER_SPRITE_PATH);
    if(surface == NULL) {
        printf("Blad przy wczytywaniu plikow!");
        return;
    }
    pthread_mutex_lock(&player_lock);
    player->texture = SDL_CreateTextureFromSurface(renderer, surface);
    pthread_mutex_unlock(&player_lock);
    SDL_FreeSurface(surface);
}

void handlePlayerEvent(SDL_Event *e, SDL_Renderer *renderer, Board *board) {
    // If key was pressed
    if( e->type == SDL_KEYDOWN && e->key.repeat == 0){
        // Adjust velocity (start moving)
        switch (e->key.keysym.sym) {
            /*case SDLK_UP: player->velY -= player->velocity; break;
            case SDLK_DOWN: player->velY += player->velocity; break;
            case SDLK_LEFT: player->velX -= player->velocity; break;
            case SDLK_RIGHT: player->velX += player->velocity; break;*/
            case SDLK_w: player->velY -= player->velocity; break;
            case SDLK_s: player->velY += player->velocity; break;
            case SDLK_a: player->velX -= player->velocity; break;
            case SDLK_d: player->velX += player->velocity; break;
            case SDLK_SPACE: placeBombPlayer(board); break;
        }
    }
    else if( e->type == SDL_KEYUP && e->key.repeat == 0){
        // Adjust velocity (stop moving)
        switch (e->key.keysym.sym) {
            /*case SDLK_UP: player->velY += player->velocity; break;
            case SDLK_DOWN: player->velY -= player->velocity; break;
            case SDLK_LEFT: player->velX += player->velocity; break;
            case SDLK_RIGHT: player->velX -= player->velocity; break;*/
            case SDLK_w: player->velY += player->velocity; break;
            case SDLK_s: player->velY -= player->velocity; break;
            case SDLK_a: player->velX += player->velocity; break;
            case SDLK_d: player->velX -= player->velocity; break;
        }
        sendPlayerData((int)player->x, (int)player->y, &player->counter);
    }
}

void movePlayer(Board* board, double timeStep){
    // Scale for diagonal movement
    double scale = 1.;
    // If the diagonal movement occurs, scale it down
    if(player->velX != 0 && player->velY != 0)
        scale = 1./sqrt(2);

    // Move left or right
    player->x += player->velX * scale * timeStep;
    // Move up or down
    player->y += player->velY * scale * timeStep;

    SDL_Rect collide = player->image;
    collide.x = (int)player->x - player->image.w / 2;
    collide.y = (int)player->y - player->image.h / 2;
    // check collisions with walls
    for(int i = 0; i < 4; i++){
        if(!SDL_HasIntersection(&collide, &board->outsideWalls[i]))
            // no collision happened with wall
            continue;
        else{
            // collision happened with outside wall
            if(i == 0) // top wall
                player->y = board->start_y + player->image.h / 2;
            else if(i == 1) // bottom wall
                player->y = board->end_y - player->image.h / 2;
            if(i == 2) // left wall
                player->x = board->start_x + player->image.w / 2;
            else if(i == 3) // right wall
                player->x = board->end_x - player->image.w / 2;
        }
    }

    // check collisions with ice walls
    for( int i  = 0; i < board->iceBlocksCount; i++){
        if(!SDL_HasIntersection(&collide, &board->iceBlocks[i]))
            // no collision happened with ice block
            continue;
        else{
            // collision happened with ice wall
            /*
            TEMP[i][j] - j:             0                                   1
                    i = 0:      x collision delta                   x callback position
                    i = 1:      y collision delta                   y callback position
                    
            collision delta means how deep is the collision
            it is needed to ignore the unwanted dimension when user try to move in both dimensions
            
            callback position is the position to use if this collision should revert the movement
            */
            double temp[2][2];
            for (int j = 0; j < 2; ++j) {
                temp[j][0] = 0.0f;
            }
            
            // player coordinates point to the center, but coordinates of block point to upper left corner
      
            // player is on the left side of the block
            if(player->x >= board->iceBlocks[i].x - player->image.w / 2 && player->velX > 0.0f) {
                temp[0][0] = fabs(player->x - (board->iceBlocks[i].x - player->image.w / 2));
                temp[0][1] = board->iceBlocks[i].x - player->image.w / 2;
            }
            // player is on the right side of the block
            if(player->x <= board->iceBlocks[i].x + board->iceBlocks[i].w + player->image.w / 2 && player->velX < 0.0f) { 
                temp[0][0] = fabs(player->x - (board->iceBlocks[i].x + board->iceBlocks[i].w + player->image.w / 2));
                temp[0][1] = board->iceBlocks[i].x + board->iceBlocks[i].w + player->image.w / 2;
            }
            // player above the block
            if(player->y >= board->iceBlocks[i].y - player->image.h / 2 && player->velY > 0.0f) {
                temp[1][0] = fabs(player->y - (board->iceBlocks[i].y - player->image.h / 2));
                temp[1][1] = board->iceBlocks[i].y - player->image.h / 2;
            }
            // player below the block
            if(player->y <= board->iceBlocks[i].y + board->iceBlocks[i].w + player->image.w / 2 && player->velY < 0.0f) {
                temp[1][0] = fabs(player->y - (board->iceBlocks[i].y + board->iceBlocks[i].w + player->image.w / 2));
                temp[1][1] = board->iceBlocks[i].y + board->iceBlocks[i].h + player->image.h / 2;
            }
            
            // choose collision with smaller delta and use proper callback
            if (temp[1][0] == 0 || (temp[0][0] != 0 && temp[0][0] <= temp[1][0]))
                player->x = temp[0][1];
            else
                player->y = temp[1][1];
        }
    }

    // check collisions with breakable ice walls
    for( int i  = 0; i < board->breakableIceBlocksCount; i++){
        if(!SDL_HasIntersection(&collide, board->breakableIceBlocks[i]))
            // no collision happened with ice block
            continue;
        else{
            // collision happened with ice wall
            double temp[2][2];
            for (int j = 0; j < 2; ++j) {
                temp[j][0] = 0.0f;
            }

            // player coordinates point to the center, but coordinates of block point to upper left corner

            // player is on the left side of the block
            if(player->x >= board->breakableIceBlocks[i]->x - player->image.w / 2 && player->velX > 0.0f) {
                temp[0][0] = fabs(player->x - (board->breakableIceBlocks[i]->x - player->image.w / 2));
                temp[0][1] = board->breakableIceBlocks[i]->x - player->image.w / 2;
            }
            // player is on the right side of the block
            if(player->x <= board->breakableIceBlocks[i]->x + board->breakableIceBlocks[i]->w + player->image.w / 2 && player->velX < 0.0f) {
                temp[0][0] = fabs(player->x - (board->breakableIceBlocks[i]->x + board->breakableIceBlocks[i]->w + player->image.w / 2));
                temp[0][1] = board->breakableIceBlocks[i]->x + board->breakableIceBlocks[i]->w + player->image.w / 2;
            }
            // player above the block
            if(player->y >= board->breakableIceBlocks[i]->y - player->image.h / 2 && player->velY > 0.0f) {
                temp[1][0] = fabs(player->y - (board->breakableIceBlocks[i]->y - player->image.h / 2));
                temp[1][1] = board->breakableIceBlocks[i]->y - player->image.h / 2;
            }
            // player below the block
            if(player->y <= board->breakableIceBlocks[i]->y + board->breakableIceBlocks[i]->w + player->image.w / 2 && player->velY < 0.0f) {
                temp[1][0] = fabs(player->y - (board->breakableIceBlocks[i]->y + board->breakableIceBlocks[i]->w + player->image.w / 2));
                temp[1][1] = board->breakableIceBlocks[i]->y + board->breakableIceBlocks[i]->h + player->image.h / 2;
            }

            // choose collision with smaller delta and use proper callback
            if (temp[1][0] == 0 || (temp[0][0] != 0 && temp[0][0] <= temp[1][0]))
                player->x = temp[0][1];
            else
                player->y = temp[1][1];
        }
    }

    // check if player went out of bomb
    for(int i = 0; i < 4; i++) {
        if (!SDL_HasIntersection(&collide, &bombs[i]->bombRect)) {
            pthread_mutex_lock(&bombs_lock);
            bombs[i]->underPlayer = 0;
            pthread_mutex_unlock(&bombs_lock);
        }
    }

    // collision with bomb
    for(int i = 0; i < 4; i++) {
        if (!bombs[i]->underPlayer && SDL_HasIntersection(&collide, &bombs[i]->bombRect)) {
            double temp[2][2];
            for (int j = 0; j < 2; ++j) {
                temp[j][0] = 0.0f;
            }

            // player is on the left side of the bomb
            if (player->x >= bombs[i]->bombRect.x - player->image.w / 2 && player->velX > 0.0f) {
                temp[0][0] = fabs(player->x - (bombs[i]->bombRect.x - player->image.w / 2));
                temp[0][1] = bombs[i]->bombRect.x - player->image.w / 2;
            }
            // player is on the right side of the bomb
            if (player->x <= bombs[i]->bombRect.x + bombs[i]->bombRect.w + player->image.w / 2 && player->velX < 0.0f) {
                temp[0][0] = fabs(player->x - (bombs[i]->bombRect.x + bombs[i]->bombRect.w + player->image.w / 2));
                temp[0][1] = bombs[i]->bombRect.x + bombs[i]->bombRect.w + player->image.w / 2;
            }
            // player above the bomb
            if (player->y >= bombs[i]->bombRect.y - player->image.h / 2 && player->velY > 0.0f) {
                temp[1][0] = fabs(player->y - (bombs[i]->bombRect.y - player->image.h / 2));
                temp[1][1] = bombs[i]->bombRect.y - player->image.h / 2;
            }
            // player below the bomb
            if (player->y <= bombs[i]->bombRect.y + bombs[i]->bombRect.w + player->image.w / 2 && player->velY < 0.0f) {
                temp[1][0] = fabs(player->y - (bombs[i]->bombRect.y + bombs[i]->bombRect.w + player->image.w / 2));
                temp[1][1] = bombs[i]->bombRect.y + bombs[i]->bombRect.h + player->image.h / 2;
            }

            // choose collision with smaller delta and use proper callback
            if (temp[1][0] == 0 || (temp[0][0] != 0 && temp[0][0] <= temp[1][0]))
                player->x = temp[0][1];
            else
                player->y = temp[1][1];
        }
    }
    // assign new coords if they didn't go out of bounds
    player->image.x = (int)player->x - player->image.w / 2;
    player->image.y = (int)player->y - player->image.h / 2;

    player->current_tile = ((int)(player->y - board->start_y) / board->tile_length) * board->size
                           + ((player->x - board->start_x) / board->tile_length);
}

void renderPlayer(SDL_Renderer *renderer) {
    //render player
    pthread_mutex_lock(&player_lock); // LOCK
    SDL_RenderCopy(renderer, player->texture, NULL, &player->image);
    pthread_mutex_unlock(&player_lock); // UNLOCK
}

void placeBombPlayer(Board *board){
    // send to server message about bomb placement
    if(player->placedBomb == 0) {
    //    placeBomb(bomb, board, player->current_tile);
        player->placedBomb = 1;
        sendBombEvent(player->current_tile);
    }
}

void closePlayer() {
    if(player->texture != NULL)
        SDL_DestroyTexture(player->texture);
    free(player);
    pthread_mutex_destroy(&player_lock);
}