#include "common.h"

void move_cursor(u8 dir);
void update_matrix_transforms();
void draw_scene();
void reset_display_list();

void update_state(fixed dt) {
  // int check_collision = 0;
  if (!cursor_enabled) {
    if (mouse_moved) { // rotate camera
      cam.rot.y -= cam.mouse_rot_speed * i_cursor_dx;
      cam.rot.x += cam.mouse_rot_speed * i_cursor_dy;
      
      if (cam.rot.x < 192 << 8 && cam.rot.x > 128 << 8) {
        cam.rot.x = 192 << 8;
      } else if (cam.rot.x > 64 << 8 && cam.rot.x < 128 << 8) {
        cam.rot.x = 64 << 8;
      }
      
      redraw_scene = 1;
    } else if (!mouse_capture && i_direction_pressed) {
      if (i_direction_pressed & KEY_NUMPAD_1) {
        cam.rot.x += fp_mul(cam.rot_speed, dt);
        
        if (cam.rot.x > 64 << 8 && cam.rot.x < 128 << 8) {
          cam.rot.x = 64 << 8;
        }
      } else
      if (i_direction_pressed & KEY_NUMPAD_2) {
        cam.rot.x -= fp_mul(cam.rot_speed, dt);
        
        if (cam.rot.x < 192 << 8 && cam.rot.x > 128 << 8) {
          cam.rot.x = 192 << 8;
        }
      }
      
      redraw_scene = 1;
    }
  }
  
  if (i_direction_pressed) { // move camera
    if (!cursor_enabled) {
      fixed c_speed_t = fp_mul(cam.acc, dt);
      
      fixed cos_a = lu_cos(cam.rot.y);  // .16
      fixed sin_a = lu_sin(cam.rot.y);
      fixed delta_x = 0;
      fixed delta_z = 0;
      
      if (i_direction_pressed & KEY_W) {
        delta_x += fp_mul(sin_a, c_speed_t);
        delta_z += fp_mul(cos_a, c_speed_t);
      } else if (i_direction_pressed & KEY_S) {
        delta_x -= fp_mul(sin_a, c_speed_t);
        delta_z -= fp_mul(cos_a, c_speed_t);
      }
      
      if (i_direction_pressed & KEY_A) {
        if (mouse_capture) {
          delta_x += fp_mul(cos_a, c_speed_t);
          delta_z -= fp_mul(sin_a, c_speed_t);
        } else {
          cam.rot.y += fp_mul(cam.rot_speed, dt);
        }
      } else if (i_direction_pressed & KEY_D) {
        if (mouse_capture) {
          delta_x -= fp_mul(cos_a, c_speed_t);
          delta_z += fp_mul(sin_a, c_speed_t);
        } else {
          cam.rot.y -= fp_mul(cam.rot_speed, dt);
        }
      }
      
      if (!mouse_capture) {
        if (i_direction_pressed & KEY_NUMPAD_4) {
          delta_x += fp_mul(cos_a, c_speed_t);
          delta_z -= fp_mul(sin_a, c_speed_t);
        } else
        if (i_direction_pressed & KEY_NUMPAD_5) {
          delta_x -= fp_mul(cos_a, c_speed_t);
          delta_z += fp_mul(sin_a, c_speed_t);
        }
        
        if (i_direction_pressed & KEY_NUMPAD_3) {
          cam.pos.y -= c_speed_t;
        } else if (i_direction_pressed & KEY_NUMPAD_6) {
          cam.pos.y += c_speed_t;
        }
      }
      
      if (i_direction_pressed & KEY_Q) {
        cam.pos.y -= c_speed_t;
      } else if (i_direction_pressed & KEY_E) {
        cam.pos.y += c_speed_t;
      }
      
      cam.pos.x += delta_x;
      cam.pos.z += delta_z;
    } else {
      if (i_direction_pressed & KEY_W) {
        orbit_cam.zoom -= 0.15;
      } else
      if (i_direction_pressed & KEY_S) {
        orbit_cam.zoom += 0.15;
      }
      
      if (i_direction_pressed & KEY_A) {
        orbit_cam.yaw -= 1.5;
      } else
      if (i_direction_pressed & KEY_D) {
        orbit_cam.yaw += 1.5;
      }
      
      if (i_direction_pressed & KEY_Q) {
        orbit_cam.pitch -= 1.5;
      } else
      if (i_direction_pressed & KEY_E) {
        orbit_cam.pitch += 1.5;
      }
      
      if (orbit_cam.pitch < -89.9) {
        orbit_cam.pitch = -89.9;
      } else
      if (orbit_cam.pitch > 89.9) {
        orbit_cam.pitch = 89.9;
      }
      
      if (orbit_cam.yaw < 0) {
        orbit_cam.yaw = 360;
      } else
      if (orbit_cam.yaw >= 360) {
        orbit_cam.yaw = 0;
      }
      
      if (orbit_cam.zoom < 2) {
        orbit_cam.zoom = 2;
      } else
      if (orbit_cam.zoom > 32) {
        orbit_cam.zoom = 32;
      }
      
      if (cursor_enabled) {
        set_camera_to_cursor();
      }
    }
    
    redraw_scene = 1;
  }
  
  u8 cursor_input = 0;
  
  if (enable_mouse_cursor && mouse_moved) {
    cursor_dx_acc += i_cursor_dx;
    cursor_dy_acc += i_cursor_dy;
    
    if (cursor_dx_acc < -10) {
      cursor_input |= LEFT_DIR;
      cursor_dx_acc %= 10;
    } else if (cursor_dx_acc > 10) {
      cursor_input |= RIGHT_DIR;
      cursor_dx_acc %= 10;
    }
    if (cursor_dy_acc < -10) {
      cursor_input |= UP_DIR;
      cursor_dy_acc %= 10;
    } else if (cursor_dy_acc > 10) {
      cursor_input |= DOWN_DIR;
      cursor_dy_acc %= 10;
    }
  }
  
  if (cursor_enabled && (i_cursor_key_pressed || cursor_input)) { // move cursor    
    if (orbit_cam.yaw > 315 || orbit_cam.yaw <= 45) { // facing north
      if (i_cursor_key_pressed & KEY_UP || cursor_input & UP_DIR) {
        move_cursor(NORTH);
      } else if (i_cursor_key_pressed & KEY_DOWN || cursor_input & DOWN_DIR) {
        move_cursor(SOUTH);
      }
      
      if (i_cursor_key_pressed & KEY_LEFT || cursor_input & LEFT_DIR) {
        move_cursor(WEST);
      } else if (i_cursor_key_pressed & KEY_RIGHT || cursor_input & RIGHT_DIR) {
        move_cursor(EAST);
      }
    } else if (orbit_cam.yaw > 225 && orbit_cam.yaw <= 315) { // facing east
      if (i_cursor_key_pressed & KEY_UP || cursor_input & UP_DIR) {
        move_cursor(EAST);
      } else if (i_cursor_key_pressed & KEY_DOWN || cursor_input & DOWN_DIR) {
        move_cursor(WEST);
      }
      
      if (i_cursor_key_pressed & KEY_LEFT || cursor_input & LEFT_DIR) {
        move_cursor(NORTH);
      } else if (i_cursor_key_pressed & KEY_RIGHT || cursor_input & RIGHT_DIR) {
        move_cursor(SOUTH);
      }
    } else if (orbit_cam.yaw > 135 && orbit_cam.yaw <= 225) { // facing south
      if (i_cursor_key_pressed & KEY_UP || cursor_input & UP_DIR) {
        move_cursor(SOUTH);
      } else if (i_cursor_key_pressed & KEY_DOWN || cursor_input & DOWN_DIR) {
        move_cursor(NORTH);
      }
      
      if (i_cursor_key_pressed & KEY_LEFT || cursor_input & LEFT_DIR) {
        move_cursor(EAST);
      } else if (i_cursor_key_pressed & KEY_RIGHT || cursor_input & RIGHT_DIR) {
        move_cursor(WEST);
      }
    } else { // facing west
      if (i_cursor_key_pressed & KEY_UP || cursor_input & UP_DIR) {
        move_cursor(WEST);
      } else if (i_cursor_key_pressed & KEY_DOWN || cursor_input & DOWN_DIR) {
        move_cursor(EAST);
      }
      
      if (i_cursor_key_pressed & KEY_LEFT || cursor_input & LEFT_DIR) {
        move_cursor(SOUTH);
      } else if (i_cursor_key_pressed & KEY_RIGHT || cursor_input & RIGHT_DIR) {
        move_cursor(NORTH);
      }
    }
    
    if (i_cursor_key_pressed & KEY_Z) {
      move_cursor(DOWN);
    } else if (i_cursor_key_pressed & KEY_X) {
      move_cursor(UP);
    }
    
    if (select_multiple && !copy_selection && !move_selection) {
      set_aabb(&cursor_pos, &m_sel_cursor_pos, &aabb_select);
    }
    
    if (cursor_enabled) {
      set_camera_to_cursor();
    }
    
    i_cursor_key_pressed = 0;
    redraw_scene = 1;
  }
  
  if (i_key_pressed || i_mouse_button_pressed) {
    if (i_key_pressed == KEY_RETURN) { // enter: toggle cursor
      if (!cursor_enabled) {
        cursor_enabled = 1;
        set_camera_to_cursor();
      } else {
        cursor_enabled = 0;
      }
    } else if (i_key_pressed == KEY_SPACE) { // space: reset camera
      set_cam_pos();
    } else if (cursor_enabled) {
      if (i_key_pressed == KEY_CONTROL || i_mouse_button_pressed & LEFT_MOUSE_BUTTON) { // control: place tile
        if (cursor_mode == SELECT_MODE) {
          if (select_multiple) {
            if (move_selection) {
              move_selected_area();
              move_selection = 0;
              select_multiple = 0;
            } else if (copy_selection){
              paste_selected_area();
            }
          } else if (copy_selection) {
            map[cursor_pos.y][cursor_pos.z][cursor_pos.x] = map[m_sel_cursor_pos.y][m_sel_cursor_pos.z][m_sel_cursor_pos.x];
            copy_selection = 0;
          } else if (move_selection) {
            map[cursor_pos.y][cursor_pos.z][cursor_pos.x] = map[m_sel_cursor_pos.y][m_sel_cursor_pos.z][m_sel_cursor_pos.x];
            map[m_sel_cursor_pos.y][m_sel_cursor_pos.z][m_sel_cursor_pos.x].y = 0;
            map[m_sel_cursor_pos.y][m_sel_cursor_pos.z][m_sel_cursor_pos.x].x = 0;
            map[m_sel_cursor_pos.y][m_sel_cursor_pos.z][m_sel_cursor_pos.x].z = 0;
            move_selection = 0;
          }
        } else { // paint mode
          // insert tile
          if (!select_multiple) {
            *((u8*)&map[cursor_pos.y][cursor_pos.z][cursor_pos.x] + cursor_axis) = selected_tile;
          } else {
            set_multiple_tiles(&m_sel_cursor_pos, &cursor_pos, selected_tile, g_model);
            select_multiple = 0;
          }
        }
      } else if (i_key_pressed == KEY_BACK) { // backspace: delete tile
        if (cursor_mode == SELECT_MODE) {
          if (!select_multiple) {
            map[cursor_pos.y][cursor_pos.z][cursor_pos.x].y = 0;
            map[cursor_pos.y][cursor_pos.z][cursor_pos.x].x = 0;
            map[cursor_pos.y][cursor_pos.z][cursor_pos.x].z = 0;
          } else {
            set_multiple_tiles(&m_sel_cursor_pos, &cursor_pos, 0, NULL);  
            select_multiple = 0;
          }
        } else {
          if (!select_multiple) { // if paint mode
            *((u8*)&map[cursor_pos.y][cursor_pos.z][cursor_pos.x] + cursor_axis) = 0;
          } else {
            set_multiple_tiles(&m_sel_cursor_pos, &cursor_pos, 0, NULL);  
            select_multiple = 0;
          }
        }
      } else if (i_key_pressed == KEY_B) { // 'B': cursor mode
        if (cursor_mode == SELECT_MODE) {
          cursor_mode = PAINT_MODE;
        } else {
          cursor_mode = SELECT_MODE;
        }
      } else if (i_key_pressed == KEY_C) { // 'C': select multiple / place multiple
        if (!select_multiple) {
          select_multiple = 1;
          m_sel_cursor_pos = cursor_pos;
          set_aabb(&cursor_pos, &m_sel_cursor_pos, &aabb_select);
        } else {
          select_multiple = 0;
          move_selection = 0;
          if (copy_selection) {
            copy_selection = 0;
            free(copy_buffer);
          }
        }
      } else if (i_key_pressed == KEY_V || i_mouse_button_pressed & RIGHT_MOUSE_BUTTON) { // 'V': copy selection
        if (cursor_mode == PAINT_MODE) {
          selected_tile = *((u8*)&map[cursor_pos.y][cursor_pos.z][cursor_pos.x] + cursor_axis);
        } else if (select_multiple) { // if select mode
          if (!copy_selection) {
            copy_selection = 1;
            // m_sel_cursor_pos = cursor_pos;
            aabb_move = aabb_select;
            copy_selected_area();
          } else {
            copy_selection = 0;
            select_multiple = 0;
            free(copy_buffer);
          }
        } else {
          if (!copy_selection) {
            m_sel_cursor_pos = cursor_pos;
            copy_selection = 1;
          } else {
            copy_selection = 0;
          }
        }
      } else if (i_key_pressed == KEY_M) { // 'M': move selection
        if (cursor_mode == SELECT_MODE) {
          if (select_multiple) {
            if (!move_selection) {
              move_selection = 1;
              // m_sel_cursor_pos = cursor_pos;
              aabb_move = aabb_select;
              copy_selected_area();
            } else {
              move_selection = 0;
              select_multiple = 0;
            }
          } else {
            if (!move_selection) {
              m_sel_cursor_pos = cursor_pos;
              move_selection = 1;
              // selected_tile = map[cursor_pos.y][cursor_pos.z][cursor_pos.x];
            } else {
              move_selection = 0;
            }
          }
        }
      }
      
      if (cursor_mode == PAINT_MODE) {
        if (i_key_pressed == KEY_COMMA) { // comma: previous tile
          if (i_modifier_key_pressed & KEY_SHIFT) { // shift
            selected_tile -= 10;
            
            if (selected_tile < 0) {
              selected_tile = 0;
            }
          } else {
            selected_tile--;
          }
          
          if (selected_tile < 0) {
            selected_tile = num_tiles;
          }
        } else if (i_key_pressed == KEY_POINT) { // point: next tile
          if (i_modifier_key_pressed & KEY_SHIFT) { // shift
            selected_tile += 10;
            
            if (selected_tile > num_tiles) {
              selected_tile = num_tiles;
            }
          } else {
            selected_tile++;
          }
          
          if (selected_tile > num_tiles) {
            selected_tile = 0;
          }
        } else if (i_key_pressed == KEY_MINUS) { // minus: change cursor axis
          cursor_axis++;
          
          if (cursor_axis == 3) {
            cursor_axis = 0;
          }
        }
      }
    }
    
    i_key_pressed = 0;
    i_mouse_button_pressed = 0;
    redraw_scene = 1;
  }

  /* if (cfg.collision_enabled && check_collision) {
    check_tile_collision();
  } */
}

