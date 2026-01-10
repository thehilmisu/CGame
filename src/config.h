#ifndef CONFIG_H
#define CONFIG_H

// Window settings
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define WINDOW_TITLE "Game"

// Nuklear buffer sizes
#define MAX_VERTEX_BUFFER (512 * 1024)
#define MAX_ELEMENT_BUFFER (128 * 1024)

// Camera settings
#define CAMERA_SPEED 100.0f
#define CAMERA_SENSITIVITY 0.05f
#define CAMERA_FOV 75.0f
#define CAMERA_NEAR 0.1f
#define CAMERA_FAR 10000.0f
#define CAMERA_INITIAL_Y 50.0f

// Input settings
#define INPUT_SPRINT_MULTIPLIER 5.0f
#define INPUT_SLOW_MULTIPLIER 0.2f

#endif // CONFIG_H
