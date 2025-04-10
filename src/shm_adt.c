#include "../includes/shm_adt.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../includes/defs.h"

struct shm_cdt {
    int fd;
    void *ptr;
    size_t size;
};

static void *map_memory(int fd, size_t size, int prot) {
    void *p = mmap(NULL, size, prot, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    return p;
}

shm_adt shm_create(const char *name, size_t size) {
    shm_adt shm = malloc(sizeof(struct shm_cdt));
    if (!shm) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    shm->size = size;
    shm->fd = shm_open(name, O_RDWR | O_CREAT, 0666);
    if (shm->fd == -1) {
        perror("shm_open");
        free(shm);
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm->fd, size) == -1) {
        perror("ftruncate");
        close(shm->fd);
        free(shm);
        exit(EXIT_FAILURE);
    }

    shm->ptr = map_memory(shm->fd, size, PROT_READ | PROT_WRITE);
    return shm;
}

shm_adt shm_open_readonly(const char *name, size_t size) {
    shm_adt shm = malloc(sizeof(struct shm_cdt));
    if (!shm) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    shm->size = size;
    shm->fd = shm_open(name, O_RDONLY, 0644);
    if (shm->fd == -1) {
        perror("shm_open (readonly)");
        free(shm);
        exit(EXIT_FAILURE);
    }

    shm->ptr = map_memory(shm->fd, size, PROT_READ);
    return shm;
}

shm_adt shm_open_readwrite(const char *name, size_t size) {
    shm_adt shm = malloc(sizeof(struct shm_cdt));
    if (!shm) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    shm->size = size;
    shm->fd = shm_open(name, O_RDWR, 0666);
    if (shm->fd == -1) {
        perror("shm_open (readwrite)");
        free(shm);
        exit(EXIT_FAILURE);
    }

    shm->ptr = map_memory(shm->fd, size, PROT_READ | PROT_WRITE);
    return shm;
}

void shm_close(shm_adt shm) {
    if (!shm) return;

    if (munmap(shm->ptr, shm->size) == -1) {
        perror("munmap");
    }
    close(shm->fd);
    free(shm);
}

game_board_t *shm_get_game_board(shm_adt shm) { return (game_board_t *)shm->ptr; }

game_sync_t *shm_get_game_sync(shm_adt shm) { return (game_sync_t *)shm->ptr; }
