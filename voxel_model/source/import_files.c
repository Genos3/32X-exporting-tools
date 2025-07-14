#include <shlwapi.h>
#include "common.h"

int read_vox_header(FILE *file);
void read_size_chunk(FILE *file, vx_model_t *model);
void read_xyzi_chunk(FILE *file, vx_model_t *model);
void read_rgba_chunk(FILE *file, vx_model_t *model);
void skip_chunk(FILE *file, u32 chunk_size);
void copy_default_palette(vx_model_t *model);

#define PRINT_OUTPUT 0

void load_model(char file_path[], vx_model_t *model) {
  // char path[MAX_PATH];
  // strncpy(path, file_path, MAX_PATH - 1);
  FILE *file = fopen(file_path, "rb");
  
  if (!file) {
    PathStripPath(file_path);
    printf("Failed to open file: %s\n", file_path);
    return;
  }
  
  if (!read_vox_header(file)) {
    fclose(file);
    return;
  }
  
  char chunk_id[5] = {0};
  u32 chunk_size, child_chunk_size;
  
  fseek(file, 12, SEEK_CUR); // skip the MAIN section
  
  while (fread(chunk_id, 4, 1, file)) {
    fread(&chunk_size, sizeof(int), 1, file);
    fread(&child_chunk_size, sizeof(int), 1, file);
    
    if (!strcmp(chunk_id, "SIZE")) {
      read_size_chunk(file, model);
    }
    else if (!strcmp(chunk_id, "XYZI")) {
      read_xyzi_chunk(file, model);
    }
    else if (!strcmp(chunk_id, "RGBA")) {
      read_rgba_chunk(file, model);
    } else {
      skip_chunk(file, chunk_size);
    }
  }
  
  if (!model->pal_size) {
    copy_default_palette(model);
  }
  
  fclose(file);
}

int read_vox_header(FILE *file) {
  char name[5] = {0};
  u8 version;
  
  fread(&name, 4, 1, file);
  if (strcmp(name, "VOX ")) {
    printf("Invalid VOX file!\n");
    return 0;
  }
  
  fread(&version, sizeof(int), 1, file);
  if (version != 150) {
    printf("Unsupported VOX version: %d\n", version);
    return 0;
  }
  
  return 1;
}

// read the SIZE chunk

void read_size_chunk(FILE *file, vx_model_t *model) {
  fread(&model->size_i.w, sizeof(int), 1, file);
  fread(&model->size_i.d, sizeof(int), 1, file);
  fread(&model->size_i.h, sizeof(int), 1, file);
  
  printf("Model size: %dx%dx%d\n", model->size_i.w, model->size_i.d, model->size_i.h);
}

// read the XYZI chunk

void read_xyzi_chunk(FILE *file, vx_model_t *model) {
  int num_voxels;
  voxel_t voxel;
  
  // Read number of voxels
  fread(&num_voxels, sizeof(u32), 1, file);
  printf("Number of voxels: %u\n", num_voxels);
  
  model->num_voxels = num_voxels;
  model->length = model->size_i.w * model->size_i.d * model->size_i.h;
  model->voxels = calloc(model->length, sizeof(*model->voxels));
  
  // Read the voxel data
  for (u32 i = 0; i < num_voxels; i++) {
    fread(&voxel, sizeof(voxel_t), 1, file);
    
    int vx_pnt = voxel.z * model->size_i.d * model->size_i.w + voxel.y * model->size_i.w + voxel.x;
    
    model->voxels[vx_pnt].color_index = voxel.color_index;
    
    #if PRINT_OUTPUT
      printf("Voxel %d: x = %d, y = %d, z = %d, color_index = %d\n", i, voxel.x, voxel.y, voxel.z, voxel.color_index);
    #endif
  }
}

// read the RGBA palette chunk

void read_rgba_chunk(FILE *file, vx_model_t *model) {
  // Initialize the palette
  
  // memset(model->palette, 0, PALETTE_SIZE * sizeof(u16));
  
  model->palette[0] = 0;
  
  for (int i = 1; i < PALETTE_SIZE; i++) {
    color_t color_rgba;
    fread(&color_rgba, sizeof(color_t), 1, file);
    
    model->palette[i] = ((color_rgba.b >> 3) << 10) | ((color_rgba.g >> 3) << 5) | (color_rgba.r >> 3);
    
    #if PRINT_OUTPUT
      printf("Color %d: R = %d, G = %d, B = %d, A = %d\n", i, color_rgba.r, color_rgba.g, color_rgba.b, color_rgba.a);
    #endif
  }
  
  model->pal_size = PALETTE_SIZE;
}

// skip the chunk

void skip_chunk(FILE *file, u32 chunk_size) {
  fseek(file, chunk_size, SEEK_CUR);
}

void copy_default_palette(vx_model_t *model) {
  for (int i = 0; i < PALETTE_SIZE; i++) {
    u8 b = (default_palette[i] >> 16) & 255;
    u8 g = (default_palette[i] >> 8) & 255;
    u8 r = default_palette[i] & 255;
    
    model->palette[i] = ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
  }
  
  model->pal_size = PALETTE_SIZE;
}