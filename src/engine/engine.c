#include "engine.h"
#include "../window/window.h"
#include "../graphics/renderer.h"
#include "../graphics/state.h"
#include "../graphics/shader.h"
#include "../world/terrain.h"
#include "../entities/player.h"
#include "../gui.h"
#include "../config.h"
#include "../file_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    
    last_mouse_x = xpos;
    last_mouse_y = ypos;
}


static void engine_setup_world(Engine* engine) {
    // Create terrain seed
    srand(time(NULL));
    int random_seed = rand();
    printf("Using terrain seed: %d\n", random_seed);

    engine->seed = malloc(sizeof(TerrainSeed));
    *engine->seed = terrain_seed_create(random_seed);

    // Create terrain LOD manager
    engine->terrain = malloc(sizeof(TerrainLODManagerGL));
    *engine->terrain = terrain_lod_manager_create(engine->seed);

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

    printf("Initializin GUI ...\n");
    engine->gui_debug_elements = malloc(sizeof(DebugElements));
    *engine->gui_debug_elements = gui_debug_elements_init();
}

static void engine_setup_entities(Engine* engine) {
    printf("Initializing entity system...\n");

    // Create entity manager
    engine->entity_manager = entity_manager_create();

    // Load and compile model shaders
    const char* model_vert = load_shader_source("assets/shaders/modelvert.glsl");
    const char* model_frag = load_shader_source("assets/shaders/modelfrag.glsl");
    engine->entity_manager->model_shader = shader_compile(model_vert, model_frag);

    // Load player model
    Model* player_model = entity_manager_load_model(
        engine->entity_manager,
        "assets/models/plane.obj"
    );

    if (player_model) {
        // Position player in front of camera based on camera's initial position and yaw
        // Camera is at (0, CAMERA_INITIAL_Y, 0) with yaw=0 (looking along +Z)
        // Place player in front of camera (along +Z direction)
        float forward_distance = 30.0f;  // Distance in front of camera
        float player_x = engine->camera->pos_x + forward_distance;
        float player_y = CAMERA_INITIAL_Y;  // Same height as camera initially
        float player_z = engine->camera->pos_z + forward_distance;
        
        engine->player = entity_manager_create_entity(
            engine->entity_manager,
            ENTITY_TYPE_PLAYER,
            player_model,
            (float[]){player_x, player_y, player_z},// position in front of camera
            (float[]){0.0f, 0.0f, 0.0f},    // rotation
            (float[]){0.5f, 0.5f, 0.5f}     // scale
        );
        printf("Player entity created at (%.2f, %.2f, %.2f)\n", player_x, player_y, player_z);
    } else {
        engine->player = NULL;
    }

    printf("Entity system initialized\n");
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
    // window_set_cursor_mode(window, GLFW_CURSOR_DISABLED);
    
    // Setup OpenGL state
    renderer_set_opengl_state();
    
    // Setup game world
    engine_setup_world(engine);

    // Setup GUI
    engine_setup_gui(engine, window);

    // Setup entity system
    engine_setup_entities(engine);
    
    // Initialize camera to follow player if player exists
    if (engine->player) {
        camera_follow_target(engine->camera,
            engine->player->position[0],
            engine->player->position[1],
            engine->player->position[2]);
    }

    return engine;
}

void engine_run(Engine* engine) {
    Camera* camera = engine->camera;
    
    while (!window_should_close(engine->window)) {
        double current_time = glfwGetTime();
        float dt = (float)(current_time - engine->last_time);

        // Update Engine Time
        engine->last_time = current_time;
        
        // Update Engine mouse position
        engine->mouse_x = last_mouse_x;
        engine->mouse_y = last_mouse_y;
        
#ifdef DEBUG_MODE
        // Update Debug Elements
        fps_counter_update(engine->gui_debug_elements, current_time);
        engine->gui_debug_elements->camera_pos_x = camera->pos_x;
        engine->gui_debug_elements->camera_pos_y = camera->pos_y;
        engine->gui_debug_elements->camera_pos_z = camera->pos_z;
        
        engine->gui_debug_elements->mouse_pos_x = last_mouse_x;
        engine->gui_debug_elements->mouse_pos_y = last_mouse_y;
        
        engine->gui_debug_elements->player_pos_x = engine->player->position[0];
        engine->gui_debug_elements->player_pos_y = engine->player->position[1];
        engine->gui_debug_elements->player_pos_z = engine->player->position[2];
#endif

        // Process input
        // Handle escape key to close window
        if (glfwGetKey(engine->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(engine->window, true);

        // Process mouse for camera rotation (third-person orbit)
        // camera_process_mouse(camera, engine->mouse_x, engine->mouse_y);

        
        // Clear
        renderer_clear();
        
        // Update camera matrices
        float view_matrix[16], proj_matrix[16];
        camera_update_view(camera, view_matrix);
        
        int width, height;
        window_get_size(engine->window, &width, &height);
        camera_update_projection(camera, width, height, proj_matrix);
        
        
#ifdef DEBUG_MODE
        camera->yaw = engine->gui_debug_elements->camera_yaw;
        camera->pos_x = engine->gui_debug_elements->cam_pos_x;
        camera->pos_y = engine->gui_debug_elements->cam_pos_y;
        camera->pos_z = engine->gui_debug_elements->cam_pos_z;
        camera->pitch = engine->gui_debug_elements->cam_pitch;
        engine->player->rotation[0] = engine->gui_debug_elements->player_rotation_x;
        engine->player->rotation[1] = engine->gui_debug_elements->player_rotation_y;
        engine->player->rotation[2] = engine->gui_debug_elements->player_rotation_z;
#endif   

        // Process player movement input
        if (engine->player) {
            player_process_input(engine->player, engine->window, dt);

            // Camera follows player
            camera_follow_target(camera,
                engine->player->position[0],
                engine->player->position[1],
                engine->player->position[2]);
        }
        
        // Update terrain - generate new chunks as camera moves (infinite terrain)
        // Pass raw camera world position - the update function calculates chunk positions
        terrain_lod_manager_update(engine->terrain, engine->seed, camera->pos_x, camera->pos_z);

       // Update entities
        if (engine->entity_manager) {
            entity_manager_update(engine->entity_manager, dt);
        }
 
        // Render skybox first
        state_restore_defaults();
        skybox_render(engine->skybox, proj_matrix, view_matrix);
        
        // Restore OpenGL state for terrain
        state_restore_defaults();
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // Render terrain
        terrain_lod_manager_render(engine->terrain, view_matrix, proj_matrix,
                                   camera->pos_x, camera->pos_y, camera->pos_z,
                                   (float)current_time);

        // Render entities (opaque objects)
        if (engine->entity_manager) {
            state_restore_defaults();
            entity_manager_render(engine->entity_manager, view_matrix, proj_matrix,
                                camera->pos_x, camera->pos_y, camera->pos_z);
        }

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
#ifdef DEBUG_MODE      
        window_get_size(engine->window, &width, &height);
        gui_render_debug_elements(&engine->nk_glfw.ctx, engine->gui_debug_elements, width, height);
        
        nk_glfw3_render(&engine->nk_glfw, NK_ANTI_ALIASING_ON,
                       MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
#endif
        state_restore_defaults();
        
        glfwSwapBuffers(engine->window);
        glfwPollEvents();
    }
}

void engine_cleanup(Engine* engine) {
    printf("\nCleaning up...\n");

    // Cleanup entities
    if (engine->entity_manager) {
        entity_manager_cleanup(engine->entity_manager);
    }

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
