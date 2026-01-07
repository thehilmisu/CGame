#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>

// Load a texture from file path, returns OpenGL texture ID (0 on failure)
GLuint texture_load(const char* path);

#endif // TEXTURE_H
