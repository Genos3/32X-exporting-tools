#include "common.h"

void set_face_index(model_t *model) {
  int n = 0;
  for (int i = 0; i < model->num_faces; i++) {
    model->face_index.data[i] = n;
    if (!model->num_sprites || !(model->face_types.data[i] & SPRITE)) {
      n += model->face_num_vertices.data[i];
    } else {
      n++;
    }
  }
}

void set_tx_face_index(model_t *model) {
  int n = 0;
  for (int i = 0; i < model->num_tx_faces; i++) {
    // if (!(model->face_types.data[i] & UNTEXTURED)) {
    model->tx_face_index.data[i] = n;
    n += model->face_num_vertices.data[i];
    // } else {
    //   model->tx_face_index.data[i] = 0;
    // }
  }
}

void reset_sprite_type(model_t *model) {
  for (int i = 0; i < model->num_faces; i++) {
    if (model->face_types.data[i] & SPRITE) {
      model->face_types.data[i] &= ~SPRITE;
    }
  }
}

// separate the texture coordinates

void expand_txcoords(model_t *model) {
  u8 *seen_txcoord = calloc(model->num_txcoords, 1);
  
  for (int i = 0; i < model->num_tx_faces; i++) {
    for (int j = 0; j < model->face_num_vertices.data[i]; j++) {
      int face_index = model->tx_face_index.data[i] + j;
      int tx_pt = model->tx_faces.data[face_index];
      
      if (!seen_txcoord[tx_pt]) {
        seen_txcoord[tx_pt] = 1;
      } else {
        // create a new texture coordinate
        
        model->tx_faces.data[face_index] = model->num_txcoords;
        
        list_push_pnt(&model->txcoords, &model->txcoords.data[tx_pt]);
        model->num_txcoords++;
      }
    }
  }
  
  free(seen_txcoord);
}

// set the coordinates at 0 and allows to export only unsigned numbers
// requires unmerged texture coordinates

void make_txcoords_positive(model_t *model) {
  for (int i = 0; i < model->num_tx_faces; i++) {
    vec2_tx_t min_vt;
    int tx_pt = model->tx_faces.data[model->tx_face_index.data[i]];
    min_vt.u = model->txcoords.data[tx_pt].u;
    min_vt.v = model->txcoords.data[tx_pt].v;
    
    for (int j = 1; j < model->face_num_vertices.data[i]; j++) {
      tx_pt = model->tx_faces.data[model->tx_face_index.data[i] + j];
      
      if (model->txcoords.data[tx_pt].u < min_vt.u) min_vt.u = model->txcoords.data[tx_pt].u;
      if (model->txcoords.data[tx_pt].v < min_vt.v) min_vt.v = model->txcoords.data[tx_pt].v;
    }
    
    if (min_vt.u || min_vt.v) {
      for (int j = 0; j < model->face_num_vertices.data[i]; j++) {
        tx_pt = model->tx_faces.data[model->tx_face_index.data[i] + j];
        
        model->txcoords.data[tx_pt].u -= floorf(min_vt.u);
        model->txcoords.data[tx_pt].v -= floorf(min_vt.v);
      }
    }
  }
}

// multiply the texture coordinates by the texture size
// requires unmerged texture coordinates

void resize_txcoords(model_t *model, textures_t *textures) {
  u8 *txcoords_set = calloc(model->num_txcoords, 1);
  
  for (int i = 0; i < model->num_tx_faces; i++) {
    for (int j = 0; j < model->face_num_vertices.data[i]; j++) {
      int tx_pt = model->tx_faces.data[model->tx_face_index.data[i] + j];
      
      if (!txcoords_set[tx_pt]) {
        model->txcoords.data[tx_pt].u *= textures->texture_sizes.data[model->mtl_textures.data[model->face_materials.data[i]]].w;
        model->txcoords.data[tx_pt].v *= textures->texture_sizes.data[model->mtl_textures.data[model->face_materials.data[i]]].h;
        
        if (ini.make_fp && ini.fp_size == 8) {
          if (model->txcoords.data[tx_pt].u >= 65536) {
            model->txcoords.data[tx_pt].u = 65535;
          }
          if (model->txcoords.data[tx_pt].v >= 65536) {
            model->txcoords.data[tx_pt].v = 65535;
          }
        }
        
        txcoords_set[tx_pt] = 1;
      }
    }
  }
  
  free(txcoords_set);
}

