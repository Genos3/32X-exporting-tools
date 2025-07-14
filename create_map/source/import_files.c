#include <shlwapi.h>
#include "common.h"

void load_ini(char *file_path);
void set_ini_values();
void normalize_file_path(char *file_path);
void load_mtl(char *file_path);
void load_map(char *file_path);
void load_model(char *file_path);
void make_ini_file(char *file_path);
void uncompress_rle_map(tile_t *map);

dlist_char material_list;

// ini file

static char *file_ini_str_i[] = {
  "make_fp", "fp_size", "scale_vt", "make_sprites", "join_objects", "objects_center_xz_origin", "objects_center_y_origin", "merge_vertices", "merge_faces", "limit_dist_face_merge", "face_merge_max_sides", "face_merge_grid_tile_size", "make_grid", "grid_tile_size_bits", "remove_t_junctions", "file_type_export", "model_id", "export_model", "export_textures", "separate_texture_file", "export_texture_data"
};

static int *vars_ini_pnt_i[] = {
  &ini.make_fp, &ini.fp_size, &ini.scale_vt, &ini.make_sprites, &ini.join_objects, &ini.objects_center_xz_origin, &ini.objects_center_y_origin, &ini.merge_vertices, &ini.merge_faces, &ini.limit_dist_face_merge, &ini.face_merge_max_sides, &ini.face_merge_grid_tile_size, &ini.make_grid, &ini.grid_tile_size_bits, &ini.remove_t_junctions, &ini.file_type_export, &ini.model_id, &ini.export_model, &ini.export_textures, &ini.separate_texture_file, &ini.export_texture_data
};

static char *file_ini_str_f[] = {
  "scale_factor_f", "merge_vt_dist_f", "merge_nm_dist_f", "face_merge_dist_f"
};

static float *vars_ini_pnt_f[] = {
  &ini.scale_factor_f, &ini.merge_vt_dist_f, &ini.merge_nm_dist_f, &ini.face_merge_dist_f
};

// model file

static char *file_mdl_str_array_vec3[] = {
  "vertices", "normals"
};

static list_vec3_t *vars_mdl_pnt_array_vec3[] = {
  &tileset.vertices, &tileset.normals
};

static char *file_mdl_str_array_vec2[] = {
  "txcoords"
};

static list_vec2_tx_t *vars_mdl_pnt_array_vec2[] = {
  &tileset.txcoords
};

static char *file_mdl_str_array_size3[] = {
  "objects_size"
};

static list_size3_t *vars_mdl_pnt_array_size3[] = {
  &tileset.objects_size
};

static char *file_mdl_str_array_tx_size[] = {
  "texture_sizes", "texture_sizes_padded"
};

static list_size2i_t *vars_mdl_pnt_array_tx_size[] = {
  &textures.texture_sizes, &textures.texture_sizes_padded
};

static char *file_mdl_str_array_hex[] = {
  "cr_palette_idx", "cr_palette_tx_idx"
};

static list_u16 *vars_mdl_pnt_array_hex[] = {
  &textures.cr_palette_idx, &textures.cr_palette_tx_idx
};

static char *file_mdl_str_array_i[] = {
  "faces", "tx_faces", "face_index", "tx_face_index", "face_num_vertices", "face_materials", "face_types", "object_face_index", "object_num_faces", "mtl_textures", "material_colors", "material_colors_tx", "texture_width_bits", "texture_total_sizes", "tx_animation_id", "tx_index", "texture_data"
};

static list_int *vars_mdl_pnt_array_i[] = {
  &tileset.faces, &tileset.tx_faces, &tileset.face_index, &tileset.tx_face_index, &tileset.face_num_vertices, &tileset.face_materials, &tileset.face_types, &tileset.object_face_index, &tileset.object_num_faces, &tileset.mtl_textures, (list_int*)&textures.material_colors, (list_int*)&textures.material_colors_tx, &textures.texture_width_bits, &textures.texture_total_sizes, &textures.tx_animation_id, &textures.tx_index, (list_int*)&textures.texture_data
};

static char *file_mdl_str_i[] = {
  "num_vertices", "num_faces", "num_txcoords", "num_tx_faces", "num_objects", "num_materials", "has_textures", "num_textures", "num_animations", "pal_size", "pal_num_colors", "pal_size_tx", "pal_tx_num_colors", "lightmap_levels", "texture_data_total_size"
};

