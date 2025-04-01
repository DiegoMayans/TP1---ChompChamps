# Variables
CC = gcc
CFLAGS = -Wall -Werror -g
LDFLAGS = -lrt -lpthread -lm
SRC_DIR = .
BIN_DIR = ../bin
SOURCES = $(wildcard $(SRC_DIR)/*.c)
BINARIES = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(filter-out $(SRC_DIR)/shm.c, $(SOURCES)))

.PHONY: all clean

all: $(BINARIES)

$(BIN_DIR)/%: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(SRC_DIR)/shm.c -o $@ $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR)