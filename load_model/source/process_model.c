#include "common.h"

void snap_vertices();
void make_cw();
void set_normals();
void sort_objects();
void set_objects_vt_list();
void set_animation_id();
void scale_vertices();
void scale_sprite_vertices();
void swap_yz();

void process_data() {
  // process the texture data
  
  if (textures.num_textures) {
    if (ini.export_texture_data) {
      // set_tx_index();
      set_untextured_colors();
    }
    
    #if EXPORT_AVERAGE_PAL
      for (int i = 0; i < model.num_materials; i++) {
        textures.material_colors.data[i] = model.mtl_textures.data[i];
      }
    #endif
    
    if (ini.quantize_palette) {
      #if EXPORT_AVERAGE_PAL
        quantize_palette(&textures.cr_palette, ini.quant_pal_size);
        index_palette(&textures.cr_palette, 0);
      #endif
      if (ini.export_texture_data) {
        quantize_palette(&textures.cr_palette_tx, ini.quant_pal_size);
        index_palette(&textures.cr_palette_tx, 1);
      }
    }
    
    if (ini.create_lightmap) {
      set_palette_gradients();
    }
    
    if (ini.dup_tx_colors) {
      dup_textures_color_index();
    }
  }
  
  if (ini.export_texture_data && textures.num_textures) {
    model.has_textures = 1;
  } else {
    model.has_textures = 0;
  }
  
  // process the model data
  
  for (int i = 0; i < model.num_faces; i++) {
    model.face_types.data[i] = model.material_types.data[model.face_materials.data[i]];
    
    if (!faces_textured.data[i]) {
      model.face_types.data[i] |= UNTEXTURED;
    }
    
    if (model.has_textures) {
      int pt = model.mtl_textures.data[model.face_materials.data[i]];
      if (textures_has_alpha.data[pt]) {
        model.face_types.data[i] |= HAS_ALPHA;
      }
    }
  }
  
  #if 0
    if (!ini.make_sprites) {
      reset_sprite_type(&model);
    }
  #endif
  
  // snap the vertices to the closest axis if they are near to it
  
  snap_vertices();
  
  // remove the -0 texture coordinates
  
  // normalize_txcoords();
  
  // reduce the face index
  
  for (int i = 0; i < model.faces_size; i++) {
    model.faces.data[i]--;
  }
  for (int i = 0; i < model.tx_faces_size; i++) {
    model.tx_faces.data[i]--;
  }
  
  // invert the y coordinate on the textures
  
  if (ini.invert_tx_y && ini.file_type_export !=  OBJ_FILE) {
    for (int i = 0; i < model.num_txcoords; i++) {
      if (model.txcoords.data[i].v) {
        model.txcoords.data[i].v = -model.txcoords.data[i].v;
      }
    }
  }
  
  // make the face vertex order clockwise (the obj default is counter-clockwise)
  
  if (ini.make_cw && ini.file_type_export != OBJ_FILE) {
    make_cw();
  }
  
  // scale the model
  
  if (ini.scale_vt) {
    scale_vertices();
  }
  
  // exchange y with z in the case z is up
  
  if (ini.swap_yz) {
    swap_yz();
  }
  
  // init the face index and the create the normals
  
  set_face_index(&model);
  set_tx_face_index(&model);
  set_normals();
  
  // obtain the model size and store it in the model structure
  
  set_model_size(&model);
  
  if (ini.file_type_export != MDL_FILE && !model.num_objects) {
    // create the face list
    
    pl_list_t pl_list;
    init_pl_list(&pl_list);
    set_pl_list_from_model(&pl_list, &model);
    
    // merges the faces
    
    if (ini.merge_faces) {
      merge_mdl_faces(&pl_list, &model);
    }
    
    #if !SUBDIV_TYPE
      subdivide_faces_into_quads(&pl_list, &model);
    #endif
    
    if (ini.remove_t_junctions) {
      set_model_from_pl_list(&pl_list, &model);
      remove_mdl_t_junctions(&pl_list, &model);
    }
    
    #if SUBDIV_TYPE
      subdivide_faces_into_quads(&pl_list, &model);
    #endif
    
    set_model_from_pl_list(&pl_list, &model);
    free_pl_list(&pl_list);
  }
  
  // exports the objects
  
  if (ini.export_objects && model.num_objects) {
    set_objects_size_vt(&model);
    remove_extra_vertices(&model);
    sort_objects();
    // set_objects_vt_list();
    
    // move all objects to the origin
    if (ini.join_objects) {
      join_mdl_objects(&model);
      // update the model size
      set_model_size(&model);
    }
  }
  
  if (ini.file_type_export != OBJ_FILE) {
    // makes the sprites
    
    if (ini.make_sprites) {
      make_sprites(&model);
      
      if (ini.scale_vt) {
        scale_sprite_vertices();
      }
    }
    
    // creates a grid for the model, used for frustum culling
    
    if (ini.make_grid && ini.file_type_export == C_FILE) {
      make_scn_grid(&model); // FIX: makes one face disappear
    }
    
    if (model.has_textures) {
      // separate the texture coordinates
      
      expand_txcoords(&model);
      
      // set the coordinates at 0 and allows to export only unsigned numbers
      // requires unmerged texture coordinates
      
      make_txcoords_positive(&model);
      
      // changes the texture coordinates from normalized values (0 to 1) in floating point to the actual image size the engine requires
      // requires unmerged texture coordinates
      
      if (ini.resize_txcoords && ini.make_fp) {
        resize_txcoords(&model, &textures);
      }
    }
  }
  
  // merges the vertices and texture coordinates
  
  if (ini.merge_vertices) {
    if (!model.has_grid) {
      merge_mdl_vertices(&model);
    }
    
    if (model.has_textures) {
      merge_mdl_txcoords(&model);
    }
    
    #if 0
      if (model.num_sprites) {
        merge_mdl_sprite_vertices();
      }
    #endif
  }
  
  // marks which faces are animated based on the face types and store their animation id's
  
  if (model.has_textures) {
    set_animation_id();
  }
  
  /* for (int i = 0; i < model.num_faces; i++) {
    if (model.face_num_vertices.data[i] == 4) {
      model.face_types.data[i] |= QUAD;
    }
  } */
  
  if (ini.make_fp && ini.file_type_export != OBJ_FILE) {
    // turn the floating point values to fixed point that the engine requires
    
    convert_to_fixed(&model);
    
    // adjust the maximum texture coordinate to prevent out-of-bounds access
    // this ensures the coordinate stays within the valid range of the texture indices
    
    if (model.has_textures) {
      adjust_txcoords_max_size(&model);
    }
  }
}

