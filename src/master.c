// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "../includes/master.h"

/* Main Function */
int main(int argc, char *argv[]) {
    requester_t players_read_fds[MAX_PLAYERS];
    pid_t pid_list[MAX_PLAYERS];

    argument_t arguments = {"10", "10", 200, 10, time(NULL)};
    game_board_t *game_board = NULL;
    game_sync_t *game_sync = NULL;

    parse_arguments(&arguments, argc, argv);

    shm_adt shm_board = shm_create(GAME_STATE_PATH,
                                   sizeof(game_board_t) + sizeof(int) * atoi(arguments.width) * atoi(arguments.height));
    game_board = shm_get_game_board(shm_board);
    test_exit("Error: No se pudo crear la memoria compartida", game_board == NULL);

    shm_adt shm_sync = shm_create(GAME_SYNC_PATH, sizeof(game_sync_t));
    game_sync = shm_get_game_sync(shm_sync);
    if (!game_sync) {
        shm_close(shm_board);
        test_exit("Error: No se pudo crear la memoria compartida", true);
    }

    int players_count = parse_childs(argc, argv, &arguments, players_read_fds, pid_list);

    srand(arguments.seed);
    init_board(game_board, atoi(arguments.width), atoi(arguments.height), players_count, pid_list);
    init_sync(game_sync);

    round_robin_adt scheduler = new_round_robin();
    for (int i = 0; i < players_count; i++) {
        instantiate_requester(scheduler, &players_read_fds[i]);
    }

    time_t last_valid_move_time;
    time(&last_valid_move_time);

    while (true) {  // MAIN LOOP------------------------------------------------
        sem_post(&game_sync->print_needed);
        sem_wait(&game_sync->print_done);

        usleep(arguments.delay * 1000);

        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = 0;

        for (int i = 0; i < players_count; i++) {
            FD_SET(players_read_fds[i].fd, &read_fds);
            max_fd = players_read_fds[i].fd > max_fd ? players_read_fds[i].fd : max_fd;
        }

        char move = -1;
        requester_t *current_writer = NULL;

        struct timeval timeout = {arguments.timeout, 0};
        int ready = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (ready == 0) {
            game_board->game_has_finished = true;

            sem_post(&(game_sync->print_needed));
            break;
        }
        safe_exit("select", ready < 0, shm_board, shm_sync, players_read_fds, players_count);

        for (int i = 0; i < players_count; i++) {
            if (FD_ISSET(players_read_fds[i].fd, &read_fds)) {
                push(scheduler, &players_read_fds[i]);
            }
        }

        while ((current_writer = pop(scheduler)) != NULL) {  // REQUESTS LOOP
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

                int players_without_moves = 0;
                for (int i = 0; i < players_count; i++) {
                    if (!has_valid_moves(game_board, i)) {
                        players_without_moves++;
                    }
                }

                if (players_without_moves == players_count) {
                    game_board->game_has_finished = true;

                    sem_post(&(game_sync->game_state_access));  // Libera el recurso
                    break;
                }
            }

            if (time(NULL) - last_valid_move_time > arguments.timeout) {
                game_board->game_has_finished = true;

                sem_post(&(game_sync->game_state_access));  // Libera el recurso
                break;
            }

            // FIN SECCION ESCRITURA

            sem_post(&(game_sync->game_state_access));  // Libera el recurso
        }  // END REQUESTS LOOP

        if (game_board->game_has_finished) {
            sem_post(&(game_sync->print_needed));  // Vista imprime estado final
            break;
        }
    }  // MAIN LOOP ENDS ----------------------------------------------

    for (int i = 0; i < players_count; i++) {
        close(players_read_fds[i].fd);
    }

    free_round_robin(scheduler);
    shm_unlink(GAME_STATE_PATH);
    shm_close(shm_board);
    shm_unlink(GAME_SYNC_PATH);
    shm_close(shm_sync);

    while (wait(NULL) > 0);

    exit(EXIT_SUCCESS);
}  // END MAIN

/* GAME FUNCTIONS */
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
            strncpy(arguments->height, argv[++i], sizeof(arguments->height) - 1);
            arguments->height[sizeof(arguments->height) - 1] = '\0';
        } else if (!strcmp(argv[i], "-w") && i + 1 < argc) {
            if (atoi(argv[i + 1]) < MIN_WIDTH) test_exit("El valor mínimo para el ancho es demasiado bajo", true);
            strncpy(arguments->width, argv[++i], sizeof(arguments->width) - 1);
            arguments->width[sizeof(arguments->width) - 1] = '\0';
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
    player->cant_move = false;
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
