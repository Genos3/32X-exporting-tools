#include "common.h"

void process_textures(textures_t *textures, model_t *model);
void process_objects(object_t *object, model_t *model);
void snap_vertices(object_t *object);
void make_cw(object_t *object);
void swap_yz(object_t *object);
void scale_vertices(object_t *object);
void make_normals(object_t *object);
void set_animation_id(object_t *object, textures_t *textures);

void process_model(textures_t *textures, model_t *model) {
  if (textures->num_textures) {
    process_textures(textures, model);
  }
  
  if (ini.export_texture_data && textures->num_textures) {
    model->has_textures = 1;
  } else {
    model->has_textures = 0;
  }
  
  // obtain the objects size and store it in the object structure
  
  set_objects_size(model);
  
  // sort the objects from left to right and from top to bottom
  
  if (model->num_objects > 1) {
    sort_objects(model);
  }
  
  // move all objects to the origin
  
  if (ini.join_objects) {
    center_objects(model);
  }
  
  for (int i = 0; i < model->num_objects; i++) {
    object_t *object = &model->objects.data[i];
    process_objects(object, model);
  }
  
  if (textures->num_textures) {
    model->has_textures = 1;
  }
}

// process the texture data

void process_textures(textures_t *textures, model_t *model) {
  if (ini.export_texture_data) {
    set_untextured_colors(textures);
  }
  
  #if EXPORT_AVERAGE_PAL
    for (int i = 0; i < model->num_materials; i++) {
      model->materials.data[i].colors = model->materials.data[i].texture_id;
    }
  #endif
  
  if (ini.quantize_palette) {
    #if EXPORT_AVERAGE_PAL
      quantize_palette(&textures->cr_palette, ini.quant_pal_size, textures);
      index_palette(&textures->cr_palette, 0, textures);
    #endif
    
    if (ini.export_texture_data) {
      quantize_palette(&textures->cr_palette_tx, ini.quant_pal_size, textures);
      index_palette(&textures->cr_palette_tx, 1, textures);
    }
  }
  
  if (ini.create_lightmap) {
    set_palette_gradients(textures);
  }
  
  if (ini.dup_tx_colors) {
    dup_textures_color_index(textures);
  }
}

// process the object data

void process_objects(object_t *object, model_t *model) {
  for (int i = 0; i < object->num_faces; i++) {
    object->faces.data[i].type = model->materials.data[object->faces.data[i].material_id].type;
    
    if (object->faces.data[i].has_texture) {
      object->faces.data[i].type |= TEXTURED;
    }
    
    if (model->has_textures) {
      int tx_pt = object->faces.data[i].texture_id;
      
      if (textures.tx_group.data[tx_pt].has_alpha) {
        object->faces.data[i].type |= HAS_ALPHA;
      }
    }
  }
  
  // snap the vertices to the closest axis if they are close to it
  
  snap_vertices(object);
  
  // remove the -0 texture coordinates
  
  // normalize_txcoords();
  
  // invert the y coordinate on the textures
  
  if (object->has_textures && ini.invert_tx_y && ini.file_type_export !=  OBJ_FILE) {
    for (int i = 0; i < object->num_txcoords; i++) {
      if (object->txcoords.data[i].v) {
        object->txcoords.data[i].v = -object->txcoords.data[i].v;
      }
    }
  }
  
  // make the face vertex order clockwise (the obj default is counter-clockwise)
  
  if (ini.make_cw && ini.file_type_export != OBJ_FILE) {
    make_cw(object);
  }
  
  // scale the model
  
  if (ini.scale_vt) {
    scale_vertices(object);
  }
  
  // exchange y with z in the case z is up
  
  if (ini.swap_yz) {
    swap_yz(object);
  }
  
  // create the normals
  
  make_normals(object);
  
  // pass the vertices from the object to the face list
  
  set_face_vertices_from_object(object);
  
  if (ini.file_type_export != MDL_FILE) {
    // merges the faces
    
    if (ini.merge_faces) {
      merge_obj_faces(object);
    }
    
    #if !T_JUNC_SUBDIV_TYPE
      subdivide_faces_into_quads(object);
    #endif
    
    // remove the t-junctions
    
    if (ini.remove_t_junctions) {
      remove_obj_t_junctions(object);
    }
    
    #if T_JUNC_SUBDIV_TYPE
      subdivide_faces_into_quads(object);
    #endif
    
    // make the sprites
    
    if (ini.file_type_export != OBJ_FILE && ini.make_sprites) {
      make_sprites(object);
    }
  }
  
  // move the vertices back to the vertex list
  
  #if ENABLE_VERTEX_GRID
    set_object_vertices_from_face(object);
    merge_obj_vertices(object);
  #else
    set_merged_object_vertices_from_face(object);
  #endif
  
  // remove_extra_vertices(object);
  
  // creates a grid for the model, used for frustum culling
  
  if (ini.make_grid && ini.file_type_export == C_FILE) {
    make_scn_grid(object); // FIX: makes one face disappear
    
    // exports an angle value that can be used for the lighting
    
    set_face_angles(object);
  }
  
  if (object->has_textures) {
    if (ini.file_type_export != OBJ_FILE) {
      // set the coordinates at 0 and allows to export only unsigned numbers
      // requires unmerged texture coordinates
      
      make_txcoords_positive(object);
      
      // changes the texture coordinates from normalized values (0 to 1) in floating point to the actual image size the engine requires
      // requires unmerged texture coordinates
      
      if (ini.resize_txcoords && ini.make_fp) {
        resize_txcoords(object, &textures);
      }
    }
    
    // merges the texture coordinates
    
    set_merged_object_txcoords_from_face(object);
    
    // marks which faces are animated based on the face types and store their animation id's
    
    if (ini.file_type_export != OBJ_FILE) {
      set_animation_id(object, &textures);
    }
  }
  
  /* for (int i = 0; i < object->num_faces; i++) {
    if (object->faces.data[i].num_vertices == 4) {
      object->face_types.data[i] |= QUAD;
    }
  } */
  
  if (ini.make_fp && ini.file_type_export != OBJ_FILE) {
    // turn the floating point values to fixed point that the engine requires
    
    convert_to_fixed(object);
    
    // adjust the maximum texture coordinate to prevent out-of-bounds access
    // this ensures the coordinate stays within the valid range of the texture indices
    
    if (object->has_textures) {
      adjust_txcoords_max_size(object);
    }
  }
}

