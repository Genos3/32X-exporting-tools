#define C_FILE 0
#define OBJ_FILE 1

#define EXPORT_AVERAGE_PAL 1

extern int g_ls_malloc_count, fp_size_i;
extern size3_t map_size, tile_size;

extern ini_t ini;
extern model_t tileset;
extern model_t scene;
extern textures_t textures;
extern mdl_grid_ln_t grid_scn_ln;

extern tile_t *map;
extern list_rle_segment_t rle_map;