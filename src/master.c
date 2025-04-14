#include "../includes/master.h"

#include <time.h>
#include <valgrind/valgrind.h>

void log_with_timestamp(FILE *log_file, const char *message) {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);

    fprintf(log_file, "[%s] %s\n", timestamp, message);
    fflush(log_file);  // Ensure the log is written immediately
}

int main(int argc, char *argv[]) {
    FILE *log_file = fopen("master.log", "w");
    if (!log_file) {
        perror("Error al abrir el archivo de log");
        exit(EXIT_FAILURE);
    }
    log_with_timestamp(log_file, "Iniciando el proceso maestro");

    requester_t players_read_fds[MAX_PLAYERS];

    argument_t arguments = {"10", "10", 200, 10, time(NULL)};

    parse_arguments(&arguments, argc, argv);

    shm_adt shm_board = shm_create(GAME_STATE_PATH,
                                   sizeof(game_board_t) + sizeof(int) * atoi(arguments.width) * atoi(arguments.height));
    game_board_t *game_board = shm_get_game_board(shm_board);
    test_exit("Error: No se pudo crear la memoria compartida", game_board == NULL);

    shm_adt shm_sync = shm_create(GAME_SYNC_PATH, sizeof(game_sync_t));
    game_sync_t *game_sync = shm_get_game_sync(shm_sync);
    if (!game_sync) {
        shm_close(shm_board);
        test_exit("Error: No se pudo crear la memoria compartida", true);
    }

    pid_t pid_list[MAX_PLAYERS];
    int players_count = parse_childs(argc, argv, &arguments, players_read_fds, pid_list);
    

    srand(arguments.seed);
    init_board(game_board, atoi(arguments.width), atoi(arguments.height), players_count, pid_list);
    init_sync(game_sync);

    fd_set read_fds;
    int max_fd = 0;

    struct timeval timeout;
    timeout.tv_sec = arguments.timeout;
    timeout.tv_usec = 0;

    time_t last_valid_move_time;
    time(&last_valid_move_time);

    round_robin_adt scheduler = new_round_robin();

    for(int i = 0; i < players_count; i++) {
        instantiate_requester(scheduler, &players_read_fds[i]);
    }

    while (true) {  // MAIN LOOP------------------------------------------------
        sem_post(&game_sync->print_needed);
        sem_wait(&game_sync->print_done);

        usleep(arguments.delay * 1000);

        FD_ZERO(&read_fds);
        for (int i = 0; i < players_count; i++) {
            FD_SET(players_read_fds[i].fd, &read_fds);
            max_fd = players_read_fds[i].fd > max_fd ? players_read_fds[i].fd : max_fd;
        }

        char move = -1;
        requester_t* current_writer = NULL;
        int ready = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        char message[50];
        snprintf(message, sizeof(message), "Unblocking, ready: %d", ready);
        log_with_timestamp(log_file, message);
        if (ready == 0) {
            game_board->game_has_finished = true;
            log_with_timestamp(log_file, "Exiting because of timeout");

            break;
        }
        safe_exit("select", ready < 0, shm_board, shm_sync, players_read_fds, players_count);
        for (int i = 0; i < players_count; i++) {
            if (FD_ISSET(players_read_fds[i].fd, &read_fds)) {
                push(scheduler, &players_read_fds[i]);
            }
        }
        while((current_writer = pop(scheduler)) != NULL) {
            int bytes = read(current_writer->fd, &move, sizeof(char));
            safe_exit("read", bytes < 0, shm_board, shm_sync, players_read_fds, players_count);
                
            int is_valid_move_flag = is_valid_move(game_board, move, current_writer->player_index);

            sem_wait(&(game_sync->access_queue));  // Espera a que no haya lectores

            sem_wait(&(game_sync->game_state_access));  // Toma el recurso

            sem_post(&(game_sync->access_queue));  // Libera la cola

            // SECCION ESCRITURA
            if (!is_valid_move_flag) {
                game_board->players_list[current_writer->player_index].invalid_move_req_count++;
            } else {
                update_player(game_board, move, current_writer->player_index);
                time(&last_valid_move_time);
            }

        if (time(NULL) - last_valid_move_time > arguments.timeout) {
            game_board->game_has_finished = true;
            log_with_timestamp(log_file, "Exiting because of no valid moves in 10 seconds");

            sem_post(&(game_sync->game_state_access));  // Libera el recurso
            break;
        }

        int count = 0;
        for (int i = 0, found = 0; i < players_count && found == 0; i++) {
            if (!has_valid_moves(game_board, i)) {
                count++;
            } else {
                found = 1;  // No hay necesidad de seguir contando
            }
        }

        if (count == players_count) {
            game_board->game_has_finished = true;
            log_with_timestamp(log_file, "Exiting because of no more moves");

            sem_post(&(game_sync->game_state_access));  // Libera el recurso
            break;
        }

            // FIN SECCION ESCRITURA

            sem_post(&(game_sync->game_state_access));  // Libera el recurso
        }
    }  // MAIN LOOP ENDS ----------------------------------------------

    sem_post(&(game_sync->print_needed));  // Vista imprime estado final
    sem_wait(&(game_sync->print_done));

    for (int i = 0; i < players_count; i++) {
        close(players_read_fds[i].fd);
    }
    shm_close(shm_board);
    shm_close(shm_sync);
    fclose(log_file);

    while (wait(NULL) > 0);

    exit(EXIT_SUCCESS);
}  // END MAIN

