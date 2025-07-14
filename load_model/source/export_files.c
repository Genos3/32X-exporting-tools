#include <shlwapi.h>
#include "common.h"

void make_file_c_mdl(char *export_path);
void make_file_c_tex(char *export_path);
void export_model_file(FILE *file);
void export_texture_file(FILE *file);
void make_file_mdl(char *export_path);
void make_file_obj(char *export_path);

void make_file(int argc, char *file_path, char *export_path) {
  if (argc < 3) {
    export_path = strdup(file_path);
  } else {
    // try to create the directory if it doesn't exist
    if (!PathFileExists(export_path) && !CreateDirectory(export_path, NULL)) {
      printf("can't create the export directory");
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
  } else
  if (ini.file_type_export == MDL_FILE) {
    make_file_mdl(export_path);
  } else
  if (ini.file_type_export == OBJ_FILE) {
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
    printf("can't make the model file");
    return;
  }
  
  fprintf(file,
    "#include \"../../shared/source/defines.h\"\n"
    "#include \"../../shared/source/structs.h\"\n\n"
  );
  
  if (ini.export_model) {
    export_model_file(file);
  }
  
  // textures
  
  if (ini.export_textures && !ini.separate_texture_file) {
    fprintf(file, "\n\n\n");
    
    export_texture_file(file);
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
    printf("can't make the textures file");
    return;
  }
  
  fprintf(file, 
    "#include \"../../shared/source/defines.h\"\n"
    "#include \"../../shared/source/structs.h\"\n\n"
  );
  
  export_texture_file(file);
  
  fclose(file);
}

void export_model_file(FILE *file) {
  fprintf(file, "static const u16 num_vertices = %d;\n", model.num_vertices);
  fprintf(file, "static const u16 num_faces = %d;\n", model.num_faces);
  // fprintf(file, "static const u32 faces_size = %d;\n", model.faces_size);
  fprintf(file, "static const u16 num_txcoords = %d;\n", model.num_txcoords);
  fprintf(file, "static const u16 num_tx_faces = %d;\n", model.num_tx_faces);
  // fprintf(file, "static const u32 tx_faces_size = %d;\n", model.tx_faces_size);
  fprintf(file, "static const u16 num_objects = %d;\n", model.num_objects);
  fprintf(file, "static const u8 num_materials = %d;\n", model.num_materials);
  fprintf(file, "static const u8 num_sprites = %d;\n", model.num_sprites);
  fprintf(file, "static const u8 num_sprite_vertices = %d;\n", model.num_sprite_vertices);
  fprintf(file, "static const u8 has_normals = 1;\n");
  fprintf(file, "static const u8 has_grid = %d;\n", ini.make_grid);
  fprintf(file, "static const u8 has_textures = %d;\n", model.has_textures);
  
  if (ini.make_fp) {
    fprintf(file, "static const vec3_s16_t origin = {%d, %d, %d};\n", (int)model.origin.x, (int)model.origin.y, (int)model.origin.z);
    fprintf(file, "static const size3_u16_t size = {%d, %d, %d};\n", (int)model.size.w, (int)model.size.d, (int)model.size.h);
  } else {
    fprintf(file, "static const vec3_t origin = {%.6f, %.6f, %.6f};\n", model.origin.x, model.origin.y, model.origin.z);
    fprintf(file, "static const size3_t size = {%.6f, %.6f, %.6f};\n", model.size.w, model.size.d, model.size.h);
  }
  fprintf(file, "\n");
  
  if (ini.make_fp) {
    fprintf(file, "static const vec3_s16_t vertices[] = {\n");
  } else {
    fprintf(file, "static const vec3_t vertices[] = {\n");
  }
  
  for (int i = 0; i < model.num_vertices; i++) {
    if (ini.make_fp) {
      fprintf(file, "{%d,%d,%d}", (int)model.vertices.data[i].x, (int)model.vertices.data[i].y, (int)model.vertices.data[i].z);
    } else {
      fprintf(file, "{%.6f,%.6f,%.6f}", model.vertices.data[i].x, model.vertices.data[i].y, model.vertices.data[i].z);
    }
    
    if (i < model.num_vertices - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, "};\n\n");
  
  fprintf(file, "static const u16 faces[] = {\n");
  
  for (int i = 0; i < model.num_faces; i++) {
    if (!model.num_sprites || !(model.face_types.data[i] & SPRITE)) {
      for (int j = 0; j < model.face_num_vertices.data[i]; j++) {
        fprintf(file, "%d", model.faces.data[model.face_index.data[i] + j]);
        
        if (j < model.face_num_vertices.data[i] - 1) {
          fprintf(file, ",");
        } else
        if (i < model.num_faces - 1) {
          fprintf(file, ", ");
        }
      }
    } else {
      fprintf(file, "%d", model.faces.data[model.face_index.data[i]]);
      
      if (i < model.num_faces - 1) {
        fprintf(file, ", ");
      }
    }
  }
  fprintf(file, "};\n\n");
  
  if (model.has_textures) {
    if (ini.make_fp) {
      fprintf(file, "static const vec2_tx_u16_t txcoords[] = {\n");
    } else {
      fprintf(file, "static const vec2_tx_t txcoords[] = {\n");
    }
    
    for (int i = 0; i < model.num_txcoords; i++) {
      if (ini.make_fp) {
        fprintf(file, "{%d,%d}", (int)model.txcoords.data[i].u, (int)model.txcoords.data[i].v);
      } else {
        fprintf(file, "{%.6f,%.6f}", model.txcoords.data[i].u, model.txcoords.data[i].v);
      }
      
      if (i < model.num_txcoords - 1) {
        fprintf(file, ", ");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u16 tx_faces[] = {\n");
    
    for (int i = 0; i < model.num_tx_faces; i++) {
      for (int j = 0; j < model.face_num_vertices.data[i]; j++) {
        fprintf(file, "%d", model.tx_faces.data[model.tx_face_index.data[i] + j]);
        
        if (j < model.face_num_vertices.data[i] - 1) {
          fprintf(file, ",");
        } else
        if (i < model.num_tx_faces - 1) {
          fprintf(file, ", ");
        }
      }
    }
    fprintf(file, "};\n\n");
  }
  
  if (ini.make_fp) {
    fprintf(file, "static const vec3_s16_t normals[] = {\n");
  } else {
    fprintf(file, "static const vec3_t normals[] = {\n");
  }
  
  for (int i = 0; i < model.num_faces; i++) {
    if (ini.make_fp) {
      fprintf(file, "{%d,%d,%d}", (int)model.normals.data[i].x, (int)model.normals.data[i].y, (int)model.normals.data[i].z);
    } else {
      fprintf(file, "{%.6f,%.6f,%.6f}", model.normals.data[i].x, model.normals.data[i].y, model.normals.data[i].z);
    }
    
    if (i < model.num_faces - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, "};\n\n");
  
  fprintf(file, "// face_num_vertices, face_materials, face_types, face_index, tx_face_index, sprite_face_index\n");
  fprintf(file, "static const face_group_t face_group[] = {\n");
  
  for (int i = 0; i < model.num_faces; i++) {
    fprintf(file, "{%d", model.face_num_vertices.data[i]);
    fprintf(file, ",%d", model.face_materials.data[i]);
    fprintf(file, ",%d", model.face_types.data[i]);
    fprintf(file, ",%d", model.face_index.data[i]);
    if (model.has_textures) {
      fprintf(file, ",%d", model.tx_face_index.data[i]);
    } else {
      fprintf(file, ",0");
    }
    if (model.num_sprites) {
      fprintf(file, ",%d}", model.sprite_face_index.data[i]);
    } else {
      fprintf(file, ",0}");
    }
    
    if (i < model.num_faces - 1) {
      fprintf(file, ",");
    }
  }
  fprintf(file, "};\n\n");
  
  if (model.num_sprites) {
    if (ini.make_fp) {
      fprintf(file, "static const vec3_s16_t sprite_vertices[] = {\n");
    } else {
      fprintf(file, "static const vec3_t sprite_vertices[] = {\n");
    }
    
    for (int i = 0; i < model.num_sprite_vertices; i++) {
      if (ini.make_fp) {
        fprintf(file, "{%d,%d,%d}", (int)model.sprite_vertices.data[i].x, (int)model.sprite_vertices.data[i].y, (int)model.sprite_vertices.data[i].z);
      } else {
        fprintf(file, "{%.6f,%.6f,%.6f}", model.sprite_vertices.data[i].x, model.sprite_vertices.data[i].y, model.sprite_vertices.data[i].z);
      }
      
      if (i < model.num_sprite_vertices - 1) {
        fprintf(file, ", ");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u8 sprite_faces[] = {\n");
    
    for (int i = 0; i < model.num_sprites; i++) {
      for (int j = 0; j < 4; j++) {
        fprintf(file, "%d", model.sprite_faces.data[i * 4 + j]);
        
        if (j < 3) {
          fprintf(file, ",");
        } else
        if (i < model.num_sprites - 1) {
          fprintf(file, ", ");
        }
      }
    }
    fprintf(file, "};\n\n");
    
    /* fprintf(file, "static const s16 sprite_face_index[] = {\n");
    
    for (int i = 0; i < model.num_faces; i++) {
      fprintf(file, "%d", model.sprite_face_index.data[i]);
      
      if (i < model.num_faces - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n"); */
  }
  
  if (model.num_objects) {
    fprintf(file, "static const u16 object_face_index[] = {\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      fprintf(file, "%d", model.object_face_index.data[i]);
      
      if (i < model.num_objects - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u16 object_num_faces[] = {\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      fprintf(file, "%d", model.object_num_faces.data[i]);
      
      if (i < model.num_objects - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    /* fprintf(file, "static const u16 object_vt_list[] = {\n");
    
    for (int i = 0; i < model.object_vt_list.size; i++) {
      fprintf(file, "%d", model.object_vt_list.data[i]);
      
      if (i < model.object_vt_list.size - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u16 object_num_vertices[] = {\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      fprintf(file, "%d", model.object_num_vertices.data[i]);
      
      if (i < model.num_objects - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u16 object_vt_index[] = {\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      fprintf(file, "%d", model.object_vt_index.data[i]);
      
      if (i < model.num_objects - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const vec3_s16_t objects_origin[] = {\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      if (ini.make_fp) {
        fprintf(file, "{%d,%d,%d}", (int)model.objects_origin.data[i].x, (int)model.objects_origin.data[i].y, (int)model.objects_origin.data[i].z);
      } else {
        fprintf(file, "{%.6f,%.6f,%.6f}", model.objects_origin.data[i].x, model.objects_origin.data[i].y, model.objects_origin.data[i].z);
      }
      
      if (i < model.num_objects - 1) {
        fprintf(file, ", ");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const size3_t objects_size[] = {\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      if (ini.make_fp) {
        fprintf(file, "{%d,%d,%d}", (int)model.objects_size.data[i].w, (int)model.objects_size.data[i].d, (int)model.objects_size.data[i].h);
      } else {
        fprintf(file, "{%.6f,%.6f,%.6f}", model.objects_size.data[i].w, model.objects_size.data[i].d, model.objects_size.data[i].h);
      }
      
      if (i < model.num_objects - 1) {
        fprintf(file, ", ");
      }
    }
    fprintf(file, "};\n\n"); */
  }
  
  if (model.has_textures) {
    fprintf(file, "static const u8 mtl_textures[] = {\n");
    
    for (int i = 0; i < model.num_materials; i++) {
      fprintf(file, "%d", model.mtl_textures.data[i]);
      
      if (i < model.num_materials - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
  }
  
  if (model.has_grid) {
    fprintf(file, "// grid data\n\n");
    
    fprintf(file, "static const grid_pnt_t grid_pnt[] = {\n");
    
    for (int i = 0; i < grid_scn_ln.num_tiles; i++) {
      fprintf(file, "{%d", grid_scn_ln.pl_pnt[i]);
      fprintf(file, ",%d}", grid_scn_ln.vt_pnt[i]);
      
      if (i < grid_scn_ln.num_tiles - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    /* fprintf(file, "  static const s16 grid_pnt[] = {\n  ");
    
    for (int i = 0; i < grid_scn_ln.num_tiles; i++) {
      fprintf(file, "%d", grid_scn_ln.grid_pnt[i]);
      
      if (i < grid_scn_ln.num_tiles - 1) {
        fprintf(file, ",");
      }
    }
    
    fprintf(file, "},\n\n"); */
    
    fprintf(file, "static const s16 pl_data[] = {\n");
    
    for (int i = 0; i < grid_scn_ln.pl_data.size; i++) {
      fprintf(file, "%d", grid_scn_ln.pl_data.data[i]);
      
      if (i < grid_scn_ln.pl_data.size - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const s16 vt_data[] = {\n");
    
    for (int i = 0; i < grid_scn_ln.vt_data.size; i++) {
      fprintf(file, "%d", grid_scn_ln.vt_data.data[i]);
      
      if (i < grid_scn_ln.vt_data.size - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    /* fprintf(file, "  .grid_aabb = (const aabb_t[]) {\n  ");
    
    for (int i = 0; i < grid_scn_ln.grid_aabb.size; i++) {
      fprintf(file, "{%d, %d, %d, %d, %d, %d}", (int)grid_scn_ln.grid_aabb.data[i].min.x, (int)grid_scn_ln.grid_aabb.data[i].min.y, (int)grid_scn_ln.grid_aabb.data[i].min.z, (int)grid_scn_ln.grid_aabb.data[i].max.x, (int)grid_scn_ln.grid_aabb.data[i].max.y, (int)grid_scn_ln.grid_aabb.data[i].max.z);
      
      if (i < grid_scn_ln.grid_aabb.size - 1) {
        fprintf(file, ", ");
      }
    }
    
    fprintf(file, "},\n\n"); */
    
    fprintf(file, "static const grid_t grid = {\n");
    fprintf(file, "  .grid_pnt = grid_pnt,\n");
    fprintf(file, "  .pl_data = pl_data,\n");
    fprintf(file, "  .vt_data = vt_data,\n\n");
    
    fprintf(file, "  .size_i = {%d, %d, %d},\n", grid_scn_ln.size_i.w, grid_scn_ln.size_i.d, grid_scn_ln.size_i.h);
    fprintf(file, "  .num_tiles = %d,\n", grid_scn_ln.num_tiles);
    fprintf(file, "  .tile_size = %d,\n", grid_scn_ln.tile_size_i << ini.fp_size);
    fprintf(file, "  .tile_size_bits = %d,\n", grid_scn_ln.tile_size_bits);
    fprintf(file, "};\n\n");
  }
  
  fprintf(file,
    "const model_t model_%d = {\n", ini.model_id);
  fprintf(file,
    "  .num_vertices = num_vertices,\n"
    "  .num_faces = num_faces,\n"
    // "  .faces_size = faces_size,\n"
    "  .num_txcoords = num_txcoords,\n"
    "  .num_tx_faces = num_tx_faces,\n"
    // "  .tx_faces_size = tx_faces_size,\n"
    "  .num_objects = num_objects,\n"
    "  .num_materials = num_materials,\n"
    "  .num_sprites = num_sprites,\n"
    "  .num_sprite_vertices = num_sprite_vertices,\n"
    "  .flags.has_normals = has_normals,\n"
    "  .flags.has_grid = has_grid,\n"
    "  .flags.has_textures = has_textures,\n"
    "  .origin = origin,\n"
    "  .size = size,\n"
    "  \n"
    "  .vertices = vertices,\n"
    "  .faces = faces,\n"
  );
  
  if (model.has_textures) {
    fprintf(file,
      "  .txcoords = txcoords,\n"
      "  .tx_faces = tx_faces,\n"
    );
  }
  
  fprintf(file,
    "  .normals = normals,\n"
    "  .face_group = face_group,\n"
  );
  
  if (model.num_sprites) {
    fprintf(file,
      "  .sprite_vertices = sprite_vertices,\n"
      "  .sprite_faces = sprite_faces,\n"
    );
  }
  
  if (model.num_objects) {
    fprintf(file,
      "  .object_face_index = object_face_index,\n"
      "  .object_num_faces = object_num_faces,\n"
      // "  .object_vt_list = object_vt_list,\n"
      // "  .object_num_vertices = object_num_vertices,\n"
      // "  .object_vt_index = object_vt_index,\n"
      // "  .objects_origin = objects_origin,\n"
      // "  .objects_size = objects_size,\n"
    );
  }
  
  if (model.has_textures) {
    fprintf(file,
      "  .mtl_textures = mtl_textures,\n"
    );
  }
  
  if (ini.make_grid) {
    fprintf(file,
      "  .grid = grid,\n"
    );
  }
  
  fprintf(file,
    "};"
  );
}

void export_texture_file(FILE *file) {
  fprintf(file, "// textures\n\n");
  fprintf(file, "static const u8 num_textures = %d;\n", textures.num_textures);
  fprintf(file, "static const u8 num_animations = %d;\n", textures.num_animations);
  fprintf(file, "static const u8 pal_size = %d;\n", textures.pal_size);
  fprintf(file, "static const u8 pal_num_colors = %d;\n", textures.pal_num_colors);
  fprintf(file, "static const u8 pal_size_tx = %d;\n", textures.pal_size_tx);
  fprintf(file, "static const u8 pal_tx_num_colors = %d;\n", textures.pal_tx_num_colors);
  fprintf(file, "static const u8 lightmap_levels = %d;\n", ini.lightmap_levels);
  fprintf(file, "static const u32 texture_data_total_size = %d;\n\n", textures.texture_data_total_size);
  
  #if EXPORT_AVERAGE_PAL
    fprintf(file, "static const u8 material_colors[] = {\n");
    
    for (int i = 0; i < textures.num_textures; i++) {
      fprintf(file, "%d", textures.material_colors.data[i]);
      
      if (i < textures.num_textures - 1){
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u16 cr_palette_idx[] = {\n");
    
    if (!ini.create_lightmap) {
      for (int i = 0; i < textures.num_textures; i++) {
        fprintf(file, "0x%04x", textures.cr_palette.data[i]);
        
        if (i < textures.num_textures - 1) {
          fprintf(file, ",");
          
          if ((i & 15) == 15) {
            fprintf(file, "\n");
          }
        }
      }
      fprintf(file, "};\n\n");
      
    } else {
      for (int i = 0; i < textures.num_textures; i++) {
        for (int j = 0; j < ini.lightmap_levels; j++) {
          fprintf(file, "0x%04x", textures.cr_palette_idx.data[i * ini.lightmap_levels + j]);
          
          if (j < ini.lightmap_levels - 1) {
            fprintf(file, ",");
          }
        }
        
        if (i < textures.num_textures - 1) {
          fprintf(file, ",\n");
        }
      }
      fprintf(file, "};\n\n");
    }
  #endif
  
  if (model.has_textures) {
    fprintf(file, "static const u8 material_colors_tx[] = {\n");
    
    for (int i = 0; i < textures.num_textures; i++) {
      fprintf(file, "%d", textures.material_colors_tx.data[i]);
      
      if (i < textures.num_textures - 1){
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u16 cr_palette_tx_idx[] = {\n");
    
    if (!ini.create_lightmap) {
      for (int i = 0; i < textures.pal_size_tx; i++) {
        fprintf(file, "0x%04x", textures.cr_palette_tx.data[i]);
        
        if (i < textures.pal_size_tx - 1) {
          fprintf(file, ",");
          
          if ((i & 15) == 15) {
            fprintf(file, "\n");
          }
        }
      }
      fprintf(file, "};\n\n");
    } else {
      for (int i = 0; i < textures.pal_size_tx; i++) {
        fprintf(file, "0x%04x", textures.cr_palette_tx_idx.data[i]);
        
        if (i < textures.pal_size_tx - 1) {
          fprintf(file, ",");
          
          if ((i & (ini.lightmap_levels - 1)) == ini.lightmap_levels - 1) {
            fprintf(file, "\n");
          }
        }
      }
      fprintf(file, "};\n\n");
    }
    
    /* fprintf(file, "static const size2i_u16_t texture_sizes[] = {\n");
    
    for (int i = 0; i < textures.num_textures; i++) {
      fprintf(file, "{%d,%d}", textures.texture_sizes.data[i].w, textures.texture_sizes.data[i].h);
      
      if (i < textures.num_textures - 1){
        fprintf(file, ", ");
      }
    }
    fprintf(file, "};\n\n"); */
    
    fprintf(file, "// texture_sizes_padded_wh, texture_width_bits, tx_animation_id, tx_index\n");
    fprintf(file, "static const tx_group_t tx_group[] = {\n");
    
    for (int i = 0; i < textures.num_textures; i++) {
      fprintf(file, "{%d", (textures.texture_sizes_padded.data[i].h << 16) | textures.texture_sizes_padded.data[i].w);
      fprintf(file, ",%d", textures.texture_width_bits.data[i]);
      if (textures.num_animations) {
        fprintf(file, ",%d", textures.tx_animation_id.data[i]);
      } else {
        fprintf(file, ",0");
      }
      fprintf(file, ",%d}", textures.tx_index.data[i]);
      
      if (i < textures.num_textures - 1){
        fprintf(file, ", ");
      }
    }
    fprintf(file, "};\n\n");
    
    if (ini.dup_tx_colors) {
      fprintf(file, "static const u16 texture_data[] = {\n");
    } else {
      fprintf(file, "static const u8 texture_data[] = {\n");
    }
    
    for (int i = 0; i < textures.num_textures; i++) {
      int tx_index = textures.tx_index.data[i];
      
      for (int j = 0; j < textures.texture_total_sizes.data[i]; j++) {
        fprintf(file, "%d", textures.texture_data.data[tx_index + j]);
        
        if (j < textures.texture_total_sizes.data[i] - 1 || i < textures.num_textures - 1){
          fprintf(file, ",");
          
          if ((j % textures.texture_sizes.data[i].w) == textures.texture_sizes.data[i].w - 1) {
            fprintf(file, "\n");
          }
        }
      }
      
      if (i < textures.num_textures - 1){
        fprintf(file, "\n");
      }
    }
    fprintf(file, "};\n\n");
  }
  
  fprintf(file,
    "const textures_t textures_%d = {\n", ini.model_id);
  fprintf(file,
    "  .num_textures = num_textures,\n"
    "  .num_animations = num_animations,\n"
    "  .pal_size = pal_size,\n"
    "  .pal_num_colors = pal_num_colors,\n"
    "  .pal_size_tx = pal_size_tx,\n"
    "  .pal_tx_num_colors = pal_tx_num_colors,\n"
    "  .lightmap_levels = lightmap_levels,\n"
    "  .texture_data_total_size = texture_data_total_size,\n\n"
    
    #if EXPORT_AVERAGE_PAL
      "  .material_colors = material_colors,\n"
      "  .cr_palette_idx = cr_palette_idx,\n"
    #endif
  );
  
  if (model.has_textures) {
    fprintf(file,
      "  .material_colors_tx = material_colors_tx,\n"
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
  
  fprintf(file, "num_vertices = %d;\n", model.num_vertices);
  fprintf(file, "num_faces = %d;\n", model.num_faces);
  fprintf(file, "faces_size = %d;\n", model.faces_size);
  fprintf(file, "num_txcoords = %d;\n", model.num_txcoords);
  fprintf(file, "num_tx_faces = %d;\n", model.num_tx_faces);
  fprintf(file, "tx_faces_size = %d;\n", model.tx_faces_size);
  fprintf(file, "num_objects = %d;\n", model.num_objects);
  fprintf(file, "num_materials = %d;\n", model.num_materials);
  fprintf(file, "num_sprites = %d;\n", model.num_sprites);
  fprintf(file, "num_sprite_vertices = %d;\n", model.num_sprite_vertices);
  fprintf(file, "has_normals = 1;\n");
  fprintf(file, "has_grid = 0;\n");
  fprintf(file, "has_textures = %d;\n", model.has_textures);
  
  if (ini.make_fp) {
    fprintf(file, "origin = %d, %d, %d;\n", (int)model.origin.x, (int)model.origin.y, (int)model.origin.z);
    fprintf(file, "size = %d, %d, %d;\n", (int)model.size.w, (int)model.size.d, (int)model.size.h);
  } else {
    fprintf(file, "origin = %.6f, %.6f, %.6f;\n", model.origin.x, model.origin.y, model.origin.z);
    fprintf(file, "size = %.6f, %.6f, %.6f;\n", model.size.w, model.size.d, model.size.h);
  }
  fprintf(file, "\n");
  
  fprintf(file, "vertices =\n");
  
  for (int i = 0; i < model.num_vertices; i++) {
    if (ini.make_fp) {
      fprintf(file, "%d,%d,%d", (int)model.vertices.data[i].x, (int)model.vertices.data[i].y, (int)model.vertices.data[i].z);
    } else {
      fprintf(file, "%.6f,%.6f,%.6f", model.vertices.data[i].x, model.vertices.data[i].y, model.vertices.data[i].z);
    }
    
    if (i < model.num_vertices - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, ";\n\n");
  
  fprintf(file, "faces =\n");
  
  for (int i = 0; i < model.num_faces; i++) {
    if (!model.num_sprites || !(model.face_types.data[i] & SPRITE)) {
      for (int j = 0; j < model.face_num_vertices.data[i]; j++) {
        fprintf(file, "%d", model.faces.data[model.face_index.data[i] + j]);
        
        if (j < model.face_num_vertices.data[i] - 1) {
          fprintf(file, ",");
        } else
        if (i < model.num_faces - 1) {
          fprintf(file, ", ");
        }
      }
    } else {
      fprintf(file, "%d", model.faces.data[model.face_index.data[i]]);
      
      if (i < model.num_faces - 1) {
        fprintf(file, ", ");
      }
    }
  }
  fprintf(file, ";\n\n");
  
  fprintf(file, "txcoords =\n");
  
  for (int i = 0; i < model.num_txcoords; i++) {
    if (ini.make_fp) {
      fprintf(file, "%d,%d", (int)model.txcoords.data[i].u, (int)model.txcoords.data[i].v);
    } else {
      fprintf(file, "%.6f,%.6f", model.txcoords.data[i].u, model.txcoords.data[i].v);
    }
    
    if (i < model.num_txcoords - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, ";\n\n");
  
  fprintf(file, "tx_faces =\n");
  
  for (int i = 0; i < model.num_tx_faces; i++) {
    for (int j = 0; j < model.face_num_vertices.data[i]; j++) {
      fprintf(file, "%d", model.tx_faces.data[model.tx_face_index.data[i] + j]);
      
      if (j < model.face_num_vertices.data[i] - 1) {
        fprintf(file, ",");
      } else
      if (i < model.num_tx_faces - 1) {
        fprintf(file, ", ");
      }
    }
  }
  fprintf(file, ";\n\n");
  
  fprintf(file, "normals =\n");
  
  for (int i = 0; i < model.num_faces; i++) {
    if (ini.make_fp) {
      fprintf(file, "%d,%d,%d", (int)model.normals.data[i].x, (int)model.normals.data[i].y, (int)model.normals.data[i].z);
    } else {
      fprintf(file, "%.6f,%.6f,%.6f", model.normals.data[i].x, model.normals.data[i].y, model.normals.data[i].z);
    }
    
    if (i < model.num_faces - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, ";\n\n");
  
  fprintf(file, "face_index =\n");
  
  for (int i = 0; i < model.num_faces; i++) {
    fprintf(file, "%d", model.face_index.data[i]);
    if (i < model.num_faces - 1) {
      fprintf(file, ",");
    }
  }
  fprintf(file, ";\n\n");
  
  fprintf(file, "tx_face_index =\n");
  
  for (int i = 0; i < model.num_faces; i++) {
    fprintf(file, "%d", model.tx_face_index.data[i]);
    
    if (i < model.num_faces - 1) {
      fprintf(file, ",");
    }
  }
  fprintf(file, ";\n\n");
  
  fprintf(file, "face_num_vertices =\n");
  
  for (int i = 0; i < model.num_faces; i++) {
    fprintf(file, "%d", model.face_num_vertices.data[i]);
    
    if (i < model.num_faces - 1) {
      fprintf(file, ",");
    }
  }
  fprintf(file, ";\n\n");
  
  fprintf(file, "face_materials =\n");
  
  for (int i = 0; i < model.num_tx_faces; i++) {
    fprintf(file, "%d", model.face_materials.data[i]);
    
    if (i < model.num_tx_faces - 1) {
      fprintf(file, ",");
    }
  }
  fprintf(file, ";\n\n");
  
  fprintf(file, "face_types =\n");
  
  for (int i = 0; i < model.num_faces; i++) {
    fprintf(file, "%d", model.face_types.data[i]);
    
    if (i < model.num_faces - 1) {
      fprintf(file, ",");
    }
  }
  fprintf(file, ";\n\n");
  
  if (model.num_sprites) {    
    fprintf(file, "sprite_vertices =\n");
    
    for (int i = 0; i < model.num_sprite_vertices; i++) {
      if (ini.make_fp) {
        fprintf(file, "%d,%d,%d", (int)model.sprite_vertices.data[i].x, (int)model.sprite_vertices.data[i].y, (int)model.sprite_vertices.data[i].z);
      } else {
        fprintf(file, "%.6f,%.6f,%.6f", model.sprite_vertices.data[i].x, model.sprite_vertices.data[i].y, model.sprite_vertices.data[i].z);
      }
      
      if (i < model.num_sprite_vertices - 1) {
        fprintf(file, ", ");
      }
    }
    fprintf(file, ";\n\n");
    
    fprintf(file, "sprite_faces =\n");
    
    for (int i = 0; i < model.num_sprites; i++) {
      for (int j = 0; j < 4; j++) {
        fprintf(file, "%d", model.sprite_faces.data[i * 4 + j]);
        
        if (j < 3) {
          fprintf(file, ",");
        } else
        if (i < model.num_sprites - 1) {
          fprintf(file, ", ");
        }
      }
    }
    fprintf(file, ";\n\n");
    
    fprintf(file, "sprite_face_index =\n");
    
    for (int i = 0; i < model.num_faces; i++) {
      fprintf(file, "%d", model.sprite_face_index.data[i]);
      if (i < model.num_faces - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, ";\n\n");
  }
  
  if (model.num_objects) {
    fprintf(file, "object_face_index =\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      fprintf(file, "%d", model.object_face_index.data[i]);
      
      if (i < model.num_objects - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, ";\n\n");
    
    fprintf(file, "object_num_faces =\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      fprintf(file, "%d", model.object_num_faces.data[i]);
      
      if (i < model.num_objects - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, ";\n\n");
    
    /* fprintf(file, "object_vt_list =\n");
    
    for (int i = 0; i < model.object_vt_list.size; i++) {
      fprintf(file, "%d", model.object_vt_list.data[i]);
      
      if (i < model.object_vt_list.size - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, ";\n\n");
    
    fprintf(file, "object_num_vertices =\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      fprintf(file, "%d", model.object_num_vertices.data[i]);
      
      if (i < model.num_objects - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, ";\n\n");
    
    fprintf(file, "object_vt_index =\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      fprintf(file, "%d", model.object_vt_index.data[i]);
      
      if (i < model.num_objects - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, ";\n\n");
    
    fprintf(file, "objects_origin =\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      if (ini.make_fp) {
        fprintf(file, "%d,%d,%d", (int)model.objects_origin.data[i].x, (int)model.objects_origin.data[i].y, (int)model.objects_origin.data[i].z);
      } else {
        fprintf(file, "%.6f,%.6f,%.6f", model.objects_origin.data[i].x, model.objects_origin.data[i].y, model.objects_origin.data[i].z);
      }
      
      if (i < model.num_objects - 1) {
        fprintf(file, ", ");
      }
    }
    fprintf(file, ";\n\n"); */
    
    fprintf(file, "objects_size =\n");
    
    for (int i = 0; i < model.num_objects; i++) {
      if (ini.make_fp) {
        fprintf(file, "%d,%d,%d", (int)model.objects_size.data[i].w, (int)model.objects_size.data[i].d, (int)model.objects_size.data[i].h);
      } else {
        fprintf(file, "%.6f,%.6f,%.6f", model.objects_size.data[i].w, model.objects_size.data[i].d, model.objects_size.data[i].h);
      }
      
      if (i < model.num_objects - 1) {
        fprintf(file, ", ");
      }
    }
    fprintf(file, ";\n\n");
  }
  
  fprintf(file, "mtl_textures =\n");
  
  for (int i = 0; i < model.num_materials; i++) {
    fprintf(file, "%d", model.mtl_textures.data[i]);
    
    if (i < model.num_materials - 1) {
      fprintf(file, ",");
    }
  }
  fprintf(file, ";");
  
  // textures
  
  if (ini.export_textures) {
    fprintf(file, "\n\n");
    fprintf(file, "// textures\n\n");
    fprintf(file, "num_textures = %d;\n", textures.num_textures);
    fprintf(file, "num_animations = %d;\n", textures.num_animations);
    fprintf(file, "pal_size = %d;\n", textures.pal_size);
    fprintf(file, "pal_num_colors = %d;\n", textures.pal_num_colors);
    fprintf(file, "pal_size_tx = %d;\n", textures.pal_size_tx);
    fprintf(file, "pal_tx_num_colors = %d;\n", textures.pal_tx_num_colors);
    fprintf(file, "lightmap_levels = %d;\n\n", ini.lightmap_levels);
    
    if (model.has_textures) {
      fprintf(file, "texture_data_total_size = %d;\n\n", textures.texture_data_total_size);
    }
    
    #if EXPORT_AVERAGE_PAL
      fprintf(file, "material_colors =\n");
      
      for (int i = 0; i < textures.num_textures; i++) {
        fprintf(file, "%d", textures.material_colors.data[i]);
        
        if (i < textures.num_textures - 1){
          fprintf(file, ",");
        }
      }
      fprintf(file, ";\n\n");
      
      fprintf(file, "cr_palette_idx =\n");
      
      if (!ini.create_lightmap) {
        for (int i = 0; i < textures.num_textures; i++) {
          fprintf(file, "0x%04x", textures.cr_palette.data[i]);
          
          if (i < textures.num_textures - 1) {
            fprintf(file, ",");
            
            if ((i & 15) == 15) {
              fprintf(file, "\n");
            }
          }
        }
        fprintf(file, ";\n\n");
      } else {
        for (int i = 0; i < textures.num_textures; i++) {
          for (int j = 0; j < ini.lightmap_levels; j++) {
            fprintf(file, "0x%04x", textures.cr_palette_idx.data[i * ini.lightmap_levels + j]);
            
            if (j < ini.lightmap_levels - 1) {
              fprintf(file, ",");
            }
          }
          
          if (i < textures.num_textures - 1) {
            fprintf(file, ",\n");
          }
        }
        fprintf(file, ";\n\n");
      }
    #endif
    
    if (model.has_textures) {
      fprintf(file, "material_colors_tx =\n");
      
      for (int i = 0; i < textures.num_textures; i++) {
        fprintf(file, "%d", textures.material_colors_tx.data[i]);
        
        if (i < textures.num_textures - 1){
          fprintf(file, ",");
        }
      }
      fprintf(file, ";\n\n");
      
      fprintf(file, "cr_palette_tx_idx =\n");
      
      if (!ini.create_lightmap) {
        for (int i = 0; i < textures.pal_size_tx; i++) {
          fprintf(file, "0x%04x", textures.cr_palette_tx.data[i]);
          
          if (i < textures.pal_size_tx - 1) {
            fprintf(file, ",");
            
            if ((i & 15) == 15) {
              fprintf(file, "\n");
            }
          }
        }
        fprintf(file, ";\n\n");
      } else {
        for (int i = 0; i < textures.pal_tx_num_colors; i++) {
          for (int j = 0; j < ini.lightmap_levels; j++) {
            fprintf(file, "0x%04x", textures.cr_palette_tx_idx.data[i * ini.lightmap_levels + j]);
            
            if (j < ini.lightmap_levels - 1) {
              fprintf(file, ",");
            }
          }
          
          if (i < textures.pal_tx_num_colors - 1) {
            fprintf(file, ",\n");
          }
        }
        fprintf(file, ";\n\n");
      }
      
      fprintf(file, "texture_total_sizes =\n");
      
      for (int i = 0; i < textures.num_textures; i++) {
        fprintf(file, "%d", textures.texture_total_sizes.data[i]);
        
        if (i < textures.num_textures - 1){
          fprintf(file, ",");
        }
      }
      fprintf(file, ";\n\n");
      
      fprintf(file, "texture_sizes =\n");
      for (int i = 0; i < textures.num_textures; i++) {
        fprintf(file, "%d,%d", textures.texture_sizes.data[i].w, textures.texture_sizes.data[i].h);
        
        if (i < textures.num_textures - 1){
          fprintf(file, ", ");
        }
      }
      fprintf(file, ";\n\n");
      
      fprintf(file, "texture_sizes_padded =\n");
      
      for (int i = 0; i < textures.num_textures; i++) {
        fprintf(file, "%d,%d", textures.texture_sizes_padded.data[i].w, textures.texture_sizes_padded.data[i].h);
        
        if (i < textures.num_textures - 1){
          fprintf(file, ", ");
        }
      }
      fprintf(file, ";\n\n");
      
      fprintf(file, "texture_width_bits =\n");
      
      for (int i = 0; i < textures.num_textures; i++) {
        fprintf(file, "%d", textures.texture_width_bits.data[i]);
        
        if (i < textures.num_textures - 1){
          fprintf(file, ", ");
        }
      }
      fprintf(file, ";\n\n");
      
      if (textures.num_animations) {
        fprintf(file, "tx_animation_id =\n");
        
        for (int i = 0; i < textures.num_textures; i++) {
          fprintf(file, "%d", textures.tx_animation_id.data[i]);
          
          if (i < textures.num_textures - 1) {
            fprintf(file, ",");
          }
        }
        fprintf(file, ";\n\n");
      }
      
      fprintf(file, "tx_index =\n");
      
      for (int i = 0; i < textures.num_textures; i++) {
        fprintf(file, "%d", textures.tx_index.data[i]);
        
        if (i < textures.num_textures - 1){
          fprintf(file, ",");
        }
      }
      fprintf(file, ";\n\n");
      
      fprintf(file, "texture_data =\n");
      
      for (int i = 0; i < textures.num_textures; i++) {
        int tx_index = textures.tx_index.data[i];
        
        for (int j = 0; j < textures.texture_total_sizes.data[i]; j++) {
          fprintf(file, "%d", textures.texture_data.data[tx_index + j]);
          
          if (j < textures.texture_total_sizes.data[i] - 1 || i < textures.num_textures - 1){
            fprintf(file, ",");
            
            if ((j % textures.texture_sizes.data[i].w) == textures.texture_sizes.data[i].w - 1) {
              fprintf(file, "\n");
            }
          }
        }
        
        if (i < textures.num_textures - 1){
          fprintf(file, "\n");
        }
      }
      fprintf(file, ";");
    }
  }
  
  fclose(file);
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
  //fprintf(file, "o Plane\n");
  
  for (int i = 0; i < model.num_vertices; i++) {
    fprintf(file, "v %.6f %.6f %.6f\n", model.vertices.data[i].x, model.vertices.data[i].y, model.vertices.data[i].z);
  }
  
  for (int i = 0; i < model.num_txcoords; i++) {
    fprintf(file, "vt %.6f %.6f\n", model.txcoords.data[i].u, model.txcoords.data[i].v);
  }
  
  //fprintf(file, "usemtl Material\n");
  //fprintf(file, "s off\n");
  
  int prev_mtl = -1;
  
  if (model.num_objects) {
    for (int i = 0; i < model.num_objects; i++) {
      fprintf(file, "o Plane.%d\n", i);
      
      for (int j = 0; j < model.object_num_faces.data[i]; j++) {
        int face_idx = model.object_face_index.data[i] + j;
        
        if (model.face_materials.data[face_idx] != prev_mtl) {
          fprintf(file, "usemtl %s\n", material_list.data[model.face_materials.data[face_idx]]);
          fprintf(file, "s off\n");
          prev_mtl = model.face_materials.data[face_idx];
        }
        
        fprintf(file, "f");
        
        for (int k = 0; k < model.face_num_vertices.data[face_idx]; k++) {
          fprintf(file, " %d", model.faces.data[model.face_index.data[face_idx] + k] + 1);
          fprintf(file, "/%d", model.tx_faces.data[model.tx_face_index.data[face_idx] + k] + 1);
        }
        
        fprintf(file, "\n");
      }
    }
  } else {
    for (int i = 0; i < model.num_faces; i++) {
      if (model.face_materials.data[i] != prev_mtl) {
        fprintf(file, "usemtl %s\n", material_list.data[model.face_materials.data[i]]);
        fprintf(file, "s off\n");
        prev_mtl = model.face_materials.data[i];
      }
      
      fprintf(file, "f");
      
      for (int j = 0; j < model.face_num_vertices.data[i]; j++) {
        fprintf(file, " %d", model.faces.data[model.face_index.data[i] + j] + 1);
        fprintf(file, "/%d", model.tx_faces.data[model.tx_face_index.data[i] + j] + 1);
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