#include "player.h"
#include <math.h>

void player_process_input(Entity* player, GLFWwindow* window, float dt) {
    if (!player) return;

    float speed = PLAYER_SPEED * dt;

    // Calculate forward and right vectors based on camera yaw (not pitch)
    float forward_x = cosf(0);
    float right_z = sinf(0 - M_PI / 2.0f);

    float move_x = 0.0f;
    float move_z = 0.0f;


    // WASD movement relative to camera direction
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        move_z -= right_z;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        move_z += right_z;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        move_x += forward_x;
        player->rotation[2] -= 0.01f;
        if(player->rotation[2] <=-1.0f) player->rotation[2] = -1.0f;
    }
    else if(glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE){
         player->rotation[2] += 0.01f;
         if(player->rotation[2] >=0.0f) player->rotation[2] = 0.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        move_x -= forward_x;
    }

    // Normalize movement vector if moving diagonally
    float move_length = sqrtf(move_x * move_x + move_z * move_z);
    if (move_length > 0.0f) {
        move_x /= move_length;
        move_z /= move_length;

        // Update player position
        player->position[0] += move_x * speed;
        player->position[2] += move_z * speed;
    }

    // Player rotation matches camera yaw (always faces forward relative to camera)
    // player->rotation[1] = camera_yaw;

    // Vertical movement (for flying, can be removed for ground-based movement)
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        player->position[1] += speed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        player->position[1] -= speed;
    }
}

void player_get_forward(Entity* player, float* out_x, float* out_z) {
    if (!player) {
        *out_x = 0.0f;
        *out_z = 0.0f;
        return;
    }

    float yaw_rad = player->rotation[1] * M_PI / 180.0f;
    *out_x = cosf(yaw_rad);
    *out_z = sinf(yaw_rad);
}
