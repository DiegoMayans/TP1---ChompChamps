# Variables
CC = gcc
CFLAGS = -Wall -Werror -g
LDFLAGS = -lrt -lpthread -lm
SRC_DIR = .
BIN_DIR = ../bin
SOURCES = $(wildcard $(SRC_DIR)/*.c)
FILTERED_FILES = $(SRC_DIR)/shm_adt.c $(SRC_DIR)/round_robin_adt.c 
BINARIES = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(filter-out $(FILTERED_FILES), $(SOURCES)))

.PHONY: all clean players

all: $(BINARIES) players

$(BIN_DIR)/%: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(SRC_DIR)/shm_adt.c -o $@ $(LDFLAGS)

players:
	$(MAKE) -C players

clean:
	rm -rf $(BIN_DIR)
	$(MAKE) -C players clean