// sets the objects origin and size

void set_objects_size_vt(model_t *model) {
  // model->objects_origin = malloc(model->num_objects * sizeof(vec3_t));
  // model->objects_size = malloc(model->num_objects * sizeof(size3_t));
  
  for (int i = 0; i < model->num_objects; i++) {
    aabb_t aabb;
    int vt_id = model->object_vt_index.data[i];
    
    aabb.min.x = model->vertices.data[vt_id].x;
    aabb.min.y = model->vertices.data[vt_id].y;
    aabb.min.z = model->vertices.data[vt_id].z;
    aabb.max.x = model->vertices.data[vt_id].x;
    aabb.max.y = model->vertices.data[vt_id].y;
    aabb.max.z = model->vertices.data[vt_id].z;
    vt_id++;
    
    for (int j = 1; j < model->object_num_vertices.data[i]; j++) {
      aabb.min.x = min_c(aabb.min.x, model->vertices.data[vt_id].x);
      aabb.min.y = min_c(aabb.min.y, model->vertices.data[vt_id].y);
      aabb.min.z = min_c(aabb.min.z, model->vertices.data[vt_id].z);
      aabb.max.x = max_c(aabb.max.x, model->vertices.data[vt_id].x);
      aabb.max.y = max_c(aabb.max.y, model->vertices.data[vt_id].y);
      aabb.max.z = max_c(aabb.max.z, model->vertices.data[vt_id].z);
      vt_id++;
    }
    
    // model->objects_origin[i].x = aabb.min.x;
    // model->objects_origin[i].y = aabb.min.y;
    // model->objects_origin[i].z = aabb.min.z;
    // model->objects_size[i].w = aabb.max.x - aabb.min.x;
    // model->objects_size[i].h = aabb.max.y - aabb.min.y;
    // model->objects_size[i].d = aabb.max.z - aabb.min.z;
    size3_t aabb_size;
    aabb_size.w = aabb.max.x - aabb.min.x;
    aabb_size.h = aabb.max.y - aabb.min.y;
    aabb_size.d = aabb.max.z - aabb.min.z;
    
    list_push_pnt(&model->objects_origin, &aabb);
    list_push_pnt(&model->objects_size, &aabb_size);
  }
}

// legacy code

/* void set_objects_size_face(model_t *model) {
  model->objects_origin = malloc(model->num_objects * sizeof(vec3_t));
  model->objects_size = malloc(model->num_objects * sizeof(size3_t));
  u8 *vertices_set = calloc(model->num_vertices, 1);
  
  for (int i = 0; i < model->num_objects; i++) {
    aabb_t aabb;
    int pt = model->faces.data[model->face_index.data[model->object_face_index.data[i]]];
    
    aabb.min.x = model->vertices.data[pt].x;
    aabb.min.y = model->vertices.data[pt].y;
    aabb.min.z = model->vertices.data[pt].z;
    aabb.max.x = model->vertices.data[pt].x;
    aabb.max.y = model->vertices.data[pt].y;
    aabb.max.z = model->vertices.data[pt].z;
    
    for (int j = 0; j < model->object_num_faces.data[i]; j++) {
      pt = model->object_face_index.data[i] + j;
      
      for (int k = 0; k < model->face_num_vertices.data[pt]; k++) {
        int pt2 = model->faces.data[model->face_index.data[pt] + k];
        
        if (!vertices_set[pt2]) {
          aabb.min.x = min_c(aabb.min.x, model->vertices.data[pt2].x);
          aabb.min.y = min_c(aabb.min.y, model->vertices.data[pt2].y);
          aabb.min.z = min_c(aabb.min.z, model->vertices.data[pt2].z);
          aabb.max.x = max_c(aabb.max.x, model->vertices.data[pt2].x);
          aabb.max.y = max_c(aabb.max.y, model->vertices.data[pt2].y);
          aabb.max.z = max_c(aabb.max.z, model->vertices.data[pt2].z);
          
          vertices_set[pt2] = 1;
        }
      }
    }
    
    model->objects_origin[i].x = aabb.min.x;
    model->objects_origin[i].y = aabb.min.y;
    model->objects_origin[i].z = aabb.min.z;
    model->objects_size[i].w = aabb.max.x - aabb.min.x;
    model->objects_size[i].h = aabb.max.y - aabb.min.y;
    model->objects_size[i].d = aabb.max.z - aabb.min.z;
  }
  
  free(vertices_set);
} */

