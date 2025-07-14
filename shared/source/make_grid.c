#include "common.h"

void init_mdl_grid(mdl_grid_t *grid, model_t *model);
void remove_clipped_faces(model_t *model);
void update_face_data_grid(int dest, int src, model_t *model);
void malloc_ln_grid_list(mdl_grid_ln_t *grid_ln, mdl_grid_t *grid);
void grid_lnk_ls_to_array(mdl_grid_ln_t *grid_ln, mdl_grid_t *grid);
void make_grid_vt_list(mdl_grid_ln_t *grid_ln, model_t *model);
void make_grid_aabb_list(mdl_grid_ln_t *grid_ln, model_t *model);

list_u8 grid_removed_faces;

// <make scene grid code>

// creates a grid for the model, used for frustum culling in maps

void make_scn_grid(model_t *model) {
  mdl_grid_t grid_scn;
  
  init_mdl_grid(&grid_scn, model);
  
  init_list(&grid_removed_faces, sizeof(*grid_removed_faces.data));
  list_malloc_size(&grid_removed_faces, model->num_faces);
  memset(grid_removed_faces.data, 0, model->num_faces);
  
  int model_num_faces = model->num_faces;
  
  // clip the faces
  
  for (int i = 0; i < model_num_faces; i++) {
    poly_t poly;
    aabb_t aabb;
    if (model->face_types.data[i] & SPRITE) continue;
    poly.num_vertices = model->face_num_vertices.data[i];
    
    for (int j = 0; j < poly.num_vertices; j++) {
      int pt = model->faces.data[model->face_index.data[i] + j];
      poly.vertices[j].x = (model->vertices.data[pt].x - model->origin.x) / grid_scn.tile_size_i;
      poly.vertices[j].y = (model->vertices.data[pt].y - model->origin.y) / grid_scn.tile_size_i;
      poly.vertices[j].z = (model->vertices.data[pt].z - model->origin.z) / grid_scn.tile_size_i;
      
      // poly_oc.vertices[j].x = clamp_i(model->vertices.data[pt].x, 0, grid_size - 1);
      // poly_oc.vertices[j].y = clamp_i(model->vertices.data[pt].y, 0, grid_size - 1);
      // poly_oc.vertices[j].z = clamp_i(model->vertices.data[pt].z, 0, grid_size - 1);
    }
    
    make_poly_aabb(&aabb, &poly);
    adjust_poly_tile_size(&aabb);
    
    // if the face is shared between the nodes clip it
    if (aabb.min.x != aabb.max.x || aabb.min.y != aabb.max.y || aabb.min.z != aabb.max.z) {
      grid_face_clipping(i, &aabb, &grid_scn, model);
    }
  }
  
  // remove the clipped faces
  remove_clipped_faces(model);
  
  remove_extra_vertices(model);
  remove_extra_txcoords(model);
  merge_mdl_vertices(model);
  // merge_mdl_txcoords(model);
  set_face_index(model);
  set_tx_face_index(model);
  
  // create the grid
  
  for (int i = 0; i < model->num_faces; i++) {
    vec3_t min_vt;
    int pt = model->faces.data[model->face_index.data[i]];
    min_vt.x = model->vertices.data[pt].x;
    min_vt.y = model->vertices.data[pt].y;
    min_vt.z = model->vertices.data[pt].z;
    
    if (!model->num_sprites || !(model->face_types.data[i] & SPRITE)) {
      for (int j = 1; j < model->face_num_vertices.data[i]; j++) {
        pt = model->faces.data[model->face_index.data[i] + j];
        min_vt.x = min_c(min_vt.x, model->vertices.data[pt].x);
        min_vt.y = min_c(min_vt.y, model->vertices.data[pt].y);
        min_vt.z = min_c(min_vt.z, model->vertices.data[pt].z);
      }
    }
    
    // grid face position
    vec3i_t grid_pos_i;
    grid_pos_i.x = (int)((min_vt.x - model->origin.x) / grid_scn.tile_size_i);
    grid_pos_i.y = (int)((min_vt.y - model->origin.y) / grid_scn.tile_size_i);
    grid_pos_i.z = (int)((min_vt.z - model->origin.z) / grid_scn.tile_size_i);
    
    grid_pos_i.x = clamp_i(grid_pos_i.x, 0, grid_scn.size_i.w - 1);
    grid_pos_i.y = clamp_i(grid_pos_i.y, 0, grid_scn.size_i.h - 1);
    grid_pos_i.z = clamp_i(grid_pos_i.z, 0, grid_scn.size_i.d - 1);
    
    // tile pointer for grid
    int tile_pnt = grid_pos_i.y * grid_scn.size_i.d * grid_scn.size_i.w + grid_pos_i.z * grid_scn.size_i.w + grid_pos_i.x;
    
    // adds the face to the grid face list
    add_mdl_grid_lnk_ls_element(tile_pnt, i, &grid_scn.pl_data, &grid_scn);
    
    grid_scn.tile_num_faces[tile_pnt]++;
  }
  
  malloc_ln_grid_list(&grid_scn_ln, &grid_scn);
  grid_lnk_ls_to_array(&grid_scn_ln, &grid_scn);
  make_grid_vt_list(&grid_scn_ln, model);
  make_grid_aabb_list(&grid_scn_ln, model);
  model->has_grid = 1;
  
  free_mdl_grid(&grid_scn);
}

