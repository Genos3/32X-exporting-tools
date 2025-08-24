#include "common.h"

void scale_vertices(object_t *object);
void make_ccw(object_t *object);
void convert_to_float(object_t *object);

void process_data() {
  // convert_to_float(object);
  
  if (ini.join_objects) {
    join_mdl_objects(&tileset);
  }
  
  make_model();
  
  // scale the model
  
  set_objects_size(&scene);
  
  object_t *object = scene.object.data
  
  // pass the vertices from the object to the face list
  
  set_face_vertices_from_object(object);
  
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
  
  // move the vertices back to the vertex list
  
  #if ENABLE_VERTEX_GRID
    set_object_vertices_from_face(object);
    merge_obj_vertices(object);
  #else
    set_merged_object_vertices_from_face(object);
  #endif
  
  // creates a grid for the model, used for frustum culling
  
  if (ini.make_grid && ini.file_type_export != OBJ_FILE) {
    make_scn_grid(&scene);
    
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
  }
  
  // scale the model
  
  if (ini.scale_vt) {
    scale_vertices(object);
  }
  
  // invert the y coordinate on the textures
  
  if (ini.file_type_export == OBJ_FILE) {
    if (object->has_textures) {
      for (int i = 0; i < object->num_txcoords; i++) {
        if (object->txcoords.data[i].v) {
          object->txcoords.data[i].v = -object->txcoords.data[i].v;
        }
      }
    }
    
    // make the face vertex order counter-clockwise
    
    make_ccw(object);
  }
  else if (ini.make_fp && ini.file_type_export == C_FILE) {
    convert_to_fixed(object);
  }
}

void scale_vertices(object_t *object) {
  for (int i = 0; i < object->num_vertices; i++) {
    object->vertices.data[i].x *= ini.scale_factor_f;
    object->vertices.data[i].y *= ini.scale_factor_f;
    object->vertices.data[i].z *= ini.scale_factor_f;
  }
  
  if (object->has_grid) {
    object->grid_ln.tile_size_i *= ini.scale_factor_f;
  }
}

void make_ccw(object_t *object) {
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

void convert_to_float(object_t *object) {
  for (int i = 0; i < object->num_vertices; i++) {
    object->vertices.data[i].x /= fp_size_i;
    object->vertices.data[i].y /= fp_size_i;
    object->vertices.data[i].z /= fp_size_i;
  }
  
  for (int i = 0; i < object->num_txcoords; i++) {
    object->txcoords.data[i].u /= fp_size_i;
    object->txcoords.data[i].v /= fp_size_i;
  }
  
  for (int i = 0; i < object->num_faces; i++) {
    face_t *face = &object->faces.data[i];
    
    face->normal.x /= fp_size_i;
    face->normal.y /= fp_size_i;
    face->normal.z /= fp_size_i;
  }
}