// move the objects in the model to 0,0

void join_mdl_objects(model_t *model) {
  u8 *vertices_set = calloc(model->num_vertices, 1);
  
  for (int i = 0; i < model->num_objects; i++) {
    float offset_x = model->objects_origin.data[i].x; // - fmodf(min.x, map_scn.tile_size_f);
    float offset_y = model->objects_origin.data[i].y;
    float offset_z = model->objects_origin.data[i].z; // - fmodf(min.z, map_scn.tile_size_f);
    
    if (ini.objects_center_xz_origin) {
      offset_x += model->objects_size.data[i].w / 2;
      offset_z += model->objects_size.data[i].d / 2;
    }
    
    if (ini.objects_center_y_origin) {
      offset_y += model->objects_size.data[i].h / 2;
    }
    
    for (int j = 0; j < model->object_num_faces.data[i]; j++) {
      int pt = model->object_face_index.data[i] + j;
      
      for (int k = 0; k < model->face_num_vertices.data[pt]; k++) {
        int pt2 = model->faces.data[model->face_index.data[pt] + k];
        
        if (!vertices_set[pt2]) {
          model->vertices.data[pt2].x -= offset_x;
          model->vertices.data[pt2].z -= offset_z;
          
          if (ini.objects_center_y_origin) {
            model->vertices.data[pt2].y -= offset_y;
          }
          
          vertices_set[pt2] = 1;
        }
      }
    }
    
    model->objects_origin.data[i].x -= offset_x;
    model->objects_origin.data[i].z -= offset_z;
    
    if (ini.objects_center_y_origin) {
      model->objects_origin.data[i].y -= offset_y;
    }
  }
  
  free(vertices_set);
}

// requires non merged vertices

void make_sprites(model_t *model) {
  u8 *removed_faces_vt = calloc(model->faces_size, 1);
  
  for (int i = 0; i < model->num_faces; i++) {
    if (!(model->face_types.data[i] & SPRITE)) continue;
    
    aabb_t aabb;
    poly_t poly;
    
    set_poly(i, &poly, model);
    make_poly_aabb(&aabb, &poly);
    
    // calculate the midpoint
    float mid_x = aabb.min.x + (aabb.max.x - aabb.min.x) / 2;
    float mid_y = aabb.min.y + (aabb.max.y - aabb.min.y) / 2;
    float mid_z = aabb.min.z + (aabb.max.z - aabb.min.z) / 2;
    
    // creates the sprite vertices
    int face_index = model->face_index.data[i];
    for (int j = 0; j < model->face_num_vertices.data[i]; j++) {
      vec3_t vt;
      vt.x = model->vertices.data[model->faces.data[face_index + j]].x - mid_x;
      vt.y = mid_y - model->vertices.data[model->faces.data[face_index + j]].y; // invert y
      vt.z = model->vertices.data[model->faces.data[face_index + j]].z - mid_z;
      
      int repeated_sprite_vt = 0;
      
      for (int k = 0; k < model->num_sprite_vertices; k++) {
        if (vt.x == model->sprite_vertices.data[k].x &&
            vt.y == model->sprite_vertices.data[k].y &&
            vt.z == model->sprite_vertices.data[k].z) {
          list_push_int(&model->sprite_faces, k);
          repeated_sprite_vt = 1;
          break;
        }
      }
      
      if (!repeated_sprite_vt) {
        list_push_pnt(&model->sprite_vertices, &vt);
        list_push_int(&model->sprite_faces, model->num_sprite_vertices);
        model->num_sprite_vertices++;
      }
    }
    
    // store the midpoint in the first vertex
    model->vertices.data[model->faces.data[face_index]].x = mid_x;
    model->vertices.data[model->faces.data[face_index]].y = mid_y;
    model->vertices.data[model->faces.data[face_index]].z = mid_z;
    model->sprite_face_index.data[i] = model->num_sprites * 4;
    model->num_sprites++;
    
    // move the last vertices to the removed spaces
    for (int j = 1; j < model->face_num_vertices.data[i]; j++) {
      model->num_vertices--;
      model->vertices.size--;
      model->vertices.data[model->faces.data[face_index + j]].x = model->vertices.data[model->num_vertices].x;
      model->vertices.data[model->faces.data[face_index + j]].y = model->vertices.data[model->num_vertices].y;
      model->vertices.data[model->faces.data[face_index + j]].z = model->vertices.data[model->num_vertices].z;
      
      // update the faces
      for (int k = 0; k < model->faces_size; k++) {
        if (model->faces.data[k] == model->num_vertices) {
          model->faces.data[k] = model->faces.data[face_index + j];
        }
      }
      
      removed_faces_vt[face_index + j] = 1;
    }
  }
  
  if (!model->num_sprites) {
    free(removed_faces_vt);
    return;
  }
  
  // remove the empty spaces
  int final_faces_size = 0;
  
  for (int i = 0; i < model->faces_size; i++) {
    if (removed_faces_vt[i]) continue;
    
    if (i != final_faces_size) {
      model->faces.data[final_faces_size] = model->faces.data[i];
    }
    
    final_faces_size++;
  }
  
  model->faces_size = final_faces_size;
  model->faces.size = final_faces_size;
  
  // reset the face index
  set_face_index(model);
  
  free(removed_faces_vt);
}

