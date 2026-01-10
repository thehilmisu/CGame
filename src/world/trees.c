#include "trees.h"
#include "../math/math_ops.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FMAX(a, b) ((a) > (b) ? (a) : (b))
#define IMAX(a, b) ((a) > (b) ? (a) : (b))

// ============================================================================
// Branch Properties (transformation state for L-System interpretation)
// ============================================================================

typedef struct {
    float transform[16];  // Rotation and scale matrix
    float position[3];    // Current position in 3D space
    unsigned int depth;   // How far up the branch tree
} BranchProperties;

// Simple stack for branch properties
typedef struct {
    BranchProperties* data;
    int top;
    int capacity;
} BranchStack;

static BranchStack stack_create(int capacity) {
    BranchStack stack;
    stack.data = malloc(capacity * sizeof(BranchProperties));
    stack.top = -1;
    stack.capacity = capacity;
    return stack;
}

static void stack_free(BranchStack* stack) {
    free(stack->data);
    stack->data = NULL;
    stack->top = -1;
    stack->capacity = 0;
}

static void stack_push(BranchStack* stack, BranchProperties* props) {
    if (stack->top >= stack->capacity - 1) {
        stack->capacity *= 2;
        stack->data = realloc(stack->data, stack->capacity * sizeof(BranchProperties));
    }
    stack->top++;
    memcpy(&stack->data[stack->top], props, sizeof(BranchProperties));
}

static BranchProperties stack_pop(BranchStack* stack) {
    if (stack->top < 0) {
        BranchProperties empty;
        mat4_identity(empty.transform);
        empty.position[0] = empty.position[1] = empty.position[2] = 0.0f;
        empty.depth = 0;
        return empty;
    }
    return stack->data[stack->top--];
}

// ============================================================================
// L-System String Generation
// ============================================================================

char* lsystem_generate(unsigned int iterations, const char* axiom, const char* rule) {
    // Estimate maximum string length
    size_t axiom_len = strlen(axiom);
    size_t rule_len = strlen(rule);

    // Worst case: every F becomes the rule
    size_t max_len = axiom_len;
    for (unsigned int i = 0; i < iterations; i++) {
        max_len = max_len * rule_len;
    }

    char* result = malloc(max_len + 1);
    strcpy(result, axiom);

    for (unsigned int iter = 0; iter < iterations; iter++) {
        size_t current_len = strlen(result);
        char* temp = malloc(current_len * rule_len + 1);
        temp[0] = '\0';

        for (size_t i = 0; i < current_len; i++) {
            if (result[i] == 'F') {
                strcat(temp, rule);
            } else {
                size_t len = strlen(temp);
                temp[len] = result[i];
                temp[len + 1] = '\0';
            }
        }

        free(result);
        result = temp;
    }

    return result;
}

// ============================================================================
// Branch Mesh Generation
// ============================================================================

static ProceduralMesh create_branch_segment(BranchProperties* branch, float thickness,
                                   float decrease_amt, float length,
                                   unsigned int detail) {
    float radius1 = FMAX(thickness - decrease_amt * (float)branch->depth, 0.01f);
    float radius2 = FMAX(thickness - decrease_amt * (float)(branch->depth + 1), 0.01f);

    ProceduralMesh segment = mesh_create_frustum(detail, radius1, radius2);

    // Transform texture coordinates (bark texture is left half)
    float tc_transform[16];
    mat4_identity(tc_transform);
    mat4_translate(tc_transform, 0.01f, 0.0f, 0.0f);
    float scale_mat[16];
    mat4_identity(scale_mat);
    mat4_scale(scale_mat, 0.48f, 2.0f, 1.0f);
    float tc_final[16];
    mat4_multiply(tc_transform, scale_mat, tc_final);
    mesh_transform_texcoords(&segment, tc_final);

    // Scale to desired length (frustum has height 1.0 by default)
    float scale_height[16];
    mat4_identity(scale_height);
    mat4_scale(scale_height, 1.0f, length, 1.0f);

    // Combine transformations: translation * branch_transform * scale
    float translate[16];
    mat4_identity(translate);
    mat4_translate(translate, branch->position[0], branch->position[1], branch->position[2]);

    float temp[16], final_transform[16];
    mat4_multiply(branch->transform, scale_height, temp);
    mat4_multiply(translate, temp, final_transform);

    mesh_transform(&segment, final_transform);

    return segment;
}

