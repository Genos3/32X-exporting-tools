#include "common.h"

int check_valid_face(face_t *merged_pl, int face0_id, model_t *model);
void remove_mid_vertices(face_t *merged_pl);
void average_plane(int face0_id, int face1_id, face_t *merged_pl, pl_list_t *pl_list, model_t *model);
void add_merged_face(int face0_id, int face1_id, face_t *merged_pl, d_lnk_ls_t *grid_list, grid_t *grid, pl_list_t *pl_list);
void merge_grid_faces(int face0_id, int face1_id, d_lnk_ls_t *grid_list, grid_t *grid);
void merge_aabb(int face0_id, int face1_id);

#define WEIGHTED_AVERAGE_MERGED_PLANE 1

extern int merged_face, invalid_face, tiled_faces;
extern int pl_shared_vt_list[2][8];
extern aabb_t *grid_pl_list_aabb_i;
extern u8 *removed_faces;

// merges the faces

void merge_shared_faces(int face0_id, int face1_id, int shared_vt0_id, d_lnk_ls_t *grid_list, grid_t *grid, pl_list_t *pl_list, model_t *model) {
  face_t merged_pl;
  vec2_tx_t dt_tx_vt = {};
  merged_pl.num_vertices = 0;
  int dt_pl_index_h = 0;
  int dt_pl_index_v = 0;
  int vt_index = 0; // current vertex
  int face_index = 0; // current face
  int face_id = face0_id; // current face id
  
  // if the textures are tiled get the difference between the texture coordinates in order to add them after and obtain the leftmost and topmost face
  if (tiled_faces) {
    vec2_tx_t face0_tx_vt = pl_list->txcoords.data[face0_id * 8 + shared_vt0_id];
    vec2_tx_t face1_tx_vt = pl_list->txcoords.data[face1_id * 8 + pl_shared_vt_list[0][shared_vt0_id]];
    
    // substract the texture coordinates in the first shared vertex
    dt_tx_vt.u = face1_tx_vt.u - face0_tx_vt.u;
    dt_tx_vt.v = face1_tx_vt.v - face0_tx_vt.v;
    
    if (dt_tx_vt.u > 0) {
      dt_pl_index_h = 0;
    } else if (dt_tx_vt.u < 0) {
      dt_pl_index_h = 1;
      dt_tx_vt.u = -dt_tx_vt.u;
    }
    
    if (dt_tx_vt.v > 0) {
      dt_pl_index_v = 0;
    } else if (dt_tx_vt.v < 0) {
      dt_pl_index_v = 1;
      dt_tx_vt.v = -dt_tx_vt.v;
    }
  }
  
  // iterate back until it finds a non shared vertex
  while (pl_shared_vt_list[face_index][vt_index] >= 0) {
    vt_index--;
    if (vt_index < 0) {
      vt_index = pl_list->face_num_vertices.data[face_id] - 1;
    }
  }
  
  int initial_vt = vt_index;
  
  // iterate over the two faces until it cames back to the initial vertex
  while (!merged_pl.num_vertices || vt_index != initial_vt || face_index) {
    if (merged_pl.num_vertices == 8) return; // polygon size limit
    
    // add the current vertex to the new face
    merged_pl.vertices[merged_pl.num_vertices] = pl_list->vertices.data[face_id * 8 + vt_index];
    merged_pl.txcoords[merged_pl.num_vertices] = pl_list->txcoords.data[face_id * 8 + vt_index];
    
    if (tiled_faces) {
      if (dt_tx_vt.u && dt_pl_index_h == face_index) {
        merged_pl.txcoords[merged_pl.num_vertices].u += dt_tx_vt.u;
      }
      
      if (dt_tx_vt.v && dt_pl_index_v == face_index) {
        merged_pl.txcoords[merged_pl.num_vertices].v += dt_tx_vt.v;
      }
    }
    
    // if the vertex is shared jump to the other face
    if (pl_shared_vt_list[face_index][vt_index] >= 0) {
      if (!face_index) {
        face_id = face1_id;
      } else {
        face_id = face0_id;
      }
      
      vt_index = pl_shared_vt_list[face_index][vt_index];
      // margin = sin(1.0 * PI / 180); // 1.0°
      
      face_index = !face_index; // invert face_index
      
      // if the next vertex is also shared return
      int vt_idx_t = vt_index + 1;
      if (vt_idx_t >= pl_list->face_num_vertices.data[face_id]) {
        vt_idx_t = 0;
      }
      
      if (pl_shared_vt_list[face_index][vt_idx_t] >= 0) return;
    }
    
    merged_pl.num_vertices++;
    vt_index++;
    if (vt_index >= pl_list->face_num_vertices.data[face_id]) {
      vt_index = 0;
    }
  }
  
  // checks if the final face is convex, if not discard it
  if (!check_valid_face(&merged_pl, face0_id, model)) return;
  
  remove_mid_vertices(&merged_pl);
  
  if (merged_pl.num_vertices > ini.face_merge_max_sides) return;
  
  average_plane(face0_id, face1_id, &merged_pl, pl_list, model);
  add_merged_face(face0_id, face1_id, &merged_pl, grid_list, grid, pl_list);
  merged_face = 1;
}