void snap_vertices() {
  float offset = 0.01;
  
  for (int i = 0; i < model.num_vertices; i++) {
    model.vertices.data[i].x = roundf(model.vertices.data[i].x / offset) * offset;
    model.vertices.data[i].y = roundf(model.vertices.data[i].y / offset) * offset;
    model.vertices.data[i].z = roundf(model.vertices.data[i].z / offset) * offset;
  }
  
  for (int i = 0; i < model.num_txcoords; i++) {
    model.txcoords.data[i].u = roundf(model.txcoords.data[i].u / offset) * offset;
    model.txcoords.data[i].v = roundf(model.txcoords.data[i].v / offset) * offset;
  }
}

void make_cw() {
  int faces_pt = 0;
  
  for (int i = 0; i < model.num_faces; i++) {
    int faces_t[12];
    for (int j = 0; j < model.face_num_vertices.data[i]; j++) {
      faces_t[j] = model.faces.data[faces_pt + j];
    }
    for (int j = 0; j < model.face_num_vertices.data[i]; j++) {
      model.faces.data[faces_pt + j] = faces_t[model.face_num_vertices.data[i] - j - 1];
    }
    faces_pt += model.face_num_vertices.data[i];
  }
  
  faces_pt = 0;
  
  for (int i = 0; i < model.num_tx_faces; i++) {
    int faces_t[12];
    
    for (int j = 0; j < model.face_num_vertices.data[i]; j++) {
      faces_t[j] = model.tx_faces.data[faces_pt + j];
    }
    
    for (int j = 0; j < model.face_num_vertices.data[i]; j++) {
      model.tx_faces.data[faces_pt + j] = faces_t[model.face_num_vertices.data[i] - j - 1];
    }
    
    faces_pt += model.face_num_vertices.data[i];
  }
}

void set_normals() {
  vec3_t poly_vt[3];
  
  for (int i = 0; i < model.num_faces; i++) {
    for (int j = 0; j < 3; j++) {
      int pt = model.faces.data[model.face_index.data[i] + j];
      poly_vt[j].x = model.vertices.data[pt].x;
      poly_vt[j].y = model.vertices.data[pt].y;
      poly_vt[j].z = model.vertices.data[pt].z;
    }
    
    vec3_t nm, uv;
    calc_normal(&poly_vt[0], &poly_vt[1], &poly_vt[2], &nm);
    normalize(&nm, &uv);
    
    model.normals.data[i].x = uv.x;
    model.normals.data[i].y = uv.y;
    model.normals.data[i].z = uv.z;
  }
}

// sort the objects according to how they are layed out in the model

