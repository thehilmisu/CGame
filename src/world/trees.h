#ifndef TREES_H
#define TREES_H

#include "mesh_utils.h"
#include "../entities/model.h"

// L-System based procedural plant generation
// Based on: https://ilchoi.weebly.com/procedural-tree-generator.html
//
// L-System commands:
//   F = grow forward (create branch segment)
//   + = rotate along Z axis (positive)
//   - = rotate along Z axis (negative)
//   & = rotate along Y axis (positive)
//   < = rotate along X axis (positive)
//   > = rotate along X axis (negative)
//   [ = push transformation state
//   ] = pop transformation state

// Generate L-System string
// iterations: number of times to apply the rule
// axiom: starting string (usually "F")
// rule: replacement rule for 'F'
char* lsystem_generate(unsigned int iterations, const char* axiom, const char* rule);

// Create a plant model from L-System string
// str: L-System command string
// angle: rotation angle in radians for each rotation command
// length: length of each branch segment
// thickness: starting thickness at the base
// decrease_amt: how much thickness decreases per depth level
// detail: polygon detail for meshes (higher = more vertices)
Model* tree_create_from_string(const char* str, float angle, float length,
                                float thickness, float decrease_amt,
                                unsigned int detail);

// Predefined tree generators
Model* tree_create_generic(unsigned int detail);  // Branching deciduous tree
Model* tree_create_pine(unsigned int detail);     // Conical pine tree

#endif // TREES_H
