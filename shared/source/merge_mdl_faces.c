#include "common.h"

void set_snapped_normals(model_t *model);
void m_make_face_grid(d_lnk_ls_t *grid_list, grid_t *grid, pl_list_t *pl_list, model_t *model);
void check_map(int face_id, pl_list_t *pl_list, model_t *model);
void check_face_map(int map_pnt, int face_id, d_lnk_ls_t *grid_list, grid_t *grid, pl_list_t *pl_list, model_t *model);
void check_face_merge(int face_id, int map_face_id, pl_list_t *pl_list, model_t *model);
int compare_edge_length();
void merge_faces(int face0_id, u8 edge_id, d_lnk_ls_t *grid_list, grid_t *grid, pl_list_t *pl_list, model_t *model);

static aabb_t aabb_0;
int merged_face, invalid_face, tiled_faces;
int pl_shared_vt_list[2][8];
vec3_t *snapped_normals;
aabb_t *grid_pl_list_aabb_i;
u8 *visited_faces, *removed_faces;
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

// requires normalized texture coordinates, snapped vertices and model size set
// TODO: add sprites support

void merge_mdl_faces(pl_list_t *pl_list, model_t *model) {
  // pl_list_t pl_list;
  removed_faces = calloc(model->num_faces, 1);
  visited_faces = malloc(model->num_faces);
  // init_pl_list(&pl_list);
  // set_pl_list(&pl_list, model);
  set_snapped_normals(model);
  m_make_face_grid(&face_grid_list, &face_grid, pl_list, model);
  
  for (int i = 0; i < pl_list->num_faces; i++) {
    if (removed_faces[i] || model->face_types.data[i] & SPRITE) continue;
    
    merged_face = 1; // intializes the loop
    
    while (merged_face) {
      merged_face = 0;
      
      if (ini.limit_dist_face_merge) {
        poly_t poly;
        pl_list_set_poly(i, &poly, pl_list);
        make_poly_aabb(&aabb_0, &poly);
      }
      
      num_shared_edges = 0;
      
      check_map(i, pl_list, model);
      
      if (!num_shared_edges) continue;
      
      u8 edge_id;
      if (num_shared_edges > 1) {
        edge_id = compare_edge_length();
      } else {
        edge_id = 0;
      }
      
      merge_faces(i, edge_id, &face_grid_list, &face_grid, pl_list, model);
    }
  }
  
  free(snapped_normals);
  free(grid_pl_list_aabb_i);
  free_lnk_ls_grid(&face_grid_list, &face_grid);
  
  // removes the marked faces
  pl_list_remove_marked_faces(pl_list, model);
  
  // free_pl_list(pl_list);
  free(removed_faces);
  free(visited_faces);
  
  // set_face_index(model);
  // set_tx_face_index(model);
  // remove_extra_vertices(model);
}

// snap the normals within a specific margin so they coincide when they are compared

void set_snapped_normals(model_t *model) {
  snapped_normals = malloc(model->num_faces * sizeof(vec3_t));
  
  for (int i = 0; i < model->num_faces; i++) {
    snapped_normals[i].x = (int)(model->normals.data[i].x / ini.merge_nm_dist_f);
    snapped_normals[i].y = (int)(model->normals.data[i].y / ini.merge_nm_dist_f);
    snapped_normals[i].z = (int)(model->normals.data[i].z / ini.merge_nm_dist_f);
  }
}

// makes a grid that stores every face of the model on it, used to optimize the face search

