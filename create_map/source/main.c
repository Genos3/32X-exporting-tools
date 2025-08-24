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
  ini.scale_vt = 0; // scale vertices
  ini.scale_factor_f = 0.25;
  ini.make_sprites = 1;
  ini.join_objects = 1; // move all objects to 0,0
  ini.objects_center_xz_origin = 0; // put the X and Z origin in the center if join_objects is enabled
  ini.objects_center_y_origin = 0; // put the Y origin in the center if join_objects is enabled
  ini.merge_vertices = 1; // merge vertices that are close to each other
  ini.merge_vt_dist_f = 0.01;
  ini.merge_faces = 1; // merge faces that share the same texture and are coplanar
  ini.merge_nm_dist_f = 0.1; // difference between the face angle
  ini.limit_dist_face_merge = 1; // limit distance between faces to merge them
  ini.face_merge_dist_f = 2;
  ini.face_merge_max_sides = 8; // max number of sides of the merged polygon, max 8
  ini.face_merge_grid_tile_size = 4; // grid tile size used for optimizing the face search
  ini.make_grid = 1;
  ini.grid_tile_size_bits = 2; // size of the tile in bits, it can only be a power of two
  // ini.make_grid_face_index = 1; // export extra fields for the grid
  ini.remove_t_junctions = 1;
  
  ini.file_type_export = 0; // 0 = export c file, 1 = export obj file
  ini.model_id = 0; // id for the exported model
  
  ini.export_model = 1;
  ini.export_textures = 1;
  ini.separate_texture_file = 0;
  ini.export_texture_data = 1;
  
  g_ls_malloc_count = 0;
  
  // merge_uv_dist = ini.merge_vt_dist_f / 32;
  init_model();
  init_memory();
}

void init_model() {
  model.num_materials = 0;
  model.num_objects = 0;
  model.has_textures = 0;
  model.has_grid = 0;
  
  textures.num_textures = 0;
  textures.num_animations = 0;
  textures.pal_size = 0;
  textures.pal_size_tx = 0;
  textures.texture_data_total_size = 0;
}

void init_memory() {
  init_list(&rle_map, sizeof(*rle_map.data));
  
  init_model_struct(&tileset);
  init_model_struct(&scene);
  init_textures_struct(&textures);
}

void free_memory() {
  free_list(&rle_map);
  
  for (int i = 0; i < tilese.num_objects; i++) {
    free_object_memory(&tilese.objects.data[i]);
  }
  
  free_object_memory(&scene.objects.data[0]);
  
  if (ini.make_grid) {
    free_ln_grid_list(&scene.objects.data[0].grid_ln);
  }
  
  free_model_memory(&tileset);
  free_model_memory(&scene);
  free_textures_memory(&textures);
}