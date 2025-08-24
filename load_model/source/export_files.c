#include <shlwapi.h>
#include "common.h"

void make_file_c_mdl(char *export_path);
void make_file_c_tex(char *export_path);
void export_model_file(FILE *file, model_t *model);
void export_object_file(int obj_id, FILE *file, object_t *object);
void export_texture_file(FILE *file, textures_t *textures, model_t *model);
void make_file_mdl(char *export_path);
void make_file_mdl_object(FILE *file, object_t *object);
void make_file_mdl_textures(FILE *file, textures_t *textures, model_t *model);
void make_file_obj(char *export_path);

void make_file(int argc, char *file_path, char *export_path) {
  if (argc < 3) {
    export_path = strdup(file_path);
  } else {
    // try to create the directory if it doesn't exist
    if (!PathFileExists(export_path) && !CreateDirectory(export_path, NULL)) {
      printf("can't create the export directory\n");
      return;
    }
    
    PathStripPath(file_path);
    PathAppend(export_path, file_path);
  }
  
  if (ini.file_type_export == C_FILE) {
    if (ini.export_model) {
      make_file_c_mdl(export_path);
    }
    
    if (ini.export_textures && (ini.separate_texture_file || !ini.export_model)) {
      make_file_c_tex(export_path);
    }
  }
  else if (ini.file_type_export == MDL_FILE) {
    make_file_mdl(export_path);
  }
  else if (ini.file_type_export == OBJ_FILE) {
    make_file_obj(export_path);
  }
  
  if (argc < 3) {
    free(export_path);
  }
}

void make_file_c_mdl(char *export_path) {
  FILE *file;
  char path[MAX_PATH];
  strncpy(path, export_path, MAX_PATH - 1);
  
  PathRenameExtension(path, ".c");
  
  file = fopen(path, "w");
  
  if (file == NULL) {
    printf("can't make the model file\n");
    return;
  }
  
  fprintf(file,
    "#include \"../../shared/source/defines.h\"\n"
    "#include \"../../shared/source/structs.h\"\n\n"
  );
  
  if (ini.export_model) {
    export_model_file(file, &model);
  }
  
  // textures
  
  if (ini.export_textures && !ini.separate_texture_file) {
    fprintf(file, "\n\n");
    export_texture_file(file, &textures, &model);
  }
  
  fclose(file);
}

void make_file_c_tex(char *export_path) {
  FILE *file;
  char path[MAX_PATH];
  strncpy(path, export_path, MAX_PATH - 1);
  
  PathRemoveFileSpec(path);
  PathAppend(path, "textures.c");
  
  file = fopen(path, "w");
  
  if (file == NULL) {
    printf("can't make the texture file\n");
    return;
  }
  
  fprintf(file, 
    "#include \"../../shared/source/defines.h\"\n"
    "#include \"../../shared/source/structs.h\"\n\n"
  );
  
  export_texture_file(file, &textures, &model);
  
  fclose(file);
}

void export_model_file(FILE *file, model_t *model) {
  fprintf(file, "static const u8 num_objects = %d;\n", model->num_objects);
  fprintf(file, "static const u8 flags = %d; // has_grid, has_textures\n", (model->has_grid << 1) | model->has_textures);
  
  for (int i = 0; i < model->num_objects; i++) {
    int obj_id = model->objects_id.data[i];
    export_object_file(obj_id, file, &model->objects.data[obj_id]);
  }
  
  fprintf(file, "static const object_t objects[] = {\n");
  
  for (int i = 0; i < model->num_objects; i++) {
    fprintf(file, "object_%d", i);
    
    if (i < model->num_objects - 1) {
      fprintf(file, ", ");
    }
  }
  
  fprintf(file, "};\n\n");
  
  fprintf(file,
    "const model_t model_%d = {\n", ini.model_id);
  fprintf(file,
    "  .num_objects = num_objects,\n"
    "  .flags = flags,\n"
    "  .objects = objects\n"
    "}\n"
  );
}

