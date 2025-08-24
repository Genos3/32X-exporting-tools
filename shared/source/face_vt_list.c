#include "common.h"

// copies the vertices in the object to the face list

void set_face_vertices_from_object(object_t *object) {
  for (int i = 0; i < object->num_faces; i++) {
    for (int j = 0; j < object->faces.data[i].num_vertices; j++) {
      object->faces.data[i].vertices[j] = object->vertices.data[object->faces.data[i].vt_index[j]];
      
      if (object->faces.data[i].has_texture) {
        object->faces.data[i].txcoords[j] = object->txcoords.data[object->faces.data[i].tx_vt_index[j]];
      }
    }
  }
}

// copies the vertices in the face back to the object

void set_object_vertices_from_face(object_t *object) {
  int num_vertices = 0;
  int num_txcoords = 0;
  
  for (int i = 0; i < object->num_faces; i++) {
    for (int j = 0; j < object->faces.data[i].num_vertices; j++) {
      object->vertices.data[num_vertices] = object->faces.data[i].vertices[j];
      num_vertices++;
      
      if (object->faces.data[i].has_texture) {
        object->txcoords.data[num_txcoords] = object->faces.data[i].txcoords[j];
        num_txcoords++;
      }
    }
  }
  
  object->num_vertices = num_vertices;
  object->num_txcoords = num_txcoords;
}

// transfer the vertices from the object face to the poly_t struct

void set_poly_from_obj_face(int face_id, poly_t *poly, object_t *object) {
  poly->num_vertices = object->faces.data[face_id].num_vertices;
  
  for (int i = 0; i < poly->num_vertices; i++) {
    poly->vertices[i] = object->faces.data[face_id].vertices[i];
  }
}

// transfer the vertices from the poly_t struct to the object face

void set_obj_face_from_poly(int face_id, poly_t *poly, object_t *object) {
  object->faces.data[face_id].num_vertices = poly->num_vertices;
  
  for (int i = 0; i < poly->num_vertices; i++) {
    object->faces.data[face_id].vertices[i] = poly->vertices[i];
  }
}

void add_obj_face_from_poly(int face_id, poly_t *poly, object_t *object) {
  list_malloc_inc(&object->faces);
  object->faces.data[object->num_faces].num_vertices = poly->num_vertices;
  
  memcpy(object->faces.data[object->num_faces].vertices, poly->vertices, poly->num_vertices * sizeof(vec3_t));
  memcpy(object->faces.data[object->num_faces].txcoords, poly->txcoords, poly->num_vertices * sizeof(vec2_tx_t));
  
  object->faces.data[object->num_faces].normal = object->faces.data[face_id].normal;
  object->faces.data[object->num_faces].material_id = object->faces.data[face_id].material_id;
  object->faces.data[object->num_faces].type = object->faces.data[face_id].type;
  
  object->num_faces++;
}

// removes any marked faces

void remove_object_marked_faces(object_t *object) {
  int num_faces = 0;
  
  for (int i = 0; i < object->num_faces; i++) {
    if (object->faces.data[i].remove) continue;
    
    if (i != num_faces) {
      object->faces.data[num_faces] = object->faces.data[i];
    }
    
    num_faces++;
  }
  
  object->num_faces = num_faces;
  object->faces.size = num_faces;
}

// copies the vertices in the face back to the object while merging them
// TODO: alternatively add grid for optimization

void set_merged_object_vertices_from_face(object_t *object) {
  list_vec3_t snapped_vertices;
  init_list(&snapped_vertices, sizeof(vec3_t));
  
  int num_vertices = 0;
  
  for (int i = 0; i < object->num_faces; i++) {
    face_t *face = &object->faces.data[i];
    
    for (int j = 0; j < face->num_vertices; j++) {
      vec3_t vt = face->vertices[j];
      
      vec3_t sv0;
      sv0.x = (int)(vt.x / ini.merge_vt_dist_f);
      sv0.y = (int)(vt.y / ini.merge_vt_dist_f);
      sv0.z = (int)(vt.z / ini.merge_vt_dist_f);
      
      for (int k = 0; k < num_vertices; k++) {
        vec3_t sv1 = snapped_vertices.data[num_vertices];
        
        if (sv0.x == sv1.x && sv0.y == sv1.y && sv0.z == sv1.z) {
          face->vt_index[j] = k;
          goto next_vt;
        }
      }
      
      list_push_pnt(&snapped_vertices, &sv0);
      
      object->vertices.data[num_vertices] = vt;
      face->vt_index[j] = num_vertices;
      num_vertices++;
      
      next_vt:
    }
  }
  
  object->num_vertices = num_vertices;
  
  free_list(&snapped_vertices);
}

// copies the texture coordinates in the face back to the object while merging them

void set_merged_object_txcoords_from_face(object_t *object) {
  list_vec2_tx_t snapped_txcoords;
  init_list(&snapped_txcoords, sizeof(vec2_tx_t));
  
  int num_txcoords = 0;
  
  for (int i = 0; i < object->num_faces; i++) {
    face_t *face = &object->faces.data[i];
    
    for (int j = 0; j < face->num_vertices; j++) {
      vec2_tx_t tx_vt = face->txcoords[j];
      
      vec2_tx_t sv0;
      sv0.u = (int)(tx_vt.u / ini.merge_vt_dist_f);
      sv0.v = (int)(tx_vt.v / ini.merge_vt_dist_f);
      
      for (int k = 0; k < num_txcoords; k++) {
        vec2_tx_t sv1 = snapped_txcoords.data[num_txcoords];
        
        if (sv0.u == sv1.u && sv0.v == sv1.v) {
          face->tx_vt_index[j] = k;
          goto next_tx_vt;
        }
      }
      
      list_push_pnt(&snapped_txcoords, &sv0);
      
      object->txcoords.data[num_txcoords] = tx_vt;
      face->tx_vt_index[j] = num_txcoords;
      num_txcoords++;
      
      next_tx_vt:
    }
  }
  
  object->num_txcoords = num_txcoords;
  
  free_list(&snapped_txcoords);
}