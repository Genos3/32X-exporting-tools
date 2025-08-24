#include <shlwapi.h>
#include "common.h"

void load_ini(char *file_path);
void set_ini_values();
void load_mtl(char *file_path);
int check_image_repeated(char *token);
void load_material_types(char *file_path);
char* fix_path(char *str);
void load_model(char *file_path, model_t *model);
void make_ini_file(char *file_path);

static dlist_char image_list_path;
dlist_char material_list;

// ini file
static const char *file_ini_str_i[] = {
  "make_fp", "fp_size", "swap_yz", "invert_tx_y", "make_cw", "resize_txcoords", "scale_vt", "make_sprites", "export_objects", "join_objects" ,"objects_center_xz_origin", "objects_center_y_origin", "merge_vertices", "dup_tx_colors", "quantize_tx_colors", "quant_tx_pal_size", "quant_dithering", "quantize_palette", "quant_pal_size", "export_tileset", "export_scene", "merge_faces", "limit_dist_face_merge", "face_merge_max_sides", "face_merge_grid_tile_size", "make_grid", "grid_tile_size_bits", "remove_t_junctions", "file_type_export", "model_id", "export_model", "export_textures", "separate_texture_file", "export_texture_data", "create_lightmap", "lightmap_level_bits", "light_color_r", "light_color_g", "light_color_b", "face_angle_bits", "has_alpha_cr", "alpha_cr_r", "alpha_cr_g", "alpha_cr_b"
};

static int* const vars_ini_pnt_i[] = {
  &ini.make_fp, &ini.fp_size, &ini.swap_yz, &ini.invert_tx_y, &ini.make_cw, &ini.resize_txcoords, &ini.scale_vt, &ini.make_sprites, &ini.export_objects, &ini.join_objects, &ini.objects_center_xz_origin, &ini.objects_center_y_origin, &ini.merge_vertices, &ini.dup_tx_colors, &ini.quantize_tx_colors, &ini.quant_tx_pal_size, &ini.quant_dithering, &ini.quantize_palette, &ini.quant_pal_size, &ini.export_tileset, &ini.export_scene, &ini.merge_faces, &ini.limit_dist_face_merge, &ini.face_merge_max_sides, &ini.face_merge_grid_tile_size, &ini.make_grid, &ini.grid_tile_size_bits, &ini.remove_t_junctions, &ini.file_type_export, &ini.model_id, &ini.export_model, &ini.export_textures, &ini.separate_texture_file, &ini.export_texture_data, &ini.create_lightmap, &ini.lightmap_level_bits, &ini.light_color_r, &ini.light_color_g, &ini.light_color_b, &ini.face_angle_bits, &ini.has_alpha_cr, &ini.alpha_cr_r, &ini.alpha_cr_g, &ini.alpha_cr_b
};

static const char *file_ini_str_f[] = {
  "scale_factor_f", "merge_vt_dist_f", "merge_nm_dist_f", "face_merge_dist_f"
};

static float* const vars_ini_pnt_f[] = {
  &ini.scale_factor_f, &ini.merge_vt_dist_f, &ini.merge_nm_dist_f, &ini.face_merge_dist_f
};

#define NUM_INI_STR_I (sizeof(vars_ini_pnt_i) / sizeof(int *))
#define NUM_INI_STR_F (sizeof(vars_ini_pnt_f) / sizeof(float *))

static byte seen_ini_str_i[NUM_INI_STR_I];
static byte seen_ini_str_f[NUM_INI_STR_F];

void load_files(char *file_path) {
  load_ini(file_path);
  set_ini_values();
  set_alpha_color(&textures);
  load_mtl(file_path);
  load_material_types(file_path);
  load_model(file_path, &model);
}

