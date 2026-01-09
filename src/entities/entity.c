#include "entity.h"
#include "../math/math_ops.h"
#include <stdlib.h>
#include <string.h>

Entity* entity_create(unsigned int id, EntityType type, Model* model,
                     float pos[3], float rot[3], float scale[3]) {
    Entity* entity = (Entity*)malloc(sizeof(Entity));
    if (!entity) return NULL;

    entity->id = id;
    entity->type = type;
    entity->model = model;
    entity->visible = true;

    memcpy(entity->position, pos, sizeof(float) * 3);
    memcpy(entity->rotation, rot, sizeof(float) * 3);
    memcpy(entity->scale, scale, sizeof(float) * 3);

    return entity;
}

void entity_get_transform_matrix(Entity* entity, float* out_matrix) {
    // Create individual transformation matrices
    float scale_mat[16], rot_x[16], rot_y[16], rot_z[16], trans_mat[16];
    float temp1[16], temp2[16], temp3[16];

    // 1. Scale matrix
    mat4_scale(scale_mat, entity->scale[0], entity->scale[1], entity->scale[2]);

    // 2. Rotation matrices (apply in Z * Y * X order)
    mat4_rotate_x(rot_x, entity->rotation[0]);
    mat4_rotate_y(rot_y, entity->rotation[1]);
    mat4_rotate_z(rot_z, entity->rotation[2]);

    // 3. Translation matrix
    mat4_translate(trans_mat, entity->position[0], entity->position[1], entity->position[2]);

    // 4. Combine: T * Rz * Ry * Rx * S
    mat4_multiply(temp1, rot_x, scale_mat);      // Rx * S
    mat4_multiply(temp2, rot_y, temp1);          // Ry * (Rx * S)
    mat4_multiply(temp3, rot_z, temp2);          // Rz * (Ry * Rx * S)
    mat4_multiply(out_matrix, trans_mat, temp3); // T * (Rz * Ry * Rx * S)
}

void entity_render(Entity* entity, GLuint shader, float* view, float* proj,
                  float cam_x, float cam_y, float cam_z) {
    if (!entity->visible || !entity->model) return;

    // Calculate transform matrix
    float transform[16];
    entity_get_transform_matrix(entity, transform);

    // Set common uniforms
    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "persp"), 1, GL_FALSE, proj);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(shader, "transform"), 1, GL_FALSE, transform);

    // Set light direction (normalized diagonal from top-right-front)
    glUniform3f(glGetUniformLocation(shader, "lightdir"), -0.57735f, -0.57735f, -0.57735f);
    glUniform3f(glGetUniformLocation(shader, "camerapos"), cam_x, cam_y, cam_z);

    // Render each mesh in the model
    for (unsigned int i = 0; i < entity->model->mesh_count; i++) {
        Mesh* mesh = &entity->model->meshes[i];
        Material* mat = mesh->material;

        if (!mat) {
            // Use default material if none specified
            Material default_mat = material_create_default();
            mat = &default_mat;
        }

        // Set material uniforms
        glUniform3fv(glGetUniformLocation(shader, "material_ambient"), 1, mat->ambient);
        glUniform3fv(glGetUniformLocation(shader, "material_diffuse"), 1, mat->diffuse);
        glUniform3fv(glGetUniformLocation(shader, "material_specular"), 1, mat->specular);
        glUniform1f(glGetUniformLocation(shader, "material_shininess"), mat->shininess);

        // Bind diffuse texture
        if (mat->diffuse_map) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mat->diffuse_map);
            glUniform1i(glGetUniformLocation(shader, "diffuse_map"), 0);
            glUniform1i(glGetUniformLocation(shader, "has_diffuse_map"), 1);
        } else {
            glUniform1i(glGetUniformLocation(shader, "has_diffuse_map"), 0);
        }

        // Bind specular texture
        if (mat->specular_map) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mat->specular_map);
            glUniform1i(glGetUniformLocation(shader, "specular_map"), 1);
            glUniform1i(glGetUniformLocation(shader, "has_specular_map"), 1);
        } else {
            glUniform1i(glGetUniformLocation(shader, "has_specular_map"), 0);
        }

        // Bind VAO and draw
        glBindVertexArray(mesh->vao);
        glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
    }

    // Unbind textures to prevent them from affecting subsequent rendering
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glBindVertexArray(0);
}

void entity_free(Entity* entity) {
    if (!entity) return;
    // Note: We don't free the model here as it's a shared reference
    // The EntityManager will handle model cleanup
    free(entity);
}
