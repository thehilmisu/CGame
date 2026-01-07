#ifndef TERRAIN_WORLD_H
#define TERRAIN_WORLD_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

// Constants from original (infworld.hpp)
#define PREC 40
#define CHUNK_SZ 64.0f
#define HEIGHT 270.0f
#define SCALE 2.5f
#define FREQUENCY 720.0f
#define MAX_LOD 5
#define LOD_SCALE 2.0f
#define RANGE 14

// Chunk position
typedef struct {
    int x, z;
} ChunkPos;

// Noise permutation
typedef struct {
    int perm[256];
} NoisePermutation;

// Terrain seed (permutations for noise)
typedef struct {
    NoisePermutation perms[9];
} TerrainSeed;

// Chunk data (stores only height + normals like original)
typedef struct {
    float* vertices;  // Only stores: [height, normal.x, normal.y] per vertex
    unsigned int vertex_count;
    ChunkPos pos;
} ChunkMesh;

// Chunk table (one LOD level)
typedef struct {
    GLuint* vaos;
    GLuint* vbos;
    GLuint* ibos;
    ChunkPos* positions;
    unsigned int chunk_count;
    unsigned int size;
    float scale;
    float height;
    ChunkPos center;
    // For incremental updates (like original chunktable.cpp)
    ChunkPos* new_chunks;       // Queue of chunks to generate
    int* reusable_indices;      // Queue of indices that can be reused
    int new_chunk_count;
    int reusable_index_count;
    int new_chunk_capacity;
    int reusable_capacity;
} ChunkTable;

// LOD Manager (manages multiple chunk tables)
typedef struct {
    ChunkTable* lod_levels;
    int num_lods;
    GLuint terrain_shader;
    GLuint terrain_texture;
} TerrainLODManagerGL;

// Functions
TerrainSeed terrain_seed_create(int seed);
float terrain_get_height(float x, float z, const TerrainSeed* seed);

ChunkTable chunk_table_create(unsigned int range, float scale, float h);
void chunk_table_gen_buffers(ChunkTable* ct);
void chunk_table_add_chunk(ChunkTable* ct, unsigned int index, const ChunkMesh* mesh, int x, int z);
void chunk_table_draw(ChunkTable* ct, GLuint shader_program, float* view_matrix, float* proj_matrix);
void chunk_table_cleanup(ChunkTable* ct);

ChunkMesh chunk_create(const TerrainSeed* seed, int chunkx, int chunkz, float maxheight, float chunkscale);
void chunk_mesh_free(ChunkMesh* mesh);

TerrainLODManagerGL terrain_lod_manager_create(const TerrainSeed* seed);
void terrain_lod_manager_generate_all(TerrainLODManagerGL* lod, const TerrainSeed* seed, int center_x, int center_z);
void terrain_lod_manager_update(TerrainLODManagerGL* lod, const TerrainSeed* seed, int center_x, int center_z);
void terrain_lod_manager_render(TerrainLODManagerGL* lod, float* view, float* proj, 
                                float camera_x, float camera_y, float camera_z, float time);
void terrain_lod_manager_cleanup(TerrainLODManagerGL* lod);

// Water rendering (instanced quads like original)
typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    GLuint shader;
    int range;
    float scale;
} WaterManagerGL;

WaterManagerGL water_manager_init(void);
void water_render_gl(WaterManagerGL* water, float* persp, float* view, 
                     float camera_x, float camera_y, float camera_z, float time);
void water_cleanup_gl(WaterManagerGL* water);

// Skybox rendering (cubemap like original)
typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    GLuint shader;
    GLuint cubemap_texture;
} SkyboxGL;

SkyboxGL skybox_init(void);
GLuint load_cubemap(const char* faces[6]);
void skybox_render(SkyboxGL* skybox, float* persp, float* view);
void skybox_cleanup(SkyboxGL* skybox);

#endif // TERRAIN_WORLD_H
