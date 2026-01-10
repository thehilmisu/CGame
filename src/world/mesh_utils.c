#include "mesh_utils.h"
#include "../math/math_ops.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FMAX(a, b) ((a) > (b) ? (a) : (b))
#define FMIN(a, b) ((a) < (b) ? (a) : (b))
#define IMAX(a, b) ((a) > (b) ? (a) : (b))

// ============================================================================
// Mesh Management
// ============================================================================

ProceduralMesh mesh_create(unsigned int initial_capacity) {
    ProceduralMesh mesh;
    mesh.capacity = initial_capacity;
    mesh.vertex_count = 0;
    mesh.index_count = 0;

    mesh.vertices = malloc(initial_capacity * 3 * sizeof(float));
    mesh.normals = malloc(initial_capacity * 3 * sizeof(float));
    mesh.texcoords = malloc(initial_capacity * 2 * sizeof(float));
    mesh.indices = malloc(initial_capacity * 3 * sizeof(unsigned int));

    return mesh;
}

void mesh_free(ProceduralMesh* mesh) {
    if (mesh->vertices) free(mesh->vertices);
    if (mesh->normals) free(mesh->normals);
    if (mesh->texcoords) free(mesh->texcoords);
    if (mesh->indices) free(mesh->indices);

    mesh->vertices = NULL;
    mesh->normals = NULL;
    mesh->texcoords = NULL;
    mesh->indices = NULL;
    mesh->vertex_count = 0;
    mesh->index_count = 0;
    mesh->capacity = 0;
}

void mesh_reserve(ProceduralMesh* mesh, unsigned int vertex_count) {
    if (vertex_count <= mesh->capacity) return;

    unsigned int new_capacity = vertex_count * 2;

    mesh->vertices = realloc(mesh->vertices, new_capacity * 3 * sizeof(float));
    mesh->normals = realloc(mesh->normals, new_capacity * 3 * sizeof(float));
    mesh->texcoords = realloc(mesh->texcoords, new_capacity * 2 * sizeof(float));
    mesh->indices = realloc(mesh->indices, new_capacity * 3 * sizeof(unsigned int));

    mesh->capacity = new_capacity;
}

void mesh_add_vertex(ProceduralMesh* mesh, float x, float y, float z,
                     float nx, float ny, float nz,
                     float u, float v) {
    if (mesh->vertex_count >= mesh->capacity) {
        mesh_reserve(mesh, mesh->vertex_count + 1);
    }

    unsigned int idx = mesh->vertex_count;
    mesh->vertices[idx * 3 + 0] = x;
    mesh->vertices[idx * 3 + 1] = y;
    mesh->vertices[idx * 3 + 2] = z;

    mesh->normals[idx * 3 + 0] = nx;
    mesh->normals[idx * 3 + 1] = ny;
    mesh->normals[idx * 3 + 2] = nz;

    mesh->texcoords[idx * 2 + 0] = u;
    mesh->texcoords[idx * 2 + 1] = v;

    mesh->vertex_count++;
}

void mesh_add_triangle(ProceduralMesh* mesh, unsigned int i1, unsigned int i2, unsigned int i3) {
    if (mesh->index_count + 3 > mesh->capacity * 3) {
        mesh_reserve(mesh, mesh->vertex_count + 10);
    }

    mesh->indices[mesh->index_count++] = i1;
    mesh->indices[mesh->index_count++] = i2;
    mesh->indices[mesh->index_count++] = i3;
}

// ============================================================================
// Mesh Transformation
// ============================================================================