void m_make_face_grid(d_lnk_ls_t *grid_list, grid_t *grid, pl_list_t *pl_list, model_t *model) {
  poly_t poly;
  grid->size_i.w = ceilf(model->size.w / ini.face_merge_grid_tile_size);
  grid->size_i.h = ceilf(model->size.h / ini.face_merge_grid_tile_size);
  grid->size_i.d = ceilf(model->size.d / ini.face_merge_grid_tile_size);
  
  grid->size_i.w = max_c(grid->size_i.w, 1);
  grid->size_i.h = max_c(grid->size_i.h, 1);
  grid->size_i.d = max_c(grid->size_i.d, 1);
  
  malloc_d_lnk_ls_grid(grid_list, grid);
  
  grid_pl_list_aabb_i = malloc(pl_list->num_faces * sizeof(aabb_t));
  
  for (int i = 0; i < pl_list->num_faces; i++) {
    if (model->face_types.data[i] & SPRITE) continue;
    poly.num_vertices = pl_list->face_num_vertices.data[i];
    
    if (ini.make_grid) {
      for (int j = 0; j < poly.num_vertices; j++) {
        poly.vertices[j].x = (pl_list->vertices.data[i * 8 + j].x - model->origin.x) / ini.face_merge_grid_tile_size;
        poly.vertices[j].y = (pl_list->vertices.data[i * 8 + j].y - model->origin.y) / ini.face_merge_grid_tile_size;
        poly.vertices[j].z = (pl_list->vertices.data[i * 8 + j].z - model->origin.z) / ini.face_merge_grid_tile_size;
      }
      
      make_poly_aabb(&grid_pl_list_aabb_i[i], &poly);
      adjust_poly_tile_size(&grid_pl_list_aabb_i[i]);
      set_map_list(i, &grid_pl_list_aabb_i[i], grid_list, grid);
    } else {
      for (int j = 0; j < poly.num_vertices; j++) {
        poly.vertices[j].x = (int)((pl_list->vertices.data[i * 8 + j].x - model->origin.x) / ini.face_merge_grid_tile_size);
        poly.vertices[j].y = (int)((pl_list->vertices.data[i * 8 + j].y - model->origin.y) / ini.face_merge_grid_tile_size);
        poly.vertices[j].z = (int)((pl_list->vertices.data[i * 8 + j].z - model->origin.z) / ini.face_merge_grid_tile_size);
      }
      
      make_poly_aabb(&grid_pl_list_aabb_i[i], &poly);
      set_map_list(i, &grid_pl_list_aabb_i[i], grid_list, grid);
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

void check_map(int face_id, pl_list_t *pl_list, model_t *model) {
  //visited_faces = calloc(model->num_faces, 1);
  memset(visited_faces, 0, pl_list->num_faces);
  visited_faces[face_id] = 1;
  aabb_t grid_aabb = grid_pl_list_aabb_i[face_id]; // grid aabb
  tiled_faces = 0;
  
  for (int i = grid_aabb.min.z; i <= grid_aabb.max.z; i++) {
    for (int j = grid_aabb.min.y; j <= grid_aabb.max.y; j++) {
      for (int k = grid_aabb.min.x; k <= grid_aabb.max.x; k++) {
        int map_pnt = j * face_grid.size_i.d * face_grid.size_i.w + i * face_grid.size_i.w + k;
        check_face_map(map_pnt, face_id, &face_grid_list, &face_grid, pl_list, model);
      }
    }
  }
}

// iterate every face on the grid tile and compare them against the passed one

void check_face_map(int map_pnt, int face_id, d_lnk_ls_t *grid_list, grid_t *grid, pl_list_t *pl_list, model_t *model) {
  int list_pnt = grid->data_pnt[map_pnt];
  
  while (list_pnt >= 0) {
    invalid_face = 0; // debug
    int map_face_id = grid_list->nodes.data[list_pnt].data;
    
    if (!visited_faces[map_face_id] && !removed_faces[map_face_id]) {
      check_face_merge(face_id, map_face_id, pl_list, model);
      visited_faces[map_face_id] = 1;
    }
    
    list_pnt = grid_list->nodes.data[list_pnt].next;
  }
}

// test if the two faces can be merged

void check_face_merge(int face0_id, int face1_id, pl_list_t *pl_list, model_t *model) {
  
  // if the materials are not shared return
  if (model->face_materials.data[face0_id] != model->face_materials.data[face1_id]) return;
  
  // memset(pl_shared_vt_list, -1, sizeof(pl_shared_vt_list));
  vec3_t nm = snapped_normals[face0_id];
  vec3_t nm2 = snapped_normals[face1_id];
  
  //if the normals doesn't coincide return
  if (nm.x != nm2.x || nm.y != nm2.y || nm.z != nm2.z) return;
  
  u8 textured_face;
  if (model->face_types.data[face0_id] & UNTEXTURED) {
    textured_face = 0;
  } else {
    textured_face = 1;
  }
  
  edge_t shared_face_0, shared_face_1;
  int shared_vertices = 0;
  int shared_mod_txcoords = 0;
  int shared_vt_txcoords = 0;
  
  // for each vertex in the first face
  for (int i = 0; i < pl_list->face_num_vertices.data[face0_id]; i++) {
    vec3_t vt_face0 = pl_list->vertices.data[face0_id * 8 + i];
    
    // for each vertex in the second face
    for (int j = 0; j < pl_list->face_num_vertices.data[face1_id]; j++) {
      vec3_t vt_face1 = pl_list->vertices.data[face1_id * 8 + j];
      
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
        
        vec2_tx_t txcoord_face0 = pl_list->txcoords.data[face0_id * 8 + i];
        vec2_tx_t txcoord_face1 = pl_list->txcoords.data[face1_id * 8 + j];
        
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
  if (shared_vertices == model->face_num_vertices.data[face0_id]) return;
  
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
      pl_list_set_poly(face1_id, &poly, pl_list);
      make_poly_aabb(&aabb_1, &poly);
      
      // obtain the size of the two faces
      size3_t face;
      face.w = max_c(aabb_0.max.x, aabb_1.max.x) - min_c(aabb_0.min.x, aabb_1.min.x);
      face.h = max_c(aabb_0.max.y, aabb_1.max.y) - min_c(aabb_0.min.y, aabb_1.min.y);
      face.d = max_c(aabb_0.max.z, aabb_1.max.z) - min_c(aabb_0.min.z, aabb_1.min.z);
      
      // if the size of the merged face is smaller or equal than the allowed distance then add the edge
      if (face.w <= ini.face_merge_dist_f && face.d <= ini.face_merge_dist_f && face.h <= ini.face_merge_dist_f) {
        // merge_shared_faces(face0_id, face1_id, pl_list, model);
        add_edge_to_list = 1;
      }
    } else {
      // merge_shared_faces(face0_id, face1_id, pl_list, model);
      add_edge_to_list = 1;
    }
  }
  
  if (add_edge_to_list) {
    vec3_t vt0 = pl_list->vertices.data[face0_id * 8 + shared_face_0.vt0];
    vec3_t vt1 = pl_list->vertices.data[face0_id * 8 + shared_face_0.vt1];
    
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

void merge_faces(int face0_id, u8 edge_id, d_lnk_ls_t *grid_list, grid_t *grid, pl_list_t *pl_list, model_t *model) {
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
  
  merge_shared_faces(face0_id, face1_id, shared_vt0_id, grid_list, grid, pl_list, model);
}

// remove any unreferenced vertices

void remove_extra_vertices(model_t *model) {
  int final_num_vt = 0;
  
  for (int i = 0; i < model->num_vertices; i++) {
    int used_vt = 0;
    
    for (int j = 0; j < model->faces_size; j++) {
      if (model->faces.data[j] == i) {
        used_vt = 1;
        break;
      }
    }
    
    if (used_vt) {
      if (i != final_num_vt) {
        // move the next vertices to the empty spaces
        model->vertices.data[final_num_vt].x = model->vertices.data[i].x;
        model->vertices.data[final_num_vt].y = model->vertices.data[i].y;
        model->vertices.data[final_num_vt].z = model->vertices.data[i].z;
        
        // update the faces
        for (int j = 0; j < model->faces_size; j++) {
          if (model->faces.data[j] == i) {
            model->faces.data[j] = final_num_vt;
          }
        }
      }
      
      final_num_vt++;
    }
  }
  
  model->num_vertices = final_num_vt;
  model->vertices.size = final_num_vt;
}

// remove any unreferenced texture coords

void remove_extra_txcoords(model_t *model) {
  int final_num_vt = 0;
  
  for (int i = 0; i < model->num_txcoords; i++) {
    int used_vt = 0;
    
    for (int j = 0; j < model->tx_faces_size; j++) {
      if (model->tx_faces.data[j] == i) {
        used_vt = 1;
        break;
      }
    }
    
    if (used_vt) {
      if (i != final_num_vt) {
        // move the next texture coords to the empty spaces
        model->txcoords.data[final_num_vt].u = model->txcoords.data[i].u;
        model->txcoords.data[final_num_vt].v = model->txcoords.data[i].v;
        
        // update the faces
        for (int j = 0; j < model->tx_faces_size; j++) {
          if (model->tx_faces.data[j] == i) {
            model->tx_faces.data[j] = final_num_vt;
          }
        }
      }
      
      final_num_vt++;
    }
  }
  
  model->num_txcoords = final_num_vt;
  model->txcoords.size = final_num_vt;
}