void move_cursor(u8 dir) {
  if (dir == NORTH) {
    if (select_multiple & (copy_selection || move_selection)) {
      if (aabb_move.min.z > 0) {
        aabb_move.min.z--;
        aabb_move.max.z--;
        cursor_pos.z--;
      }
    } else if (cursor_mode == PAINT_MODE && select_multiple && selected_tile) {
      int tile_size = (max_c(g_model->objects_size[selected_tile - 1].d - 1, 0) >> FP) + 1;
      
      if (cursor_pos.z >= tile_size) {
        cursor_pos.z -= tile_size;
      }
    } else {
      if (i_modifier_key_pressed & KEY_SHIFT) {
        cursor_pos.z -= 4;
        
        if (cursor_pos.z < 0) {
          cursor_pos.z = 0;
        }
      } else if (cursor_pos.z > 0) {
        cursor_pos.z--;
      }
    }
  } else if (dir == SOUTH) {
    if (select_multiple & (copy_selection || move_selection)) {
      if (aabb_move.max.z < MAX_MAP_DEPTH - 1) {
        aabb_move.min.z++;
        aabb_move.max.z++;
        cursor_pos.z++;
      }
    } else if (cursor_mode == PAINT_MODE && select_multiple && selected_tile) {
      int tile_size = (max_c(g_model->objects_size[selected_tile - 1].d - 1, 0) >> FP) + 1;
      
      if (cursor_pos.z < MAX_MAP_DEPTH - tile_size) {
        cursor_pos.z += tile_size;
      }
    } else {
      if (i_modifier_key_pressed & KEY_SHIFT) {
        cursor_pos.z += 4;
        
        if (cursor_pos.z >= MAX_MAP_DEPTH) {
          cursor_pos.z = MAX_MAP_DEPTH - 1;
        }
      } else if (cursor_pos.z < MAX_MAP_DEPTH - 1) {
        cursor_pos.z++;
      }
    }
  } else if (dir == WEST) {
    if (select_multiple & (copy_selection || move_selection)) {
      if (aabb_move.min.x > 0) {
        aabb_move.min.x--;
        aabb_move.max.x--;
        cursor_pos.x--;
      }
    } else if (cursor_mode == PAINT_MODE && select_multiple && selected_tile) {
      int tile_size = (max_c(g_model->objects_size[selected_tile - 1].w - 1, 0) >> FP) + 1;
      
      if (cursor_pos.x >= tile_size) {
        cursor_pos.x -= tile_size;
      }
    } else {
      if (i_modifier_key_pressed & KEY_SHIFT) {
        cursor_pos.x -= 4;
        
        if (cursor_pos.x < 0) {
          cursor_pos.x = 0;
        }
      } else if (cursor_pos.x > 0) {
        cursor_pos.x--;
      }
    }
  } else if (dir == EAST) {
    if (select_multiple & (copy_selection || move_selection)) {
      if (aabb_move.max.x < MAX_MAP_WIDTH - 1) {
        aabb_move.min.x++;
        aabb_move.max.x++;
        cursor_pos.x++;
      }
    } else if (cursor_mode == PAINT_MODE && select_multiple && selected_tile) {
      int tile_size = (max_c(g_model->objects_size[selected_tile - 1].w - 1, 0) >> FP) + 1;
      
      if (cursor_pos.x < MAX_MAP_WIDTH - tile_size) {
        cursor_pos.x += tile_size;
      }
    } else {
      if (i_modifier_key_pressed & KEY_SHIFT) {
        cursor_pos.x += 4;
        
        if (cursor_pos.x >= MAX_MAP_WIDTH) {
          cursor_pos.x = MAX_MAP_WIDTH - 1;
        }
      } else if (cursor_pos.x < MAX_MAP_WIDTH - 1) {
        cursor_pos.x++;
      }
    }
  } else if (dir == DOWN) {
    if (select_multiple & (copy_selection || move_selection)) {
      if (aabb_move.min.y > 0) {
        aabb_move.min.y--;
        aabb_move.max.y--;
        cursor_pos.y--;
      }
    } else if (cursor_mode == PAINT_MODE && select_multiple && selected_tile) {
      int tile_size = (max_c(g_model->objects_size[selected_tile - 1].h - 1, 0) >> FP) + 1;
      
      #if TILE_HALF_HEIGHT
        tile_size <<= 1;
      #endif
      
      if (cursor_pos.y >= tile_size) {
        cursor_pos.y -= tile_size;
      }
    } else {
      if (i_modifier_key_pressed & KEY_SHIFT) {
        cursor_pos.y -= 4;
        
        if (cursor_pos.y < 0) {
          cursor_pos.y = 0;
        }
      } else if (cursor_pos.y > 0) {
        cursor_pos.y--;
      }
    }
  } else if (dir == UP) {
    if (select_multiple & (copy_selection || move_selection)) {
      if (aabb_move.max.y < MAX_MAP_HEIGHT - 1) {
        aabb_move.min.y++;
        aabb_move.max.y++;
        cursor_pos.y++;
      }
    } else if (cursor_mode == PAINT_MODE && select_multiple && selected_tile) {
      int tile_size = (max_c(g_model->objects_size[selected_tile - 1].h - 1, 0) >> FP) + 1;
      
      #if TILE_HALF_HEIGHT
        tile_size <<= 1;
      #endif
      
      if (cursor_pos.y < MAX_MAP_HEIGHT - tile_size) {
        cursor_pos.y += tile_size;
      }
    } else {
      if (i_modifier_key_pressed & KEY_SHIFT) {
        cursor_pos.y += 4;
        
        if (cursor_pos.y >= MAX_MAP_HEIGHT) {
          cursor_pos.y = MAX_MAP_HEIGHT - 1;
        }
      } else if (cursor_pos.y < MAX_MAP_HEIGHT - 1) {
        cursor_pos.y++;
      }
    }
  }
}

