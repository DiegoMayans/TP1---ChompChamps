# Variables
CC = gcc
CFLAGS = -Wall -Werror -g
LDFLAGS = -lrt -lpthread -lm
SRC_DIR = ..
PLAYERS_DIR = .
BIN_DIR = ../../bin
SOURCES = $(wildcard $(PLAYERS_DIR)/*.c)
BINARIES = $(patsubst $(PLAYERS_DIR)/%.c, $(BIN_DIR)/%, $(SOURCES))

.PHONY: all clean

all: $(BINARIES)

$(BIN_DIR)/%: $(PLAYERS_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(SRC_DIR)/shm_adt.c $(SRC_DIR)/shared.c -o $@ $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR)