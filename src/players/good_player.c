#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>  // For timestamps
#include <unistd.h>

#include "../../includes/defs.h"
#include "../../includes/shm_adt.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

void move(direction_t direction);

void set_coordinates(int *x, int *y, direction_t move);

void log_with_timestamp(FILE *log_file, const char *message);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "Error: ancho y alto deben ser valores positivos.\n");
        exit(EXIT_FAILURE);
    }

    // Open the log file for writing
    FILE *log_file = fopen("good_player.log", "w");
    if (!log_file) {
        perror("Error al abrir el archivo de log");
        exit(EXIT_FAILURE);
    }

    shm_adt shm_board = shm_open_readonly(GAME_STATE_PATH, sizeof(game_board_t) + sizeof(int) * height * width);
    game_board_t *board = shm_get_game_board(shm_board);
    shm_adt shm_sync = shm_open_readwrite(GAME_SYNC_PATH, sizeof(game_sync_t));
    game_sync_t *sync = shm_get_game_sync(shm_sync);

    pid_t pid = getpid();

    int no_move_counter = 0;

    int x = -1, y = -1, first_read = 1, my_index = 0, best_move;
    while (!board->game_has_finished) {
        best_move = -1;

        sem_wait(&sync->access_queue);
        sem_wait(&sync->count_access);
        sync->players_reading_count++;
        if (sync->players_reading_count == 1) {
            sem_wait(&sync->game_state_access);
        }
        sem_post(&sync->count_access);
        sem_post(&sync->access_queue);

        if (first_read) {
            for (; board->players_list[my_index].pid != pid; my_index++);
        }

        // If x and y are the same as before, skip the move
        if (x != board->players_list[my_index].x || y != board->players_list[my_index].y) {
            x = board->players_list[my_index].x;
            y = board->players_list[my_index].y;

            int best_score = -1;
            for (int i = 0; i < 8; i++) {
                int new_x = x;
                int new_y = y;
                set_coordinates(&new_x, &new_y, i);

                // Verifica que la celda esté dentro del tablero y no esté ocupada.
                if (new_x < 0 || new_x >= board->width || new_y < 0 || new_y >= board->height ||
                    board->board[new_x + new_y * board->width] <= 0)
                    continue;

                int score = board->board[new_x + new_y * board->width];

                // Actualiza el movimiento si se encontró un valor mejor.
                if (score > best_score) {
                    best_score = score;
                    best_move = i;
                }
            }
        }

        sem_wait(&sync->count_access);
        sync->players_reading_count--;
        if (sync->players_reading_count == 0) {
            sem_post(&sync->game_state_access);
        }
        sem_post(&sync->count_access);

        if (best_move != -1) {
            move(best_move);
            char move_message[50];
            snprintf(move_message, sizeof(move_message), "Wrote move: %d", best_move);
            log_with_timestamp(log_file, move_message);
        } else {
            no_move_counter++;
        }
    }

    char no_move_message[50];
    snprintf(no_move_message, sizeof(no_move_message), "No move counter: %d", no_move_counter);
    log_with_timestamp(log_file, no_move_message);

    fclose(log_file);

    shm_close(shm_board);
    shm_close(shm_sync);
    return 0;
}

void move(direction_t direction) { write(1, &direction, sizeof(unsigned char)); }

void set_coordinates(int *x, int *y, direction_t move) {
    static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

    *x += dx[move];
    *y += dy[move];
}

void log_with_timestamp(FILE *log_file, const char *message) {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);

    fprintf(log_file, "[%s] %s\n", timestamp, message);
    fflush(log_file);  // Ensure the log is written immediately
}