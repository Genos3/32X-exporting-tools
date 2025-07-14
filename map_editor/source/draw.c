#include "common.h"

void draw_map_tile(int x, int y, int z, model_t *model);
void draw_cursor_guides(vec3_t pos);
void draw_cursor_grid(vec3_t pos);
void transform_map_cube(fixed tile_size);
void draw_map_cube_lines(vec3_t pos, u16 color);
void draw_map_cube_face_lines(vec3_t pos, u8 axis, u16 color);

#define TS fix(0.5)
static fixed mdl_cube_matrix[12];
static vec3_t mdl_cube_tr_vertices[8]; // vertices pre transformed

static vec3_t mdl_cube_vertices[] = {
  {-TS,TS,-TS}, {TS,TS,-TS}, {TS,TS,TS}, {-TS,TS,TS}, {-TS,-TS,-TS}, {TS,-TS,-TS}, {TS,-TS,TS}, {-TS,-TS,TS}
};

static u8 mdl_cube_lines[] = {
  0,1, 1,2, 2,3, 3,0,
  4,5, 5,6, 6,7, 7,4,
  0,4, 1,5, 2,6, 3,7
};

static u8 mdl_cube_face_lines[][8] = {
  {4,5, 5,6, 6,7, 7,4},
  {0,1, 1,5, 5,4, 4,0},
  {3,0, 0,4, 4,7, 7,3}
};

#define MDL_CUBE_NUM_VT 8
#define MDL_CUBE_LINES_SIZE sizeof(mdl_cube_lines)

void draw_map(model_t *model) {  
  r_scene_num_polys = 0;
  
  // tr_vertices = model_tr_vertices;
  
  if (model->num_sprite_vertices) {
    transform_vertices(camera_sprite_matrix, model->sprite_vertices, tr_sprite_vertices, model->num_sprite_vertices);
  }
  
  set_matrix(model_matrix, identity_matrix);
  rotate_matrix(-cam.rot.x, -cam.rot.y, -cam.rot.z, model_matrix, 1);
  transform_vertices(model_matrix, model->vertices, model_tr_vertices, model->num_vertices);
  
  #if DRAW_NORMALS
    if (model->has_normals) {
      transform_vertices(model_matrix, model->normals, tr_normals, model->num_faces);
    }
  #endif
  
  aabb_t map_aabb;
  // obtain the frustum aabb
  get_frustum_aabb(&map_aabb);
  
  // normalize the frustum aabb and the camera position against the grid size
  
  map_aabb.min.x = clamp_i(map_aabb.min.x >> FP, 0, MAX_MAP_WIDTH - 1);
  map_aabb.max.x = clamp_i(map_aabb.max.x >> FP, 0, MAX_MAP_WIDTH - 1);
  map_aabb.min.z = clamp_i(map_aabb.min.z >> FP, 0, MAX_MAP_DEPTH - 1);
  map_aabb.max.z = clamp_i(map_aabb.max.z >> FP, 0, MAX_MAP_DEPTH - 1);
  #if TILE_HALF_HEIGHT
    map_aabb.min.y = clamp_i(map_aabb.min.y >> (FP - 1), 0, MAX_MAP_HEIGHT - 1);
    map_aabb.max.y = clamp_i(map_aabb.max.y >> (FP - 1), 0, MAX_MAP_HEIGHT - 1);
  #else
    map_aabb.min.y = clamp_i(map_aabb.min.y >> FP, 0, MAX_MAP_HEIGHT - 1);
    map_aabb.max.y = clamp_i(map_aabb.max.y >> FP, 0, MAX_MAP_HEIGHT - 1);
  #endif
  
  // clamp the camera tile position within the map
  
  vec3i_t cam_tile_pos;
  cam_tile_pos.x = clamp_i(cam.pos.x >> FP, map_aabb.min.x, map_aabb.max.x);
  cam_tile_pos.z = clamp_i(cam.pos.z >> FP, map_aabb.min.z, map_aabb.max.z);
  #if TILE_HALF_HEIGHT
    cam_tile_pos.y = clamp_i(cam.pos.y >> (FP - 1), map_aabb.min.y, map_aabb.max.y);
  #else
    cam_tile_pos.y = clamp_i(cam.pos.y >> FP, map_aabb.min.y, map_aabb.max.y);
  #endif
  
  if (cursor_enabled) {
    draw_cursor_guides(cursor_pos);
    
    if (map_grid_enabled) {
      draw_cursor_grid(cursor_pos);
    }
    
    transform_map_cube(TILE_SIZE);
  }
  
  // draw the map drawing each octant separately from the outside towards the camera
  
  // draw from back to front
  
  for (int i = map_aabb.min.y; i < cam_tile_pos.y; i++) {
    for (int j = map_aabb.min.z; j < cam_tile_pos.z; j++) {
      for (int k = map_aabb.min.x; k < cam_tile_pos.x; k++) {
        draw_map_tile(k, i, j, model);
      }
      
      for (int k = map_aabb.max.x; k >= cam_tile_pos.x; k--) {
        draw_map_tile(k, i, j, model);
      }
    }
    
    for (int j = map_aabb.max.z; j >= cam_tile_pos.z; j--) {
      for (int k = map_aabb.min.x; k < cam_tile_pos.x; k++) {
        draw_map_tile(k, i, j, model);
      }
      
      for (int k = map_aabb.max.x; k >= cam_tile_pos.x; k--) {
        draw_map_tile(k, i, j, model);
      }
    }
  }
  
  for (int i = map_aabb.max.y; i >= cam_tile_pos.y; i--) {
    for (int j = map_aabb.min.z; j < cam_tile_pos.z; j++) {
      for (int k = map_aabb.min.x; k < cam_tile_pos.x; k++) {
        draw_map_tile(k, i, j, model);
      }
      for (int k = map_aabb.max.x; k >= cam_tile_pos.x; k--) {
        draw_map_tile(k, i, j, model);
      }
    }
    
    for (int j = map_aabb.max.z; j >= cam_tile_pos.z; j--) {
      for (int k = map_aabb.min.x; k < cam_tile_pos.x; k++) {
        draw_map_tile(k, i, j, model);
      }
      
      for (int k = map_aabb.max.x; k >= cam_tile_pos.x; k--) {
        draw_map_tile(k, i, j, model);
      }
    }
  }
}

