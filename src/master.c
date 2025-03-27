// Prototipo de master
// Modo de uso ./master -p ./player1 ./player2 ... -v ./view

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PLAYERS 9

void crear_proceso(char *ejecutable, int fd[2]) {
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

    char *new_argv[] = {ejecutable, NULL};
    char *new_envp[] = {NULL};

    execve(ejecutable, new_argv, new_envp);
    perror("execve");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  // Crear pipe fd[0] -> lectura, fd[1] -> escritura
  // Master lee de fd[0] y escribe en memoria compartida
  // Players y vista leen de memoria compartida y escriben en fd[1]
  int fd[2];
  if (pipe(fd) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  int players = 0;
  int views = 0;
  int flag_players = 0; // Opcion -p obligatoria
  int i = 1;

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

        crear_proceso(argv[i], fd);
        players++;
        i++;
      }
    } else if (strcmp(argv[i], "-v") == 0) { // -v args
      i++;
      while (i < argc && argv[i][0] != '-') { // Creacion de procesos vistas
        crear_proceso(argv[i], fd);
        views++;
        i++;
      }
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

  close(fd[1]); // Cerramos pipe de escritura

  while (wait(NULL) > 0)
    ;

  exit(EXIT_SUCCESS);
}