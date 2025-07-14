#include "common.h"

void check_vt_grid(d_lnk_ls_t *list, grid_t *grid, model_t *model);
void check_grid_vt_list(int vt_id, aabb_t *aabb, d_lnk_ls_t *list, grid_t *grid, model_t *model);
void check_vertex_map(int x, int y, int z, int vt_id, d_lnk_ls_t *list, grid_t *grid, model_t *model);
int check_vertex_merge(int vt0_id, int vt1_id, model_t *model);

static grid_t grid_vt;
static d_lnk_ls_t grid_list_vt;
static vec3_t *snapped_vertices;
static u8 *removed_vertices;

void merge_mdl_vertices(model_t *model) {
  make_vt_grid(&grid_list_vt, &grid_vt, model);
  
  removed_vertices = calloc(model->num_vertices, 1);
  snapped_vertices = malloc(model->num_vertices * sizeof(vec3_t));
  
  for (int i = 0; i < model->num_vertices; i++) {
    snapped_vertices[i].x = (int)(model->vertices.data[i].x / ini.merge_vt_dist_f);
    snapped_vertices[i].y = (int)(model->vertices.data[i].y / ini.merge_vt_dist_f);
    snapped_vertices[i].z = (int)(model->vertices.data[i].z / ini.merge_vt_dist_f);
  }
  
  // check and merge the vertices 
  check_vt_grid(&grid_list_vt, &grid_vt, model);
  
  // remove the merged vertices
  int final_num_vt = 0;
  for (int i = 0; i < model->num_vertices; i++) {
    if (removed_vertices[i]) continue;
    
    if (i != final_num_vt) {
      model->vertices.data[final_num_vt] = model->vertices.data[i];
      
      // update the faces
      for (int j = 0; j < model->faces_size; j++) {
        if (model->faces.data[j] == i) {
          model->faces.data[j] = final_num_vt;
        }
      }
    }
    
    final_num_vt++;
  }
  
  model->num_vertices = final_num_vt;
  model->vertices.size = final_num_vt;
  
  free(snapped_vertices);
  free(removed_vertices);
  free_lnk_ls_grid(&grid_list_vt, &grid_vt);
}

// makes a grid that stores every vertex of the model on it, used to optimize the vertex search

void make_vt_grid(d_lnk_ls_t *list, grid_t *grid, model_t *model) {
  grid->size_i.w = ceilf(model->size.w / ini.face_merge_grid_tile_size);
  grid->size_i.h = ceilf(model->size.h / ini.face_merge_grid_tile_size);
  grid->size_i.d = ceilf(model->size.d / ini.face_merge_grid_tile_size);
  
  grid->size_i.w = max_c(grid->size_i.w, 1);
  grid->size_i.h = max_c(grid->size_i.h, 1);
  grid->size_i.d = max_c(grid->size_i.d, 1);
  
  malloc_d_lnk_ls_grid(list, grid);
  
  for (int i = 0; i < model->num_vertices; i++) {
    // if (model->type_vt[i] & SPRITE) continue;
    
    vec3_t vt;
    vt.x = (int)((model->vertices.data[i].x - model->origin.x) / ini.face_merge_grid_tile_size);
    vt.y = (int)((model->vertices.data[i].y - model->origin.y) / ini.face_merge_grid_tile_size);
    vt.z = (int)((model->vertices.data[i].z - model->origin.z) / ini.face_merge_grid_tile_size);
    
    vt.x = clamp_i(vt.x, 0, grid->size_i.w - 1);
    vt.y = clamp_i(vt.y, 0, grid->size_i.h - 1);
    vt.z = clamp_i(vt.z, 0, grid->size_i.d - 1);
    
    add_grid_d_lnk_ls_element(vt.x, vt.y, vt.z, i, list, grid);
  }
}

void check_vt_grid(d_lnk_ls_t *list, grid_t *grid, model_t *model) {
  aabb_t aabb;
  
  for (int i = 0; i < model->num_vertices; i++) {
    if (removed_vertices[i]) continue;
    
    vec3_t vt;
    vt.x = model->vertices.data[i].x;
    vt.y = model->vertices.data[i].y;
    vt.z = model->vertices.data[i].z;
    
    aabb.min.x = (int)((vt.x - model->origin.x - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    aabb.max.x = (int)((vt.x - model->origin.x + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    aabb.min.y = (int)((vt.y - model->origin.y - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    aabb.max.y = (int)((vt.y - model->origin.y + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    aabb.min.z = (int)((vt.z - model->origin.z - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    aabb.max.z = (int)((vt.z - model->origin.z + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    
    aabb.min.x = clamp_i(aabb.min.x, 0, grid->size_i.w - 1);
    aabb.max.x = clamp_i(aabb.max.x, 0, grid->size_i.w - 1);
    aabb.min.y = clamp_i(aabb.min.y, 0, grid->size_i.h - 1);
    aabb.max.y = clamp_i(aabb.max.y, 0, grid->size_i.h - 1);
    aabb.min.z = clamp_i(aabb.min.z, 0, grid->size_i.d - 1);
    aabb.max.z = clamp_i(aabb.max.z, 0, grid->size_i.d - 1);
    
    check_grid_vt_list(i, &aabb, list, grid, model);
  }
}

// checks the passed aabb against every tile it touches

void check_grid_vt_list(int vt_id, aabb_t *aabb, d_lnk_ls_t *list, grid_t *grid, model_t *model) {
  for (int i = aabb->min.z; i <= aabb->max.z; i++) {
    for (int j = aabb->min.y; j <= aabb->max.y; j++) {
      for (int k = aabb->min.x; k <= aabb->max.x; k++) {
        check_vertex_map(k, j, i, vt_id, list, grid, model);
      }
    }
  }
}

// iterate every vertex on the grid tile and compare them against the passed one

void check_vertex_map(int x, int y, int z, int vt_id, d_lnk_ls_t *list, grid_t *grid, model_t *model) {
  int map_pnt = y * grid->size_i.d * grid->size_i.w + z * grid->size_i.w + x;
  int list_pnt = grid->data_pnt[map_pnt];
  
  while (list_pnt >= 0) {
    int map_vt_id = list->nodes.data[list_pnt].data;
    int list_pnt_next = list->nodes.data[list_pnt].next;
    
    if (!removed_vertices[map_vt_id] && vt_id != map_vt_id) {
      if (check_vertex_merge(vt_id, map_vt_id, model)) {
        // save the next pointer before deallocating the nodes
        // remove the vertex from the linked list
        remove_grid_lnk_list_element(list_pnt, list, grid);
        removed_vertices[map_vt_id] = 1;
      }
    }
    
    list_pnt = list_pnt_next;
  }
}

// test if the two vertices can be merged

int check_vertex_merge(int vt0_id, int vt1_id, model_t *model) {
  vec3_t vt0 = snapped_vertices[vt0_id];
  vec3_t vt1 = snapped_vertices[vt1_id];
  
  if (vt0.x == vt1.x && vt0.y == vt1.y && vt0.z == vt1.z) {
    // merge vertices by updating the faces
    for (int i = 0; i < model->faces_size; i++) {
      if (model->faces.data[i] == vt1_id) {
        model->faces.data[i] = vt0_id;
      }
    }
    
    return 1;
  }
  
  return 0;
}