void mesh_transform(ProceduralMesh* mesh, float* transform_matrix) {
    for (unsigned int i = 0; i < mesh->vertex_count; i++) {
        float* v = &mesh->vertices[i * 3];
        float result[3];

        // Transform position (w = 1)
        result[0] = transform_matrix[0] * v[0] + transform_matrix[4] * v[1] +
                    transform_matrix[8] * v[2] + transform_matrix[12];
        result[1] = transform_matrix[1] * v[0] + transform_matrix[5] * v[1] +
                    transform_matrix[9] * v[2] + transform_matrix[13];
        result[2] = transform_matrix[2] * v[0] + transform_matrix[6] * v[1] +
                    transform_matrix[10] * v[2] + transform_matrix[14];

        v[0] = result[0];
        v[1] = result[1];
        v[2] = result[2];

        // Transform normal (w = 0, no translation)
        float* n = &mesh->normals[i * 3];
        result[0] = transform_matrix[0] * n[0] + transform_matrix[4] * n[1] +
                    transform_matrix[8] * n[2];
        result[1] = transform_matrix[1] * n[0] + transform_matrix[5] * n[1] +
                    transform_matrix[9] * n[2];
        result[2] = transform_matrix[2] * n[0] + transform_matrix[6] * n[1] +
                    transform_matrix[10] * n[2];

        // Normalize
        float len = sqrtf(result[0] * result[0] + result[1] * result[1] + result[2] * result[2]);
        if (len > 0.0f) {
            n[0] = result[0] / len;
            n[1] = result[1] / len;
            n[2] = result[2] / len;
        }
    }
}

void mesh_transform_texcoords(ProceduralMesh* mesh, float* transform_matrix) {
    for (unsigned int i = 0; i < mesh->vertex_count; i++) {
        float* tc = &mesh->texcoords[i * 2];
        float result[2];

        // Transform as 2D point (u, v, 0, 1)
        result[0] = transform_matrix[0] * tc[0] + transform_matrix[4] * tc[1] + transform_matrix[12];
        result[1] = transform_matrix[1] * tc[0] + transform_matrix[5] * tc[1] + transform_matrix[13];

        tc[0] = result[0];
        tc[1] = result[1];
    }
}

// ============================================================================
// Mesh Merging
// ============================================================================

ProceduralMesh mesh_merge(ProceduralMesh* mesh1, ProceduralMesh* mesh2) {
    ProceduralMesh result = mesh_create(mesh1->vertex_count + mesh2->vertex_count);

    // Copy mesh1
    memcpy(result.vertices, mesh1->vertices, mesh1->vertex_count * 3 * sizeof(float));
    memcpy(result.normals, mesh1->normals, mesh1->vertex_count * 3 * sizeof(float));
    memcpy(result.texcoords, mesh1->texcoords, mesh1->vertex_count * 2 * sizeof(float));
    memcpy(result.indices, mesh1->indices, mesh1->index_count * sizeof(unsigned int));
    result.vertex_count = mesh1->vertex_count;
    result.index_count = mesh1->index_count;

    // Append mesh2
    unsigned int offset = mesh1->vertex_count;
    memcpy(&result.vertices[offset * 3], mesh2->vertices, mesh2->vertex_count * 3 * sizeof(float));
    memcpy(&result.normals[offset * 3], mesh2->normals, mesh2->vertex_count * 3 * sizeof(float));
    memcpy(&result.texcoords[offset * 2], mesh2->texcoords, mesh2->vertex_count * 2 * sizeof(float));

    // Copy indices with offset
    for (unsigned int i = 0; i < mesh2->index_count; i++) {
        result.indices[result.index_count++] = mesh2->indices[i] + offset;
    }

    result.vertex_count += mesh2->vertex_count;

    return result;
}

// ============================================================================
// Primitive Generators
// ============================================================================