static int *vars_mdl_pnt_i[] = {
  &tileset.num_vertices, &tileset.num_faces, &tileset.num_txcoords, &tileset.num_tx_faces, &tileset.num_objects, &tileset.num_materials, (int*)&tileset.has_textures, &textures.num_textures, &textures.num_animations, &textures.pal_size, &textures.pal_num_colors, &textures.pal_size_tx, &textures.pal_tx_num_colors, &textures.lightmap_levels, &textures.texture_data_total_size
};

#define NUM_INI_STR_F (sizeof(vars_ini_pnt_f) / sizeof(float *))
#define NUM_INI_STR_I (sizeof(vars_ini_pnt_i) / sizeof(int *))
#define NUM_MDL_STR_ARRAY_VEC3 (sizeof(vars_mdl_pnt_array_vec3) / sizeof(list_def *))
#define NUM_MDL_STR_ARRAY_VEC2 (sizeof(vars_mdl_pnt_array_vec2) / sizeof(list_def *))
#define NUM_MDL_STR_ARRAY_SIZE3 (sizeof(vars_mdl_pnt_array_size3) / sizeof(list_def *))
#define NUM_MDL_STR_ARRAY_TX_SIZE (sizeof(vars_mdl_pnt_array_tx_size) / sizeof(list_def *))
#define NUM_MDL_STR_ARRAY_HEX (sizeof(vars_mdl_pnt_array_hex) / sizeof(list_int *))
#define NUM_MDL_STR_ARRAY_I (sizeof(vars_mdl_pnt_array_i) / sizeof(list_int *))
#define NUM_MDL_STR_I (sizeof(vars_mdl_pnt_i) / sizeof(int *))

static byte seen_ini_str_f[NUM_INI_STR_F];
static byte seen_ini_str_i[NUM_INI_STR_I];
static byte seen_mdl_str_array_vec3[NUM_MDL_STR_ARRAY_VEC3];
static byte seen_mdl_str_array_vec2[NUM_MDL_STR_ARRAY_VEC2];
static byte seen_mdl_str_array_size3[NUM_MDL_STR_ARRAY_SIZE3];
static byte seen_mdl_str_array_tx_size[NUM_MDL_STR_ARRAY_TX_SIZE];
static byte seen_mdl_str_array_hex[NUM_MDL_STR_ARRAY_HEX];
static byte seen_mdl_str_array_i[NUM_MDL_STR_ARRAY_I];
static byte seen_mdl_str_i[NUM_MDL_STR_I];

void load_files(char *file_path) {
  load_ini(file_path);
  set_ini_values();
  
  // normalize_file_path(file_path);
  
  if (ini.file_type_export == OBJ_FILE) {
    load_mtl(file_path);
  }
  
  load_map(file_path);
  load_model(file_path);
}

void load_ini(char *file_path) {
  FILE *file;
  char ini_path[MAX_PATH];
  char str[STR_SIZE];
  
  strncpy(ini_path, file_path, MAX_PATH - 1);
  PathRemoveFileSpec(ini_path);
  PathAppend(ini_path, "create_map.ini");
  
  file = fopen(ini_path, "r");
  // if the model folder doesn't have an ini file use the one in the program folder
  if (file == NULL) {
    GetModuleFileName(NULL, ini_path, MAX_PATH);
    PathRenameExtension(ini_path, ".ini");
    //PathRemoveFileSpec(ini_path);
    //PathAppend(ini_path, "load_model.ini");
    
    file = fopen(ini_path, "r");
    if (file == NULL) {
      printf("ini file not found\n");
      // if an ini file isn't found create one with some default values
      make_ini_file(ini_path);
      
      file = fopen(ini_path, "r");
      if (file == NULL) {
        printf("can't create an ini file\n");
      }
    }
  }
  
  char *token;
  while (fgets(str, STR_SIZE, file) != NULL) {
    token = strtok(str, "\n");
    token = strtok(str, " =");
    
    if (token == NULL) continue;
    
    for (int i = 0; i < NUM_INI_STR_F; i++) {
      if (!seen_ini_str_f[i] && !strcmp(token, file_ini_str_f[i])) {
        token = strtok(NULL, " =");
        *vars_ini_pnt_f[i] = atof(token);
        seen_ini_str_f[i] = 1;
        goto next;
      }
    }
    
    for (int i = 0; i < NUM_INI_STR_I; i++) {
      if (!seen_ini_str_i[i] && !strcmp(token, file_ini_str_i[i])) {
        token = strtok(NULL, " =");
        *vars_ini_pnt_i[i] = atoi(token);
        seen_ini_str_i[i] = 1;
      }
    }
    
    next:;
  }
  
  fclose(file);
}