// checks if the final face is convex

int check_valid_face(face_t *merged_pl, int face0_id, model_t *model) {
  int vt_prev = merged_pl->num_vertices - 1;
  int vt_next = 1;
  vec3_t v0, v1, v2, normal;
  vec3_t normal_0 = model->normals.data[face0_id];
  
  for (int i = 0; i < merged_pl->num_vertices; i++) {
    v0.x = merged_pl->vertices[i].x;
    v0.y = merged_pl->vertices[i].y;
    v0.z = merged_pl->vertices[i].z;
    v1.x = merged_pl->vertices[vt_prev].x;
    v1.y = merged_pl->vertices[vt_prev].y;
    v1.z = merged_pl->vertices[vt_prev].z;
    v2.x = merged_pl->vertices[vt_next].x;
    v2.y = merged_pl->vertices[vt_next].y;
    v2.z = merged_pl->vertices[vt_next].z;
    
    vec3_t d0, d1;
    d0.x = v1.x - v0.x;
    d0.y = v1.y - v0.y;
    d0.z = v1.z - v0.z;
    d1.x = v2.x - v0.x;
    d1.y = v2.y - v0.y;
    d1.z = v2.z - v0.z;
    normalize(&d0, &d0);
    normalize(&d1, &d1);
    // calc normal
    cross(&d0, &d1, &normal);
    normalize(&normal, &normal);
    
    // test if the polygon is convex
    if (i && dot(&normal, &normal_0) < -0.01) {
      invalid_face = 1; // debug
      return 0; // polygon is not convex
    }
    
    merged_pl->angles[i] = dot(&d0, &d1);
    
    vt_prev = i;
    vt_next++;
    if (vt_next == merged_pl->num_vertices) {
      vt_next = 0;
    }
  }
  
  return 1;
}

// remove any vertices that have ended inside a straight line, happens when merging two rectangles

void remove_mid_vertices(face_t *merged_pl) {
  int final_num_vt = 0;
  for (int i = 0; i < merged_pl->num_vertices; i++) {
    // if (merged_pl_angles[i] < 180 - merge_angle_dist)
    if (merged_pl->angles[i] >= -1 + 0.01) { // 180
      if (i != final_num_vt) {
        // remove vertex
        merged_pl->angles[final_num_vt] = merged_pl->angles[i];
        merged_pl->vertices[final_num_vt] = merged_pl->vertices[i];
        merged_pl->txcoords[final_num_vt] = merged_pl->txcoords[i];
      }
      final_num_vt++;
    }
  }
  merged_pl->num_vertices = final_num_vt;
}

// if the normals for the two faces doesn't coincide project the vertices of the final face on an average plane of the other two faces

