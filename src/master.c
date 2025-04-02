// Prototipo de master
// Modo de uso ./master -p ./player1 ./player2 ... -v ./view

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../includes/defs.h"
#include "../includes/shm.h"

#define MAX_PLAYERS 9
#define MAX_VIEWS 1
#define INITIAL_FD_COUNT 3
#define MIN_WIDTH 10
#define MIN_HEIGHT 10
#define MAX_DIGITS 3
#define NON_VALID_MOVES_TIME 10  // 10 segundos

typedef struct {
    char height[MAX_DIGITS], width[MAX_DIGITS];
    int delay;
    int timeout;
    int seed;
} argument_t;

pid_t create_player(char *executable, int fd[2], char *height, char *width);
void create_view(char *executable, char *height, char *width);
void parse_arguments(argument_t *arguments, int argc, char *argv[]);
int parse_childs(int argc, char *argv[], argument_t *arguments, int players_read_fds[], player_t players[]);
void validate_arguments(int flag_players, int players, int views);
void init_board(game_board_t *game_board, int width, int height, int players_count);
void init_sync(game_sync_t *game_sync);
void initialize_player(player_t *player, pid_t pid);
void initialize_player_positions(game_board_t *game_board, int width, int height, int players_count);
int is_valid_position(int x, int y, player_t players_list[], int count, int min_dist);
int is_valid_move(game_board_t *game_board, char move, int player_index);
void set_coordinates(int *x, int *y, direction_t move);
void update_player(game_board_t *game_board, direction_t move, int player_index);
int has_valid_moves(game_board_t *game_board, int player_index);

