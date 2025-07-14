#include "common.h"

void t_make_face_grid(d_lnk_ls_t *list, grid_t *grid, pl_list_t *pl_list, model_t *model);
void check_face_edge(int face_id, poly_t *poly, d_lnk_ls_t *list, grid_t *grid, pl_list_t *pl_list, model_t *model);
void make_line_aabb(vec3_t *vt_0, vec3_t *vt_1, aabb_t *aabb);
void set_line_grid_aabb(aabb_t *line_aabb, aabb_t *aabb, grid_t *grid, model_t *model);
void check_edge_face_map(int map_pnt, int edge_id, int face_id, vec3_t *vt_0, vec3_t *vt_1, aabb_t *line_aabb, d_lnk_ls_t *list, grid_t *grid, pl_list_t *pl_list, model_t *model);
float check_point_line_distance(vec3_t *mid_vt, vec3_t *vt_0, vec3_t *vt_1);
void calculate_mid_txcoords(int edge_id, int face_id, vec3_t *mid_vt, vec3_t *vt0, vec3_t *vt1, pl_list_t *pl_list);
void add_extra_t_junction_faces(int face_id, pl_list_t *pl_list, model_t *model);
void add_extra_vertices_to_face(int face_id, pl_list_t *pl_list, model_t *model);
void subdiv_face_edge_lut(int face_id, pl_list_t *pl_list, model_t *model);

// static grid_t face_grid;
// static d_lnk_ls_t face_grid_list;
static vec3_t mid_vertices[8];
static vec2_tx_t mid_txcoords[8];
static u8 subdivide_face, subdivide_edge;
static int num_subdiv_edges;
static u8 subdiv_edges_bit;
#if SUBDIV_TYPE == 2
  static u8 *visited_faces;
#endif
static grid_t face_grid;
static d_lnk_ls_t face_grid_list;

typedef struct {
  vec3_t vertices[16];
  vec2_tx_t txcoords[16];
  int num_vertices;
} sub_poly_t;

// if a t-junction is found along a face edge it subdivides the face recursively

void remove_mdl_t_junctions(pl_list_t *pl_list, model_t *model) {
  // pl_list_t pl_list;
  // set_type_vt(model);
  #if SUBDIV_TYPE == 2
    visited_faces = malloc(model->num_faces);
  #endif
  // init_pl_list(&pl_list);
  // set_pl_list(&pl_list, model);
  // make_vt_grid(&face_grid_list, &face_grid, model);
  t_make_face_grid(&face_grid_list, &face_grid, pl_list, model);
  
  for (int i = 0; i < pl_list->num_faces; i++) {
    if (model->face_types.data[i] & SPRITE) continue;
    
    subdivide_face = 1; // initializes the loop
    
    while (subdivide_face) {
      subdivide_face = 0;
      
      poly_t poly;
      pl_list_set_poly(i, &poly, pl_list);
      
      check_face_edge(i, &poly, &face_grid_list, &face_grid, pl_list, model);
      
      if (subdivide_face) {
        #if !SUBDIV_TYPE
          subdiv_face_edge_lut(i, pl_list, model); // FIX
        #elif SUBDIV_TYPE == 1
          add_extra_vertices_to_face(i, pl_list, model);
        #else
          add_extra_t_junction_faces(i, pl_list, model);
        #endif
      }
    }
  }
  
  // t_set_face_list(&pl_list, model);
  free_lnk_ls_grid(&face_grid_list, &face_grid);
  
  // free_pl_list(&pl_list);
  // free_lnk_ls_grid(&face_grid_list, &face_grid);
  // free(model->type_vt);
  #if SUBDIV_TYPE == 2
    free(visited_faces);
  #endif
  
  // set_face_index(model);
  // set_tx_face_index(model);
}

// makes a grid that stores every face of the model on it, used to optimize the face search