void update_matrix_transforms() {
  set_matrix(camera_matrix, identity_matrix);
  set_matrix(camera_sprite_matrix, identity_matrix);
  
  #if !PERSPECTIVE_ENABLED
    scale_matrix(fix(16), fix(16), fix(16), camera_matrix); // 16
    scale_matrix(fix(16), fix(16), fix(16), camera_sprite_matrix);
  #endif
  
  translate_matrix(-cam.pos.x, -cam.pos.y, -cam.pos.z, camera_matrix);
  rotate_matrix(-cam.rot.x, -cam.rot.y, -cam.rot.z, camera_matrix, 1);
  
  #if ENABLE_GRID_FRUSTUM_CULLING
    set_matrix(frustum_matrix, identity_matrix);
    
    #if DRAW_FRUSTUM
      set_matrix(view_frustum_matrix, identity_matrix);
      #if ADVANCE_FRUSTUM
        translate_matrix(0, 0, 1 << FP, frustum_matrix); // view frustum area
        translate_matrix(0, 0, 1 << FP, view_frustum_matrix);
      #endif
    #endif
    
    rotate_matrix(-cam.rot.x, -cam.rot.y, -cam.rot.z, frustum_matrix, 0);
    #if DRAW_FRUSTUM && FIXED_FRUSTUM
      rotate_matrix(-cam.rot.x, -cam.rot.y, -cam.rot.z, view_frustum_matrix, 0);
    #endif
    
    #if !FIXED_FRUSTUM
      translate_matrix(cam.pos.x, cam.pos.y, cam.pos.z, frustum_matrix);
    #endif
    
    #if DRAW_FRUSTUM
      #if FIXED_FRUSTUM
        transform_matrix(view_frustum_matrix, camera_matrix); // set frustum in origin
       #endif
      transform_frustum(&frustum, &tr_view_frustum, view_frustum_matrix, 0);
    #endif
    
    transform_frustum(&frustum, &tr_frustum, frustum_matrix, 1);
    calc_dist_frustum();
  #endif
  
  //set_matrix_sp(1, 0, 0, 0, camera_sprite_matrix, camera_matrix);
  /* #if ENABLE_GRID_FRUSTUM_CULLING && DRAW_GRID
    change_model(model_id);
  #endif */
}

