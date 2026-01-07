#include "terrain.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include "file_ops.h"

#include <stb_image/stb_image.h>

// Use proper noise implementation from noise.c (working implementation)
extern float noise_perlin_2d(float x, float y, const NoisePermutation* perm);

// Noise implementation (from original)
static void init_permutation(NoisePermutation* perm, int seed) {
    // EXACTLY like original rng::createPermutation (noise.cpp lines 14-30)
    int values[256];
    int count = 256;
    for (int i = 0; i < 256; i++) {
        values[i] = i;
    }
    
    int index = 0;
    // std::minstd_rand: multiplier=48271, increment=0, modulus=2147483647
    uint32_t lcg = (uint32_t)seed;
    const uint32_t a = 48271;
    const uint32_t m = 2147483647;
    
    while (count > 0) {
        lcg = (uint64_t)lcg * a % m;
        int randindex = lcg % count;
        perm->perm[index++] = values[randindex];
        values[randindex] = values[count - 1];
        count--;
    }
}

TerrainSeed terrain_seed_create(int seed) {
    TerrainSeed ts;
    // Use std::minstd_rand parameters (EXACTLY like original infworld.cpp line 11-14)
    // std::minstd_rand: multiplier=48271, increment=0, modulus=2147483647
    uint32_t rng = (uint32_t)seed;
    const uint32_t a = 48271;
    const uint32_t m = 2147483647;
    
    for (int i = 0; i < 9; i++) {
        // Generate next seed using std::minstd_rand formula
        rng = (uint64_t)rng * a % m;
        init_permutation(&ts.perms[i], (int)rng);
    }
    return ts;
}

float terrain_get_height(float x, float z, const TerrainSeed* seed) {
    float height = 0.0f;
    float freq = FREQUENCY;
    float amplitude = 1.0f;
    
    // Multi-octave noise - EXACTLY like original (uses all 9 permutations)
    for (int i = 0; i < 9; i++) {
        float h = noise_perlin_2d(x / freq, z / freq, &seed->perms[i]) * amplitude;
        height += h;
        freq /= 2.0f;
        amplitude /= 2.0f;
    }
    
    // Height interpolation (from original - EXACTLY like infworld.cpp lines 42-49)
    if (height < -0.1f)
        height = (height - (-1.0f)) / (-0.1f - (-1.0f)) * (0.003f - (-1.0f)) + (-1.0f);
    else if (height >= -0.1f && height < 0.0f)
        height = (height - (-0.1f)) / (0.0f - (-0.1f)) * (0.03f - 0.003f) + 0.003f;
    else if (height >= 0.0f && height < 0.15f)
        height = (height - 0.0f) / (0.15f - 0.0f) * (0.12f - 0.03f) + 0.03f;
    else if (height >= 0.15f)
        height = (height - 0.15f) / (1.0f - 0.15f) * (1.0f - 0.12f) + 0.12f;
    
    // Return NORMALIZED height between -1 and 1 (like original getHeight)
    return height;
}

ChunkTable chunk_table_create(unsigned int range, float scale, float h) {
    ChunkTable ct;
    ct.size = 2 * range + 1;
    ct.chunk_count = ct.size * ct.size;
    ct.scale = scale;
    ct.height = h;
    ct.center = (ChunkPos){0, 0};
    
    ct.vaos = (GLuint*)calloc(ct.chunk_count, sizeof(GLuint));
    ct.vbos = (GLuint*)calloc(ct.chunk_count * 3, sizeof(GLuint)); // 3 buffers per chunk
    ct.ibos = (GLuint*)calloc(ct.chunk_count, sizeof(GLuint));
    ct.positions = (ChunkPos*)calloc(ct.chunk_count, sizeof(ChunkPos));
    
    // Initialize incremental update queues
    ct.new_chunk_capacity = ct.chunk_count * 2;  // Generous capacity
    ct.reusable_capacity = ct.chunk_count;
    ct.new_chunks = (ChunkPos*)calloc(ct.new_chunk_capacity, sizeof(ChunkPos));
    ct.reusable_indices = (int*)calloc(ct.reusable_capacity, sizeof(int));
    ct.new_chunk_count = 0;
    ct.reusable_index_count = 0;
    
    return ct;
}

