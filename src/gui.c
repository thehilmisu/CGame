#include "gui.h"
#include <stdio.h>

DebugElements gui_debug_elements_init() {

    DebugElements elements = {0};

    return elements;
}

void fps_counter_update(DebugElements* elements, double current_time) {
    elements->frame_count++;
    
    if (elements->last_time == 0.0) {
        elements->last_time = current_time;
        return;
    }
    
    double delta = current_time - elements->last_time;
    
    // Update FPS every 0.5 seconds for smoother reading
    if (delta >= 0.5) {
        elements->fps = (float)(elements->frame_count / delta);
        elements->frame_time = (float)((delta / elements->frame_count) * 1000.0); // Convert to ms
        elements->frame_count = 0;
        elements->last_time = current_time;
    }
}

void gui_render_debug_elements(struct nk_context* ctx, DebugElements* elements, int window_width, int window_height) {
    (void)window_height; // Unused parameter
    (void)window_width;
    
    // Create a small window in the top-left corner
    const float width = 350.0f;
    const float height = 110.0f;
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
        snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", elements->fps);
        nk_label(ctx, fps_text, NK_TEXT_LEFT);
        
        // Display frame time
        char frame_time_text[64];
        snprintf(frame_time_text, sizeof(frame_time_text), "Frame: %.2f ms", elements->frame_time);
        nk_label(ctx, frame_time_text, NK_TEXT_LEFT);
        
        // Display camera position
        char camera_position_text[64];
        snprintf(camera_position_text, sizeof(camera_position_text), "Camera Position: %.2f, %.2f, %.2f", elements->camera_pos_x, elements->camera_pos_y, elements->camera_pos_z);
        nk_label(ctx, camera_position_text, NK_TEXT_LEFT);
    }
    nk_end(ctx);
}
