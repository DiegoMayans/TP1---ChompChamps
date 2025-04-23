#ifndef SHM_ADT_C
#define SHM_ADT_C

#include <semaphore.h>
#include <stdbool.h>

typedef struct shm_cdt *shm_adt;

typedef struct {
    char player_name[16];                 // Nombre del jugador
    unsigned int score;                   // Puntaje
    unsigned int invalid_move_req_count;  // Cantidad de solicitudes de movimientos
                                          // inválidas realizadas
    unsigned int move_req_count;          // Cantidad de solicitudes de movimientos válidas
                                          // realizadas
    unsigned short x, y;                  // Coordenadas x e y en el tablero
    pid_t pid;                            // Identificador de proceso
    bool cant_move;                 // Indica si el jugador NO tiene movimientos válidos
                                          // disponibles
} player_t;

typedef struct {
    unsigned short width;       // Ancho del tablero
    unsigned short height;      // Alto del tablero
    unsigned int player_count;  // Cantidad de jugadores
    player_t players_list[9];   // Lista de jugadores
    bool game_has_finished;     // Indica si el juego se ha terminado
    int board[];                // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1
} game_board_t;

typedef struct {
    sem_t print_needed;                  // Se usa para indicarle a la vista que hay cambios por
                                         // imprimir
    sem_t print_done;                    // Se usa para indicarle al master que la vista terminó de
                                         // imprimir
    sem_t access_queue;                  // Mutex para evitar inanición del master al acceder al
                                         // estado
    sem_t game_state_access;             // Mutex para el estado del juego
    sem_t count_access;                  // Mutex para la siguiente variable
    unsigned int players_reading_count;  // Cantidad de jugadores leyendo el estado
} game_sync_t;

shm_adt shm_create(const char *name, size_t size);
shm_adt shm_open_readonly(const char *name, size_t size);
shm_adt shm_open_readwrite(const char *name, size_t size);
game_board_t *shm_get_game_board(shm_adt shm);
game_sync_t *shm_get_game_sync(shm_adt shm);
void shm_close(shm_adt shm);

#endif