void draw_map_tile(int x, int y, int z, model_t *model) {
  vec3_t tile_pos;
  tile_t tile;
  u8 draw_selected_tile = 0; // draw the selected tile on the cursor
  u8 draw_buffer_tile = 0; // draw the tile being copied or moved
  u8 curr_tile_draw_cursor = 0; // draw a cursor on the current tile
  curr_selected_tile = 0; // mark the selected tile
  
  if (cursor_enabled) {
    if (cursor_mode == PAINT_MODE) {
      if (!select_multiple) {
        // if the current tile contains the cursor
        if (cursor_pos.x == x && cursor_pos.y == y && cursor_pos.z == z) {
          curr_selected_tile = 1;
          curr_tile_draw_cursor = 1;
          draw_selected_tile = 1;
        }
      } else { // select multiple
        if (x >= aabb_select.min.x &&
          y >= aabb_select.min.y &&
          z >= aabb_select.min.z &&
          x <= aabb_select.max.x &&
          y <= aabb_select.max.y &&
          z <= aabb_select.max.z) { // else if the current tile is within the selected area
            // if the tile is on one of the edges
            if ((y == aabb_select.min.y && z == aabb_select.min.z && x == aabb_select.min.x) ||
              (y == aabb_select.min.y && z == aabb_select.min.z && x == aabb_select.max.x) ||
              (y == aabb_select.min.y && z == aabb_select.max.z && x == aabb_select.min.x) ||
              (y == aabb_select.min.y && z == aabb_select.max.z && x == aabb_select.max.x) ||
              (y == aabb_select.max.y && z == aabb_select.min.z && x == aabb_select.min.x) ||
              (y == aabb_select.max.y && z == aabb_select.min.z && x == aabb_select.max.x) ||
              (y == aabb_select.max.y && z == aabb_select.max.z && x == aabb_select.min.x) ||
              (y == aabb_select.max.y && z == aabb_select.max.z && x == aabb_select.max.x)) {
              curr_tile_draw_cursor = 1;
            }
            
            size3_t tile_size;
            tile_size.w = (max_c(model->objects_size[selected_tile - 1].w - 1, 0) >> FP) + 1;
            tile_size.d = (max_c(model->objects_size[selected_tile - 1].d - 1, 0) >> FP) + 1;
            #if TILE_HALF_HEIGHT
              tile_size.h = (max_c(model->objects_size[selected_tile - 1].h - 1, 0) >> (FP - 1)) + 1;
            #else
              tile_size.h = (max_c(model->objects_size[selected_tile - 1].h - 1, 0) >> FP) + 1;
            #endif
            
            if (!((x - aabb_select.min.x) % tile_size.w ||
              (y - aabb_select.min.y) % tile_size.h ||
              (z - aabb_select.min.z) % tile_size.d)) {
              curr_selected_tile = 1;
              draw_selected_tile = 1;
            }
        }
      }
    } else { // the cursor is in selection mode
      if (!select_multiple) {
        if (!copy_selection && !move_selection) {
          // if the current tile contains the cursor
          if (cursor_pos.x == x && cursor_pos.y == y && cursor_pos.z == z) {
            curr_selected_tile = 1;
            curr_tile_draw_cursor = 1;
          }
        } else {
          if (cursor_pos.x == x && cursor_pos.y == y && cursor_pos.z == z) {
            tile = map[m_sel_cursor_pos.y][m_sel_cursor_pos.z][m_sel_cursor_pos.x];
            curr_selected_tile = 1;
            curr_tile_draw_cursor = 1;
            draw_buffer_tile = 1;
          } else if (move_selection && m_sel_cursor_pos.x == x && m_sel_cursor_pos.y == y && m_sel_cursor_pos.z == z) {
            goto skip_tile;
          }
        }
      } else { // select multiple
        if (copy_selection || move_selection) {
          // if the current tile is within the area to move
          if (x >= aabb_move.min.x &&
            y >= aabb_move.min.y &&
            z >= aabb_move.min.z &&
            x <= aabb_move.max.x &&
            y <= aabb_move.max.y &&
            z <= aabb_move.max.z) {
              if ((y == aabb_select.min.y && z == aabb_select.min.z && x == aabb_select.min.x) ||
                (y == aabb_select.min.y && z == aabb_select.min.z && x == aabb_select.max.x) ||
                (y == aabb_select.min.y && z == aabb_select.max.z && x == aabb_select.min.x) ||
                (y == aabb_select.min.y && z == aabb_select.max.z && x == aabb_select.max.x) ||
                (y == aabb_select.max.y && z == aabb_select.min.z && x == aabb_select.min.x) ||
                (y == aabb_select.max.y && z == aabb_select.min.z && x == aabb_select.max.x) ||
                (y == aabb_select.max.y && z == aabb_select.max.z && x == aabb_select.min.x) ||
                (y == aabb_select.max.y && z == aabb_select.max.z && x == aabb_select.max.x)) {
                curr_tile_draw_cursor = 1;
              }
              
              tile = copy_buffer[(y - aabb_move.min.y) * aabb_move_size.d * aabb_move_size.w + (z - aabb_move.min.z) * aabb_move_size.w + x - aabb_move.min.x];
              curr_selected_tile = 1;
              draw_buffer_tile = 1;
          } else if (move_selection) { // if the current tile is within the area to move from
            if (x >= aabb_select.min.x &&
              y >= aabb_select.min.y &&
              z >= aabb_select.min.z &&
              x <= aabb_select.max.x &&
              y <= aabb_select.max.y &&
              z <= aabb_select.max.z) {
              goto skip_tile;
            }
          }
        } else {
          if (x >= aabb_select.min.x &&
            y >= aabb_select.min.y &&
            z >= aabb_select.min.z &&
            x <= aabb_select.max.x &&
            y <= aabb_select.max.y &&
            z <= aabb_select.max.z) { // if the current tile is within the selected area
            if ((y == aabb_select.min.y && z == aabb_select.min.z && x == aabb_select.min.x) ||
              (y == aabb_select.min.y && z == aabb_select.min.z && x == aabb_select.max.x) ||
              (y == aabb_select.min.y && z == aabb_select.max.z && x == aabb_select.min.x) ||
              (y == aabb_select.min.y && z == aabb_select.max.z && x == aabb_select.max.x) ||
              (y == aabb_select.max.y && z == aabb_select.min.z && x == aabb_select.min.x) ||
              (y == aabb_select.max.y && z == aabb_select.min.z && x == aabb_select.max.x) ||
              (y == aabb_select.max.y && z == aabb_select.max.z && x == aabb_select.min.x) ||
              (y == aabb_select.max.y && z == aabb_select.max.z && x == aabb_select.max.x)) {
              curr_tile_draw_cursor = 1;
            }
            
            curr_selected_tile = 1;
          }
        }
      }
    }
  }
  
  if (!draw_buffer_tile) {
    tile = map[y][z][x];
  }
  
  for (int i = 0; i < 3; i++) {
    int tile_id;
    if (draw_selected_tile && i == cursor_axis) {
      tile_id = selected_tile - 1;
    } else {
      tile_id = *((u8*)&tile + i) - 1;
    }
    
    // if the tile is empty continue
    if (tile_id < 0) continue;
    
    // tile mid point
    tile_pos.x = (x << FP) + (model->objects_size[tile_id].w >> 1);
    tile_pos.z = (z << FP) + (model->objects_size[tile_id].d >> 1);
    #if TILE_HALF_HEIGHT
      tile_pos.y = y << (FP - 1);
    #else
      tile_pos.y = y << FP;
    #endif
    
    vec3_t tile_center_pos;
    tile_center_pos.x = tile_pos.x;
    tile_center_pos.y = tile_pos.y + (model->objects_size[tile_id].h >> 1); // fix(0.25);
    tile_center_pos.z = tile_pos.z;
    
    fixed tile_radius = max_c(max_c(model->objects_size[tile_id].w, model->objects_size[tile_id].d), model->objects_size[tile_id].h) >> 1;
    tile_radius = fp_mul(tile_radius, DIAG_UNIT_DIST_RC);
    
    // frustum culling
    // 0 = outside, 1 = partially inside, 2 = totally inside
    
    u8 clip_flags;
    u8 frustum_side = check_frustum_culling_vis(tile_center_pos, tile_radius, 0, &clip_flags);
    
    if (!frustum_side) return;
    
    vec3_t tr_tile_pos;
    transform_vertex(tile_pos, &tr_tile_pos, camera_matrix);
    set_matrix(model_matrix, identity_matrix);
    translate_matrix(tr_tile_pos.x, tr_tile_pos.y, tr_tile_pos.z, model_matrix);
    transform_vertices_pos(model_matrix, model_tr_vertices, tr_vertices, model->num_vertices);
    
    for (int i = 0; i < model->object_num_faces[tile_id]; i++) {
      int face_id = model->object_face_index[tile_id] + i;
      /* u8 shared_face = shared_face_dir[face_id];
      
      if (shared_face) {
        if (shared_face == 1 && x && map[y][z][x - 1]) continue; // left
        if (shared_face == 2 && x < MAX_MAP_WIDTH - 1 && map[y][z][x + 1]) continue; // right
        if (shared_face == 3 && z && map[y][z - 1][x]) continue; // up
        if (shared_face == 4 && z < MAX_MAP_DEPTH - 1 && map[y][z + 1][x]) continue; // down
      } */
      
      clipping_pl_list[face_id] = clip_flags;
      
      draw_face_vis(tile_pos, face_id, 0, model);
    }
  }
  
  skip_tile:
  if (curr_tile_draw_cursor) {
    tile_pos.x = (x << FP) + fix(0.5);
    tile_pos.z = (z << FP) + fix(0.5);
    #if TILE_HALF_HEIGHT
      tile_pos.y = y << (FP - 1);
    #else
      tile_pos.y = y << FP;
    #endif
    
    draw_cursor(tile_pos);
  }
}