void init_mdl_grid(mdl_grid_t *grid, model_t *model) {
  grid->tile_size_bits = ini.grid_tile_size_bits;
  grid->tile_size_i = 1 << ini.grid_tile_size_bits;
  
  grid->size_i.w = ceilf(model->size.w / grid->tile_size_i);
  grid->size_i.h = ceilf(model->size.h / grid->tile_size_i);
  grid->size_i.d = ceilf(model->size.d / grid->tile_size_i);
  
  grid->size_i.w = max_c(grid->size_i.w, 1);
  grid->size_i.h = max_c(grid->size_i.h, 1);
  grid->size_i.d = max_c(grid->size_i.d, 1);
  
  malloc_mdl_grid(grid);
}

// floors the aabb and keeps it to the tile boundaries in the case it touches a nearest tile

void adjust_poly_tile_size(aabb_t *aabb) {
  if (aabb->min.x < aabb->max.x) {
    aabb->max.x = ceilf(aabb->max.x) - 1;
  } else {
    aabb->max.x = (int)aabb->max.x;
  }
  
  if (aabb->min.y < aabb->max.y) {
    aabb->max.y = ceilf(aabb->max.y) - 1;
  } else {
    aabb->max.y = (int)aabb->max.y;
  }
  
  if (aabb->min.z < aabb->max.z) {
    aabb->max.z = ceilf(aabb->max.z) - 1;
  } else {
    aabb->max.z = (int)aabb->max.z;
  }
  
  aabb->min.x = (int)aabb->min.x;
  aabb->min.y = (int)aabb->min.y;
  aabb->min.z = (int)aabb->min.z;
}

// removes the original clipped faces from the model

