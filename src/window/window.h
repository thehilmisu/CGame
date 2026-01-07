#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>

// Window creation and management
GLFWwindow* window_create(int width, int height, const char* title);

// Set callbacks
void window_set_callbacks(GLFWwindow* window, 
                          GLFWframebuffersizefun framebuffer_callback,
                          GLFWcursorposfun cursor_callback);

// Set cursor mode (GLFW_CURSOR_DISABLED, GLFW_CURSOR_NORMAL, etc.)
void window_set_cursor_mode(GLFWwindow* window, int mode);

// Get window size
void window_get_size(GLFWwindow* window, int* width, int* height);

// Check if window should close
int window_should_close(GLFWwindow* window);

#endif // WINDOW_H
