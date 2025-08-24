#include "common.h"

void set_objects_size(model_t *model) {
  for (int i = 0; i < model->num_objects; i++) {
    object_t *object = &model->objects.data[i];
    
    aabb_t aabb;
    make_object_aabb(&aabb, object);
    
    object->origin.x = aabb.min.x;
    object->origin.y = aabb.min.y;
    object->origin.z = aabb.min.z;
    object->size.w = aabb.max.x - aabb.min.x;
    object->size.h = aabb.max.y - aabb.min.y;
    object->size.d = aabb.max.z - aabb.min.z;
  }
}

// sort the objects according to how they are layed out in the model

void sort_objects(model_t *model) {
  u8 *visited_elements = calloc(model->num_objects, 1);
  
  for (int i = 0; i < model->num_objects; i++) {
    int n = 0;
    while (visited_elements[n]) n++;
    
    float obj_x = model->objects.data[n].origin.x;
    float obj_z = model->objects.data[n].origin.z;
    int obj_id = n;
    
    // get the first object in the list
    for (int j = n; j < model->num_objects; j++) {
      if (visited_elements[j]) continue;
      
      if (model->objects.data[j].origin.z < obj_z || (model->objects.data[j].origin.z == obj_z && model->objects.data[j].origin.x < obj_x)) {
        obj_x = model->objects.data[j].origin.x;
        obj_z = model->objects.data[j].origin.z;
        obj_id = j;
      }
    }
    
    model->objects_id.data[i] = obj_id;
    visited_elements[obj_id] = 1;
  }
  
  free(visited_elements);
}

// move the objects in the model to 0,0

void center_objects(model_t *model) {
  for (int i = 0; i < model->num_objects; i++) {
    object_t *object = &model->objects.data[i];
    
    float offset_x = object->origin.x; // - fmodf(min.x, map_scn.tile_size_f);
    float offset_y = object->origin.y;
    float offset_z = object->origin.z; // - fmodf(min.z, map_scn.tile_size_f);
    
    if (ini.objects_center_xz_origin) {
      offset_x += object->size.w / 2;
      offset_z += object->size.d / 2;
    }
    
    if (ini.objects_center_y_origin) {
      offset_y += object->size.h / 2;
    }
    
    for (int j = 0; j < object->num_vertices; j++) {
      object->vertices.data[i].x -= offset_x;
      object->vertices.data[i].z -= offset_z;
      
      if (ini.objects_center_y_origin) {
        object->vertices.data[i].y -= offset_y;
      }
    }
    
    object->origin.x -= offset_x;
    object->origin.z -= offset_z;
    
    if (ini.objects_center_y_origin) {
      object->origin.y -= offset_y;
    }
  }
}

// requires non merged vertices

void make_sprites(object_t *object) {
  int sprite_vt_index[8];
  
  for (int i = 0; i < object->num_faces; i++) {
    if (!(object->faces.data[i].type & SPRITE)) continue;
    
    aabb_t aabb;
    poly_t poly;
    
    set_poly_from_obj_vertices(i, &poly, object);
    make_poly_aabb(&aabb, &poly);
    
    // calculate the midpoint
    vec3_t mid_vt;
    mid_vt.x = aabb.min.x + (aabb.max.x - aabb.min.x) / 2;
    mid_vt.y = aabb.min.y + (aabb.max.y - aabb.min.y) / 2;
    mid_vt.z = aabb.min.z + (aabb.max.z - aabb.min.z) / 2;
    
    face_t *face = &object->faces.data[i];
    
    // creates the sprite vertices
    for (int j = 0; j < face->num_vertices; j++) {
      vec3_t vt;
      vt.x = object->vertices.data[face->vt_index[j]].x - mid_vt.x;
      vt.y = object->vertices.data[face->vt_index[j]].y - mid_vt.y;
      vt.z = object->vertices.data[face->vt_index[j]].z - mid_vt.z;
      
      int repeated_sprite_vt = 0;
      
      for (int k = 0; k < object->num_sprite_vertices; k++) {
        if (vt.x == object->sprite_vertices.data[k].x &&
            vt.y == object->sprite_vertices.data[k].y &&
            vt.z == object->sprite_vertices.data[k].z) {
          sprite_vt_index[j] = k;
          repeated_sprite_vt = 1;
          break;
        }
      }
      
      if (!repeated_sprite_vt) {
        list_push_pnt(&object->sprite_vertices, &vt);
        sprite_vt_index[j] = object->num_sprite_vertices;
        object->num_sprite_vertices++;
      }
    }
    
    for (int j = 0; j < face->num_vertices; j++) {
      face->vt_index[j] = sprite_vt_index[j];
    }
    
    face->vertices[0] = mid_vt;
    face->num_vertices = 1;
    object->num_sprites++;
  }
}

