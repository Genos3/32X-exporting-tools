#include "common.h"

void scale_vertices();
void scale_sprite_vertices();
void make_ccw();
void convert_to_float();

void process_data() {
  // convert_to_float();
  // set_objects_size_vt(&tileset);
  
  if (ini.join_objects) {
    join_mdl_objects(&tileset);
  }
  
  #if 0
    if (!ini.make_sprites) {
      reset_sprite_type(&tileset);
    }
  #endif
  
  make_model();
  
  // scale the model
  
  set_model_size(&scene);
  
  // create the face list
  
  pl_list_t pl_list;
  
  init_pl_list(&pl_list);
  set_pl_list_from_model(&pl_list, &scene);
  
  if (ini.merge_faces) {
    merge_mdl_faces(&pl_list, &scene);
  }
  
  #if !SUBDIV_TYPE
    subdivide_faces_into_quads(&pl_list, &scene);
  #endif
  
  if (ini.remove_t_junctions) {
    set_model_from_pl_list(&pl_list, &scene);
    remove_mdl_t_junctions(&pl_list, &scene);
  }
  
  #if SUBDIV_TYPE
    subdivide_faces_into_quads(&pl_list, &scene);
  #endif
  
  set_model_from_pl_list(&pl_list, &scene);
  free_pl_list(&pl_list);
  
  if (ini.scale_vt) {
    scale_vertices();
  }
  
  if (ini.file_type_export != OBJ_FILE) {
    if (ini.make_sprites) {
      make_sprites(&scene);
      
      if (ini.scale_vt) {
        scale_sprite_vertices();
      }
    }
    
    if (ini.make_grid) {
      make_scn_grid(&scene);
    }
    
    if (scene.has_textures) {
      make_txcoords_positive(&scene);
      
      if (ini.make_fp) {
        resize_txcoords(&scene, &textures);
      }
    }
  }
  
  if (ini.merge_vertices) {
    if (!tileset.has_grid) {
      merge_mdl_vertices(&scene);
    }
    
    if (scene.has_textures) {
      merge_mdl_txcoords(&scene);
    }
  }
  
  if (ini.file_type_export == OBJ_FILE) {
    // invert texture y
    if (scene.has_textures) {
      for (int i = 0; i < scene.num_txcoords; i++) {
        if (scene.txcoords.data[i].v) {
          scene.txcoords.data[i].v = -scene.txcoords.data[i].v;
        }
      }
    }
    
    make_ccw();
  } else if (ini.make_fp && ini.file_type_export == C_FILE) {
    convert_to_fixed(&scene);
  }
}

void scale_vertices() {
  for (int i = 0; i < scene.num_vertices; i++) {
    scene.vertices.data[i].x *= ini.scale_factor_f;
    scene.vertices.data[i].y *= ini.scale_factor_f;
    scene.vertices.data[i].z *= ini.scale_factor_f;
  }
}

void scale_sprite_vertices() {
  for (int i = 0; i < scene.num_sprite_vertices; i++) {
    scene.sprite_vertices.data[i].x *= ini.scale_factor_f;
    scene.sprite_vertices.data[i].y *= ini.scale_factor_f;
    scene.sprite_vertices.data[i].z *= ini.scale_factor_f;
  }
}

void make_ccw() {
  int faces_pt = 0;
  for (int i = 0; i < scene.num_faces; i++) {
    int faces_t[12];
    for (int j = 0; j < scene.face_num_vertices.data[i]; j++) {
      faces_t[j] = scene.faces.data[faces_pt + j];
    }
    for (int j = 0; j < scene.face_num_vertices.data[i]; j++) {
      scene.faces.data[faces_pt + j] = faces_t[scene.face_num_vertices.data[i] - j - 1];
    }
    faces_pt += scene.face_num_vertices.data[i];
  }
  faces_pt = 0;
  for (int i = 0; i < scene.num_tx_faces; i++) {
    int faces_t[12];
    for (int j = 0; j < scene.face_num_vertices.data[i]; j++) {
      faces_t[j] = scene.tx_faces.data[faces_pt + j];
    }
    for (int j = 0; j < scene.face_num_vertices.data[i]; j++) {
      scene.tx_faces.data[faces_pt + j] = faces_t[scene.face_num_vertices.data[i] - j - 1];
    }
    faces_pt += scene.face_num_vertices.data[i];
  }
}

void convert_to_float() {
  for (int i = 0; i < scene.num_vertices; i++) {
    scene.vertices.data[i].x = scene.vertices.data[i].x / fp_size_i;
    scene.vertices.data[i].y = scene.vertices.data[i].y / fp_size_i;
    scene.vertices.data[i].z = scene.vertices.data[i].z / fp_size_i;
  }
  for (int i = 0; i < scene.num_faces; i++) {
    scene.normals.data[i].x = scene.normals.data[i].x / fp_size_i;
    scene.normals.data[i].y = scene.normals.data[i].y / fp_size_i;
    scene.normals.data[i].z = scene.normals.data[i].z / fp_size_i;
  }
  for (int i = 0; i < scene.num_txcoords; i++) {
    scene.txcoords.data[i].u = scene.txcoords.data[i].u / fp_size_i;
    scene.txcoords.data[i].v = scene.txcoords.data[i].v / fp_size_i;
  }
}