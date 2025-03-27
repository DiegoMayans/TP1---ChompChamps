#ifndef SHM_H
#define SHM_H
#include "defs.h"

void *createSHM(char *name, size_t size);

game_board_t *get_board_state();

game_sync_t *get_sync();

#endif