#include "renderer.h"

void renderer_set_opengl_state(void) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);  // Sky blue
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glShadeModel(GL_SMOOTH);
    
    // Enable polygon offset to help with potential z-fighting at chunk boundaries
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.5f, 0.5f);
}

void renderer_clear(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