// mark which vertices belong to sprites

void set_type_vt(model_t *model) {
  model->type_vt = calloc(model->num_vertices, 1);
  
  for (int i = 0; i < model->num_faces; i++) {
    if (model->face_types.data[i] & (SPRITE | BACKFACE)) {
      for (int j = 0; j < model->face_num_vertices.data[i]; j++) {
        int pt_vt = model->faces.data[model->face_index.data[i] + j];
        model->type_vt[pt_vt] = model->face_types.data[i];
      }
    }
  }
}

// requires resized texture coordinates to the size of the textures

void merge_mdl_txcoords(model_t *model) {
  vec2_tx_t *snapped_txcoords = malloc(model->num_txcoords * sizeof(vec2_tx_t));
  float merge_uv_dist = 32 / ini.merge_vt_dist_f;
  
  for (int i = 0; i < model->num_txcoords; i++) {
    snapped_txcoords[i].u = (int)(model->txcoords.data[i].u * merge_uv_dist);
    snapped_txcoords[i].v = (int)(model->txcoords.data[i].v * merge_uv_dist);
  }
  
  int final_num_vt = 0;
  for (int i = 0; i < model->num_txcoords; i++) {
    vec2_tx_t vt = snapped_txcoords[i];
    
    for (int j = 0; j < final_num_vt; j++) {
      vec2_tx_t vt2 = snapped_txcoords[j];
      
      //merge the vertices by updating the faces
      if (vt.u == vt2.u && vt.v == vt2.v) {
        for (int k = 0; k < model->tx_faces_size; k++) {
          if (model->tx_faces.data[k] == i) {
            model->tx_faces.data[k] = j;
          }
        }
        goto out;
      }
    }
    
    // add the vertex to the final list
    if (i != final_num_vt) {
      model->txcoords.data[final_num_vt].u = model->txcoords.data[i].u;
      model->txcoords.data[final_num_vt].v = model->txcoords.data[i].v;
      snapped_txcoords[final_num_vt] = snapped_txcoords[i];
      
      // update the faces
      for (int k = 0; k < model->tx_faces_size; k++) {
        if (model->tx_faces.data[k] == i) {
          model->tx_faces.data[k] = final_num_vt;
        }
      }
    }
    final_num_vt++;
    out:;
  }
  
  free(snapped_txcoords);
  model->num_txcoords = final_num_vt;
  model->txcoords.size = final_num_vt;
}

