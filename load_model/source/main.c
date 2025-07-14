#include "common.h"

void init();
void init_model();
void init_memory();
void free_memory();

int main(int argc, char *argv[]) {
  if (argc < 2) return 1;
  
  init();
  
  char *file_path = argv[1];
  char *export_path = NULL;
  
  if (argc == 3){
    export_path = argv[2];
  }
  
  load_files(file_path);
  process_data();
  make_file(argc, file_path, export_path);
  free_memory();
}

void init() {
  ini.make_fp = 1; // make fixed point
  ini.fp_size = 8; // fixed point size
  ini.swap_yz = 0; // exchange y and z
  ini.invert_tx_y = 1; // invert y on mtl_textured coords
  ini.make_cw = 0; // make clockwise
  ini.resize_txcoords = 1; // pre multiply the texture coordinates by the texture size
  // ini.enable_types = 1; // allow faces to have different types (untextured, backface, sprite, animated)
  ini.scale_vt = 0; // scale vertices
  ini.scale_factor_f = 0.1;
  ini.make_sprites = 1;
  ini.export_objects = 1;
  ini.join_objects = 0; // move all objects to 0,0
  ini.objects_center_xz_origin = 0; // put the X and Z origin in the center if join_objects is enabled
  ini.objects_center_y_origin = 0; // put the Y origin in the center if join_objects is enabled
  ini.merge_vertices = 1; // merge vertices that are close to each other
  ini.merge_vt_dist_f = 0.01;
  ini.dup_tx_colors = 1; // duplicates the color indices inside the textures
  ini.quantize_tx_colors = 1;
  ini.quant_tx_pal_size = 16;
  ini.quant_dithering = 0;
  ini.quantize_palette = 1;
  ini.quant_pal_size = 248;
  
  // presets
  ini.export_tileset = 0; // export a tileset to be imported, if enabled export_scene is disabled
  ini.export_scene = 0; // if enabled export_objects is disabled otherwise merge_faces and make_grid are disabled
  
  ini.merge_faces = 0; // merge faces that share the same texture and are coplanar
  ini.merge_nm_dist_f = 0.1; // difference between the face angle
  ini.limit_dist_face_merge = 1; // limit distance between faces to merge them
  ini.face_merge_dist_f = 2;
  ini.face_merge_max_sides = 8; // max number of sides of the merged polygon, max 8
  ini.face_merge_grid_tile_size = 4; // grid tile size used for optimizing the face search
  ini.make_grid = 1;
  ini.grid_tile_size_bits = 2; // size of the tile in bits, it can only be a power of two
  ini.remove_t_junctions = 1;
  
  ini.file_type_export = 0; // 0 = export c file, 1 = export mdl file, 2 = export obj file
  ini.model_id = 0; // id for the exported model
  
  ini.export_model = 1;
  ini.export_textures = 1;
  ini.separate_texture_file = 0;
  ini.export_texture_data = 1;
  ini.create_lightmap = 1;
  ini.lightmap_levels = 8;
  ini.light_color_r = 31;
  ini.light_color_g = 31;
  ini.light_color_b = 31;
  ini.has_alpha_cr = 1; // if one of the textures has an alpha color
  ini.alpha_cr_r = 76;
  ini.alpha_cr_g = 105;
  ini.alpha_cr_b = 113;
  
  g_ls_malloc_count = 0;
  
  // merge_uv_dist = ini.merge_vt_dist_f / 32;
  init_model();
  init_memory();
}

void init_model() {
  model.num_vertices = 0;
  model.num_txcoords = 0;
  model.num_faces = 0;
  model.faces_size = 0;
  model.num_tx_faces = 0;
  model.tx_faces_size = 0;
  model.num_objects = 0;
  model.num_materials = 0;
  model.num_sprites = 0;
  model.num_sprite_vertices = 0;
  model.has_textures = 0;
  
  textures.num_textures = 0;
  textures.num_animations = 0;
  textures.pal_size = 0;
  textures.pal_size_tx = 0;
  textures.texture_data_total_size = 0;
}

void init_memory() {
  init_model_struct(&model);
  init_textures_struct(&textures);
  
  init_list(&faces_textured, sizeof(*faces_textured.data));
  init_list(&textures_has_alpha, sizeof(*textures_has_alpha.data));
}

void free_memory() {
  if (ini.make_grid) {
    free_ln_grid_list(&grid_scn_ln);
  }
  
  free_model_memory(&model);
  free_textures_memory(&textures);
  
  free_list(&faces_textured);
  free_list(&textures_has_alpha);
}