// mark which vertices belong to sprites

#if 0
  void set_type_vt(object_t *object) {
    object->type_vt = calloc(object->num_vertices, 1);
    
    for (int i = 0; i < object->num_faces; i++) {
      if (object->faces.data[i].type & (SPRITE | BACKFACE)) {
        for (int j = 0; j < object->faces.data[i].num_vertices; j++) {
          int pt_vt = object->faces.data[object->face_index.data[i] + j];
          object->type_vt[pt_vt] = object->faces.data[i].type;
        }
      }
    }
  }
#endif

// set the coordinates at 0 and allows to export only unsigned numbers
// requires the face texture coordinates list

void make_txcoords_positive(object_t *object) {
  for (int i = 0; i < object->num_faces; i++) {
    vec2_tx_t min_vt;
    min_vt.u = object->faces.data[i].txcoords[0].u;
    min_vt.v = object->faces.data[i].txcoords[0].v;
    
    for (int j = 1; j < object->faces.data[i].num_vertices; j++) {
      if (object->faces.data[i].txcoords[j].u < min_vt.u) min_vt.u = object->faces.data[i].txcoords[j].u;
      if (object->faces.data[i].txcoords[j].v < min_vt.v) min_vt.v = object->faces.data[i].txcoords[j].v;
    }
    
    if (min_vt.u || min_vt.v) {
      for (int j = 0; j < object->faces.data[i].num_vertices; j++) {
        object->faces.data[i].txcoords[j].u -= floorf(min_vt.u);
        object->faces.data[i].txcoords[j].v -= floorf(min_vt.v);
      }
    }
  }
}

// multiply the texture coordinates by the texture size
// requires the face texture coordinates list

void resize_txcoords(object_t *object, textures_t *textures) {
  for (int i = 0; i < object->num_faces; i++) {
    tx_group_t *tx_group = &textures->tx_group.data[object->faces.data[i].texture_id];
    
    for (int j = 0; j < object->faces.data[i].num_vertices; j++) {
      object->faces.data[i].txcoords[j].u *= tx_group->size.w;
      object->faces.data[i].txcoords[j].v *= tx_group->size.h;
      
      if (ini.make_fp && ini.fp_size == 8) {
        if (object->faces.data[i].txcoords[j].u >= 65536) {
          object->faces.data[i].txcoords[j].u = 65535;
        }
        if (object->faces.data[i].txcoords[j].v >= 65536) {
          object->faces.data[i].txcoords[j].v = 65535;
        }
      }
    }
  }
}

// requires resized texture coordinates to the size of the textures

void merge_mdl_txcoords(object_t *object) {
  vec2_tx_t *snapped_txcoords = malloc(object->num_txcoords * sizeof(vec2_tx_t));
  
  for (int i = 0; i < object->num_txcoords; i++) {
    snapped_txcoords[i].u = (int)(object->txcoords.data[i].u / ini.merge_vt_dist_f);
    snapped_txcoords[i].v = (int)(object->txcoords.data[i].v / ini.merge_vt_dist_f);
  }
  
  int num_txcoords = 0;
  
  for (int i = 0; i < object->num_txcoords; i++) {
    vec2_tx_t vt = snapped_txcoords[i];
    
    for (int j = 0; j < num_txcoords; j++) {
      vec2_tx_t vt2 = snapped_txcoords[j];
      
      // update the faces
      if (vt.u == vt2.u && vt.v == vt2.v) {
        for (int k = 0; k < object->num_faces; k++) {
          face_t *face = &object->faces.data[k];
          
          for (int l = 0; l < face->num_vertices; l++) {
            if (face->tx_vt_index[l] == k) {
              face->tx_vt_index[l] = num_txcoords;
            }
          }
        }
        
        goto out;
      }
    }
    
    // add the vertex to the final list
    if (i != num_txcoords) {
      object->txcoords.data[num_txcoords].u = object->txcoords.data[i].u;
      object->txcoords.data[num_txcoords].v = object->txcoords.data[i].v;
      snapped_txcoords[num_txcoords] = snapped_txcoords[i];
      
      // update the faces
      for (int j = 0; j < object->num_faces; j++) {
        face_t *face = &object->faces.data[j];
        
        for (int k = 0; k < face->num_vertices; k++) {
          if (face->tx_vt_index[k] == i) {
            face->tx_vt_index[k] = num_txcoords;
          }
        }
      }
    }
    
    num_txcoords++;
    out:;
  }
  
  free(snapped_txcoords);
  object->num_txcoords = num_txcoords;
  object->txcoords.size = num_txcoords;
}

