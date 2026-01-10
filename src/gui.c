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
    const float height = 1000.0f;
    const float padding = 10.0f;
    
    struct nk_rect bounds = nk_rect(
        padding,
        padding,
        width,
        height
    );
    struct nk_style *s = &ctx->style;
    // nk_style_push_color(ctx, &s->window.background, nk_rgba(0,0,0,150)); // Fully transparent background
    nk_style_push_style_item(ctx, &s->window.fixed_background, nk_style_item_color(nk_rgba(0,0,0,150))); // Transparent fixed background

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
        
        // Display camera position
        char player_position_text[64];
        snprintf(player_position_text, sizeof(player_position_text), "Player Position: %.2f, %.2f, %.2f", elements->player_pos_x, elements->player_pos_y, elements->player_pos_z);
        nk_label(ctx, player_position_text, NK_TEXT_LEFT);
        
        // Display camera position
        char mouse_position_text[64];
        snprintf(mouse_position_text, sizeof(mouse_position_text), "Mouse Position: %.2f, %.2f", elements->mouse_pos_x, elements->mouse_pos_y);
        nk_label(ctx, mouse_position_text, NK_TEXT_LEFT);

        //Camera Yaw slider
        // nk_spacing(ctx, 1);
        char temp_text[64];
        snprintf(temp_text, sizeof(temp_text), "Camera Yaw: %.2f", elements->camera_yaw);
        nk_label(ctx, temp_text, NK_TEXT_LEFT);
        nk_slider_float(ctx, -100.0f, &elements->camera_yaw, 100.0f, 1.0f);
        
        snprintf(temp_text, sizeof(temp_text), "Camera Pitch: %.2f", elements->cam_pitch);
        nk_label(ctx, temp_text, NK_TEXT_LEFT);
        nk_slider_float(ctx, -100.0f, &elements->cam_pitch, 100.0f, 1.0f);

        // Player Rotation Sliders
        // nk_spacing(ctx, 1);
        snprintf(temp_text, sizeof(temp_text), "Player Rotation X: %.2f", elements->player_rotation_x);
        nk_label(ctx, temp_text, NK_TEXT_LEFT);
        nk_slider_float(ctx, -1.0f, &elements->player_rotation_x, 1.0f, 0.1f);

        
        // nk_spacing(ctx, 1);
        snprintf(temp_text, sizeof(temp_text), "Player Rotation Y: %.2f", elements->player_rotation_y);
        nk_label(ctx, temp_text, NK_TEXT_LEFT);
        nk_slider_float(ctx, -1.0f, &elements->player_rotation_y, 1.0f, 0.1f);

        // nk_spacing(ctx, 1);
        snprintf(temp_text, sizeof(temp_text), "Player Rotation Z: %.2f", elements->player_rotation_z);
        nk_label(ctx, temp_text, NK_TEXT_LEFT);
        nk_slider_float(ctx, -1.0f, &elements->player_rotation_z, 1.0f, 0.1f);


        // printf("%s --- %s\n", camera_position_text, player_position_text);
    }

    // nk_style_pop_color(ctx);
    nk_style_pop_style_item(ctx);
    nk_end(ctx);
}
