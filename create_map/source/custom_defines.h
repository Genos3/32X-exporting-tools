#pragma once

// ini file
typedef struct {
  int make_fp, fp_size, scale_vt, make_sprites, join_objects, objects_center_xz_origin, objects_center_y_origin, merge_vertices, merge_faces, limit_dist_face_merge, face_merge_max_sides, face_merge_grid_tile_size, make_grid, grid_tile_size_bits, remove_t_junctions, file_type_export, model_id, export_model, export_textures, separate_texture_file, export_texture_data;
  
  float scale_factor_f, merge_vt_dist_f, merge_nm_dist_f, face_merge_dist_f;
} ini_t;

typedef struct {
  u8 x, y, z;
} tile_t;

typedef struct {
  int tile_id;
  int length;
} rle_segment_t;

list(rle_segment_t);