void set_ini_values() {
  fp_size_i = 1 << ini.fp_size;
}

void normalize_file_path(char *file_path) {
  char *ext = file_path;
  while (*ext && *ext != '.') ext++;
  
  if (strcmp(ext, ".mdl") != 0) {
    printf("incorrect model file\n");
    exit(1);
  }
  
  // check for "_i" or "_f" just before the extension
  
  if ((ext - 2) >= file_path && 
     ((ext[-2] == '_' && ext[-1] == 'i') ||
      (ext[-2] == '_' && ext[-1] == 'f'))) {
    // remove the suffix by shifting the extension left
    memmove(ext - 2, ext, strlen(ext) + 1);
  }
}

void load_mtl(char *file_path) {
  FILE *file;
  char str[STR_SIZE];
  
  char *mtl_path = strdup(file_path);
  PathRenameExtension(mtl_path, ".mtl");
  
  file = fopen(mtl_path, "r");
  free(mtl_path);
  
  if (file == NULL) {
    printf("mtl file not found\n");
    return;
  }
  
  init_list((list_def*)&material_list, sizeof(*material_list.data));
  char *token;
  
  while (fgets(str, STR_SIZE, file) != NULL) {
    token = strtok(str, "\n");
    token = strtok(str, "\t ");
    
    if (token == NULL) continue;
    
    if (!strcmp(token, "newmtl")) {
      token = strtok(NULL, " ");
      char *str = strdup(token);
      list_push_pnt((list_def*)&material_list, &str);
      tileset.num_materials++;
    }
  }
  
  fclose(file);
}

void load_map(char *file_path) {
  FILE *file;
  //char str[STR_SIZE];
  
  char *path = strdup(file_path);
  // PathRemoveFileSpec(path);
  // PathAppend(path, "map.txt"); // .map
  PathRenameExtension(path, ".map");
  
  file = fopen(path, "r");
  free(path);
  
  if (file == NULL) {
    printf("map file not found\n");
    return;
  }
  
  // obtain file size
  fseek(file, 0, SEEK_END); // seek to end of file
  int size = ftell(file); // get current file pointer
  fseek(file, 0, SEEK_SET); // seek back to beginning of file
  
  // read file data
  char *data = malloc(size + 1);
  size = fread(data, 1, size, file);
  fclose(file);
  data[size] = '\0';
  
  char *token, *saveptr, *saveptr2;
  token = strtok_r(data, ";", &saveptr);
  
  while (token) {
    while (token[0] == '\n' || token[0] == ' ') {
      token++;
    }
    
    if (token[0] == '/' && token[1] == '/') {
      strtok_r(token, "\n", &saveptr2);
      token = saveptr2;
      continue;
    }
    
    token = strtok_r(token, " =", &saveptr2);
    if (token == NULL) goto next;
    
    if (!strcmp(token, "map_size")) {
      token = strtok_r(NULL, " ,=\n", &saveptr2);
      
      map_size.w = atoi(token);
      token = strtok_r(NULL, " ,", &saveptr2);
      map_size.d = atoi(token);
      token = strtok_r(NULL, " ,", &saveptr2);
      map_size.h = atoi(token);
      
      goto next;
    }
    
    if (!strcmp(token, "tile_size")) {
      token = strtok_r(NULL, " ,=\n", &saveptr2);
      
      tile_size.w = atof(token);
      token = strtok_r(NULL, " ,", &saveptr2);
      tile_size.d = atof(token);
      token = strtok_r(NULL, " ,", &saveptr2);
      tile_size.h = atof(token);
      
      goto next;
    }
    
    if (!strcmp(token, "rle_map")) {
      rle_segment_t rle_segment;
      token = strtok_r(NULL, " ,=\n", &saveptr2);
      
      while (token) {
        rle_segment.tile_id = atoi(token);
        token = strtok_r(NULL, " ,\n", &saveptr2);
        rle_segment.length = atoi(token);
        list_push_pnt(&rle_map, &rle_segment);
        token = strtok_r(NULL, " ,\n", &saveptr2);
      }
      
      goto next;
    }
    
    next:
    token = strtok_r(NULL, ";", &saveptr);
  }
  
  if (map_size.w && map_size.d && map_size.h) {
    int map_length = map_size.w * map_size.h * map_size.d;
    map = calloc(map_length, sizeof(tile_t));
    uncompress_rle_map(map);
    // printf("map file loaded\n");
  } else {
    printf("map is empty\n");
  }
  
  free(data);
}