static ProceduralMesh create_branch_end(BranchProperties* branch, float thickness,
                               float decrease_amt, float length,
                               unsigned int detail) {
    ProceduralMesh end_segment = mesh_create_cone_type1(detail);

    // Transform texture coordinates (bark)
    float tc_transform[16];
    mat4_identity(tc_transform);
    mat4_translate(tc_transform, 0.01f, 0.0f, 0.0f);
    float scale_mat[16];
    mat4_identity(scale_mat);
    mat4_scale(scale_mat, 0.48f, 2.0f, 1.0f);
    float tc_final[16];
    mat4_multiply(tc_transform, scale_mat, tc_final);
    mesh_transform_texcoords(&end_segment, tc_final);

    // Scale cone
    float radius = FMAX(thickness - decrease_amt * (float)branch->depth, 0.01f);
    float scale_cone[16];
    mat4_identity(scale_cone);
    mat4_scale(scale_cone, radius, length, radius);
    mesh_transform(&end_segment, scale_cone);

    // Transform to position
    float translate[16];
    mat4_identity(translate);
    mat4_translate(translate, branch->position[0], branch->position[1], branch->position[2]);

    float final_transform[16];
    mat4_multiply(translate, branch->transform, final_transform);
    mesh_transform(&end_segment, final_transform);

    // Add leaves
    float scale = 1.0f;
    if (branch->depth <= 2)
        scale = 0.5f;

    // Leaf texture coordinates (right half of texture)
    mat4_identity(tc_transform);
    mat4_translate(tc_transform, 0.51f, 0.0f, 0.0f);
    mat4_identity(scale_mat);
    mat4_scale(scale_mat, 0.48f, 1.0f, 1.0f);
    mat4_multiply(tc_transform, scale_mat, tc_final);

    int leaf_count = IMAX((2 * (int)detail) / 3 - 1, 1);
    int leaf_detail = IMAX((int)detail / 2 - 2, 0);

    for (int i = 0; i < leaf_count; i++) {
        ProceduralMesh leaves = mesh_create_plane(leaf_detail);
        mesh_transform_texcoords(&leaves, tc_final);

        // Calculate offset
        float offset_vec[3];
        offset_vec[0] = branch->transform[4] * length;
        offset_vec[1] = branch->transform[5] * length;
        offset_vec[2] = branch->transform[6] * length;

        offset_vec[0] *= (1.0f - scale);
        offset_vec[1] *= (1.0f - scale);
        offset_vec[2] *= (1.0f - scale);

        // Build leaf transformation
        float rotation[16];
        mat4_identity(rotation);
        mat4_rotate_y(rotation, (120.0f * M_PI / 180.0f) * (float)i);

        float scale_leaf[16];
        mat4_identity(scale_leaf);
        mat4_scale(scale_leaf, scale, scale, scale);

        float translate_up[16];
        mat4_identity(translate_up);
        mat4_translate(translate_up, 0.0f, 1.0f, 0.0f);

        float translate_pos[16];
        mat4_identity(translate_pos);
        mat4_translate(translate_pos,
                      branch->position[0] + offset_vec[0],
                      branch->position[1] + offset_vec[1],
                      branch->position[2] + offset_vec[2]);

        // Combine: translate_pos * branch_transform * rotation * scale * translate_up
        float temp1[16], temp2[16], temp3[16], temp4[16];
        mat4_multiply(rotation, scale_leaf, temp1);
        mat4_multiply(temp1, translate_up, temp2);
        mat4_multiply(branch->transform, temp2, temp3);
        mat4_multiply(translate_pos, temp3, temp4);

        mesh_transform(&leaves, temp4);

        ProceduralMesh merged = mesh_merge(&end_segment, &leaves);
        mesh_free(&end_segment);
        mesh_free(&leaves);
        end_segment = merged;
    }

    return end_segment;
}

// ============================================================================
// L-System String Interpreter
// ============================================================================

