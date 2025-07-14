#include "common.h"

void init_pl_list(pl_list_t *pl_list) {
  init_list(&pl_list->vertices, sizeof(*pl_list->vertices.data) * 8);
  init_list(&pl_list->txcoords, sizeof(*pl_list->txcoords.data) * 8);
  init_list(&pl_list->face_num_vertices, sizeof(*pl_list->face_num_vertices.data));
  pl_list->num_faces = 0;
}

void free_pl_list(pl_list_t *pl_list) {
  free_list(&pl_list->vertices);
  free_list(&pl_list->txcoords);
  free_list(&pl_list->face_num_vertices);
}

// copies the faces into a separate expanded list

void set_pl_list_from_model(pl_list_t *pl_list, model_t *model) {
  for (int i = 0; i < model->num_faces; i++) {
    vec3_t vertex_array[8];
    vec2_tx_t txcoord_array[8];
    int face_index = model->face_index.data[i];
    int tx_face_index = model->tx_face_index.data[i];
    
    for (int j = 0; j < model->face_num_vertices.data[i]; j++) {
      vertex_array[j] = model->vertices.data[model->faces.data[face_index + j]];
      txcoord_array[j] = model->txcoords.data[model->tx_faces.data[tx_face_index + j]];
    }
    
    list_push_pnt(&pl_list->vertices, &vertex_array);
    list_push_pnt(&pl_list->txcoords, &txcoord_array);
    list_push_int(&pl_list->face_num_vertices, model->face_num_vertices.data[i]);
    pl_list->num_faces = model->num_faces;
  }
}

// transfer the face from the face list to the poly_t struct

void pl_list_set_poly(int face_id, poly_t *poly, pl_list_t *pl_list) {
  poly->num_vertices = pl_list->face_num_vertices.data[face_id];
  
  for (int i = 0; i < poly->num_vertices; i++) {
    poly->vertices[i] = pl_list->vertices.data[face_id * 8 + i];
  }
}

void pl_list_add_face(int face_id, u8 add_new, poly_t *poly, pl_list_t *pl_list, model_t *model) {
  if (!add_new) {
    for (int i = 0; i < poly->num_vertices; i++) {
      pl_list->vertices.data[face_id * 8 + i] = poly->vertices[i];
      pl_list->txcoords.data[face_id * 8 + i] = poly->txcoords[i];
    }
    
    pl_list->face_num_vertices.data[face_id] = poly->num_vertices;
  } else {
    list_push_pnt(&pl_list->vertices, &poly->vertices);
    list_push_pnt(&pl_list->txcoords, &poly->txcoords);
    list_push_int(&pl_list->face_num_vertices, poly->num_vertices);
    
    list_malloc_inc(&model->face_index);
    list_malloc_inc(&model->tx_face_index);
    list_malloc_inc(&model->face_num_vertices);
    list_push_pnt(&model->normals, &model->normals.data[face_id]);
    list_push_int(&model->face_materials, model->face_materials.data[face_id]);
    list_push_int(&model->face_types, model->face_types.data[face_id]);
    list_push_int(&model->sprite_face_index, -1);
    
    pl_list->num_faces++;
  }
}

// removes any marked faces for removal

void pl_list_remove_marked_faces(pl_list_t *pl_list, model_t *model) {
  int num_faces = 0;
  int faces_size = 0;
  
  for (int i = 0; i < pl_list->num_faces; i++) {
    if (removed_faces[i]) continue;
    
    for (int j = 0; j < pl_list->face_num_vertices.data[i]; j++) {
      pl_list->vertices.data[num_faces * 8 + j] = pl_list->vertices.data[i * 8 + j];
      pl_list->txcoords.data[num_faces * 8 + j] = pl_list->txcoords.data[i * 8 + j];
      faces_size++;
    }
    
    pl_list->face_num_vertices.data[num_faces] = pl_list->face_num_vertices.data[i];
    
    // update the model face data
    model->normals.data[num_faces] = model->normals.data[i];
    model->face_materials.data[num_faces] = model->face_materials.data[i];
    model->face_types.data[num_faces] = model->face_types.data[i];
    if (model->num_sprites) {
      model->sprite_face_index.data[num_faces] = model->sprite_face_index.data[i];
    }
    
    num_faces++;
  }
  
  pl_list->num_faces = num_faces;
  pl_list->vertices.size = num_faces;
  pl_list->txcoords.size = num_faces;
  pl_list->face_num_vertices.size = num_faces;
  
  model->num_faces = pl_list->num_faces;
  model->num_tx_faces = model->num_faces;
  model->faces_size = faces_size;
  model->tx_faces_size = faces_size;
  model->num_vertices = faces_size;
  model->num_txcoords = faces_size;
  
  // update the list sizes
  
  model->faces.size = faces_size;
  model->tx_faces.size = faces_size;
  model->face_index.size = num_faces;
  model->tx_face_index.size = num_faces;
  model->normals.size = num_faces;
  model->face_num_vertices.size = num_faces;
  model->face_materials.size = num_faces;
  model->face_types.size = num_faces;
  model->sprite_face_index.size = num_faces;
}

// copies the faces data back to the main model

void set_model_from_pl_list(pl_list_t *pl_list, model_t *model) {
  // int num_faces = 0;
  int faces_size = 0;
  model->vertices.size = 0;
  model->txcoords.size = 0;
  model->faces.size = 0;
  model->tx_faces.size = 0;
  
  for (int i = 0; i < pl_list->num_faces; i++) {
    // rebuild the face index
    model->face_index.data[i] = faces_size;
    model->tx_face_index.data[i] = faces_size;
    
    for (int j = 0; j < pl_list->face_num_vertices.data[i]; j++) {
      list_push_pnt(&model->vertices, &pl_list->vertices.data[i * 8 + j]);
      list_push_pnt(&model->txcoords, &pl_list->txcoords.data[i * 8 + j]);
      list_push_int(&model->faces, faces_size);
      list_push_int(&model->tx_faces, faces_size);
      faces_size++;
    }
    
    model->face_num_vertices.data[i] = pl_list->face_num_vertices.data[i];
  }
  
  model->num_faces = pl_list->num_faces;
  model->num_tx_faces = model->num_faces;
  model->faces_size = faces_size;
  model->tx_faces_size = faces_size;
  model->num_vertices = faces_size;
  model->num_txcoords = faces_size;
}