void sort_objects() {
  int *objects_id = malloc(model.num_objects * sizeof(int));
  u8 *visited_elements = calloc(model.num_objects, 1);
  
  for (int i = 0; i < model.num_objects; i++) {
    int n = 0;
    while (visited_elements[n]) n++;
    
    float obj_x = model.objects_origin.data[n].x;
    float obj_z = model.objects_origin.data[n].z;
    int obj_id = n;

    // get the first object in the list
    for (int j = n; j < model.num_objects; j++) {
      if (visited_elements[j]) continue;
      
      if (model.objects_origin.data[j].z < obj_z || (model.objects_origin.data[j].z == obj_z && model.objects_origin.data[j].x < obj_x)) {
        obj_x = model.objects_origin.data[j].x;
        obj_z = model.objects_origin.data[j].z;
        obj_id = j;
      }
    }

    // remove it from the list
    objects_id[i] = obj_id;
    visited_elements[obj_id] = 1;
  }
  
  // reorder the objects
  
  int *object_face_index_t = malloc(model.num_objects * sizeof(int));
  int *object_num_faces_t = malloc(model.num_objects * sizeof(int));
  vec3_t *objects_origin_t = malloc(model.num_objects * sizeof(vec3_t));
  size3_t *objects_size_t = malloc(model.num_objects * sizeof(size3_t));
  
  for (int i = 0; i < model.num_objects; i++) {
    object_face_index_t[i] = model.object_face_index.data[objects_id[i]];
    object_num_faces_t[i] = model.object_num_faces.data[objects_id[i]];
    objects_origin_t[i] = model.objects_origin.data[objects_id[i]];
    objects_size_t[i] = model.objects_size.data[objects_id[i]];
  }
  
  memcpy(model.object_face_index.data, object_face_index_t, model.num_objects * sizeof(int));
  memcpy(model.object_num_faces.data, object_num_faces_t, model.num_objects * sizeof(int));
  memcpy(model.objects_origin.data, objects_origin_t, model.num_objects * sizeof(vec3_t));
  memcpy(model.objects_size.data, objects_size_t, model.num_objects * sizeof(size3_t));
  
  free(object_face_index_t);
  free(object_num_faces_t);
  free(objects_origin_t);
  free(objects_size_t);
  
  free(objects_id);
  free(visited_elements);
}

void set_objects_vt_list() {
  int num_vt = 0;
  
  for (int i = 0; i < model.num_objects; i++) {
    int num_vt_obj = 0;
    list_push_int(&model.object_vt_index, num_vt);
    
    for (int j = 0; j < model.object_num_faces.data[i]; j++) {
      int face_id = model.object_face_index.data[i] + j;
      
      for (int k = 0; k < model.face_num_vertices.data[face_id]; k++) {
        list_push_int(&model.object_vt_list, model.faces.data[face_id + k]);
        num_vt_obj++;
      }
    }
    
    list_push_int(&model.object_num_vertices, num_vt_obj);
    num_vt += num_vt_obj;
  }
}

void set_animation_id() {
  u8 *animated_textures = calloc(textures.num_textures, 1);
  int has_animations = 0;
  
  for (int i = 0; i < model.num_faces; i++) {
    if (model.face_types.data[i] & ANIMATED) {
      has_animations = 1;
      animated_textures[model.mtl_textures.data[model.face_materials.data[i]]] = 1;
    }
  }
  
  if (!has_animations) {
    free(animated_textures);
    return;
  }
  
  int n = 0;
  
  for (int i = 0; i < textures.num_textures; i++) {
    if (animated_textures[i]) {
      list_push_int(&textures.tx_animation_id, n);
      n++;
    } else {
      list_push_int(&textures.tx_animation_id, -1);
    }
  }
  
  textures.num_animations = n;
  
  free(animated_textures);
}

void scale_vertices() {
  for (int i = 0; i < model.num_vertices; i++) {
    model.vertices.data[i].x *= ini.scale_factor_f;
    model.vertices.data[i].y *= ini.scale_factor_f;
    model.vertices.data[i].z *= ini.scale_factor_f;
  }
}

void scale_sprite_vertices() {
  for (int i = 0; i < model.num_sprite_vertices; i++) {
    model.sprite_vertices.data[i].x *= ini.scale_factor_f;
    model.sprite_vertices.data[i].y *= ini.scale_factor_f;
    model.sprite_vertices.data[i].z *= ini.scale_factor_f;
  }
}

void swap_yz() {
  for (int i = 0; i < model.num_vertices; i++) {
    int t = model.vertices.data[i].y;
    model.vertices.data[i].y = model.vertices.data[i].z;
    model.vertices.data[i].z = t;
  }
  
  for (int i = 0; i < model.num_faces; i++) {
    int t = model.normals.data[i].y;
    model.normals.data[i].y = model.normals.data[i].z;
    model.normals.data[i].z = t;
  }
  
  if (model.num_sprites) {
    for (int i = 0; i < model.num_sprite_vertices; i++) {
      int t = model.sprite_vertices.data[i].y;
      model.sprite_vertices.data[i].y = model.sprite_vertices.data[i + 2].z;
      model.sprite_vertices.data[i].z = t;
    }
  }
}