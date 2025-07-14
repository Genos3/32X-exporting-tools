#include "common.h"

int check_backface_culling(vec3_t pos, int face_id, model_t *model) {
  vec3_t view_dir;
  int pt_vt = model->faces[model->face_index[face_id]];
  
  view_dir.x = pos.x + model->vertices[pt_vt].x - cam.pos.x;
  view_dir.y = pos.y + model->vertices[pt_vt].y - cam.pos.y;
  view_dir.z = pos.z + model->vertices[pt_vt].z - cam.pos.z;
  
  int r_poly_normal_z = dp3(view_dir.x, model->normals[face_id].x, view_dir.y, model->normals[face_id].y, view_dir.z, model->normals[face_id].z);
  
  if (r_poly_normal_z >= 0) return 0;
  return 1;
}

int check_2d_backface_culling(g_poly_t *poly) {
  vec5_t v0 = poly->vertices[0];
  vec5_t v1 = poly->vertices[1];
  vec5_t v2 = poly->vertices[2];
  
  fixed area = fp_mul(v1.x - v0.x, v2.y - v0.y) - fp_mul(v1.y - v0.y, v2.x - v0.x);
  
  if (area <= 0) return 0;
  return 1;
}

fixed calc_view_normal_z(vec3_t pos, int face_id, model_t *model) {
  vec3_t view_dir;
  int pt_vt = model->faces[model->face_index[face_id]];
  
  view_dir.x = pos.x + model->vertices[pt_vt].x - cam.pos.x;
  view_dir.y = pos.y + model->vertices[pt_vt].y - cam.pos.y;
  view_dir.z = pos.z + model->vertices[pt_vt].z - cam.pos.z;
  
  return dp3(view_dir.x, model->normals[face_id].x, view_dir.y, model->normals[face_id].y, view_dir.z, model->normals[face_id].z);
}

int check_frustum_clipping(g_poly_t *poly) {
  poly->frustum_clip_sides = 0;
  int outside_frustum = 0xff;
  
  for (int i = 0; i < poly->num_vertices; i++) {
    fixed w = fp_mul(poly->vertices[i].z, vp.screen_side_x_dt);
    fixed h = fp_mul(poly->vertices[i].z, vp.screen_side_y_dt);
    int plane_sides = 0;
    
    plane_sides |= (poly->vertices[i].z < Z_NEAR);
    plane_sides |= (poly->vertices[i].z > Z_FAR) << 1;
    plane_sides |= (poly->vertices[i].x < -w) << 2;
    plane_sides |= (poly->vertices[i].x > w) << 3;
    plane_sides |= (poly->vertices[i].y < -h) << 4;
    plane_sides |= (poly->vertices[i].y > h) << 5;
    
    outside_frustum &= plane_sides;
    poly->frustum_clip_sides |= plane_sides;
  }
  
  return !outside_frustum; //totally outside
}

int test_tx_dist(g_poly_t *poly) { //determine fw tx mapping
  fixed x0 = poly->vertices[2].x - poly->vertices[1].x;
  fixed y0 = poly->vertices[2].y - poly->vertices[1].y;
  fixed u0 = poly->vertices[2].u - poly->vertices[1].u;
  fixed v0 = poly->vertices[2].v - poly->vertices[1].v;
  fixed x1 = poly->vertices[0].x - poly->vertices[1].x;
  fixed y1 = poly->vertices[0].y - poly->vertices[1].y;
  fixed u1 = poly->vertices[0].u - poly->vertices[1].u;
  fixed v1 = poly->vertices[0].v - poly->vertices[1].v;
  
  fixed dxy = fp_mul(x0, y1) - fp_mul(x1, y0);
  // fixed a = rc_fp_div_luts(fp_mul(u0, v1) - fp_mul(u1, v0), dxy);
  fixed a = div_luts_s(dxy >> PROJ_SHIFT) >> PROJ_SHIFT;
  a = fp_mul(fp_mul(u0, v1) - fp_mul(u1, v0), a);
  
  if (dxy && abs_c(a) >= 1 << FP) {
    return 0;
  } else {
    return 1;
  }
}

int test_poly_dist(g_poly_t *poly) {
  for (int i = 0; i < poly->num_vertices; i++) {
    if (poly->vertices[i].z < TX_PERSP_DIST) return 1; // fix(4)
  }
  
  return 0;
}

int test_poly_ratio(g_poly_t *poly) {
  fixed max_z = poly->vertices[0].z;
  fixed min_z = poly->vertices[0].z;
  
  for (int i = 1; i < poly->num_vertices; i++) {
    max_z = max_c(max_z, poly->vertices[i].z);
    min_z = min_c(min_z, poly->vertices[i].z);
  }
  
  // if (max_z > min_z * 2 && max_z < TX_PERSP_DIST && max_z - min_z > fix(0.6)) return 1;
  if (max_z > fp_mul(min_z, fix(1.5)) && max_z < TX_PERSP_DIST && max_z - min_z > fix(0.6)) return 1;
  
  return 0;
}

// frustum culling
// return: 0 = outside, 1 = partially inside, 2 = totally inside
// type: 1 = skip the near and far planes

RAM_CODE int check_frustum_culling(vec3_t pos, fixed radius, u8 type) {
  int start_plane;
  int output = 2; // totally inside
  
  if (type) {
    start_plane = 2;
  } else {
    start_plane = 0;
  }
  
  for (int i = start_plane; i < 6; i++) {
    fixed dp = dp3(tr_frustum.normals[i].x, pos.x,
                   tr_frustum.normals[i].y, pos.y,
                   tr_frustum.normals[i].z, pos.z) + tr_frustum.normals[i].d; // dot product
    
    if (dp >= radius) {
      return 0; // outside
    } else
    if (dp > -radius) {
      output = 1; // partially inside
    }
  }
  
  return output;
}

RAM_CODE int check_frustum_culling_vis(vec3_t pos, fixed radius, u8 type, u8 *clip_flags) {
  int start_plane;
  int output = 2; // totally inside
  *clip_flags = 0;
  
  if (type) {
    start_plane = 2;
  } else {
    start_plane = 0;
  }
  
  for (int i = start_plane; i < 6; i++) {
    fixed dp = dp3(tr_frustum.normals[i].x, pos.x,
                   tr_frustum.normals[i].y, pos.y,
                   tr_frustum.normals[i].z, pos.z) + tr_frustum.normals[i].d; // dot product
    
    if (dp >= radius) {
      return 0; // outside
    } else
    if (dp > -radius) {
      output = 1; // partially inside
      *clip_flags |= 1 << i;
    }
  }
  
  return output;
}