Model* tree_create_from_string(const char* str, float angle, float length,
                                float thickness, float decrease_amt,
                                unsigned int detail) {
    // Rotation matrices for each command
    float rot_x_pos[16], rot_x_neg[16];
    float rot_y_pos[16];
    float rot_z_pos[16], rot_z_neg[16];

    mat4_identity(rot_x_pos);
    mat4_rotate_x(rot_x_pos, angle);

    mat4_identity(rot_x_neg);
    mat4_rotate_x(rot_x_neg, -angle);

    mat4_identity(rot_y_pos);
    mat4_rotate_y(rot_y_pos, angle);

    mat4_identity(rot_z_pos);
    mat4_rotate_z(rot_z_pos, angle);

    mat4_identity(rot_z_neg);
    mat4_rotate_z(rot_z_neg, -angle);

    // Initialize branch stack
    BranchStack branch_stack = stack_create(64);

    // Current branch state
    BranchProperties branch;
    mat4_identity(branch.transform);
    branch.position[0] = branch.position[1] = branch.position[2] = 0.0f;
    branch.depth = 0;

    // Accumulate meshes
    ProceduralMesh plant = mesh_create(1024);
    size_t str_len = strlen(str);

    for (size_t i = 0; i < str_len; i++) {
        char ch = str[i];
        ProceduralMesh tree_part;
        bool has_part = false;

        // Check for rotation commands
        if (ch == '<') {
            float temp[16];
            mat4_multiply(branch.transform, rot_x_pos, temp);
            memcpy(branch.transform, temp, sizeof(float) * 16);
            continue;
        } else if (ch == '>') {
            float temp[16];
            mat4_multiply(branch.transform, rot_x_neg, temp);
            memcpy(branch.transform, temp, sizeof(float) * 16);
            continue;
        } else if (ch == '&') {
            float temp[16];
            mat4_multiply(branch.transform, rot_y_pos, temp);
            memcpy(branch.transform, temp, sizeof(float) * 16);
            continue;
        } else if (ch == '+') {
            float temp[16];
            mat4_multiply(branch.transform, rot_z_pos, temp);
            memcpy(branch.transform, temp, sizeof(float) * 16);
            continue;
        } else if (ch == '-') {
            float temp[16];
            mat4_multiply(branch.transform, rot_z_neg, temp);
            memcpy(branch.transform, temp, sizeof(float) * 16);
            continue;
        }

        switch (ch) {
        case 'F':
            // Check if this is a terminal branch (end of string or before ])
            if (i == str_len - 1 || str[i + 1] == ']') {
                tree_part = create_branch_end(&branch, thickness, decrease_amt, length, detail);
            } else {
                tree_part = create_branch_segment(&branch, thickness, decrease_amt, length, detail);
            }
            has_part = true;

            // Update position (move forward along Y in local space)
            branch.position[0] += branch.transform[4] * length;
            branch.position[1] += branch.transform[5] * length;
            branch.position[2] += branch.transform[6] * length;
            branch.depth++;
            break;

        case '[':
            stack_push(&branch_stack, &branch);
            break;

        case ']':
            branch = stack_pop(&branch_stack);
            break;

        default:
            break;
        }

        // Merge the part into the plant
        if (has_part) {
            ProceduralMesh merged = mesh_merge(&plant, &tree_part);
            mesh_free(&plant);
            mesh_free(&tree_part);
            plant = merged;
        }
    }

    stack_free(&branch_stack);

    printf("Tree mesh generated: %u vertices, %u indices\n", plant.vertex_count, plant.index_count);

    // Convert mesh to Model
    Model* model = malloc(sizeof(Model));
    model->mesh_count = 1;
    model->meshes = malloc(sizeof(Mesh));

    // Create single mesh with the plant data
    Mesh* final_mesh = &model->meshes[0];

    // Generate VAO/VBO/IBO
    glGenVertexArrays(1, &final_mesh->vao);
    glGenBuffers(1, &final_mesh->vbo);
    glGenBuffers(1, &final_mesh->ibo);

    glBindVertexArray(final_mesh->vao);

    // Interleave vertex data: pos(3) + normal(3) + texcoord(2) = 8 floats per vertex
    float* interleaved = malloc(plant.vertex_count * 8 * sizeof(float));
    for (unsigned int i = 0; i < plant.vertex_count; i++) {
        interleaved[i * 8 + 0] = plant.vertices[i * 3 + 0];
        interleaved[i * 8 + 1] = plant.vertices[i * 3 + 1];
        interleaved[i * 8 + 2] = plant.vertices[i * 3 + 2];
        interleaved[i * 8 + 3] = plant.normals[i * 3 + 0];
        interleaved[i * 8 + 4] = plant.normals[i * 3 + 1];
        interleaved[i * 8 + 5] = plant.normals[i * 3 + 2];
        interleaved[i * 8 + 6] = plant.texcoords[i * 2 + 0];
        interleaved[i * 8 + 7] = plant.texcoords[i * 2 + 1];
    }

    glBindBuffer(GL_ARRAY_BUFFER, final_mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, plant.vertex_count * 8 * sizeof(float),
                 interleaved, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, final_mesh->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, plant.index_count * sizeof(unsigned int),
                 plant.indices, GL_STATIC_DRAW);

    // Set attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    free(interleaved);

    final_mesh->vertex_count = plant.vertex_count;
    final_mesh->index_count = plant.index_count;
    final_mesh->material = NULL;  // Will be set by caller

    mesh_free(&plant);

    // Initialize model metadata
    model->material_count = 0;
    model->materials = NULL;
    strcpy(model->name, "procedural_tree");
    strcpy(model->filepath, "");

    // Bounding box (approximate)
    model->min[0] = model->min[1] = model->min[2] = -5.0f;
    model->max[0] = model->max[1] = model->max[2] = 5.0f;

    return model;
}