void load_model(char *file_path) {
  FILE *file;
  //char str[STR_SIZE];
  
  char *path = strdup(file_path);
  //PathRemoveFileSpec(path);
  //PathAppend(path, "model.mdl");
  PathRenameExtension(path, ".mdl");
  
  file = fopen(path, "r");
  free(path);
  
  if (file == NULL) {
    printf("mdl file not found\n");
    exit(1);
  }
  
  // obtain file size
  fseek(file, 0, SEEK_END); // seek to end of file
  int size = ftell(file); // get current file pointer
  fseek(file, 0, SEEK_SET); // seek back to beginning of file
  
  // read file data
  char *data = malloc(size + 1);
  size = fread(data, 1, size, file);
  fclose(file);
  data[size] = '\0';
  
  char *token, *saveptr, *saveptr2;
  token = strtok_r(data, ";", &saveptr);
  
  while (token) {
    while (token[0] == '\n' || token[0] == ' ') {
      token++;
    }
    
    if (token[0] == '/' && token[1] == '/') {
      strtok_r(token, "\n", &saveptr2);
      token = saveptr2;
      continue;
    }
    
    token = strtok_r(token, " =", &saveptr2);
    if (token == NULL) goto next;
    
    /* for (int i = 0; i < NUM_MDL_STR_ARRAY_F; i++) {
      if (!seen_mdl_str_array_f[i] && !strcmp(token, file_mdl_str_array_f[i])) {
        token = strtok_r(NULL, " ,=\n", &saveptr2);
        while (token) {
          float value = atof(token);
          list_push_val_type(vars_mdl_pnt_array_f[i], &value, sizeof(float));
          //vars_mdl_pnt_array_f[i][j] = (float*)atof(token);
          token = strtok_r(NULL, " ,\n", &saveptr2);
        }
        seen_mdl_str_array_f[i] = 1;
        goto next;
      }
    } */
    for (int i = 0; i < NUM_MDL_STR_ARRAY_VEC3; i++) {
      if (!seen_mdl_str_array_vec3[i] && !strcmp(token, file_mdl_str_array_vec3[i])) {
        token = strtok_r(NULL, " ,=\n", &saveptr2);
        
        while (token) {
          vec3_t vt;
          vt.x = atof(token);
          token = strtok_r(NULL, " ,", &saveptr2);
          vt.y = atof(token);
          token = strtok_r(NULL, " ,", &saveptr2);
          vt.z = atof(token);
          list_push_pnt((list_def*)vars_mdl_pnt_array_vec3[i], &vt);
          token = strtok_r(NULL, " ,\n", &saveptr2);
        }
        
        seen_mdl_str_array_vec3[i] = 1;
        goto next;
      }
    }
    
    for (int i = 0; i < NUM_MDL_STR_ARRAY_VEC2; i++) {
      if (!seen_mdl_str_array_vec2[i] && !strcmp(token, file_mdl_str_array_vec2[i])) {
        token = strtok_r(NULL, " ,=\n", &saveptr2);
        
        while (token) {
          vec2_tx_t vt;
          vt.u = atof(token);
          token = strtok_r(NULL, " ,", &saveptr2);
          vt.v = atof(token);
          list_push_pnt((list_def*)vars_mdl_pnt_array_vec2[i], &vt);
          token = strtok_r(NULL, " ,\n", &saveptr2);
        }
        
        seen_mdl_str_array_vec2[i] = 1;
        goto next;
      }
    }
    
    for (int i = 0; i < NUM_MDL_STR_ARRAY_SIZE3; i++) {
      if (!seen_mdl_str_array_size3[i] && !strcmp(token, file_mdl_str_array_size3[i])) {
        token = strtok_r(NULL, " ,=\n", &saveptr2);
        
        while (token) {
          size3_t size;
          size.w = atoi(token);
          token = strtok_r(NULL, " ,", &saveptr2);
          size.d = atoi(token);
          token = strtok_r(NULL, " ,", &saveptr2);
          size.h = atoi(token);
          list_push_pnt((list_def*)vars_mdl_pnt_array_size3[i], &size);
          token = strtok_r(NULL, " ,\n", &saveptr2);
        }
        
        seen_mdl_str_array_size3[i] = 1;
        goto next;
      }
    }
    
    for (int i = 0; i < NUM_MDL_STR_ARRAY_TX_SIZE; i++) {
      if (!seen_mdl_str_array_tx_size[i] && !strcmp(token, file_mdl_str_array_tx_size[i])) {
        token = strtok_r(NULL, " ,=\n", &saveptr2);
        
        while (token) {
          size2i_t size;
          size.w = atof(token);
          token = strtok_r(NULL, " ,", &saveptr2);
          size.h = atof(token);
          list_push_pnt((list_def*)vars_mdl_pnt_array_tx_size[i], &size);
          token = strtok_r(NULL, " ,\n", &saveptr2);
        }
        
        seen_mdl_str_array_tx_size[i] = 1;
        goto next;
      }
    }
    
    for (int i = 0; i < NUM_MDL_STR_ARRAY_HEX; i++) {
      if (!seen_mdl_str_array_hex[i] && !strcmp(token, file_mdl_str_array_hex[i])) {
        token = strtok_r(NULL, " ,=\n", &saveptr2);
        
        while (token) {
          list_push_int((list_def*)vars_mdl_pnt_array_hex[i], strtol(token, 0, 16));
          //vars_mdl_pnt_array_i[i][j] = atoi(token);
          token = strtok_r(NULL, " ,\n", &saveptr2);
        }
        
        seen_mdl_str_array_i[i] = 1;
        goto next;
      }
    }
    
    for (int i = 0; i < NUM_MDL_STR_ARRAY_I; i++) {
      if (!seen_mdl_str_array_i[i] && !strcmp(token, file_mdl_str_array_i[i])) {
        token = strtok_r(NULL, " ,=\n", &saveptr2);
        
        while (token) {
          list_push_int((list_def*)vars_mdl_pnt_array_i[i], atoi(token));
          //vars_mdl_pnt_array_i[i][j] = atoi(token);
          token = strtok_r(NULL, " ,\n", &saveptr2);
        }
        
        seen_mdl_str_array_i[i] = 1;
        goto next;
      }
    }
    
    for (int i = 0; i < NUM_MDL_STR_I; i++) {
      if (!seen_mdl_str_i[i] && !strcmp(token, file_mdl_str_i[i])) {
        token = strtok_r(NULL, " =", &saveptr2);
        *vars_mdl_pnt_i[i] = atoi(token);
        seen_mdl_str_i[i] = 1;
        goto next;
      }
    }
    
    next:
    token = strtok_r(NULL, ";", &saveptr);
  }
  
  free(data);
}