ProceduralMesh mesh_create_cone_type1(unsigned int prec) {
    ProceduralMesh cone = mesh_create(prec + 1);

    // Top vertex
    mesh_add_vertex(&cone, 0.0f, 1.0f, 0.0f,
                    0.0f, 1.0f, 0.0f,
                    0.5f, 1.0f);

    // Bottom circle vertices
    for (unsigned int i = 0; i < prec; i++) {
        float angle = 2.0f * M_PI / (float)prec * (float)i;
        float x = cosf(angle);
        float z = sinf(angle);

        // Calculate normal
        float next_angle = 2.0f * M_PI / (float)prec * (float)(i + 1);
        float next_x = cosf(next_angle);
        float next_z = sinf(next_angle);

        // Cross product of (top - vert) and (nextvert - vert)
        float edge1_x = 0.0f - x, edge1_y = 1.0f - 0.0f, edge1_z = 0.0f - z;
        float edge2_x = next_x - x, edge2_y = 0.0f - 0.0f, edge2_z = next_z - z;

        float nx = edge1_y * edge2_z - edge1_z * edge2_y;
        float ny = edge1_z * edge2_x - edge1_x * edge2_z;
        float nz = edge1_x * edge2_y - edge1_y * edge2_x;
        float nlen = sqrtf(nx * nx + ny * ny + nz * nz);
        if (nlen > 0.0f) {
            nx /= nlen; ny /= nlen; nz /= nlen;
        }

        // Texture coordinates
        float fract = (float)(i % (prec / 2)) / (float)(prec - 1);
        float tcx;
        if (i < prec / 2)
            tcx = FMIN(fract * 2.0f, 1.0f);
        else
            tcx = FMAX(1.0f - fract * 2.0f, 0.0f);

        mesh_add_vertex(&cone, x, 0.0f, z, nx, ny, nz, tcx, 0.0f);

        // Add triangle (CCW winding for outward-facing normals)
        unsigned int index1 = i + 1;
        unsigned int index2 = index1 + 1;
        if (index2 >= prec + 1)
            index2 = 1;
        mesh_add_triangle(&cone, 0, index1, index2);
    }

    return cone;
}

ProceduralMesh mesh_create_cone_type2(unsigned int prec) {
    ProceduralMesh cone = mesh_create(prec + 1);

    // Top vertex
    mesh_add_vertex(&cone, 0.0f, 1.0f, 0.0f,
                    0.0f, 1.0f, 0.0f,
                    0.5f, 0.5f);

    // Bottom circle vertices
    for (unsigned int i = 0; i < prec; i++) {
        float angle = 2.0f * M_PI / (float)prec * (float)i;
        float x = cosf(angle);
        float z = sinf(angle);

        // Calculate normal (same as type1)
        float next_angle = 2.0f * M_PI / (float)prec * (float)(i + 1);
        float next_x = cosf(next_angle);
        float next_z = sinf(next_angle);

        float edge1_x = 0.0f - x, edge1_y = 1.0f - 0.0f, edge1_z = 0.0f - z;
        float edge2_x = next_x - x, edge2_y = 0.0f - 0.0f, edge2_z = next_z - z;

        float nx = edge1_y * edge2_z - edge1_z * edge2_y;
        float ny = edge1_z * edge2_x - edge1_x * edge2_z;
        float nz = edge1_x * edge2_y - edge1_y * edge2_x;
        float nlen = sqrtf(nx * nx + ny * ny + nz * nz);
        if (nlen > 0.0f) {
            nx /= nlen; ny /= nlen; nz /= nlen;
        }

        // Texture coordinates (radial mapping)
        float tc_x = cosf(angle) * 0.5f + 0.5f;
        float tc_y = sinf(angle) * 0.5f + 0.5f;

        mesh_add_vertex(&cone, x, 0.0f, z, nx, ny, nz, tc_x, tc_y);

        // Add triangle (CCW winding for outward-facing normals)
        unsigned int index1 = i + 1;
        unsigned int index2 = index1 + 1;
        if (index2 >= prec + 1)
            index2 = 1;
        mesh_add_triangle(&cone, 0, index1, index2);
    }

    return cone;
}

