#include <stdio.h>
#include <unistd.h>  // Para sleep()

#define RESET   "\033[0m"  
#define WHITE   "\033[47m"  // Fondo blanco
#define BLACK   "\033[46m"  // Fondo negro
#define RED     "\033[41m"  // Fondo rojo
#define GREEN   "\033[42m"  // Fondo verde

#define HEIGHT 10  // Tamaño del tablero
#define WIDTH 20

void printBoard(char board[HEIGHT][WIDTH]) {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if ((i + j) % 2 == 0)  
                printf(WHITE "  " RESET);  // Celdas blancas
            else  
                printf(BLACK "  " RESET);  // Celdas negras
        }
        printf("\n");
    }
}


int main() {
    char board[HEIGHT][WIDTH] = {0};  
    printBoard(board);  

    sleep(1);  

    
    return 0;
}

