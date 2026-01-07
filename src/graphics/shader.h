#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

// Compile and link shader program from source strings
GLuint shader_compile(const char* vertex_src, const char* fragment_src);

// Shader wrapper structure (for future use)
typedef struct {
    GLuint program;
    char name[64];
} Shader;

Shader shader_create(const char* vertex_src, const char* fragment_src);
void shader_destroy(Shader* shader);
void shader_use(Shader* shader);
void shader_set_int(Shader* shader, const char* name, int value);
void shader_set_float(Shader* shader, const char* name, float value);
void shader_set_vec3(Shader* shader, const char* name, float x, float y, float z);
void shader_set_mat4(Shader* shader, const char* name, float* matrix);

#endif // SHADER_H
