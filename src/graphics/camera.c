#include "camera.h"
#include "../math/math_ops.h"
#include <math.h>
#include <stdbool.h>

void camera_init(Camera* camera) {
    camera->pos_x = 0.0f;
    camera->pos_y = CAMERA_INITIAL_Y;
    camera->pos_z = 0.0f;
    camera->yaw = 0.0f;
    camera->pitch = 0.0f;
    camera->fov = CAMERA_FOV;
}

void camera_process_input(Camera* camera, GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Press R to reset to water level
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camera->pos_y = CAMERA_INITIAL_Y;
    }
    
    float speed = CAMERA_SPEED * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        speed *= INPUT_SPRINT_MULTIPLIER;
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
        speed *= INPUT_SLOW_MULTIPLIER;
    
    float yaw_rad = camera->yaw * M_PI / 180.0f;
    float pitch_rad = camera->pitch * M_PI / 180.0f;
    
    // Calculate direction vectors
    float front_x = cosf(pitch_rad) * cosf(yaw_rad);
    float front_y = sinf(pitch_rad);
    float front_z = cosf(pitch_rad) * sinf(yaw_rad);
    
    float right_x = cosf(yaw_rad - M_PI / 2.0f);
    float right_z = sinf(yaw_rad - M_PI / 2.0f);
    
    // WASD movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera->pos_x += front_x * speed;
        camera->pos_y += front_y * speed;
        camera->pos_z += front_z * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera->pos_x -= front_x * speed;
        camera->pos_y -= front_y * speed;
        camera->pos_z -= front_z * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera->pos_x -= right_x * speed;
        camera->pos_z -= right_z * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera->pos_x += right_x * speed;
        camera->pos_z += right_z * speed;
    }
    
    // Up/Down movement
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera->pos_y += speed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera->pos_y -= speed;
    }
}

void camera_update_view(Camera* camera, float* view_matrix) {
    // View matrix: view = rotate(pitch, X) * rotate(yaw, Y) * translate(-pos)
    float yaw_rad = camera->yaw * M_PI / 180.0f;
    float pitch_rad = camera->pitch * M_PI / 180.0f;
    
    float pitch_mat[16], yaw_mat[16], trans_mat[16];
    
    mat4_rotate_x(pitch_mat, pitch_rad);
    mat4_rotate_y(yaw_mat, yaw_rad);
    mat4_translate(trans_mat, -camera->pos_x, -camera->pos_y, -camera->pos_z);
    
    // view = pitch * yaw * translate
    mat4_multiply(view_matrix, pitch_mat, yaw_mat);
    mat4_multiply(view_matrix, view_matrix, trans_mat);
}

void camera_update_projection(Camera* camera, int window_width, int window_height, float* proj_matrix) {
    float aspect = (float)window_width / (float)window_height;
    mat4_perspective(proj_matrix, camera->fov, aspect, CAMERA_NEAR, CAMERA_FAR);
}