void t_make_face_grid(d_lnk_ls_t *list, grid_t *grid, pl_list_t *pl_list, model_t *model) {
  poly_t poly;
  grid->size_i.w = ceilf(model->size.w / ini.face_merge_grid_tile_size);
  grid->size_i.h = ceilf(model->size.h / ini.face_merge_grid_tile_size);
  grid->size_i.d = ceilf(model->size.d / ini.face_merge_grid_tile_size);
  
  grid->size_i.w = max_c(grid->size_i.w, 1);
  grid->size_i.h = max_c(grid->size_i.h, 1);
  grid->size_i.d = max_c(grid->size_i.d, 1);
  
  malloc_d_lnk_ls_grid(list, grid);
  
  for (int i = 0; i < pl_list->num_faces; i++) {
    if (model->face_types.data[i] & SPRITE) continue;
    poly.num_vertices = pl_list->face_num_vertices.data[i];
    
    if (ini.make_grid) {
      for (int j = 0; j < poly.num_vertices; j++) {
        poly.vertices[j].x = (pl_list->vertices.data[i * 8 + j].x - model->origin.x) / ini.face_merge_grid_tile_size;
        poly.vertices[j].y = (pl_list->vertices.data[i * 8 + j].y - model->origin.y) / ini.face_merge_grid_tile_size;
        poly.vertices[j].z = (pl_list->vertices.data[i * 8 + j].z - model->origin.z) / ini.face_merge_grid_tile_size;
      }
      
      aabb_t aabb;
      
      make_poly_aabb(&aabb, &poly);
      adjust_poly_tile_size(&aabb);
      set_map_list(i, &aabb, list, grid);
    } else {
      for (int j = 0; j < poly.num_vertices; j++) {
        poly.vertices[j].x = (int)((pl_list->vertices.data[i * 8 + j].x - model->origin.x) / ini.face_merge_grid_tile_size);
        poly.vertices[j].y = (int)((pl_list->vertices.data[i * 8 + j].y - model->origin.y) / ini.face_merge_grid_tile_size);
        poly.vertices[j].z = (int)((pl_list->vertices.data[i * 8 + j].z - model->origin.z) / ini.face_merge_grid_tile_size);
      }
      
      aabb_t aabb;
      
      make_poly_aabb(&aabb, &poly);
      set_map_list(i, &aabb, list, grid);
    }
  }
}

void check_face_edge(int face_id, poly_t *poly, d_lnk_ls_t *list, grid_t *grid, pl_list_t *pl_list, model_t *model) {
  subdiv_edges_bit = 0;
  num_subdiv_edges = 0;
  
  vec3_t vt_0 = poly->vertices[0];
  
  // for each edge
  for (int i = 0; i < poly->num_vertices; i++) {
    aabb_t line_aabb, grid_aabb;
    
    int n = i + 1;
    if (n == poly->num_vertices) n = 0;
    
    vec3_t vt_1 = poly->vertices[n];
    make_line_aabb(&vt_0, &vt_1, &line_aabb);
    set_line_grid_aabb(&line_aabb, &grid_aabb, grid, model);
    subdivide_edge = 0;
    
    // checks every grid tile the line aabb touches
    
    for (int j = grid_aabb.min.z; j <= grid_aabb.max.z; j++) {
      for (int k = grid_aabb.min.y; k <= grid_aabb.max.y; k++) {
        for (int l = grid_aabb.min.x; l <= grid_aabb.max.x; l++) {
          int map_pnt = k * grid->size_i.d * grid->size_i.w + j * grid->size_i.w + l;
          check_edge_face_map(map_pnt, i, face_id, &vt_0, &vt_1, &line_aabb, list, grid, pl_list, model);
          if (subdivide_edge) goto exit_loop;
        }
      }
    }
    exit_loop:;
    
    vt_0 = vt_1;
  }
}

void make_line_aabb(vec3_t *vt_0, vec3_t *vt_1, aabb_t *aabb) {
  aabb->min.x = min_c(vt_0->x, vt_1->x);
  aabb->min.y = min_c(vt_0->y, vt_1->y);
  aabb->min.z = min_c(vt_0->z, vt_1->z);
  aabb->max.x = max_c(vt_0->x, vt_1->x);
  aabb->max.y = max_c(vt_0->y, vt_1->y);
  aabb->max.z = max_c(vt_0->z, vt_1->z);
}

// normalize the line aabb to the grid size

