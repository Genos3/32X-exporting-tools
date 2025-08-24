typedef struct {
  vec3_t vertices[8];
  vec2_tx_t txcoords[8];
  int num_vertices;
  u8 has_texture;
} poly_t;

typedef struct {
  vec3_t min, max;
} aabb_t;

list(aabb_t);

typedef struct {
  vec3i_t min, max;
} aabb_i_t;

list(aabb_i_t);

typedef struct {
  vec2_t min, max;
} aabb_2d_t;

typedef struct {
  vec2_tx_t min, max;
} tx_aabb_t;

typedef struct {
  int next, data;
} lnk_ls_node_t;

list(lnk_ls_node_t);

typedef struct {
  int back_pnt, next, prev, data;
} d_lnk_ls_node_t;

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
  int *data_pnt;
  int *data_last;
  size3i_t size_i;
  int num_tiles;
  int tile_size_i;
  u8 tile_size_bits;
} grid_t;

// object grid

typedef struct {
  int *pl_pnt;
  int *pl_last;
  int *tile_num_faces;
  lnk_ls_t pl_data;
  size3i_t size_i;
  int num_tiles;
  int tile_size_i;
  u8 tile_size_bits;
} obj_grid_t;

typedef struct {
  int *pl_pnt;
  int *vt_pnt;
  list_int pl_data;
  list_int vt_data;
  size3i_t size_i;
  int num_tiles;
  int tile_size_i;
  u8 tile_size_bits;
} obj_grid_ln_t; // linear version of the model grid structure

typedef struct {
  int vt_index[8];
  int tx_vt_index[8];
  vec3_t vertices[8];
  vec2_tx_t txcoords[8];
  int num_vertices;
  int material_id;
  int texture_id;
  int type;
  int angle;
  u8 has_texture;
  u8 remove;
  vec3_t normal;
} face_t;

list(face_t);

typedef struct {
  int type;
  int texture_id;
  u16 colors;
  u16 colors_tx;
} material_t;

list(material_t);

typedef struct {
  int num_vertices;
  int num_txcoords;
  int num_faces;
  int num_sprites;
  int num_sprite_vertices;
  u8 has_textures;
  u8 has_grid;
  vec3_t origin;
  size3_t size;
  
  list_vec3_t vertices;
  list_vec2_tx_t txcoords;
  list_vec3_t sprite_vertices;
  list_face_t faces;
  obj_grid_ln_t grid_ln;
} object_t;

list(object_t);

typedef struct {
  int num_materials;
  int num_objects;
  u8 has_textures;
  u8 has_grid;
  
  list_material_t materials;
  list_object_t objects;
  list_int objects_id;
} model_t;

typedef struct {
  size2i_t size;
  size2i_t size_padded;
  int width_bits;
  int total_size;
  int animation_id;
  int tx_index;
  u8 has_alpha;
} tx_group_t;

list(tx_group_t);

typedef struct {
  int num_textures;
  int num_animations;
  int pal_size;
  int pal_num_colors;
  int pal_size_tx;
  int pal_tx_num_colors;
  int lightmap_levels;
  int lightmap_level_bits;
  int texture_data_total_size;
  
  list_u16 cr_palette_idx;
  list_u16 cr_palette_tx_idx;
  list_u16 cr_palette;
  list_u16 cr_palette_tx;
  
  list_tx_group_t tx_group;
  
  list_u16 texture_data;
} textures_t;