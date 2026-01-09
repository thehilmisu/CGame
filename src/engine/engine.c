#include "engine.h"
#include "../window/window.h"
#include "../graphics/camera.h"
#include "../graphics/renderer.h"
#include "../graphics/state.h"
#include "../graphics/shader.h"
#include "../world/terrain.h"
#include "../world/water.h"
#include "../world/skybox.h"
#include "../graphics/texture.h"
#include "../file_ops.h"
#include "../gui.h"
#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>

// Mouse tracking for camera
static double last_mouse_x = WINDOW_WIDTH / 2.0;
static double last_mouse_y = WINDOW_HEIGHT / 2.0;
static bool first_mouse = true;

// Callbacks
static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;
    
    if (first_mouse) {
        last_mouse_x = xpos;
        last_mouse_y = ypos;
        first_mouse = false;
        return;
    }
    
    Camera* camera = glfwGetWindowUserPointer(window);
    if (!camera) return;
    
    float xoffset = (float)(xpos - last_mouse_x);
    float yoffset = (float)(last_mouse_y - ypos);
    last_mouse_x = xpos;
    last_mouse_y = ypos;
    
    xoffset *= CAMERA_SENSITIVITY;
    yoffset *= CAMERA_SENSITIVITY;
    
    camera->yaw += xoffset;
    camera->pitch += yoffset;
    
    // Clamp pitch
    if (camera->pitch > 89.0f) camera->pitch = 89.0f;
    if (camera->pitch < -89.0f) camera->pitch = -89.0f;
}

static void engine_setup_world(Engine* engine) {
    // Create terrain seed
    srand(time(NULL));
    int random_seed = rand();
    printf("Using terrain seed: %d\n", random_seed);
    
    engine->seed = malloc(sizeof(TerrainSeed));
    *engine->seed = terrain_seed_create(random_seed);
    
    // Load terrain shader and texture
    const char* vertex_shader_src = load_shader_source("assets/shaders/terrainvert.glsl");
    const char* fragment_shader_src = load_shader_source("assets/shaders/terrainfrag.glsl");
    GLuint terrain_shader = shader_compile(vertex_shader_src, fragment_shader_src);
    GLuint terrain_texture = texture_load("assets/textures/terraintextures.png");
    
    free((void*)vertex_shader_src);
    free((void*)fragment_shader_src);
    
    if (terrain_texture == 0) {
        fprintf(stderr, "Warning: Failed to load terrain texture, using fallback colors\n");
    }
    
    // Create terrain LOD manager
    engine->terrain = malloc(sizeof(TerrainLODManagerGL));
    *engine->terrain = terrain_lod_manager_create(engine->seed);
    engine->terrain->terrain_shader = terrain_shader;
    engine->terrain->terrain_texture = terrain_texture;
    
    printf("\nInitializing terrain with %d LOD levels...\n", MAX_LOD);
    printf("Generating terrain chunks...\n");
    terrain_lod_manager_generate_all(engine->terrain, engine->seed, 0, 0);
    printf("Terrain initialized\n");
    
    // Create water manager
    printf("Initializing water...\n");
    engine->water = malloc(sizeof(WaterManagerGL));
    *engine->water = water_manager_init();
    printf("Water initialized\n");
    
    // Create skybox
    printf("Initializing skybox...\n");
    engine->skybox = malloc(sizeof(SkyboxGL));
    *engine->skybox = skybox_init();
    printf("Skybox initialized\n");
}

static void engine_setup_gui(Engine* engine, GLFWwindow* window) {
    nk_glfw3_init(&engine->nk_glfw, window, NK_GLFW3_DEFAULT);
    
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&engine->nk_glfw, &atlas);
    nk_glfw3_font_stash_end(&engine->nk_glfw);
    
    fps_counter_init(&engine->fps_counter);
}

