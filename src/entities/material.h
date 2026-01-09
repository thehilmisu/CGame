#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>

typedef struct {
    // Material properties from MTL
    float ambient[3];       // Ka
    float diffuse[3];       // Kd
    float specular[3];      // Ks
    float shininess;        // Ns

    // Texture maps
    GLuint diffuse_map;     // map_Kd
    GLuint specular_map;    // map_Ks
    GLuint normal_map;      // map_bump/bump
    GLuint alpha_map;       // map_d

    char name[64];
} Material;

// Create a default material
Material material_create_default(void);

// Cleanup material (delete textures)
void material_cleanup(Material* mat);

#endif // MATERIAL_H