void average_plane(int face0_id, int face1_id, face_t *merged_pl, pl_list_t *pl_list, model_t *model) {
  vec3_t normal_0 = model->normals.data[face0_id];
  vec3_t normal_1 = model->normals.data[face1_id];
  
  if (normal_0.x == normal_1.x &&
      normal_0.y == normal_1.y &&
      normal_0.z == normal_1.z) return;
  
  poly_t poly;
  for (int i = 0; i < merged_pl->num_vertices; i++) {
    poly.vertices[i].x = merged_pl->vertices[i].x;
    poly.vertices[i].y = merged_pl->vertices[i].y;
    poly.vertices[i].z = merged_pl->vertices[i].z;
  }
  poly.num_vertices = merged_pl->num_vertices;
  
  vec3_t avg_normal;
  #if !WEIGHTED_AVERAGE_MERGED_PLANE
    avg_normal.x = (normal_0.x + normal_1.x) / 2;
    avg_normal.y = (normal_0.y + normal_1.y) / 2;
    avg_normal.z = (normal_0.z + normal_1.z) / 2;
    normalize(&avg_normal, &avg_normal);
  #else
    vec3_t v0, v1, v2, cross;
    v0 = pl_list->vertices.data[face0_id * 8];
    v1 = pl_list->vertices.data[face0_id * 8 + 1];
    v2 = pl_list->vertices.data[face0_id * 8 + 2];
    calc_normal(&v0, &v1, &v2, &cross);
    float triangle_0_area = calc_length(&cross) / 2;
    
    v0 = pl_list->vertices.data[face1_id * 8];
    v1 = pl_list->vertices.data[face1_id * 8 + 1];
    v2 = pl_list->vertices.data[face1_id * 8 + 2];
    calc_normal(&v0, &v1, &v2, &cross);
    float triangle_1_area = calc_length(&cross) / 2;
    
    float sum_triangle_areas = triangle_0_area + triangle_1_area;
    float w0 = triangle_0_area / sum_triangle_areas;
    float w1 = triangle_1_area / sum_triangle_areas;
    
    avg_normal.x = normal_0.x * w0 + normal_1.x * w1;
    avg_normal.y = normal_0.y * w0 + normal_1.y * w1;
    avg_normal.z = normal_0.z * w0 + normal_1.z * w1;
    normalize(&avg_normal, &avg_normal);
  #endif
  
  // d = -dot(n, (p1 + p2 + p3 + p4) / 4)
  float avg_distance;
  vec3_t avg_vt;
  avg_vt.x = poly.vertices[0].x;
  avg_vt.y = poly.vertices[0].y;
  avg_vt.z = poly.vertices[0].z;
  
  for (int i = 1; i < poly.num_vertices; i++) {
    avg_vt.x += poly.vertices[i].x;
    avg_vt.y += poly.vertices[i].y;
    avg_vt.z += poly.vertices[i].z;
    //avg_distance += dot(poly[i], avg_normal);
  }
  
  avg_vt.x /= poly.num_vertices;
  avg_vt.y /= poly.num_vertices;
  avg_vt.z /= poly.num_vertices;
  avg_distance = -dot(&avg_normal, &avg_vt);
  
  // project the vertices on the plane, p - n(dot(n, p) + d) * n
  
  for (int i = 0; i < poly.num_vertices; i++) {
    float a = dot(&poly.vertices[i], &avg_normal) + avg_distance;
    merged_pl->vertices[i].x = poly.vertices[i].x - avg_normal.x * a;
    merged_pl->vertices[i].y = poly.vertices[i].y - avg_normal.y * a;
    merged_pl->vertices[i].z = poly.vertices[i].z - avg_normal.z * a;
  }
  
  model->normals.data[face0_id] = avg_normal;
}

// replaces face_0 with the merged face and mark to remove the second face

void add_merged_face(int face0_id, int face1_id, face_t *merged_pl, d_lnk_ls_t *grid_list, grid_t *grid, pl_list_t *pl_list) {
  for (int i = 0; i < merged_pl->num_vertices; i++) {
    pl_list->vertices.data[face0_id * 8 + i] = merged_pl->vertices[i];
    pl_list->txcoords.data[face0_id * 8 + i] = merged_pl->txcoords[i];
  }
  pl_list->face_num_vertices.data[face0_id] = merged_pl->num_vertices;
  merge_grid_faces(face0_id, face1_id, grid_list, grid);
  merge_aabb(face0_id, face1_id);
  removed_faces[face1_id] = 1;
}

// updates the grid

void merge_grid_faces(int face0_id, int face1_id, d_lnk_ls_t *grid_list, grid_t *grid) {
  aabb_t aabb_0 = grid_pl_list_aabb_i[face0_id];
  aabb_t aabb_1 = grid_pl_list_aabb_i[face1_id];
  
  for (int i = aabb_1.min.z; i <= aabb_1.max.z; i++) {
    for (int j = aabb_1.min.y; j <= aabb_1.max.y; j++) {
      for (int k = aabb_1.min.x; k <= aabb_1.max.x; k++) {
        
        if (i >= aabb_0.min.z && i <= aabb_0.max.z &&
            j >= aabb_0.min.y && j <= aabb_0.max.y &&
            k >= aabb_0.min.x && k <= aabb_0.max.x) {
          // face 0 and face 1 are in the same tile
          remove_grid_element(k, j, i, face1_id, grid_list, grid);
        } else {
          // tile only contains face 1, replace face 1 with face 0
          replace_grid_element(k, j, i, face1_id, face0_id, grid_list, grid);
        }
      }
    }
  }
}

// merge the two faces aabb

void merge_aabb(int face0_id, int face1_id) {
  aabb_t aabb_0 = grid_pl_list_aabb_i[face0_id];
  aabb_t aabb_1 = grid_pl_list_aabb_i[face1_id];
  aabb_0.min.x = min_c(aabb_0.min.x, aabb_1.min.x);
  aabb_0.max.x = max_c(aabb_0.max.x, aabb_1.max.x);
  aabb_0.min.y = min_c(aabb_0.min.y, aabb_1.min.y);
  aabb_0.max.y = max_c(aabb_0.max.y, aabb_1.max.y);
  aabb_0.min.z = min_c(aabb_0.min.z, aabb_1.min.z);
  aabb_0.max.z = max_c(aabb_0.max.z, aabb_1.max.z);
  grid_pl_list_aabb_i[face0_id] = aabb_0;
  // grid_pl_list_aabb_i[face1_id] = grid_pl_list_aabb_i[model->num_faces - 1];
}