void remove_clipped_faces(model_t *model) {
  int num_faces = 0;
  int faces_size = 0;
  int tx_faces_size = 0;
  
  for (int i = 0; i < model->num_faces; i++) {
    if (grid_removed_faces.data[i]) continue;
    
    if (i != num_faces) {
      if (model->num_sprites && model->face_types.data[i] & SPRITE) {
        model->faces.data[faces_size] = model->faces.data[model->face_index.data[i]];
        
        for (int j = 0; j < 4; j++) {
          model->tx_faces.data[tx_faces_size] = model->tx_faces.data[model->tx_face_index.data[i] + j];
          tx_faces_size++;
        }
        
        faces_size++;
      } else {
        for (int j = 0; j < model->face_num_vertices.data[i]; j++) {
          model->faces.data[faces_size] = model->faces.data[model->face_index.data[i] + j];
          model->tx_faces.data[tx_faces_size] = model->tx_faces.data[model->tx_face_index.data[i] + j];
          faces_size++;
          tx_faces_size++;
        }
      }
      
      update_face_data_grid(num_faces, i, model);
    } else {
      if (model->num_sprites && model->face_types.data[i] & SPRITE) {
        faces_size++;
      } else {
        faces_size += model->face_num_vertices.data[i];
      }
      tx_faces_size += model->face_num_vertices.data[i];
    }
    
    num_faces++;
  }
  
  model->num_faces = num_faces;
  model->faces_size = faces_size;
  model->num_tx_faces = num_faces;
  model->tx_faces_size = tx_faces_size;
  model->num_objects = 0;
  
  // update the list sizes
  
  model->faces.size = faces_size;
  model->tx_faces.size = tx_faces_size;
  model->face_index.size = num_faces;
  model->tx_face_index.size = num_faces;
  model->normals.size = num_faces;
  model->face_num_vertices.size = num_faces;
  model->face_materials.size = num_faces;
  model->face_types.size = num_faces;
  model->sprite_face_index.size = num_faces;
  
  free_list(&grid_removed_faces);
}

inline void update_face_data_grid(int dest, int src, model_t *model) {
  model->normals.data[dest] = model->normals.data[src];
  model->face_num_vertices.data[dest] = model->face_num_vertices.data[src];
  model->face_materials.data[dest] = model->face_materials.data[src];
  model->face_types.data[dest] = model->face_types.data[src];
  
  if (model->num_sprites) {
    model->sprite_face_index.data[dest] = model->sprite_face_index.data[src];
  }
}

// </make scene grid code>

// creates the linear version of the grid structure used for exporting

void malloc_ln_grid_list(mdl_grid_ln_t *grid_ln, mdl_grid_t *grid) {
  grid_ln->pl_pnt = malloc(grid->num_tiles * sizeof(*grid_ln->pl_pnt));
  grid_ln->vt_pnt = malloc(grid->num_tiles * sizeof(*grid_ln->vt_pnt));
  // grid_ln->grid_pnt = malloc(grid->num_tiles * sizeof(*grid_ln->grid_pnt));
  
  init_list(&grid_ln->pl_data, sizeof(*grid_ln->pl_data.data));
  init_list(&grid_ln->vt_data, sizeof(*grid_ln->vt_data.data));
  // init_list(&grid_ln->grid_aabb, sizeof(*grid_ln->grid_aabb.data));
  
  grid_ln->size_i = grid->size_i;
  grid_ln->num_tiles = grid->num_tiles;
  grid_ln->tile_size_i = grid->tile_size_i;
  grid_ln->tile_size_bits = grid->tile_size_bits;
}

void free_ln_grid_list(mdl_grid_ln_t *grid_ln) {
  free(grid_ln->pl_pnt);
  free(grid_ln->vt_pnt);
  // free(grid_ln->grid_pnt);
  
  free_list(&grid_ln->pl_data);
  free_list(&grid_ln->vt_data);
  // free_list(&grid_ln->grid_aabb);
}

// turns the linked list from the grid structure into a linear array stored in a separate structure used for exporting

void grid_lnk_ls_to_array(mdl_grid_ln_t *grid_ln, mdl_grid_t *grid) {
  for (int i = 0; i < grid->num_tiles; i++) {
    int pl_pnt = grid->pl_pnt[i];
    
    if (pl_pnt < 0) {
      grid_ln->pl_pnt[i] = -1;
      continue;
    }
    
    grid_ln->pl_pnt[i] = grid_ln->pl_data.size;
    
    // iterate the linked list
    while (pl_pnt >= 0) {
      list_push_int(&grid_ln->pl_data, grid->pl_data.nodes.data[pl_pnt].data);
      pl_pnt = grid->pl_data.nodes.data[pl_pnt].next;
    }
    
    list_push_int(&grid_ln->pl_data, -1);
  }
}

