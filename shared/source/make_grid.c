#include "common.h"

void init_obj_grid(obj_grid_t *grid, object_t *object);
void malloc_ln_grid_list(obj_grid_ln_t *grid_ln, obj_grid_t *grid);
void grid_lnk_ls_to_array(obj_grid_ln_t *grid_ln, obj_grid_t *grid);
void make_grid_vt_list(obj_grid_ln_t *grid_ln, object_t *object);

// <make scene grid code>

// creates a grid for the object, used for frustum culling

void make_scn_grid(object_t *object) {
  obj_grid_t grid_scn;
  
  init_obj_grid(&grid_scn, object);
  
  int model_num_faces = object->num_faces;
  
  // clip the faces
  
  for (int i = 0; i < model_num_faces; i++) {
    poly_t poly;
    aabb_t aabb;
    
    if (object->faces.data[i].type & SPRITE) continue;
    
    poly.num_vertices = object->faces.data[i].num_vertices;
    
    for (int j = 0; j < poly.num_vertices; j++) {
      poly.vertices[j].x = (object->faces.data[i].vertices[j].x - object->origin.x) / grid_scn.tile_size_i;
      poly.vertices[j].y = (object->faces.data[i].vertices[j].y - object->origin.y) / grid_scn.tile_size_i;
      poly.vertices[j].z = (object->faces.data[i].vertices[j].z - object->origin.z) / grid_scn.tile_size_i;
    }
    
    make_poly_aabb(&aabb, &poly);
    adjust_poly_tile_size(&aabb);
    
    // if the face is shared between the nodes clip it
    if (aabb.min.x != aabb.max.x || aabb.min.y != aabb.max.y || aabb.min.z != aabb.max.z) {
      grid_face_clipping(i, &aabb, &grid_scn, object);
    }
  }
  
  // remove the clipped faces
  remove_object_marked_faces(object);
  
  // remove_extra_vertices(object);
  // remove_extra_txcoords(object);
  // merge_obj_vertices(object);
  // merge_obj_txcoords(object);
  
  // create the grid
  
  for (int i = 0; i < object->num_faces; i++) {
    vec3_t min_vt;
    min_vt.x = object->faces.data[i].vertices[0].x;
    min_vt.y = object->faces.data[i].vertices[0].y;
    min_vt.z = object->faces.data[i].vertices[0].z;
    
    if (!(object->faces.data[i].type & SPRITE)) {
      for (int j = 1; j < object->faces.data[i].num_vertices; j++) {
        min_vt.x = min_c(min_vt.x, object->faces.data[i].vertices[j].x);
        min_vt.y = min_c(min_vt.y, object->faces.data[i].vertices[j].y);
        min_vt.z = min_c(min_vt.z, object->faces.data[i].vertices[j].z);
      }
    }
    
    // grid face position
    vec3i_t grid_pos_i;
    grid_pos_i.x = (int)((min_vt.x - object->origin.x) / grid_scn.tile_size_i);
    grid_pos_i.y = (int)((min_vt.y - object->origin.y) / grid_scn.tile_size_i);
    grid_pos_i.z = (int)((min_vt.z - object->origin.z) / grid_scn.tile_size_i);
    
    grid_pos_i.x = clamp_i(grid_pos_i.x, 0, grid_scn.size_i.w - 1);
    grid_pos_i.y = clamp_i(grid_pos_i.y, 0, grid_scn.size_i.h - 1);
    grid_pos_i.z = clamp_i(grid_pos_i.z, 0, grid_scn.size_i.d - 1);
    
    // tile pointer for grid
    int tile_pnt = grid_pos_i.y * grid_scn.size_i.d * grid_scn.size_i.w + grid_pos_i.z * grid_scn.size_i.w + grid_pos_i.x;
    
    // adds the face to the grid face list
    add_mdl_grid_lnk_ls_element(tile_pnt, i, &grid_scn.pl_data, &grid_scn);
    
    grid_scn.tile_num_faces[tile_pnt]++;
  }
  
  malloc_ln_grid_list(&object->grid_ln, &grid_scn);
  grid_lnk_ls_to_array(&object->grid_ln, &grid_scn);
  make_grid_vt_list(&object->grid_ln, object);
  object->has_grid = 1;
  
  free_obj_grid(&grid_scn);
}

void init_obj_grid(obj_grid_t *grid, object_t *object) {
  grid->tile_size_bits = ini.grid_tile_size_bits;
  grid->tile_size_i = 1 << ini.grid_tile_size_bits;
  
  grid->size_i.w = ceilf(object->size.w / grid->tile_size_i);
  grid->size_i.h = ceilf(object->size.h / grid->tile_size_i);
  grid->size_i.d = ceilf(object->size.d / grid->tile_size_i);
  
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

// </make scene grid code>

// creates the linear version of the grid structure used for exporting

void malloc_ln_grid_list(obj_grid_ln_t *grid_ln, obj_grid_t *grid) {
  grid_ln->pl_pnt = malloc(grid->num_tiles * sizeof(*grid_ln->pl_pnt));
  grid_ln->vt_pnt = malloc(grid->num_tiles * sizeof(*grid_ln->vt_pnt));
  
  init_list(&grid_ln->pl_data, sizeof(*grid_ln->pl_data.data));
  init_list(&grid_ln->vt_data, sizeof(*grid_ln->vt_data.data));
  
  grid_ln->size_i = grid->size_i;
  grid_ln->num_tiles = grid->num_tiles;
  grid_ln->tile_size_i = grid->tile_size_i;
  grid_ln->tile_size_bits = grid->tile_size_bits;
}

void free_ln_grid_list(obj_grid_ln_t *grid_ln) {
  free(grid_ln->pl_pnt);
  free(grid_ln->vt_pnt);
  
  free_list(&grid_ln->pl_data);
  free_list(&grid_ln->vt_data);
}

// turns the linked list from the grid structure into a linear array stored in a separate structure used for exporting

void grid_lnk_ls_to_array(obj_grid_ln_t *grid_ln, obj_grid_t *grid) {
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

void make_grid_vt_list(obj_grid_ln_t *grid_ln, object_t *object) {
  int *visited_vt = malloc(object->num_vertices * sizeof(int));
  
  for (int i = 0; i < grid_ln->num_tiles; i++) {
    int pl_pnt = grid_ln->pl_pnt[i];
    
    if (pl_pnt < 0) {
      grid_ln->vt_pnt[i] = -1;
      continue;
    }
    
    // reset the traversed vertices
    memset32(visited_vt, 0, object->num_vertices);
    
    grid_ln->vt_pnt[i] = grid_ln->vt_data.size;
    int pl_id = grid_ln->pl_data.data[pl_pnt];
    
    while (pl_id >= 0) {
      if (object->faces.data[pl_id].type & SPRITE) {
        int vt_id = object->faces.data[pl_id].vt_index[0];
        list_push_int(&grid_ln->vt_data, vt_id);
      } else {
        for (int j = 0; j < object->faces.data[pl_id].num_vertices; j++) {
          int vt_id = object->faces.data[pl_id].vt_index[j];
          
          if (!visited_vt[vt_id]) {
            list_push_int(&grid_ln->vt_data, vt_id);
            visited_vt[vt_id] = 1;
          }
        }
      }
      
      pl_pnt++;
      pl_id = grid_ln->pl_data.data[pl_pnt];
    }
    
    list_push_int(&grid_ln->vt_data, -1);
  }
  
  free(visited_vt);
}