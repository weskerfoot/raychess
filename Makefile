# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -O2 -std=c99

# Raylib Flags (adjust the path if Raylib is not in the default location)
RAYLIB_FLAGS = $(shell pkg-config --cflags --libs raylib)

# Output executable name
TARGET = c_chess

# Source files
SRC = main.c

# Default rule
all: $(TARGET)

# Compile and link in one step
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(RAYLIB_FLAGS) -lm

# Clean up build files
clean:
	rm -f $(TARGET)

# Phony targets (not actual files)
.PHONY: all clean
