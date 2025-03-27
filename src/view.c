#include "../includes/view.h"
#include "../includes/defs.h"
#include "../includes/shm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int ancho = atoi(argv[1]);
  int alto = atoi(argv[2]);

  if (ancho <= 0 || alto <= 0) {
    fprintf(stderr, "Error: ancho y alto deben ser valores positivos.\n");
    exit(EXIT_FAILURE);
  }

  game_board_t *board_state = get_board_state();
  game_sync_t *game_sync = get_sync();

  sem_t *sem_A =
      &game_sync->print_needed; // de aca sabe si hay cambios para imprimir
  sem_t *sem_B = &game_sync->print_done;
  (void)sem_A;
  (void)sem_B;

  while (!board_state->game_has_finished) {
    // sem_wait(sem_A);
    clear_screen();
    print_board(board_state);

    // sem_post(sem_B);
  }

  return 0;
}

void print_board(game_board_t *board_state) {
  // Limpiar pantalla
  printf("\033[H\033[J");

  printf("----------TABLERO----------\n\n\n\n\n\n");

  for (int i = 0; i < board_state->height; i++) {
    for (int j = 0; j < board_state->width; j++) {
      printf("%2d ", board_state->board[i * board_state->width + j]);
    }
    printf("\n");
  }
  printf("\n");
}

void clear_screen() {
  printf("\033[H\033[J");
  fflush(stdout);
}