void chunk_table_gen_buffers(ChunkTable* ct) {
    glGenVertexArrays(ct->chunk_count, ct->vaos);
    glGenBuffers(ct->chunk_count * 3, ct->vbos); // 3 buffers per chunk
    glGenBuffers(ct->chunk_count, ct->ibos);
}

// Global chunk indices (shared by ALL chunks - like original CHUNK_INDICES)
static unsigned int* g_chunk_indices = NULL;
static unsigned int g_index_count = 0;

static void init_global_indices() {
    if (g_chunk_indices != NULL) return;
    
    g_index_count = PREC * PREC * 6;
    g_chunk_indices = (unsigned int*)malloc(g_index_count * sizeof(unsigned int));
    
    unsigned int idx = 0;
    for (unsigned int i = 0; i < PREC; i++) {
        for (unsigned int j = 0; j < PREC; j++) {
            unsigned int base = i * (PREC + 1) + j;
            g_chunk_indices[idx++] = base + (PREC + 1);
            g_chunk_indices[idx++] = base + 1;
            g_chunk_indices[idx++] = base;
            
            g_chunk_indices[idx++] = base + 1;
            g_chunk_indices[idx++] = base + (PREC + 1);
            g_chunk_indices[idx++] = base + (PREC + 1) + 1;
        }
    }
}

void chunk_table_add_chunk(ChunkTable* ct, unsigned int index, const ChunkMesh* mesh, int x, int z) {
    init_global_indices();
    
    ct->positions[index] = (ChunkPos){x, z};
    
    // EXACTLY like original chunktable.cpp addChunk()
    glBindVertexArray(ct->vaos[index]);
    
    // Buffer 0 (vertex data - interleaved: height, normal.x, normal.y)
    glBindBuffer(GL_ARRAY_BUFFER, ct->vbos[index * 3 + 0]);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * 3 * sizeof(float), 
                 mesh->vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Buffer 1 (reuse same data for normals, different offset)
    glBindBuffer(GL_ARRAY_BUFFER, ct->vbos[index * 3 + 1]);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * 3 * sizeof(float), 
                 mesh->vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(1 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Buffer 2 (indices - shared across all chunks)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ct->ibos[index]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, g_index_count * sizeof(unsigned int), 
                 g_chunk_indices, GL_STATIC_DRAW);
}

