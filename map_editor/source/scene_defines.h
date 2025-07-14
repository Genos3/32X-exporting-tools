#define UPDATE_RATE 30

#define SKY_NUM_COLOR_GRADIENT_BITS 3
#define SKY_NUM_COLOR_GRADIENT (1 << SKY_NUM_COLOR_GRADIENT_BITS)
#define PAL_SKY_GRD_OFFSET (240 - SKY_NUM_COLOR_GRADIENT)

#define OT_FACE_TYPE 0
#define OT_OBJECT_TYPE 1

#define OBJ_SPRITE 0
#define OBJ_VOXEL 1
#define OBJ_MODEL 2

#define SCN_MAX_NUM_OBJECTS 256  // maximum number of objects in the scene

#define NORTH 0
#define SOUTH 1
#define WEST 2
#define EAST 3
#define UP 4
#define DOWN 5

#define LEFT_DIR 1
#define RIGHT_DIR 2
#define UP_DIR 4
#define DOWN_DIR 8


typedef struct {
  u16 start_color, end_color;
  rgb_t start_color_ch;
  rgb_t end_color_ch;
  u16 start_angle, end_angle;
  fixed fp_deg2px;
} sky_t;

typedef struct {
  vec3_t pos;
  vec3_u16_t rot;
  u8 static_pos, static_rot;
  void *mdl_pnt;
  u8 type;
} object_t;

typedef struct {
  vec3_t lightdir;
  vec3_t lightdir_n;
  u16 bg_color;
  u8 directional_lighting_enabled: 1;
  u8 draw_sky: 1;
  u32 curr_model; // u8
  u32 cam_curr_map; // u8
  
  object_t obj_list[SCN_MAX_NUM_OBJECTS];
  vec3_t obj_tr_pos[SCN_MAX_NUM_OBJECTS];
} scene_t;

typedef struct {
  int tile_id;
  int length;
} rle_segment_t;

list(rle_segment_t);

typedef struct {
  u8 x, y, z;
} tile_t;

typedef struct {
  float zoom, pitch, yaw;
} orbit_camera_t;