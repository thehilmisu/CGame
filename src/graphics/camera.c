#include "camera.h"
#include "../math/math_ops.h"
#include "../config.h"
#include <math.h>
#include <stdbool.h>
#include <GLFW/glfw3.h>

void camera_init(Camera* camera) {
    camera->pos_x = 0.0f;
    camera->pos_y = CAMERA_INITIAL_Y;
    camera->pos_z = 0.0f;
    camera->yaw = 0.0f;
    camera->pitch = 12.0f;
    camera->fov = CAMERA_FOV;

    // Third-person camera settings
    camera->distance = 25.0f;      // Distance behind player
    camera->height_offset = 15.0f;   // Height above player
    camera->target_x = 0.0f;
    camera->target_y = 0.0f;
    camera->target_z = 0.0f;
}

void camera_process_mouse(Camera *camera, double x, double y){

    static double last_mouse_x = 0;
    static double last_mouse_y = 0;
    
    float xoffset = (float)(x - last_mouse_x);
    float yoffset = (float)(last_mouse_y - y);
    last_mouse_x = x;
    last_mouse_y = y;
    
    xoffset *= CAMERA_SENSITIVITY;
    yoffset *= CAMERA_SENSITIVITY;
    
    camera->yaw += xoffset;
    camera->pitch += yoffset;
    
    // Clamp pitch
    if (camera->pitch > 89.0f) camera->pitch = 89.0f;
    if (camera->pitch < -89.0f) camera->pitch = -89.0f;
}

void camera_update_view(Camera* camera, float* view_matrix) {
    // For third-person camera following a target, use look-at matrix
    // Look from camera position towards target position
    float world_up_x = 0.0f, world_up_y = 1.0f, world_up_z = 0.0f;
    
    // Calculate forward vector (from camera to target, normalized)
    float forward_x = camera->target_x - camera->pos_x;
    float forward_y = camera->target_y - camera->pos_y;
    float forward_z = camera->target_z - camera->pos_z;
    
    // Normalize forward vector
    float forward_len = sqrtf(forward_x * forward_x + forward_y * forward_y + forward_z * forward_z);
    if (forward_len < 0.0001f) {
        // If camera and target are at same position, use default forward
        forward_x = 0.0f;
        forward_y = 0.0f;
        forward_z = -1.0f;
    } else {
        forward_x /= forward_len;
        forward_y /= forward_len;
        forward_z /= forward_len;
    }
    
    // Calculate right vector: right = forward × world_up
    float right_x = forward_y * world_up_z - forward_z * world_up_y;
    float right_y = forward_z * world_up_x - forward_x * world_up_z;
    float right_z = forward_x * world_up_y - forward_y * world_up_x;
    
    // Normalize right vector
    float right_len = sqrtf(right_x * right_x + right_y * right_y + right_z * right_z);
    if (right_len < 0.0001f) {
        // If forward is parallel to world_up, use default right
        right_x = 1.0f;
        right_y = 0.0f;
        right_z = 0.0f;
    } else {
        right_x /= right_len;
        right_y /= right_len;
        right_z /= right_len;
    }
    
    // Recalculate up vector: up = right × forward
    float up_x = right_y * forward_z - right_z * forward_y;
    float up_y = right_z * forward_x - right_x * forward_z;
    float up_z = right_x * forward_y - right_y * forward_x;
    
    // Build look-at view matrix (column-major OpenGL format)
    // [Rx  Ux  -Fx  -dot(R,pos)]
    // [Ry  Uy  -Fy  -dot(U,pos)]
    // [Rz  Uz  -Fz  -dot(-F,pos)]
    // [0   0   0    1]
    view_matrix[0] = right_x;
    view_matrix[1] = right_y;
    view_matrix[2] = right_z;
    view_matrix[3] = 0.0f;
    
    view_matrix[4] = up_x;
    view_matrix[5] = up_y;
    view_matrix[6] = up_z;
    view_matrix[7] = 0.0f;
    
    view_matrix[8] = -forward_x;
    view_matrix[9] = -forward_y;
    view_matrix[10] = -forward_z;
    view_matrix[11] = 0.0f;
    
    view_matrix[12] = -(right_x * camera->pos_x + right_y * camera->pos_y + right_z * camera->pos_z);
    view_matrix[13] = -(up_x * camera->pos_x + up_y * camera->pos_y + up_z * camera->pos_z);
    view_matrix[14] = -(-forward_x * camera->pos_x + -forward_y * camera->pos_y + -forward_z * camera->pos_z);
    view_matrix[15] = 1.0f;
}

void camera_update_projection(Camera* camera, int window_width, int window_height, float* proj_matrix) {
    float aspect = (float)window_width / (float)window_height;
    mat4_perspective(proj_matrix, camera->fov, aspect, CAMERA_NEAR, CAMERA_FAR);
}

void camera_follow_target(Camera* camera, float target_x, float target_y, float target_z) {
    // Store target for look-at
    camera->target_x = target_x;
    camera->target_y = target_y;
    camera->target_z = target_z;

    // Calculate camera position behind and above the target
    // Use camera's yaw and pitch to determine offset direction
    float yaw_rad = camera->yaw * M_PI / 180.0f;
    float pitch_rad = camera->pitch * M_PI / 180.0f;

    // Calculate offset: behind the target (opposite of forward direction)
    // Forward direction is (sin(yaw), 0, cos(yaw))
    // Behind is the negative of forward
    float forward_x = sinf(yaw_rad);
    float forward_z = cosf(yaw_rad);
    
    // Camera offset: behind target, adjusted by pitch for height
    float horizontal_distance = cosf(pitch_rad) * camera->distance;
    float offset_x = -forward_x * horizontal_distance;
    float offset_y = camera->height_offset - sinf(pitch_rad) * camera->distance;
    float offset_z = -forward_z * horizontal_distance;

    // Set camera position
    camera->pos_x = target_x + offset_x;
    camera->pos_y = target_y + offset_y;
    camera->pos_z = target_z + offset_z;
}
