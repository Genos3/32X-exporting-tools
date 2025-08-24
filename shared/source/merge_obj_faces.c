#include "common.h"

void set_snapped_normals(object_t *object);
void m_make_face_grid(d_lnk_ls_t *grid_list, grid_t *grid, object_t *object);
void check_map(int face_id, object_t *object);
void check_face_map(int map_pnt, int face_id, d_lnk_ls_t *grid_list, grid_t *grid, object_t *object);
void check_face_merge(int face_id, int map_face_id, object_t *object);
int compare_edge_length();
void merge_faces(int face0_id, u8 edge_id, d_lnk_ls_t *grid_list, grid_t *grid, object_t *object);

static aabb_t aabb_0;
int merged_face, invalid_face, tiled_faces;
int pl_shared_vt_list[2][8];
vec3_t *snapped_normals;
aabb_t *grid_object_aabb_i;
u8 *visited_faces;
grid_t face_grid;
d_lnk_ls_t face_grid_list;

typedef struct {
  u8 vt0, vt1;
} edge_t;

typedef struct {
  edge_t face0_edge, face1_edge;
  float length;
  int face1_id;
} shared_edge_t;

static shared_edge_t shared_edges[8];
static int num_shared_edges;

// requires normalized texture coordinates, snapped vertices and object size set
// TODO: add sprite support

void merge_obj_faces(object_t *object) {
  visited_faces = malloc(object->num_faces);
  set_snapped_normals(object);
  m_make_face_grid(&face_grid_list, &face_grid, object);
  
  for (int i = 0; i < object->num_faces; i++) {
    if (object->faces.data[i].remove || object->faces.data[i].type & SPRITE) continue;
    
    merged_face = 1; // intializes the loop
    
    while (merged_face) {
      merged_face = 0;
      
      if (ini.limit_dist_face_merge) {
        poly_t poly;
        set_poly_from_obj_face(i, &poly, object);
        make_poly_aabb(&aabb_0, &poly);
      }
      
      num_shared_edges = 0;
      
      check_map(i, object);
      
      if (!num_shared_edges) continue;
      
      u8 edge_id;
      if (num_shared_edges > 1) {
        edge_id = compare_edge_length();
      } else {
        edge_id = 0;
      }
      
      merge_faces(i, edge_id, &face_grid_list, &face_grid, object);
    }
  }
  
  free(snapped_normals);
  free(grid_object_aabb_i);
  free_lnk_ls_grid(&face_grid_list, &face_grid);
  
  // removes the marked faces
  remove_object_marked_faces(object);
  
  free(visited_faces);
}

// snap the normals within a specific margin so they coincide when they are compared

void set_snapped_normals(object_t *object) {
  snapped_normals = malloc(object->num_faces * sizeof(vec3_t));
  
  for (int i = 0; i < object->num_faces; i++) {
    snapped_normals[i].x = (int)(object->faces.data[i].normal.x / ini.merge_nm_dist_f);
    snapped_normals[i].y = (int)(object->faces.data[i].normal.y / ini.merge_nm_dist_f);
    snapped_normals[i].z = (int)(object->faces.data[i].normal.z / ini.merge_nm_dist_f);
  }
}

// makes a grid that stores every face of the object on it, used to optimize the face search

