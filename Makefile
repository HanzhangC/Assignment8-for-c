# define C compiler & flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g

TARGET = allocate
OBJ = memory_management.o

# Target to build the final executable
.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile memory_management.o from memory_management.c
memory_management.o: memory_management.c memory_management.h
	$(CC) $(CFLAGS) -c $<

# Clean target to remove compiled files
clean:
	rm -f $(TARGET) $(OBJ)
