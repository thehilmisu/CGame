#include "player.h"
#include <math.h>
#include "../math/math_ops.h"
#include <stdio.h>


void player_process_input(Entity* player, GLFWwindow* window, float dt) {
    if (!player) return;

    float speed = PLAYER_SPEED * dt;

    // Calculate forward and right vectors based on camera yaw (not pitch)
    float forward_x = cosf(0);
    float right_z = sinf(0 - M_PI / 2.0f);

    float move_x = 0.0f;
    float move_z = 0.0f;

    float rotation_speed = 3.0f;

    float target_rotation = 0.0f;
    float target_rotation_x = 0.0f;
    // Always move to -Z direction
    // move_z -= right_z;
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        move_z -= right_z;
        target_rotation_x = 0.5f;
        player->position[1] -= speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        move_z += right_z;
        target_rotation_x = -0.5f;
        player->position[1] += speed;
    }


    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        move_x += forward_x;
        target_rotation = -1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        move_x -= forward_x;
        target_rotation = 1.0f;
    }

    // Smoothly interpolate current rotation towards target rotation
    player->rotation[2] = lerp(player->rotation[2], target_rotation, rotation_speed * dt);
    player->rotation[0] = lerp(player->rotation[0], target_rotation_x,  dt);

    // Normalize movement vector if moving diagonally
    float move_length = sqrtf(move_x * move_x + move_z * move_z);
    if (move_length > 0.0f) {
        move_x /= move_length;
        move_z /= move_length;

        // Update player position
        player->position[0] += move_x * speed;
        player->position[2] += move_z * speed;
    }
}

