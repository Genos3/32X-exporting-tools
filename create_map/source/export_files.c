#include <shlwapi.h>
#include "common.h"

void make_file_c_mdl(char *export_path);
void make_file_c_tex(char *export_path);
void export_model_file(FILE *file);
void export_texture_file(FILE *file);
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
  
  PathRemoveFileSpec(path);
  char file_name[STR_SIZE];
  sprintf(file_name, "map_mdl_%d.c", ini.model_id);
  PathAppend(path, file_name);
  
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
  char file_name[STR_SIZE];
  sprintf(file_name, "map_tex_%d.c", ini.model_id);
  PathAppend(path, file_name);
  
  file = fopen(path, "w");
  
  if (file == NULL) {
    printf("can't make the texture file\n");
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
  fprintf(file, "static const u16 num_vertices = %d;\n", scene.num_vertices);
  fprintf(file, "static const u16 num_faces = %d;\n", scene.num_faces);
  // fprintf(file, "static const u32 faces_size = %d;\n", scene.faces_size);
  fprintf(file, "static const u16 num_txcoords = %d;\n", scene.num_txcoords);
  fprintf(file, "static const u16 num_tx_faces = %d;\n", scene.num_tx_faces);
  // fprintf(file, "static const u32 tx_faces_size = %d;\n", scene.tx_faces_size);
  fprintf(file, "static const u16 num_objects = %d;\n", scene.num_objects);
  fprintf(file, "static const u8 num_materials = %d;\n", scene.num_materials);
  fprintf(file, "static const u8 num_sprites = %d;\n", scene.num_sprites);
  fprintf(file, "static const u8 num_sprite_vertices = %d;\n", scene.num_sprite_vertices);
  fprintf(file, "static const u8 has_normals = 1;\n");
  fprintf(file, "static const u8 has_grid = %d;\n", ini.make_grid);
  fprintf(file, "static const u8 has_textures = %d;\n", scene.has_textures);
  if (ini.make_fp) {
    fprintf(file, "static const vec3_s16_t origin = {%d, %d, %d};\n", (int)scene.origin.x, (int)scene.origin.y, (int)scene.origin.z);
    fprintf(file, "static const size3_u16_t size = {%d, %d, %d};\n", (int)scene.size.w, (int)scene.size.d, (int)scene.size.h);
  } else {
    fprintf(file, "static const vec3_s16_t origin = {%.6f, %.6f, %.6f};\n", scene.origin.x, scene.origin.y, scene.origin.z);
    fprintf(file, "static const size3_u16_t size = {%.6f, %.6f, %.6f};\n", scene.size.w, scene.size.d, scene.size.h);
  }
  fprintf(file, "\n");
  
  if (ini.make_fp) {
    fprintf(file, "static const vec3_s16_t vertices[] = {\n");
  } else {
    fprintf(file, "static const vec3_t vertices[] = {\n");
  }
  
  for (int i = 0; i < scene.num_vertices; i++) {
    if (ini.make_fp) {
      fprintf(file, "{%d,%d,%d}", (int)scene.vertices.data[i].x, (int)scene.vertices.data[i].y, (int)scene.vertices.data[i].z);
    } else {
      fprintf(file, "{%.6f,%.6f,%.6f}", scene.vertices.data[i].x, scene.vertices.data[i].y, scene.vertices.data[i].z);
    }
    
    if (i < scene.num_vertices - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, "};\n\n");
  
  fprintf(file, "static const u16 faces[] = {\n");
  
  for (int i = 0; i < scene.num_faces; i++) {
    if (!scene.num_sprites || !(scene.face_types.data[i] & SPRITE)) {
      for (int j = 0; j < scene.face_num_vertices.data[i]; j++) {
        fprintf(file, "%d", scene.faces.data[scene.face_index.data[i] + j]);
        
        if (j < scene.face_num_vertices.data[i] - 1) {
          fprintf(file, ",");
        } else
        if (i < scene.num_faces - 1) {
          fprintf(file, ", ");
        }
      }
    } else {
      fprintf(file, "%d", scene.faces.data[scene.face_index.data[i]]);
      
      if (i < scene.num_faces - 1) {
        fprintf(file, ", ");
      }
    }
  }
  fprintf(file, "};\n\n");
  
  if (scene.has_textures) {
    if (ini.make_fp) {
      fprintf(file, "static const vec2_tx_u16_t txcoords[] = {\n");
    } else {
      fprintf(file, "static const vec2_tx_t txcoords[] = {\n");
    }
    
    for (int i = 0; i < scene.num_txcoords; i++) {
      if (ini.make_fp) {
        fprintf(file, "{%d,%d}", (int)scene.txcoords.data[i].u, (int)scene.txcoords.data[i].v);
      } else {
        fprintf(file, "{%.6f,%.6f}", scene.txcoords.data[i].u, scene.txcoords.data[i].v);
      }
      
      if (i < scene.num_txcoords - 1) {
        fprintf(file, ", ");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u16 tx_faces[] = {\n");
    
    for (int i = 0; i < scene.num_tx_faces; i++) {
      for (int j = 0; j < scene.face_num_vertices.data[i]; j++) {
        fprintf(file, "%d", scene.tx_faces.data[scene.tx_face_index.data[i] + j]);
        
        if (j < scene.face_num_vertices.data[i] - 1) {
          fprintf(file, ",");
        } else
        if (i < scene.num_tx_faces - 1) {
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
  
  for (int i = 0; i < scene.num_faces; i++) {
    if (ini.make_fp) {
      fprintf(file, "{%d,%d,%d}", (int)scene.normals.data[i].x, (int)scene.normals.data[i].y, (int)scene.normals.data[i].z);
    } else {
      fprintf(file, "{%.6f,%.6f,%.6f}", scene.normals.data[i].x, scene.normals.data[i].y, scene.normals.data[i].z);
    }
    
    if (i < scene.num_faces - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, "};\n\n");
  
  fprintf(file, "// face_num_vertices, face_materials, face_types, face_index, tx_face_index, sprite_face_index\n");
  fprintf(file, "static const face_group_t face_group[] = {\n");
  
  for (int i = 0; i < scene.num_faces; i++) {
    fprintf(file, "{%d", scene.face_num_vertices.data[i]);
    fprintf(file, ",%d", scene.face_materials.data[i]);
    fprintf(file, ",%d", scene.face_types.data[i]);
    fprintf(file, ",%d", scene.face_index.data[i]);
    if (scene.has_textures) {
      fprintf(file, ",%d", scene.tx_face_index.data[i]);
    } else {
      fprintf(file, ",0");
    }
    if (scene.num_sprites) {
      fprintf(file, ",%d}", scene.sprite_face_index.data[i]);
    } else {
      fprintf(file, ",0}");
    }
    
    if (i < scene.num_faces - 1) {
      fprintf(file, ",");
    }
  }
  fprintf(file, "};\n\n");
  
  if (scene.num_sprites) {
    /* // determines which vertex belongs to a sprite
    fprintf(file, "static const u8 type_vt[] = {\n");
    
    for (int i = 0; i < scene.num_vertices; i++) {
      fprintf(file, "%d", scene.type_vt[i]);
      
      if (i < scene.num_vertices - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n"); */
    
    if (ini.make_fp) {
      fprintf(file, "static const vec3_s16_t sprite_vertices[] = {\n");
    } else {
      fprintf(file, "static const vec3_t sprite_vertices[] = {\n");
    }
    
    for (int i = 0; i < scene.num_sprite_vertices; i++) {
      if (ini.make_fp) {
        fprintf(file, "{%d,%d,%d}", (int)scene.sprite_vertices.data[i].x, (int)scene.sprite_vertices.data[i].y, (int)scene.sprite_vertices.data[i].z);
      } else {
        fprintf(file, "{%.6f,%.6f,%.6f}", scene.sprite_vertices.data[i].x, scene.sprite_vertices.data[i].y, scene.sprite_vertices.data[i].z);
      }
      
      if (i < scene.num_sprite_vertices - 1) {
        fprintf(file, ", ");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u8 sprite_faces[] = {\n");
    
    for (int i = 0; i < scene.num_sprites; i++) {
      for (int j = 0; j < 4; j++) {
        fprintf(file, "%d", scene.sprite_faces.data[i * 4 + j]);
        
        if (j < 3) {
          fprintf(file, ",");
        } else
        if (i < scene.num_sprites - 1) {
          fprintf(file, ", ");
        }
      }
    }
    fprintf(file, "};\n\n");
    
    /* fprintf(file, "static const s16 sprite_face_index[] = {\n");
    
    for (int i = 0; i < scene.num_faces; i++) {
      fprintf(file, "%d", scene.sprite_face_index.data[i]);
      
      if (i < scene.num_faces - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n"); */
  }
  
  if (scene.num_objects) {
    fprintf(file, "static const u16 object_face_index[] = {\n");
    
    for (int i = 0; i < scene.num_objects; i++) {
      fprintf(file, "%d", scene.object_face_index.data[i]);
      
      if (i < scene.num_objects - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u16 object_num_faces[] = {\n");
    
    for (int i = 0; i < scene.num_objects; i++) {
      fprintf(file, "%d", scene.object_num_faces.data[i]);
      
      if (i < scene.num_objects - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
  }
  
  if (scene.has_textures) {
    fprintf(file, "static const u8 mtl_textures[] = {\n");
    
    for (int i = 0; i < scene.num_materials; i++) {
      fprintf(file, "%d", scene.mtl_textures.data[i]);
      if (i < scene.num_materials - 1) {
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
  }
  
  if (scene.has_grid) {
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
    
    /* fprintf(file, "  .grid_pnt = (const s16[]) {\n  ");
    
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
  
  if (scene.has_textures) {
    fprintf(file,
      "  .txcoords = txcoords,\n"
      "  .tx_faces = tx_faces,\n"
    );
  }
  
  fprintf(file,
    "  .normals = normals,\n"
    "  .face_group = face_group,\n"
  );
  
  if (scene.num_sprites) {
    fprintf(file,
      // "  .type_vt = type_vt,\n"
      "  .sprite_vertices = sprite_vertices,\n"
      "  .sprite_faces = sprite_faces,\n"
    );
  }
  
  if (scene.num_objects) {
    fprintf(file,
      "  .object_face_index = object_face_index,\n"
      "  .object_num_faces = object_num_faces,\n"
    );
  }
  
  if (scene.has_textures) {
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
  fprintf(file, "static const u8 lightmap_levels = %d;\n", textures.lightmap_levels);
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
    
    if (!textures.lightmap_levels) {
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
        for (int j = 0; j < textures.lightmap_levels; j++) {
          fprintf(file, "0x%04x", textures.cr_palette_idx.data[i * textures.lightmap_levels + j]);
          
          if (j < textures.lightmap_levels - 1) {
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
  
  if (scene.has_textures) {
    fprintf(file, "static const u8 material_colors_tx[] = {\n");
    
    for (int i = 0; i < textures.num_textures; i++) {
      fprintf(file, "%d", textures.material_colors_tx.data[i]);
      
      if (i < textures.num_textures - 1){
        fprintf(file, ",");
      }
    }
    fprintf(file, "};\n\n");
    
    fprintf(file, "static const u16 cr_palette_tx_idx[] = {\n");
    
    if (!textures.lightmap_levels) {
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
          
          if ((i & (textures.lightmap_levels - 1)) == textures.lightmap_levels - 1) {
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
    
    fprintf(file, "static const u16 texture_data[] = {\n");
    
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
    "const textures_t textures_%d = {\n", ini.model_id
  );
  
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
  
  if (scene.has_textures) {
    fprintf(file,
      "  .material_colors_tx = material_colors_tx,\n"
      "  .cr_palette_tx_idx = cr_palette_tx_idx,\n"
      "  .tx_group = tx_group,\n"
      "  .texture_data = texture_data,\n"
    );
  }
  
  fprintf(file, "};");
}

void make_file_obj(char *export_path) {
  FILE *file;
  char path[MAX_PATH];
  strncpy(path, export_path, MAX_PATH - 1);
  //PathRenameExtension(path, ".obj");
  
  PathRenameExtension(path, ".mtl");
  
  file = fopen(path, "rb");
  
  if (file == NULL) {
    printf("mtl file not found\n");
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
  PathAppend(path, "map.mtl");
  
  file = fopen(path, "wb");
  if (file == NULL) {
    printf("can't make the mtl file\n");
    return;
  }
  
  fwrite(data, 1, size, file);
  fclose(file);
  
  PathRemoveFileSpec(path);
  PathAppend(path, "map.obj");
  
  file = fopen(path, "w");
  if (file == NULL) {
    printf("can't make the obj file\n");
    return;
  }
  
  fprintf(file, "#custom OBJ exporter\n");
  
  fprintf(file, "mtllib map.mtl\n");
  fprintf(file, "o Plane\n");
  
  for (int i = 0; i < scene.num_vertices; i++) {
    fprintf(file, "v %.6f %.6f %.6f\n", scene.vertices.data[i].x, scene.vertices.data[i].y, scene.vertices.data[i].z);
  }
  for (int i = 0; i < scene.num_txcoords; i++) {
    fprintf(file, "vt %.6f %.6f\n", scene.txcoords.data[i].u, scene.txcoords.data[i].v);
  }
  
  //fprintf(file, "usemtl Material\n");
  //fprintf(file, "s off\n");
  
  int prev_mtl = -1;
  for (int i = 0; i < scene.num_faces; i++) {
    if (scene.face_materials.data[i] != prev_mtl) {
      fprintf(file, "usemtl %s\n", material_list.data[scene.face_materials.data[i]]);
      fprintf(file, "s off\n");
      prev_mtl = scene.face_materials.data[i];
    }
    
    fprintf(file, "f");
    for (int j = 0; j < scene.face_num_vertices.data[i]; j++) {
      fprintf(file, " %d", scene.faces.data[scene.face_index.data[i] + j] + 1);
      fprintf(file, "/%d", scene.tx_faces.data[scene.tx_face_index.data[i] + j] + 1);
    }
    
    fprintf(file, "\n");
  }
  
  for (int i = 0; i < scene.num_materials; i++) {
    free(material_list.data[i]);
  }
  
  free_list((list_def*)&material_list);
  
  fclose(file);
}