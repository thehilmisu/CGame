#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include <GLFW/glfw3.h>

// Player movement configuration
#define PLAYER_SPEED 50.0f
#define PLAYER_SPRINT_MULTIPLIER 2.5f
#define PLAYER_TURN_SPEED 120.0f  // degrees per second

// Process player input and update entity position/rotation
void player_process_input(Entity* player, GLFWwindow* window, float dt, float camera_yaw);

// Get player forward direction based on rotation
void player_get_forward(Entity* player, float* out_x, float* out_z);

#endif // PLAYER_H
