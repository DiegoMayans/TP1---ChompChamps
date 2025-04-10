#ifndef ROUND_ROBIN_ADT_H
#define ROUND_ROBIN_ADT_H

typedef struct round_robin_cdt* round_robin_adt;

//Struct type for Requesters
typedef struct {
    //Must be >= 0
    int id;
    //User-defined object
    int fd;
} requester_t;

//User must define an upper bound:
#define MAX_REQUESTERS 9

//Notas para nosotros:

//Retorna un puntero a shared memory
round_robin_adt new_round_robin(requester_t requesters[], int cant_requesters);
//No retorna nada
void push_request(round_robin_adt round_robin, requester_t requester);
//Retorna id de solicitudes restantes, -1 si no hay más solicitudes
int pop_request(round_robin_adt round_robin);

#endif