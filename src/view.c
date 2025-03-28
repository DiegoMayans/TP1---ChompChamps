#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include "../includes/defs.h"
#include "../includes/shm.h"
#include "../includes/view.h"
#include "math.h"


int main(int argc, char *argv[]){

    if (argc != 3){
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int ancho = atoi(argv[1]);
    int alto = atoi(argv[2]);

    if (ancho <= 0 || alto <= 0)
    {
        fprintf(stderr, "Error: ancho y alto deben ser valores positivos.\n");
        exit(EXIT_FAILURE);
    }


    game_board_t *board_state = get_board_state(alto * ancho);
    if (board_state == NULL) {
        fprintf(stderr, "Error: No se pudo obtener el estado del tablero\n");
        exit(EXIT_FAILURE);
    }

    game_sync_t *game_sync = get_sync();
    if (game_sync == NULL) {
        fprintf(stderr, "Error: No se pudo obtener la sincronización\n");
        exit(EXIT_FAILURE);
    }

    sem_t *sem_A = &game_sync->print_needed; //de aca sabe si hay cambios para imprimir
    sem_t *sem_B = &game_sync->print_done; 

    while (!board_state->game_has_finished)
    {
        sem_wait(sem_A); 

        clear_screen();
        print_board(board_state, alto, ancho);
        print_stats(board_state);

        sem_post(sem_B);
    }

    return 0;


}

void print_board(game_board_t * board_state, int alto, int ancho){
    for (int i=0; i < alto; i++){
        for (int j=0; j < ancho; j++){
            if (board_state->board[i * (board_state->width) + j] < 0){
                int player = abs(board_state->board[i * (board_state->width) + j]);
                if ((i == board_state->players_list[player].x) && (j == board_state->players_list[player].y)){
                    printf("\033[38;5;%dm| %d |\033[0m", head_colors[player], board_state->board[i * (board_state->width) + j]);
                } else {
                    printf("\033[38;5;%dm| %d |\033[0m", colors[player], board_state->board[i * (board_state->width) + j]);
                }
            } else {
                printf("| %d |", board_state->board[i * (board_state->width) + j]);
            }
        }
        printf("\n");
    }
    printf("\n");
}

void clear_screen(){
    printf("\033[H\033[J");
}
void print_stats(game_board_t *board_state) {
    printf("\n==== Estadísticas de Jugadores ====\n");
    for (int i = 0; i < board_state->player_count; i++) {
        printf("\033[38;5;%dmJugador %d:\033[0m\n", colors[i], i);
        printf("  - Posición: (%d, %d)\n", board_state->players_list[i].x, board_state->players_list[i].y);
        printf("  - Puntaje: %d\n", board_state->players_list[i].score);
        // printf("  - Movimientos: %d\n", board_state->players_list[i].moves);
        // printf("  - Estado: %s\n", board_state->players_list[i].active ? "Activo" : "Inactivo");
        printf("---------------------------\n");
    }
}