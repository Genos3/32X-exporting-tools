#include <math.h>
#include "common.h"

void set_cam_pos() {
  cam.pos.x = fix(0);
  cam.pos.z = fix(5);
  cam.pos.y = fix(1);
  cam.rot.x = 0;
  cam.rot.y = 128 << 8;
}

void set_multiple_tiles(vec3_t *p0, vec3_t *p1, int tile_id, model_t *model) {
  vec3_t min, max;
  min.x = min_c(p0->x, p1->x);
  min.y = min_c(p0->y, p1->y);
  min.z = min_c(p0->z, p1->z);
  max.x = max_c(p0->x, p1->x);
  max.y = max_c(p0->y, p1->y);
  max.z = max_c(p0->z, p1->z);
  
  int w, d, h;
  if (tile_id) {
    w = (max_c(model->objects_size[tile_id - 1].w - 1, 0) >> FP) + 1;
    d = (max_c(model->objects_size[tile_id - 1].d - 1, 0) >> FP) + 1;
    #if TILE_HALF_HEIGHT
      h = (max_c(model->objects_size[tile_id - 1].h - 1, 0) >> (FP - 1)) + 1;
    #else
      h = (max_c(model->objects_size[tile_id - 1].h - 1, 0) >> FP) + 1;
    #endif
    
    if (!w) w = 1;
    if (!d) d = 1;
    if (!h) h = 1;
  } else {
    w = 1;
    d = 1;
    h = 1;
  }
  
  for (int i = min.y; i <= max.y; i += h) {
    for (int j = min.z; j <= max.z; j += d) {
      for (int k = min.x; k <= max.x; k += w) {
        if (cursor_mode == SELECT_MODE) {
          map[i][j][k].y = tile_id;
          map[i][j][k].x = tile_id;
          map[i][j][k].z = tile_id;
        } else {
          *((u8*)&map[i][j][k] + cursor_axis) = tile_id;
        }
      }
    }
  }
}

void set_aabb(vec3_t *p0, vec3_t *p1, aabb_t *aabb) {
  aabb->min.x = min_c(p0->x, p1->x);
  aabb->min.y = min_c(p0->y, p1->y);
  aabb->min.z = min_c(p0->z, p1->z);
  aabb->max.x = max_c(p0->x, p1->x);
  aabb->max.y = max_c(p0->y, p1->y);
  aabb->max.z = max_c(p0->z, p1->z);
}

void copy_selected_area() {
  aabb_move_size.w = aabb_select.max.x - aabb_select.min.x + 1;
  aabb_move_size.h = aabb_select.max.y - aabb_select.min.y + 1;
  aabb_move_size.d = aabb_select.max.z - aabb_select.min.z + 1;
  
  int aabb_size = aabb_move_size.w * aabb_move_size.h * aabb_move_size.d;
  copy_buffer = malloc(aabb_size * sizeof(tile_t));
  int n = 0;
  
  for (int i = aabb_select.min.y; i <= aabb_select.max.y; i++) {
    for (int j = aabb_select.min.z; j <= aabb_select.max.z; j++) {
      for (int k = aabb_select.min.x; k <= aabb_select.max.x; k++) {
        copy_buffer[n] = map[i][j][k];
        n++;
      }
    }
  }
}

void paste_selected_area() {
  int n = 0;
  
  for (int i = aabb_move.min.y; i <= aabb_move.max.y; i++) {
    for (int j = aabb_move.min.z; j <= aabb_move.max.z; j++) {
      for (int k = aabb_move.min.x; k <= aabb_move.max.x; k++) {
        map[i][j][k] = copy_buffer[n];
        n++;
      }
    }
  }
}

void move_selected_area() {
  for (int i = aabb_select.min.y; i <= aabb_select.max.y; i++) {
    for (int j = aabb_select.min.z; j <= aabb_select.max.z; j++) {
      for (int k = aabb_select.min.x; k <= aabb_select.max.x; k++) {
        map[i][j][k].y = 0;
        map[i][j][k].x = 0;
        map[i][j][k].z = 0;
      }
    }
  }
  
  int n = 0;
  
  for (int i = aabb_move.min.y; i <= aabb_move.max.y; i++) {
    for (int j = aabb_move.min.z; j <= aabb_move.max.z; j++) {
      for (int k = aabb_move.min.x; k <= aabb_move.max.x; k++) {
        map[i][j][k] = copy_buffer[n];
        n++;
      }
    }
  }
  
  free(copy_buffer);
}

