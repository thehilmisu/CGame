#include "water.h"
#include "../file_ops.h"
#include "../graphics/shader.h"
#include "../graphics/texture.h"
#include "../config.h"

WaterManagerGL water_manager_init(void) {
    WaterManagerGL water;
    water.range = 4;  // Like original waterrange
    water.scale = CHUNK_SZ * 32.0f * SCALE;  // Like original quadscale
    water.texture = 0;  // Initialize texture to 0

    // Create quad mesh (slightly larger to create overlap between instances)
    float quad_vertices[] = {
        // pos (x, y, z, w)          normal (x, y, z)
        -1.05f, 0.0f, -1.05f, 1.0f,   0.0f, 1.0f, 0.0f,
         1.05f, 0.0f, -1.05f, 1.0f,   0.0f, 1.0f, 0.0f,
         1.05f, 0.0f,  1.05f, 1.0f,   0.0f, 1.0f, 0.0f,
        -1.05f, 0.0f,  1.05f, 1.0f,   0.0f, 1.0f, 0.0f,
    };

    unsigned int quad_indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &water.vao);
    glGenBuffers(1, &water.vbo);
    glGenBuffers(1, &water.ibo);

    glBindVertexArray(water.vao);

    glBindBuffer(GL_ARRAY_BUFFER, water.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, water.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

    // Load water shader (simplified version - watersimplefrag.glsl)
    const char* water_vert = load_shader_source("assets/shaders/instancedvert.glsl");
    const char* water_frag = load_shader_source("assets/shaders/waterfrag.glsl"); 
    water.shader = shader_compile(water_vert, water_frag);
    
    // Load watermaps texture for normal mapping
    water.texture = texture_load("assets/textures/watermaps.png");
    if (water.texture == 0) {
        fprintf(stderr, "Warning: Failed to load watermaps texture\n");
    }
    
    return water;
}

void water_render_gl(WaterManagerGL* water, float* persp, float* view, 
                     float camera_x, float camera_y, float camera_z, float time) {
    glUseProgram(water->shader);

    // Unbind any textures from entity rendering to prevent them from being used by water shader
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind watermaps texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, water->texture);
    glUniform1i(glGetUniformLocation(water->shader, "watermaps"), 0);

    // Uniforms
    glUniformMatrix4fv(glGetUniformLocation(water->shader, "persp"), 1, GL_FALSE, persp);
    glUniformMatrix4fv(glGetUniformLocation(water->shader, "view"), 1, GL_FALSE, view);
    
    // Like original display.cpp line 67: water follows camera position directly
    // transform = translate(camera.x, 0, camera.z) * scale(quadscale)
    float transform[16] = {
        water->scale, 0, 0, 0,
        0, water->scale, 0, 0,
        0, 0, water->scale, 0,
        camera_x, 0, camera_z, 1
    };
    glUniformMatrix4fv(glGetUniformLocation(water->shader, "transform"), 1, GL_FALSE, transform);
    
    glUniform1i(glGetUniformLocation(water->shader, "range"), water->range);
    glUniform1f(glGetUniformLocation(water->shader, "scale"), water->scale);
    glUniform3f(glGetUniformLocation(water->shader, "lightdir"), -0.57735f, -0.57735f, -0.57735f);
    glUniform3f(glGetUniformLocation(water->shader, "camerapos"), camera_x, camera_y, camera_z);
    glUniform1f(glGetUniformLocation(water->shader, "viewdist"), 10000.0f);
    glUniform1f(glGetUniformLocation(water->shader, "time"), time);

    // Draw instanced
    glBindVertexArray(water->vao);
    int instance_count = (water->range * 2 + 1) * (water->range * 2 + 1);
    // Add polygon offset for water to reduce potential seam visibility
    glPolygonOffset(-1.0f, -1.0f);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, instance_count);
    glPolygonOffset(0.5f, 0.5f);  // Reset for next render
}

void water_cleanup_gl(WaterManagerGL* water) {
    glDeleteVertexArrays(1, &water->vao);
    glDeleteBuffers(1, &water->vbo);
    glDeleteBuffers(1, &water->ibo);
    glDeleteProgram(water->shader);
    if (water->texture > 0) {
        glDeleteTextures(1, &water->texture);
    }
}
