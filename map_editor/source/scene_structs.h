typedef struct {
  int num_vertices;
  int num_txcoords;
  int num_faces;
  int faces_size;
  int num_tx_faces;
  int tx_faces_size;
  int num_objects;
  int num_materials;
  int num_sprites;
  int num_sprite_vertices;
  u8 has_normals;
  u8 has_grid;
  u8 has_textures;
  vec3_t origin;
  size3_t size;
  
  list_vec3_t vertices;
  list_vec2_tx_t txcoords;
  list_u16 faces;
  list_u16 tx_faces;
  list_u16 face_index;
  list_u16 tx_face_index;
  list_vec3_t normals;
  list_u8 face_num_vertices;
  list_u8 face_materials;
  list_u8 face_types;
  list_u8 mtl_textures;
  list_vec3_t sprite_vertices;
  list_u8 sprite_faces;
  list_s16 sprite_face_index;
  list_u16 object_face_index;
  list_u16 object_num_faces;
  list_size3_t objects_size;
} list_model_t;

typedef struct {
  int num_textures;
  int num_animations;
  int pal_size;
  int pal_num_colors;
  int pal_size_tx;
  int pal_tx_num_colors;
  int lightmap_levels;
  int texture_data_total_size;
  
  list_u16 material_colors;
  list_u16 cr_palette_idx;
  list_u16 material_colors_tx;
  list_u16 cr_palette_tx_idx;
  
  // list_size2i_t texture_sizes;
  list_size2i_t texture_sizes_padded;
  
  list_u8 texture_width_bits;
  list_s8 tx_animation_id;
  list_int tx_index;
  
  list_u16 texture_data;
} list_textures_t;