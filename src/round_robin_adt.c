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

void print_request_list(round_robin_adt round_robin) {
    struct request_t* current = round_robin->requests_first;
    while (current != NULL) {
        printf("Requester ID: %d, Requests Amount: %d\n", current->requester->id->id, current->requests_amount);
        current = current->request_next;
    }
}
void print_priority_list(round_robin_adt round_robin) {
    struct request_t* current = round_robin->priority_first;
    while (current != NULL) {
        printf("Requester ID: %d, Requests Amount: %d\n", current->requester->id->id, current->requests_amount);
        current = current->priority_next;
    }
}

void print_lists(round_robin_adt round_robin) {
    printf("Request List:\n");
    print_request_list(round_robin);
    printf("Priority List:\n");
    print_priority_list(round_robin);
}

static void assign_valid_key(round_robin_adt round_robin, requester_t *requester) {
    // Check if we need to grow the valid_keys array.
    if ((round_robin->cant_keys % BLOCK) == 0) {
        size_t old_count = round_robin->cant_keys;
        struct requester_key* old_valid_keys = round_robin->valid_keys;
        round_robin->valid_keys = realloc(round_robin->valid_keys, sizeof(struct requester_key) * (old_count + BLOCK));
        if (round_robin->valid_keys == NULL) {
            perror("Realloc failed");
            exit(EXIT_FAILURE);
        }
        // If valid_keys moved, update all stored pointers.
        if (old_valid_keys != round_robin->valid_keys) {
            struct request_t* current = round_robin->priority_first;
            while (current != NULL) {
                int id_index = current->requester->id->id;
                current->requester->id = &round_robin->valid_keys[id_index];
                current = current->priority_next;
            }
        }
    }
    
    // Use the current cant_keys as the index for the new key.
    int new_index = round_robin->cant_keys;
    // Create and store the new key.
    round_robin->valid_keys[new_index].id = new_index;
    
    // Set the requester->id pointer to the address within valid_keys.
    requester->id = &round_robin->valid_keys[new_index];
    round_robin->cant_keys++;
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

    free(round_robin->valid_keys);
    free(round_robin);
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LARGE_TEST_SIZE 100 // Number of requesters for stress testing

void stress_test_round_robin() {
    printf("Starting stress test...\n");
    round_robin_adt rr = new_round_robin();
    if (rr == NULL) {
        fprintf(stderr, "Failed to create round_robin_adt\n");
        exit(EXIT_FAILURE);
    }

    // Create a large number of requesters
    requester_t *req = malloc(sizeof(requester_t) * LARGE_TEST_SIZE);
    if (req == NULL) {
        perror("Failed to allocate memory for requesters");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < LARGE_TEST_SIZE; i++) {
        req[i].fd = i;
        req[i].player_index = i;
        instantiate_requester(rr, &req[i]);
    }

    // Push all requesters into the round-robin
    for (int i = 0; i < LARGE_TEST_SIZE; i++) {
        push(rr, &req[i]);
    }

    // Pop all requesters and verify the order
    for (int i = 0; i < LARGE_TEST_SIZE; i++) {
        requester_t *popped = pop(rr);
        if (popped != &req[i]) {
            fprintf(stderr, "Error: Expected requester %d, but got %d\n", req[i].fd, popped->fd);
            free(req);
            free_round_robin(rr);
            exit(EXIT_FAILURE);
        }
    }

    printf("Stress test passed successfully.\n");

    // Free resources
    free(req);
    free_round_robin(rr);
}

void edge_case_test_round_robin() {
    printf("Starting edge case tests...\n");
    round_robin_adt rr = new_round_robin();
    if (rr == NULL) {
        fprintf(stderr, "Failed to create round_robin_adt\n");
        exit(EXIT_FAILURE);
    }

    // Test popping from an empty round-robin
    requester_t *popped = pop(rr);
    if (popped != NULL) {
        fprintf(stderr, "Error: Expected NULL when popping from an empty round-robin\n");
        free_round_robin(rr);
        exit(EXIT_FAILURE);
    }

    // Test pushing duplicate requesters
    requester_t req;
    req.fd = 1;
    req.player_index = 1;
    instantiate_requester(rr, &req);

    push(rr, &req);
    push(rr, &req); // Push the same requester again

    popped = pop(rr);
    if (popped != &req) {
        fprintf(stderr, "Error: Expected requester %d, but got %d\n", req.fd, popped->fd);
        free_round_robin(rr);
        exit(EXIT_FAILURE);
    }

    popped = pop(rr);
    if (popped != NULL) {
        fprintf(stderr, "Error: Expected NULL after popping all requesters\n");
        free_round_robin(rr);
        exit(EXIT_FAILURE);
    }

    printf("Edge case tests passed successfully.\n");

    // Free resources
    free_round_robin(rr);
}
