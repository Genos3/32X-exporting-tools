#include "defines.h"
#include "engine_structs.h"
#include "scene_structs.h"
#include "scene_defines.h"
#include "scene_globals.h"

u8 cursor_enabled, cursor_mode, cursor_axis, select_multiple, map_grid_enabled, move_selection, copy_selection, curr_selected_tile;
s8 cursor_dx_acc, cursor_dy_acc;
vec3_t cursor_pos, m_sel_cursor_pos;
int selected_tile, num_tiles;
u8 *shared_face_dir;
aabb_t aabb_select, aabb_move;
size3_t aabb_move_size;
list_rle_segment_t rle_map;

list_model_t ts_list_model; // tileset model
list_textures_t ts_list_textures; // tileset textures
model_t ts_model;
textures_t ts_textures;

orbit_camera_t orbit_cam;

tile_t map[MAX_MAP_HEIGHT][MAX_MAP_DEPTH][MAX_MAP_WIDTH] = {0};
tile_t *copy_buffer;