void make_ini_file(char *file_path) {
  FILE *file;
  
  file = fopen(file_path, "w");
  if (file == NULL) {
    printf("can't make the ini file\n");
    return;
  }
  
  fprintf(file,
    "// config file\n"
    "make_fp = 1  // make fixed point\n"
    "fp_size = 12  // fixed point size\n"
    "scale_vt = 0  // scale vertices\n"
    "scale_factor_f = 0.25\n"
    "make_sprites = 1\n"
    "join_objects = 1  // move all objects to 0,0\n"
    "objects_center_xz_origin = 0  // put the X and Z origin in the center if join_objects is enabled\n"
    "objects_center_y_origin = 0  // put the Y origin in the center if join_objects is enabled\n"
    "merge_vertices = 1  // merge vertices that are close to each other\n"
    "merge_vt_dist_f = 0.01\n"
    "merge_faces = 0  // merge faces that share the same texture and are coplanar\n"
    "merge_nm_dist_f = 0.1  // difference between the face angle\n"
    "limit_dist_face_merge = 1  // limit distance between faces to merge them\n"
    "face_merge_dist_f = 2\n"
    "face_merge_max_sides = 8  // max number of sides of the merged polygon, max 8\n"
    "face_merge_grid_tile_size = 4  // grid tile size used for optimizing the face search\n"
    "make_grid = 1\n"
    "grid_tile_size_bits = 2  // size of the tile in bits, it can only be a power of two\n"
     // "ini.make_grid_face_index = 1  // export extra fields for the grid\n\n"
    "remove_t_junctions = 1\n\n"
    
    "file_type_export = 0  // 0 = export c file, 1 = export obj file\n"
    "model_id = 0  // id for the exported model\n\n"
    
    "export_model = 1\n"
    "export_textures = 1\n"
    "separate_texture_file = 0\n"
    "export_texture_data = 1"
  );
  
  fclose(file);
}

void uncompress_rle_map(tile_t *map) {
  int map_length = 0;
  
  for (int i = 0; i < rle_map.size; i++) {
    if (!rle_map.data[i].tile_id) {
      map_length += rle_map.data[i].length;
      continue;
    }
    
    for (int j = 0; j < rle_map.data[i].length; j++) {
      int tile_index = map_length / 3;
      int tile_byte_offset = map_length % 3;
      *((u8*)&map[tile_index] + tile_byte_offset) = rle_map.data[i].tile_id;
      map_length++;
    }
  }
}