void set_shared_faces_list(model_t *model) {
  float half_tile_size = map_scn.tile_size / 2;
  
  for (int i = 0; i < model->num_faces; i++) {
    int vt_id = model->faces[model->face_index[i]];
    
    if (model->normals[i].x == -1 && model->vertices[vt_id].x <= -half_tile_size + fix(0.01)) {
      shared_face_dir[i] = 1; // left
    } else
    if (model->normals[i].x == 1 && model->vertices[vt_id].x >= half_tile_size - fix(0.01)) {
      shared_face_dir[i] = 2; // right
    } else
    if (model->normals[i].z == -1 && model->vertices[vt_id].z <= -half_tile_size + fix(0.01)) {
      shared_face_dir[i] = 3; // up
    } else
    if (model->normals[i].z == 1 && model->vertices[vt_id].z >= half_tile_size - fix(0.01)) {
      shared_face_dir[i] = 4; // down
    }
  }
}

#define CR_LINE_DIST (32 << FP)

void draw_cursor_guides(vec3_t pos) {
  line_t line, tr_line;
  u16 color = PAL_BLUE;
  
  pos.x = (pos.x << FP) + fix(0.5);
  pos.z = (pos.z << FP) + fix(0.5);
  #if TILE_HALF_HEIGHT
    pos.y = (pos.y << (FP - 1)) + fix(0.5);
  #else
    pos.y = (pos.y << FP) + fix(0.5);
  #endif
  
  line = (line_t){{pos.x - CR_LINE_DIST, pos.y, pos.z}, {pos.x + CR_LINE_DIST, pos.y, pos.z}};
  transform_vertex(line.p0, &tr_line.p0, camera_matrix);
  transform_vertex(line.p1, &tr_line.p1, camera_matrix);
  draw_line_3d(tr_line, color);
  
  line = (line_t){{pos.x, pos.y - CR_LINE_DIST, pos.z}, {pos.x, pos.y + CR_LINE_DIST, pos.z}};
  transform_vertex(line.p0, &tr_line.p0, camera_matrix);
  transform_vertex(line.p1, &tr_line.p1, camera_matrix);
  draw_line_3d(tr_line, color);
  
  line = (line_t){{pos.x, pos.y, pos.z - CR_LINE_DIST}, {pos.x, pos.y, pos.z + CR_LINE_DIST}};
  transform_vertex(line.p0, &tr_line.p0, camera_matrix);
  transform_vertex(line.p1, &tr_line.p1, camera_matrix);
  draw_line_3d(tr_line, color);
}

