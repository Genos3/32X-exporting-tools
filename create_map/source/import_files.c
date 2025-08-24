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

#define NUM_INI_STR_F (sizeof(vars_ini_pnt_f) / sizeof(float *))
#define NUM_INI_STR_I (sizeof(vars_ini_pnt_i) / sizeof(int *))

static byte seen_ini_str_f[NUM_INI_STR_F];
static byte seen_ini_str_i[NUM_INI_STR_I];

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
  // char str[STR_SIZE];
  
  char *path = strdup(file_path);
  // PathRemoveFileSpec(path);
  // PathAppend(path, "model.mdl");
  PathRenameExtension(path, ".mdl");
  
  file = fopen(path, "r");
  free(path);
  
  if (file == NULL) {
    printf("mdl file not found\n");
    exit(1);
  }
  
  // obtain the file size
  fseek(file, 0, SEEK_END); // seek to end of file
  int size = ftell(file); // get current file pointer
  fseek(file, 0, SEEK_SET); // seek back to beginning of file
  
  // read the file data
  char *data = malloc(size + 1);
  size = fread(data, 1, size, file);
  fclose(file);
  data[size] = '\0';
  
  char *token, *saveptr, *saveptr2;
  token = strtok_r(data, ";", &saveptr);
  
  int obj_id = -1;
  
  while (token) {
    while (token[0] == '\n' || token[0] == ' ') {
      token++;
    }
    
    if (token[0] == '/' && token[1] == '/') {
      strtok_r(token, "\n", &saveptr2);
      token = saveptr2;
      continue;
    }
    
    token = strtok_r(token, " =\n", &saveptr2);
    if (token == NULL) goto next;
    
    // model
    
    if (!strcmp(token, "num_objects")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      model->num_objects = atoi(token);
      
      if (!model->num_objects) {
        fprintf(file, "no objects found\n");
        exit(1);
      }
      
      list_malloc_size(&model->objects, model->num_objects);
      
      for (int i = 0; i < model->num_objects; i++) {
        object_t *object = &model->objects.data[i];
        init_object_struct(object);
        
        object->num_sprites = 0;
      }
      
      goto next;
    }
    
    // objects
    
    object_t *object;
    
    if (!strcmp(token, "o")) { // object
      obj_id++;
      object = &model->objects.data[obj_id];
      
      goto next;
    }
    
    if (!strcmp(token, "num_vertices")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      object->num_vertices = atoi(token);
      
      list_malloc_size(&object->vertices, object->num_vertices);
      
      goto next;
    }
    
    if (!strcmp(token, "num_txcoords")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      object->num_txcoords = atoi(token);
      
      list_malloc_size(&object->txcoords, object->num_txcoords);
      
      goto next;
    }
    
    if (!strcmp(token, "num_faces")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      object->num_faces = atoi(token);
      
      list_malloc_size(&object->faces, object->num_faces);
      
      goto next;
    }
    
    if (!strcmp(token, "num_sprites")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      object->num_sprites = atoi(token);
      
      list_malloc_size(&object->sprites, object->num_sprites);
      
      goto next;
    }
    
    if (!strcmp(token, "num_sprite_vertices")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      object->num_sprite_vertices = atoi(token);
      
      list_malloc_size(&object->sprite_vertices, object->num_sprite_vertices);
      
      goto next;
    }
    
    if (!strcmp(token, "flags")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      int flags = atoi(token);
      
      object->has_grid = flags >> 1;
      object->has_textures = flags & 1;
      
      goto next;
    }
    
    if (!strcmp(token, "origin")) {
      token = strtok_r(NULL, " ,=", &saveptr2);
      
      object->origin.x = atof(token);
      token = strtok_r(NULL, " ,", &saveptr2);
      object->origin.y = atof(token);
      token = strtok_r(NULL, " ,;", &saveptr2);
      object->origin.z = atof(token);
      
      goto next;
    }
    
    if (!strcmp(token, "size")) {
      token = strtok_r(NULL, " ,=", &saveptr2);
      
      object->size.w = atof(token);
      token = strtok_r(NULL, " ,", &saveptr2);
      object->size.d = atof(token);
      token = strtok_r(NULL, " ,;", &saveptr2);
      object->size.h = atof(token);
      
      goto next;
    }
    
    if (!strcmp(token, "vertices")) {
      token = strtok_r(NULL, " ,=", &saveptr2);
      
      for (int i = 0; i < object->num_vertices; i++) {
        object->vertices.data[i].x = atof(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        object->vertices.data[i].y = atof(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        object->vertices.data[i].z = atof(token);
      }
      
      goto next;
    }
    
    if (!strcmp(token, "txcoords")) {
      token = strtok_r(NULL, " ,=", &saveptr2);
      
      for (int i = 0; i < object->num_txcoords; i++) {
        object->txcoords.data[i].u = atof(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        object->txcoords.data[i].v = atof(token);
      }
      
      goto next;
    }
    
    if (!strcmp(token, "face_group")) {
      token = strtok_r(NULL, " ,=", &saveptr2);
      
      for (int i = 0; i < object->num_faces; i++) {
        face_t *face = &object->faces.data[i];
        
        face->num_vertices = atoi(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        int material = atoi(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        face->type = atoi(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        face->normal.x = atof(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        face->normal.y = atof(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        face->normal.z = atof(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        face->angle = atoi(token);
        
        if (face->type & TEXTURED) {
          face->texture_id = material;
          face->has_texture = 1;
        } else {
          face->has_texture = 0;
        }
        
        face->remove = 0;
      }
      
      goto next;
    }
    
    if (!strcmp(token, "face_data")) {
      token = strtok_r(NULL, " ,=", &saveptr2);
      
      for (int i = 0; i < object->num_faces; i++) {
        face_t *face = &object->faces.data[i];
        int num_vertices = face->num_vertices;
        
        if (num_vertices == 3) { // triangle
          for (int j = 0; j < 3; j++) {
            face->vt_index[j] = atoi(token);
            token = strtok_r(NULL, " ,", &saveptr2);
          }
          
          if (face->data[i].has_textures) {
            for (int j = 0; j < 3; j++) {
              face->tx_vt_index[j] = atoi(token);
              token = strtok_r(NULL, " ,", &saveptr2);
            }
          }
        }
        else if (num_vertices == 4) { // quad
          for (int j = 0; j < 4; j++) {
            face->vt_index[j] = atoi(token);
            token = strtok_r(NULL, " ,", &saveptr2);
          }
          
          if (face->data[i].has_textures) {
            for (int j = 0; j < 4; j++) {
              face->tx_vt_index[j] = atoi(token);
              token = strtok_r(NULL, " ,", &saveptr2);
            }
          }
        }
        else if (num_vertices == 1) { // sprite          
          sprite_t *sprite = &object->sprites.data[object->num_sprites];
          
          for (int j = 0; j < 5; j++) {
            face->vt_index[j] = atoi(token);
            token = strtok_r(NULL, " ,", &saveptr2);
          }
          
          for (int j = 0; j < 4; j++) {
            face->tx_vt_index[j] = atoi(token);
            token = strtok_r(NULL, " ,", &saveptr2);
          }
          
          object->num_sprites++;
        }
      }
      
      goto next;
    }
    
    // textures
    
    if (!strcmp(token, "num_textures")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      textures->num_textures = atoi(token);
      
      goto next;
    }
    
    if (!strcmp(token, "num_animations")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      textures->num_animations = atoi(token);
      
      goto next;
    }
    
    if (!strcmp(token, "pal_size")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      textures->pal_size = atoi(token);
      
      goto next;
    }
    
    if (!strcmp(token, "pal_size_tx")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      textures->pal_size_tx = atoi(token);
      
      goto next;
    }
    
    if (!strcmp(token, "lightmap_level_bits")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      textures->lightmap_level_bits = atoi(token);
      
      textures->lightmap_levels = 1 << textures->lightmap_level_bits;
      
      goto next;
    }
    
    if (!strcmp(token, "texture_data_total_size")) {
      token = strtok_r(NULL, " =", &saveptr2);
      
      textures->texture_data_total_size = atoi(token);
      
      goto next;
    }
    
    if (!strcmp(token, "cr_palette_idx")) {
      token = strtok_r(NULL, " ,=", &saveptr2);
      
      for (int i = 0; i < textures->pal_size; i++) {
        textures->cr_palette_idx = atoi(token);
        token = strtok_r(NULL, " ,\n", &saveptr2);
      }
      
      goto next;
    }
    if (!strcmp(token, "cr_palette_tx_idx")) {
      token = strtok_r(NULL, " ,=", &saveptr2);
      
      for (int i = 0; i < textures->pal_size_tx; i++) {
        textures->cr_palette_tx_idx = atoi(token);
        token = strtok_r(NULL, " ,\n", &saveptr2);
      }
      
      goto next;
    }
    
    if (!strcmp(token, "tx_group")) {
      token = strtok_r(NULL, " ,=", &saveptr2);
      
      for (int i = 0; i < textures->num_textures; i++) {
        tx_group_t *tx_group = &textures->tx_group.data[i];
        
        textures->size_padded_wh = atoi(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        textures->width_bits = atoi(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        textures->animation_id = atoi(token);
        token = strtok_r(NULL, " ,", &saveptr2);
        textures->tx_index = atoi(token);
      }
      
      goto next;
    }
    
    if (!strcmp(token, "texture_data")) {
      token = strtok_r(NULL, " ,=", &saveptr2);
      
      list_malloc_size(&object->texture_data, object->texture_data_total_size);
      
      for (int i = 0; i < textures->texture_data_total_size; i++) {
        textures->texture_data.data[i] = atoi(token);
        token = strtok_r(NULL, ",\n", &saveptr2);
      }
      
      goto next;
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