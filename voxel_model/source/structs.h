#define PALETTE_SIZE 256

typedef struct {
  u8 x, y, z;
  u8 color_index;
} voxel_t;

typedef struct {
  u8 r, g, b, a;
} color_t;

typedef struct {
  u8 color_index;
  u8 vis;
} voxel_vis_t;

typedef struct {
  int pnt;
  int length;
} grid_t;

typedef struct {
  u8 color_index;
  u8 vis;
  u8 length;
} rle_columns_t;

list(rle_columns_t);

typedef struct {
  int length;
  int num_voxels;
  size3i_t size_i;
  int rle_grid_size;
  int pal_size;
  float voxel_radius;
  float model_radius;
  size3_t size;
  
  voxel_vis_t *voxels;
  
  grid_t *rle_grid;
  list_rle_columns_t rle_columns;
  
  vec3_t vertices[8];
  u16 palette[PALETTE_SIZE];
} vx_model_t;