// Update existing chunk data (like original chunktable.cpp updateChunk)
void chunk_table_update_chunk(ChunkTable* ct, unsigned int index, const ChunkMesh* mesh, int x, int z) {
    ct->positions[index] = (ChunkPos){x, z};
    
    glBindVertexArray(ct->vaos[index]);
    
    // Update Buffer 0 (vertex data)
    glBindBuffer(GL_ARRAY_BUFFER, ct->vbos[index * 3 + 0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mesh->vertex_count * 3 * sizeof(float), mesh->vertices);
    
    // Update Buffer 1 (normals - same data)
    glBindBuffer(GL_ARRAY_BUFFER, ct->vbos[index * 3 + 1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mesh->vertex_count * 3 * sizeof(float), mesh->vertices);
}

void chunk_table_draw(ChunkTable* ct, GLuint shader_program, float* view_matrix, float* proj_matrix) {
    unsigned int index_count = PREC * PREC * 6;
    
    // Set uniforms that are constant for all chunks
    GLint persp_loc = glGetUniformLocation(shader_program, "persp");
    GLint view_loc = glGetUniformLocation(shader_program, "view");
    GLint lightdir_loc = glGetUniformLocation(shader_program, "lightdir");
    GLint maxheight_loc = glGetUniformLocation(shader_program, "maxheight");
    GLint chunksz_loc = glGetUniformLocation(shader_program, "chunksz");
    GLint prec_loc = glGetUniformLocation(shader_program, "prec");
    GLint transform_loc = glGetUniformLocation(shader_program, "transform");
    
    glUniformMatrix4fv(persp_loc, 1, GL_FALSE, proj_matrix);
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view_matrix);
    
    float light_dir[3] = {-1.0f, 1.0f, -1.0f};
    float len = sqrtf(light_dir[0]*light_dir[0] + light_dir[1]*light_dir[1] + light_dir[2]*light_dir[2]);
    light_dir[0] /= len; light_dir[1] /= len; light_dir[2] /= len;
    glUniform3fv(lightdir_loc, 1, light_dir);
    
    glUniform1f(maxheight_loc, ct->height * SCALE);
    glUniform1f(chunksz_loc, ct->scale);
    glUniform1i(prec_loc, PREC);
    
    // Draw each chunk with its transform
    for (unsigned int i = 0; i < ct->chunk_count; i++) {
        // Create transform matrix (translation to chunk position)
        float transform[16] = {0};
        transform[0] = transform[5] = transform[10] = transform[15] = 1.0f;
        transform[12] = ct->positions[i].x * ct->scale * 2.0f;
        transform[13] = 0.0f;
        transform[14] = ct->positions[i].z * ct->scale * 2.0f;
        
        glUniformMatrix4fv(transform_loc, 1, GL_FALSE, transform);
        
        glBindVertexArray(ct->vaos[i]);
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    }
    
    glBindVertexArray(0);
}

void chunk_table_cleanup(ChunkTable* ct) {
    if (ct->vaos) {
        glDeleteVertexArrays(ct->chunk_count, ct->vaos);
        free(ct->vaos);
    }
    if (ct->vbos) {
        glDeleteBuffers(ct->chunk_count, ct->vbos);
        free(ct->vbos);
    }
    if (ct->ibos) {
        glDeleteBuffers(ct->chunk_count, ct->ibos);
        free(ct->ibos);
    }
    if (ct->positions) {
        free(ct->positions);
    }
    if (ct->new_chunks) {
        free(ct->new_chunks);
    }
    if (ct->reusable_indices) {
        free(ct->reusable_indices);
    }
}

// Compress normal to 2 floats (spherical coordinates) - like original
static void compress_normal(float nx, float ny, float nz, float* out_x, float* out_y) {
    // Convert to spherical coordinates
    float theta = atan2f(nz, nx);
    float phi = asinf(ny);
    *out_x = theta;
    *out_y = phi;
}

ChunkMesh chunk_create(const TerrainSeed* seed, int chunkx, int chunkz, float maxheight, float chunkscale) {
    ChunkMesh mesh;
    mesh.vertex_count = (PREC + 1) * (PREC + 1);
    // EXACTLY like original: only height (1) + compressed normal (2) = 3 floats per vertex!
    mesh.vertices = (float*)malloc(mesh.vertex_count * 3 * sizeof(float));
    mesh.pos = (ChunkPos){chunkx, chunkz};
    
    // Generate vertices EXACTLY like original C++ infworld.cpp lines 79-84
    // DO NOT modify this - it must match the C++ exactly
    for (unsigned int i = 0; i <= PREC; i++) {
        for (unsigned int j = 0; j <= PREC; j++) {
            // EXACTLY like C++ lines 81-82: uses i/PREC and j/PREC (NOT PREC+1)
            float x = -chunkscale + (float)i / (float)PREC * chunkscale * 2.0f;
            float z = -chunkscale + (float)j / (float)PREC * chunkscale * 2.0f;
            
            // World position - EXACTLY like C++ lines 83-84
            float tx = x + (float)chunkx * chunkscale * 2.0f;
            float tz = z + (float)chunkz * chunkscale * 2.0f;
            
            // Get height (normalized -1 to 1 from terrain_get_height)
            float height = terrain_get_height(tx, tz, seed);
            
            // Apply min height like original getTerrainVertex (lines 61-64)
            float actual_height = height * maxheight;
            if (actual_height <= 0.0f)
                actual_height = fminf(-0.007f, actual_height);
            else
                actual_height = fmaxf(0.007f, actual_height);
            
            // Calculate normals (like original - using actual world height)
            float h1_norm = terrain_get_height(tx + 0.01f, tz, seed);
            float h2_norm = terrain_get_height(tx, tz + 0.01f, seed);
            float h1 = h1_norm * maxheight;
            float h2 = h2_norm * maxheight;
            
            if (h1 <= 0.0f) h1 = fminf(-0.007f, h1);
            else h1 = fmaxf(0.007f, h1);
            if (h2 <= 0.0f) h2 = fminf(-0.007f, h2);
            else h2 = fmaxf(0.007f, h2);
            
            // Cross product to get normal (original uses v2 - vertex, v1 - vertex)
            float v1x = 0.01f, v1y = h1 - actual_height, v1z = 0.0f;
            float v2x = 0.0f, v2y = h2 - actual_height, v2z = 0.01f;
            
            // cross(v2, v1) like original line 91
            float nx = v2y * v1z - v2z * v1y;
            float ny = v2z * v1x - v2x * v1z;
            float nz = v2x * v1y - v2y * v1x;
            
            float len = sqrtf(nx*nx + ny*ny + nz*nz);
            if (len > 0.0f) {
                nx /= len; ny /= len; nz /= len;
            }
            
            // Compress normal (like original compressNormal)
            float norm_x, norm_y;
            compress_normal(nx, ny, nz, &norm_x, &norm_y);
            
            // Sequential vertex storage - original C++ uses push_back
            // With i outer, j inner: VertexID = i * (PREC+1) + j
            unsigned int idx = (i * (PREC + 1) + j) * 3;
            mesh.vertices[idx + 0] = actual_height / maxheight;  // Store normalized (like original line 94)
            mesh.vertices[idx + 1] = norm_x;
            mesh.vertices[idx + 2] = norm_y;
        }
    }
    
    return mesh;
}

void chunk_mesh_free(ChunkMesh* mesh) {
    if (mesh->vertices) {
        free(mesh->vertices);
        mesh->vertices = NULL;
    }
}


GLuint shader_compile(const char* vertex_src, const char* fragment_src) {
    // Compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_src, NULL);
    glCompileShader(vertex_shader);
    
    // Check vertex shader
    GLint success;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        fprintf(stderr, "Vertex shader compilation failed:\n%s\n", info_log);
    }
    
    // Compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_src, NULL);
    glCompileShader(fragment_shader);
    
    // Check fragment shader
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        fprintf(stderr, "Fragment shader compilation failed:\n%s\n", info_log);
    }
    
    // Link program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    // Check program
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        fprintf(stderr, "Shader program linking failed:\n%s\n", info_log);
    }
    
    // Cleanup
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return program;
}