/* GAME FUNCTIONS */

int is_valid_move(game_board_t *game_board, char move, int player_index) {
    if (move < MIN_MOVE || move > MAX_MOVE) {
        return 0;
    }

    int x = game_board->players_list[player_index].x;
    int y = game_board->players_list[player_index].y;
    set_coordinates(&x, &y, move);

    if (x < 0 || x >= game_board->width || y < 0 || y >= game_board->height) {  // Check colision con borde
        return 0;
    }

    if (game_board->board[x + y * game_board->width] <= 0) {  // Check colision con player
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

    game_board->players_list[player_index].move_req_count++;
    game_board->players_list[player_index].score += cell_score;
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
            game_board->board[new_x + new_y * game_board->width] > 0) {
            return 1;
        }
    }
    return 0;
}

/* AUXILIARY FUNCTIONS */

pid_t create_process(char *executable, char *height, char *width, int fd[2], int redirect_stdout) {
    pid_t pid = fork();
    test_exit("fork", pid < 0);

    if (pid == 0) {
        // Child process
        if (redirect_stdout) {
            int i = fd[0];
            while (i >= INITIAL_FD_COUNT) {
                close(i);
                i--;
            }
            close(STDOUT_FILENO);  // Close stdout
            dup(fd[1]);            // Redirect stdout to the pipe
            close(fd[1]);          // Close the write end of the pipe after duplicating
        }

        if (RUNNING_ON_VALGRIND) {
            char *new_argv[] = {"valgrind",
                                "--log-file=/dev/null",
                                "--leak-check=full",
                                "--track-origins=yes",
                                "--error-exitcode=1",
                                executable,
                                height,
                                width,
                                NULL};
            char *new_envp[] = {NULL};
            execve("/usr/bin/valgrind", new_argv, new_envp);
        } else {
            char *new_argv[] = {executable, height, width, NULL};
            char *new_envp[] = {NULL};
            execve(executable, new_argv, new_envp);
        }

        test_exit("execve", true);
    }

    return pid;
}

