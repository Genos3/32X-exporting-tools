#include "common.h"

void t_make_face_grid(d_lnk_ls_t *list, grid_t *grid, object_t *object);
void check_face_edge(int face_id, poly_t *poly, d_lnk_ls_t *list, grid_t *grid, object_t *object);
void make_line_aabb(vec3_t *vt_0, vec3_t *vt_1, aabb_t *aabb);
void set_line_grid_aabb(aabb_t *line_aabb, aabb_t *aabb, grid_t *grid, object_t *object);
void check_edge_face_map(int map_pnt, int edge_id, int face_id, vec3_t *vt_0, vec3_t *vt_1, aabb_t *line_aabb, d_lnk_ls_t *list, grid_t *grid, object_t *object);
float check_point_line_distance(vec3_t *mid_vt, vec3_t *vt_0, vec3_t *vt_1);
void calculate_mid_txcoords(int edge_id, int face_id, vec3_t *mid_vt, vec3_t *vt0, vec3_t *vt1, object_t *object);
void add_extra_t_junction_faces(int face_id, object_t *object);
void add_extra_vertices_to_face(int face_id, object_t *object);
void subdiv_face_edge_lut(int face_id, object_t *object);

// static grid_t face_grid;
// static d_lnk_ls_t face_grid_list;
static vec3_t mid_vertices[8];
static vec2_tx_t mid_txcoords[8];
static u8 subdivide_face, subdivide_edge;
static int num_subdiv_edges;
static u8 subdiv_edges_bit;
#if T_JUNC_SUBDIV_TYPE == 2
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

void remove_obj_t_junctions(object_t *object) {
  // set_type_vt(model);
  #if T_JUNC_SUBDIV_TYPE == 2
    visited_faces = malloc(object->num_faces);
  #endif
  // make_vt_grid(&face_grid_list, &face_grid, object);
  t_make_face_grid(&face_grid_list, &face_grid, object);
  
  for (int i = 0; i < object->num_faces; i++) {
    if (object->faces.data[i].type & SPRITE) continue;
    
    subdivide_face = 1; // initializes the loop
    
    while (subdivide_face) {
      subdivide_face = 0;
      
      poly_t poly;
      set_poly_from_obj_face(i, &poly, object);
      
      check_face_edge(i, &poly, &face_grid_list, &face_grid, object);
      
      if (subdivide_face) {
        #if !T_JUNC_SUBDIV_TYPE
          subdiv_face_edge_lut(i, object); // FIX
        #elif T_JUNC_SUBDIV_TYPE == 1
          add_extra_vertices_to_face(i, object);
        #else
          add_extra_t_junction_faces(i, object);
        #endif
      }
    }
  }
  
  // t_set_face_list(&object);
  free_lnk_ls_grid(&face_grid_list, &face_grid);
  
  // free_lnk_ls_grid(&face_grid_list, &face_grid);
  // free(object->type_vt);
  #if T_JUNC_SUBDIV_TYPE == 2
    free(visited_faces);
  #endif
  
  // set_face_index(model);
  // set_tx_face_index(model);
}

// makes a grid that stores every face of the model on it, used to optimize the face search

void t_make_face_grid(d_lnk_ls_t *list, grid_t *grid, object_t *object) {
  poly_t poly;
  grid->size_i.w = ceilf(object->size.w / ini.face_merge_grid_tile_size);
  grid->size_i.h = ceilf(object->size.h / ini.face_merge_grid_tile_size);
  grid->size_i.d = ceilf(object->size.d / ini.face_merge_grid_tile_size);
  
  grid->size_i.w = max_c(grid->size_i.w, 1);
  grid->size_i.h = max_c(grid->size_i.h, 1);
  grid->size_i.d = max_c(grid->size_i.d, 1);
  
  malloc_d_lnk_ls_grid(list, grid);
  
  for (int i = 0; i < object->num_faces; i++) {
    if (object->faces.data[i].type & SPRITE) continue;
    poly.num_vertices = object->faces.data[i].num_vertices;
    
    if (ini.make_grid) {
      for (int j = 0; j < poly.num_vertices; j++) {
        poly.vertices[j].x = (object->faces.data[i].vertices[j].x - object->origin.x) / ini.face_merge_grid_tile_size;
        poly.vertices[j].y = (object->faces.data[i].vertices[j].y - object->origin.y) / ini.face_merge_grid_tile_size;
        poly.vertices[j].z = (object->faces.data[i].vertices[j].z - object->origin.z) / ini.face_merge_grid_tile_size;
      }
      
      aabb_t aabb;
      
      make_poly_aabb(&aabb, &poly);
      adjust_poly_tile_size(&aabb);
      set_map_list(i, &aabb, list, grid);
    } else {
      for (int j = 0; j < poly.num_vertices; j++) {
        poly.vertices[j].x = (int)((object->faces.data[i].vertices[j].x - object->origin.x) / ini.face_merge_grid_tile_size);
        poly.vertices[j].y = (int)((object->faces.data[i].vertices[j].y - object->origin.y) / ini.face_merge_grid_tile_size);
        poly.vertices[j].z = (int)((object->faces.data[i].vertices[j].z - object->origin.z) / ini.face_merge_grid_tile_size);
      }
      
      aabb_t aabb;
      
      make_poly_aabb(&aabb, &poly);
      set_map_list(i, &aabb, list, grid);
    }
  }
}

