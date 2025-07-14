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
  vec3_t p0, p1;
} line_t;

typedef struct {
  vec3_t vertices[8];
  u8 num_vertices;
} poly_t;

typedef struct {
  vec5_t vertices[8];
  u8 num_vertices;
} poly_tx_t;

typedef struct {
  vec5_t vertices[8];
  u32 color; // u16
  
  struct {
    u32 is_backface : 1;
    u32 has_texture : 1;
    u32 has_transparency : 1;
    u32 is_visible : 1;
  } flags;
  
  u32 num_vertices; // u8
  u32 texture_width_s; // u16
  u32 texture_height_s; // u16
  u32 texture_width_bits; // u8
  u32 frustum_clip_sides; // u8
  u32 face_type; // u8
  u32 final_light_factor; // u16
  u32 face_id; // u16
  u16 *texture_image;
  u16 *cr_palette_tx_idx;
} g_poly_t;

typedef struct {
  s16 pl;
  s16 vt;
} grid_pnt_t;

typedef struct {
  grid_pnt_t *grid_pnt;
  s16 *pl_data;
  s16 *vt_data;
  size3i_t size_i;
  int num_tiles;
  fixed tile_size;
  u8 tile_size_bits;
} grid_t;

typedef struct {
  int num_vertices;
  int num_faces;
  int num_txcoords;
  int num_tx_faces;
  int num_objects;
  int num_materials;
  int num_sprites;
  int num_sprite_vertices;
  
  struct {
    u8 has_normals : 1;
    u8 has_grid : 1;
    u8 has_textures : 1;
  } flags;
  
  vec3_t origin;
  size3_t size;
  
  vec3_t *vertices;
  u16 *faces;
  vec2_tx_t *txcoords;
  u16 *tx_faces;
  vec3_t *normals;
  u16 *face_index;
  u16 *tx_face_index;
  u8 *face_num_vertices;
  u8 *face_materials;
  u8 *face_types;
  vec3_t *sprite_vertices;
  u8 *sprite_faces;
  s16 *sprite_face_index;
  u16 *object_face_index;
  u16 *object_num_faces;
  size3_t *objects_size;
  u8 *mtl_textures;
  u8 *lines;
  grid_t grid;
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
  
  u16 *material_colors;
  u16 *cr_palette_idx;
  u16 *material_colors_tx;
  u16 *cr_palette_tx_idx;
  
  // size2i_u16_t *texture_sizes;
  size2i_t *texture_sizes_padded;
  u8 *texture_width_bits;
  s8 *tx_animation_id;
  int *tx_index;
  
  u16 *texture_data;
} textures_t;

typedef struct {
  fixed acc;
  fixed rot_speed;
  fixed mouse_rot_speed;
  vec3_t pos;
  vec3_t speed;
  vec3_u16_t rot;
} camera_t;

typedef struct {
  fixed focal_length_i;
  fixed aspect_ratio;
  fixed focal_length;
  fixed hfov;
  fixed vfov;
  fixed half_hfov;
  fixed half_vfov;
  fixed screen_side_x_dt;
  fixed screen_side_y_dt;
} viewport_t;

typedef struct {
  u32 draw_lines : 1;
  u32 draw_polys : 1;
  u32 draw_textures : 1;
  u32 draw_textured_poly : 1;
  u32 perspective_enabled : 1;
  u32 tx_perspective_mapping_enabled : 1;
  u32 static_light : 1;
  u32 directional_lighting : 1;
  u32 draw_normals : 1;
  u32 draw_z_buffer : 1;
  u32 enable_far_plane_clipping : 1;
  u32 draw_grid : 1;
  u32 tx_alpha_cr : 1;
  u32 debug : 1;
  u32 animation_play : 1;
  u32 occlusion_cl_enabled : 1;
  u32 clean_screen : 1;
  u32 face_enable_persp_tx : 1;
} config_t;

typedef struct {
  vec3_t vertices[8];
  vec3_t normals[6];
} frustum_t;

typedef struct {
  vec3_t vertices[8];
  vec4_nm_t normals[6];
} tr_frustum_t;

typedef struct {
  u16* pl_list;
  u16* vt_list;
  u16* obj_list;
  u16* tr_vt_index; // order in which the vertices were stored in vertex_id
  u8* visible_vt_list; // visibility in the frustum of the tile containing the vertex
  
  int num_tiles;
  int num_faces;
  int num_vertices;
  int num_objects;
  int total_vertices;
} display_list_t;

typedef struct {
  u8 bin, pnt;
} occ_buffer_t;

typedef struct {
  u16 id;
  s16 pnt;
  u8 type;
} pl_list_t;

typedef struct {
  s16 *pnt;
  pl_list_t *pl_list;
  u32 size;
} ot_list_t;

/* typedef struct {
  vec3_t pos;
  u8 type;
  void *model;
} object_t; */

typedef struct {
  int pnt;
  int length;
} rle_grid_t;

typedef struct {
  u8 color;
  u8 vis;
  u8 length;
} rle_column_t;

typedef struct {
  size3i_t size_i;
  int pal_size;
  int num_vertices;
  fixed voxel_radius;
  fixed model_radius;
  size3_t size;
  
  vec3_t *vertices;
  rle_grid_t *rle_grid; // - VX_MDL_GRID_MAX_SIZE
  rle_column_t *rle_columns; // - VX_MDL_CLM_MAX_SIZE
  u16 *palette;
  
  vec3_t *tr_vertices;
} vx_model_t;