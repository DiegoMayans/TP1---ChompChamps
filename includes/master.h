#ifndef MASTER_H
#define MASTER_H

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../includes/defs.h"
#include "../includes/shm_adt.h"
#include "../includes/round_robin_adt.h"

typedef struct {
    char height[MAX_DIGITS], width[MAX_DIGITS];
    int delay;
    int timeout;
    int seed;
} argument_t;

pid_t create_process(char *executable, char *height, char *width, int fd[2], int redirect_stdout);
void parse_arguments(argument_t *arguments, int argc, char *argv[]);
int parse_childs(int argc, char *argv[], argument_t *arguments, requester_t players_read_fds[], pid_t pid_list[]);
void init_board(game_board_t *game_board, int width, int height, int players_count, pid_t pid_list[]);
void init_sync(game_sync_t *game_sync);
void initialize_player(player_t *player, pid_t pid);
void initialize_player_positions(game_board_t *game_board, int width, int height, int players_count);
int is_valid_move(game_board_t *game_board, char move, int player_index);
void set_coordinates(int *x, int *y, direction_t move);
void update_player(game_board_t *game_board, direction_t move, int player_index);
// int has_valid_moves(game_board_t *game_board, int player_index);
void test_exit(const char *msg, int condition);
void safe_exit(const char *msg, int condition, shm_adt shm_board, shm_adt shm_sync, requester_t fds[], int players_count);

#endif
