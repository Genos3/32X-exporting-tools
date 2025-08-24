#include "common.h"

void check_vt_grid(d_lnk_ls_t *list, grid_t *grid, object_t *object);
void check_grid_vt_list(int vt_id, aabb_t *aabb, d_lnk_ls_t *list, grid_t *grid, object_t *object);
void check_vertex_map(int x, int y, int z, int vt_id, d_lnk_ls_t *list, grid_t *grid, object_t *object);
int check_vertex_merge(int vt0_id, int vt1_id, object_t *object);

static grid_t grid_vt;
static d_lnk_ls_t grid_list_vt;
static vec3_t *snapped_vertices;
static u8 *removed_vertices;

void merge_obj_vertices(object_t *object) {
  make_vt_grid(&grid_list_vt, &grid_vt, object);
  
  removed_vertices = calloc(object->num_vertices, 1);
  snapped_vertices = malloc(object->num_vertices * sizeof(vec3_t));
  
  for (int i = 0; i < object->num_vertices; i++) {
    snapped_vertices[i].x = (int)(object->vertices.data[i].x / ini.merge_vt_dist_f);
    snapped_vertices[i].y = (int)(object->vertices.data[i].y / ini.merge_vt_dist_f);
    snapped_vertices[i].z = (int)(object->vertices.data[i].z / ini.merge_vt_dist_f);
  }
  
  // check and merge the vertices 
  check_vt_grid(&grid_list_vt, &grid_vt, object);
  
  // remove the merged vertices
  int num_vertices = 0;
  
  for (int i = 0; i < object->num_vertices; i++) {
    if (removed_vertices[i]) continue;
    
    if (i != num_vertices) {
      object->vertices.data[num_vertices] = object->vertices.data[i];
      
      // update the faces
      for (int j = 0; j < object->num_faces; j++) {
        face_t *face = &object->faces.data[j];
        
        for (int k = 0; k < face->num_vertices; k++) {
          if (face->vt_index[k] == i) {
            face->vt_index[k] = num_vertices;
          }
        }
      }
    }
    
    num_vertices++;
  }
  
  object->num_vertices = num_vertices;
  object->vertices.size = num_vertices;
  
  free(snapped_vertices);
  free(removed_vertices);
  free_lnk_ls_grid(&grid_list_vt, &grid_vt);
}

// makes a grid that stores every vertex of the model on it, used to optimize the vertex search

void make_vt_grid(d_lnk_ls_t *list, grid_t *grid, object_t *object) {
  grid->size_i.w = ceilf(object->size.w / ini.face_merge_grid_tile_size);
  grid->size_i.h = ceilf(object->size.h / ini.face_merge_grid_tile_size);
  grid->size_i.d = ceilf(object->size.d / ini.face_merge_grid_tile_size);
  
  grid->size_i.w = max_c(grid->size_i.w, 1);
  grid->size_i.h = max_c(grid->size_i.h, 1);
  grid->size_i.d = max_c(grid->size_i.d, 1);
  
  malloc_d_lnk_ls_grid(list, grid);
  
  for (int i = 0; i < object->num_vertices; i++) {
    // if (object->type_vt[i] & SPRITE) continue;
    
    vec3_t vt;
    vt.x = (int)((object->vertices.data[i].x - object->origin.x) / ini.face_merge_grid_tile_size);
    vt.y = (int)((object->vertices.data[i].y - object->origin.y) / ini.face_merge_grid_tile_size);
    vt.z = (int)((object->vertices.data[i].z - object->origin.z) / ini.face_merge_grid_tile_size);
    
    vt.x = clamp_i(vt.x, 0, grid->size_i.w - 1);
    vt.y = clamp_i(vt.y, 0, grid->size_i.h - 1);
    vt.z = clamp_i(vt.z, 0, grid->size_i.d - 1);
    
    add_grid_d_lnk_ls_element(vt.x, vt.y, vt.z, i, list, grid);
  }
}

void check_vt_grid(d_lnk_ls_t *list, grid_t *grid, object_t *object) {
  aabb_t aabb;
  
  for (int i = 0; i < object->num_vertices; i++) {
    if (removed_vertices[i]) continue;
    
    vec3_t vt;
    vt.x = object->vertices.data[i].x;
    vt.y = object->vertices.data[i].y;
    vt.z = object->vertices.data[i].z;
    
    aabb.min.x = (int)((vt.x - object->origin.x - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    aabb.max.x = (int)((vt.x - object->origin.x + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    aabb.min.y = (int)((vt.y - object->origin.y - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    aabb.max.y = (int)((vt.y - object->origin.y + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    aabb.min.z = (int)((vt.z - object->origin.z - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    aabb.max.z = (int)((vt.z - object->origin.z + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
    
    aabb.min.x = clamp_i(aabb.min.x, 0, grid->size_i.w - 1);
    aabb.max.x = clamp_i(aabb.max.x, 0, grid->size_i.w - 1);
    aabb.min.y = clamp_i(aabb.min.y, 0, grid->size_i.h - 1);
    aabb.max.y = clamp_i(aabb.max.y, 0, grid->size_i.h - 1);
    aabb.min.z = clamp_i(aabb.min.z, 0, grid->size_i.d - 1);
    aabb.max.z = clamp_i(aabb.max.z, 0, grid->size_i.d - 1);
    
    check_grid_vt_list(i, &aabb, list, grid, object);
  }
}

// checks the passed aabb against every tile it touches

void check_grid_vt_list(int vt_id, aabb_t *aabb, d_lnk_ls_t *list, grid_t *grid, object_t *object) {
  for (int i = aabb->min.z; i <= aabb->max.z; i++) {
    for (int j = aabb->min.y; j <= aabb->max.y; j++) {
      for (int k = aabb->min.x; k <= aabb->max.x; k++) {
        check_vertex_map(k, j, i, vt_id, list, grid, object);
      }
    }
  }
}

// iterate every vertex on the grid tile and compare them against the passed one

void check_vertex_map(int x, int y, int z, int vt_id, d_lnk_ls_t *list, grid_t *grid, object_t *object) {
  int map_pnt = y * grid->size_i.d * grid->size_i.w + z * grid->size_i.w + x;
  int list_pnt = grid->data_pnt[map_pnt];
  
  while (list_pnt >= 0) {
    int map_vt_id = list->nodes.data[list_pnt].data;
    int list_pnt_next = list->nodes.data[list_pnt].next;
    
    if (!removed_vertices[map_vt_id] && vt_id != map_vt_id) {
      if (check_vertex_merge(vt_id, map_vt_id, object)) {
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

int check_vertex_merge(int vt0_id, int vt1_id, object_t *object) {
  vec3_t vt0 = snapped_vertices[vt0_id];
  vec3_t vt1 = snapped_vertices[vt1_id];
  
  if (vt0.x == vt1.x && vt0.y == vt1.y && vt0.z == vt1.z) {
    // merge the vertices and update the faces
    for (int i = 0; i < object->num_faces; i++) {
      face_t *face = &object->faces.data[i];
      
      for (int j = 0; j < face->num_vertices; j++) {
        if (face->vt_index[j] == vt1_id) {
          face->vt_index[j] = vt0_id;
        }
      }
    }
    
    return 1;
  }
  
  return 0;
}