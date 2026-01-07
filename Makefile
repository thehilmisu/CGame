
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
SRC_DIR = src
BUILD_DIR = build
TARGET = Game

# Include paths
INCLUDES = -I$(SRC_DIR) \
           -Iinclude \
           -Iinclude/glad \
           -Iinclude/nuklear \
           $(shell pkg-config --cflags glfw3)

# Library paths and libraries
LIBS = $(shell pkg-config --libs glfw3) -framework OpenGL -lm

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/terrain.c \
          $(SRC_DIR)/noise.c \
          $(SRC_DIR)/glad.c \
          $(SRC_DIR)/file_ops.c \
	      $(SRC_DIR)/texture.c \
	      $(SRC_DIR)/stb_impl.c \
	      $(SRC_DIR)/nuklear_impl.c \
	      $(SRC_DIR)/gui.c

# Object files
OBJECTS = $(BUILD_DIR)/main.o \
          $(BUILD_DIR)/terrain.o \
          $(BUILD_DIR)/noise.o \
          $(BUILD_DIR)/glad.o \
          $(BUILD_DIR)/file_ops.o \
          $(BUILD_DIR)/texture.o \
          $(BUILD_DIR)/stb_impl.o \
          $(BUILD_DIR)/nuklear_impl.o \
          $(BUILD_DIR)/gui.o

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile object files from src_c
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile GLAD from src
$(BUILD_DIR)/glad.o: $(SRC_DIR)/glad.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)
	@echo ""
	@echo "Build complete: $(TARGET)"
	@echo ""
	@echo "Run with: ./$(TARGET)"
	@echo ""

# Clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