int main(int argc, char *argv[]) {
    // Se crean las dos zonas de memoria compartida
    int players_read_fds[MAX_PLAYERS];  // Colección de File Descriptors para
                                        // players

    argument_t arguments = {"10", "10", 200, 10, 0};

    parse_arguments(&arguments, argc, argv);

    game_board_t *game_board = (game_board_t *)createSHM(
        GAME_STATE_PATH, sizeof(game_board_t) + sizeof(int) * atoi(arguments.width) * atoi(arguments.height));
    game_sync_t *game_sync = (game_sync_t *)createSHM(GAME_SYNC_PATH, sizeof(game_sync_t));
    (void)game_sync;

    int players_count = parse_childs(argc, argv, &arguments, players_read_fds, game_board->players_list);

    // A partir de aca se escribe la logica del master

    // Primero inicializamos las memorias compartidas
    init_board(game_board, atoi(arguments.width), atoi(arguments.height), players_count);
    init_sync(game_sync);

    // Con todo inicializado, se procede a ejecutar el juego
    // Procedimiento: Los jugadores desde que se forkean ya estan mandando solicitudes de movimiento
    // La vista esta suspendida por el semaforo print_needed
    // Lo primero que va a hacer el master es decirle a la vista que imprima
    // Luego empezara a leer solicitudes de movimiento, validando cada una y realizandola si es valida
    // Luego de cada movimiento valido el juego revisa si ocurrio alguna de las siguientes tres cosas:
    // 1. Algun jugador se quedo sin movimientos validos
    // 2. El juego termino por que no ocurrio un movimiento valido durante X tiempo
    // 3. El juego termino por que ningun jugador tiene movimientos validos
    // timeout de 10 segundos
    fd_set read_fds;  // struct que contiene los fd que se van a monitorear
    int max_fd = 0;
    int current_pipe = 0;
    int players_finished = 0;

    time_t last_valid_move_time;
    time(&last_valid_move_time);
    while (1) {
        sem_post(&game_sync->print_needed);  // Mandamos a imprimir
        sem_wait(&game_sync->print_done);    // Esperamos a que termine de imprimir

        if (game_board->game_has_finished) {
            break;
        }

        // Ahora el master va a leer las solicitudes de movimiento
        // Tenemos los pipes donde escribe cada player en players_read_fds
        // Vamos a leer de cada uno de esos pipes usando select y siguiendo un procedimiento round robin
        // select() monitorea varios fd y hace que el master espere a que uno o mas se vuelvan READY.
        // Un fd esta READY si se puede realizar una operacion I/O de inmediato (sin bloquear).
        FD_ZERO(&read_fds);                        // Inicializa el set de fd
        for (int i = 0; i < players_count; i++) {  // Agrego los fds de los players al set
            FD_SET(players_read_fds[i], &read_fds);
            max_fd = players_read_fds[i] > max_fd ? players_read_fds[i] : max_fd;
        }

        int ready;
        if ((ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL)) == 0) {  // select requiere pasar el max_fd + 1
            // Aca no deberiamos llegar nunca porque no pasamos timeout
        } else if (ready < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        // Si llegamos aca es por que algun jugador se movio

        char move;
        int offset = 0, index;
        for (; offset < players_count; offset++) {
            index = (current_pipe + offset) % players_count;  // Ciclo circular

            if (FD_ISSET(players_read_fds[index], &read_fds)) {
                // Leer el movimiento del jugador (es 1 char)
                int bytes;
                if ((bytes = read(players_read_fds[index], &move, sizeof(char))) < 0) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                printf("Movimiento recibido: %d, del jugador %d, bytes: %d\n", move, index, bytes);

                current_pipe = (index + 1) % players_count;  // Avanzar round robin
                break;
            }
        }

        // Tenemos el movimiento, ahora lo validamos

        int is_valid_move_flag = is_valid_move(game_board, move, index);

        // Si es valido lo escribimos y hacemos todos los chequeos

        sem_wait(&(game_sync->access_queue));  // Espera a que no haya lectores

        sem_wait(&(game_sync->game_state_access));  // Toma el recurso

        sem_post(&(game_sync->access_queue));  // Libera la cola

        // INICIO SECCION ESCRITURA

        if (!is_valid_move_flag) {
            game_board->players_list[index].invalid_move_req_count++;
        } else {
            update_player(game_board, move, index);
            time(&last_valid_move_time);  // Reseteamos el timer

            if (!has_valid_moves(game_board, index)) {
                game_board->players_list[index].has_valid_moves = false;
                players_finished++;
            }

            // Chequear si el juego termino por que todos los jugadores se quedaron sin movimientos validos
            if (players_finished == players_count) {
                game_board->game_has_finished = true;
            }
        }

        printf("time elapsed: %ld\n", time(NULL) - last_valid_move_time);

        // Chequear si el juego termino por que no ocurrio un movimiento valido en 10 segundos
        if (time(NULL) - last_valid_move_time > NON_VALID_MOVES_TIME) {
            game_board->game_has_finished = true;
        }

        // FIN SECCION ESCRITURA

        sem_post(&(game_sync->game_state_access));  // Libera el recurso
    }

    for (int i = 0; i < players_count; i++) {
        close(players_read_fds[i]);  // Cerramos los pipes de escritura para los
                                     // players
    }

    while (wait(NULL) > 0)
        ;

    exit(EXIT_SUCCESS);
}  // END MAIN

/* GAME FUNCTIONS */

void init_board(game_board_t *game_board, int width, int height, int players_count) {
    // init_board no inicializa players_list porque se inicializa en el parseo de argumentos
    game_board->width = width;
    game_board->height = height;
    game_board->player_count = players_count;
    game_board->game_has_finished = false;

    for (int i = 0; i < width * height; i++) {
        game_board->board[i] = rand() % 9 + 1;
    }

    // Inicializar las posiciones x e y de los jugadores
    for (int i = 0; i < players_count; i++) {
        game_board->players_list[i].x = rand() % width;
        game_board->players_list[i].y = rand() % height;
    }

    initialize_player_positions(game_board, width, height, players_count);
}

void init_sync(game_sync_t *game_sync) {
    sem_init(&game_sync->print_needed, 1, 0);
    sem_init(&game_sync->print_done, 1, 0);
    sem_init(&game_sync->access_queue, 1, 1);
    sem_init(&game_sync->game_state_access, 1, 1);
    sem_init(&game_sync->count_access, 1, 1);
    game_sync->players_reading_count = 0;
}