// ============================================================================
// Predefined Tree Generators
// ============================================================================

Model* tree_create_generic(unsigned int detail) {
    const float ANGLE = 30.0f * M_PI / 180.0f;
    const float LENGTH = 0.8f;
    const float THICKNESS = 0.15f;
    const float DECREASE = 0.025f;
    const unsigned int ITERATIONS = 2;
    const char* RULE = "F[&&>F]F[--F][&&&&-F][&&&&&&&&-F]";

    char* str = lsystem_generate(ITERATIONS, "F", RULE);
    Model* tree_model = tree_create_from_string(str, ANGLE, LENGTH, THICKNESS, DECREASE, detail);
    free(str);

    // Add trunk base if detail is high enough
    if (detail > 4) {
        ProceduralMesh trunk_base = mesh_create_frustum(detail, THICKNESS, THICKNESS);

        float translate[16];
        mat4_identity(translate);
        mat4_translate(translate, 0.0f, -1.0f, 0.0f);
        mesh_transform(&trunk_base, translate);

        // Transform texture coordinates
        float tc_transform[16];
        mat4_identity(tc_transform);
        mat4_translate(tc_transform, 0.01f, 0.0f, 0.0f);
        float scale_mat[16];
        mat4_identity(scale_mat);
        mat4_scale(scale_mat, 0.48f, 8.0f / 5.0f, 1.0f);
        float tc_final[16];
        mat4_multiply(tc_transform, scale_mat, tc_final);
        mesh_transform_texcoords(&trunk_base, tc_final);

        // Merge with existing tree (would need to rebuild model)
        // For simplicity, we'll skip this for now
        mesh_free(&trunk_base);
    }

    return tree_model;
}

