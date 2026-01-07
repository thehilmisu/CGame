#ifndef GUI_H
#define GUI_H

#include <nuklear/nuklear.h>

// FPS counter structure
typedef struct {
    float fps;
    float frame_time;
    int frame_count;
    double last_time;
} FPSCounter;

// Initialize the FPS counter
void fps_counter_init(FPSCounter* counter);

// Update FPS counter (call this every frame with current time)
void fps_counter_update(FPSCounter* counter, double current_time);

// Render the FPS display using Nuklear
void gui_render_fps(struct nk_context* ctx, FPSCounter* counter, int window_width, int window_height);

#endif // GUI_H
