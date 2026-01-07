
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
SRC_DIR = src
BUILD_DIR = build
TARGET = Game

# Include paths
INCLUDES = -I$(SRC_DIR) \
		   -I$(SRC_DIR)/engine \
		   -I$(SRC_DIR)/graphics \
		   -I$(SRC_DIR)/math \
		   -I$(SRC_DIR)/window \
		   -I$(SRC_DIR)/world \
           -Iinclude \
           -Iinclude/glad \
           -Iinclude/nuklear \
           $(shell pkg-config --cflags glfw3)

# Library paths and libraries
LIBS = $(shell pkg-config --libs glfw3) -framework OpenGL -lm

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/noise.c \
          $(SRC_DIR)/glad.c \
          $(SRC_DIR)/file_ops.c \
          $(SRC_DIR)/graphics/texture.c \
          $(SRC_DIR)/stb_impl.c \
          $(SRC_DIR)/nuklear_impl.c \
          $(SRC_DIR)/gui.c \
          $(SRC_DIR)/math/math_ops.c \
          $(SRC_DIR)/window/window.c \
          $(SRC_DIR)/graphics/camera.c \
          $(SRC_DIR)/graphics/renderer.c \
          $(SRC_DIR)/graphics/state.c \
          $(SRC_DIR)/graphics/shader.c \
          $(SRC_DIR)/engine/engine.c \
          $(SRC_DIR)/world/terrain.c \
          $(SRC_DIR)/world/water.c \
          $(SRC_DIR)/world/skybox.c

# Object files
OBJECTS = $(BUILD_DIR)/main.o \
          $(BUILD_DIR)/noise.o \
          $(BUILD_DIR)/glad.o \
          $(BUILD_DIR)/file_ops.o \
          $(BUILD_DIR)/graphics/texture.o \
          $(BUILD_DIR)/stb_impl.o \
          $(BUILD_DIR)/nuklear_impl.o \
          $(BUILD_DIR)/gui.o \
          $(BUILD_DIR)/math/math_ops.o \
          $(BUILD_DIR)/window/window.o \
          $(BUILD_DIR)/graphics/camera.o \
          $(BUILD_DIR)/graphics/renderer.o \
          $(BUILD_DIR)/graphics/state.o \
          $(BUILD_DIR)/graphics/shader.o \
          $(BUILD_DIR)/engine/engine.o \
          $(BUILD_DIR)/world/terrain.o \
          $(BUILD_DIR)/world/water.o \
          $(BUILD_DIR)/world/skybox.o

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Create subdirectories in build
$(BUILD_DIR)/math $(BUILD_DIR)/window $(BUILD_DIR)/graphics $(BUILD_DIR)/engine $(BUILD_DIR)/world:
	mkdir -p $@

# Compile object files from src (with subdirectories)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
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
