// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <unistd.h>

#include "../../includes/defs.h"
#include "../../includes/shm_adt.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

void move(direction_t direction);

int main(int argc, char *argv[]) {
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
    while (!board->game_has_finished) {
        int rand_num = RIGHT;

        sem_wait(&sync->access_queue);
        sem_wait(&sync->count_access);
        sync->players_reading_count++;
        if (sync->players_reading_count == 1) {
            sem_wait(&sync->game_state_access);
        }
        sem_post(&sync->count_access);
        sem_post(&sync->access_queue);

        // Sección crítica: Lectura
        sleep(1);

        sem_wait(&sync->count_access);
        sync->players_reading_count--;
        if (sync->players_reading_count == 0) {
            sem_post(&sync->game_state_access);
        }
        sem_post(&sync->count_access);

        move(rand_num);
    }

    shm_close(shm_board);
    shm_close(shm_sync);
    return 0;
}

void move(direction_t direction) { write(1, &direction, sizeof(unsigned char)); }
