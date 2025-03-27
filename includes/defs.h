#ifndef DEFS_H
#define DEFS_H

#include <sys/types.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct
{
  char player_name[16];                 // Nombre del jugador
  unsigned int score;                   // Puntaje
  unsigned int invalid_move_req_count; // Cantidad de solicitudes de movimientos inválidas realizadas
  unsigned int move_req_count;         // Cantidad de solicitudes de movimientos válidas realizadas
  unsigned short x, y;                  // Coordenadas x e y en el tablero
  pid_t pid;                            // Identificador de proceso
  bool has_valid_moves;                 // Indica si el jugador tiene movimientos válidos disponibles
} player_t;

typedef struct
{
  unsigned short width;       // Ancho del tablero
  unsigned short height;      // Alto del tablero
  unsigned int player_count; // Cantidad de jugadores
  player_t players_list[9];   // Lista de jugadores
  bool game_has_finished;     // Indica si el juego se ha terminado
  int board[];                // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} game_board_t;

typedef struct {
sem_t print_needed; // Se usa para indicarle a la vista que hay cambios por imprimir
sem_t print_done; // Se usa para indicarle al master que la vista terminó de imprimir
sem_t master_access; // Mutex para evitar inanición del master al acceder al estado
sem_t game_state_access; // Mutex para el estado del juego
sem_t variable_access; // Mutex para la siguiente variable
unsigned int players_reading_count; // Cantidad de jugadores leyendo el estado
} game_sync_t;

typedef enum
{
  UP = 0,
  UP_RIGHT = 1,
  RIGHT = 2,
  DOWN_RIGHT = 3,
  DOWN = 4,
  DOWN_LEFT = 5,
  LEFT = 6,
  UP_LEFT = 7
} direction_t;

#endif