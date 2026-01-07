#include "state.h"

void state_restore_defaults(void) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void state_enable_blend(void) {
    glEnable(GL_BLEND);
}

void state_disable_blend(void) {
    glDisable(GL_BLEND);
}

void state_set_blend_func(GLenum src, GLenum dst) {
    glBlendFunc(src, dst);
}

void state_set_depth_mask(GLboolean flag) {
    glDepthMask(flag);
}

void state_set_depth_func(GLenum func) {
    glDepthFunc(func);
}

void state_enable_cull_face(void) {
    glEnable(GL_CULL_FACE);
}

void state_disable_cull_face(void) {
    glDisable(GL_CULL_FACE);
}
