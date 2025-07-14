#include <shlwapi.h>
#include "common.h"

void load_model(char *file_path);
void load_map(char *file_path);
void uncompress_rle_map();

#define FP_SIZE (1 << FP)

// ts_list_model file

static char *file_mdl_str_array_vec3[] = {
  "vertices", "normals", "sprite_vertices"
};

static list_vec3_t *vars_mdl_pnt_array_vec3[] = {
  &ts_list_model.vertices, &ts_list_model.normals, &ts_list_model.sprite_vertices
};

static char *file_mdl_str_array_vec2[] = {
  "txcoords"
};

static list_vec2_tx_t *vars_mdl_pnt_array_vec2[] = {
  &ts_list_model.txcoords
};

static char *file_mdl_str_array_size3[] = {
  "objects_size"
};

static list_size3_t *vars_mdl_pnt_array_size3[] = {
  &ts_list_model.objects_size
};

static char *file_mdl_str_array_tx_size[] = {
  "texture_sizes_padded"
};

static list_size2i_t *vars_mdl_pnt_array_tx_size[] = {
  &ts_list_textures.texture_sizes_padded
};

static char *file_mdl_str_array_hex[] = {
  "cr_palette_idx", "cr_palette_tx_idx"
};

static list_u16 *vars_mdl_pnt_array_hex[] = {
  &ts_list_textures.cr_palette_idx, &ts_list_textures.cr_palette_tx_idx
};

static char *file_mdl_str_array_i[] = {
  "faces", "tx_faces", "face_index", "tx_face_index", "face_num_vertices", "face_materials", "face_types", "sprite_faces", "sprite_face_index", "object_face_index", "object_num_faces", "mtl_textures", "material_colors", "material_colors_tx", "texture_width_bits", "tx_animation_id", "tx_index", "texture_data"
};

static list_int *vars_mdl_pnt_array_i[] = {
  (list_int*)&ts_list_model.faces, (list_int*)&ts_list_model.tx_faces, (list_int*)&ts_list_model.face_index, (list_int*)&ts_list_model.tx_face_index, (list_int*)&ts_list_model.face_num_vertices, (list_int*)&ts_list_model.face_materials, (list_int*)&ts_list_model.face_types, (list_int*)&ts_list_model.sprite_faces, (list_int*)&ts_list_model.sprite_face_index, (list_int*)&ts_list_model.object_face_index, (list_int*)&ts_list_model.object_num_faces, (list_int*)&ts_list_model.mtl_textures, (list_int*)&ts_list_textures.material_colors, (list_int*)&ts_list_textures.material_colors_tx, (list_int*)&ts_list_textures.texture_width_bits, (list_int*)&ts_list_textures.tx_animation_id, &ts_list_textures.tx_index, (list_int*)&ts_list_textures.texture_data
};

static char *file_mdl_str_i[] = {
  "num_vertices", "num_faces", "num_txcoords", "num_tx_faces", "num_objects", "num_materials", "num_sprites", "num_sprite_vertices", "has_normals", "has_grid", "has_textures", "num_textures", "num_animations", "pal_size", "pal_num_colors", "pal_size_tx", "pal_tx_num_colors", "lightmap_levels", "texture_data_total_size"
};

static int *vars_mdl_pnt_i[] = {
  &ts_list_model.num_vertices, &ts_list_model.num_faces, &ts_list_model.num_txcoords, &ts_list_model.num_tx_faces, &ts_list_model.num_objects, &ts_list_model.num_materials, &ts_list_model.num_sprites, &ts_list_model.num_sprite_vertices, (int*)&ts_list_model.has_normals, (int*)&ts_list_model.has_grid, (int*)&ts_list_model.has_textures, &ts_list_textures.num_textures, &ts_list_textures.num_animations, &ts_list_textures.pal_size, &ts_list_textures.pal_num_colors, &ts_list_textures.pal_size_tx, &ts_list_textures.pal_tx_num_colors, &ts_list_textures.lightmap_levels, &ts_list_textures.texture_data_total_size
};

#define NUM_MDL_STR_ARRAY_VEC3 (sizeof(vars_mdl_pnt_array_vec3) / sizeof(list_def *))
#define NUM_MDL_STR_ARRAY_VEC2 (sizeof(vars_mdl_pnt_array_vec2) / sizeof(list_def *))
#define NUM_MDL_STR_ARRAY_SIZE3 (sizeof(vars_mdl_pnt_array_size3) / sizeof(list_def *))
#define NUM_MDL_STR_ARRAY_TX_SIZE (sizeof(vars_mdl_pnt_array_tx_size) / sizeof(list_def *))
#define NUM_MDL_STR_ARRAY_HEX (sizeof(vars_mdl_pnt_array_hex) / sizeof(list_int *))
#define NUM_MDL_STR_ARRAY_I (sizeof(vars_mdl_pnt_array_i) / sizeof(list_int *))
#define NUM_MDL_STR_I (sizeof(vars_mdl_pnt_i) / sizeof(int *))

