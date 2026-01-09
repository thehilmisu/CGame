#ifndef MODEL_H
#define MODEL_H

#include "material.h"
#include <GLFW/glfw3.h>

typedef struct {
  GLuint vao;
  GLuint vbo;           // Interleaved: position, normal, texcoord
  GLuint ibo;


  unsigned int vertex_count;
  unsigned int index_count;

  Material* material;
}Mesh;  

typedef struct {
    Mesh* meshes;
    unsigned int mesh_count;

    Material* materials;
    unsigned int material_count;

    char name[128];
    char filepath[256];

    // For bounding boxes
    float min[3];
    float max[3];
} Model;

// Load model from OBJ file
Model* model_load(const char* obj_path);

// Free model and all its resources
void model_free(Model* model);

#endif // MODEL_H
