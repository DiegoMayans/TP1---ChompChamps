#ifndef ROUND_ROBIN_ADT_H
#define ROUND_ROBIN_ADT_H

typedef struct round_robin_cdt* round_robin_adt;
typedef struct requester_key* requester_id;

//Struct type for Requesters
typedef struct {
    requester_id id;
    //User-defined object
    int fd, player_index;
} requester_t;


//Retorna un puntero a shared memory
round_robin_adt new_round_robin();
//No retorna nada
int push(round_robin_adt round_robin, requester_t* requester);
//Retorna el siguiente requester, NULL si no hay más solicitudes
requester_t* pop(round_robin_adt round_robin);
// In order to use a requester it must be first instantiated
void instantiate_requester(round_robin_adt round_robin, requester_t* requester);

#endif