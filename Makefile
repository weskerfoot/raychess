# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -O2 -std=c99 -I./raylib/src

# Raylib Flags (adjust the path if Raylib is not in the default location)
RAYLIB_FLAGS = $(shell pkg-config --cflags --libs raylib)

# Output executable name
TARGET = c_chess

# Source files
SRC = main.c camera/rlTPCamera.c

# Default rule
all: $(TARGET)

# Compile and link in one step
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) -I./raylib/src -L./raylib/raylib -lraylib -lm

# Debug target
debug: CC = clang
debug: CFLAGS = -Wall -O0 -g -std=c99 -fsanitize=address -fno-omit-frame-pointer -I./raylib/src
debug: LDFLAGS = -fsanitize=address
debug:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(RAYLIB_FLAGS) $(LDFLAGS) -lm

# Clean up build files
clean:
	rm -f $(TARGET)

# Phony targets (not actual files)
.PHONY: all clean debug

