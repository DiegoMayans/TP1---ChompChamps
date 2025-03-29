#include <unistd.h>
#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "../includes/defs.h"
#include "../includes/shm.h"

void move(direction_t direction);

int main(int argc, char *argv[]) {

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

  game_board_t *board = get_board_state(alto * ancho);
  game_sync_t *sync = get_sync();


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
    sleep(1);

    sem_wait(&sync->count_access);
    sync->players_reading_count--;
    if (sync->players_reading_count == 0) {
      sem_post(&sync->game_state_access);
    }
    sem_post(&sync->count_access);

    move(RIGHT);
  }
  return 0;
}

void move(direction_t direction) {
  write(1, &direction, sizeof(unsigned char));
}
