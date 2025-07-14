#include "common.h"

void grid_clip_face(int x, int y, int z, int face_id, int clip_flags, poly_t *poly, mdl_grid_t *grid, model_t *model);
void grid_clip_side_plane(poly_t *poly, float plane_pos, int side);
void line_clip_plane(poly_t *poly, int i, int num_vt, float dist_1, float dist_2, vec5_t *i_vt);
void grid_add_face(int face_id, poly_t *poly, model_t *model);

extern list_u8 grid_removed_faces;

void grid_face_clipping(int face_id, aabb_t *aabb, mdl_grid_t *grid, model_t *model) {
  poly_t poly;
  poly.num_vertices = model->face_num_vertices.data[face_id];
  
  for (int i = 0; i < poly.num_vertices; i++) {
    int pt_vt = model->faces.data[model->face_index.data[face_id] + i];
    poly.vertices[i].x = model->vertices.data[pt_vt].x;
    poly.vertices[i].y = model->vertices.data[pt_vt].y;
    poly.vertices[i].z = model->vertices.data[pt_vt].z;
  }
  
  if (!(model->face_types.data[face_id] & UNTEXTURED)) {
    for (int i = 0; i < poly.num_vertices; i++) {
      int pt_tx = model->tx_faces.data[model->tx_face_index.data[face_id] + i];
      poly.txcoords[i].u = model->txcoords.data[pt_tx].u;
      poly.txcoords[i].v = model->txcoords.data[pt_tx].v;
    }
    
    poly.has_texture = 1;
  }
  
  int clip_flags = 0;
  
  for (int i = aabb->min.z; i <= aabb->max.z; i++) {
    clip_flags &= ~(3 << 4); // remove bits from previous loop
    clip_flags |= (i > aabb->min.z) << 4; // near
    clip_flags |= (i < aabb->max.z) << 5; // far
    
    for (int j = aabb->min.y; j <= aabb->max.y; j++) {
      clip_flags &= ~(3 << 2);
      clip_flags |= (j > aabb->min.y) << 2; // top
      clip_flags |= (j < aabb->max.y) << 3; // bottom
      
      for (int k = aabb->min.x; k <= aabb->max.x; k++) {        
        clip_flags &= ~3;
        clip_flags |= (k > aabb->min.x); // left
        clip_flags |= (k < aabb->max.x) << 1; // right
        
        grid_clip_face(k, j, i, face_id, clip_flags, &poly, grid, model);
      }
    }
  }
  
  // remove face
  
  grid_removed_faces.data[face_id] = 1;
}

void grid_clip_face(int x, int y, int z, int face_id, int clip_flags, poly_t *poly, mdl_grid_t *grid, model_t *model) {
  poly_t poly_oc;
  
  for (int i = 0; i < poly->num_vertices; i++) {
    poly_oc.vertices[i] = poly->vertices[i];
  }
  
  if (poly->has_texture) {
    for (int i = 0; i < poly->num_vertices; i++) {
      poly_oc.txcoords[i] = poly->txcoords[i];
    }
  }
  
  poly_oc.num_vertices = poly->num_vertices;
  poly_oc.has_texture = poly->has_texture;
  
  if (clip_flags & 1) { // left
    float plane_pos = model->origin.x + x * grid->tile_size_i;
    grid_clip_side_plane(&poly_oc, plane_pos, 0);
  }
  if (clip_flags & 2) { // right
    float plane_pos = model->origin.x + x * grid->tile_size_i + grid->tile_size_i;
    grid_clip_side_plane(&poly_oc, plane_pos, 1);
  }
  if (clip_flags & 4) { // top
    float plane_pos = model->origin.y + y * grid->tile_size_i;
    grid_clip_side_plane(&poly_oc, plane_pos, 2);
  }
  if (clip_flags & 8) { // bottom
    float plane_pos = model->origin.y + y * grid->tile_size_i + grid->tile_size_i;
    grid_clip_side_plane(&poly_oc, plane_pos, 3);
  }
  if (clip_flags & 16) { // near
    float plane_pos = model->origin.z + z * grid->tile_size_i;
    grid_clip_side_plane(&poly_oc, plane_pos, 4);
  }
  if (clip_flags & 32) { // far
    float plane_pos = model->origin.z + z * grid->tile_size_i + grid->tile_size_i;
    grid_clip_side_plane(&poly_oc, plane_pos, 5);
  }
  
  grid_add_face(face_id, &poly_oc, model);
}

