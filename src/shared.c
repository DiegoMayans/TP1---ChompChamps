// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "../includes/shared.h"

void set_coordinates(int *x, int *y, direction_t move) {
    static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

    *x += dx[move];
    *y += dy[move];
}

void test_exit(const char *msg, int condition) {
    if (condition) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

int is_valid_move(game_board_t *game_board, char move, int player_index) {
    if (move < MIN_MOVE || move > MAX_MOVE) {
        return 0;
    }

    int x = game_board->players_list[player_index].x;
    int y = game_board->players_list[player_index].y;
    set_coordinates(&x, &y, move);

    if (x < 0 || x >= game_board->width || y < 0 || y >= game_board->height) {
        return 0;
    }

    if (game_board->board[x + y * game_board->width] <= 0) {
        return 0;
    }

    return 1;
}