void m_make_face_grid(d_lnk_ls_t *grid_list, grid_t *grid, object_t *object) {
  poly_t poly;
  grid->size_i.w = ceilf(object->size.w / ini.face_merge_grid_tile_size);
  grid->size_i.h = ceilf(object->size.h / ini.face_merge_grid_tile_size);
  grid->size_i.d = ceilf(object->size.d / ini.face_merge_grid_tile_size);
  
  grid->size_i.w = max_c(grid->size_i.w, 1);
  grid->size_i.h = max_c(grid->size_i.h, 1);
  grid->size_i.d = max_c(grid->size_i.d, 1);
  
  malloc_d_lnk_ls_grid(grid_list, grid);
  
  grid_object_aabb_i = malloc(object->num_faces * sizeof(aabb_t));
  
  for (int i = 0; i < object->num_faces; i++) {
    if (object->faces.data[i].type & SPRITE) continue;
    poly.num_vertices = object->faces.data[i].num_vertices;
    
    if (ini.make_grid) {
      for (int j = 0; j < poly.num_vertices; j++) {
        poly.vertices[j].x = (object->faces.data[i].vertices[j].x - object->origin.x) / ini.face_merge_grid_tile_size;
        poly.vertices[j].y = (object->faces.data[i].vertices[j].y - object->origin.y) / ini.face_merge_grid_tile_size;
        poly.vertices[j].z = (object->faces.data[i].vertices[j].z - object->origin.z) / ini.face_merge_grid_tile_size;
      }
      
      make_poly_aabb(&grid_object_aabb_i[i], &poly);
      adjust_poly_tile_size(&grid_object_aabb_i[i]);
      set_map_list(i, &grid_object_aabb_i[i], grid_list, grid);
    } else {
      for (int j = 0; j < poly.num_vertices; j++) {
        poly.vertices[j].x = (int)((object->faces.data[i].vertices[j].x - object->origin.x) / ini.face_merge_grid_tile_size);
        poly.vertices[j].y = (int)((object->faces.data[i].vertices[j].y - object->origin.y) / ini.face_merge_grid_tile_size);
        poly.vertices[j].z = (int)((object->faces.data[i].vertices[j].z - object->origin.z) / ini.face_merge_grid_tile_size);
      }
      
      make_poly_aabb(&grid_object_aabb_i[i], &poly);
      set_map_list(i, &grid_object_aabb_i[i], grid_list, grid);
    }
  }
}

// store the face on every tile it touches

void set_map_list(int face_id, aabb_t *aabb, d_lnk_ls_t *grid_list, grid_t *grid) {
  for (int i = aabb->min.z; i <= aabb->max.z; i++) {
    for (int j = aabb->min.y; j <= aabb->max.y; j++) {
      for (int k = aabb->min.x; k <= aabb->max.x; k++) {
        //int pnt = y * map_width * map_depth + z * map_width + x;
        add_grid_d_lnk_ls_element(k, j, i, face_id, grid_list, grid);
      }
    }
  }
}

// checks every grid tile the face aabb touches

void check_map(int face_id, object_t *object) {
  //visited_faces = calloc(object->num_faces, 1);
  memset(visited_faces, 0, object->num_faces);
  visited_faces[face_id] = 1;
  aabb_t grid_aabb = grid_object_aabb_i[face_id]; // grid aabb
  tiled_faces = 0;
  
  for (int i = grid_aabb.min.z; i <= grid_aabb.max.z; i++) {
    for (int j = grid_aabb.min.y; j <= grid_aabb.max.y; j++) {
      for (int k = grid_aabb.min.x; k <= grid_aabb.max.x; k++) {
        int map_pnt = j * face_grid.size_i.d * face_grid.size_i.w + i * face_grid.size_i.w + k;
        check_face_map(map_pnt, face_id, &face_grid_list, &face_grid, object);
      }
    }
  }
}

// iterate every face on the grid tile and compare them against the passed one

void check_face_map(int map_pnt, int face_id, d_lnk_ls_t *grid_list, grid_t *grid, object_t *object) {
  int list_pnt = grid->data_pnt[map_pnt];
  
  while (list_pnt >= 0) {
    invalid_face = 0; // debug
    int map_face_id = grid_list->nodes.data[list_pnt].data;
    
    if (!visited_faces[map_face_id] && !object->faces.data[map_face_id].remove) {
      check_face_merge(face_id, map_face_id, object);
      visited_faces[map_face_id] = 1;
    }
    
    list_pnt = grid_list->nodes.data[list_pnt].next;
  }
}

// test if the two faces can be merged

