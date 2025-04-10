#include "../includes/master.h"

int main(int argc, char *argv[]) {
    int players_read_fds[MAX_PLAYERS];

    argument_t arguments = {"10", "10", 200, 10, 0};

    parse_arguments(&arguments, argc, argv);

    shm_adt shm_board = shm_create(GAME_STATE_PATH,
                                   sizeof(game_board_t) + sizeof(int) * atoi(arguments.width) * atoi(arguments.height));
    game_board_t *game_board = shm_get_game_board(shm_board);

    shm_adt shm_sync = shm_create(GAME_SYNC_PATH, sizeof(game_sync_t));
    game_sync_t *game_sync = shm_get_game_sync(shm_sync);

    int players_count = parse_childs(argc, argv, &arguments, players_read_fds, game_board->players_list);

    srand(arguments.seed);
    init_board(game_board, atoi(arguments.width), atoi(arguments.height), players_count);
    init_sync(game_sync);

    fd_set read_fds;
    int max_fd = 0;
    int current_pipe = 0;
    int players_finished = 0;

    time_t last_valid_move_time;
    time(&last_valid_move_time);
    while (true) {
        sem_post(&game_sync->print_needed);
        sem_wait(&game_sync->print_done);
        usleep(arguments.delay * 1000);

        if (game_board->game_has_finished) {
            break;
        }

        FD_ZERO(&read_fds);
        for (int i = 0; i < players_count; i++) {
            FD_SET(players_read_fds[i], &read_fds);
            max_fd = players_read_fds[i] > max_fd ? players_read_fds[i] : max_fd;
        }

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        error_exit("select", ready < 0);

        char move;
        int offset = 0, index;
        for (; offset < players_count; offset++) {
            index = (current_pipe + offset) % players_count;

            if (FD_ISSET(players_read_fds[index], &read_fds)) {
                int bytes = read(players_read_fds[index], &move, sizeof(char));
                error_exit("read", bytes < 0);

                current_pipe = (current_pipe + 1) % players_count;
                break;
            }
        }

        int is_valid_move_flag = is_valid_move(game_board, move, index);

        sem_wait(&(game_sync->access_queue));  // Espera a que no haya lectores

        sem_wait(&(game_sync->game_state_access));  // Toma el recurso

        sem_post(&(game_sync->access_queue));  // Libera la cola

        // SECCION ESCRITURA
        if (!is_valid_move_flag) {
            game_board->players_list[index].invalid_move_req_count++;
        } else {
            update_player(game_board, move, index);
            time(&last_valid_move_time);

            if (!has_valid_moves(game_board, index)) {
                game_board->players_list[index].has_valid_moves = false;
                players_finished++;
            }

            if (players_finished == players_count) {
                game_board->game_has_finished = true;
            }
        }

        if (time(NULL) - last_valid_move_time > arguments.timeout) {
            game_board->game_has_finished = true;
        }
        // FIN SECCION ESCRITURA

        sem_post(&(game_sync->game_state_access));  // Libera el recurso
    }

    for (int i = 0; i < players_count; i++) {
        close(players_read_fds[i]);
    }

    while (wait(NULL) > 0)
        ;

    shm_close(shm_board);
    shm_close(shm_sync);

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
            game_board->board[new_x + new_y * game_board->width] >= 0) {
            return 1;
        }
    }
    return 0;
}

/* AUXILIARY FUNCTIONS */

pid_t create_process(char *executable, char *height, char *width, int fd[2], int redirect_stdout) {
    pid_t pid = fork();
    error_exit("fork", pid < 0);

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

        char *new_argv[] = {executable, height, width, NULL};
        char *new_envp[] = {NULL};

        execve(executable, new_argv, new_envp);
        error_exit("execve", true);
    }

    return pid;
}

void parse_arguments(argument_t *arguments, int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") && i + 1 < argc) {
            if (atoi(argv[i + 1]) < MIN_HEIGHT) error_exit("El valor mínimo para el alto es demasiado bajo", true);
            strcpy(arguments->height, argv[++i]);
        } else if (!strcmp(argv[i], "-w") && i + 1 < argc) {
            if (atoi(argv[i + 1]) < MIN_WIDTH) error_exit("El valor mínimo para el ancho es demasiado bajo", true);
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

int parse_childs(int argc, char *argv[], argument_t *arguments, int players_read_fds[], player_t players[]) {
    int players_count = 0, views_count = 0, i = 1;

    while (i < argc) {
        if (strcmp(argv[i], "-p") == 0) {
            i++;
            while (i < argc && argv[i][0] != '-') {
                error_exit("Error: No se pueden tener mas de 9 jugadores", players_count >= MAX_PLAYERS);

                int fd[2];
                pipe(fd);
                pid_t pid = create_process(argv[i], arguments->height, arguments->width, fd, 1);
                initialize_player(&players[players_count], pid);

                players_read_fds[players_count++] = fd[0];
                close(fd[1]);

                i++;
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            i++;
            while (i < argc && argv[i][0] != '-') {
                error_exit("Error: No se pueden tener mas de 1 vista", views_count >= MAX_VIEWS);

                create_process(argv[i], arguments->height, arguments->width, NULL, 0);
                views_count++;
                i++;
            }
        } else {
            i++;
        }
    }

    error_exit("Error: se requiere por lo menos un jugador y una vista\n", players_count < 1 || views_count < 1);

    return players_count;
}

void set_coordinates(int *x, int *y, direction_t move) {
    static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

    *x += dx[move];
    *y += dy[move];
}

void error_exit(const char *msg, int condition) {
    if (condition) {
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

void init_board(game_board_t *game_board, int width, int height, int players_count) {
    game_board->width = width;
    game_board->height = height;
    game_board->player_count = players_count;
    game_board->game_has_finished = false;

    for (int i = 0; i < width * height; i++) {
        game_board->board[i] = rand() % 9 + 1;
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