#ifndef ENGINE_H
#define ENGINE_H

// Include glad before GLFW to avoid OpenGL header conflicts
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>

#include "../graphics/camera.h"
#include "../gui.h"
#include "../world/terrain.h"
#include <stdbool.h>

typedef struct {
    GLFWwindow* window;
    Camera* camera;
    
    // World
    TerrainLODManagerGL* terrain;
    TerrainSeed* seed;
    WaterManagerGL* water;
    SkyboxGL* skybox;
    
    // GUI
    struct nk_glfw nk_glfw;
    DebugElements* gui_debug_elements;
    
    // Timing
    double last_time;
    bool running;
} Engine;

// Initialize the engine (creates window, loads resources, etc.)
Engine* engine_init(void);

// Run the main game loop
void engine_run(Engine* engine);

// Cleanup and shutdown
void engine_cleanup(Engine* engine);

#endif // ENGINE_H