void draw_cursor_grid(vec3_t pos) {
  aabb_t grid_aabb;
  u16 color = PAL_BLUE;
  
  grid_aabb.min.x = max_c(pos.x - 4, 0);
  grid_aabb.min.z = max_c(pos.z - 4, 0);
  grid_aabb.max.x = min_c(pos.x + 5, MAX_MAP_WIDTH);
  grid_aabb.max.z = min_c(pos.z + 5, MAX_MAP_DEPTH);
  
  for (int i = grid_aabb.min.z; i <= grid_aabb.max.z; i++) { // MAX_MAP_DEPTH
    #if TILE_HALF_HEIGHT
      line_t line = {{grid_aabb.min.x << FP, pos.y << (FP - 1), i << FP}, {grid_aabb.max.x << FP, pos.y << (FP - 1), i << FP}};
    #else
      line_t line = {{grid_aabb.min.x << FP, pos.y << FP, i << FP}, {grid_aabb.max.x << FP, pos.y << FP, i << FP}};
    #endif
    
    line_t tr_line;
    transform_vertex(line.p0, &tr_line.p0, camera_matrix);
    transform_vertex(line.p1, &tr_line.p1, camera_matrix);
    
    draw_line_3d(tr_line, color);
  }
  
  for (int i = grid_aabb.min.x; i <= grid_aabb.max.x; i++) { // MAX_MAP_WIDTH
    #if TILE_HALF_HEIGHT
      line_t line = {{i << FP, pos.y << (FP - 1), grid_aabb.min.z << FP}, {i << FP, pos.y << (FP - 1), grid_aabb.max.z << FP}};
    #else
      line_t line = {{i << FP, pos.y << FP, grid_aabb.min.z << FP}, {i << FP, pos.y << FP, grid_aabb.max.z << FP}};
    #endif
    
    line_t tr_line;
    transform_vertex(line.p0, &tr_line.p0, camera_matrix);
    transform_vertex(line.p1, &tr_line.p1, camera_matrix);
    
    draw_line_3d(tr_line, color);
  }
}

