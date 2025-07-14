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
  u8 has_textures;
  u8 has_grid;
  vec3_t origin;
  size3_t size;
  
  list_vec3_t vertices;
  list_vec2_tx_t txcoords;
  list_int faces;
  list_int tx_faces;
  list_int face_index;
  list_int tx_face_index;
  list_vec3_t normals;
  list_int face_num_vertices;
  list_int face_materials;
  list_int face_types;
  list_int material_types;
  list_int mtl_textures;
  list_vec3_t sprite_vertices;
  list_int sprite_faces;
  list_int sprite_face_index;
  list_int object_face_index;
  list_int object_num_faces;
  list_int object_vt_list;
  list_int object_num_vertices;
  list_int object_vt_index;
  list_vec3_t objects_origin;
  list_size3_t objects_size;
  
  // vec3_t *objects_origin;
  // size3_t *objects_size;
  u8 *type_vt;
} model_t;

typedef struct {
  int num_textures;
  int num_animations;
  int pal_size;
  int pal_num_colors;
  int pal_size_tx;
  int pal_tx_num_colors;
  int lightmap_levels;
  int texture_data_total_size;
  
  list_u16 cr_palette_idx;
  list_u16 cr_palette_tx_idx;
  list_u16 cr_palette;
  list_u16 cr_palette_tx;
  list_u16 material_colors;
  list_u16 material_colors_tx;
  
  list_int texture_width_bits;
  list_int texture_total_sizes;
  list_int tx_index;
  list_int tx_animation_id;
  
  list_size2i_t texture_sizes;
  list_size2i_t texture_sizes_padded;
  
  list_u16 texture_data;
} textures_t;

typedef struct {
  vec3_t vertices[8];
  vec2_tx_t txcoords[8];
  int num_vertices;
  int has_texture;
} poly_t;

typedef struct {
  vec3_t min, max;
} aabb_t;

typedef struct {
  vec3i_t min, max;
} aabb_i_t;

typedef struct {
  vec2_t min, max;
} aabb_2d_t;

typedef struct {
  int next, data;
} lnk_ls_node_t;

typedef struct {
  int back_pnt, next, prev, data;
} d_lnk_ls_node_t;

list(aabb_t);
list(aabb_i_t);
list(lnk_ls_node_t);
list(d_lnk_ls_node_t);

typedef struct {
  list_lnk_ls_node_t nodes;
  int length;
} lnk_ls_t;

typedef struct {
  list_d_lnk_ls_node_t nodes;
  int length;
} d_lnk_ls_t;

// normal grid

typedef struct {
  int *data_pnt, *data_last;
  size3i_t size_i;
  int num_tiles;
  int tile_size_i;
  u8 tile_size_bits;
} grid_t;

// model grid

typedef struct {
  int *pl_pnt, *pl_last, *tile_num_faces;
  lnk_ls_t pl_data;
  size3i_t size_i;
  int num_tiles;
  int tile_size_i;
  u8 tile_size_bits;
} mdl_grid_t;

typedef struct {
  int *pl_pnt, *vt_pnt; // *grid_pnt;
  list_int pl_data, vt_data; // faces, face_index, num_faces, num_vertices
  // list_aabb_t grid_aabb;
  size3i_t size_i;
  int num_tiles;
  int tile_size_i;
  u8 tile_size_bits;
} mdl_grid_ln_t;

typedef struct {
  list_vec3_t vertices;
  list_vec2_tx_t txcoords;
  list_int face_num_vertices;
  int num_faces;
} pl_list_t;