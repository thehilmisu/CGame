#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GL_SILENCE_DEPRECATION

GLuint texture_load(const char* path);

#endif
