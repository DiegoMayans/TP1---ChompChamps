#ifndef VIEW_H
#define VIEW_H

#include "../includes/shm_adt.h"


const int colors[] = {210, 199, 192, 85, 144, 55, 30, 96, 222};

const int head_colors[] = {208, 201, 190, 87, 142, 57, 28, 94, 220};

void print_board(game_board_t *board_state, int alto, int ancho);

void print_winner_and_stats(game_board_t *board);

void clear_screen(bool full_clear);

#endif