void export_object_file(int obj_id, FILE *file, object_t *object) {
  fprintf(file, "static const u16 num_vertices_%d = %d;\n", obj_id, object->num_vertices);
  fprintf(file, "static const u16 num_txcoords_%d = %d;\n", obj_id, object->num_txcoords);
  fprintf(file, "static const u16 num_faces_%d = %d;\n", obj_id, object->num_faces);
  fprintf(file, "static const u8 num_sprites_%d = %d;\n", obj_id, object->num_sprites);
  fprintf(file, "static const u8 num_sprite_vertices_%d = %d;\n", obj_id, object->num_sprites);
  fprintf(file, "static const u8 flags_%d = %d; // has_grid, has_textures\n", obj_id, (object->has_grid << 1) | object->has_textures);
  
  fprintf(file, "static const vec3_s16_t origin_%d = {%d, %d, %d};\n", obj_id, (int)object->origin.x, (int)object->origin.y, (int)object->origin.z);
  fprintf(file, "static const size3_u16_t size_%d = {%d, %d, %d};\n", obj_id, (int)object->size.w, (int)object->size.d, (int)object->size.h);
  fprintf(file, "\n");
  
  fprintf(file, "static const vec3_s16_t vertices_%d[] = {\n", obj_id);
  
  for (int i = 0; i < object->num_vertices; i++) {
    fprintf(file, "{%d,%d,%d}", (int)object->vertices.data[i].x, (int)object->vertices.data[i].y, (int)object->vertices.data[i].z);
    
    if (i < object->num_vertices - 1) {
      fprintf(file, ", ");
    }
  }
  
  fprintf(file, "};\n\n");
  
  if (object->has_textures) {
    fprintf(file, "static const vec2_tx_u16_t txcoords_%d[] = {\n", obj_id);
    
    for (int i = 0; i < object->num_txcoords; i++) {
      fprintf(file, "{%d,%d}", (int)object->txcoords.data[i].u, (int)object->txcoords.data[i].v);
      
      if (i < object->num_txcoords - 1) {
        fprintf(file, ", ");
      }
    }
    
    fprintf(file, "};\n\n");
  }
  
  fprintf(file, "static const vec3_s16_t sprite_vertices_%d[] = {\n", obj_id);
  
  for (int i = 0; i < object->num_sprite_vertices; i++) {
    fprintf(file, "{%d,%d,%d}", (int)object->sprite_vertices.data[i].x, (int)object->sprite_vertices.data[i].y, (int)object->sprite_vertices.data[i].z);
    
    if (i < object->num_sprite_vertices - 1) {
      fprintf(file, ", ");
    }
  }
  
  fprintf(file, "// num_vertices (u8), material (u8), type (u8), normal (3 * s8), angle (u8)\n");
  fprintf(file, "static const ALIGN_8 face_group_t face_group_%d[] = {\n", obj_id);
  
  for (int i = 0; i < object->num_faces; i++) {
    fprintf(file, "{%d,", object->faces.data[i].num_vertices);
    
    if (object->faces.data[i].type & TEXTURED) {
      fprintf(file, "%d,", object->faces.data[i].texture_id);
    } else {
      fprintf(file, "%d,", object->faces.data[i].material_id);
    }
    
    fprintf(file, "%d,", object->faces.data[i].type);
    fprintf(file, "{%d,%d,%d},", (int)object->faces.data[i].normal.x, (int)object->faces.data[i].normal.y, (int)object->faces.data[i].normal.z);
    fprintf(file, "%d,", object->faces.data[i].angle);
    fprintf(file, "0");
    
    if (i < object->num_faces - 1) {
      fprintf(file, ", ");
    }
  }
  
  fprintf(file, "};\n\n");
  
  fprintf(file, "triangle:\n");
  fprintf(file, "vt_id (3 * u16), tx_vt_id (3 * u16)\n");
  fprintf(file, "quad:\n");
  fprintf(file, "vt_id (4 * u16), tx_vt_id (4 * u16)\n");
  fprintf(file, "sprite:\n");
  fprintf(file, "vt_id (u16), sp_vt_id (4 * u8), tx_vt_id (4 * u16)\n\n");
  
  fprintf(file, "static const ALIGN_16 face_data_t face_data_%d[] = {\n", obj_id);
  
  for (int i = 0; i < object->num_faces; i++) {
    face_t *face = &object->faces.data[i];
    
    if (face->num_vertices == 3) { // triangle
      fprintf(file, "%d,", face->vt_index[0]);
      fprintf(file, "%d,", face->vt_index[1]);
      fprintf(file, "%d,", face->vt_index[2]);
      fprintf(file, "0,");
      
      if (face->type & TEXTURED) {
        fprintf(file, "%d,", face->tx_vt_index[0]);
        fprintf(file, "%d,", face->tx_vt_index[1]);
        fprintf(file, "%d,", face->tx_vt_index[2]);
        fprintf(file, "0");
      } else {
        fprintf(file, "0,0,0,0");
      }
    }
    else if (face->num_vertices == 4) { // quad
      fprintf(file, "%d,", face->vt_index[0]);
      fprintf(file, "%d,", face->vt_index[1]);
      fprintf(file, "%d,", face->vt_index[2]);
      fprintf(file, "%d", face->vt_index[3]);
      
      if (face->type & TEXTURED) {
        fprintf(file, "%d,", face->tx_vt_index[0]);
        fprintf(file, "%d,", face->tx_vt_index[1]);
        fprintf(file, "%d,", face->tx_vt_index[2]);
        fprintf(file, "%d", face->tx_vt_index[3]);
      } else {
        fprintf(file, "0,0,0,0");
      }
    }
    else if (face->type & SPRITE) { // sprite
      fprintf(file, "%d,", face->vt_index[0]); // u16
      fprintf(file, "%d,", face->vt_index[1]); // u8
      fprintf(file, "%d,", face->vt_index[2]); // u8
      fprintf(file, "%d,", face->vt_index[3]); // u8
      fprintf(file, "%d,", face->vt_index[4]); // u8
      fprintf(file, "%d,", face->tx_vt_index[0]); // u16
      fprintf(file, "%d,", face->tx_vt_index[1]); // u16
      fprintf(file, "%d,", face->tx_vt_index[2]); // u16
      fprintf(file, "%d", face->tx_vt_index[3]); // u16
      fprintf(file, "0,0");
    }
    
    if (i < object->num_faces - 1) {
      fprintf(file, ", ");
    }
  }
  
  fprintf(file, "};\n\n");
  
  if (object->has_grid) {
    fprintf(file, "// grid data\n\n");
    
    fprintf(file, "static const ALIGN_2 grid_pnt_t grid_pnt_%d[] = {\n", obj_id);
    
    for (int i = 0; i < object->grid_ln.num_tiles; i++) {
      fprintf(file, "{%d", object->grid_ln.pl_pnt[i]);
      fprintf(file, ",%d}", object->grid_ln.vt_pnt[i]);
      
      if (i < object->grid_ln.num_tiles - 1) {
        fprintf(file, ",");
      }
    }
    
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const s16 pl_data_%d[] = {\n", obj_id);
    
    for (int i = 0; i < object->grid_ln.pl_data.size; i++) {
      fprintf(file, "%d", object->grid_ln.pl_data.data[i]);
      
      if (i < object->grid_ln.pl_data.size - 1) {
        fprintf(file, ",");
      }
    }
    
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const s16 vt_data_%d[] = {\n", obj_id);
    
    for (int i = 0; i < object->grid_ln.vt_data.size; i++) {
      fprintf(file, "%d", object->grid_ln.vt_data.data[i]);
      
      if (i < object->grid_ln.vt_data.size - 1) {
        fprintf(file, ",");
      }
    }
    
    fprintf(file, "};\n\n");
    
    fprintf(file,
      "static const grid_t grid_%1$d = {\n"
      "  .grid_pnt = grid_pnt_%1$d,\n"
      "  .pl_data = pl_data_%1$d,\n"
      "  .vt_data = vt_data_%1$d,\n\n", obj_id
    );
    
    fprintf(file, "  .grid_size_i = {%d, %d, %d},\n", object->grid_ln.size_i.w, object->grid_ln.size_i.d, object->grid_ln.size_i.h);
    fprintf(file, "  .num_tiles = %d,\n", object->grid_ln.num_tiles);
    fprintf(file, "  .pl_data_size = %d,\n", object->grid_ln.pl_data.size);
    fprintf(file, "  .vt_data_size = %d,\n", object->grid_ln.vt_data.size);
    fprintf(file, "  .tile_size = %d,\n", object->grid_ln.tile_size_i << ini.fp_size);
    fprintf(file, "  .tile_size_bits = %d,\n", object->grid_ln.tile_size_bits);
    fprintf(file, "};\n\n");
  }
  
  fprintf(file,
    "const object_t object_%d = {\n", obj_id);
  fprintf(file,
    "  .num_vertices = num_vertices_%1$d,\n"
    "  .num_txcoords = num_txcoords_%1$d,\n"
    "  .num_vertices = num_sprite_vertices_%1$d,\n"
    "  .num_faces = num_faces_%1$d,\n"
    "  .num_sprites = num_sprites_%1$d,\n"
    "  .origin = origin_%1$d,\n"
    "  .size = size_%1$d,\n\n"
    
    "  .vertices = vertices_%1$d,\n", obj_id
  );
  
  if (object->has_textures) {
    fprintf(file,
      "  .txcoords = txcoords_%d,\n", obj_id
    );
  }
  
  fprintf(file,
    "  .sprite_vertices = sprite_vertices_%1$d,\n"
    "  .face_data = face_data_%1$d,\n"
    "  .face_group = face_group_%1$d,\n", obj_id
  );
  
  if (object->has_grid) {
    fprintf(file,
      "  .grid = grid_%d,\n", obj_id
    );
  }
  
  fprintf(file,
    "};"
  );
}

void export_texture_file(FILE *file, textures_t *textures, model_t *model) {
  fprintf(file, "// textures\n\n");
  fprintf(file, "static const u8 num_textures = %d;\n", textures->num_textures);
  fprintf(file, "static const u8 num_animations = %d;\n", textures->num_animations);
  fprintf(file, "static const u8 pal_size = %d;\n", textures->pal_size);
  // fprintf(file, "static const u8 pal_num_colors = %d;\n", textures->pal_num_colors);
  fprintf(file, "static const u8 pal_size_tx = %d;\n", textures->pal_size_tx);
  // fprintf(file, "static const u8 pal_tx_num_colors = %d;\n", textures->pal_tx_num_colors);
  fprintf(file, "static const u8 lightmap_level_bits = %d;\n", textures->lightmap_level_bits);
  fprintf(file, "static const u32 texture_data_total_size = %d;\n\n", textures->texture_data_total_size);
  
  #if EXPORT_AVERAGE_PAL
    fprintf(file, "static const u16 cr_palette_idx[] = {\n");
    
    if (!textures->lightmap_levels) {
      for (int i = 0; i < textures->num_textures; i++) {
        fprintf(file, "0x%04x", textures->cr_palette.data[i]);
        
        if (i < textures->num_textures - 1) {
          fprintf(file, ",");
          
          if ((i & 15) == 15) {
            fprintf(file, "\n");
          }
        }
      }
      
      fprintf(file, "};\n\n");
    } else {
      for (int i = 0; i < textures->num_textures; i++) {
        for (int j = 0; j < textures->lightmap_levels; j++) {
          fprintf(file, "0x%04x", textures->cr_palette_idx.data[i * textures->lightmap_levels + j]);
          
          if (j < textures->lightmap_levels - 1) {
            fprintf(file, ",");
          }
        }
        
        if (i < textures->num_textures - 1) {
          fprintf(file, ",\n");
        }
      }
      
      fprintf(file, "};\n\n");
    }
  #endif
  
  if (model->has_textures) {
    fprintf(file, "static const u16 cr_palette_tx_idx[] = {\n");
    
    if (!textures->lightmap_levels) {
      for (int i = 0; i < textures->pal_size_tx; i++) {
        fprintf(file, "0x%04x", textures->cr_palette_tx.data[i]);
        
        if (i < textures->pal_size_tx - 1) {
          fprintf(file, ",");
          
          if ((i & 15) == 15) {
            fprintf(file, "\n");
          }
        }
      }
      
      fprintf(file, "};\n\n");
    } else {
      for (int i = 0; i < textures->pal_size_tx; i++) {
        fprintf(file, "0x%04x", textures->cr_palette_tx_idx.data[i]);
        
        if (i < textures->pal_size_tx - 1) {
          fprintf(file, ",");
          
          if ((i & (textures->lightmap_levels - 1)) == textures->lightmap_levels - 1) {
            fprintf(file, "\n");
          }
        }
      }
      
      fprintf(file, "};\n\n");
    }
    
    fprintf(file, "// size_padded_wh (u32), width_bits (u8), tx_animation_id (s8), tx_index (u32)\n");
    fprintf(file, "static const ALIGN_16 tx_group_t tx_group[] = {\n");
    
    for (int i = 0; i < textures->num_textures; i++) {
      fprintf(file, "{%d,", (textures->tx_group.data[i].size_padded.h << 16) | textures->tx_group.data[i].size_padded.w);
      fprintf(file, "%d,", textures->tx_group.data[i].width_bits);
      
      if (textures->num_animations) {
        fprintf(file, "%d,", textures->tx_group.data[i].animation_id);
      } else {
        fprintf(file, "0,");
      }
      
      fprintf(file, "%d,", textures->tx_group.data[i].tx_index);
      fprintf(file, "0,0,0,0,0,0}");
      
      if (i < textures->num_textures - 1){
        fprintf(file, ", ");
      }
    }
    
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u16 texture_data[] = {\n");
    
    for (int i = 0; i < textures->num_textures; i++) {
      int tx_index = textures->tx_group.data[i].tx_index;
      
      for (int j = 0; j < textures->tx_group.data[i].total_size; j++) {
        fprintf(file, "%d", textures->texture_data.data[tx_index + j]);
        
        if (j < textures->tx_group.data[i].total_size - 1 || i < textures->num_textures - 1){
          fprintf(file, ",");
          
          if ((j % textures->tx_group.data[i].size.w) == textures->tx_group.data[i].size.w - 1) {
            fprintf(file, "\n");
          }
        }
      }
      
      if (i < textures->num_textures - 1){
        fprintf(file, "\n");
      }
    }
    fprintf(file, "};\n\n");
  }
  
  fprintf(file,
    "const textures_t textures_%d = {\n", ini.model_id
  );
  
  fprintf(file,
    "  .num_textures = num_textures,\n"
    "  .num_animations = num_animations,\n"
    "  .pal_size = pal_size,\n"
    "  .pal_num_colors = pal_num_colors,\n"
    "  .pal_size_tx = pal_size_tx,\n"
    "  .pal_tx_num_colors = pal_tx_num_colors,\n"
    "  .lightmap_level_bits = lightmap_level_bits,\n"
    "  .texture_data_total_size = texture_data_total_size,\n\n"
    
    #if EXPORT_AVERAGE_PAL
      "  .cr_palette_idx = cr_palette_idx,\n"
    #endif
  );
  
  if (model->has_textures) {
    fprintf(file,
      "  .cr_palette_tx_idx = cr_palette_tx_idx,\n"
      "  .tx_group = tx_group,\n"
      "  .texture_data = texture_data,\n"
    );
  }
  
  fprintf(file, "};");
}

void make_file_mdl(char *export_path) {
  FILE *file;
  char path[MAX_PATH];
  strncpy(path, export_path, MAX_PATH - 1);
  
  PathRenameExtension(path, ".mdl");
  
  file = fopen(path, "w");
  
  if (file == NULL) {
    printf("can't make the mdl file");
    return;
  }
  
  fprintf(file, "num_objects = %d;\n", model.num_objects);
  fprintf(file, "flags = %d; // has_grid, has_textures\n", (model.has_grid << 1) | model.has_textures);
  
  for (int i = 0; i < model.num_objects; i++) {
    int obj_id = model.objects_id.data[i];
    make_file_mdl_object(file, &model.objects.data[obj_id]);
  }
  
  if (ini.export_textures) {
    fprintf(file, "\n\n");
    make_file_mdl_textures(file, &textures, &model);
  }
  
  fclose(file);
}

void make_file_mdl_object(FILE *file, object_t *object) {
  fprintf(file, "o\n");
  fprintf(file, "num_vertices = %d;\n", object->num_vertices);
  fprintf(file, "num_txcoords = %d;\n", object->num_txcoords);
  fprintf(file, "num_faces = %d;\n", object->num_faces);
  fprintf(file, "num_sprites = %d;\n", object->num_sprites);
  fprintf(file, "num_sprite_vertices = %d;\n", object->num_sprite_vertices);
  fprintf(file, "flags = %d; // has_grid, has_textures\n", (object->has_grid << 1) | object->has_textures);
  
  fprintf(file, "origin = %.6f, %.6f, %.6f;\n", object->origin.x, object->origin.y, object->origin.z);
  fprintf(file, "size = %.6f, %.6f, %.6f;\n\n", object->size.w, object->size.d, object->size.h);
  
  fprintf(file, "vertices =\n");
  
  for (int i = 0; i < object->num_vertices; i++) {
    fprintf(file, "%.6f,%.6f,%.6f", object->vertices.data[i].x, object->vertices.data[i].y, object->vertices.data[i].z);
    
    if (i < object->num_vertices - 1) {
      fprintf(file, ", ");
    }
  }
  
  fprintf(file, ";\n\n");
  
  if (object->has_textures) {
    fprintf(file, "txcoords =\n");
    
    for (int i = 0; i < object->num_txcoords; i++) {
      fprintf(file, "%d,%d", (int)object->txcoords.data[i].u, (int)object->txcoords.data[i].v);
      
      if (i < object->num_txcoords - 1) {
        fprintf(file, ", ");
      }
    }
    
    fprintf(file, ";\n\n");
  }
  
  fprintf(file, "sprite_vertices =\n");
  
  for (int i = 0; i < object->num_sprite_vertices; i++) {
    fprintf(file, "%.6f,%.6f,%.6f", object->sprite_vertices.data[i].x, object->sprite_vertices.data[i].y, object->sprite_vertices.data[i].z);
    
    if (i < object->num_sprite_vertices - 1) {
      fprintf(file, ", ");
    }
  }
  
  fprintf(file, "// num_vertices (u8), material (u8), type (u8), normal (3 * s8), angle (u8)\n");
  fprintf(file, "face_group =\n");
  
  for (int i = 0; i < object->num_faces; i++) {
    fprintf(file, "%d,", object->faces.data[i].num_vertices);
    
    if (object->faces.data[i].type & TEXTURED) {
      fprintf(file, "%d,", object->faces.data[i].texture_id);
    } else {
      fprintf(file, "%d,", object->faces.data[i].material_id);
    }
    
    fprintf(file, "%d,", object->faces.data[i].type);
    fprintf(file, "%d,%d,%d,", (int)object->faces.data[i].normal.x, (int)object->faces.data[i].normal.y, (int)object->faces.data[i].normal.z);
    fprintf(file, "%d,", object->faces.data[i].angle);
    fprintf(file, "0");
    
    if (i < object->num_faces - 1) {
      fprintf(file, ", ");
    }
  }
  
  fprintf(file, ";\n\n");
  
  fprintf(file, "triangle:\n");
  fprintf(file, "vt_id (3 * u16), tx_vt_id (3 * u16)\n");
  fprintf(file, "quad:\n");
  fprintf(file, "vt_id (4 * u16), tx_vt_id (4 * u16)\n");
  fprintf(file, "sprite:\n");
  fprintf(file, "vt_id (u16), sp_vt_id (4 * u8), tx_vt_id (4 * u16)\n\n");
  
  fprintf(file, "face_data =\n");
  
  for (int i = 0; i < object->num_faces; i++) {
    face_t *face = &object->faces.data[i];
    
    if (face->num_vertices == 3) { // triangle
      fprintf(file, "%d,", face->vt_index[0]);
      fprintf(file, "%d,", face->vt_index[1]);
      fprintf(file, "%d,", face->vt_index[2]);
      fprintf(file, "0,");
      
      if (face->type & TEXTURED) {
        fprintf(file, "%d,", face->tx_vt_index[0]);
        fprintf(file, "%d,", face->tx_vt_index[1]);
        fprintf(file, "%d,", face->tx_vt_index[2]);
        fprintf(file, "0");
      } else {
        fprintf(file, "0,0,0,0");
      }
    }
    else if (face->num_vertices == 4) { // quad
      fprintf(file, "%d,", face->vt_index[0]);
      fprintf(file, "%d,", face->vt_index[1]);
      fprintf(file, "%d,", face->vt_index[2]);
      fprintf(file, "%d", face->vt_index[3]);
      
      if (face->type & TEXTURED) {
        fprintf(file, "%d,", face->tx_vt_index[0]);
        fprintf(file, "%d,", face->tx_vt_index[1]);
        fprintf(file, "%d,", face->tx_vt_index[2]);
        fprintf(file, "%d", face->tx_vt_index[3]);
      } else {
        fprintf(file, "0,0,0,0");
      }
    }
    else if (face->type & SPRITE) { // sprite
      fprintf(file, "%d,", face->vt_index[0]); // u16
      fprintf(file, "%d,", face->vt_index[1]); // u8
      fprintf(file, "%d,", face->vt_index[2]); // u8
      fprintf(file, "%d,", face->vt_index[3]); // u8
      fprintf(file, "%d,", face->vt_index[4]); // u8
      fprintf(file, "%d,", face->tx_vt_index[0]); // u16
      fprintf(file, "%d,", face->tx_vt_index[1]); // u16
      fprintf(file, "%d,", face->tx_vt_index[2]); // u16
      fprintf(file, "%d", face->tx_vt_index[3]); // u16
      fprintf(file, "0,0");
    }
    
    if (i < object->num_faces - 1) {
      fprintf(file, ", ");
    }
  }
  
  fprintf(file, ";\n\n");
}

void make_file_mdl_textures(FILE *file, textures_t *textures, model_t *model) {
  fprintf(file, "// textures\n\n");
  fprintf(file, "num_textures = %d;\n", textures->num_textures);
  fprintf(file, "num_animations = %d;\n", textures->num_animations);
  fprintf(file, "pal_size = %d;\n", textures->pal_size);
  // fprintf(file, "pal_num_colors = %d;\n", textures->pal_num_colors);
  fprintf(file, "pal_size_tx = %d;\n", textures->pal_size_tx);
  // fprintf(file, "pal_tx_num_colors = %d;\n", textures->pal_tx_num_colors);
  fprintf(file, "lightmap_level_bits = %d;\n", textures->lightmap_level_bits);
  fprintf(file, "texture_data_total_size = %d;\n\n", textures->texture_data_total_size);
  
  #if EXPORT_AVERAGE_PAL
    fprintf(file, "cr_palette_idx =\n");
    
    if (!textures->lightmap_levels) {
      for (int i = 0; i < textures->num_textures; i++) {
        fprintf(file, "0x%04x", textures->cr_palette.data[i]);
        
        if (i < textures->num_textures - 1) {
          fprintf(file, ",");
          
          if ((i & 15) == 15) {
            fprintf(file, "\n");
          }
        }
      }
      
      fprintf(file, ";\n\n");
    } else {
      for (int i = 0; i < textures->num_textures; i++) {
        for (int j = 0; j < textures->lightmap_levels; j++) {
          fprintf(file, "0x%04x", textures->cr_palette_idx.data[i * textures->lightmap_levels + j]);
          
          if (j < textures->lightmap_levels - 1) {
            fprintf(file, ",");
          }
        }
        
        if (i < textures->num_textures - 1) {
          fprintf(file, ",\n");
        }
      }
      
      fprintf(file, ";\n\n");
    }
  #endif
  
  if (model->has_textures) {
    fprintf(file, "cr_palette_tx_idx =\n");
    
    if (!textures->lightmap_levels) {
      for (int i = 0; i < textures->pal_size_tx; i++) {
        fprintf(file, "0x%04x", textures->cr_palette_tx.data[i]);
        
        if (i < textures->pal_size_tx - 1) {
          fprintf(file, ",");
          
          if ((i & 15) == 15) {
            fprintf(file, "\n");
          }
        }
      }
      
      fprintf(file, ";\n\n");
    } else {
      for (int i = 0; i < textures->pal_size_tx; i++) {
        fprintf(file, "0x%04x", textures->cr_palette_tx_idx.data[i]);
        
        if (i < textures->pal_size_tx - 1) {
          fprintf(file, ",");
          
          if ((i & (textures->lightmap_levels - 1)) == textures->lightmap_levels - 1) {
            fprintf(file, "\n");
          }
        }
      }
      
      fprintf(file, ";\n\n");
    }
    
    fprintf(file, "// size_padded_wh (u32), width_bits (u8), tx_animation_id (s8), tx_index (u32)\n");
    fprintf(file, "tx_group =\n");
    
    for (int i = 0; i < textures->num_textures; i++) {
      fprintf(file, "%d,", (textures->tx_group.data[i].size_padded.h << 16) | textures->tx_group.data[i].size_padded.w);
      fprintf(file, "%d,", textures->tx_group.data[i].width_bits);
      
      if (textures->num_animations) {
        fprintf(file, "%d,", textures->tx_group.data[i].animation_id);
      } else {
        fprintf(file, "0,");
      }
      
      fprintf(file, "%d", textures->tx_group.data[i].tx_index);
      
      if (i < textures->num_textures - 1){
        fprintf(file, ", ");
      }
    }
    
    fprintf(file, ";\n\n");
    
    fprintf(file, "texture_data =\n");
    
    for (int i = 0; i < textures->num_textures; i++) {
      int tx_index = textures->tx_group.data[i].tx_index;
      
      for (int j = 0; j < textures->tx_group.data[i].total_size; j++) {
        fprintf(file, "%d", textures->texture_data.data[tx_index + j]);
        
        if (j < textures->tx_group.data[i].total_size - 1 || i < textures->num_textures - 1){
          fprintf(file, ",");
          
          if ((j % textures->tx_group.data[i].size.w) == textures->tx_group.data[i].size.w - 1) {
            fprintf(file, "\n");
          }
        }
      }
      
      if (i < textures->num_textures - 1){
        fprintf(file, "\n");
      }
    }
    fprintf(file, ";\n\n");
  }
}

void make_file_obj(char *export_path) {
  FILE *file;
  char path[MAX_PATH];
  strncpy(path, export_path, MAX_PATH - 1);
  // PathRenameExtension(path, ".obj");
  
  PathRenameExtension(path, ".mtl");
  
  file = fopen(path, "rb");
  
  if (file == NULL) {
    printf("mtl file not found");
    return;
  }
  
  // obtain file size
  fseek(file, 0, SEEK_END); // seek to end of file
  int size = ftell(file); // get current file pointer
  fseek(file, 0, SEEK_SET); // seek back to beginning of file
  
  // read file data
  char *data = malloc(size);
  size = fread(data, 1, size, file);
  fclose(file);
  
  PathRemoveFileSpec(path);
  PathAppend(path, "model.mtl");
  
  file = fopen(path, "wb");
  if (file == NULL) {
    printf("can't make the mtl file");
    return;
  }
  
  fwrite(data, 1, size, file);
  fclose(file);
  free(data);
  
  PathRemoveFileSpec(path);
  PathAppend(path, "model.obj");
  
  file = fopen(path, "w");
  if (file == NULL) {
    printf("can't make the obj file");
    return;
  }
  
  fprintf(file, "#custom OBJ exporter\n");
  
  fprintf(file, "mtllib model.mtl\n");
  
  int prev_mtl = -1;
  
  for (int i = 0; i < model.num_objects; i++) {
    object_t *object = &model.objects.data[model.objects_id.data[i]];
    
    fprintf(file, "o Plane.%d\n", i);
    
    for (int j = 0; j < object->num_vertices; j++) {
      fprintf(file, "v %.6f %.6f %.6f\n", object->vertices.data[j].x, object->vertices.data[j].y, object->vertices.data[j].z);
    }
    
    for (int j = 0; j < object->num_txcoords; j++) {
      fprintf(file, "vt %.6f %.6f\n", object->txcoords.data[j].u, object->txcoords.data[j].v);
    }
    
    for (int j = 0; j < object->num_faces; j++) {
      if (object->faces.data[i].material_id != prev_mtl) {
        fprintf(file, "usemtl %s\n", material_list.data[object->faces.data[i].material_id]);
        fprintf(file, "s off\n");
        prev_mtl = object->faces.data[i].material_id;
      }
      
      fprintf(file, "f");
      
      for (int j = 0; j < object->faces.data[i].num_vertices; j++) {
        fprintf(file, " %d", object->faces.data[i].vt_index[j] + 1);
        
        if (object->faces.data[i].has_texture) {
          fprintf(file, "/%d", object->faces.data[i].tx_vt_index[j] + 1);
        }
      }
      
      fprintf(file, "\n");
    }
  }
  
  for (int i = 0; i < model.num_materials; i++) {
    free(material_list.data[i]);
  }
  
  free_list(&material_list);
  
  fclose(file);
}