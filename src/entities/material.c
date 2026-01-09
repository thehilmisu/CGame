#include "material.h"
#include <string.h>

Material material_create_default(void) {
    Material mat;

    // Default gray material
    mat.ambient[0] = 0.2f;
    mat.ambient[1] = 0.2f;
    mat.ambient[2] = 0.2f;

    mat.diffuse[0] = 0.8f;
    mat.diffuse[1] = 0.8f;
    mat.diffuse[2] = 0.8f;

    mat.specular[0] = 0.5f;
    mat.specular[1] = 0.5f;
    mat.specular[2] = 0.5f;

    mat.shininess = 32.0f;

    mat.diffuse_map = 0;
    mat.specular_map = 0;
    mat.normal_map = 0;
    mat.alpha_map = 0;

    strcpy(mat.name, "default");

    return mat;
}

void material_cleanup(Material* mat) {
    if (mat->diffuse_map) glDeleteTextures(1, &mat->diffuse_map);
    if (mat->specular_map) glDeleteTextures(1, &mat->specular_map);
    if (mat->normal_map) glDeleteTextures(1, &mat->normal_map);
    if (mat->alpha_map) glDeleteTextures(1, &mat->alpha_map);
}
