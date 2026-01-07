#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
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

#include "terrain.h"
#include "file_ops.h"
#include "texture.h"
#include "stb_image/stb_image.h"
#include "gui.h"

// Window settings
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

// Nuklear buffer sizes
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

// Camera
typedef struct {
    float pos_x, pos_y, pos_z;
    float yaw, pitch;
    float fov;
} Camera;

Camera camera = {0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 75.0f};  // Start at water level looking forward
float camera_speed = 100.0f;  // Faster movement

// Mouse
double last_mouse_x = WINDOW_WIDTH / 2.0;
double last_mouse_y = WINDOW_HEIGHT / 2.0;
bool first_mouse = true;

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;
    
    if (first_mouse) {
        last_mouse_x = xpos;
        last_mouse_y = ypos;
        first_mouse = false;
        return;
    }
    
    float xoffset = (float)(xpos - last_mouse_x);
    float yoffset = (float)(last_mouse_y - ypos);
    last_mouse_x = xpos;
    last_mouse_y = ypos;
    
    float sensitivity = 0.05f;  // Reduced for smoother control
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    
    camera.yaw += xoffset;
    camera.pitch += yoffset;
    
    // Clamp pitch
    if (camera.pitch > 89.0f) camera.pitch = 89.0f;
    if (camera.pitch < -89.0f) camera.pitch = -89.0f;
}

// Matrix helpers (column-major like OpenGL)
void mat4_identity(float* m) {
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void mat4_multiply(float* result, const float* a, const float* b) {
    float temp[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[j * 4 + i] = 
                a[0 * 4 + i] * b[j * 4 + 0] +
                a[1 * 4 + i] * b[j * 4 + 1] +
                a[2 * 4 + i] * b[j * 4 + 2] +
                a[3 * 4 + i] * b[j * 4 + 3];
        }
    }
    memcpy(result, temp, 16 * sizeof(float));
}

void mat4_rotate_x(float* m, float angle) {
    mat4_identity(m);
    float c = cosf(angle);
    float s = sinf(angle);
    m[5] = c;
    m[6] = s;
    m[9] = -s;
    m[10] = c;
}

void mat4_rotate_y(float* m, float angle) {
    mat4_identity(m);
    float c = cosf(angle);
    float s = sinf(angle);
    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;
}

void mat4_translate(float* m, float x, float y, float z) {
    mat4_identity(m);
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

void process_input(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Press R to reset to water level
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camera.pos_y = 10.0f;
        printf("Reset to water level: y=%.1f\n", camera.pos_y);
    }
    
    float speed = camera_speed * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        speed *= 5.0f;  // Faster sprint
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
        speed *= 0.2f;  // Slower
    
    float yaw_rad = camera.yaw * M_PI / 180.0f;
    float pitch_rad = camera.pitch * M_PI / 180.0f;
    
    // Calculate direction vectors
    float front_x = cosf(pitch_rad) * cosf(yaw_rad);
    float front_y = sinf(pitch_rad);
    float front_z = cosf(pitch_rad) * sinf(yaw_rad);
    
    float right_x = cosf(yaw_rad - M_PI / 2.0f);
    float right_z = sinf(yaw_rad - M_PI / 2.0f);
    
    // WASD movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.pos_x += front_x * speed;
        camera.pos_y += front_y * speed;
        camera.pos_z += front_z * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.pos_x -= front_x * speed;
        camera.pos_y -= front_y * speed;
        camera.pos_z -= front_z * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.pos_x -= right_x * speed;
        camera.pos_z -= right_z * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.pos_x += right_x * speed;
        camera.pos_z += right_z * speed;
    }
    
    // Up/Down movement
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.pos_y += speed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.pos_y -= speed;
    }
}