void draw_cursor(vec3_t pos) {
  pos.y += fix(0.5);
  
  if (cursor_mode == SELECT_MODE) {
    draw_map_cube_lines(pos, PAL_BLUE);
  } else {
    draw_map_cube_face_lines(pos, cursor_axis, PAL_RED);
  }
}

void transform_map_cube(fixed tile_size) {
  set_matrix(mdl_cube_matrix, identity_matrix);
  scale_matrix(tile_size, tile_size, tile_size, mdl_cube_matrix);
  rotate_matrix(-cam.rot.x, -cam.rot.y, -cam.rot.z, mdl_cube_matrix, 1);
  transform_vertices(mdl_cube_matrix, mdl_cube_vertices, mdl_cube_tr_vertices, MDL_CUBE_NUM_VT);
}

void draw_map_cube_lines(vec3_t pos, u16 color) {
  transform_vertex(pos, &pos, camera_matrix);
  set_matrix(mdl_cube_matrix, identity_matrix);
  translate_matrix(pos.x, pos.y, pos.z, mdl_cube_matrix);
  transform_vertices_pos(mdl_cube_matrix, mdl_cube_tr_vertices, tr_vertices, MDL_CUBE_NUM_VT);
  
  for (int i = 0; i < MDL_CUBE_LINES_SIZE; i += 2) {
    line_t line;
    line.p0.x = tr_vertices[mdl_cube_lines[i]].x;
    line.p0.y = tr_vertices[mdl_cube_lines[i]].y;
    line.p0.z = tr_vertices[mdl_cube_lines[i]].z;
    line.p1.x = tr_vertices[mdl_cube_lines[i + 1]].x;
    line.p1.y = tr_vertices[mdl_cube_lines[i + 1]].y;
    line.p1.z = tr_vertices[mdl_cube_lines[i + 1]].z;
    
    draw_line_3d(line, color);
  }
}

void draw_map_cube_face_lines(vec3_t pos, u8 axis, u16 color) {
  transform_vertex(pos, &pos, camera_matrix);
  set_matrix(mdl_cube_matrix, identity_matrix);
  translate_matrix(pos.x, pos.y, pos.z, mdl_cube_matrix);
  transform_vertices_pos(mdl_cube_matrix, mdl_cube_tr_vertices, tr_vertices, MDL_CUBE_NUM_VT);
  
  for (int i = 0; i < 8; i += 2) {
    line_t line;
    line.p0.x = tr_vertices[mdl_cube_face_lines[axis][i]].x;
    line.p0.y = tr_vertices[mdl_cube_face_lines[axis][i]].y;
    line.p0.z = tr_vertices[mdl_cube_face_lines[axis][i]].z;
    line.p1.x = tr_vertices[mdl_cube_face_lines[axis][i + 1]].x;
    line.p1.y = tr_vertices[mdl_cube_face_lines[axis][i + 1]].y;
    line.p1.z = tr_vertices[mdl_cube_face_lines[axis][i + 1]].z;
    
    draw_line_3d(line, color);
  }
}