void grid_clip_side_plane(poly_t *poly, float plane_pos, int side) {
  poly_t poly_cl;
  int num_vt = poly->num_vertices - 1;
  float dist_1 = 0; // distance to border
  
  if (!side) { // left
    dist_1 = poly->vertices[num_vt].x - plane_pos;
  } else
  if (side == 1) { // right
    dist_1 = plane_pos - poly->vertices[num_vt].x;
  } else
  if (side == 2) { // top
    dist_1 = poly->vertices[num_vt].y - plane_pos;
  } else
  if (side == 3) { // bottom
    dist_1 = plane_pos - poly->vertices[num_vt].y;
  } else
  if (side == 4) { // near
    dist_1 = poly->vertices[num_vt].z - plane_pos;
  } else
  if (side == 5) { // far
    dist_1 = plane_pos - poly->vertices[num_vt].z;
  }
  
  int next_vt = 0;
  
  for (int i = 0; i < poly->num_vertices; i++) {
    float dist_2 = 0; // distance to border
    
    if (!side) { // left
      dist_2 = poly->vertices[i].x - plane_pos;
    } else
    if (side == 1) { // right
      dist_2 = plane_pos - poly->vertices[i].x;
    } else
    if (side == 2) { // top
      dist_2 = poly->vertices[i].y - plane_pos;
    } else
    if (side == 3) { // bottom
      dist_2 = plane_pos - poly->vertices[i].y;
    } else
    if (side == 4) { // near
      dist_2 = poly->vertices[i].z - plane_pos;
    } else
    if (side == 5) { // far
      dist_2 = plane_pos - poly->vertices[i].z;
    }
    
    vec5_t i_vt;
    if (dist_2 >= 0) { // inside
      if (dist_1 < 0) { // outside
        line_clip_plane(poly, i, num_vt, dist_1, dist_2, &i_vt);
        
        poly_cl.vertices[next_vt].x = i_vt.x;
        poly_cl.vertices[next_vt].y = i_vt.y;
        poly_cl.vertices[next_vt].z = i_vt.z;
        
        if (poly->has_texture) {
          poly_cl.txcoords[next_vt].u = i_vt.u;
          poly_cl.txcoords[next_vt].v = i_vt.v;
        }
        
        next_vt++;
      }
      
      poly_cl.vertices[next_vt].x = poly->vertices[i].x;
      poly_cl.vertices[next_vt].y = poly->vertices[i].y;
      poly_cl.vertices[next_vt].z = poly->vertices[i].z;
      
      if (poly->has_texture) {
        poly_cl.txcoords[next_vt].u = poly->txcoords[i].u;
        poly_cl.txcoords[next_vt].v = poly->txcoords[i].v;
      }
      
      next_vt++;
    } else
    if (dist_1 >= 0) { // inside
      line_clip_plane(poly, i, num_vt, dist_1, dist_2, &i_vt);
      
      poly_cl.vertices[next_vt].x = i_vt.x;
      poly_cl.vertices[next_vt].y = i_vt.y;
      poly_cl.vertices[next_vt].z = i_vt.z;
      
      if (poly->has_texture) {
        poly_cl.txcoords[next_vt].u = i_vt.u;
        poly_cl.txcoords[next_vt].v = i_vt.v;
      }
      
      next_vt++;
    }
    
    num_vt = i;
    dist_1 = dist_2;
  }
  
  poly->num_vertices = next_vt;
  //poly = poly_cl;
  
  for (int i = 0; i < poly->num_vertices; i++) {
    poly->vertices[i].x = poly_cl.vertices[i].x;
    poly->vertices[i].y = poly_cl.vertices[i].y;
    poly->vertices[i].z = poly_cl.vertices[i].z;
    
    if (poly->has_texture) {
      poly->txcoords[i].u = poly_cl.txcoords[i].u;
      poly->txcoords[i].v = poly_cl.txcoords[i].v;
    }
  }
}

void line_clip_plane(poly_t *poly, int i, int num_vt, float dist_1, float dist_2, vec5_t *i_vt) {
  float x0 = poly->vertices[num_vt].x;
  float y0 = poly->vertices[num_vt].y;
  float z0 = poly->vertices[num_vt].z;
  float x1 = poly->vertices[i].x;
  float y1 = poly->vertices[i].y;
  float z1 = poly->vertices[i].z;
  float length = dist_1 - dist_2;
  float s;
  
  if (length != 0) {
    s = dist_1 / length;
  } else {
    s = 1;
  }
  
  i_vt->x = x0 + s * (x1 - x0);
  i_vt->y = y0 + s * (y1 - y0);
  i_vt->z = z0 + s * (z1 - z0);
  
  if (poly->has_texture) {
    float u0 = poly->txcoords[num_vt].u;
    float v0 = poly->txcoords[num_vt].v;
    float u1 = poly->txcoords[i].u;
    float v1 = poly->txcoords[i].v;
    
    i_vt->u = u0 + s * (u1 - u0);
    i_vt->v = v0 + s * (v1 - v0);
  }
}

void grid_add_face(int face_id, poly_t *poly, model_t *model) {
  list_push_int(&model->face_index, model->faces_size);
  list_push_int(&model->tx_face_index, model->tx_faces_size);
  // list_malloc_inc(&model->face_index);
  // list_malloc_inc(&model->tx_face_index);
  
  for (int i = 0; i < poly->num_vertices; i++) {
    list_push_pnt(&model->vertices, &poly->vertices[i]);
    list_push_pnt(&model->txcoords, &poly->txcoords[i]);
    
    list_push_int(&model->faces, model->num_vertices);
    list_push_int(&model->tx_faces, model->num_txcoords);
    
    model->num_vertices++;
    model->num_txcoords++;
  }
  
  model->faces_size += poly->num_vertices;
  model->tx_faces_size += poly->num_vertices;
  
  list_push_int(&model->face_num_vertices, poly->num_vertices);
  
  list_push_pnt(&model->normals, &model->normals.data[face_id]);
  list_push_int(&model->face_materials, model->face_materials.data[face_id]);
  list_push_int(&model->face_types, model->face_types.data[face_id]);
  if (model->num_sprites) {
    list_push_int(&model->sprite_face_index, model->sprite_face_index.data[face_id]);
  }
  
  list_push_int(&grid_removed_faces, 0);
  
  model->num_faces++;
  model->num_tx_faces++;
}