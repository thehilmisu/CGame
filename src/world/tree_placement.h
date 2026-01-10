#ifndef TREE_PLACEMENT_H
#define TREE_PLACEMENT_H

#include "../entities/entity_manager.h"
#include "terrain.h"

// Tree placement configuration
#define TREE_GRID_SIZE 80.0f       // Space between potential tree positions
#define TREE_PLACEMENT_DENSITY 0.2f // Probability of tree at each grid point (0-1)
#define TREE_RENDER_RANGE 400.0f   // Maximum distance to render trees
#define MAX_TREES_PER_UPDATE 5     // Limit trees spawned per frame

// Tree placement manager
typedef struct {
    EntityManager* entity_manager;
    const TerrainSeed* terrain_seed;
    Model* tree_model;  // Loaded from OBJ file

    // Track camera position to spawn/despawn trees
    float last_camera_x;
    float last_camera_z;

    // Seed for deterministic placement
    int placement_seed;
} TreePlacementManager;

// Initialize tree placement system
TreePlacementManager* tree_placement_create(EntityManager* entity_manager,
                                           const TerrainSeed* terrain_seed,
                                           int placement_seed);

// Update trees based on camera position (spawn/despawn)
void tree_placement_update(TreePlacementManager* manager, float camera_x, float camera_z);

// Cleanup tree placement manager
void tree_placement_cleanup(TreePlacementManager* manager);

#endif // TREE_PLACEMENT_H