void initialize_player_positions(game_board_t *game_board, int width, int height, int players_count) {
    srand(time(NULL));

    int min_dist = (int)sqrt((width * height) / players_count);  // + area -> + dist | + players -> - dist
    if (min_dist < 1) min_dist = 1;                              // Asegurar mínimo 1

    for (int i = 0; i < players_count; i++) {
        int x, y;
        do {
            x = rand() % width;
            y = rand() % height;
        } while (!is_valid_position(x, y, game_board->players_list, i, min_dist));

        game_board->players_list[i].x = x;
        game_board->players_list[i].y = y;

        game_board->board[(y * width) + x] = -i;
    }
}

int is_valid_move(game_board_t *game_board, char move, int player_index) {
    if (move < MIN_MOVE || move > MAX_MOVE) {
        return 0;
    }

    int x = game_board->players_list[player_index].x;
    int y = game_board->players_list[player_index].y;
    set_coordinates(&x, &y, move);

    // Chequear que este dentro de los limites del tablero
    if (x < 0 || x >= game_board->width || y < 0 || y >= game_board->height) {
        return 0;
    }

    // Chequear que no haya otro jugador ahi
    if (game_board->board[x + y * game_board->width] < 0) {
        return 0;
    }

    return 1;
}

void update_player(game_board_t *game_board, direction_t move, int player_index) {
    int x = game_board->players_list[player_index].x;
    int y = game_board->players_list[player_index].y;
    set_coordinates(&x, &y, move);

    int cell_score = game_board->board[x + y * game_board->width];
    game_board->board[x + y * game_board->width] = -player_index;

    game_board->players_list[player_index].move_req_count++;     // Aumentar contador de movimientos validos
    game_board->players_list[player_index].score += cell_score;  // Aumentar score
    game_board->players_list[player_index].x = x;
    game_board->players_list[player_index].y = y;
}

int has_valid_moves(game_board_t *game_board, int player_index) {
    int x = game_board->players_list[player_index].x;
    int y = game_board->players_list[player_index].y;

    for (int i = 0; i < 8; i++) {
        int new_x = x;
        int new_y = y;
        set_coordinates(&new_x, &new_y, i);

        if (new_x >= 0 && new_y >= 0 && new_x < game_board->width && new_y < game_board->height &&
            game_board->board[new_x + new_y * game_board->width] >= 0) {
            return 1;
        }
    }
    return 0;
}

/* AUXILIARY FUNCTIONS */

pid_t create_player(char *executable, int fd[2], char *height, char *width) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Proceso hijo
        int i = fd[0];
        while (i >= INITIAL_FD_COUNT) {
            close(i);
            i--;
        }                      // No necesita leer del pipe
        close(STDOUT_FILENO);  // Cerramos stdout
        dup(fd[1]);            // Redirigimos stdout al pipe
        close(fd[1]);          // Cerramos pipe después de duplicar

        char *new_argv[] = {executable, height, width, NULL};
        char *new_envp[] = {NULL};

        execve(executable, new_argv, new_envp);
        perror("execve");
        exit(EXIT_FAILURE);
    }

    return pid;
}

void create_view(char *executable, char *height, char *width) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        char *new_argv[] = {executable, height, width, NULL};
        char *new_envp[] = {NULL};

        execve(executable, new_argv, new_envp);
        perror("execve");
        exit(EXIT_FAILURE);
    }
}

