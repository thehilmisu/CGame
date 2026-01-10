#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include <glad/glad.h>

// Dynamic mesh structure for procedural generation (different from entity Mesh)
typedef struct {
    float* vertices;       // Position data (x, y, z)
    float* normals;        // Normal vectors (x, y, z)
    float* texcoords;      // Texture coordinates (u, v)
    unsigned int* indices; // Triangle indices

    unsigned int vertex_count;
    unsigned int index_count;
    unsigned int capacity;  // Current array capacity
} ProceduralMesh;

// Mesh management
ProceduralMesh mesh_create(unsigned int initial_capacity);
void mesh_free(ProceduralMesh* mesh);
void mesh_add_vertex(ProceduralMesh* mesh, float x, float y, float z,
                     float nx, float ny, float nz,
                     float u, float v);
void mesh_add_triangle(ProceduralMesh* mesh, unsigned int i1, unsigned int i2, unsigned int i3);
void mesh_reserve(ProceduralMesh* mesh, unsigned int vertex_count);

// Mesh transformation
void mesh_transform(ProceduralMesh* mesh, float* transform_matrix);
void mesh_transform_texcoords(ProceduralMesh* mesh, float* transform_matrix);

// Mesh merging
ProceduralMesh mesh_merge(ProceduralMesh* mesh1, ProceduralMesh* mesh2);

// Primitive mesh generators
ProceduralMesh mesh_create_frustum(unsigned int detail, float radius1, float radius2);
ProceduralMesh mesh_create_cone_type1(unsigned int detail);
ProceduralMesh mesh_create_cone_type2(unsigned int detail);
ProceduralMesh mesh_create_plane(unsigned int subdivision);

#endif // MESH_UTILS_H
