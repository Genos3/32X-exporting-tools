#pragma once

// ini file
typedef struct {
  // custom
  int swap_yz, invert_tx_y, make_cw, export_objects, resize_txcoords, scale_vt, face_angle_bits, create_lightmap, lightmap_level_bits, has_alpha_cr, dup_tx_colors, quantize_tx_colors, quant_tx_pal_size, quant_dithering, quantize_palette, quant_pal_size, light_color_r, light_color_g, light_color_b, alpha_cr_r, alpha_cr_g, alpha_cr_b, export_tileset, export_scene;
  
  float scale_factor_f;
  
  // shared
  int make_fp, fp_size, make_sprites, join_objects, objects_center_xz_origin, objects_center_y_origin, merge_vertices, merge_faces, limit_dist_face_merge, face_merge_max_sides, face_merge_grid_tile_size, make_grid, grid_tile_size_bits, remove_t_junctions, file_type_export, model_id, export_model, export_textures, separate_texture_file, export_texture_data;
  
  float merge_vt_dist_f, merge_nm_dist_f, face_merge_dist_f;
} ini_t;