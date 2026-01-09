#ifndef CAMERA_H
#define CAMERA_H


#include <GLFW/glfw3.h>

typedef struct {
    float pos_x, pos_y, pos_z;
    float yaw, pitch;
    float fov;
} Camera;

// Initialize camera with default values
void camera_init(Camera* camera);

// Process keyboard input for camera movement
void camera_process_input(Camera* camera, GLFWwindow* window, float dt);

void camera_process_mouse(Camera* camera, double x, double y);

// Update view matrix based on camera position and rotation
void camera_update_view(Camera* camera, float* view_matrix);

// Update projection matrix
void camera_update_projection(Camera* camera, int window_width, int window_height, float* proj_matrix);

#endif // CAMERA_H
