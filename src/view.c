#include <stdio.h>
#include <unistd.h>  
#include <defs.h>
#include<shm.h>
#include<stdint.h>
#include<stdlib.h>

int main(int argc, char *argv[]){

    int ancho = atoi(argv[1]);
    int alto = atoi(argv[2]);
    
    if (ancho <= 0 || alto <= 0) {
        fprintf(stderr, "Error: ancho y alto deben ser valores positivos.\n");
        exit(EXIT_FAILURE);
    }

    game_board_t *board_state = get_board_state();
    game_sync_t *game_sync = get_sync();
    
    return 0;
}

