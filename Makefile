CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
LDFLAGS = -pthread

# Targets
TARGETS = builder library client

# Default target
all: $(TARGETS)

builder: builder.c common.h
	$(CC) $(CFLAGS) -o builder builder.c $(LDFLAGS)

library: library.c common.h
	$(CC) $(CFLAGS) -o library library.c $(LDFLAGS)

client: client.c common.h
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

# Run the simulation
run: all
	./builder

# Clean up
clean:
	rm -f $(TARGETS) log.txt

.PHONY: all run clean
