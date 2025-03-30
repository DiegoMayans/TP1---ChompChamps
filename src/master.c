// Prototipo de master
// Modo de uso ./master -p ./player1 ./player2 ... -v ./view

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../includes/shm.h"
#include "../includes/defs.h"

#define MAX_PLAYERS 9
#define MAX_VIEWS 1

#define MIN_WIDTH 10
#define MIN_HEIGHT 10

void create_process(char *ejecutable, int fd[2], char *height, char *width) {
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    // Proceso hijo
    close(fd[0]);         // No necesita leer del pipe
    close(STDOUT_FILENO); // Cerramos stdout
    dup(fd[1]);           // Redirigimos stdout al pipe
    close(fd[1]);         // Cerramos pipe después de duplicar

    char *new_argv[] = {ejecutable, height, width, NULL};
    char *new_envp[] = {NULL};

    execve(ejecutable, new_argv, new_envp);
    perror("execve");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  // Se crean las dos zonas de memoria compartida
  game_board_t *game_board = (game_board_t*) createSHM(GAME_STATE_PATH, sizeof(game_board_t));
  game_sync_t *game_sync = (game_sync_t*) createSHM(GAME_SYNC_PATH, sizeof(game_sync_t));

  int players_read_fds[MAX_PLAYERS];  // Colección de File Descriptors para players
  int views_read_fds[MAX_VIEWS];      // Colección de File Descriptors para las views

  int players = 0;
  int views = 0;
  int flag_players = 0; // Opcion -p obligatoria
  int i = 1;
  char *width = "10", *height = "10";

  while (i < argc) {                  // Loop para recorrer args
    if (strcmp(argv[i], "-p") == 0) { // -p args
      flag_players = 1;
      i++;

      while (i < argc && argv[i][0] != '-') { // Creacion de procesos jugadores
        if (players >= MAX_PLAYERS) {
          fprintf(stderr, "Error: No se pueden tener mas de %d jugadores\n",
                  MAX_PLAYERS);
          exit(EXIT_FAILURE);
        }
        int fd[2];
        pipe(fd);
        create_process(argv[i], fd, height, width);
        players_read_fds[players] = fd[0];
        close(fd[1]);
        players++;
        i++;
      }
    } else if (strcmp(argv[i], "-v") == 0) { // -v args
      i++;
      while (i < argc && argv[i][0] != '-') { // Creacion de procesos vistas
        if(views >= MAX_VIEWS) {
          fprintf(stderr, "Error: No se pueden tener mas de %d vistas\n",
                  MAX_VIEWS);
          exit(EXIT_FAILURE);
        }
        int fd[2];
        pipe(fd);
        create_process(argv[i], fd, height, width);
        views_read_fds[views] = fd[0];
        close(fd[1]);
        views++;
        i++;
      }
    } else if(strcmp(argv[i], "-h")) {
      i++;
      if(atoi(argv[i]) < MIN_HEIGHT) {
        fprintf(stderr, "Error: El valor mínimo para el ancho de la pantalla es %d\n", 
                  MIN_WIDTH);
        exit(EXIT_FAILURE);
      }
      width = argv[i];
      i++;
    } else if(strcmp(argv[i], "-w")) {
      i++;
      if(atoi(argv[i]) < MIN_WIDTH) {
        fprintf(stderr, "Error: El valor mínimo para el ancho de la pantalla es %d\n", 
                  MIN_WIDTH);
        exit(EXIT_FAILURE);
      }
      width = argv[i];
      i++;
    } else {
      i++; // Saltear arg
    }
  } // Termino de recorrer los argumentos

  // Validaciones de argumentos
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
  
  for(int i = 0; i < players; i++) {
    close(players_read_fds[i]); // Cerramos los pipes de escritura para los players
  }

  for(int i = 0; i < views; i++) {
    close(views_read_fds[i]);   // Cerramos los pipes de escritura para las vistas
  }
  
  while (wait(NULL) > 0) {printf("a\n");};

  exit(EXIT_SUCCESS);
}