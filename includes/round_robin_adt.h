#ifndef ROUND_ROBIN_ADT_H
#define ROUND_ROBIN_ADT_H

typedef struct round_robin_cdt* round_robin_adt;
typedef struct requester_key* requester_id;

//Struct type for Requesters
typedef struct {
    requester_id id;
    //User-defined object
    int fd;
} requester_t;

//User must define an upper bound:
#define MAX_REQUESTERS 9

//Notas para nosotros:

//Retorna un puntero a shared memory
round_robin_adt new_round_robin(requester_t requesters[], int cant_requesters);
//No retorna nada
int push_request(round_robin_adt round_robin, requester_t requester);
//Retorna id de solicitudes restantes, -1 si no hay más solicitudes
requester_t pop_request(round_robin_adt round_robin);
//Retorna si dos requesters son iguales
bool equals(requester_t req1, requester_t req2);
//Retorna si el id es valido
bool is_id_valid (requester_t requester);

#endif