void check_face_merge(int face0_id, int face1_id, object_t *object) {
  
  // if the materials are not shared return
  if (object->faces.data[face0_id].material_id != object->faces.data[face1_id].material_id) return;
  
  // memset(pl_shared_vt_list, -1, sizeof(pl_shared_vt_list));
  vec3_t nm = snapped_normals[face0_id];
  vec3_t nm2 = snapped_normals[face1_id];
  
  //if the normals doesn't coincide return
  if (nm.x != nm2.x || nm.y != nm2.y || nm.z != nm2.z) return;
  
  u8 textured_face;
  if (object->faces.data[face0_id].type & TEXTURED) {
    textured_face = 0;
  } else {
    textured_face = 1;
  }
  
  edge_t shared_face_0, shared_face_1;
  int shared_vertices = 0;
  int shared_mod_txcoords = 0;
  int shared_vt_txcoords = 0;
  
  // for each vertex in the first face
  for (int i = 0; i < object->faces.data[face0_id].num_vertices; i++) {
    vec3_t vt_face0 = object->faces.data[face0_id].vertices[i];
    
    // for each vertex in the second face
    for (int j = 0; j < object->faces.data[face1_id].num_vertices; j++) {
      vec3_t vt_face1 = object->faces.data[face1_id].vertices[j];
      
      // if the vertex is shared
      if (vt_face0.x == vt_face1.x && vt_face0.y == vt_face1.y && vt_face0.z == vt_face1.z) {
        if (!shared_vertices) {
          shared_face_0.vt0 = i;
          shared_face_1.vt0 = j;
        } else {
          shared_face_0.vt1 = i;
          shared_face_1.vt1 = j;
        }
        
        shared_vertices++;
        
        if (!textured_face) continue; // the following code is not required if the face is not textured
        
        vec2_tx_t txcoord_face0 = object->faces.data[face0_id].txcoords[i];
        vec2_tx_t txcoord_face1 = object->faces.data[face1_id].txcoords[j];
        
        // if the remainder of the texture coordinate is shared (used for tiled textures)
        // requires normalized texture coordinates
        if (fmod(txcoord_face0.u, 1) == fmod(txcoord_face1.u, 1) && fmod(txcoord_face0.v, 1) == fmod(txcoord_face1.v, 1)) {
          shared_mod_txcoords++; // the modulo for both is the same
          
          if (txcoord_face0.u == txcoord_face1.u && txcoord_face0.v == txcoord_face1.v) {
            shared_vt_txcoords++; // exact same values
          }
        }
      }
    }
  }
  
  // if all the vertices in the first face are shared return
  if (shared_vertices == object->faces.data[face0_id].num_vertices) return;
  
  u8 add_edge_to_list = 0;
  
  // if two or more vertices and texture coords are shared calculate the edge length and add it to the shared edge grid_list
  if (shared_vertices >= 2 && (!textured_face || shared_mod_txcoords >= 2)) {
    
    // if the texture can be repeated
    // happens when the vertices are shared and the remainder of the modulo between the texture coordinates are the same
    if (textured_face && !shared_vt_txcoords) {
      tiled_faces = 1;
    }
    
    // if the limit to the final size of the merged face is enabled
    if (ini.limit_dist_face_merge) {
      aabb_t aabb_1;
      poly_t poly;
      set_poly_from_obj_face(face1_id, &poly, object);
      make_poly_aabb(&aabb_1, &poly);
      
      // obtain the size of the two faces
      size3_t face;
      face.w = max_c(aabb_0.max.x, aabb_1.max.x) - min_c(aabb_0.min.x, aabb_1.min.x);
      face.h = max_c(aabb_0.max.y, aabb_1.max.y) - min_c(aabb_0.min.y, aabb_1.min.y);
      face.d = max_c(aabb_0.max.z, aabb_1.max.z) - min_c(aabb_0.min.z, aabb_1.min.z);
      
      // if the size of the merged face is smaller or equal than the allowed distance then add the edge
      if (face.w <= ini.face_merge_dist_f && face.d <= ini.face_merge_dist_f && face.h <= ini.face_merge_dist_f) {
        // merge_shared_faces(face0_id, face1_id, object);
        add_edge_to_list = 1;
      }
    } else {
      // merge_shared_faces(face0_id, face1_id, object);
      add_edge_to_list = 1;
    }
  }
  
  if (add_edge_to_list) {
    vec3_t vt0 = object->faces.data[face0_id].vertices[shared_face_0.vt0];
    vec3_t vt1 = object->faces.data[face0_id].vertices[shared_face_0.vt1];
    
    // calculate the edge length
    float length = calc_length_vt(&vt0, &vt1);
    
    shared_edges[num_shared_edges].face0_edge = shared_face_0;
    shared_edges[num_shared_edges].face1_edge = shared_face_1;
    shared_edges[num_shared_edges].length = length;
    shared_edges[num_shared_edges].face1_id = face1_id;
    num_shared_edges++;
  }
}

