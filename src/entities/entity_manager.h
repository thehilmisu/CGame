#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

#include "entity.h"

typedef struct {
    Entity* entities;
    unsigned int entity_count;
    unsigned int entity_capacity;

    // Model cache (shared models)
    Model** loaded_models;
    unsigned int loaded_model_count;
    unsigned int loaded_model_capacity;

    // Shader for model rendering
    GLuint model_shader;

    // Next entity ID
    unsigned int next_entity_id;
} EntityManager;

// Create entity manager
EntityManager* entity_manager_create(void);

// Load a model (caches it if not already loaded)
Model* entity_manager_load_model(EntityManager* manager, const char* obj_path);

// Create entity and add to manager
Entity* entity_manager_create_entity(EntityManager* manager, EntityType type,
                                    Model* model, float pos[3], float rot[3], float scale[3]);

// Update all entities
void entity_manager_update(EntityManager* manager, float delta_time);

// Render all entities
void entity_manager_render(EntityManager* manager, float* view, float* proj,
                          float cam_x, float cam_y, float cam_z);

// Cleanup entity manager
void entity_manager_cleanup(EntityManager* manager);

#endif // ENTITYMANAGER_H
