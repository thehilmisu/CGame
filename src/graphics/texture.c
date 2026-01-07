#include "texture.h"
#include <stb_image/stb_image.h>
#include <stdio.h>

GLuint texture_load(const char* path) {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    
    if (!data) {
        fprintf(stderr, "Failed to load texture: %s\n", path);
        glDeleteTextures(1, &texture_id);
        return 0;
    }
    
    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;
    
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(data);
    
    return texture_id;
}
