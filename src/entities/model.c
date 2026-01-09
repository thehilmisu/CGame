#include "model.h"
#include "../graphics/texture.h"
#include <fast_obj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Helper to resolve texture paths from MTL files
static char* resolve_texture_path(const char* texture_name, const char* base_dir) {
    if (!texture_name || strlen(texture_name) == 0) {
        return NULL;
    }

    // Extract filename from path (handle absolute paths in MTL)
    const char* filename = strrchr(texture_name, '/');
    if (!filename) {
        filename = strrchr(texture_name, '\\');
    }
    if (filename) {
        filename++; // Skip the slash
    } else {
        filename = texture_name; // No path separator, use as-is
    }

    // Build path: base_dir + filename
    static char resolved_path[512];
    snprintf(resolved_path, sizeof(resolved_path), "%s%s", base_dir, filename);

    return resolved_path;
}

Model* model_load(const char* obj_path) {
    printf("Loading model: %s\n", obj_path);

    // Load OBJ file with fast_obj
    fastObjMesh* obj = fast_obj_read(obj_path);
    if (!obj) {
        fprintf(stderr, "Failed to load OBJ file: %s\n", obj_path);
        return NULL;
    }

    Model* model = (Model*)malloc(sizeof(Model));
    if (!model) {
        fast_obj_destroy(obj);
        return NULL;
    }

    // Initialize model
    memset(model, 0, sizeof(Model));
    strncpy(model->filepath, obj_path, sizeof(model->filepath) - 1);

    // Extract model name from path
    const char* name_start = strrchr(obj_path, '/');
    if (!name_start) name_start = strrchr(obj_path, '\\');
    if (name_start) name_start++;
    else name_start = obj_path;
    strncpy(model->name, name_start, sizeof(model->name) - 1);

    // Load materials
    model->material_count = obj->material_count;
    printf("Found %u materials, %u textures\n", obj->material_count, obj->texture_count);

    // Debug: Print all textures
    for (unsigned int i = 0; i < obj->texture_count; i++) {
        fastObjTexture* tex = &obj->textures[i];
        printf("Texture %u: name='%s', path='%s'\n", i,
               tex->name ? tex->name : "NULL",
               tex->path ? tex->path : "NULL");
    }

    if (model->material_count > 0) {
        model->materials = (Material*)malloc(sizeof(Material) * model->material_count);

        for (unsigned int i = 0; i < model->material_count; i++) {
            fastObjMaterial* mtl = &obj->materials[i];
            Material* mat = &model->materials[i];

            // Copy material properties
            memcpy(mat->ambient, mtl->Ka, sizeof(float) * 3);
            memcpy(mat->diffuse, mtl->Kd, sizeof(float) * 3);
            memcpy(mat->specular, mtl->Ks, sizeof(float) * 3);
            mat->shininess = mtl->Ns;

            printf("Material %u: '%s'\n", i, mtl->name);
            printf("  Texture indices: Kd=%u, Ks=%u, bump=%u, d=%u\n",
                   mtl->map_Kd, mtl->map_Ks, mtl->map_bump, mtl->map_d);
            strncpy(mat->name, mtl->name, sizeof(mat->name) - 1);

            // Load textures (resolve paths to assets/textures/)
            mat->diffuse_map = 0;
            mat->specular_map = 0;
            mat->normal_map = 0;
            mat->alpha_map = 0;

            // fast_obj uses direct array indices (not 1-based OBJ indices)
            // Index 0 is often NULL/reserved, actual textures start at higher indices
            if (mtl->map_Kd > 0 && mtl->map_Kd < obj->texture_count) {
                fastObjTexture* tex = &obj->textures[mtl->map_Kd];
                printf("  Diffuse texture: name='%s', path='%s'\n",
                       tex->name ? tex->name : "NULL",
                       tex->path ? tex->path : "NULL");
                if (tex->path) {
                    char* path = resolve_texture_path(tex->path, "assets/textures/");
                    printf("  Resolved to: %s\n", path);
                    mat->diffuse_map = texture_load(path);
                    if (mat->diffuse_map == 0) {
                        printf("  ERROR: Failed to load diffuse map: %s\n", path);
                    } else {
                        printf("  SUCCESS: Loaded texture ID %u\n", mat->diffuse_map);
                    }
                }
            }

            if (mtl->map_Ks > 0 && mtl->map_Ks < obj->texture_count) {
                fastObjTexture* tex = &obj->textures[mtl->map_Ks];
                if (tex->path) {
                    char* path = resolve_texture_path(tex->path, "assets/textures/");
                    mat->specular_map = texture_load(path);
                }
            }

            if (mtl->map_bump > 0 && mtl->map_bump < obj->texture_count) {
                fastObjTexture* tex = &obj->textures[mtl->map_bump];
                if (tex->path) {
                    char* path = resolve_texture_path(tex->path, "assets/textures/");
                    mat->normal_map = texture_load(path);
                }
            }

            if (mtl->map_d > 0 && mtl->map_d < obj->texture_count) {
                fastObjTexture* tex = &obj->textures[mtl->map_d];
                if (tex->path) {
                    char* path = resolve_texture_path(tex->path, "assets/textures/");
                    mat->alpha_map = texture_load(path);
                }
            }
        }
    }

    // Create one mesh per material
    model->mesh_count = obj->group_count > 0 ? obj->group_count : 1;
    model->meshes = (Mesh*)malloc(sizeof(Mesh) * model->mesh_count);
    memset(model->meshes, 0, sizeof(Mesh) * model->mesh_count);

    // Initialize bounding box
    model->min[0] = model->min[1] = model->min[2] = INFINITY;
    model->max[0] = model->max[1] = model->max[2] = -INFINITY;

    // Process each group (or create one default group)
    unsigned int mesh_idx = 0;
    for (unsigned int g = 0; g < (obj->group_count > 0 ? obj->group_count : 1); g++) {
        fastObjGroup* group = obj->group_count > 0 ? &obj->groups[g] : NULL;

        unsigned int face_start = group ? group->face_offset : 0;
        unsigned int face_count = group ? group->face_count : obj->face_count;

        if (face_count == 0) continue;

        // Count vertices for this mesh
        unsigned int vertex_count = 0;
        unsigned int index_count = 0;
        for (unsigned int f = face_start; f < face_start + face_count; f++) {
            unsigned int fv = obj->face_vertices[f];
            vertex_count += fv;
            index_count += (fv - 2) * 3; // Triangulate
        }

        if (vertex_count == 0) continue;

        // Allocate vertex data (interleaved: pos + normal + uv)
        float* vertex_data = (float*)malloc(sizeof(float) * vertex_count * 8);
        unsigned int* index_data = (unsigned int*)malloc(sizeof(unsigned int) * index_count);

        unsigned int v_offset = 0;
        unsigned int i_offset = 0;
        unsigned int base_vertex = 0;

        // Get material for this mesh
        unsigned int mat_idx = 0;
        if (obj->face_count > 0 && obj->face_materials) {
            mat_idx = obj->face_materials[face_start];
        }

        // Extract vertex data
        for (unsigned int f = face_start; f < face_start + face_count; f++) {
            unsigned int fv = obj->face_vertices[f];

            // Calculate correct index offset
            unsigned int index_base = 0;
            for (unsigned int ff = 0; ff < f; ff++) {
                index_base += obj->face_vertices[ff];
            }

            for (unsigned int v = 0; v < fv; v++) {
                fastObjIndex idx = obj->indices[index_base + v];

                // Position
                vertex_data[v_offset++] = obj->positions[idx.p * 3 + 0];
                vertex_data[v_offset++] = obj->positions[idx.p * 3 + 1];
                vertex_data[v_offset++] = obj->positions[idx.p * 3 + 2];

                // Update bounding box
                model->min[0] = fminf(model->min[0], obj->positions[idx.p * 3 + 0]);
                model->min[1] = fminf(model->min[1], obj->positions[idx.p * 3 + 1]);
                model->min[2] = fminf(model->min[2], obj->positions[idx.p * 3 + 2]);
                model->max[0] = fmaxf(model->max[0], obj->positions[idx.p * 3 + 0]);
                model->max[1] = fmaxf(model->max[1], obj->positions[idx.p * 3 + 1]);
                model->max[2] = fmaxf(model->max[2], obj->positions[idx.p * 3 + 2]);

                // Normal
                if (idx.n > 0 && obj->normals) {
                    vertex_data[v_offset++] = obj->normals[idx.n * 3 + 0];
                    vertex_data[v_offset++] = obj->normals[idx.n * 3 + 1];
                    vertex_data[v_offset++] = obj->normals[idx.n * 3 + 2];
                } else {
                    vertex_data[v_offset++] = 0.0f;
                    vertex_data[v_offset++] = 1.0f;
                    vertex_data[v_offset++] = 0.0f;
                }

                // Texcoord
                if (idx.t > 0 && obj->texcoords) {
                    vertex_data[v_offset++] = obj->texcoords[idx.t * 2 + 0];
                    vertex_data[v_offset++] = obj->texcoords[idx.t * 2 + 1];
                } else {
                    vertex_data[v_offset++] = 0.0f;
                    vertex_data[v_offset++] = 0.0f;
                }
            }

            // Triangulate face (fan triangulation)
            for (unsigned int t = 2; t < fv; t++) {
                index_data[i_offset++] = base_vertex;
                index_data[i_offset++] = base_vertex + t - 1;
                index_data[i_offset++] = base_vertex + t;
            }

            base_vertex += fv;
        }

        // Create OpenGL buffers
        Mesh* mesh = &model->meshes[mesh_idx];
        mesh->vertex_count = vertex_count;
        mesh->index_count = index_count;
        mesh->material = (mat_idx < model->material_count) ? &model->materials[mat_idx] : NULL;

        glGenVertexArrays(1, &mesh->vao);
        glGenBuffers(1, &mesh->vbo);
        glGenBuffers(1, &mesh->ibo);

        glBindVertexArray(mesh->vao);

        // Upload vertex data
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * 8 * sizeof(float), vertex_data, GL_STATIC_DRAW);

        // Set vertex attributes
        // Position (location 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

        // Normal (location 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

        // Texcoord (location 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        // Upload index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), index_data, GL_STATIC_DRAW);

        glBindVertexArray(0);

        free(vertex_data);
        free(index_data);

        mesh_idx++;
    }

    // Adjust mesh count if some were skipped
    model->mesh_count = mesh_idx;

    fast_obj_destroy(obj);

    printf("Model loaded: %s (%u meshes, %u materials)\n",
           model->name, model->mesh_count, model->material_count);

    return model;
}

void model_free(Model* model) {
    if (!model) return;

    // Free meshes
    for (unsigned int i = 0; i < model->mesh_count; i++) {
        Mesh* mesh = &model->meshes[i];
        glDeleteVertexArrays(1, &mesh->vao);
        glDeleteBuffers(1, &mesh->vbo);
        glDeleteBuffers(1, &mesh->ibo);
    }
    free(model->meshes);

    // Free materials
    for (unsigned int i = 0; i < model->material_count; i++) {
        material_cleanup(&model->materials[i]);
    }
    free(model->materials);

    free(model);
}
