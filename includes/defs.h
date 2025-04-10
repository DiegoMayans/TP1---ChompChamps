#ifndef DEFS_H
#define DEFS_H

#define GAME_STATE_PATH "/game_state"
#define GAME_SYNC_PATH "/game_sync"

#define CANT_MOVES 8

typedef enum {
    UP = 0,
    UP_RIGHT = 1,
    RIGHT = 2,
    DOWN_RIGHT = 3,
    DOWN = 4,
    DOWN_LEFT = 5,
    LEFT = 6,
    UP_LEFT = 7
} direction_t;

#define MIN_MOVE UP
#define MAX_MOVE UP_LEFT
#define CANT_DIRECTIONS 8

#define MAX_PLAYERS 9
#define MAX_VIEWS 1
#define INITIAL_FD_COUNT 3
#define MIN_WIDTH 10
#define MIN_HEIGHT 10
#define MAX_DIGITS 3

#endif