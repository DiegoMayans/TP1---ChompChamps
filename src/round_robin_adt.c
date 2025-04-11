#include <sys/mman.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "../includes/round_robin_adt.h"

struct requester_key {
	//id >= 0
	int id;
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

static struct requester_key valid_id[MAX_REQUESTERS];

round_robin_adt new_round_robin(requester_t requesters[], int cant_requesters) {
	for(int i = 0; i < cant_requesters; i++) {
		requesters[i].id = i;
		valid_id[i].id = i;
	}
	round_robin.requests_size = 0;
	round_robin.cant_requesters = cant_requesters;
	round_robin.priority_current_size = 0;
	return &round_robin;
}

bool equals(requester_t req1, requester_t req2) {
	return (req1.id - req2.id) == 0;
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
			if(requester.id == valid_id[i].id) {
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

int pop_request(round_robin_adt round_robin) {
	if(round_robin->requests_size <= 0) {
		return -1;
	}
	request_t toReturn = round_robin->requests_queue[0];

	for(int i = 1; i < round_robin->requests_size; i++) {
		round_robin->requests_queue[i - 1] = round_robin->requests_queue[i];
	}
	if((--toReturn.requests_amount) > 0) {
		round_robin->requests_queue[round_robin->requests_size - 1] = toReturn;
	} else {
		round_robin->requests_size--;
	}
	int index = -1;
	for(int i = 0; i < round_robin->priority_current_size && index == -1; i++) {
		if(equals(toReturn.requester, round_robin->priority_queue[i])) {
			index = i;
		}
	}
	for(int i = index + 1; i < round_robin->priority_current_size; i++) {
		round_robin->priority_queue[i - 1] = round_robin->priority_queue[i];
	}
	round_robin->priority_queue[round_robin->priority_current_size - 1] = toReturn.requester;

	return toReturn.requester.id;
}

#define CANT 3

int main(void) {
	requester_t requesters[CANT];
	requesters[0].name = "Diego"; requesters[1].name = "Mitch"; requesters[2].name = "Juampi";	

	round_robin_adt robin_hood = new_round_robin(requesters, CANT);
	push_request(robin_hood, requesters[0]);
	push_request(robin_hood, requesters[0]);
	push_request(robin_hood, requesters[0]);
	push_request(robin_hood, requesters[2]);
	for(int i = 0; i < 2; i++) {
		printf("requests_amount: %d\nid: %d\tname: %s\n", robin_hood->requests_queue[i].requests_amount, robin_hood->requests_queue[i].requester.id, robin_hood->requests_queue[i].requester.name);
	}
	for(int i = 0; i < 5; i++) {
		printf("pop: %d\n", pop_request(robin_hood));
	}
	for(int i = 0; i < 2; i++) {
		printf("requests_amount: %d\nid: %d\tname: %s\n", robin_hood->requests_queue[i].requests_amount, robin_hood->requests_queue[i].requester.id, robin_hood->requests_queue[i].requester.name);
	}
}
