#include "../../shared/source/defines.h"
#include "../../shared/source/structs.h"
#include "custom_defines.h"
#include "globals.h"

int g_ls_malloc_count, fp_size_i;
size3_t map_size, tile_size;

ini_t ini; // ini file
model_t tileset; // tileset model
model_t scene; // final map model
textures_t textures; // tileset textures
mdl_grid_ln_t grid_scn_ln; // linear version of the model grid structure

tile_t *map;
list_rle_segment_t rle_map;