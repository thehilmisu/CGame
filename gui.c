#include "gui.h"
#include <stdio.h>

void fps_counter_init(FPSCounter* counter) {
    counter->fps = 0.0f;
    counter->frame_time = 0.0f;
    counter->frame_count = 0;
    counter->last_time = 0.0;
}

void fps_counter_update(FPSCounter* counter, double current_time) {
    counter->frame_count++;
    
    if (counter->last_time == 0.0) {
        counter->last_time = current_time;
        return;
    }
    
    double delta = current_time - counter->last_time;
    
    // Update FPS every 0.5 seconds for smoother reading
    if (delta >= 0.5) {
        counter->fps = (float)(counter->frame_count / delta);
        counter->frame_time = (float)((delta / counter->frame_count) * 1000.0); // Convert to ms
        counter->frame_count = 0;
        counter->last_time = current_time;
    }
}

void gui_render_fps(struct nk_context* ctx, FPSCounter* counter, int window_width, int window_height) {
    (void)window_height; // Unused parameter
    (void)window_width;
    
    // Create a small window in the top-left corner
    const float width = 150.0f;
    const float height = 80.0f;
    const float padding = 10.0f;
    
    struct nk_rect bounds = nk_rect(
        padding,
        padding,
        width,
        height
    );
    
    // Create window with minimal decorations
    if (nk_begin(ctx, "FPS Counter", bounds,
                 NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
        
        nk_layout_row_dynamic(ctx, 25, 1);
        
        // Display FPS
        char fps_text[64];
        snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", counter->fps);
        nk_label(ctx, fps_text, NK_TEXT_LEFT);
        
        // Display frame time
        char frame_time_text[64];
        snprintf(frame_time_text, sizeof(frame_time_text), "Frame: %.2f ms", counter->frame_time);
        nk_label(ctx, frame_time_text, NK_TEXT_LEFT);
    }
    nk_end(ctx);
}
