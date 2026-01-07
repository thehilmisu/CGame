#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>

// Initialize OpenGL state (depth test, culling, etc.)
void renderer_set_opengl_state(void);

// Clear the framebuffer
void renderer_clear(void);

#endif // RENDERER_H