void merge_mdl_sprite_vertices(model_t *model) {
  vec3_t *snapped_vertices = malloc(model->num_sprite_vertices * sizeof(vec3_t));
  
  for (int i = 0; i < model->num_sprite_vertices; i++) {
    snapped_vertices[i].x = (int)(model->sprite_vertices.data[i].x / ini.merge_vt_dist_f);
    snapped_vertices[i].y = (int)(model->sprite_vertices.data[i].y / ini.merge_vt_dist_f);
    snapped_vertices[i].z = (int)(model->sprite_vertices.data[i].z / ini.merge_vt_dist_f);
  }
  
  int final_num_vt = 0;
  for (int i = 0; i < model->num_sprite_vertices; i++) {
    vec3_t vt = snapped_vertices[i];
    
    for (int j = 0; j < final_num_vt; j++) {
      vec3_t vt2 = snapped_vertices[j];
      
      // merge the vertices by updating the faces
      if (vt.x == vt2.x && vt.y == vt2.y && vt.z == vt2.z) {
        for (int k = 0; k < model->num_faces; k++) {
          if (model->face_types.data[k] & SPRITE) {
            for (int l = 0; l < model->face_num_vertices.data[k]; l++) {
              if (model->sprite_faces.data[model->sprite_face_index.data[k] + l] == i) {
                model->sprite_faces.data[model->sprite_face_index.data[k] + l] = j;
              }
            }
          }
        }
        goto out;
      }
    }
    
    //remove vertices
    if (i != final_num_vt) {
      model->sprite_vertices.data[final_num_vt].x = model->sprite_vertices.data[i].x;
      model->sprite_vertices.data[final_num_vt].y = model->sprite_vertices.data[i].y;
      model->sprite_vertices.data[final_num_vt].z = model->sprite_vertices.data[i].z;
      snapped_vertices[final_num_vt] = snapped_vertices[i];
      
      // update the faces
      for (int j = 0; j < model->num_faces; j++) {
        if (model->face_types.data[j] & SPRITE) {
          for (int k = 0; k < model->face_num_vertices.data[j]; k++) {
            if (model->sprite_faces.data[model->sprite_face_index.data[j] + k] == i) {
              model->sprite_faces.data[model->sprite_face_index.data[j] + k] = final_num_vt;
            }
          }
        }
      }
    }
    final_num_vt++;
    out:;
  }
  
  free(snapped_vertices);
  model->num_sprite_vertices = final_num_vt;
  model->sprite_vertices.size = final_num_vt;
}

// subdivides the faces into quads and triangles

void subdivide_faces_into_quads(pl_list_t *pl_list, model_t *model) {
  for (int i = 0; i < pl_list->num_faces; i++) {
    if (pl_list->face_num_vertices.data[i] > 4) {
      poly_t sub_poly, poly;
      
      sub_poly.num_vertices = pl_list->face_num_vertices.data[i];
      
      for (int j = 0; j < sub_poly.num_vertices; j++) {
        sub_poly.vertices[j] = pl_list->vertices.data[i * 8 + j];
        sub_poly.txcoords[j] = pl_list->txcoords.data[i * 8 + j];
      }
      
      poly.vertices[0] = sub_poly.vertices[0];
      poly.txcoords[0] = sub_poly.txcoords[0];
      
      for (int j = 0; j < sub_poly.num_vertices - 3; j += 2) {
        poly.num_vertices = 4;
        
        poly.vertices[1] = sub_poly.vertices[j + 1];
        poly.vertices[2] = sub_poly.vertices[j + 2];
        poly.vertices[3] = sub_poly.vertices[j + 3];
        poly.txcoords[1] = sub_poly.txcoords[j + 1];
        poly.txcoords[2] = sub_poly.txcoords[j + 2];
        poly.txcoords[3] = sub_poly.txcoords[j + 3];
        
        if (!j) {
          pl_list_add_face(i, 0, &poly, pl_list, model);
        } else {
          pl_list_add_face(i, 1, &poly, pl_list, model);
        }
      }
      
      if (sub_poly.num_vertices & 1) {
        poly.num_vertices = 3;
        
        poly.vertices[1] = sub_poly.vertices[sub_poly.num_vertices - 2];
        poly.vertices[2] = sub_poly.vertices[sub_poly.num_vertices - 1];
        poly.txcoords[1] = sub_poly.txcoords[sub_poly.num_vertices - 2];
        poly.txcoords[2] = sub_poly.txcoords[sub_poly.num_vertices - 1];
        
        pl_list_add_face(i, 1, &poly, pl_list, model);
      }
    }
  }
}

void set_model_size(model_t *model) {
  aabb_t aabb;
  make_model_aabb(&aabb, model);
  model->origin.x = aabb.min.x;
  model->origin.y = aabb.min.y;
  model->origin.z = aabb.min.z;
  model->size.w = aabb.max.x - aabb.min.x;
  model->size.h = aabb.max.y - aabb.min.y;
  model->size.d = aabb.max.z - aabb.min.z;
}

