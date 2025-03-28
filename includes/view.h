#include "defs.h"
#include "stdio.h"
#include "stdlib.h"

const int colors[] = {209, 200, 191, 86, 140, 56, 29, 95, 221};


const int head_colors[] = {208, 201, 190, 87, 141, 57, 28, 94, 220};


void print_board(game_board_t * board_state, int alto, int ancho);

void print_stats(game_board_t * board_state);

void clear_screen();