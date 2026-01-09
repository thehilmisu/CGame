#ifndef GUI_H
#define GUI_H

#include <nuklear/nuklear.h>

typedef struct {
    float fps;
    float frame_time;
    int frame_count;
    float camera_pos_x, camera_pos_y, camera_pos_z;
    double last_time;
} DebugElements;

// Initialize the FPS counter
DebugElements gui_debug_elements_init();

// Update FPS counter (call this every frame with current time)
void fps_counter_update(DebugElements* elements, double current_time);

// Render the FPS display using Nuklear
void gui_render_debug_elements(struct nk_context* ctx, DebugElements* elements, int window_width, int window_height);

#endif // GUI_H