int main(void) {
    
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 
                                          "Terrain Demo - OpenGL", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Load OpenGL functions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }
    
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    // OpenGL settings (like original)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);  // Sky blue

    // Ensure we're in FILL mode (not LINE mode which would show wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Enable smooth shading (interpolate between vertices)
    glShadeModel(GL_SMOOTH);

    // Enable polygon offset to help with potential z-fighting at chunk boundaries
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.5f, 0.5f);
    
    // Create terrain seed (use same approach as original - random seed for variety)
    srand(time(NULL));
    int random_seed = rand();
    printf("Using terrain seed: %d\n", random_seed);
    TerrainSeed seed = terrain_seed_create(random_seed);
    
    printf("\nInitializing terrain with %d LOD levels...\n", MAX_LOD);
    
    // Original terrain shader (ported from terrainvert.glsl and terrainfrag.glsl)
    const char* vertex_shader_src = load_shader_source("assets/shaders/terrainvert.glsl");
    // EXACTLY like original terrainfrag.glsl - with texture atlas and smooth blending
    const char* fragment_shader_src = load_shader_source("assets/shaders/terrainfrag.glsl");
    GLuint terrain_shader = shader_compile(vertex_shader_src, fragment_shader_src);
    
    // Load terrain texture
    GLuint terrain_texture = texture_load("assets/textures/terraintextures.png");
    if (terrain_texture == 0) {
        fprintf(stderr, "Warning: Failed to load terrain texture, using fallback colors\n");
    }
    
    // Create LOD manager (like original)
    TerrainLODManagerGL terrain_lod = terrain_lod_manager_create(&seed);
    terrain_lod.terrain_shader = terrain_shader;
    terrain_lod.terrain_texture = terrain_texture;
    
    // Generate all chunks for all LOD levels
    printf("Generating terrain chunks...\n");
    terrain_lod_manager_generate_all(&terrain_lod, &seed, 0, 0);
    printf("Terrain initialized\n");
    
    // Create water manager (like original)
    printf("Initializing water...\n");
    WaterManagerGL water = water_manager_init();
    printf("Water initialized\n");
    
    // Create skybox (like original)
    printf("Initializing skybox...\n");
    SkyboxGL skybox = skybox_init();

    // Initialize Nuklear (use NK_GLFW3_DEFAULT to not override our mouse callbacks)
    struct nk_glfw glfw = {0};
    nk_glfw3_init(&glfw, window, NK_GLFW3_DEFAULT);
    
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&glfw, &atlas);
    nk_glfw3_font_stash_end(&glfw);
    
    FPSCounter fps_counter;
    fps_counter_init(&fps_counter);
    
    
    printf("Controls:\n");
    printf("  WASD - Move\n");
    printf("  Mouse - Look\n");
    printf("  Space - Up\n");
    printf("  Shift - Down\n");
    printf("  Ctrl - Fast (5x)\n");
    printf("  Alt - Slow (0.2x)\n");
    printf("  R - Reset to water level\n");
    printf("  ESC - Exit\n\n");
    
    // Main loop
    double last_time = glfwGetTime();
    
    while (!glfwWindowShouldClose(window)) {
        double current_time = glfwGetTime();
        float dt = (float)(current_time - last_time);
        last_time = current_time;
        fps_counter_update(&fps_counter,current_time);
        
        process_input(window, dt);
        
        // Clear
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // View matrix (exactly like original Camera::viewMatrix())
        // view = rotate(pitch, X) * rotate(yaw, Y) * translate(-pos)
        float yaw_rad = camera.yaw * M_PI / 180.0f;
        float pitch_rad = camera.pitch * M_PI / 180.0f;
        
        float pitch_mat[16], yaw_mat[16], trans_mat[16];
        float view[16];
        
        mat4_rotate_x(pitch_mat, pitch_rad);
        mat4_rotate_y(yaw_mat, yaw_rad);
        mat4_translate(trans_mat, -camera.pos_x, -camera.pos_y, -camera.pos_z);
        
        // view = pitch * yaw * translate
        mat4_multiply(view, pitch_mat, yaw_mat);
        mat4_multiply(view, view, trans_mat);
        
        // Projection matrix (perspective)
        float projection[16];
        memset(projection, 0, sizeof(projection));
        float fov = camera.fov * M_PI / 180.0f;
        float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
        float near = 0.1f;
        float far = 10000.0f;
        float f = 1.0f / tanf(fov / 2.0f);
        projection[0] = f / aspect;
        projection[5] = f;
        projection[10] = (far + near) / (near - far);
        projection[11] = -1.0f;
        projection[14] = (2.0f * far * near) / (near - far);
        
        // Update terrain LOD based on camera position
        // Match original chunktable.cpp lines 165-166:
        // ix = floor((cameraz + chunksz*SCALE) / (chunksz*SCALE*2))
        // iz = floor((camerax + chunksz*SCALE) / (chunksz*SCALE*2))
        // Note: original uses cameraz for ix (x-chunk) and camerax for iz (z-chunk)
        float chunksz = CHUNK_SZ * (float)PREC / (float)(PREC + 1);
        int cam_chunk_x = (int)floorf((camera.pos_z + chunksz * SCALE) / (chunksz * SCALE * 2.0f));
        int cam_chunk_z = (int)floorf((camera.pos_x + chunksz * SCALE) / (chunksz * SCALE * 2.0f));
        terrain_lod_manager_update(&terrain_lod, &seed, cam_chunk_x, cam_chunk_z);
        
        // Render skybox first (like original display.cpp - skybox before terrain)
        skybox_render(&skybox, projection, view);
        
        // Render terrain (all LOD levels)
        terrain_lod_manager_render(&terrain_lod, view, projection, 
                                   camera.pos_x, camera.pos_y, camera.pos_z, 
                                   (float)current_time);
        
        // Draw water (with blending for transparency)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);  // Don't write to depth buffer
        glDisable(GL_CULL_FACE);  // Ensure both sides visible
        
        water_render_gl(&water, projection, view, 
                       camera.pos_x, camera.pos_y, camera.pos_z, 
                       (float)current_time);

        // Restore state after water rendering
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        
        // Render Nuklear GUI
        nk_glfw3_new_frame(&glfw);
        
        // Render FPS display
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        gui_render_fps(&glfw.ctx, &fps_counter, width, height);
        
        // Render Nuklear
        nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, 
                    MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
        
        // Restore OpenGL state after Nuklear (it changes many states)
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    printf("\nCleaning up...\n");
    terrain_lod_manager_cleanup(&terrain_lod);
    water_cleanup_gl(&water);
    skybox_cleanup(&skybox);
    nk_glfw3_shutdown(&glfw);
    
    glfwTerminate();
    printf("Done!\n");
    return 0;
}