// return the longest edge

int compare_edge_length() {
  int longest_edge_id = 0;
  float length = shared_edges[0].length;
  
  for (int j = 1; j <= num_shared_edges; j++) {
    if (shared_edges[j].length > length) {
      length = shared_edges[j].length;
      longest_edge_id = j;
    }
  }
  
  return longest_edge_id;
}

void merge_faces(int face0_id, u8 edge_id, d_lnk_ls_t *grid_list, grid_t *grid, object_t *object) {
  memset(pl_shared_vt_list, -1, sizeof(pl_shared_vt_list));
  
  // set which vertex is shared with which
  int vt_id = shared_edges[edge_id].face0_edge.vt0;
  pl_shared_vt_list[0][vt_id] = shared_edges[edge_id].face1_edge.vt0;
  vt_id = shared_edges[edge_id].face0_edge.vt1;
  pl_shared_vt_list[0][vt_id] = shared_edges[edge_id].face1_edge.vt1;
  vt_id = shared_edges[edge_id].face1_edge.vt0;
  pl_shared_vt_list[1][vt_id] = shared_edges[edge_id].face0_edge.vt0;
  vt_id = shared_edges[edge_id].face1_edge.vt1;
  pl_shared_vt_list[1][vt_id] = shared_edges[edge_id].face0_edge.vt1;
  
  int face1_id = shared_edges[edge_id].face1_id;
  int shared_vt0_id = shared_edges[edge_id].face0_edge.vt0;
  
  merge_shared_faces(face0_id, face1_id, shared_vt0_id, grid_list, grid, object);
}

// remove any unreferenced vertices

void remove_extra_vertices(object_t *object) {
  int final_num_vt = 0;
  
  for (int i = 0; i < object->num_vertices; i++) {
    int used_vt = 0;
    
    for (int j = 0; j < object->num_faces; j++) {
      for (int k = 0; k < object->faces.data[j].num_vertices; k++) {
        if (object->faces.data[j].vt_index[k] == i) {
          used_vt = 1;
          break;
        }
      }
    }
    
    if (used_vt) {
      if (i != final_num_vt) {
        // move the next vertices to the empty spaces
        object->vertices.data[final_num_vt].x = object->vertices.data[i].x;
        object->vertices.data[final_num_vt].y = object->vertices.data[i].y;
        object->vertices.data[final_num_vt].z = object->vertices.data[i].z;
        
        // update the faces
        for (int j = 0; j < object->num_faces; j++) {
          for (int k = 0; k < object->faces.data[j].num_vertices; k++) {
            if (object->faces.data[j].vt_index[k] == i) {
              object->faces.data[j].vt_index[k] = final_num_vt;
            }
          }
        }
      }
      
      final_num_vt++;
    }
  }
  
  object->num_vertices = final_num_vt;
  object->vertices.size = final_num_vt;
}

// remove any unreferenced texture coords

void remove_extra_txcoords(object_t *object) {
  int final_num_vt = 0;
  
  for (int i = 0; i < object->num_txcoords; i++) {
    int used_vt = 0;
    
    for (int j = 0; j < object->num_faces; j++) {
      for (int k = 0; k < object->faces.data[j].num_vertices; k++) {
        if (object->faces.data[j].tx_vt_index[k] == i) {
          used_vt = 1;
          break;
        }
      }
    }
    
    if (used_vt) {
      if (i != final_num_vt) {
        // move the next texture coords to the empty spaces
        object->txcoords.data[final_num_vt].u = object->txcoords.data[i].u;
        object->txcoords.data[final_num_vt].v = object->txcoords.data[i].v;
        
        // update the faces
        for (int j = 0; j < object->num_faces; j++) {
          for (int k = 0; k < object->faces.data[j].num_vertices; k++) {
            if (object->faces.data[j].tx_vt_index[k] == i) {
              object->faces.data[j].tx_vt_index[k] = final_num_vt;
            }
          }
        }
      }
      
      final_num_vt++;
    }
  }
  
  object->num_txcoords = final_num_vt;
  object->txcoords.size = final_num_vt;
}