void parse_arguments(argument_t *arguments, int argc, char *argv[]) {
    int i = 0;
    while (i < argc) {
        if (!strcmp(argv[i], "-h")) {
            i++;
            if (atoi(argv[i]) < MIN_HEIGHT) {
                fprintf(stderr, "Error: El valor mínimo para el alto de la pantalla es %d\n", MIN_HEIGHT);
                exit(EXIT_FAILURE);
            }
            strcpy(arguments->height, argv[i]);
            i++;
        } else if (!strcmp(argv[i], "-w")) {
            i++;
            if (atoi(argv[i]) < MIN_WIDTH) {
                fprintf(stderr, "Error: El valor mínimo para el ancho de la pantalla es %d\n", MIN_WIDTH);
                exit(EXIT_FAILURE);
            }
            strcpy(arguments->width, argv[i]);
            i++;
        } else if (!strcmp(argv[i], "-d")) {
            i++;
            arguments->delay = atoi(argv[i]);
        } else if (!strcmp(argv[i], "-t")) {
            i++;
            arguments->timeout = atoi(argv[i]);
        } else if (!strcmp(argv[i], "-s")) {
            i++;
            arguments->seed = atoi(argv[i]);
        } else {
            i++;
        }
    }
}

int parse_childs(int argc, char *argv[], argument_t *arguments, int players_read_fds[], player_t players[]) {
    int players_count = 0, flag_players = 0, views = 0;
    int i = 1;
    while (i < argc) {                     // Loop through args
        if (strcmp(argv[i], "-p") == 0) {  // -p args
            flag_players = 1;
            i++;

            while (i < argc && argv[i][0] != '-') {  // Create player processes
                if (players_count >= MAX_PLAYERS) {
                    fprintf(stderr, "Error: No se pueden tener mas de %d jugadores\n", MAX_PLAYERS);
                    exit(EXIT_FAILURE);
                }

                int fd[2];
                pipe(fd);
                pid_t pid = create_player(argv[i], fd, arguments->height, arguments->width);

                initialize_player(&players[players_count], pid);

                players_read_fds[players_count++] = fd[0];
                close(fd[1]);

                i++;
            }
        } else if (strcmp(argv[i], "-v") == 0) {  // -v args
            i++;
            while (i < argc && argv[i][0] != '-') {  // Create view processes
                if (views >= MAX_VIEWS) {
                    fprintf(stderr, "Error: No se pueden tener mas de %d vistas\n", MAX_VIEWS);
                    exit(EXIT_FAILURE);
                }
                create_view(argv[i], arguments->height, arguments->width);
                views++;
                i++;
            }
        } else {
            i++;  // Skip arg
        }
    }

    // Validate arguments
    validate_arguments(flag_players, players_count, views);

    return players_count;
}

void validate_arguments(int flag_players, int players, int views) {
    if (!flag_players) {
        fprintf(stderr, "Error: No se especificaron jugadores\n");
        exit(EXIT_FAILURE);
    }
    if (players < 1) {
        fprintf(stderr, "Error: No se especificaron jugadores\n");
        exit(EXIT_FAILURE);
    }
    if (views < 1) {
        fprintf(stderr, "Error: No se especificaron vistas\n");
        exit(EXIT_FAILURE);
    }
}

void initialize_player(player_t *player, pid_t pid) {
    player->score = 0;
    player->invalid_move_req_count = 0;
    player->move_req_count = 0;
    player->x = 0;
    player->y = 0;
    player->has_valid_moves = true;
    player->pid = pid;
}

int is_valid_position(int x, int y, player_t players_list[], int count, int min_dist) {
    for (int i = 0; i < count; i++) {
        int dx = players_list[i].x - x;
        int dy = players_list[i].y - y;
        if (sqrt(dx * dx + dy * dy) < min_dist) {
            return 0;  // Demasiado cerca de otro jugador
        }
    }
    return 1;  // Posición válida
}

void set_coordinates(int *x, int *y, direction_t move) {
    switch (move) {
        case UP:
            (*y)--;
            break;
        case UP_RIGHT:
            (*x)++;
            (*y)--;
            break;
        case RIGHT:
            (*x)++;
            break;
        case DOWN_RIGHT:
            (*x)++;
            (*y)++;
            break;
        case DOWN:
            (*y)++;
            break;
        case DOWN_LEFT:
            (*x)--;
            (*y)++;
            break;
        case LEFT:
            (*x)--;
            break;
        case UP_LEFT:
            (*x)--;
            (*y)--;
            break;
    }
}