Model* tree_create_pine(unsigned int detail) {
    ProceduralMesh pine_tree = mesh_create(1024);

    // Generate trunk (cone)
    ProceduralMesh trunk = mesh_create_cone_type1(detail);
    float scale_trunk[16];
    mat4_identity(scale_trunk);
    mat4_scale(scale_trunk, 0.3f, 5.0f, 0.3f);
    mesh_transform(&trunk, scale_trunk);

    // Transform texture coordinates
    float tc_transform[16];
    mat4_identity(tc_transform);
    mat4_scale(tc_transform, 0.5f, 8.0f, 1.0f);
    mesh_transform_texcoords(&trunk, tc_transform);

    pine_tree = mesh_merge(&pine_tree, &trunk);
    mesh_free(&trunk);

    // Add trunk base if detail > 4
    if (detail > 4) {
        ProceduralMesh bottom = mesh_create_frustum(detail, 0.3f, 0.3f);
        float translate[16];
        mat4_identity(translate);
        mat4_translate(translate, 0.0f, -1.0f, 0.0f);
        mesh_transform(&bottom, translate);

        mat4_identity(tc_transform);
        mat4_translate(tc_transform, 0.01f, 0.0f, 0.0f);
        float scale_mat[16];
        mat4_identity(scale_mat);
        mat4_scale(scale_mat, 0.48f, 8.0f / 5.0f, 1.0f);
        float tc_final[16];
        mat4_multiply(tc_transform, scale_mat, tc_final);
        mesh_transform_texcoords(&bottom, tc_final);

        ProceduralMesh merged = mesh_merge(&pine_tree, &bottom);
        mesh_free(&pine_tree);
        mesh_free(&bottom);
        pine_tree = merged;
    }

    // Generate foliage (stacked cones)
    float top_y = 5.0f;
    mat4_identity(tc_transform);
    mat4_translate(tc_transform, 0.51f, 0.02f, 0.0f);
    float scale_mat[16];
    mat4_identity(scale_mat);
    mat4_scale(scale_mat, 0.48f, 0.96f, 1.0f);
    float tc_final[16];
    mat4_multiply(tc_transform, scale_mat, tc_final);

    for (int i = 0; i < 4; i++) {
        ProceduralMesh part = mesh_create_cone_type2(detail);
        float scale = 0.75f + 0.25f * powf(1.5f, (float)i);
        float height = 1.6f + 0.2f * (float)i;

        float transform[16];
        mat4_identity(transform);
        mat4_translate(transform, 0.0f, top_y - height, 0.0f);

        float scale_part[16];
        mat4_identity(scale_part);
        mat4_scale(scale_part, scale, height, scale);

        float rotation[16];
        mat4_identity(rotation);
        mat4_rotate_y(rotation, (M_PI / 16.0f) * (float)i);

        float temp1[16], temp2[16];
        mat4_multiply(transform, rotation, temp1);
        mat4_multiply(temp1, scale_part, temp2);

        mesh_transform(&part, temp2);
        mesh_transform_texcoords(&part, tc_final);

        ProceduralMesh merged = mesh_merge(&pine_tree, &part);
        mesh_free(&pine_tree);
        mesh_free(&part);
        pine_tree = merged;

        top_y -= scale * 0.6f;
    }

    printf("Pine tree mesh generated: %u vertices, %u indices\n", pine_tree.vertex_count, pine_tree.index_count);

    // Convert to Model
    Model* model = malloc(sizeof(Model));
    model->mesh_count = 1;
    model->meshes = malloc(sizeof(Mesh));

    Mesh* final_mesh = &model->meshes[0];

    glGenVertexArrays(1, &final_mesh->vao);
    glGenBuffers(1, &final_mesh->vbo);
    glGenBuffers(1, &final_mesh->ibo);

    glBindVertexArray(final_mesh->vao);

    // Interleave vertex data
    float* interleaved = malloc(pine_tree.vertex_count * 8 * sizeof(float));
    for (unsigned int i = 0; i < pine_tree.vertex_count; i++) {
        interleaved[i * 8 + 0] = pine_tree.vertices[i * 3 + 0];
        interleaved[i * 8 + 1] = pine_tree.vertices[i * 3 + 1];
        interleaved[i * 8 + 2] = pine_tree.vertices[i * 3 + 2];
        interleaved[i * 8 + 3] = pine_tree.normals[i * 3 + 0];
        interleaved[i * 8 + 4] = pine_tree.normals[i * 3 + 1];
        interleaved[i * 8 + 5] = pine_tree.normals[i * 3 + 2];
        interleaved[i * 8 + 6] = pine_tree.texcoords[i * 2 + 0];
        interleaved[i * 8 + 7] = pine_tree.texcoords[i * 2 + 1];
    }

    glBindBuffer(GL_ARRAY_BUFFER, final_mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, pine_tree.vertex_count * 8 * sizeof(float),
                 interleaved, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, final_mesh->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, pine_tree.index_count * sizeof(unsigned int),
                 pine_tree.indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    free(interleaved);

    final_mesh->vertex_count = pine_tree.vertex_count;
    final_mesh->index_count = pine_tree.index_count;
    final_mesh->material = NULL;

    mesh_free(&pine_tree);

    model->material_count = 0;
    model->materials = NULL;
    strcpy(model->name, "pine_tree");
    strcpy(model->filepath, "");
    model->min[0] = model->min[1] = model->min[2] = -5.0f;
    model->max[0] = model->max[1] = model->max[2] = 5.0f;

    return model;
}
