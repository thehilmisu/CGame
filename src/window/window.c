#include "window.h"
#include <stdio.h>

GLFWwindow* window_create(int width, int height, const char* title) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window) {
        return NULL;
    }

    glfwMakeContextCurrent(window);
    return window;
}

void window_set_callbacks(GLFWwindow* window, 
                          GLFWframebuffersizefun framebuffer_callback,
                          GLFWcursorposfun cursor_callback) {
    if (framebuffer_callback) {
        glfwSetFramebufferSizeCallback(window, framebuffer_callback);
    }
    if (cursor_callback) {
        glfwSetCursorPosCallback(window, cursor_callback);
    }
}

void window_set_cursor_mode(GLFWwindow* window, int mode) {
    glfwSetInputMode(window, GLFW_CURSOR, mode);
}

void window_get_size(GLFWwindow* window, int* width, int* height) {
    glfwGetWindowSize(window, width, height);
}

int window_should_close(GLFWwindow* window) {
    return glfwWindowShouldClose(window);
}