void load_ini(char *file_path) {
  FILE *file;
  char ini_path[MAX_PATH];
  char str[STR_SIZE];
  
  strncpy(ini_path, file_path, MAX_PATH - 1);
  PathRemoveFileSpec(ini_path);
  PathAppend(ini_path, "load_model.ini");
  
  file = fopen(ini_path, "r");
  
  // if the model folder doesn't have an ini file use the one in the program folder
  if (file == NULL) {
    GetModuleFileName(NULL, ini_path, MAX_PATH);
    PathRenameExtension(ini_path, ".ini");
    //PathRemoveFileSpec(ini_path);
    //PathAppend(ini_path, "load_model.ini");
    file = fopen(ini_path, "r");
    
    if (file == NULL) {
      printf("ini file not found");
      // if an ini file isn't found create one with some default values
      make_ini_file(ini_path);
      file = fopen(ini_path, "r");
      
      if (file == NULL) {
        printf("can't create an ini file");
      }
    }
  }
  
  char *token;
  while (fgets(str, STR_SIZE, file) != NULL) {
    token = strtok(str, "\n");
    token = strtok(str, " =/");
    if (token == NULL) continue;
    
    for (int i = 0; i < NUM_INI_STR_I; i++) {
      if (!seen_ini_str_i[i] && !strcmp(token, file_ini_str_i[i])) {
        token = strtok(NULL, " =");
        *vars_ini_pnt_i[i] = atoi(token);
        seen_ini_str_i[i] = 1;
        goto next;
      }
    }
    
    for (int i = 0; i < NUM_INI_STR_F; i++) {
      if (!seen_ini_str_f[i] && !strcmp(token, file_ini_str_f[i])) {
        token = strtok(NULL, " =");
        *vars_ini_pnt_f[i] = atof(token);
        seen_ini_str_f[i] = 1;
      }
    }
    
    next:;
  }
  
  fclose(file);
}

void set_ini_values() {
  fp_size_i = 1 << ini.fp_size;
  
  if (ini.export_tileset) {
    // ini.make_fp = 0;
    ini.export_scene = 0;
  }
  
  if (!ini.export_scene) {
    ini.make_grid = 0;
  } else {
    ini.export_objects = 0;
  }
}

void load_mtl(char *file_path) {
  FILE *file;
  char str[STR_SIZE];
  // char *mtl_path = strdup(file_path);
  char mtl_path[MAX_PATH];
  
  strncpy(mtl_path, file_path, MAX_PATH - 1);
  PathRenameExtension(mtl_path, ".mtl");
  
  file = fopen(mtl_path, "r");
  if (file == NULL) {
    printf("mtl file not found");
    // free(mtl_path);
    return;
  }
  
  init_list(&material_list, sizeof(*material_list.data));
  init_list(&image_list_path, sizeof(*image_list_path.data));
  
  char *token;
  PathRemoveFileSpec(mtl_path);
  
  while (fgets(str, STR_SIZE, file) != NULL) {
    token = strtok(str, "\n");
    token = strtok(str, "\t ");
    
    if (token == NULL) continue;
    
    // int c = 0;
    // if (test_loop(&c, 10)) exit(1);
    
    if (!strcmp(token, "newmtl")) { // new material
      token = strtok(NULL, " ");
      char *str = strdup(token);
      list_push_pnt(&material_list, &str);
      list_malloc_inc(&model.materials);
      model.materials.data[model.num_materials].texture_id = 0;
      model.materials.data[model.num_materials].type = 0;
      model.materials.data[model.num_materials].colors = 0;
      model.materials.data[model.num_materials].colors_tx = 0;
      model.num_materials++;
    }
    else if (!strcmp(token, "map_Kd")) { // new texture
      char path[MAX_PATH];
      strncpy(path, mtl_path, MAX_PATH);
      token = strtok(NULL, "\n");
      
      if (check_image_repeated(token)) continue;
      
      char *str = strdup(token);
      list_push_pnt(&image_list_path, &str);
      list_malloc_inc(&textures.tx_group);
      
      textures.lightmap_levels = 1 << ini.lightmap_level_bits;
      textures.lightmap_level_bits = ini.lightmap_level_bits;
      
      #if EXPORT_AVERAGE_PAL
        for (int i = 0; i < textures.lightmap_levels; i++) {
          list_malloc_inc(&textures.cr_palette_idx);
        }
        
        list_malloc_inc(&textures.cr_palette);
      #endif
      
      //token = fix_path(token);
      PathAppend(path, token);
      
      process_image(path, &textures);
      
      list_push_int(&textures_has_alpha, tx_has_alpha);
      
      textures.num_textures++;
    }
  }
  
  for (int i = 0; i < textures.num_textures; i++) {
    free(image_list_path.data[i]);
  }
  free_list(&image_list_path);
  // free(mtl_path);
  fclose(file);
}

int check_image_repeated(char *token) {
  for (int i = 0; i < textures.num_textures; i++) {
    if (!strcmp(token, image_list_path.data[i])) {
      model.materials.data[model.num_materials - 1].texture_id = i;
      return 1;
    }
  }
  
  model.materials.data[model.num_materials - 1].texture_id = textures.num_textures;
  return 0;
}

