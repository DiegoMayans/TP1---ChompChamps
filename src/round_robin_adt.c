#include <sys/mman.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "../includes/round_robin_adt.h"

#define BLOCK 10

struct requester_key {
    int id; // >= 0
};

struct request_t {
    requester_t* requester;
    int requests_amount;
    struct request_t* request_next;
    struct request_t* priority_next;
};

struct round_robin_cdt {
    struct request_t* requests_first;
    struct request_t* requests_last;
    struct request_t* priority_first;
    struct request_t* priority_last;
    struct requester_key* valid_keys;
    int cant_keys;
};

round_robin_adt new_round_robin() {
    round_robin_adt new = (round_robin_adt) malloc(sizeof(struct round_robin_cdt));
    new->valid_keys = NULL;
    new->cant_keys = 0;
	new->priority_first = NULL;
	new->priority_last = NULL;
	new->requests_first = NULL;
	new->requests_last = NULL;
    if (new == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }
    return new;
}

static void assign_valid_key(round_robin_adt round_robin, requester_t* requester) {
    if((round_robin->cant_keys % BLOCK) == 0)
    	round_robin->valid_keys = realloc(round_robin->valid_keys, sizeof(struct requester_key) * (round_robin->cant_keys + BLOCK));
	struct requester_key new_key = {.id = round_robin->cant_keys++};
    round_robin->valid_keys[round_robin->cant_keys - 1] = new_key;
	requester->id = &round_robin->valid_keys[round_robin->cant_keys - 1];
}

	// In order to use a requester it must be first instantiated
void instantiate_requester(round_robin_adt round_robin, requester_t* requester) {
    assign_valid_key(round_robin, requester);
}

bool equals(requester_t* req1, requester_t* req2) {
    return (req1->id->id - req2->id->id) == 0;
}

// If found, return the request_t node corresponding to the requester. If not, return NULL
static struct request_t* find_requester(round_robin_adt round_robin, requester_t* requester) {
    struct request_t* current = round_robin->priority_first;
    if (current == NULL) {
        return NULL;
    }
    while (current != NULL) {
        if (equals(requester, current->requester)) {
            return current;
        }
        current = current->priority_next;
    }
    return NULL;
}

// Returns true if incoming has more priority than active
static bool has_priority(requester_t* incoming, requester_t* active, round_robin_adt round_robin) {
    struct request_t* current = round_robin->priority_first;

    while (current != NULL) {
        if (equals(current->requester, incoming)) {
            return true;
        }
        if (equals(current->requester, active)) {
            return false;
        }
        current = current->priority_next;
    }
    return false;
}

// Associates requester to a request node
static struct request_t* create_node(round_robin_adt round_robin, requester_t* requester) {
    struct request_t* new = (struct request_t*) malloc(sizeof(struct request_t));
    if (new == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }
    new->requester = requester;
    new->requests_amount = 1;
    new->priority_next = NULL;
    new->request_next = NULL;
    if (round_robin->requests_first == NULL) {
        round_robin->requests_first = new;
        if (round_robin->priority_first == NULL) {
            round_robin->priority_first = new;
        }
    }
    if (round_robin->priority_last != NULL) {
        round_robin->priority_last->priority_next = new;
    }
    round_robin->priority_last = new;
    if (round_robin->requests_last != NULL) {
        round_robin->requests_last->request_next = new;
    }
    round_robin->requests_last = new;
    return new;
}

static struct request_t* push_helper(round_robin_adt round_robin, struct request_t* current, struct request_t* to_add) {
    if (current == NULL) {
        to_add->requests_amount = 1;
        round_robin->requests_last->request_next = to_add;
        return to_add;
    }
    if (equals(to_add->requester, current->requester)) {
        current->requests_amount++;
        return current;
    } else if (has_priority(to_add->requester, current->requester, round_robin)) {
        to_add->requests_amount = 1;
        to_add->request_next = current;
        return to_add;
    } else {
        current->request_next = push_helper(round_robin, current->request_next, to_add);
        return current;
    }
}

int push(round_robin_adt round_robin, requester_t* requester) {
    struct request_t* requester_node = find_requester(round_robin, requester);
    if (requester_node == NULL) {
        requester_node = create_node(round_robin, requester);
    } else {
        if (round_robin->requests_first == NULL) {
            requester_node->requests_amount = 1;
            round_robin->requests_first = requester_node;
            round_robin->requests_last = requester_node;
        } else {
            round_robin->requests_first = push_helper(round_robin, round_robin->requests_first, requester_node);
        }
    }
    return EXIT_SUCCESS;
}

requester_t* pop(round_robin_adt round_robin) {
    if (round_robin->requests_first == NULL) {
        return NULL;
    }

    // Request list management
    struct request_t* to_return = round_robin->requests_first;
    round_robin->requests_first = to_return->request_next;
    if (--to_return->requests_amount > 0) {
        // If there only was 1 element, and it had more than 1 request, we need for it to keep at request_first
        if (to_return->request_next == NULL)
            round_robin->requests_first = to_return;
        round_robin->requests_last->request_next = to_return;
        round_robin->requests_last = to_return;
    }
    to_return->request_next = NULL;

    // Priority list management
    if (to_return->priority_next != NULL) {
        round_robin->priority_first = to_return->priority_next;
        round_robin->priority_last->priority_next = to_return;
    }
    round_robin->priority_last = to_return;
    to_return->priority_next = NULL;

    return to_return->requester;
}

static void free_helper(round_robin_adt round_robin, struct request_t* current) {
    if (current != NULL)
        free_helper(round_robin, current->priority_next);
    else 
        return;
    free(current);
}

void free_round_robin(round_robin_adt round_robin) {
    struct request_t* current = round_robin->priority_first;
    free_helper(round_robin, current);

    // Liberar el arreglo de valid_keys
    free(round_robin->valid_keys);
    free(round_robin);
}