void set_line_grid_aabb(aabb_t *line_aabb, aabb_t *aabb, grid_t *grid, model_t *model) {
  aabb->min.x = (int)((line_aabb->min.x - model->origin.x - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  aabb->max.x = (int)((line_aabb->max.x - model->origin.x + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  aabb->min.y = (int)((line_aabb->min.y - model->origin.y - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  aabb->max.y = (int)((line_aabb->max.y - model->origin.y + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  aabb->min.z = (int)((line_aabb->min.z - model->origin.z - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  aabb->max.z = (int)((line_aabb->max.z - model->origin.z + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  
  aabb->min.x = clamp_i(aabb->min.x, 0, grid->size_i.w - 1);
  aabb->max.x = clamp_i(aabb->max.x, 0, grid->size_i.w - 1);
  aabb->min.y = clamp_i(aabb->min.y, 0, grid->size_i.h - 1);
  aabb->max.y = clamp_i(aabb->max.y, 0, grid->size_i.h - 1);
  aabb->min.z = clamp_i(aabb->min.z, 0, grid->size_i.d - 1);
  aabb->max.z = clamp_i(aabb->max.z, 0, grid->size_i.d - 1);
}

// iterate every face on the grid tile and for each edge and vertex compare them against the edges and vertices of the passed face 

void check_edge_face_map(int map_pnt, int edge_id, int face_id, vec3_t *vt0, vec3_t *vt1, aabb_t *line_aabb, d_lnk_ls_t *list, grid_t *grid, pl_list_t *pl_list, model_t *model) {
  int list_pnt = grid->data_pnt[map_pnt];
  
  while (list_pnt >= 0) {
    int map_face_id = list->nodes.data[list_pnt].data;
    list_pnt = list->nodes.data[list_pnt].next;
    
    if (map_face_id == face_id) continue;
    if (model->face_types.data[map_face_id] & SPRITE) continue;
    
    int map_face_num_vt = pl_list->face_num_vertices.data[map_face_id];
    
    vec3_t edge1_vt0 = pl_list->vertices.data[map_face_id * 8];
    
    // for each edge
    for (int i = 0; i < map_face_num_vt; i++) {
      vec3_t edge1_vt1, edge0_dt_vt, edge1_dt_vt, cross_result;
      
      int n = i + 1;
      if (n == map_face_num_vt) n = 0;
      
      edge1_vt1 = pl_list->vertices.data[map_face_id * 8 + n];
      
      edge1_dt_vt.x = edge1_vt1.x - edge1_vt0.x;
      edge1_dt_vt.y = edge1_vt1.y - edge1_vt0.y;
      edge1_dt_vt.z = edge1_vt1.z - edge1_vt0.z;
      
      edge0_dt_vt.x = vt1->x - vt0->x;
      edge0_dt_vt.y = vt1->y - vt0->y;
      edge0_dt_vt.z = vt1->z - vt0->z;
      
      cross(&edge0_dt_vt, &edge1_dt_vt, &cross_result);
      
      // if the two lines aren't collinear continue
      
      if (calc_length(&cross_result) > 0.01) {
        edge1_vt0 = edge1_vt1;
        continue;
      }
      
      // vec3_t map_vt = model->vertices.data[map_vt_id];
      
      vec3_t map_vt = edge1_vt0;
      
      // run for each vertex
      for (int j = 0; j < 1; j++) {
        // if the vertex is not within the line aabb continue
        
        if (map_vt.x < line_aabb->min.x || map_vt.x > line_aabb->max.x ||
            map_vt.y < line_aabb->min.y || map_vt.y > line_aabb->max.y ||
            map_vt.z < line_aabb->min.z || map_vt.z > line_aabb->max.z) continue;
        
        // if the vertex coincides with the line vertices continue
        
        if ((map_vt.x == vt0->x && map_vt.y == vt0->y && map_vt.z == vt0->z) ||
            (map_vt.x == vt1->x && map_vt.y == vt1->y && map_vt.z == vt1->z)) continue;
        
        // check the distance between the vertex and the line
        
        if (check_point_line_distance(&map_vt, vt0, vt1) <= ini.merge_vt_dist_f) {
          subdiv_edges_bit |= 1 << edge_id;
          mid_vertices[num_subdiv_edges] = map_vt;
          calculate_mid_txcoords(edge_id, face_id, &map_vt, vt0, vt1, pl_list);
          subdivide_face = 1;
          subdivide_edge = 1;
          num_subdiv_edges++;
          return;
        }
        
        map_vt = edge1_vt1;
      }
      
      edge1_vt0 = edge1_vt1;
    }
  }
}

// obtain the distance of a point to a line

float check_point_line_distance(vec3_t *mid_vt, vec3_t *vt_0, vec3_t *vt_1) {
  vec3_t d0, d1;
  d0.x = vt_1->x - vt_0->x;
  d0.y = vt_1->y - vt_0->y;
  d0.z = vt_1->z - vt_0->z;
  d1.x = mid_vt->x - vt_0->x;
  d1.y = mid_vt->y - vt_0->y;
  d1.z = mid_vt->z - vt_0->z;
  
  vec3_t c;
  cross(&d0, &d1, &c);
  
  float d_len = calc_length(&d0);
  float c_len = calc_length(&c);
  
  if (!d_len) return 0;
  
  return c_len / d_len;
}

void calculate_mid_txcoords(int edge_id, int face_id, vec3_t *mid_vt, vec3_t *vt_0, vec3_t *vt_1, pl_list_t *pl_list) {
  vec3_t d0, d1;
  d0.x = vt_1->x - vt_0->x;
  d0.y = vt_1->y - vt_0->y;
  d0.z = vt_1->z - vt_0->z;
  d1.x = mid_vt->x - vt_0->x;
  d1.y = mid_vt->y - vt_0->y;
  d1.z = mid_vt->z - vt_0->z;
  
  float d0_len = calc_length(&d0);
  float d1_len = calc_length(&d1);
  
  int vt1_id = edge_id + 1;
  if (vt1_id == pl_list->face_num_vertices.data[face_id]) vt1_id = 0;
  
  vec2_tx_t vt_0_tx = pl_list->txcoords.data[face_id * 8 + edge_id];
  vec2_tx_t vt_1_tx = pl_list->txcoords.data[face_id * 8 + vt1_id];
  
  float du = vt_1_tx.u - vt_0_tx.u;
  float dv = vt_1_tx.v - vt_0_tx.v;
  
  float mid_vt_dist = d1_len / d0_len;
  
  mid_txcoords[num_subdiv_edges].u = vt_0_tx.u + du * mid_vt_dist;
  mid_txcoords[num_subdiv_edges].v = vt_0_tx.v + dv * mid_vt_dist;
}

/* void subdivide_face(int face_id, int poly_vt[], model_t *model) {
  int sub_poly_vt[8];
  int subdiv_edge;
  int sub_poly_num_vt = 0;
  
  for (int i = 0; i < poly->num_vertices; i++) {
    if (subdiv_edges_bit & (1 << i)) {
      sub_poly_vt[sub_poly_num_vt] = subdiv_vt[subdiv_edge];
      subdiv_edge++;
      sub_poly_num_vt++;
      continue;
    } else {
      sub_poly_vt[sub_poly_num_vt] = poly_vt[i];
      sub_poly_num_vt++;
    }
  }
} */

// creates a thin triangle using the t-junction vertex
// requires a visited_faces array

void add_extra_t_junction_faces(int face_id, pl_list_t *pl_list, model_t *model) {
  int num_vertices = pl_list->face_num_vertices.data[face_id];
  int num_subdiv_vertices = 0;
  
  // for each edge
  for (int i = 0; i < num_vertices; i++) {
    // if the edge contains a mid vertex
    if (subdiv_edges_bit & (1 << i)) {
      int n = i + 1;
      if (n == num_vertices) n = 0;
      
      poly_t poly;
      poly.vertices[0] = pl_list->vertices.data[face_id * 8 + i];
      poly.vertices[1] = mid_vertices[num_subdiv_vertices];
      poly.vertices[2] = pl_list->vertices.data[face_id * 8 + n];
      poly.txcoords[0] = pl_list->txcoords.data[face_id * 8 + i];
      poly.txcoords[1] = mid_txcoords[num_subdiv_vertices];
      poly.txcoords[2] = pl_list->txcoords.data[face_id * 8 + n];
      poly.num_vertices = 3;
      num_subdiv_vertices++;
      
      pl_list_add_face(face_id, 1, &poly, pl_list, model);
    }
  }
}

// adds the vertices to the main face

void add_extra_vertices_to_face(int face_id, pl_list_t *pl_list, model_t *model) {
  poly_t poly;
  sub_poly_t sub_poly;
  sub_poly.num_vertices = 0;
  int num_subdiv_vertices = 0;
  
  // copy the vertices
  
  for (int i = 0; i < pl_list->face_num_vertices.data[face_id]; i++) {
    sub_poly.vertices[sub_poly.num_vertices] = pl_list->vertices.data[face_id * 8 + i];
    sub_poly.txcoords[sub_poly.num_vertices] = pl_list->txcoords.data[face_id * 8 + i];
    sub_poly.num_vertices++;
    
    if (subdiv_edges_bit & (1 << i)) {
      sub_poly.vertices[sub_poly.num_vertices] = mid_vertices[num_subdiv_vertices];
      sub_poly.txcoords[sub_poly.num_vertices] = mid_txcoords[num_subdiv_vertices];
      sub_poly.num_vertices++;
      num_subdiv_vertices++;
    }
  }
  
  // if the resulting face has more vertices than what is allowed subdivide it
  
  if (sub_poly.num_vertices > 8) {
    for (int i = 0; i < 8; i++) {
      poly.vertices[i] = sub_poly.vertices[i];
      poly.txcoords[i] = sub_poly.txcoords[i];
    }
    
    poly.num_vertices = 8;
    pl_list_add_face(face_id, 0, &poly, pl_list, model);
    
    poly.vertices[0] = sub_poly.vertices[0];
    poly.txcoords[0] = sub_poly.txcoords[0];
    
    for (int i = 0; i < sub_poly.num_vertices - 7; i++) {
      poly.vertices[i] = sub_poly.vertices[i];
      poly.txcoords[i] = sub_poly.txcoords[i];
    }
    
    poly.num_vertices = sub_poly.num_vertices - (7 - 1);
    pl_list_add_face(face_id, 1, &poly, pl_list, model);
  } else {
    for (int i = 0; i < sub_poly.num_vertices; i++) {
      poly.vertices[i] = sub_poly.vertices[i];
      poly.txcoords[i] = sub_poly.txcoords[i];
    }
    
    poly.num_vertices = sub_poly.num_vertices;
    pl_list_add_face(face_id, 0, &poly, pl_list, model);
  }
}

// subdivide the faces using a lut
// requires quads and triangles only

void subdiv_face_edge_lut(int face_id, pl_list_t *pl_list, model_t *model) {
  poly_t poly;
  vec3_t sub_poly_vt[8];
  vec2_tx_t sub_poly_tx[8];
  int sub_poly_num_vertices = pl_list->face_num_vertices.data[face_id];
  
  // copy the vertices
  
  for (int i = 0; i < sub_poly_num_vertices; i++) {
    sub_poly_vt[i] = pl_list->vertices.data[face_id * 8 + i];
    sub_poly_tx[i] = pl_list->txcoords.data[face_id * 8 + i];
  }
  
  // add the mid vertex to the polygon vertex list
  
  for (int i = 0; i < num_subdiv_edges; i++) {
    sub_poly_vt[sub_poly_num_vertices + i] = mid_vertices[i];
    sub_poly_tx[sub_poly_num_vertices + i] = mid_txcoords[i];
  }
  
  if (sub_poly_num_vertices == 3) { // triangle
    for (int i = 0; i < tri_sub_num_pl[subdiv_edges_bit]; i++) {
      u8 group_pnt = tri_sub_group_idx[subdiv_edges_bit] + i;
      poly.num_vertices = tri_sub_num_vt[group_pnt];
      
      for (int j = 0; j < poly.num_vertices; j++) {
        u8 sub_pl_vt_pnt = tri_sub_faces_lut[tri_sub_face_index[group_pnt] + j];
        poly.vertices[j] = sub_poly_vt[sub_pl_vt_pnt];
        poly.txcoords[j] = sub_poly_tx[sub_pl_vt_pnt];
      }
      
      if (!i) {
        pl_list_add_face(face_id, 0, &poly, pl_list, model);
      } else {
        pl_list_add_face(face_id, 1, &poly, pl_list, model);
      }
    }
  } else { // quad
    for (int i = 0; i < quad_sub_num_pl[subdiv_edges_bit]; i++) {
      u8 group_pnt = quad_sub_group_idx[subdiv_edges_bit] + i;
      poly.num_vertices = quad_sub_num_vt[group_pnt];
      
      for (int j = 0; j < poly.num_vertices; j++) {
        u8 sub_pl_vt_pnt = quad_sub_faces_lut[quad_sub_face_index[group_pnt] + j];
        poly.vertices[j] = sub_poly_vt[sub_pl_vt_pnt];
        poly.txcoords[j] = sub_poly_tx[sub_pl_vt_pnt];
      }
      
      if (!i) {
        pl_list_add_face(face_id, 0, &poly, pl_list, model);
      } else {
        pl_list_add_face(face_id, 1, &poly, pl_list, model);
      }
    }
  }
}