void parse_arguments(argument_t *arguments, int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") && i + 1 < argc) {
            if (atoi(argv[i + 1]) < MIN_HEIGHT) test_exit("El valor mínimo para el alto es demasiado bajo", true);
            strcpy(arguments->height, argv[++i]);
        } else if (!strcmp(argv[i], "-w") && i + 1 < argc) {
            if (atoi(argv[i + 1]) < MIN_WIDTH) test_exit("El valor mínimo para el ancho es demasiado bajo", true);
            strcpy(arguments->width, argv[++i]);
        } else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            arguments->delay = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-t") && i + 1 < argc) {
            arguments->timeout = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-s") && i + 1 < argc) {
            arguments->seed = atoi(argv[++i]);
        }
    }
}

int parse_childs(int argc, char *argv[], argument_t *arguments, requester_t players_read_fds[], pid_t pid_list[]) {
    int players_count = 0, views_count = 0, i = 1;

    while (i < argc) {
        if (strcmp(argv[i], "-p") == 0) {
            i++;
            while (i < argc && argv[i][0] != '-') {
                test_exit("Error: No se pueden tener mas de 9 jugadores", players_count >= MAX_PLAYERS);

                int fd[2];
                pipe(fd);
                pid_t pid = create_process(argv[i], arguments->height, arguments->width, fd, 1);
                pid_list[players_count] = pid;

                players_read_fds[players_count].player_index = players_count;
                players_read_fds[players_count++].fd = fd[0];
                
                close(fd[1]);

                i++;
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            i++;
            while (i < argc && argv[i][0] != '-') {
                test_exit("Error: No se pueden tener mas de 1 vista", views_count >= MAX_VIEWS);

                create_process(argv[i], arguments->height, arguments->width, NULL, 0);
                views_count++;
                i++;
            }
        } else {
            i++;
        }
    }

    test_exit("Error: se requiere por lo menos un jugador\n", players_count < 1);

    return players_count;
}

void set_coordinates(int *x, int *y, direction_t move) {
    static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

    *x += dx[move];
    *y += dy[move];
}

void test_exit(const char *msg, int condition) {
    if (condition) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void safe_exit(const char *msg, int condition, shm_adt shm_board, shm_adt shm_sync, requester_t fds[],
               int players_count) {
    if (condition) {
        for (int i = 0; i < players_count; i++) {
            if (fds[i].fd > 0) close(fds[i].fd);
        }
        shm_close(shm_board);
        shm_close(shm_sync);
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

/* INIT FUNCTIONS */

void initialize_player(player_t *player, pid_t pid) {
    player->score = 0;
    player->invalid_move_req_count = 0;
    player->move_req_count = 0;
    player->x = 0;
    player->y = 0;
    player->has_valid_moves = true;
    player->pid = pid;
}

void init_board(game_board_t *game_board, int width, int height, int players_count, pid_t pid_list[]) {
    game_board->width = width;
    game_board->height = height;
    game_board->player_count = players_count;
    game_board->game_has_finished = false;

    for (int i = 0; i < width * height; i++) {
        game_board->board[i] = rand() % 9 + 1;
    }

    for (int i = 0; i < players_count; i++) {
        initialize_player(&game_board->players_list[i], pid_list[i]);
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
    int min_dist = (int)sqrt((width * height) / players_count);
    if (min_dist < 1) min_dist = 1;

    for (int i = 0; i < players_count; i++) {
        int x, y, is_valid_position, loop_count = 0, min = min_dist;
        do {
            x = rand() % width;
            y = rand() % height;

            is_valid_position = 1;
            for (int j = 0; j < players_count && is_valid_position == 1; j++) {
                int dx = game_board->players_list[j].x - x;
                int dy = game_board->players_list[j].y - y;
                if (sqrt(dx * dx + dy * dy) < min) {
                    is_valid_position = 0;
                }
            }
            if (++loop_count > 3 && min > 1) {
                min--;
                loop_count = 0;
            }
        } while (!is_valid_position);

        game_board->players_list[i].x = x;
        game_board->players_list[i].y = y;

        game_board->board[(y * width) + x] = -i;
    }
}
