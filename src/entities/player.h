#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include <GLFW/glfw3.h>

// Player movement configuration
#define PLAYER_SPEED 30.0f
#define PLAYER_SPRINT_MULTIPLIER 2.5f
#define PLAYER_TURN_SPEED 120.0f  // degrees per second

// Process player input and update entity position/rotation
void player_process_input(Entity* player, GLFWwindow* window, float dt);


#endif // PLAYER_H