// subdivides the faces into quads and triangles
// requires the face vertex list

void subdivide_faces_into_quads(object_t *object) {
  for (int i = 0; i < object->num_faces; i++) {
    if (object->faces.data[i].num_vertices > 4) {
      poly_t sub_poly, poly;
      
      sub_poly.num_vertices = object->faces.data[i].num_vertices;
      
      for (int j = 0; j < sub_poly.num_vertices; j++) {
        sub_poly.vertices[j] = object->faces.data[i].vertices[j];
        sub_poly.txcoords[j] = object->faces.data[i].txcoords[j];
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
          set_obj_face_from_poly(i, &poly, object);
        } else {
          add_obj_face_from_poly(i, &poly, object);
        }
      }
      
      if (sub_poly.num_vertices & 1) {
        poly.num_vertices = 3;
        
        poly.vertices[1] = sub_poly.vertices[sub_poly.num_vertices - 2];
        poly.vertices[2] = sub_poly.vertices[sub_poly.num_vertices - 1];
        poly.txcoords[1] = sub_poly.txcoords[sub_poly.num_vertices - 2];
        poly.txcoords[2] = sub_poly.txcoords[sub_poly.num_vertices - 1];
        
        add_obj_face_from_poly(i, &poly, object);
      }
    }
  }
}

void convert_to_fixed(object_t *object) {
  for (int i = 0; i < object->num_vertices; i++) {
    object->vertices.data[i].x *= fp_size_i;
    object->vertices.data[i].y *= fp_size_i;
    object->vertices.data[i].z *= fp_size_i;
  }
  
  if (object->has_textures) {
    for (int i = 0; i < object->num_txcoords; i++) {
      object->txcoords.data[i].u *= fp_size_i;
      object->txcoords.data[i].v *= fp_size_i;
    }
  }
  
  if (object->num_sprites) {
    for (int i = 0; i < object->num_sprite_vertices; i++) {
      object->sprite_vertices.data[i].x *= fp_size_i;
      object->sprite_vertices.data[i].y *= fp_size_i;
      object->sprite_vertices.data[i].z *= fp_size_i;
    }
  }
  
  for (int i = 0; i < object->num_faces; i++) {
    object->faces.data[i].normal.x *= fp_size_i;
    object->faces.data[i].normal.y *= fp_size_i;
    object->faces.data[i].normal.z *= fp_size_i;
  }
  
  object->origin.x *= fp_size_i;
  object->origin.y *= fp_size_i;
  object->origin.z *= fp_size_i;
  object->size.w *= fp_size_i;
  object->size.h *= fp_size_i;
  object->size.d *= fp_size_i;
}

void adjust_txcoords_max_size(object_t *object) {
  for (int i = 0; i < object->num_txcoords; i++) {
    if (object->txcoords.data[i].u && !((int)object->txcoords.data[i].u & (fp_size_i - 1))) {
      object->txcoords.data[i].u--;
    }
    
    if (object->txcoords.data[i].v && !((int)object->txcoords.data[i].v & (fp_size_i - 1))) {
      object->txcoords.data[i].v--;
    }
  }
}

// origin at x = -1 and z = 0 with clockwise order

void set_face_angles(object_t *object) {
  for (int i = 0; i < object->num_faces; i++) {
    vec3_t vt;
    vt = object->faces.data[i].normal;
    
    float azimuth = atan2f(-vt.z, -vt.x); // -z and -x makes the origin start at the left
    
    if (azimuth < 0.0f) {
      azimuth += 2.0f * PI;
    }
    
    float elevation = asinf(vt.y) + PI / 2;
    
    int azimuth_i = (azimuth / 2 * PI) * (1 << ini.face_angle_bits);
    int elevation_i = (elevation / 2 * PI) * (1 << ini.face_angle_bits);
    
    object->faces.data[i].angle = ((elevation_i - 1) << ini.face_angle_bits) + azimuth_i + 1;
  }
}