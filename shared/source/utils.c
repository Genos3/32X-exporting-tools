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

// transfer the face from the object vertex list to the poly_t struct

void set_poly_from_obj_vertices(int face_id, poly_t *poly, object_t *object) {
  poly->num_vertices = object->faces.data[face_id].num_vertices;
  
  for (int i = 0; i < poly->num_vertices; i++) {
    int pt = object->faces.data[face_id].vt_index[i];
    poly->vertices[i] = object->vertices.data[pt];
    
    if (object->faces.data[face_id].has_texture) {
      int pt_tx = object->faces.data[face_id].tx_vt_index[i];
      poly->txcoords[i] = object->txcoords.data[pt_tx];
    }
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

void make_poly_tx_aabb(tx_aabb_t *aabb, poly_t *poly) {
  aabb->min.u = poly->txcoords[0].u;
  aabb->max.u = poly->txcoords[0].u;
  aabb->min.v = poly->txcoords[0].v;
  aabb->max.v = poly->txcoords[0].v;
  
  for (int i = 1; i < poly->num_vertices; i++) {
    aabb->min.u = min_c(aabb->min.u, poly->txcoords[i].u);
    aabb->max.u = max_c(aabb->max.u, poly->txcoords[i].u);
    aabb->min.v = min_c(aabb->min.v, poly->txcoords[i].v);
    aabb->max.v = max_c(aabb->max.v, poly->txcoords[i].v);
  }
}

void make_object_aabb(aabb_t *aabb, object_t *object) {
  aabb->min.x = object->vertices.data[0].x;
  aabb->max.x = object->vertices.data[0].x;
  aabb->min.y = object->vertices.data[0].y;
  aabb->max.y = object->vertices.data[0].y;
  aabb->min.z = object->vertices.data[0].z;
  aabb->max.z = object->vertices.data[0].z;
  
  for (int i = 1; i < object->num_vertices; i++) {
    aabb->min.x = min_c(aabb->min.x, object->vertices.data[i].x);
    aabb->max.x = max_c(aabb->max.x, object->vertices.data[i].x);
    aabb->min.y = min_c(aabb->min.y, object->vertices.data[i].y);
    aabb->max.y = max_c(aabb->max.y, object->vertices.data[i].y);
    aabb->min.z = min_c(aabb->min.z, object->vertices.data[i].z);
    aabb->max.z = max_c(aabb->max.z, object->vertices.data[i].z);
  }
}

void print_model_face(int face_id, object_t *object) {
  int num_vertices = object->faces.data[face_id].num_vertices;
  
  for (int i = 0; i < num_vertices; i++) {
    vec5_t p;
    p.x = object->faces.data[face_id].vertices[i].x;
    p.y = object->faces.data[face_id].vertices[i].y;
    p.z = object->faces.data[face_id].vertices[i].z;
    p.u = object->faces.data[face_id].txcoords[i].u;
    p.v = object->faces.data[face_id].txcoords[i].v;
    
    printf("x %.4f, y %.4f, z %.4f, u %.4f, v %.4f\n", p.x, p.y, p.z, p.u, p.v);
  }
  
  printf("num_vertices %d\n", num_vertices);
}

inline int clamp_i(int x, int min, int max) {
  if (x < min) {
    x = min;
  }
  else if (x > max) {
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