void convert_to_fixed(model_t *model) {
  for (int i = 0; i < model->num_vertices; i++) {
    model->vertices.data[i].x = (int)(model->vertices.data[i].x * fp_size_i);
    model->vertices.data[i].y = (int)(model->vertices.data[i].y * fp_size_i);
    model->vertices.data[i].z = (int)(model->vertices.data[i].z * fp_size_i);
  }
  
  for (int i = 0; i < model->num_faces; i++) {
    model->normals.data[i].x = (int)(model->normals.data[i].x * fp_size_i);
    model->normals.data[i].y = (int)(model->normals.data[i].y * fp_size_i);
    model->normals.data[i].z = (int)(model->normals.data[i].z * fp_size_i);
  }
  
  for (int i = 0; i < model->num_txcoords; i++) {
    model->txcoords.data[i].u = (int)(model->txcoords.data[i].u * fp_size_i);
    model->txcoords.data[i].v = (int)(model->txcoords.data[i].v * fp_size_i);
  }
  
  if (model->num_sprites) {
    for (int i = 0; i < model->num_sprite_vertices; i++) {
      model->sprite_vertices.data[i].x = (int)(model->sprite_vertices.data[i].x * fp_size_i);
      model->sprite_vertices.data[i].y = (int)(model->sprite_vertices.data[i].y * fp_size_i);
      model->sprite_vertices.data[i].z = (int)(model->sprite_vertices.data[i].z * fp_size_i);
    }
  }
  
  if (model->num_objects) {
    for (int i = 0; i < model->num_objects; i++) {
      model->objects_origin.data[i].x = (int)(model->objects_origin.data[i].x * fp_size_i);
      model->objects_origin.data[i].y = (int)(model->objects_origin.data[i].y * fp_size_i);
      model->objects_origin.data[i].z = (int)(model->objects_origin.data[i].z * fp_size_i);
      model->objects_size.data[i].w = (int)(model->objects_size.data[i].w * fp_size_i);
      model->objects_size.data[i].d = (int)(model->objects_size.data[i].d * fp_size_i);
      model->objects_size.data[i].h = (int)(model->objects_size.data[i].h * fp_size_i);
    }
  }
  
  /* if (ini.make_grid) {
    for (int i = 0; i < grid_scn_ln.grid_aabb.size; i++) {
      grid_scn_ln.grid_aabb.data[i].min.x = (int)(grid_scn_ln.grid_aabb.data[i].min.x * fp_size_i);
      grid_scn_ln.grid_aabb.data[i].min.y = (int)(grid_scn_ln.grid_aabb.data[i].min.y * fp_size_i);
      grid_scn_ln.grid_aabb.data[i].min.z = (int)(grid_scn_ln.grid_aabb.data[i].min.z * fp_size_i);
      grid_scn_ln.grid_aabb.data[i].max.x = (int)(grid_scn_ln.grid_aabb.data[i].max.x * fp_size_i);
      grid_scn_ln.grid_aabb.data[i].max.y = (int)(grid_scn_ln.grid_aabb.data[i].max.y * fp_size_i);
      grid_scn_ln.grid_aabb.data[i].max.z = (int)(grid_scn_ln.grid_aabb.data[i].max.z * fp_size_i);
    }
    
    grid_scn_ln.tile_size_i = (int)(grid_scn_ln.tile_size_i * fp_size_i);
  } */
  
  model->origin.x *= fp_size_i;
  model->origin.y *= fp_size_i;
  model->origin.z *= fp_size_i;
  model->size.w *= fp_size_i;
  model->size.h *= fp_size_i;
  model->size.d *= fp_size_i;
}

void adjust_txcoords_max_size(model_t *model) {
  for (int i = 0; i < model->num_txcoords; i++) {
    if (model->txcoords.data[i].u && !((int)model->txcoords.data[i].u & (fp_size_i - 1))) {
      model->txcoords.data[i].u--;
    }
    
    if (model->txcoords.data[i].v && !((int)model->txcoords.data[i].v & (fp_size_i - 1))) {
      model->txcoords.data[i].v--;
    }
  }
}