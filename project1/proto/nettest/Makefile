# Compiler and flags
CC = gcc
CFLAGS = -g

# Targets
TARGETS = server-test client-test

# Default target builds both programs
all: $(TARGETS)

server-test: server-test.c
	$(CC) $(CFLAGS) -o server-test server-test.c

client-test: client-test.c
	$(CC) $(CFLAGS) -o client-test client-test.c

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