static byte seen_mdl_str_array_vec3[NUM_MDL_STR_ARRAY_VEC3];
static byte seen_mdl_str_array_vec2[NUM_MDL_STR_ARRAY_VEC2];
static byte seen_mdl_str_array_size3[NUM_MDL_STR_ARRAY_SIZE3];
static byte seen_mdl_str_array_tx_size[NUM_MDL_STR_ARRAY_TX_SIZE];
static byte seen_mdl_str_array_hex[NUM_MDL_STR_ARRAY_HEX];
static byte seen_mdl_str_array_i[NUM_MDL_STR_ARRAY_I];
static byte seen_mdl_str_i[NUM_MDL_STR_I];

void load_files(char *file_path) {
  load_model(file_path);
  load_map(file_path);
}

void load_model(char *file_path) {
  FILE *file;
  //char str[STR_SIZE];
  
  char *path = strdup(file_path);
  //PathRemoveFileSpec(path);
  //PathAppend(path, "ts_list_model.mdl");
  PathRenameExtension(path, ".mdl");
  
  file = fopen(path, "r");
  free(path);
  
  if (file == NULL) {
    printf("mdl file not found\n");
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
    
    for (int i = 0; i < NUM_MDL_STR_ARRAY_VEC3; i++) {
      if (!seen_mdl_str_array_vec3[i] && !strcmp(token, file_mdl_str_array_vec3[i])) {
        token = strtok_r(NULL, " ,=\n", &saveptr2);
        
        while (token) {
          vec3_t vt;
          vt.x = atof(token) * FP_SIZE;
          token = strtok_r(NULL, " ,", &saveptr2);
          vt.y = atof(token) * FP_SIZE;
          token = strtok_r(NULL, " ,", &saveptr2);
          vt.z = atof(token) * FP_SIZE;
          list_push_pnt(vars_mdl_pnt_array_vec3[i], &vt);
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
          vt.u = atof(token) * FP_SIZE;
          token = strtok_r(NULL, " ,", &saveptr2);
          vt.v = atof(token) * FP_SIZE;
          list_push_pnt(vars_mdl_pnt_array_vec2[i], &vt);
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
          size.w = atof(token) * FP_SIZE;
          token = strtok_r(NULL, " ,", &saveptr2);
          size.d = atof(token) * FP_SIZE;
          token = strtok_r(NULL, " ,", &saveptr2);
          size.h = atof(token) * FP_SIZE;
          list_push_pnt(vars_mdl_pnt_array_size3[i], &size);
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
          size.w = atoi(token);
          token = strtok_r(NULL, " ,", &saveptr2);
          size.h = atoi(token);
          list_push_pnt(vars_mdl_pnt_array_tx_size[i], &size);
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
        
        seen_mdl_str_array_hex[i] = 1;
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
  } else {
    printf("map file found\n");
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
  
  size3_t map_size;
  
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
      
      map_size.w = atof(token);
      token = strtok_r(NULL, " ,", &saveptr2);
      map_size.d = atof(token);
      token = strtok_r(NULL, " ,", &saveptr2);
      map_size.h = atof(token);
      
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
  
  if (map_size.w > MAX_MAP_WIDTH || map_size.h > MAX_MAP_HEIGHT || map_size.d > MAX_MAP_DEPTH) {
    printf("map size is larger than the max size\n");
  } else
  if (map_size.w && map_size.d && map_size.h) {
    uncompress_rle_map(map_size);
    printf("map file loaded\n");
  } else {
    printf("map is empty\n");
  }
  
  free(data);
}

void uncompress_rle_map(size3_t map_size) {
  int map_byte_offset = 0;
  // u8 *_map = (u8 *)map;
  
  for (int i = 0; i < rle_map.size; i++) {
    if (!rle_map.data[i].tile_id) {
      map_byte_offset += rle_map.data[i].length;
      continue;
    }
    
    for (int j = 0; j < rle_map.data[i].length; j++) {
      int tile_index = map_byte_offset / 3;
      int tile_byte_offset = map_byte_offset % 3;
      
      int h = tile_index / (map_size.d * map_size.w);
      int d = (tile_index % (map_size.d * map_size.w)) / map_size.w;
      int w = tile_index % map_size.w;
      
      // u8* tile_ptr = (u8*)&map[h][d][w];
      // tile_ptr[tile_byte_offset] = rle_map.data[i].tile_id;
      
      *((u8*)&map[h][d][w] + tile_byte_offset) = rle_map.data[i].tile_id;
      // _map[map_byte_offset] = rle_map.data[i].tile_id;
      map_byte_offset++;
    }
  }
}