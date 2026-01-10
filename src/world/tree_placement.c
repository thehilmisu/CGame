#include "tree_placement.h"
#include "terrain.h"
#include "../entities/model.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

// Terrain height scale (same as used in terrain rendering)
#define TERRAIN_MAX_HEIGHT (HEIGHT * SCALE)

// Simple hash function for deterministic tree placement
static unsigned int hash_position(int x, int z, int seed) {
    unsigned int h = seed;
    h ^= (unsigned int)x * 73856093;
    h ^= (unsigned int)z * 19349663;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

// Convert hash to float [0, 1]
static float hash_to_float(unsigned int hash) {
    return (float)(hash & 0xFFFF) / 65536.0f;
}

// Check if a tree should be placed at this grid position
static bool should_place_tree(int grid_x, int grid_z, int seed, float density) {
    unsigned int h = hash_position(grid_x, grid_z, seed);
    return hash_to_float(h) < density;
}

// Get tree scale variation
static float get_tree_scale(int grid_x, int grid_z, int seed) {
    unsigned int h = hash_position(grid_x, grid_z, seed + 2);
    // Scale between 1.5 and 3.0
    return 1.5f + hash_to_float(h) * 3.0f;
}

// Get tree rotation (in radians)
static float get_tree_rotation(int grid_x, int grid_z, int seed) {
    unsigned int h = hash_position(grid_x, grid_z, seed + 3);
    // Random rotation 0-2*PI radians
    return hash_to_float(h) * 2.0f * 3.14159265358979323846f;
}

// Add small random offset to tree position (within grid cell)
static void get_tree_offset(int grid_x, int grid_z, int seed, float* offset_x, float* offset_z) {
    unsigned int h1 = hash_position(grid_x, grid_z, seed + 4);
    unsigned int h2 = hash_position(grid_x, grid_z, seed + 5);

    // Offset within +/- half grid size
    *offset_x = (hash_to_float(h1) - 0.5f) * TREE_GRID_SIZE * 0.8f;
    *offset_z = (hash_to_float(h2) - 0.5f) * TREE_GRID_SIZE * 0.8f;
}

TreePlacementManager* tree_placement_create(EntityManager* entity_manager,
                                           const TerrainSeed* terrain_seed,
                                           int placement_seed) {

    TreePlacementManager* manager = malloc(sizeof(TreePlacementManager));

    manager->entity_manager = entity_manager;
    manager->terrain_seed = terrain_seed;
    manager->placement_seed = placement_seed;
    // manager->last_camera_x = 0.0f;
    // manager->last_camera_z = 0.0f;

    // Load tree model from OBJ file
    printf("Loading tree model from OBJ...\n");
    manager->tree_model = model_load("assets/models/Tree.obj");
    
    if (manager->tree_model) {
        printf("Tree model loaded: %u meshes\n", manager->tree_model->mesh_count);
    } else {
        printf("ERROR: Failed to load tree model!\n");
    }

    printf("Tree placement system initialized\n");

    return manager;
}

void tree_placement_update(TreePlacementManager* manager, float camera_x, float camera_z) {
    if (!manager->tree_model) return;

    // Calculate which grid cells are in range
    int min_grid_x = (int)floor((camera_x - TREE_RENDER_RANGE) / TREE_GRID_SIZE);
    int max_grid_x = (int)ceil((camera_x + TREE_RENDER_RANGE) / TREE_GRID_SIZE);
    int min_grid_z = (int)floor((camera_z - TREE_RENDER_RANGE) / TREE_GRID_SIZE);
    int max_grid_z = (int)ceil((camera_z + TREE_RENDER_RANGE) / TREE_GRID_SIZE);

    int trees_spawned = 0;

    // Iterate through grid cells in range
    for (int grid_x = min_grid_x; grid_x <= max_grid_x; grid_x++) {
        for (int grid_z = min_grid_z; grid_z <= max_grid_z; grid_z++) {
            // Check if tree should exist at this position
            if (!should_place_tree(grid_x, grid_z, manager->placement_seed, TREE_PLACEMENT_DENSITY)) {
                continue;
            }

            // Calculate world position
            float world_x = (float)grid_x * TREE_GRID_SIZE;
            float world_z = (float)grid_z * TREE_GRID_SIZE;

            // Add random offset
            float offset_x, offset_z;
            get_tree_offset(grid_x, grid_z, manager->placement_seed, &offset_x, &offset_z);
            world_x += offset_x;
            world_z += offset_z;

            // Check distance to camera
            float dx = world_x - camera_x;
            float dz = world_z - camera_z;
            float dist_sq = dx * dx + dz * dz;

            // Don't spawn trees too close to camera (minimum 100 units away)
            if (dist_sq < 100.0f * 100.0f) {
                continue;
            }

            if (dist_sq > TREE_RENDER_RANGE * TREE_RENDER_RANGE) {
                continue;
            }

            // Get terrain height at this position (normalized -1 to 1)
            float height_normalized = terrain_get_height(world_x, world_z, manager->terrain_seed);
            
            // Convert to actual world height (same scaling as terrain shader)
            float world_y = height_normalized * TERRAIN_MAX_HEIGHT;
            
            // Apply same min height logic as terrain (from terrain.c)
            if (world_y <= 0.0f)
                world_y = fminf(-0.007f * TERRAIN_MAX_HEIGHT, world_y);
            else
                world_y = fmaxf(0.007f * TERRAIN_MAX_HEIGHT, world_y);

            // Don't place trees underwater (water is at y=0) or on very low terrain
            if (world_y < 10.0f) {
                continue;
            }

            // Check if tree already exists at this position
            bool already_exists = false;
            for (unsigned int i = 0; i < manager->entity_manager->entity_count; i++) {
                Entity* entity = &manager->entity_manager->entities[i];
                if (entity->type != ENTITY_TYPE_PROP) continue;

                float ex = entity->position[0];
                float ez = entity->position[2];
                float edx = ex - world_x;
                float edz = ez - world_z;

                // If there's an entity within 5 units, assume it's this tree
                if (edx * edx + edz * edz < 25.0f) {
                    already_exists = true;
                    break;
                }
            }

            if (already_exists) {
                continue;
            }

            // Limit spawning per frame to avoid lag
            if (trees_spawned >= MAX_TREES_PER_UPDATE) {
                continue;
            }
            
            // Limit total trees to prevent memory issues
            if (manager->entity_manager->entity_count > 200) {
                continue;
            }

            // Get tree properties
            float scale = get_tree_scale(grid_x, grid_z, manager->placement_seed);
            float rotation_y = get_tree_rotation(grid_x, grid_z, manager->placement_seed);

            // Create tree entity
            Entity* tree = entity_manager_create_entity(
                manager->entity_manager,
                ENTITY_TYPE_PROP,
                manager->tree_model,
                (float[]){world_x, world_y, world_z},
                (float[]){0.0f, rotation_y, 0.0f},
                (float[]){scale, scale, scale}
            );

            if (tree) {
                trees_spawned++;
            }
        }
    }

    manager->last_camera_x = camera_x;
    manager->last_camera_z = camera_z;
}

void tree_placement_cleanup(TreePlacementManager* manager) {
    if (!manager) return;

    // Free tree model
    if (manager->tree_model) {
        model_free(manager->tree_model);
    }

    free(manager);
}
