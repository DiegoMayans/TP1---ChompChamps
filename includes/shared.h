#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <stdlib.h>

#include "./defs.h"
#include "./shm_adt.h"

void set_coordinates(int *x, int *y, direction_t move);
void test_exit(const char *msg, int condition);

#endif
