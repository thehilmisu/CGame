#ifndef CAMERA_H
#define CAMERA_H


#include <GLFW/glfw3.h>

typedef struct {
    float pos_x, pos_y, pos_z;
    float yaw, pitch;
    float fov;

    // Third-person camera settings
    float distance;           // Distance from target
    float height_offset;      // Height above target
    float target_x, target_y, target_z;  // Look-at target
} Camera;

// Initialize camera with default values
void camera_init(Camera* camera);

// Capturing mouse movements
void camera_process_mouse(Camera* camera, double x, double y);

// Update view matrix based on camera position and rotation
void camera_update_view(Camera* camera, float* view_matrix);

// Update projection matrix
void camera_update_projection(Camera* camera, int window_width, int window_height, float* proj_matrix);

// Third-person camera: follow a target position
void camera_follow_target(Camera* camera, float target_x, float target_y, float target_z);

#endif // CAMERA_H
