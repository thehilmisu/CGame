#ifndef STATE_H
#define STATE_H

#include <glad/glad.h>

// Restore default OpenGL state (after GUI rendering, etc.)
void state_restore_defaults(void);

// Blend state
void state_enable_blend(void);
void state_disable_blend(void);
void state_set_blend_func(GLenum src, GLenum dst);

// Depth state
void state_set_depth_mask(GLboolean flag);
void state_set_depth_func(GLenum func);

// Cull face state
void state_enable_cull_face(void);
void state_disable_cull_face(void);

#endif // STATE_H
