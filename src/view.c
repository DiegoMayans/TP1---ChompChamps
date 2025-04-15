// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "../includes/view.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../includes/defs.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    int height = atoi(argv[1]);
    int width = atoi(argv[2]);

    if (height <= 0 || width <= 0) {
        fprintf(stderr, "Error: ancho y alto deben ser valores positivos.\n");
        exit(EXIT_FAILURE);
    }

    shm_adt shm_board = shm_open_readonly(GAME_STATE_PATH, sizeof(game_board_t) + sizeof(int) * height * width);
    game_board_t *game_board = shm_get_game_board(shm_board);
    shm_adt shm_sync = shm_open_readwrite(GAME_SYNC_PATH, sizeof(game_sync_t));
    game_sync_t *game_sync = shm_get_game_sync(shm_sync);

    sem_t *sem_A = &game_sync->print_needed;  // de aca sabe si hay cambios para imprimir
    sem_t *sem_B = &game_sync->print_done;

    clear_screen(true);
    while (!game_board->game_has_finished) {
        sem_wait(sem_A);

        clear_screen(false);
        print_board(game_board, height, width);

        sem_post(sem_B);
    }

    print_winner_and_stats(game_board);

    shm_close(shm_board);
    shm_close(shm_sync);

    return 0;
}

void print_board(game_board_t *board_state, int height, int width) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (board_state->board[(i * (board_state->width)) + j] <= 0) {
                int player = abs(board_state->board[(i * (board_state->width)) + j]);
                if (player < MAX_PLAYERS) {
                    if ((i == board_state->players_list[player].y) && (j == board_state->players_list[player].x)) {
                        printf("\033[38;5;%dm [:) \033[0m", head_colors[player]);
                    } else {
                        printf("\033[38;5;%dm ### \033[0m", colors[player]);
                    }
                }
            } else {
                printf("|%2d |", board_state->board[(i * (board_state->width)) + j]);
            }
        }
        printf("\n");
    }
    printf("\n");
}

void clear_screen(bool full_clear) {
    if (full_clear) {
        printf("\033[H\033[2J\033[3J");  // Limpieza completa
    } else {
        printf("\033[H");  // Solo mover cursor
    }
}

void print_stats(game_board_t *board_state) {
    printf("\n==== Estadísticas de Jugadores ====\n");
    for (int i = 0; i < board_state->player_count; i++) {
        printf("\033[38;5;%dm%s %d:\033[0m\n", colors[i], board_state->players_list[i].player_name, i);
        printf("  - Posición: (%d, %d)\n", board_state->players_list[i].x, board_state->players_list[i].y);
        printf("  - Puntaje: %d\n", board_state->players_list[i].score);
        printf("  - Movimientos validos: %d\n", board_state->players_list[i].move_req_count);
        printf("  - Movimientos invalidos: %d\n", board_state->players_list[i].invalid_move_req_count);
        printf("---------------------------\n");
    }
}

void print_winner_and_stats(game_board_t *board) {
    clear_screen(true);
    int max_score = -1;
    int winner = -1;
    for (int i = 0; i < board->player_count; i++) {
        int player_score = board->players_list[i].score;
        if (player_score > max_score) {
            max_score = board->players_list[i].score;
            winner = i;
        } else if (player_score == max_score) {
            if (board->players_list[i].move_req_count < board->players_list[winner].move_req_count) {
                winner = i;
            } else if (board->players_list[i].move_req_count == board->players_list[winner].move_req_count) {
                if (board->players_list[i].invalid_move_req_count <
                    board->players_list[winner].invalid_move_req_count) {
                    winner = i;
                } else if (board->players_list[i].invalid_move_req_count ==
                           board->players_list[winner].invalid_move_req_count) {
                    winner = -1;
                }
            }
        }
    }

    if (winner != -1) {
        printf("\n");
        printf("========================================================\n");
        printf("             FELICIDADES, JUGADOR %d!            \n", winner);
        printf("          Ganaste el juego con %d puntos.        \n", max_score);
        printf("========================================================\n");
        printf("\n");
    } else {
        printf("\n");
        printf("========================================================\n");
        printf("               ¡EMPATE ENTRE JUGADORES!            \n");
        printf("               empataron con %d puntos.         \n", max_score);
        printf("========================================================\n");
        printf("\n");
    }

    int ordenados[board->player_count];
    int total_players = board->player_count;

    // Inicializar índices
    for (int i = 0; i < total_players; i++) {
        ordenados[i] = i;
    }

    // Ordenamiento con múltiples criterios
    for (int i = 0; i < total_players - 1; i++) {
        for (int j = 0; j < total_players - i - 1; j++) {
            int a = ordenados[j];
            int b = ordenados[j + 1];

            player_t *pa = &board->players_list[a];
            player_t *pb = &board->players_list[b];

            // Comparación múltiple
            bool swap = false;

            if (pa->score < pb->score) {
                swap = true;
            } else if (pa->score == pb->score) {
                if (pa->move_req_count > pb->move_req_count) {
                    swap = true;
                } else if (pa->move_req_count == pb->move_req_count) {
                    if (pa->invalid_move_req_count > pb->invalid_move_req_count) {
                        swap = true;
                    }
                }
            }

            if (swap) {
                int tmp = ordenados[j];
                ordenados[j] = ordenados[j + 1];
                ordenados[j + 1] = tmp;
            }
        }
    }

    // Mostrar el ranking final
    printf("\n=============== Estadisticas de los jugadores ===============\n");
    for (int i = 0; i < total_players; i++) {
        int idx = ordenados[i];
        printf(" %d° Jugador %d — %2d pts | Válidos: %d | Inválidos: %d\n", i + 1, idx, board->players_list[idx].score,
               board->players_list[idx].move_req_count, board->players_list[idx].invalid_move_req_count);
    }
    printf("==============================================================\n\n");
}