Engine* engine_init(void) {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return NULL;
    }
    
    // Create window
    GLFWwindow* window = window_create(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return NULL;
    }
    
    // Load OpenGL functions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        glfwTerminate();
        return NULL;
    }
    
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    // Create engine
    Engine* engine = malloc(sizeof(Engine));
    if (!engine) {
        fprintf(stderr, "Failed to allocate engine\n");
        glfwTerminate();
        return NULL;
    }
    
    engine->window = window;
    engine->last_time = glfwGetTime();
    engine->running = true;
    
    // Allocate and initialize camera
    engine->camera = malloc(sizeof(Camera));
    if (!engine->camera) {
        fprintf(stderr, "Failed to allocate camera\n");
        free(engine);
        glfwTerminate();
        return NULL;
    }
    camera_init(engine->camera);
    glfwSetWindowUserPointer(window, engine->camera);
    
    // Set window callbacks
    window_set_callbacks(window, framebuffer_size_callback, mouse_callback);
    window_set_cursor_mode(window, GLFW_CURSOR_DISABLED);
    
    // Setup OpenGL state
    renderer_set_opengl_state();
    
    // Setup game world
    engine_setup_world(engine);
    
    // Setup GUI
    engine_setup_gui(engine, window);
    
    return engine;
}

void engine_run(Engine* engine) {
    Camera* camera = engine->camera;
    
    while (!window_should_close(engine->window)) {
        double current_time = glfwGetTime();
        float dt = (float)(current_time - engine->last_time);
        engine->last_time = current_time;
        
        // Update FPS counter
        fps_counter_update(&engine->fps_counter, current_time);
        
        // Process input
        camera_process_input(camera, engine->window, dt);
        
        // Clear
        renderer_clear();
        
        // Update camera matrices
        float view_matrix[16], proj_matrix[16];
        camera_update_view(camera, view_matrix);
        
        int width, height;
        window_get_size(engine->window, &width, &height);
        camera_update_projection(camera, width, height, proj_matrix);
        
        // Update terrain - generate new chunks as camera moves (infinite terrain)
        // Pass raw camera world position - the update function calculates chunk positions
        terrain_lod_manager_update(engine->terrain, engine->seed, camera->pos_x, camera->pos_z);
        
        // Render skybox first
        skybox_render(engine->skybox, proj_matrix, view_matrix);
        
        // Restore OpenGL state for terrain
        state_restore_defaults();
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        
        // Render terrain
        terrain_lod_manager_render(engine->terrain, view_matrix, proj_matrix,
                                   camera->pos_x, camera->pos_y, camera->pos_z,
                                   (float)current_time);
        
        // Render water with transparency
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        state_enable_blend();
        state_set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        state_set_depth_mask(GL_FALSE);
        state_disable_cull_face();
        
        water_render_gl(engine->water, proj_matrix, view_matrix,
                       camera->pos_x, camera->pos_y, camera->pos_z,
                       (float)current_time);
        
        // Restore depth function
        glDepthFunc(GL_LESS);
        state_restore_defaults();
        
        // Render GUI
        nk_glfw3_new_frame(&engine->nk_glfw);
        
        window_get_size(engine->window, &width, &height);
        gui_render_fps(&engine->nk_glfw.ctx, &engine->fps_counter, width, height);
        
        nk_glfw3_render(&engine->nk_glfw, NK_ANTI_ALIASING_ON,
                       MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
        
        state_restore_defaults();
        
        glfwSwapBuffers(engine->window);
        glfwPollEvents();
    }
}

void engine_cleanup(Engine* engine) {
    printf("\nCleaning up...\n");
    
    // Cleanup world
    if (engine->terrain) {
        terrain_lod_manager_cleanup(engine->terrain);
        free(engine->terrain);
    }
    if (engine->water) {
        water_cleanup_gl(engine->water);
        free(engine->water);
    }
    if (engine->skybox) {
        skybox_cleanup(engine->skybox);
        free(engine->skybox);
    }
    if (engine->seed) {
        free(engine->seed);
    }
    
    // Cleanup camera
    if (engine->camera) {
        free(engine->camera);
    }
    
    // Cleanup GUI
    nk_glfw3_shutdown(&engine->nk_glfw);
    
    // Cleanup window
    glfwTerminate();
    
    free(engine);
    
    printf("Done!\n");
}
