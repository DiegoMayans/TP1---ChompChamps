#include "../includes/shared.h"

void set_coordinates(int *x, int *y, direction_t move) {
    static const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

    *x += dx[move];
    *y += dy[move];
}