// creates a vertex list for each tile in the main grid

void make_grid_vt_list(mdl_grid_ln_t *grid_ln, model_t *model) {
  int *visited_vt = malloc(model->num_vertices * sizeof(int));
  // memset(visited_vt, 0, model->num_vertices);
  
  for (int i = 0; i < grid_ln->num_tiles; i++) {
    int pl_pnt = grid_ln->pl_pnt[i];
    
    if (pl_pnt < 0) {
      grid_ln->vt_pnt[i] = -1;
      continue;
    }
    
    // reset the traversed vertices
    memset32(visited_vt, -1, model->num_vertices);
    
    grid_ln->vt_pnt[i] = grid_ln->vt_data.size;
    int pl_id = grid_ln->pl_data.data[pl_pnt];
    
    while (pl_id >= 0) {
      if (model->num_sprites && model->face_types.data[pl_id] & SPRITE) {
        int vt_id = model->faces.data[model->face_index.data[pl_id]];
        list_push_int(&grid_ln->vt_data, vt_id);
      } else {
        for (int j = 0; j < model->face_num_vertices.data[pl_id]; j++) {
          int vt_id = model->faces.data[model->face_index.data[pl_id] + j];
          
          if (visited_vt[vt_id] < 0) {
            list_push_int(&grid_ln->vt_data, vt_id);
            visited_vt[vt_id] = 1;
          }
        }
      }
      
      pl_pnt++;
      pl_id = grid_ln->pl_data.data[pl_pnt];
    }
    
    list_push_int(&grid_ln->vt_data, -1);
    
    /* for (int j = grid_ln->vt_pnt[i]; j < list_ln->vt_data.size - 1; j++) {
      visited_vt[list_ln->vt_data.data[j]] = 0;
    } */
  }
  
  free(visited_vt);
}

void make_grid_aabb_list(mdl_grid_ln_t *grid_ln, model_t *model) {
  int tile_cnt = 0;
  
  for (int i = 0; i < grid_ln->num_tiles; i++) {
    aabb_t aabb;
    int vt_pnt = grid_ln->vt_pnt[i];
    
    // if there's no content in the current tile
    if (vt_pnt < 0) {
      // grid_ln->grid_pnt[i] = -1;
      continue;
    }
    
    // make the tile aabb
    
    int vt_id = grid_ln->vt_data.data[vt_pnt];
    
    aabb.min.x = model->vertices.data[vt_id].x;
    aabb.min.y = model->vertices.data[vt_id].y;
    aabb.min.z = model->vertices.data[vt_id].z;
    aabb.max.x = model->vertices.data[vt_id].x;
    aabb.max.y = model->vertices.data[vt_id].y;
    aabb.max.z = model->vertices.data[vt_id].z;
    
    vt_pnt++;
    vt_id = grid_ln->vt_data.data[vt_pnt];
    
    while (vt_id >= 0) {
      aabb.min.x = min_c(aabb.min.x, model->vertices.data[vt_id].x);
      aabb.min.y = min_c(aabb.min.y, model->vertices.data[vt_id].y);
      aabb.min.z = min_c(aabb.min.z, model->vertices.data[vt_id].z);
      aabb.max.x = max_c(aabb.max.x, model->vertices.data[vt_id].x);
      aabb.max.y = max_c(aabb.max.y, model->vertices.data[vt_id].y);
      aabb.max.z = max_c(aabb.max.z, model->vertices.data[vt_id].z);
      
      vt_pnt++;
      vt_id = grid_ln->vt_data.data[vt_pnt];
    }
    
    // grid_ln->grid_pnt[i] = tile_cnt;
    // list_push_pnt(&grid_ln->grid_aabb, &aabb);
    tile_cnt++;
  }
}