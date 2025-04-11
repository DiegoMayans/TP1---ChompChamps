#include <unistd.h>

#include "../../includes/defs.h"
#include "../../includes/shm_adt.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

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
    // shm_adt shm_sync = shm_open_readwrite(GAME_SYNC_PATH, sizeof(game_sync_t));
    // game_sync_t *sync = shm_get_game_sync(shm_sync);

    // shm->size = size;
    // shm->fd = shm_open(name, O_RDWR, 0666);
    // if (shm->fd == -1) {
    //     perror("shm_open_rdwr");
    //     free(shm);
    //     return NULL;
    // }

    // shm->ptr = map_memory(shm->fd, size, PROT_READ | PROT_WRITE);
    // if (!shm->ptr) {
    //     close(shm->fd);
    //     free(shm);
    //     return NULL;
    // }
    int fd = shm_open(GAME_SYNC_PATH, O_RDWR, 0666);
    game_sync_t *sync = mmap(NULL, sizeof(game_sync_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    direction_t move_to = 0;
    while (!board->game_has_finished) {
        sem_wait(&sync->access_queue);
        sem_wait(&sync->count_access);
        sync->players_reading_count++;
        if (sync->players_reading_count == 1) {
            sem_wait(&sync->game_state_access);
        }
        sem_post(&sync->count_access);
        sem_post(&sync->access_queue);

        // Sección crítica: Lectura
        usleep(100 * 1000);

        sem_wait(&sync->count_access);
        sync->players_reading_count--;
        if (sync->players_reading_count == 0) {
            sem_post(&sync->game_state_access);
        }
        sem_post(&sync->count_access);

        move_to = (move_to + 1) % 8;
        move(move_to);
    }

    shm_close(shm_board);
    // shm_close(shm_sync);
    return 0;
}

void move(direction_t direction) { write(1, &direction, sizeof(unsigned char)); }
