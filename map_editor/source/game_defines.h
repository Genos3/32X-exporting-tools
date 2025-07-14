#define ANIM_TIMER_INTERVAL fix(0.43) // 26 frames per second in the original game

#define SCN_NUM_MAPS 0
#define VIEW_SINGLE_MAP 0

typedef struct {
  fixed tile_size, tile_height, rc_tile_size, rc_tile_height, tile_radius, half_tile_size, slope;
  aabb_t map_aabb[SCN_NUM_MAPS];
} map_scn_t;

typedef struct {
  const int width, depth, height;
  const u8 *fl_height, *cl_tiles;
} map_cl_t;

typedef struct {
  fixed_u general_timer;
  u8 flower_texture_id, water_texture_id;
} animation_t;

typedef struct {
  animation_t anim;
  fixed gravity, cam_speed_y_limit;
  u8 collision_enabled, gravity_enabled, cam_is_on_ground;
} game_t;