void snap_vertices(object_t *object) {
  float offset = 0.01;
  
  for (int i = 0; i < object->num_vertices; i++) {
    object->vertices.data[i].x = roundf(object->vertices.data[i].x / offset) * offset;
    object->vertices.data[i].y = roundf(object->vertices.data[i].y / offset) * offset;
    object->vertices.data[i].z = roundf(object->vertices.data[i].z / offset) * offset;
  }
  
  if (object->has_textures) {
    for (int i = 0; i < object->num_txcoords; i++) {
      object->txcoords.data[i].u = roundf(object->txcoords.data[i].u / offset) * offset;
      object->txcoords.data[i].v = roundf(object->txcoords.data[i].v / offset) * offset;
    }
  }
}

void make_cw(object_t *object) {
  for (int i = 0; i < object->num_faces; i++) {
    int t_faces[8], t_tx_faces[8];
    face_t *face = &object->faces.data[i];
    
    for (int j = 0; j < face->num_vertices; j++) {
      t_faces[j] = face->vt_index[j];
      
      if (face->has_texture) {
        t_tx_faces[j] = face->tx_vt_index[j];
      }
    }
    
    for (int j = 0; j < face->num_vertices; j++) {
      face->vt_index[j] = t_faces[face->num_vertices - j - 1];
      
      if (face->has_texture) {
        face->tx_vt_index[j] = t_tx_faces[face->num_vertices - j - 1];
      }
    }
  }
}

void scale_vertices(object_t *object) {
  for (int i = 0; i < object->num_vertices; i++) {
    object->vertices.data[i].x *= ini.scale_factor_f;
    object->vertices.data[i].y *= ini.scale_factor_f;
    object->vertices.data[i].z *= ini.scale_factor_f;
  }
}

void swap_yz(object_t *object) {
  for (int i = 0; i < object->num_vertices; i++) {
    int t = object->vertices.data[i].y;
    object->vertices.data[i].y = object->vertices.data[i].z;
    object->vertices.data[i].z = t;
  }
  
  #if 0
    for (int i = 0; i < object->num_faces; i++) {
      int t = object->face.data[i].normal.y;
      object->face.data[i].normal.y = object->face.data[i].normal.z;
      object->face.data[i].normal.z = t;
    }
  #endif
}

void make_normals(object_t *object) {
  vec3_t poly_vt[3];
  
  for (int i = 0; i < object->num_faces; i++) {
    for (int j = 0; j < 3; j++) {
      int pt = object->faces.data[i].vt_index[j];
      
      poly_vt[j] = object->vertices.data[pt];
    }
    
    vec3_t nm, uv;
    calc_normal(&poly_vt[0], &poly_vt[1], &poly_vt[2], &nm);
    normalize(&nm, &uv);
    
    object->faces.data[i].normal = uv;
  }
}

void set_animation_id(object_t *object, textures_t *textures) {
  u8 *animated_textures = calloc(textures->num_textures, 1);
  int has_animations = 0;
  
  for (int i = 0; i < object->num_faces; i++) {
    if (object->faces.data[i].type & ANIMATED) {
      has_animations = 1;
      animated_textures[object->faces.data[i].texture_id] = 1;
    }
  }
  
  if (!has_animations) {
    free(animated_textures);
    return;
  }
  
  int num_animations = 0;
  
  for (int i = 0; i < textures->num_textures; i++) {
    if (animated_textures[i]) {
      textures->tx_group.data[i].animation_id = num_animations;
      num_animations++;
    } else {
      textures->tx_group.data[i].animation_id = -1;
    }
  }
  
  textures->num_animations = num_animations;
  
  free(animated_textures);
}