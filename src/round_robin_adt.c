#include <sys/mman.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "../includes/round_robin_adt.h"

struct requester_key {
	int id; //	>= 0
};

typedef struct {
	requester_t requester;
	int requests_amount;
} request_t;

struct round_robin_cdt {
	request_t requests_queue[MAX_REQUESTERS];
	int requests_size;
	requester_t priority_queue[MAX_REQUESTERS];
	int priority_current_size;
	int cant_requesters;
};

static struct round_robin_cdt round_robin = {0};

struct requester_key keys[MAX_REQUESTERS];
static requester_id valid_id[MAX_REQUESTERS];

round_robin_adt new_round_robin(requester_t requesters[], int cant_requesters) {
	for(int i = 0; i < cant_requesters; i++) {
		keys[i].id = i;
		requesters[i].id = &keys[i];
		valid_id[i] = &keys[i];
	}
	round_robin.requests_size = 0;
	round_robin.cant_requesters = cant_requesters;
	round_robin.priority_current_size = 0;
	return &round_robin;
}

bool equals(requester_t req1, requester_t req2) {
	return (req1.id->id - req2.id->id) == 0;
}

static bool belongs(round_robin_adt round_robin, requester_t requester) {
	bool found = false;
	for(int i = 0; i < round_robin->priority_current_size && !found; i++) {
		if(equals(requester, round_robin->priority_queue[i])) {
			found = true;
		}
	}
	return found;
}


static void init_requester(round_robin_adt round_robin, requester_t requester, int index) {
	round_robin->requests_queue[index].requester = requester;
	round_robin->requests_queue[index].requests_amount = 1;
}

static int has_priority(requester_t incoming, requester_t active, round_robin_adt round_robin) {
	int incoming_index, active_index;
	for(int i = 0; i < round_robin->priority_current_size; i++) {
		if(equals(incoming, round_robin->priority_queue[i])) {
			incoming_index = i;
		}
		if(equals(active, round_robin->priority_queue[i])) {
			active_index = i;
		}
	}
	return incoming_index - active_index;
}

int push_request(round_robin_adt round_robin, requester_t requester) {
	if(!belongs(round_robin, requester)) {
		bool found = false;
		for(int i = 0; i < round_robin->cant_requesters && !found; i++) {
			if(requester.id->id == valid_id[i]->id) {
				round_robin->priority_queue[round_robin->priority_current_size++] = requester;
				found = true;
			}
		}
		if(!found) {
			return EXIT_FAILURE;
		}
	}
	bool added = false;
	int i = 0;
	for(; i < round_robin->requests_size && !added; i++) {
		int priority;
		if((priority = has_priority(requester, round_robin->requests_queue[i].requester, round_robin)) < 0) {
			for(int k = round_robin->requests_size; k > i; k--) {
				round_robin->requests_queue[k] = round_robin->requests_queue[k - 1];
			}
			init_requester(round_robin, requester, i);
			round_robin->requests_size++;
			added = true;
		} else if(priority == 0) {
			round_robin->requests_queue[i].requests_amount++;
			added = true;
		}
	}
	if(!added) {
		init_requester(round_robin, requester, i);
		round_robin->requests_size++;
	}

	return EXIT_SUCCESS;
}

requester_t pop_request(round_robin_adt round_robin) {
	if(round_robin->requests_size <= 0) {
		static struct requester_key error_key = {.id = -1}; 
		requester_t to_return_error = {.id = &error_key};

		return to_return_error;
	}
	request_t to_return = round_robin->requests_queue[0];

	for(int i = 1; i < round_robin->requests_size; i++) {
		round_robin->requests_queue[i - 1] = round_robin->requests_queue[i];
	}
	if((--to_return.requests_amount) > 0) {
		round_robin->requests_queue[round_robin->requests_size - 1] = to_return;
	} else {
		round_robin->requests_size--;
	}
	int index = -1;
	for(int i = 0; i < round_robin->priority_current_size && index == -1; i++) {
		if(equals(to_return.requester, round_robin->priority_queue[i])) {
			index = i;
		}
	}
	for(int i = index + 1; i < round_robin->priority_current_size; i++) {
		round_robin->priority_queue[i - 1] = round_robin->priority_queue[i];
	}
	round_robin->priority_queue[round_robin->priority_current_size - 1] = to_return.requester;

	return to_return.requester;
}

bool is_id_valid (requester_t requester){
	return (requester.id->id != -1);
}