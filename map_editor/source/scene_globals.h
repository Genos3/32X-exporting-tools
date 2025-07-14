#define MAX_MAP_WIDTH 255
#define MAX_MAP_DEPTH 255
#define MAX_MAP_HEIGHT 100

#define TILE_HALF_HEIGHT 1

#define TILE_SIZE (1 << FP)
#define HALF_TILE_SIZE (TILE_SIZE >> 1)

#if TILE_HALF_HEIGHT
  #define TILE_HEIGHT HALF_TILE_SIZE
#else
  #define TILE_HEIGHT TILE_SIZE
#endif
// #define TILE_RADIUS DIAG_UNIT_DIST_RC

#define CL_DIST fix(0.25)
#define CL_HEIGHT fix(0.7)

#define SELECT_MODE 0
#define PAINT_MODE 1

extern u8 cursor_enabled, cursor_mode, cursor_axis, select_multiple, map_grid_enabled, move_selection, copy_selection, curr_selected_tile;
extern s8 cursor_dx_acc, cursor_dy_acc;
extern vec3_t cursor_pos, m_sel_cursor_pos;
extern int selected_tile, num_tiles;
extern u8 *shared_face_dir;
extern aabb_t aabb_select, aabb_move;
extern size3_t aabb_move_size;
extern list_rle_segment_t rle_map;

extern list_model_t ts_list_model;
extern list_textures_t ts_list_textures;
extern model_t ts_model;
extern textures_t ts_textures;

extern tile_t map[MAX_MAP_HEIGHT][MAX_MAP_DEPTH][MAX_MAP_WIDTH];
extern tile_t *copy_buffer;

extern orbit_camera_t orbit_cam;