void load_material_types(char file_path[]) {
  FILE *file;
  // int file_size = 0;
  // char *path = strdup(file_path);
  char path[MAX_PATH];
  strncpy(path, file_path, MAX_PATH - 1);
  // PathRenameExtension(path, ".txt");
  PathRemoveFileSpec(path);
  PathAppend(path, "bitflags.txt");
  file = fopen(path, "r");
  // free(path);
  
  if (file == NULL) {
    printf("bitflags.txt not found");
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
  
  char *token;
  int num_material_types = 0;
  token = strtok(data, "\n");
  
  while (token) {
    token = strtok(data, "\t /");
    if (token == NULL) continue;
    
    if (!strcmp(token, "t")) {
      while ((token = strtok(NULL, " "))) {
        if (!model.num_materials) {
          list_malloc_inc(&model.materials);
        }
        
        model.materials.data[num_material_types].type = atoi(token);
        num_material_types++;
      }
    }
  }
  
  free(data);
}

char* fix_path(char *str) {
  /* if (*str == '/') {
    str++;
  } */
  for (char *p = str; *p; p++) {
    if (*p == '/') {
      *p = '\\';
    }
  }
  
  return str;
}

// parse obj file

void load_model(char *file_path, model_t *model) {
  FILE *file;
  char str[STR_SIZE];
  // char *path = strdup(file_path);
  char path[MAX_PATH];
  strncpy(path, file_path, MAX_PATH - 1);
  PathRenameExtension(path, ".obj");
  file = fopen(file_path, "r");
  // free(path);
  
  if (file == NULL) {
    printf("obj file not found");
    exit(1);
  }
  
  char *token, *saveptr;
  char *prev_mtl = "";
  int current_material = 0;
  
  object_t *object = model->objects.data;
  
  if (!ini.export_objects) {
    list_malloc_inc(&model->objects);
    init_object_struct(object);
    
    model->num_objects = 1;
  }
  
  while (fgets(str, STR_SIZE, file) != NULL) {
    token = strtok_r(str, "\n", &saveptr);
    token = strtok_r(str, " ", &saveptr);
    if (token == NULL) continue;
    
    if (!strcmp(token, "v")) { // vertices
      vec3_t vt;
      token = strtok_r(NULL, " ", &saveptr);
      vt.x = atof(token);
      token = strtok_r(NULL, " ", &saveptr);
      vt.y = atof(token);
      token = strtok_r(NULL, " ", &saveptr);
      vt.z = atof(token);
      list_push_pnt(&object->vertices, &vt);
      
      object->num_vertices++;
    }
    else if (!strcmp(token, "vt")) { // texture coordinates
      vec2_tx_t vt;
      token = strtok_r(NULL, " ", &saveptr);
      vt.u = atof(token);
      token = strtok_r(NULL, " ", &saveptr);
      vt.v = atof(token);
      list_push_pnt(&object->txcoords, &vt);
      
      object->num_txcoords++;
    }
    else if (!strcmp(token, "usemtl")) { // material
      token = strtok_r(NULL, " ", &saveptr);
      
      if (!strcmp(token, prev_mtl)) continue;
      
      prev_mtl = strdup(token);
      
      for (int i = 0; i < model->num_materials; i++) {
        if (!strcmp(token, material_list.data[i])) {
          current_material = i;
          break;
        }
      }
    }
    else if (!strcmp(token, "f")) { // faces
      int num_vt = 0;
      int face_textured = 0;
      char *saveptr2;
      //token = strtok(NULL, " ");
      
      while ((token = strtok_r(NULL, " ", &saveptr))) {
        token = strtok_r(token, "/", &saveptr2);
        list_malloc_inc(&object->faces);
        // decrement the face index
        object->faces.data[object->num_faces].vt_index[num_vt] = atoi(token) - 1;
        
        if (saveptr2[0] != '/') {
          token = strtok_r(NULL, "/", &saveptr2);
          if (token) {
            // if (!(object->face_types.data[object->num_faces] & UNTEXTURED)) {
            face_textured = 1;
            object->faces.data[object->num_faces].tx_vt_index[num_vt] = atoi(token) - 1;
            // }
          }
        }
        
        num_vt++;
      }
      
      if (num_vt > 8) {
        printf("face %d has %d vertices, aborting", object->faces.size, num_vt);
        exit(1);
      }
      
      object->faces.data[object->num_faces].has_texture = face_textured;
      object->faces.data[object->num_faces].num_vertices = num_vt;
      object->faces.data[object->num_faces].material_id = current_material;
      object->faces.data[object->num_faces].texture_id = model->materials.data[current_material].texture_id;
      object->faces.data[object->num_faces].type = 0;
      object->faces.data[object->num_faces].remove = 0;
      object->num_faces++;
    }
    else if (ini.export_objects && !strcmp(token, "o")) { // object
      list_malloc_inc(&model->objects);
      object = &model->objects.data[model->num_objects];
      init_object_struct(object);
      
      object->num_vertices = 0;
      object->num_txcoords = 0;
      object->num_faces = 0;
      object->num_sprites = 0;
      
      model->num_objects++;
    }
  }
  
  free(prev_mtl);
  
  if (ini.file_type_export != OBJ_FILE) {
    for (int i = 0; i < model->num_materials; i++) {
      free(material_list.data[i]);
    }
    
    free_list(&material_list);
  }
  
  fclose(file);
}

void make_ini_file(char *file_path) {
  FILE *file;
  file = fopen(file_path, "w");
  if (file == NULL) {
    printf("can't make the ini file");
    return;
  }
  
  fprintf(file,
    "// config file\n"
    "make_fp = 1  // make fixed point\n"
    "fp_size = 8  // fixed point size\n"
    "swap_yz = 0  // exchange y and z\n"
    "invert_tx_y = 1  // invert y on textured coords\n"
    "make_cw = 1  // make clockwise\n"
    "resize_txcoords = 1  // pre multiply the texture coordinates by the texture size\n"
    "// enable_types = 1  // allow faces to have different types (sprite, animated)\n"
    "scale_vt = 0  // scale vertices\n"
    "scale_factor_f = 0.1\n"
    "make_sprites = 1\n"
    "export_objects = 1\n"
    "join_objects = 0  // move all objects to 0,0\n"
    "objects_center_xz_origin = 0  // put the X and Z origin in the center if join_objects is enabled\n"
    "objects_center_y_origin = 0  // put the Y origin in the center if join_objects is enabled\n"
    "merge_vertices = 1  // merge vertices that are close to each other\n"
    "merge_vt_dist_f = 0.01\n"
    "dup_tx_colors = 1  // duplicates the color indices inside the textures\n"
    "quantize_tx_colors = 1\n"
    "quant_tx_pal_size = 16\n"
    "quant_dithering = 0\n"
    "quantize_palette = 1\n"
    "quant_pal_size = 248\n\n"
    
    "// presets\n"
    "export_tileset = 0  // export a tileset to be imported, if enabled export_scene is disabled\n"
    "export_scene = 0  // if enabled export_objects is disabled otherwise merge_faces and make_grid are disabled\n\n"
    
    "merge_faces = 0  // merge faces that share the same texture and are coplanar\n"
    "merge_nm_dist_f = 0.2  // difference between the face angle, 0.2\n"
    "limit_dist_face_merge = 0  // limit distance between faces to merge them\n"
    "face_merge_dist_f = 2  // distance for merging the faces\n"
    "face_merge_max_sides = 8  // max number of sides of the merged polygon, max 8\n"
    "face_merge_grid_tile_size = 4  // grid tile size used for optimizing the face search\n"
    "make_grid = 1\n"
    "grid_tile_size_bits = 2  // size of the tile in bits, it can only be a power of two\n"
    "remove_t_junctions = 1\n\n"
    
    "file_type_export = 0  // 0 = export c file, 1 = export obj file, 2 = export mdl file\n"
    "model_id = 0  // id for the exported model\n\n"
    
    "export_model = 1\n"
    "export_textures = 1\n"
    "export_texture_data = 1\n"
    "create_lightmap = 1\n"
    "lightmap_level_bits = 3\n"
    "light_color_r = 31\n"
    "light_color_g = 31\n"
    "light_color_b = 31\n"
    "face_angle_bits = 3\n"
    "has_alpha_cr = 1  // if one of the textures has an alpha color\n"
    "alpha_cr_r = 168  // 76\n"
    "alpha_cr_g = 230  // 105\n"
    "alpha_cr_b = 29  // 113"
  );
  
  fclose(file);
}