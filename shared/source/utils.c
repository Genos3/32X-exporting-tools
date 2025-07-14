#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"
#include "structs.h"
#include "globals.h"
#include "utils.h"

void memset16(void *dst, s16 val, uint count) {
  s16 *dst_r = dst;
  
  for (int i = 0; i < count; i++) {
    *dst_r++ = val;
  }
}

void memset32(void *dst, s32 val, uint count) {
  s32 *dst_r = dst;
  
  for (int i = 0; i < count; i++) {
    *dst_r++ = val;
  }
}

void memcpy16(void *dst, const void *src, uint count) {
  s16 *dst_r = dst;
  const s16 *src_r = src;
  
  for (int i = 0; i < count; i++) {
    *dst_r++ = *src_r++;
  }
}

void memcpy32(void *dst, const void *src, uint count) {
  s32 *dst_r = dst;
  const s32 *src_r = src;
  
  for (int i = 0; i < count; i++) {
    *dst_r++ = *src_r++;
  }
}

// transfer the face from the model to the poly_t struct

void set_poly(int face_id, poly_t *poly, model_t *model) {
  poly->num_vertices = model->face_num_vertices.data[face_id];
  
  for (int i = 0; i < poly->num_vertices; i++) {
    int pt = model->faces.data[model->face_index.data[face_id] + i];
    poly->vertices[i] = model->vertices.data[pt];
  }
}

void make_poly_aabb(aabb_t *aabb, poly_t *poly) {
  aabb->min.x = poly->vertices[0].x;
  aabb->max.x = poly->vertices[0].x;
  aabb->min.y = poly->vertices[0].y;
  aabb->max.y = poly->vertices[0].y;
  aabb->min.z = poly->vertices[0].z;
  aabb->max.z = poly->vertices[0].z;
  
  for (int i = 1; i < poly->num_vertices; i++) {
    aabb->min.x = min_c(aabb->min.x, poly->vertices[i].x);
    aabb->max.x = max_c(aabb->max.x, poly->vertices[i].x);
    aabb->min.y = min_c(aabb->min.y, poly->vertices[i].y);
    aabb->max.y = max_c(aabb->max.y, poly->vertices[i].y);
    aabb->min.z = min_c(aabb->min.z, poly->vertices[i].z);
    aabb->max.z = max_c(aabb->max.z, poly->vertices[i].z);
  }
}

void make_model_aabb(aabb_t *aabb, model_t *model) {
  aabb->min.x = model->vertices.data[0].x;
  aabb->max.x = model->vertices.data[0].x;
  aabb->min.y = model->vertices.data[0].y;
  aabb->max.y = model->vertices.data[0].y;
  aabb->min.z = model->vertices.data[0].z;
  aabb->max.z = model->vertices.data[0].z;
  
  for (int i = 1; i < model->num_vertices; i++) {
    aabb->min.x = min_c(aabb->min.x, model->vertices.data[i].x);
    aabb->max.x = max_c(aabb->max.x, model->vertices.data[i].x);
    aabb->min.y = min_c(aabb->min.y, model->vertices.data[i].y);
    aabb->max.y = max_c(aabb->max.y, model->vertices.data[i].y);
    aabb->min.z = min_c(aabb->min.z, model->vertices.data[i].z);
    aabb->max.z = max_c(aabb->max.z, model->vertices.data[i].z);
  }
}

void print_model_face(int face_id, model_t *model) {
  int num_vertices = model->face_num_vertices.data[face_id];
  
  for (int i = 0; i < num_vertices; i++) {
    vec5_t p;
    int pt = model->faces.data[model->face_index.data[face_id] + i];
    int pt2 = model->tx_faces.data[model->tx_face_index.data[face_id] + i];
    p.x = model->vertices.data[pt].x;
    p.y = model->vertices.data[pt].y;
    p.z = model->vertices.data[pt].z;
    p.u = model->txcoords.data[pt2].u;
    p.v = model->txcoords.data[pt2].v;
    
    printf("x %.4f, y %.4f, z %.4f, u %.4f, v %.4f\n", p.x, p.y, p.z, p.u, p.v);
  }
  
  printf("num_vertices %d\n", num_vertices);
}

inline int clamp_i(int x, int min, int max) {
  if (x < min) {
    x = min;
  } else
  if (x > max) {
    x = max;
  }
  
  return x;
}

int count_bits(int n) {
  int i = 0;
  
  while (n) {
    n >>= 1;
    i++;
  }
  
  return i;
}

int log2_c(int n) {
  int i = 0;
  
  while (n >>= 1) {
    i++;
  }
  
  return i;
}

int test_loop(int *input, int count) {
  (*input)++;
  
  if (*input == count) {
    puts("infinite_loop");
    return 1;
  }
  
  return 0;
}