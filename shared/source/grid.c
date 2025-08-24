#include "common.h"

void malloc_lnk_ls_grid(lnk_ls_t *list, grid_t *grid) {
  grid->num_tiles = grid->size_i.w * grid->size_i.h * grid->size_i.d;
  
  grid->data_pnt = malloc(grid->num_tiles * sizeof(*grid->data_pnt));
  grid->data_last = calloc(grid->num_tiles * sizeof(*grid->data_last), 1);
  
  memset(grid->data_pnt, -1, grid->num_tiles * sizeof(*grid->data_pnt));
  
  init_list(&list->nodes, sizeof(*list->nodes.data));
  list->length = 0;
}

void malloc_d_lnk_ls_grid(d_lnk_ls_t *list, grid_t *grid) {
  grid->num_tiles = grid->size_i.w * grid->size_i.h * grid->size_i.d;
  
  grid->data_pnt = malloc(grid->num_tiles * sizeof(*grid->data_pnt));
  grid->data_last = calloc(grid->num_tiles * sizeof(*grid->data_last), 1);
  
  memset(grid->data_pnt, -1, grid->num_tiles * sizeof(*grid->data_pnt));
  
  init_list(&list->nodes, sizeof(*list->nodes.data));
  list->length = 0;
}

void free_lnk_ls_grid(void *list, grid_t *grid) {
  free(grid->data_pnt);
  free(grid->data_last);
  free_list(list);
}

void malloc_mdl_grid(obj_grid_t *grid) {
  grid->num_tiles = grid->size_i.w * grid->size_i.h * grid->size_i.d;
  
  grid->pl_pnt = malloc(grid->num_tiles * sizeof(*grid->pl_pnt));
  grid->pl_last = calloc(grid->num_tiles * sizeof(*grid->pl_last), 1);
  grid->tile_num_faces = calloc(grid->num_tiles * sizeof(*grid->tile_num_faces), 1);
  
  memset(grid->pl_pnt, -1, grid->num_tiles * sizeof(*grid->pl_pnt));
  
  init_list(&grid->pl_data.nodes, sizeof(*grid->pl_data.nodes.data));
  grid->pl_data.length = 0;
}

void free_obj_grid(obj_grid_t *grid) {
  free(grid->pl_pnt);
  free(grid->pl_last);
  free(grid->tile_num_faces);
  free_list(&grid->pl_data);
}

void add_grid_lnk_ls_element(int x, int y, int z, int id, lnk_ls_t *list, grid_t *grid) {
  list_malloc_inc(&list->nodes);
  int tile_pnt = y * grid->size_i.d * grid->size_i.w + z * grid->size_i.w + x;
  
  if (grid->data_pnt[tile_pnt] >= 0) {
    list->nodes.data[grid->data_last[tile_pnt]].next = list->length;
  } else {
    grid->data_pnt[tile_pnt] = list->length; // point to linked list
  }
  grid->data_last[tile_pnt] = list->length;
  
  list->nodes.data[list->length].next = -1;
  list->nodes.data[list->length].data = id;
  list->length++;
}

void add_grid_d_lnk_ls_element(int x, int y, int z, int id, d_lnk_ls_t *list, grid_t *grid) {
  list_malloc_inc(&list->nodes);
  int tile_pnt = y * grid->size_i.d * grid->size_i.w + z * grid->size_i.w + x;

  if (grid->data_pnt[tile_pnt] >= 0) {
    list->nodes.data[grid->data_last[tile_pnt]].next = list->length;
    list->nodes.data[list->length].prev = grid->data_last[tile_pnt];
  } else {
    grid->data_pnt[tile_pnt] = list->length; // point to linked list
    list->nodes.data[list->length].prev = -1;
  }
  grid->data_last[tile_pnt] = list->length;
  
  list->nodes.data[list->length].next = -1;
  list->nodes.data[list->length].data = id;
  list->nodes.data[list->length].back_pnt = tile_pnt; // point to grid
  list->length++;
}

void add_mdl_grid_lnk_ls_element(int tile_pnt, int id, lnk_ls_t *list, obj_grid_t *grid) {
  list_malloc_inc(&list->nodes);
  
  if (grid->pl_pnt[tile_pnt] >= 0) {
    list->nodes.data[grid->pl_last[tile_pnt]].next = list->length;
  } else {
    grid->pl_pnt[tile_pnt] = list->length; // point to linked list
  }
  grid->pl_last[tile_pnt] = list->length;
  
  list->nodes.data[list->length].next = -1;
  list->nodes.data[list->length].data = id;
  list->length++;
}

void remove_grid_element(int x, int y, int z, int id, d_lnk_ls_t *list, grid_t *grid) {
  int tile_pnt = y * grid->size_i.d * grid->size_i.w + z * grid->size_i.w + x;
  int ls_pnt = grid->data_pnt[tile_pnt];
  
  while (ls_pnt >= 0) {
    if (list->nodes.data[ls_pnt].data == id) {
      // remove element
      remove_grid_lnk_list_element(ls_pnt, list, grid);
      return;
    }
    
    ls_pnt = list->nodes.data[ls_pnt].next;
  }
}

void replace_grid_element(int x, int y, int z, int id0, int id1, d_lnk_ls_t *list, grid_t *grid) {
  int tile_pnt = y * grid->size_i.d * grid->size_i.w + z * grid->size_i.w + x;
  int ls_pnt = grid->data_pnt[tile_pnt];
  
  while (ls_pnt >= 0) {
    if (list->nodes.data[ls_pnt].data == id0) {
      // replace element
      list->nodes.data[ls_pnt].data = id1;
      return;
    }
    
    ls_pnt = list->nodes.data[ls_pnt].next;
  }
}

// replaces the element with the last one in the list

void remove_grid_lnk_list_element(int ls_pnt, d_lnk_ls_t *list, grid_t *grid) {
  // connect the previous and next node
  
  if (list->nodes.data[ls_pnt].prev >= 0) {
    list->nodes.data[list->nodes.data[ls_pnt].prev].next = list->nodes.data[ls_pnt].next;
  } else {
    grid->data_pnt[list->nodes.data[ls_pnt].back_pnt] = list->nodes.data[ls_pnt].next;
  }
  
  if (list->nodes.data[ls_pnt].next >= 0) {
    list->nodes.data[list->nodes.data[ls_pnt].next].prev = list->nodes.data[ls_pnt].prev;
  } else {
    grid->data_last[list->nodes.data[ls_pnt].back_pnt] = list->nodes.data[ls_pnt].prev;
  }
  
  // move the last element to the current one
  
  list->nodes.data[ls_pnt].data = list->nodes.data[list->length - 1].data;
  list->nodes.data[ls_pnt].next = list->nodes.data[list->length - 1].next;
  list->nodes.data[ls_pnt].prev = list->nodes.data[list->length - 1].prev;
  list->nodes.data[ls_pnt].back_pnt = list->nodes.data[list->length - 1].back_pnt;
  
  // update the links for the last element
  
  if (list->nodes.data[ls_pnt].prev >= 0) {
    list->nodes.data[list->nodes.data[ls_pnt].prev].next = ls_pnt;
  } else {
    grid->data_pnt[list->nodes.data[ls_pnt].back_pnt] = ls_pnt;
  }
  
  if (list->nodes.data[ls_pnt].next >= 0) {
    list->nodes.data[list->nodes.data[ls_pnt].next].prev = ls_pnt;
  } else {
    grid->data_last[list->nodes.data[ls_pnt].back_pnt] = ls_pnt;
  }
  
  list->length--;
}