void draw() {
  // clean the screen
  if (cfg.clean_screen) {
    if (scn.draw_sky) {
      #if PALETTE_MODE
        draw_sky_pal();
      #else
        draw_sky();
      #endif
    } else {
      fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, PAL_BG);
    }
  }
  
  update_matrix_transforms();
  
  #if ENABLE_Z_BUFFER
    // memset32(z_buffer, 0xffffffff, SCREEN_WIDTH * SCREEN_HEIGHT);
    memset32(z_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
    /* for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
      z_buffer[i] = 1.0;
    } */
  #endif
  
  draw_map(g_model);
  
  // draw_scene();
  
  /* if (cursor_enabled) {
    draw_cursor(cursor_pos);
  } */
  
  draw_debug_buffers();
  
  frame_counter++;
}

void draw_scene() {
  r_scene_num_polys = 0;
  r_scene_num_elements_ot = 0;
  
  reset_display_list();
  
  set_matrix(model_matrix, identity_matrix);
  
  if (!g_obj.static_rot) {
    rotate_matrix(g_obj.rot.x, g_obj.rot.y, g_obj.rot.z, model_matrix, 0);
  }
  
  if (!g_obj.static_pos) {
    translate_matrix(g_obj.pos.x, g_obj.pos.y, g_obj.pos.z, model_matrix);
  }
  
  transform_matrix(model_matrix, camera_matrix);
  
  if (g_model->num_sprite_vertices) {
    transform_vertices(camera_sprite_matrix, g_model->sprite_vertices, tr_sprite_vertices, g_model->num_sprite_vertices);
  }
  
  #if ENABLE_GRID_FRUSTUM_CULLING
    if (g_model->has_grid) {
      // profile_start();
      draw_grid(g_obj.pos, g_model); // check grid with frustum culling
      // profile_stop();
      
      if (dl.num_faces) {
        // init the ordering table
        memset32(g_ot_list.pnt, -1, g_ot_list.size >> 1);
        
        transform_model_vis(model_matrix, 0, 1, g_model);
        set_ot_list_vis(g_obj.pos, 1, g_model, &g_ot_list);
        
        if (r_scene_num_elements_ot) {
          #if ENABLE_Z_BUFFER
            draw_ot_list_vis(g_obj.pos, 1, 1, g_model, &g_ot_list); // draw from front to back
          #else
            draw_ot_list_vis(g_obj.pos, 0, 1, g_model, &g_ot_list); // draw from back to front
          #endif
        }
      }
    } else
  #endif
  {
    // init the ordering table
    memset32(g_ot_list.pnt, -1, g_ot_list.size >> 1);
    
    transform_model_vis(model_matrix, 0, 0, g_model);
    set_ot_list_vis(g_obj.pos, 0, g_model, &g_ot_list);
    
    if (r_scene_num_elements_ot) {
      #if ENABLE_Z_BUFFER
        draw_ot_list_vis(g_obj.pos, 1, 0, g_model, &g_ot_list); // draw from front to back
      #else
        draw_ot_list_vis(g_obj.pos, 0, 0, g_model, &g_ot_list); // draw from back to front
      #endif
    }
  }
}

