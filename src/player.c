#include "../includes/defs.h"
#include "../includes/shm.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    int shm_fd = shm_open("/game_state", O_RDONLY, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    void *smh_ptr = mmap(NULL, sizeof(player_t) + sizeof(game_board_t), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (smh_ptr == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    game_board_t *board = (game_board_t *)smh_ptr;

    // Solicitar movimiento
    unsigned char move_request = RIGHT;
    write(1, &move_request, sizeof(unsigned char));

    // Clean up
    munmap(smh_ptr, sizeof(player_t) + sizeof(game_board_t));
    close(shm_fd);
}