ProceduralMesh mesh_create_frustum(unsigned int prec, float radius1, float radius2) {
    ProceduralMesh frustum = mesh_create(prec * 2);

    // Bottom circle
    for (unsigned int i = 0; i < prec; i++) {
        float angle = (float)i / (float)prec * 2.0f * M_PI;
        float x = cosf(angle) * radius1;
        float z = sinf(angle) * radius1;

        // Calculate normal
        float top_x = cosf(angle) * radius2;
        float top_z = sinf(angle) * radius2;
        float next_angle = (float)(i + 1) / (float)prec * 2.0f * M_PI;
        float next_x = cosf(next_angle) * radius1;
        float next_z = sinf(next_angle) * radius1;

        float edge1_x = top_x - x, edge1_y = 1.0f - 0.0f, edge1_z = top_z - z;
        float edge2_x = next_x - x, edge2_y = 0.0f - 0.0f, edge2_z = next_z - z;

        float nx = edge1_y * edge2_z - edge1_z * edge2_y;
        float ny = edge1_z * edge2_x - edge1_x * edge2_z;
        float nz = edge1_x * edge2_y - edge1_y * edge2_x;
        float nlen = sqrtf(nx * nx + ny * ny + nz * nz);
        if (nlen > 0.0f) {
            nx /= nlen; ny /= nlen; nz /= nlen;
        }

        // Texture coordinates
        float fract = (float)(i % (prec / 2)) / (float)(prec - 1);
        float tcx;
        if (i < prec / 2)
            tcx = FMIN(fract * 2.0f, 1.0f);
        else
            tcx = FMAX(1.0f - fract * 2.0f, 0.0f);

        mesh_add_vertex(&frustum, x, 0.0f, z, nx, ny, nz, tcx, 0.0f);
    }

    // Top circle
    for (unsigned int i = 0; i < prec; i++) {
        float angle = (float)i / (float)prec * 2.0f * M_PI;
        float x = cosf(angle) * radius2;
        float z = sinf(angle) * radius2;

        // Calculate normal
        float bot_x = cosf(angle) * radius1;
        float bot_z = sinf(angle) * radius1;
        float next_angle = (float)(i + 1) / (float)prec * 2.0f * M_PI;
        float next_x = cosf(next_angle) * radius2;
        float next_z = sinf(next_angle) * radius2;

        float edge1_x = x - bot_x, edge1_y = 1.0f - 0.0f, edge1_z = z - bot_z;
        float edge2_x = next_x - x, edge2_y = 0.0f - 1.0f, edge2_z = next_z - z;

        float nx = edge1_y * edge2_z - edge1_z * edge2_y;
        float ny = edge1_z * edge2_x - edge1_x * edge2_z;
        float nz = edge1_x * edge2_y - edge1_y * edge2_x;
        float nlen = sqrtf(nx * nx + ny * ny + nz * nz);
        if (nlen > 0.0f) {
            nx /= nlen; ny /= nlen; nz /= nlen;
        }

        // Texture coordinates
        float fract = (float)(i % (prec / 2)) / (float)(prec - 1);
        float tcx;
        if (i < prec / 2)
            tcx = FMIN(fract * 2.0f, 1.0f);
        else
            tcx = FMAX(1.0f - fract * 2.0f, 0.0f);

        mesh_add_vertex(&frustum, x, 1.0f, z, nx, ny, nz, tcx, 1.0f);
    }

    // Create indices (quad = 2 triangles)
    for (unsigned int i = 0; i < prec; i++) {
        unsigned int next_i = (i + 1) % prec;

        mesh_add_triangle(&frustum, i, next_i, i + prec);
        mesh_add_triangle(&frustum, i + prec, next_i + prec, next_i);
    }

    return frustum;
}

ProceduralMesh mesh_create_plane(unsigned int subdivision) {
    unsigned int grid_size = subdivision + 3;
    ProceduralMesh plane = mesh_create(grid_size * grid_size);

    // Generate vertices
    for (unsigned int i = 0; i <= subdivision + 2; i++) {
        for (unsigned int j = 0; j <= subdivision + 2; j++) {
            float x = (float)j / (float)(subdivision + 2) * 2.0f - 1.0f;
            float y = (float)i / (float)(subdivision + 2) * 2.0f - 1.0f;
            float tcx = (float)j / (float)(subdivision + 2);
            float tcy = (float)i / (float)(subdivision + 2);

            mesh_add_vertex(&plane, x, y, 0.0f,
                           0.0f, 0.0f, 1.0f,
                           tcx, tcy);
        }
    }

    // Generate indices
    for (unsigned int i = 0; i <= subdivision + 1; i++) {
        for (unsigned int j = 0; j <= subdivision + 1; j++) {
            unsigned int idx = i * grid_size + j;
            unsigned int idx_right = i * grid_size + j + 1;
            unsigned int idx_down = (i + 1) * grid_size + j;
            unsigned int idx_diag = (i + 1) * grid_size + j + 1;

            mesh_add_triangle(&plane, idx, idx_right, idx_down);
            mesh_add_triangle(&plane, idx_down, idx_diag, idx_right);
        }
    }

    return plane;
}