// ===== Water Rendering =====

WaterManagerGL water_manager_init() {
    WaterManagerGL water;
    water.range = 4;  // Like original waterrange
    water.scale = CHUNK_SZ * 32.0f * SCALE;  // Like original quadscale
    
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
    
    return water;
}

void water_render_gl(WaterManagerGL* water, float* persp, float* view, 
                     float camera_x, float camera_y, float camera_z, float time) {
    (void)time;  // Unused for now (simple water)
    
    glUseProgram(water->shader);
    
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
}

// ============================================================================
// Skybox Implementation (like original display.cpp displaySkybox)
// ============================================================================

SkyboxGL skybox_init() {
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

// ===== LOD Manager =====

TerrainLODManagerGL terrain_lod_manager_create(const TerrainSeed* seed) {
    (void)seed;  // Not used in initialization
    TerrainLODManagerGL lod;
    lod.num_lods = MAX_LOD;
    lod.lod_levels = (ChunkTable*)malloc(MAX_LOD * sizeof(ChunkTable));
    
    // Create LOD levels (EXACTLY like original game.cpp generateChunks lines 89-93)
    float sz = CHUNK_SZ;
    for (int i = 0; i < MAX_LOD; i++) {
        unsigned int range = RANGE / (i + 1);
        if (range < 2) range = 2;
        
        lod.lod_levels[i] = chunk_table_create(range, sz, HEIGHT);
        chunk_table_gen_buffers(&lod.lod_levels[i]);
        
        sz *= LOD_SCALE;  // 64, 128, 256, 512, 1024
    }
    
    // Load terrain shader and texture (will be done in main for now)
    lod.terrain_shader = 0;
    lod.terrain_texture = 0;
    
    return lod;
}

void terrain_lod_manager_generate_all(TerrainLODManagerGL* lod, const TerrainSeed* seed, int center_x, int center_z) {
    // Generate all chunks for all LOD levels centered at (center_x, center_z)
    // This is only used for INITIAL generation at startup
    for (int level = 0; level < lod->num_lods; level++) {
        ChunkTable* ct = &lod->lod_levels[level];
        ct->center = (ChunkPos){center_x, center_z};
        
        int range = (ct->size - 1) / 2;
        
        for (int dx = -range; dx <= range; dx++) {
            for (int dz = -range; dz <= range; dz++) {
                int chunk_x = center_x + dx;
                int chunk_z = center_z + dz;
                
                // Pass ct->scale which is already the full chunkscale (64, 128, 256...)
                ChunkMesh mesh = chunk_create(seed, chunk_x, chunk_z, ct->height, ct->scale);
                
                int index = (dz + range) * ct->size + (dx + range);
                chunk_table_add_chunk(ct, index, &mesh, chunk_x, chunk_z);
                
                chunk_mesh_free(&mesh);
            }
        }
        
        // Only print during initial generation (startup)
        printf("  LOD %d: %d chunks (scale=%.0f)\n", level, ct->size * ct->size, ct->scale);
    }
}

// Generate new chunks incrementally (EXACTLY like original chunktable.cpp lines 148-191)
static void chunk_table_generate_new_chunks(ChunkTable* ct, const TerrainSeed* seed, 
                                            float camera_x, float camera_z) {
    // If there are chunks queued, update ONE chunk this frame (line 153-160)
    if (ct->reusable_index_count > 0) {
        int idx = ct->reusable_index_count - 1;
        int slot_index = ct->reusable_indices[idx];
        ChunkPos new_pos = ct->new_chunks[idx];
        
        // Generate and update this one chunk
        ChunkMesh mesh = chunk_create(seed, new_pos.x, new_pos.z, ct->height, ct->scale);
        chunk_table_update_chunk(ct, slot_index, &mesh, new_pos.x, new_pos.z);
        chunk_mesh_free(&mesh);
        
        // Pop from queues
        ct->reusable_index_count--;
        ct->new_chunk_count--;
        return;
    }
    
    // Calculate current chunk position (line 163-166)
    float chunksz = ct->scale * (float)PREC / (float)(PREC + 1);
    int ix = (int)floorf((camera_z + chunksz * SCALE) / (chunksz * SCALE * 2.0f));
    int iz = (int)floorf((camera_x + chunksz * SCALE) / (chunksz * SCALE * 2.0f));
    
    // If center hasn't changed, nothing to do (line 167-168)
    if (ix == ct->center.x && iz == ct->center.z)
        return;
    
    // Build list of new chunks needed (line 170-177)
    int range = (ct->size - 1) / 2;
    ct->new_chunk_count = 0;
    for (int x = ix - range; x <= ix + range; x++) {
        for (int z = iz - range; z <= iz + range; z++) {
            // Skip chunks that are already loaded (line 173-174)
            if (abs(x - ct->center.x) <= range && abs(z - ct->center.z) <= range)
                continue;
            if (ct->new_chunk_count < ct->new_chunk_capacity) {
                ct->new_chunks[ct->new_chunk_count++] = (ChunkPos){x, z};
            }
        }
    }
    
    // Find chunks that are out of range and can be reused (line 179-187)
    ct->reusable_index_count = 0;
    for (unsigned int i = 0; i < ct->chunk_count; i++) {
        int chunk_x = ct->positions[i].x;
        int chunk_z = ct->positions[i].z;
        if (abs(ix - chunk_x) <= range && abs(iz - chunk_z) <= range)
            continue;
        if (ct->reusable_index_count < ct->reusable_capacity) {
            ct->reusable_indices[ct->reusable_index_count++] = i;
        }
    }
    
    // Update center (line 189-190)
    ct->center = (ChunkPos){ix, iz};
}

void terrain_lod_manager_update(TerrainLODManagerGL* lod, const TerrainSeed* seed, int center_x, int center_z) {
    (void)center_x;
    (void)center_z;
    
    // Update all LOD levels incrementally (like original game.cpp lines 103-104)
    // Each LOD level processes ONE chunk per frame
    for (int i = 0; i < lod->num_lods; i++) {
        chunk_table_generate_new_chunks(&lod->lod_levels[i], seed, 
                                       (float)center_z, (float)center_x);
    }
}

void terrain_lod_manager_render(TerrainLODManagerGL* lod, float* view, float* proj, 
                                float camera_x, float camera_y, float camera_z, float time) {
    glUseProgram(lod->terrain_shader);
    
    // Set common uniforms (like original displayTerrain)
    glUniformMatrix4fv(glGetUniformLocation(lod->terrain_shader, "persp"), 1, GL_FALSE, proj);
    glUniformMatrix4fv(glGetUniformLocation(lod->terrain_shader, "view"), 1, GL_FALSE, view);
    glUniform3f(glGetUniformLocation(lod->terrain_shader, "lightdir"), -0.57735f, -0.57735f, -0.57735f);
    glUniform3f(glGetUniformLocation(lod->terrain_shader, "camerapos"), camera_x, camera_y, camera_z);
    glUniform1f(glGetUniformLocation(lod->terrain_shader, "time"), time);
    glUniform1f(glGetUniformLocation(lod->terrain_shader, "maxheight"), HEIGHT);
    glUniform1i(glGetUniformLocation(lod->terrain_shader, "prec"), PREC);
    
    // Bind terrain texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lod->terrain_texture);
    glUniform1i(glGetUniformLocation(lod->terrain_shader, "terraintexture"), 0);
    
    // Calculate center for LOD distance-based rendering (like original)
    // NOTE: In original, glm::vec2(center.z, center.x) - z is used for x!
    ChunkPos center = lod->lod_levels[0].center;
    // lod_levels[0].scale is already the full chunkscale (64)
    float center_world_x = (float)center.z * (float)PREC / (float)(PREC + 1) * lod->lod_levels[0].scale * SCALE * 2.0f;
    float center_world_z = (float)center.x * (float)PREC / (float)(PREC + 1) * lod->lod_levels[0].scale * SCALE * 2.0f;
    glUniform2f(glGetUniformLocation(lod->terrain_shader, "center"), center_world_x, center_world_z);
    
    float min_dist = 0.0f;
    
    // Render all LOD levels (like original displayTerrain)
    for (int level = 0; level < lod->num_lods; level++) {
        ChunkTable* ct = &lod->lod_levels[level];
        
        // Set chunksz uniform for this LOD (ct->scale is already the full chunkscale)
        glUniform1f(glGetUniformLocation(lod->terrain_shader, "chunksz"), ct->scale);

        // Calculate max distance for this LOD (EXACTLY like original display.cpp lines 156-165)
        float max_dist;
        if (level < lod->num_lods - 1) {
            // ct->scale is already the full chunkscale (64, 128, 256...)
            float chunkscale = ct->scale * 2.0f * (float)PREC / (float)(PREC + 1);
            float range = (float)((ct->size - 1) / 2) - 0.5f;
            float d = 200.0f * (float)level + 100.0f;  // Very aggressive overlap to eliminate visible lines
            max_dist = chunkscale * range * SCALE + d;

        glUniform1f(glGetUniformLocation(lod->terrain_shader, "minrange"), min_dist);
        glUniform1f(glGetUniformLocation(lod->terrain_shader, "maxrange"), max_dist);

        min_dist = max_dist - 2.0f * d;  // Original overlap amount
        } else {
            glUniform1f(glGetUniformLocation(lod->terrain_shader, "minrange"), min_dist);
            glUniform1f(glGetUniformLocation(lod->terrain_shader, "maxrange"), -1.0f);
        }
        
        // Debug: set different color for each LOD level
        float test_color[3];
        if (level == 0) { test_color[0] = 0.0f; test_color[1] = 1.0f; test_color[2] = 0.0f; } // Green
        else if (level == 1) { test_color[0] = 0.0f; test_color[1] = 0.0f; test_color[2] = 1.0f; } // Blue
        else if (level == 2) { test_color[0] = 1.0f; test_color[1] = 0.0f; test_color[2] = 0.0f; } // Red
        else if (level == 3) { test_color[0] = 0.0f; test_color[1] = 1.0f; test_color[2] = 1.0f; } // Cyan
        else { test_color[0] = 1.0f; test_color[1] = 0.0f; test_color[2] = 1.0f; } // Magenta
        glUniform3fv(glGetUniformLocation(lod->terrain_shader, "testcolor"), 1, test_color);
        
        // CRITICAL: For LOD levels > 0, skip inner chunks (like original chunktable.cpp lines 233-235)
        // This prevents z-fighting and glitching between different LOD levels
        int minrange = 0;
        if (level > 0) {
            // Calculate minrange like original display.cpp line 180
            minrange = lod->lod_levels[level - 1].size / 2 / 2;  // range / lodscale
            if (minrange < 1) minrange = 1;
        }
        
        // Draw all chunks in this LOD level
        for (unsigned int i = 0; i < ct->chunk_count; i++) {
            ChunkPos pos = ct->positions[i];
            
            // CRITICAL: Skip inner chunks for LOD > 0 (like original chunktable.cpp lines 233-235)
            if (level > 0) {
                if (abs(pos.x - center.x) < minrange && abs(pos.z - center.z) < minrange) {
                    continue;  // Skip this chunk - it's covered by a higher detail LOD
                }
            }
            
            // Calculate chunk position (EXACTLY like original chunktable.cpp draw())
            // NOTE: In original, p.z is used for x and p.x for z!
            // ct->scale is already the full chunkscale (64, 128, 256...)
            float x = (float)pos.z * ct->scale * 2.0f * (float)PREC / (float)(PREC + 1);
            float z = (float)pos.x * ct->scale * 2.0f * (float)PREC / (float)(PREC + 1);
            
            // Calculate chunk position and create transform matrix
            // EXACTLY like original chunktable.cpp lines 212-215
            float transform[16] = {
                SCALE, 0, 0, 0,
                0, SCALE, 0, 0,
                0, 0, SCALE, 0,
                x * SCALE, 0, z * SCALE, 1  // Translation IS multiplied by SCALE
            };
            glUniformMatrix4fv(glGetUniformLocation(lod->terrain_shader, "transform"), 1, GL_FALSE, transform);
            
            // Draw this chunk
            glBindVertexArray(ct->vaos[i]);
            glDrawElements(GL_TRIANGLES, PREC * PREC * 6, GL_UNSIGNED_INT, 0);
        }
        
        min_dist = max_dist;
    }
}

void terrain_lod_manager_cleanup(TerrainLODManagerGL* lod) {
    for (int i = 0; i < lod->num_lods; i++) {
        chunk_table_cleanup(&lod->lod_levels[i]);
    }
    free(lod->lod_levels);
    
    if (lod->terrain_shader > 0) {
        glDeleteProgram(lod->terrain_shader);
    }
    if (lod->terrain_texture > 0) {
        glDeleteTextures(1, &lod->terrain_texture);
    }
}
