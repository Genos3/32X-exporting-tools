void init_game();
void check_tile_collision();

#define CL_DIST fix(0.25)
#define CL_HEIGHT fix(0.7)

extern map_scn_t map_scn;
extern game_t game;
extern map_cl_t *map_cl;
extern u8 animation_frames[2];
extern const u8 flower_anim_frames[], water_anim_frames[];