void set_camera_to_cursor() {
  vec3_t cursor_pos_ft;
  cursor_pos_ft.x = (cursor_pos.x << FP) + fix(0.5);
  cursor_pos_ft.z = (cursor_pos.z << FP) + fix(0.5);
  #if TILE_HALF_HEIGHT
    cursor_pos_ft.y = cursor_pos.y << (FP - 1);
  #else
    cursor_pos_ft.y = cursor_pos.y << FP;
  #endif
  
  cam.pos.x = cursor_pos_ft.x + fix(orbit_cam.zoom * cos_deg(orbit_cam.pitch) * sin_deg(orbit_cam.yaw));
  cam.pos.y = cursor_pos_ft.y + fix(orbit_cam.zoom * sin_deg(orbit_cam.pitch));
  cam.pos.z = cursor_pos_ft.z + fix(orbit_cam.zoom * cos_deg(orbit_cam.pitch) * cos_deg(orbit_cam.yaw));
  
  look_at(&cursor_pos_ft, &cam);
}

void look_at(vec3_t *pnt, camera_t *cam) {
  float dx = pnt->x - cam->pos.x;
  float dy = pnt->y - cam->pos.y;
  float dz = pnt->z - cam->pos.z;
  
  float yaw_angle = atan2_deg(dx, dz) / 360 * 256;
  float pitch_angle = atan2_deg(-dy, sqrt(dx * dx + dz * dz)) / 360 * 256;
  
  if (yaw_angle < 0) {
    yaw_angle += 256;
  }
  
  if (pitch_angle < 0) {
    pitch_angle += 256;
  }
  
  cam->rot.y = fix8(yaw_angle);
  cam->rot.x = fix8(pitch_angle);
  // printf("rot y %.4f\n", yaw_angle);
  // printf("rot x %.4f\n", pitch_angle);
}

void init_memory() {
  init_list(&rle_map, sizeof(*rle_map.data));
  
  init_model_struct(&ts_list_model);
  init_textures_struct(&ts_list_textures);
}

void free_memory() {
  free_list(&rle_map);
  
  free_model_memory(&ts_list_model);
  free_textures_memory(&ts_list_textures);
  
  if (copy_buffer) {
    free(copy_buffer);
  }
}

void set_model_list_pnt(model_t *model) {
  model->num_vertices = ts_list_model.num_vertices;
  model->num_txcoords = ts_list_model.num_txcoords;
  model->num_faces = ts_list_model.num_faces;
  model->num_tx_faces = ts_list_model.num_tx_faces;
  model->num_objects = ts_list_model.num_objects;
  model->num_materials = ts_list_model.num_materials;
  model->num_sprites = ts_list_model.num_sprites;
  model->num_sprite_vertices = ts_list_model.num_sprite_vertices;
  model->flags.has_normals = ts_list_model.has_normals;
  model->flags.has_grid = ts_list_model.has_grid;
  model->flags.has_textures = ts_list_model.has_textures;
  
  model->origin = ts_list_model.origin;
  model->size = ts_list_model.size;
  
  model->vertices = ts_list_model.vertices.data;
  model->faces = ts_list_model.faces.data;
  model->txcoords = ts_list_model.txcoords.data;
  model->tx_faces = ts_list_model.tx_faces.data;
  model->normals = ts_list_model.normals.data;
  model->face_num_vertices = ts_list_model.face_num_vertices.data;
  model->face_materials = ts_list_model.face_materials.data;
  model->face_types = ts_list_model.face_types.data;
  model->face_index = ts_list_model.face_index.data;
  model->tx_face_index = ts_list_model.tx_face_index.data;
  model->sprite_vertices = ts_list_model.sprite_vertices.data;
  model->sprite_faces = ts_list_model.sprite_faces.data;
  model->sprite_face_index = ts_list_model.sprite_face_index.data;
  model->object_face_index = ts_list_model.object_face_index.data;
  model->object_num_faces = ts_list_model.object_num_faces.data;
  model->objects_size = ts_list_model.objects_size.data;
  model->mtl_textures = ts_list_model.mtl_textures.data;
}

void set_textures_list_pnt(textures_t *textures) {
  textures->num_textures = ts_list_textures.num_textures;
  textures->num_animations = ts_list_textures.num_animations;
  textures->pal_size = ts_list_textures.pal_size;
  textures->pal_num_colors = ts_list_textures.pal_num_colors;
  textures->pal_size_tx = ts_list_textures.pal_size_tx;
  textures->pal_tx_num_colors = ts_list_textures.pal_tx_num_colors;
  textures->lightmap_levels = ts_list_textures.lightmap_levels;
  textures->texture_data_total_size = ts_list_textures.texture_data_total_size;
  
  textures->material_colors = ts_list_textures.material_colors.data;
  textures->cr_palette_idx = ts_list_textures.cr_palette_idx.data;
  textures->material_colors_tx = ts_list_textures.material_colors_tx.data;
  textures->cr_palette_tx_idx = ts_list_textures.cr_palette_tx_idx.data;
  textures->texture_sizes_padded = ts_list_textures.texture_sizes_padded.data;
  textures->texture_width_bits = ts_list_textures.texture_width_bits.data;
  textures->tx_animation_id = ts_list_textures.tx_animation_id.data;
  textures->tx_index = ts_list_textures.tx_index.data;
  textures->texture_data = ts_list_textures.texture_data.data;
}