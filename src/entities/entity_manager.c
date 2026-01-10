#include "entity_manager.h"
#include "../graphics/shader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_ENTITY_CAPACITY 32
#define INITIAL_MODEL_CAPACITY 16

EntityManager* entity_manager_create(void) {
    EntityManager* manager = (EntityManager*)malloc(sizeof(EntityManager));
    if (!manager) return NULL;

    // Initialize entities array
    manager->entities = (Entity*)malloc(sizeof(Entity) * INITIAL_ENTITY_CAPACITY);
    manager->entity_count = 0;
    manager->entity_capacity = INITIAL_ENTITY_CAPACITY;

    // Initialize models array
    manager->loaded_models = (Model**)malloc(sizeof(Model*) * INITIAL_MODEL_CAPACITY);
    manager->loaded_model_count = 0;
    manager->loaded_model_capacity = INITIAL_MODEL_CAPACITY;

    // Initialize shader (will be set later)
    manager->model_shader = 0;

    // Initialize entity ID counter
    manager->next_entity_id = 1;

    printf("EntityManager created\n");

    return manager;
}

Model* entity_manager_load_model(EntityManager* manager, const char* obj_path) {
    if (!manager || !obj_path) return NULL;

    // Check if model is already loaded
    for (unsigned int i = 0; i < manager->loaded_model_count; i++) {
        if (strcmp(manager->loaded_models[i]->filepath, obj_path) == 0) {
            printf("Model already loaded: %s\n", obj_path);
            return manager->loaded_models[i];
        }
    }

    // Load new model
    Model* model = model_load(obj_path);
    if (!model) {
        fprintf(stderr, "Failed to load model: %s\n", obj_path);
        return NULL;
    }

    // Expand model array if needed
    if (manager->loaded_model_count >= manager->loaded_model_capacity) {
        manager->loaded_model_capacity *= 2;
        manager->loaded_models = (Model**)realloc(manager->loaded_models,
                                                  sizeof(Model*) * manager->loaded_model_capacity);
    }

    // Add to cache
    manager->loaded_models[manager->loaded_model_count++] = model;

    return model;
}

Entity* entity_manager_create_entity(EntityManager* manager, EntityType type,
                                    Model* model, float pos[3], float rot[3], float scale[3]) {
    if (!manager) return NULL;

    // Expand entity array if needed
    if (manager->entity_count >= manager->entity_capacity) {
        manager->entity_capacity *= 2;
        manager->entities = (Entity*)realloc(manager->entities,
                                            sizeof(Entity) * manager->entity_capacity);
    }

    // Create entity
    Entity* entity = entity_create(manager->next_entity_id++, type, model, pos, rot, scale);
    if (!entity) return NULL;

    // Copy entity to array (since we're using a direct array, not pointers)
    memcpy(&manager->entities[manager->entity_count], entity, sizeof(Entity));
    Entity* stored_entity = &manager->entities[manager->entity_count];
    manager->entity_count++;

    // Free the temporary entity (we copied it)
    free(entity);

    return stored_entity;
}

void entity_manager_update(EntityManager* manager, float delta_time) {
    if (!manager) return;

    // Suppress unused parameter warning
    (void)delta_time;

    // Update logic for entities (physics, animation, etc.)
    // For now, this is a placeholder for future functionality
    // Example: Rotate entities slowly
    // for (unsigned int i = 0; i < manager->entity_count; i++) {
    //     Entity* entity = &manager->entities[i];
    //     entity->rotation[1] += delta_time * 0.5f; // Rotate around Y axis
    // }
}

void entity_manager_render(EntityManager* manager, float* view, float* proj,
                          float cam_x, float cam_y, float cam_z) {
    if (!manager || manager->model_shader == 0) return;

    // Render all entities
    for (unsigned int i = 0; i < manager->entity_count; i++) {
        Entity* entity = &manager->entities[i];
        entity_render(entity, manager->model_shader, view, proj, cam_x, cam_y, cam_z);
    }
}

void entity_manager_cleanup(EntityManager* manager) {
    if (!manager) return;

    printf("Cleaning up EntityManager...\n");

    // Free all loaded models
    for (unsigned int i = 0; i < manager->loaded_model_count; i++) {
        model_free(manager->loaded_models[i]);
    }
    free(manager->loaded_models);

    // Free entities array (entities themselves don't own models)
    free(manager->entities);

    // Delete shader
    if (manager->model_shader) {
        glDeleteProgram(manager->model_shader);
    }

    free(manager);

    printf("EntityManager cleaned up\n");
}
