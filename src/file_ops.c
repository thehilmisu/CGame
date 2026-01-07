#include "file_ops.h"

char* load_shader_source(const char* path){
  FILE* file = fopen(path, "rb");
  if (!file) return NULL;

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Allocate exact size + 1 for null terminator
  char* buffer = malloc(length + 1);
  if (buffer) {
      fread(buffer, 1, length, file);
      buffer[length] = '\0';
  }

  fclose(file);
  return buffer; // Caller is responsible for free()
}
