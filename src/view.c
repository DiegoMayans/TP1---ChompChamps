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

    // La vista debe imprimir el estado del juego
    // Primero imprimira el tablero en forma de grilla donde cada celda es un caracter. El caracter 
    // a imprimir es el valor de la celda, es decir, board_state->board[x][y]
    // A continuacion imprime los jugadores en una lista

    while (1)
    {
        //sem_wait(&game_sync->print_needed);
        //sem_wait(&game_sync->game_state_access);

        // Imprimir tablero
        for (int i = 0; i < alto; i++)
        {
            for (int j = 0; j < ancho; j++)
            {
                printf("%c", board_state->board[i * ancho + j]);
            }
            printf("\n");
        }

        // Imprimir jugadores
        for (int i = 0; i < board_state->player_count; i++)
        {
            printf("Jugador %s: %d puntos\n", board_state->players_list[i].player_name, board_state->players_list[i].score);
        }

        //sem_post(&game_sync->print_done);
        //sem_post(&game_sync->game_state_access);
    }
    
    return 0;
}

