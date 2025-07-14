#include "common.h"

void set_tile(int y, int z, int x);
void add_face(vec3_t pos, int face_id, int tile_id);

void make_model() {
  for (int i = 0; i < map_size.h; i++) {
    for (int j = 0; j < map_size.d; j++) {
      for (int k = 0; k < map_size.w; k++) {
        set_tile(i, j, k);
      }
    }
  }
  
  for (int i = 0; i < tileset.num_materials; i++) {
    list_push_pnt(&scene.mtl_textures, &tileset.mtl_textures.data[i]);
  }
  
  scene.num_objects = 0;
  scene.num_materials = tileset.num_materials;
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
    
    for (int j = 0; j < tileset.object_num_faces.data[tile_id]; j++) {
      int face_id = tileset.object_face_index.data[tile_id] + j;
      add_face(pos, face_id, tile_id);
    }
  }
}

void add_face(vec3_t pos, int face_id, int tile_id) {
  list_push_int(&scene.face_index, scene.faces_size);
  list_push_int(&scene.tx_face_index, scene.tx_faces_size);
  
  size3_t half_tile_size;
  half_tile_size.w = tileset.objects_size.data[tile_id].w / 2;
  half_tile_size.d = tileset.objects_size.data[tile_id].d / 2;
  
  for (int i = 0; i < tileset.face_num_vertices.data[face_id]; i++) {
    int pt_vt = tileset.faces.data[tileset.face_index.data[face_id] + i];
    
    vec3_t vt;
    vt.x = pos.x + tileset.vertices.data[pt_vt].x + half_tile_size.w;
    vt.y = pos.y + tileset.vertices.data[pt_vt].y;
    vt.z = pos.z + tileset.vertices.data[pt_vt].z + half_tile_size.d;
    
    list_push_pnt(&scene.vertices, &vt);
    list_push_int(&scene.faces, scene.num_vertices);
    
    scene.num_vertices++;
    scene.faces_size++;
    
    if (!(tileset.face_types.data[face_id] & UNTEXTURED)) {
      int pt_tx = tileset.tx_faces.data[tileset.tx_face_index.data[face_id] + i];
      
      vec2_tx_t tx_vt;
      tx_vt.u = tileset.txcoords.data[pt_tx].u;
      tx_vt.v = tileset.txcoords.data[pt_tx].v;
      list_push_pnt(&scene.txcoords, &tx_vt);
      list_push_int(&scene.tx_faces, scene.num_txcoords);
      
      scene.num_txcoords++;
      scene.tx_faces_size++;
    }
  }
  
  scene.num_faces++;
  scene.num_tx_faces++;
  
  vec3_t normal;
  normal.x = tileset.normals.data[face_id].x;
  normal.y = tileset.normals.data[face_id].y;
  normal.z = tileset.normals.data[face_id].z;
  
  list_push_pnt(&scene.normals, &normal);
  list_push_int(&scene.face_num_vertices, tileset.face_num_vertices.data[face_id]);
  list_push_int(&scene.face_materials, tileset.face_materials.data[face_id]);
  list_push_int(&scene.face_types, tileset.face_types.data[face_id]);
  list_push_int(&scene.sprite_face_index, -1);
}