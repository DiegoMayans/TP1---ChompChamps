#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>  // For timestamps
#include <unistd.h>

#include "../../includes/defs.h"
#include "../../includes/master.h"
#include "../../includes/shm_adt.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

void move(int direction);
int find_best_move(game_board_t *board, int player_index);

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    pid_t pid = getpid();

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "Error: ancho y alto deben ser valores positivos.\n");
        exit(EXIT_FAILURE);
    }

    shm_adt shm_board = shm_open_readonly(GAME_STATE_PATH, sizeof(game_board_t) + sizeof(int) * height * width);
    game_board_t *board = shm_get_game_board(shm_board);
    shm_adt shm_sync = shm_open_readwrite(GAME_SYNC_PATH, sizeof(game_sync_t));
    game_sync_t *sync = shm_get_game_sync(shm_sync);
    bool should_find_best_move(game_board_t * board, int my_index, int prev_x, int prev_y, int prev_move);

    int first_read = 1, my_index = -1, best_move = -1, prev_x = -1, prev_y = -1, prev_move = -1;
    while (true) {
        sem_wait(&sync->access_queue);
        sem_wait(&sync->count_access);
        sync->players_reading_count++;
        if (sync->players_reading_count == 1) {
            sem_wait(&sync->game_state_access);
        }
        sem_post(&sync->count_access);
        sem_post(&sync->access_queue);

        if (first_read) {
            for (int i = 0; i < board->player_count; i++) {
                if (board->players_list[i].pid == pid) {
                    my_index = i;
                    break;
                }
            }
            first_read = 0;
        }

        if (board->game_has_finished) {
            sem_wait(&sync->count_access);
            sync->players_reading_count--;
            if (sync->players_reading_count == 0) {
                sem_post(&sync->game_state_access);
            }
            sem_post(&sync->count_access);
            break;
        }

        if (should_find_best_move(board, my_index, prev_x, prev_y, prev_move)) {
            best_move = find_best_move(board, my_index);
        } else {
            best_move = -1;
        }

        prev_x = board->players_list[my_index].x;
        prev_y = board->players_list[my_index].y;
        prev_move = best_move;

        sem_wait(&sync->count_access);
        sync->players_reading_count--;
        if (sync->players_reading_count == 0) {
            sem_post(&sync->game_state_access);
        }
        sem_post(&sync->count_access);

        move(best_move);
    }

    shm_close(shm_board);
    shm_close(shm_sync);
    return 0;
}

void move(int direction) {
    if (direction != -1) {
        write(1, &direction, sizeof(unsigned char));
    }
    usleep(400 * 1000);  // Damos tiempo a que el master
}

int find_best_move(game_board_t *board, int player_index) {
    int x = board->players_list[player_index].x;
    int y = board->players_list[player_index].y;

    int best_move = -1;
    int best_score = -1;

    for (int i = 0; i < 8; i++) {
        int new_x = x;
        int new_y = y;
        set_coordinates(&new_x, &new_y, i);

        if (new_x < 0 || new_x >= board->width || new_y < 0 || new_y >= board->height ||
            board->board[new_x + new_y * board->width] <= 0) {
            continue;
        }

        int score = board->board[new_x + new_y * board->width];

        if (score > best_score) {
            best_score = score;
            best_move = i;
        }
    }

    return best_move;
}

bool should_find_best_move(game_board_t *board, int my_index, int prev_x, int prev_y, int prev_move) {
    if (prev_x != board->players_list[my_index].x || prev_y != board->players_list[my_index].y) {
        return true;
    }

    if (prev_move != -1) {
        int new_x = prev_x;
        int new_y = prev_y;
        set_coordinates(&new_x, &new_y, prev_move);

        if (board->board[new_x + new_y * board->width] < 0) {
            return true;
        }
    }

    return false;
}
