#include "shader.h"
#include <stdio.h>
#include <string.h>

static void check_compile_errors(GLuint shader, const char* type) {
    GLint success;
    char info_log[512];

    if (strcmp(type, "VERTEX") == 0) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, info_log);
            fprintf(stderr, "Vertex shader compilation failed:\n%s\n", info_log);
        }
    } else {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, info_log);
            fprintf(stderr, "Fragment shader compilation failed:\n%s\n", info_log);
        }
    }
}

static void check_link_errors(GLuint program) {
    GLint success;
    char info_log[512];

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, info_log);
        fprintf(stderr, "Shader program linking failed:\n%s\n", info_log);
    }
}

GLuint shader_compile(const char* vertex_src, const char* fragment_src) {
    // Compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_src, NULL);
    glCompileShader(vertex_shader);
    check_compile_errors(vertex_shader, "VERTEX");

    // Compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_src, NULL);
    glCompileShader(fragment_shader);
    check_compile_errors(fragment_shader, "FRAGMENT");

    // Link program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    check_link_errors(program);

    // Cleanup
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

Shader shader_create(const char* vertex_src, const char* fragment_src) {
    GLuint program = shader_compile(vertex_src, fragment_src);
    
    Shader shader;
    shader.program = program;
    strncpy(shader.name, "shader", sizeof(shader.name) - 1);

    return shader;
}

void shader_destroy(Shader* shader) {
    glDeleteProgram(shader->program);
}

void shader_use(Shader* shader) {
    glUseProgram(shader->program);
}

void shader_set_int(Shader* shader, const char* name, int value) {
    glUniform1i(glGetUniformLocation(shader->program, name), value);
}

void shader_set_float(Shader* shader, const char* name, float value) {
    glUniform1f(glGetUniformLocation(shader->program, name), value);
}

void shader_set_vec3(Shader* shader, const char* name, float x, float y, float z) {
    glUniform3f(glGetUniformLocation(shader->program, name), x, y, z);
}

void shader_set_mat4(Shader* shader, const char* name, float* matrix) {
    glUniformMatrix4fv(glGetUniformLocation(shader->program, name), 1, GL_FALSE, matrix);
}
