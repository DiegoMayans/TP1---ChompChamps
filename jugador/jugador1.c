#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WIDTH 20
#define HEIGHT 10

// Snake structure
typedef struct {
    int x, y;
} Snake;

// Function to clear the screen
void clear_screen() {
    printf("\033[H\033[J");
}

// Function to display the game map
void display_map(Snake snake, int food_x, int food_y) {
    clear_screen();

    // Loop through the map and print the characters
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1) {
                printf("#"); // Wall
            } else if (x == snake.x && y == snake.y) {
                printf("O"); // Snake head
            } else if (x == food_x && y == food_y) {
                printf("*"); // Food
            } else {
                printf(" "); // Empty space
            }
        }
        printf("\n");
    }
}

int main() {
    Snake snake = {WIDTH / 2, HEIGHT / 2}; // Initial snake position
    int food_x = rand() % (WIDTH - 2) + 1;
    int food_y = rand() % (HEIGHT - 2) + 1;

    while (1) {
        display_map(snake, food_x, food_y);
        usleep(200000); // Slow down the game loop

        // Here you can add input handling, snake movement logic, etc.
    }

    return 0;
}