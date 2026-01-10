#include "skybox.h"
#include "../file_ops.h"
#include "../graphics/shader.h"
#include <stb_image/stb_image.h>
#include <stdio.h>

// ============================================================================
// Skybox Implementation (like original display.cpp displaySkybox)
// ============================================================================

SkyboxGL skybox_init(void) {
    SkyboxGL skybox = {0};

    // Cube vertices (8 corners of a cube, like original gfx.cpp createCubeVao)
    const float CUBE[] = {
        1.0f, 1.0f, 1.0f,    // 0
        -1.0f, 1.0f, 1.0f,   // 1
        -1.0f, -1.0f, 1.0f,  // 2
        1.0f, -1.0f, 1.0f,   // 3
        1.0f, 1.0f, -1.0f,   // 4
        -1.0f, 1.0f, -1.0f,  // 5
        -1.0f, -1.0f, -1.0f, // 6
        1.0f, -1.0f, -1.0f   // 7
    };
    
    // Cube indices (like original gfx.cpp lines 444-462)
    const unsigned int CUBE_INDICES[] = {
        7, 6, 5,
        5, 4, 7,
        
        5, 6, 2,
        2, 1, 5,
        
        0, 3, 7,
        7, 4, 0,
        
        0, 1, 2,
        2, 3, 0,
        
        0, 4, 5,
        5, 1, 0,
        
        7, 2, 6,
        3, 2, 7
    };
    
    // Create VAO
    glGenVertexArrays(1, &skybox.vao);
    glGenBuffers(1, &skybox.vbo);
    glGenBuffers(1, &skybox.ibo);
    
    glBindVertexArray(skybox.vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, skybox.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE), CUBE, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES, GL_STATIC_DRAW);
    
    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    printf("Skybox VAO initialized\n");
    
    // Skybox shaders (from original skyboxvert.glsl and skyboxfrag.glsl)
    const char* skybox_vertex_src = load_shader_source("assets/shaders/skyboxvert.glsl");
    const char* skybox_fragment_src = load_shader_source("assets/shaders/skyboxfrag.glsl");    
    skybox.shader = shader_compile(skybox_vertex_src, skybox_fragment_src);
    
    // Load cubemap textures (like original textures.impfile lines 118-126)
    const char* cubemap_faces[6] = {
        "assets/textures/skybox/skybox_east.png",   // +X
        "assets/textures/skybox/skybox_west.png",   // -X
        "assets/textures/skybox/skybox_up.png",     // +Y
        "assets/textures/skybox/skybox_down.png",   // -Y
        "assets/textures/skybox/skybox_north.png",  // +Z
        "assets/textures/skybox/skybox_south.png"   // -Z
    };

    skybox.cubemap_texture = load_cubemap(cubemap_faces);
    printf("Skybox initialized\n\n");

    return skybox;
}

// Cubemap loading (like original gfx.cpp loadCubemap)
GLuint load_cubemap(const char* faces[6]) {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);
    
    for (int i = 0; i < 6; i++) {
        int width, height, channels;
        unsigned char* data = stbi_load(faces[i], &width, &height, &channels, 0);
        
        if (data) {
            GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                        0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            printf("  Loaded: %s (%dx%d, %d channels)\n", faces[i], width, height, channels);
        } else {
            fprintf(stderr, "  Failed to load cubemap face: %s\n", faces[i]);
        }
        stbi_image_free(data);
    }
    
    // Set texture parameters (like original lines 591-595)
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    return texture_id;
}

void skybox_render(SkyboxGL* skybox, float* persp, float* view) {
    if (!skybox->shader || !skybox->cubemap_texture) return;
    
    // Draw skybox (like original displaySkybox in display.cpp)
    glCullFace(GL_FRONT);  // Cull front faces for skybox (line 28)
    glDepthFunc(GL_LEQUAL);  // Change depth function so depth test passes when values are equal to depth buffer's content
    
    glUseProgram(skybox->shader);
    
    // Bind cubemap texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->cubemap_texture);
    glUniform1i(glGetUniformLocation(skybox->shader, "skybox"), 0);
    
    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(skybox->shader, "persp"), 1, GL_FALSE, persp);
    
    // Remove translation from view matrix (only keep rotation, like original line 36)
    // skyboxView = mat4(mat3(cam.viewMatrix()))
    float skybox_view[16] = {
        view[0], view[1], view[2], 0.0f,
        view[4], view[5], view[6], 0.0f,
        view[8], view[9], view[10], 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    glUniformMatrix4fv(glGetUniformLocation(skybox->shader, "view"), 1, GL_FALSE, skybox_view);
    
    // Draw cube
    glBindVertexArray(skybox->vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Unbind cubemap texture to prevent interference with subsequent 2D texture rendering
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // Restore state
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
}

void skybox_cleanup(SkyboxGL* skybox) {
    if (skybox->vao) glDeleteVertexArrays(1, &skybox->vao);
    if (skybox->vbo) glDeleteBuffers(1, &skybox->vbo);
    if (skybox->ibo) glDeleteBuffers(1, &skybox->ibo);
    if (skybox->shader) glDeleteProgram(skybox->shader);
    if (skybox->cubemap_texture) glDeleteTextures(1, &skybox->cubemap_texture);
}
