#include "common.h"

void set_tile(int y, int z, int x);
void add_face(vec3_t pos, int face_id, object_t *ts_object, object_t *scn_object);

void make_model() {
  for (int i = 0; i < map_size.h; i++) {
    for (int j = 0; j < map_size.d; j++) {
      for (int k = 0; k < map_size.w; k++) {
        set_tile(i, j, k);
      }
    }
  }
  
  // for (int i = 0; i < tileset.num_materials; i++) {
  //   list_push_pnt(&scene.mtl_textures, &tileset.mtl_textures.data[i]);
  // }
  
  scene.num_objects = 1;
  // scene.num_materials = tileset.num_materials;
  scene.has_textures = tileset.has_textures;
}

void set_tile(int y, int z, int x) {
  int map_offset = y * map_size.d * map_size.w + z * map_size.w + x;
  tile_t tile = map[map_offset];
  
  vec3_t pos;
  pos.x = x * tile_size.w;
  pos.y = y * tile_size.h;
  pos.z = z * tile_size.d;
  
  for (int i = 0; i < 3; i++) {
    int tile_id = *((u8*)&tile + i) - 1;
    
    if (tile_id < 0) continue;
    
    object_t *ts_object = &tileset.objects.data[tile_id];
    object_t *scn_object = scene.objects.data;
    
    for (int j = 0; j < object->num_faces; j++) {
      add_face(pos, j, ts_object, scn_object);
    }
  }
}

void add_face(vec3_t pos, int face_id, object_t *ts_object, object_t *scn_object) {
  size3_t half_tile_size;
  half_tile_size.w = ts_object->size.w / 2;
  half_tile_size.d = ts_object->size.d / 2;
  
  face_t *ts_face = &ts_object->face.data[face_id];
  face_t *scn_face = &scn_object->face.data[face_id];
  
  for (int i = 0; i < ts_face->num_vertices; i++) {
    int pt_vt = ts_face->vt_index[i];
    vec3_t vt;
    vt.x = pos.x + ts_object->vertices.data[pt_vt].x + half_tile_size.w;
    vt.y = pos.y + ts_object->vertices.data[pt_vt].y;
    vt.z = pos.z + ts_object->vertices.data[pt_vt].z + half_tile_size.d;
    
    scn_face->vertices[i] = vt;
  }
  
  if (ts_face->type & TEXTURED) {
    for (int i = 0; i < ts_face->num_vertices; i++) {
      int pt_tx = ts_face->tx_vt_index[i];
      scn_face->txcoords[i] = ts_object->txcoords.data[pt_tx];
    }
    
    scn_face->has_texture = 1;
    scn_face->texture_id = ts_face->texture_id;
  }
  
  scn_object->num_faces++;
  scn_face->num_vertices = ts_face->num_vertices;
  scn_face->normal = ts_face->normal;
  scn_face->type = ts_face->type;
}