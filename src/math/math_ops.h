#ifndef MATHOPS_H
#define MATHOPS_H

#include <string.h>


// Matrix operations (column-major like OpenGL)
// Matrices are 16-element float arrays

// Set matrix to identity
void mat4_identity(float* m);

// Multiply two matrices: result = a * b
void mat4_multiply(float* result, const float* a, const float* b);

// Create rotation matrix around X axis
void mat4_rotate_x(float* m, float angle);

// Create rotation matrix around Y axis
void mat4_rotate_y(float* m, float angle);

// Create rotation matrix around Z axis
void mat4_rotate_z(float* m, float angle);

// Create translation matrix
void mat4_translate(float* m, float x, float y, float z);

// Create scale matrix
void mat4_scale(float* m, float sx, float sy, float sz);

// Create perspective projection matrix
void mat4_perspective(float* m, float fov_degrees, float aspect, float near, float far);

// Standard lerp function
float lerp(float start, float end, float t);

#endif // MATHOPS_H
