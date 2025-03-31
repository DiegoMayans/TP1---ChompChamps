#include "../includes/shm.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../includes/defs.h"

// codigo que va para el master
void *createSHM(char *name, size_t size) {
    int fd;
    fd = shm_open(name, O_RDWR | O_CREAT, 0666);  // mode
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Solo para crearla
    if (-1 == ftruncate(fd, size)) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return p;
}

game_board_t *get_board_state(size_t tam_tablero) {
    int fd_state;
    fd_state = shm_open(GAME_STATE_PATH, O_RDONLY, 0644);
    if (fd_state == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    game_board_t *board =
        mmap(NULL, sizeof(game_board_t) * tam_tablero * sizeof(int), PROT_READ, MAP_SHARED, fd_state, 0);
    if (board == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return board;
}

game_sync_t *get_sync() {
    int fd_sync;
    fd_sync = shm_open(GAME_SYNC_PATH, O_RDWR, 0666);
    if (fd_sync == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    game_sync_t *sync = mmap(NULL, sizeof(game_sync_t), PROT_WRITE | PROT_READ, MAP_SHARED, fd_sync, 0);
    if (sync == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return sync;
}