void draw_dbg() {
  char str[20];
  int y_pos = 4;
  
  sprintf_c(str, "%d fps", fps);
  draw_str(4, y_pos, str, PAL_YELLOW);
  sprintf_c(str, "%d ms", (int)(delta_time_f));
  draw_str(4, y_pos += 8, str, PAL_YELLOW);
  sprintf_c(str, "%d %d", cam.rot.x >> 8, cam.rot.y >> 8);
  draw_str(4, y_pos += 8, str, PAL_YELLOW);
  sprintf_c(str, "%d %d %d", cam.pos.x >> FP, cam.pos.y >> FP, cam.pos.z >> FP);
  draw_str(4, y_pos += 8, str, PAL_YELLOW);
  sprintf_c(str, "%d pl", r_scene_num_polys);
  draw_str(4, y_pos += 8, str, PAL_YELLOW);
  
  if (cursor_enabled) {
    y_pos += 8;
    sprintf_c(str, "%d %d %d", cursor_pos.x, cursor_pos.y, cursor_pos.z);
    draw_str(4, y_pos += 8, str, PAL_YELLOW);
    sprintf_c(str, "%d tile id", selected_tile);
    draw_str(4, y_pos += 8, str, PAL_YELLOW);
    sprintf_c(str, "%d total tiles", num_tiles);
    draw_str(4, y_pos += 8, str, PAL_YELLOW);
  }
  
  if (*dbg_screen_output) {
    draw_str(4, y_pos += 8, dbg_screen_output, PAL_YELLOW);
  }
  // sprintf_c(str, "%d", frame_frt);
  // draw_str(4, 68, str, PAL_YELLOW);
  
  if (dbg_show_poly_num) {
    sprintf_c(str, "%d", dbg_num_poly_dsp);
    draw_str(4, y_pos += 8, str, PAL_YELLOW);
  } else
  if (dbg_show_grid_tile_num) {
    sprintf_c(str, "%d", dbg_num_grid_tile_dsp);
    draw_str(4, y_pos += 8, str, PAL_YELLOW);
  }
}

void reset_display_list() {
  #if ENABLE_GRID_FRUSTUM_CULLING
    if (g_model->has_grid) {
      memset8(dl.visible_vt_list, 0, g_model->num_vertices);
    }
  #endif
  
  dl.num_tiles = 0;
  dl.num_faces = 0;
  dl.num_vertices = 0;
  dl.num_objects = 0;
  dl.total_vertices = 0;
}