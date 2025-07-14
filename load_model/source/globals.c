#include "../../shared/source/defines.h"
#include "../../shared/source/structs.h"
#include "custom_defines.h"
#include "globals.h"

int g_ls_malloc_count, fp_size_i;
list_u8 faces_textured, textures_has_alpha;

ini_t ini; // ini file
model_t model;
textures_t textures;
mdl_grid_ln_t grid_scn_ln; // linear version of the model grid structure