void check_face_edge(int face_id, poly_t *poly, d_lnk_ls_t *list, grid_t *grid, object_t *object) {
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
    set_line_grid_aabb(&line_aabb, &grid_aabb, grid, object);
    subdivide_edge = 0;
    
    // checks every grid tile the line aabb touches
    
    for (int j = grid_aabb.min.z; j <= grid_aabb.max.z; j++) {
      for (int k = grid_aabb.min.y; k <= grid_aabb.max.y; k++) {
        for (int l = grid_aabb.min.x; l <= grid_aabb.max.x; l++) {
          int map_pnt = k * grid->size_i.d * grid->size_i.w + j * grid->size_i.w + l;
          check_edge_face_map(map_pnt, i, face_id, &vt_0, &vt_1, &line_aabb, list, grid, object);
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

void set_line_grid_aabb(aabb_t *line_aabb, aabb_t *aabb, grid_t *grid, object_t *object) {
  aabb->min.x = (int)((line_aabb->min.x - object->origin.x - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  aabb->max.x = (int)((line_aabb->max.x - object->origin.x + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  aabb->min.y = (int)((line_aabb->min.y - object->origin.y - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  aabb->max.y = (int)((line_aabb->max.y - object->origin.y + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  aabb->min.z = (int)((line_aabb->min.z - object->origin.z - ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  aabb->max.z = (int)((line_aabb->max.z - object->origin.z + ini.merge_vt_dist_f) / ini.face_merge_grid_tile_size);
  
  aabb->min.x = clamp_i(aabb->min.x, 0, grid->size_i.w - 1);
  aabb->max.x = clamp_i(aabb->max.x, 0, grid->size_i.w - 1);
  aabb->min.y = clamp_i(aabb->min.y, 0, grid->size_i.h - 1);
  aabb->max.y = clamp_i(aabb->max.y, 0, grid->size_i.h - 1);
  aabb->min.z = clamp_i(aabb->min.z, 0, grid->size_i.d - 1);
  aabb->max.z = clamp_i(aabb->max.z, 0, grid->size_i.d - 1);
}

// iterate every face on the grid tile and for each edge and vertex compare them against the edges and vertices of the passed face 

void check_edge_face_map(int map_pnt, int edge_id, int face_id, vec3_t *vt0, vec3_t *vt1, aabb_t *line_aabb, d_lnk_ls_t *list, grid_t *grid, object_t *object) {
  int list_pnt = grid->data_pnt[map_pnt];
  
  while (list_pnt >= 0) {
    int map_face_id = list->nodes.data[list_pnt].data;
    list_pnt = list->nodes.data[list_pnt].next;
    
    if (map_face_id == face_id) continue;
    if (object->faces.data[map_face_id].type & SPRITE) continue;
    
    int map_face_num_vt = object->faces.data[map_face_id].num_vertices;
    
    vec3_t edge1_vt0 = object->faces.data[map_face_id].vertices[0];
    
    // for each edge
    for (int i = 0; i < map_face_num_vt; i++) {
      vec3_t edge1_vt1, edge0_dt_vt, edge1_dt_vt, cross_result;
      
      int n = i + 1;
      if (n == map_face_num_vt) n = 0;
      
      edge1_vt1 = object->faces.data[map_face_id].vertices[n];
      
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
      
      // vec3_t map_vt = object->vertices.data[map_vt_id];
      
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
          calculate_mid_txcoords(edge_id, face_id, &map_vt, vt0, vt1, object);
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

void calculate_mid_txcoords(int edge_id, int face_id, vec3_t *mid_vt, vec3_t *vt_0, vec3_t *vt_1, object_t *object) {
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
  if (vt1_id == object->faces.data[face_id].num_vertices) vt1_id = 0;
  
  vec2_tx_t vt_0_tx = object->faces.data[face_id].txcoords[edge_id];
  vec2_tx_t vt_1_tx = object->faces.data[face_id].txcoords[vt1_id];
  
  float du = vt_1_tx.u - vt_0_tx.u;
  float dv = vt_1_tx.v - vt_0_tx.v;
  
  float mid_vt_dist = d1_len / d0_len;
  
  mid_txcoords[num_subdiv_edges].u = vt_0_tx.u + du * mid_vt_dist;
  mid_txcoords[num_subdiv_edges].v = vt_0_tx.v + dv * mid_vt_dist;
}

/* void subdivide_face(int face_id, int poly_vt[], object_t *object) {
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

void add_extra_t_junction_faces(int face_id, object_t *object) {
  int num_vertices = object->faces.data[face_id].num_vertices;
  int num_subdiv_vertices = 0;
  
  // for each edge
  for (int i = 0; i < num_vertices; i++) {
    // if the edge contains a mid vertex
    if (subdiv_edges_bit & (1 << i)) {
      int n = i + 1;
      if (n == num_vertices) n = 0;
      
      poly_t poly;
      poly.vertices[0] = object->faces.data[face_id].vertices[i];
      poly.vertices[1] = mid_vertices[num_subdiv_vertices];
      poly.vertices[2] = object->faces.data[face_id].vertices[n];
      poly.txcoords[0] = object->faces.data[face_id].txcoords[i];
      poly.txcoords[1] = mid_txcoords[num_subdiv_vertices];
      poly.txcoords[2] = object->faces.data[face_id].txcoords[n];
      poly.num_vertices = 3;
      num_subdiv_vertices++;
      
      add_obj_face_from_poly(face_id, &poly, object);
    }
  }
}

// adds the vertices to the main face

void add_extra_vertices_to_face(int face_id, object_t *object) {
  poly_t poly;
  sub_poly_t sub_poly;
  sub_poly.num_vertices = 0;
  int num_subdiv_vertices = 0;
  
  // copy the vertices
  
  for (int i = 0; i < object->faces.data[face_id].num_vertices; i++) {
    sub_poly.vertices[sub_poly.num_vertices] = object->faces.data[face_id].vertices[i];
    sub_poly.txcoords[sub_poly.num_vertices] = object->faces.data[face_id].txcoords[i];
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
    set_obj_face_from_poly(face_id, &poly, object);
    
    poly.vertices[0] = sub_poly.vertices[0];
    poly.txcoords[0] = sub_poly.txcoords[0];
    
    for (int i = 0; i < sub_poly.num_vertices - 7; i++) {
      poly.vertices[i] = sub_poly.vertices[i];
      poly.txcoords[i] = sub_poly.txcoords[i];
    }
    
    poly.num_vertices = sub_poly.num_vertices - (7 - 1);
    add_obj_face_from_poly(face_id, &poly, object);
  } else {
    for (int i = 0; i < sub_poly.num_vertices; i++) {
      poly.vertices[i] = sub_poly.vertices[i];
      poly.txcoords[i] = sub_poly.txcoords[i];
    }
    
    poly.num_vertices = sub_poly.num_vertices;
    set_obj_face_from_poly(face_id, &poly, object);
  }
}

// subdivide the faces using a lut
// requires quads and triangles only

void subdiv_face_edge_lut(int face_id, object_t *object) {
  poly_t poly;
  vec3_t sub_poly_vt[8];
  vec2_tx_t sub_poly_tx[8];
  int sub_poly_num_vertices = object->faces.data[face_id].num_vertices;
  
  // copy the vertices
  
  for (int i = 0; i < sub_poly_num_vertices; i++) {
    sub_poly_vt[i] = object->faces.data[face_id].vertices[i];
    sub_poly_tx[i] = object->faces.data[face_id].txcoords[i];
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
        set_obj_face_from_poly(face_id, &poly, object);
      } else {
        add_obj_face_from_poly(face_id, &poly, object);
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
        set_obj_face_from_poly(face_id, &poly, object);
      } else {
        add_obj_face_from_poly(face_id, &poly, object);
      }
    }
  }
}