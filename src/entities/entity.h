#ifndef ENTITY_H
#define ENTITY_H

#include "model.h"
#include <stdbool.h>
#include <glad/glad.h>

typedef enum {
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_PROP,
    ENTITY_TYPE_DYNAMIC,
    ENTITY_TYPE_PARTICLE
} EntityType;

typedef struct {
    unsigned int id;
    EntityType type;

    // Transform
    float position[3];
    float rotation[3];    // Euler angles (yaw, pitch, roll)
    float scale[3];

    // Rendering
    Model* model;         // Shared reference
    bool visible;

    // Optional: physics, animation state can be added later
} Entity;

// Create entity
Entity* entity_create(unsigned int id, EntityType type, Model* model,
                     float pos[3], float rot[3], float scale[3]);

// Get transform matrix for entity
void entity_get_transform_matrix(Entity* entity, float* out_matrix);

// Render entity
void entity_render(Entity* entity, GLuint shader, float* view, float* proj,
                  float cam_x, float cam_y, float cam_z);

// Free entity
void entity_free(Entity* entity);

#endif // ENTITY_H
