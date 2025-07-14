#define C_FILE 0
#define OBJ_FILE 1
#define MDL_FILE 2

#define LIGHT_GRD 16
#define SPECULAR_LIGHT 0
#define LIGHT_GRD_T (LIGHT_GRD << SPECULAR_LIGHT)
#define EXPORT_AVERAGE_PAL 1

extern int g_ls_malloc_count, fp_size_i;
extern list_u8 faces_textured, textures_has_alpha;

extern ini_t ini;
extern model_t model;
extern textures_t textures;
extern mdl_grid_ln_t grid_scn_ln;