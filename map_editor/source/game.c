#include "common.h"

fixed calc_slope_height(int test_dir, vec3_t *map_cam_pos, vec3i_t *map_cam_tile, int tile_index);

#define BLOCK 1
#define UP_SLOPE 2
#define RIGHT_SLOPE 3
#define DOWN_SLOPE 4
#define LEFT_SLOPE 5
#define WATER 6

#define TYPE_SLOPE 2

map_scn_t map_scn;
game_t game;
map_cl_t *map_cl;
u8 animation_frames[2];

const u8 flower_anim_frames[] = {
  0, 1
};

const u8 water_anim_frames[] = {
  0, 1, 2, 1
};

static const u8 cl_tile_types[] = {0,1,2,2,2,2,3};

void init_game() {
  map_scn.tile_size = 1 << FP; //16
  map_scn.tile_height = fix(0.5); //8
  map_scn.half_tile_size = map_scn.tile_size >> 1;
  map_scn.rc_tile_size = fp_div(1 << FP, map_scn.tile_size);
  map_scn.rc_tile_height = fp_div(1 << FP, map_scn.tile_height);
  map_scn.tile_radius = fp_mul(map_scn.tile_size >> 1, DIAG_UNIT_DIST_RC); //rc_fp_div_luts_a
  map_scn.slope = fp_mul(map_scn.tile_height, map_scn.rc_tile_size);
  
  game.anim.general_timer = 0;
  game.anim.flower_texture_id = 0;
  game.anim.water_texture_id = 0;
  
  scn.cam_curr_map = 0;
  scn.curr_model = 0;
  game.collision_enabled = 1;
  game.cam_is_on_ground = 0;
  game.gravity_enabled = 1;
  game.gravity = fix(-0.20); // -0.15
  game.cam_speed_y_limit = fix(-20);
  
  // change_map(scn.cam_curr_map);
  // check_tile_collision();
  shared_face_dir = calloc(g_model->num_faces, 1);
  set_shared_faces_list(g_model);
}

void check_tile_collision() {
  vec3_t map_cam_pos;
  map_cam_pos.x = cam.pos.x;
  map_cam_pos.y = cam.pos.y - CL_HEIGHT;
  map_cam_pos.z = cam.pos.z;
  
  // if the center point is outside of the map return
  
  if (map_cam_pos.x < 0 || map_cam_pos.x >= g_model->size.w || map_cam_pos.z < 0 || map_cam_pos.z >= g_model->size.d) return;
  
  vec3i_t map_cam_tile;
  map_cam_tile.x = fp_mul(map_cam_pos.x, map_scn.rc_tile_size) >> FP;
  map_cam_tile.y = fp_mul(map_cam_pos.y, map_scn.rc_tile_height) >> FP;
  map_cam_tile.z = fp_mul(map_cam_pos.z, map_scn.rc_tile_size) >> FP;
  // sprintf_c(dbg_screen_output, "%d %d %d", map_cam_tile.x, map_cam_tile.y, map_cam_tile.z);
  
  fixed x_dist = 0;
  fixed z_dist = 0;
  int test_dir_x = 0;
  int test_dir_z = 0;
  int cl_dir_x = 0;
  int cl_dir_z = 0;
  
  int curr_tile_index = map_cam_tile.z * map_cl->width + map_cam_tile.x;
  int curr_cl_tile = map_cl->cl_tiles[curr_tile_index];
  
  // code for the floor collision
  
  // retrieve the maximum height for the 4 posible tiles the player is standing on
  
  fixed fl_max_height = 0;
  
  // check the center tile
  
  if (cl_tile_types[map_cl->cl_tiles[curr_tile_index]] == TYPE_SLOPE) {
    fl_max_height = calc_slope_height(0, &map_cam_pos, &map_cam_tile, curr_tile_index);
  } else {
    fl_max_height = map_cl->fl_height[curr_tile_index] * map_scn.tile_height;
  }
  
  // sides
  
  if (!cl_dir_x) {
    if (test_dir_x == 1) {
      int tile_index = curr_tile_index - 1;
      int cl_tile = map_cl->cl_tiles[tile_index];
      
      // if the tile is a slope calculate the height for it else return the height for the floor
      
      if (cl_tile_types[cl_tile] == TYPE_SLOPE) {
        if (cl_tile == UP_SLOPE || cl_tile == DOWN_SLOPE) {
          fixed slope_height = calc_slope_height(1, &map_cam_pos, &map_cam_tile, tile_index);
          
          if (fl_max_height < slope_height) {
            fl_max_height = slope_height;
          }
        }
      } else { // the tile is not a slope
        if (curr_cl_tile != LEFT_SLOPE && curr_cl_tile != RIGHT_SLOPE) {
          fixed fl_height = map_cl->fl_height[tile_index] * map_scn.tile_height;
          
          if (fl_max_height < fl_height) {
            fl_max_height = fl_height;
          }
        }
      }
    } else
    if (test_dir_x == 2) {
      int tile_index = curr_tile_index + 1;
      int cl_tile = map_cl->cl_tiles[tile_index];
      if (cl_tile_types[cl_tile] == TYPE_SLOPE) {
        if (cl_tile == UP_SLOPE || cl_tile == DOWN_SLOPE) {
          fixed slope_height = calc_slope_height(2, &map_cam_pos, &map_cam_tile, tile_index);
          
          if (fl_max_height < slope_height) {
            fl_max_height = slope_height;
          }
        }
      } else { // the tile is not a slope
        if (curr_cl_tile != LEFT_SLOPE && curr_cl_tile != RIGHT_SLOPE) {
          fixed fl_height = map_cl->fl_height[tile_index] * map_scn.tile_height;
          
          if (fl_max_height < fl_height) {
            fl_max_height = fl_height;
          }
        }
      }
    }
  }
  
  if (!cl_dir_z) {
    if (test_dir_z == 1) {
      int tile_index = curr_tile_index - map_cl->width;
      int cl_tile = map_cl->cl_tiles[tile_index];
      if (cl_tile_types[cl_tile] == TYPE_SLOPE) {
        if (cl_tile == LEFT_SLOPE || cl_tile == RIGHT_SLOPE) {
          fixed slope_height = calc_slope_height(3, &map_cam_pos, &map_cam_tile, tile_index);
          
          if (fl_max_height < slope_height) {
            fl_max_height = slope_height;
          }
        }
      } else { // the tile is not a slope
        if (curr_cl_tile != UP_SLOPE && curr_cl_tile != DOWN_SLOPE) {
          fixed fl_height = map_cl->fl_height[curr_tile_index - map_cl->width] * map_scn.tile_height;
          if (fl_max_height < fl_height) {
            fl_max_height = fl_height;
          }
        }
      }
    } else
    if (test_dir_z == 2) {
      int tile_index = curr_tile_index + map_cl->width;
      int cl_tile = map_cl->cl_tiles[tile_index];
      if (cl_tile_types[cl_tile] == TYPE_SLOPE) {
        if (cl_tile == LEFT_SLOPE || cl_tile == RIGHT_SLOPE) {
          fixed slope_height = calc_slope_height(4, &map_cam_pos, &map_cam_tile, tile_index);
          
          if (fl_max_height < slope_height) {
            fl_max_height = slope_height;
          }
        }
      } else { // the tile is not a slope
        if (curr_cl_tile != UP_SLOPE && curr_cl_tile != DOWN_SLOPE) {
          fixed fl_height = map_cl->fl_height[curr_tile_index + map_cl->width] * map_scn.tile_height;
          
          if (fl_max_height < fl_height) {
            fl_max_height = fl_height;
          }
        }
      }
    }
  }
  
  // diagonals
  
  if (!cl_dir_x && !cl_dir_z) {
    if (cl_tile_types[map_cl->cl_tiles[curr_tile_index]] != TYPE_SLOPE) {
      if (test_dir_x == 1) {
        if (test_dir_z == 1) {
          fixed fl_height = map_cl->fl_height[curr_tile_index - map_cl->width - 1] * map_scn.tile_height;
          
          if (fl_max_height < fl_height) {
            fl_max_height = fl_height;
          }
        } else
        if (test_dir_z == 2) {
          fixed fl_height = map_cl->fl_height[curr_tile_index + map_cl->width - 1] * map_scn.tile_height;
          
          if (fl_max_height < fl_height) {
            fl_max_height = fl_height;
          }
        }
      } else
      if (test_dir_x == 2) {
        if (test_dir_z == 1) {
          fixed fl_height = map_cl->fl_height[curr_tile_index - map_cl->width + 1] * map_scn.tile_height;
          
          if (fl_max_height < fl_height) {
            fl_max_height = fl_height;
          }
        } else
        if (test_dir_z == 2) {
          fixed fl_height = map_cl->fl_height[curr_tile_index + map_cl->width + 1] * map_scn.tile_height;
          
          if (fl_max_height < fl_height) {
            fl_max_height = fl_height;
          }
        }
      }
    }
  }
  
  // check if the camera is in the air
  
  if (game.gravity_enabled && map_cam_pos.y > fl_max_height + fix(0.1)) {
    game.cam_is_on_ground = 0;
  } else {
    cam.pos.y = fl_max_height + CL_HEIGHT;
    map_cam_pos.y = cam.pos.y - CL_HEIGHT;
    map_cam_tile.y = fp_mul(map_cam_pos.y, map_scn.rc_tile_height) >> FP;
    
    if (!game.cam_is_on_ground) {
      cam.speed.y = 0;
      game.cam_is_on_ground = 1;
    }
  }
  
  // code for the walls collision
  
  // if the center of the camera is inside a solid tile
  
  if (curr_cl_tile == BLOCK) {
    int free_dir_x = 0;
    int free_dir_z = 0;
    
    // if the center point is on the left side of the tile
    
    if (map_cam_pos.x < (map_cam_tile.x << FP) + map_scn.half_tile_size) {
      // if the tile to the left is free
      if (map_cam_tile.x > 0 && !map_cl->cl_tiles[curr_tile_index - 1]) {
        free_dir_x = 1;
      }
      test_dir_x = 1;
      
      // calculate the distance to the left border
      
      x_dist = map_cam_pos.x - (map_cam_tile.x << FP) + CL_DIST;
    } else {
      if (map_cam_tile.x < map_cl->width - 1 && !map_cl->cl_tiles[curr_tile_index + 1]) {
        free_dir_x = 1;
      }
      test_dir_x = 2;
      x_dist = ((map_cam_tile.x + 1) << FP) - map_cam_pos.x + CL_DIST;
    }
    
    if (map_cam_pos.z < (map_cam_tile.z << FP) + map_scn.half_tile_size) {
      if (map_cam_tile.z > 0 && !map_cl->cl_tiles[curr_tile_index - map_cl->width]) {
        free_dir_z = 1;
      }
      test_dir_z = 1;
      z_dist = map_cam_pos.z - (map_cam_tile.z << FP) + CL_DIST;
    } else {
      if (map_cam_tile.z < map_cl->depth - 1 && !map_cl->cl_tiles[curr_tile_index + map_cl->width]) {
        free_dir_z = 1;
      }
      test_dir_z = 2;
      z_dist = ((map_cam_tile.z + 1) << FP) - map_cam_pos.z + CL_DIST;
    }
    
    // if there's a free tile in one of the sides select the closer edge
    
    if (free_dir_x || free_dir_z) {
      if (x_dist < z_dist) {
        if (free_dir_x) {
          cl_dir_x = test_dir_x ^ 3;
        } else {
          cl_dir_z = test_dir_z ^ 3;
        }
      } else {
        if (free_dir_z) {
          cl_dir_z = test_dir_z ^ 3;
        } else {
          cl_dir_x = test_dir_x ^ 3;
        }
      }
    } else {
      
      // diagonal collision
      
      if (test_dir_x == 1) {
        if (test_dir_z == 1 && !map_cl->cl_tiles[curr_tile_index - map_cl->width - 1]) {
          cl_dir_x = 2;
          cl_dir_z = 2;
        } else
        if (test_dir_z == 2 && !map_cl->cl_tiles[curr_tile_index + map_cl->width - 1]) {
          cl_dir_x = 2;
          cl_dir_z = 1;
        }
      } else {
        if (test_dir_z == 1 && !map_cl->cl_tiles[curr_tile_index - map_cl->width + 1]) {
          cl_dir_x = 1;
          cl_dir_z = 2;
        } else
        if (test_dir_z == 2 && !map_cl->cl_tiles[curr_tile_index + map_cl->width + 1]) {
          cl_dir_x = 1;
          cl_dir_z = 1;
        }
      }
    }
  } else {
    
    // if the side of the camera collision box crosses the adyacent tile
    
    if (map_cam_tile.x > 0 && map_cam_pos.x - CL_DIST < map_cam_tile.x << FP) {
      test_dir_x = 1;
      
      // if the adyacent tile is full or if it is at an higher level and the current tile is not a slope in the same direction
      
      int tile_index = curr_tile_index - 1;
      if (map_cl->cl_tiles[tile_index] == BLOCK ||
          // current camera tile height is lower than adyacent tile height
          (map_cam_tile.y < map_cl->fl_height[tile_index] &&
           curr_cl_tile != LEFT_SLOPE &&
           curr_cl_tile != WATER) ||
          // current camera tile height plus one is lower than adyacent tile height
          map_cam_tile.y + 1 < map_cl->fl_height[tile_index]) {
        cl_dir_x = 1;
        x_dist = (map_cam_tile.x << FP) - (map_cam_pos.x - CL_DIST);
      }
    } else
    if (map_cam_tile.x < map_cl->width - 1 && map_cam_pos.x + CL_DIST > (map_cam_tile.x + 1) << FP) {
      test_dir_x = 2;
      int tile_index = curr_tile_index + 1;
      if (map_cl->cl_tiles[tile_index] == BLOCK ||
          (map_cam_tile.y < map_cl->fl_height[tile_index] &&
           curr_cl_tile != RIGHT_SLOPE &&
           curr_cl_tile != WATER) ||
          map_cam_tile.y + 1 < map_cl->fl_height[tile_index]) {
        cl_dir_x = 2;
        x_dist = map_cam_pos.x + CL_DIST - ((map_cam_tile.x + 1) << FP);
      }
    }
    
    if (map_cam_tile.z > 0 && map_cam_pos.z - CL_DIST < map_cam_tile.z << FP) {
      test_dir_z = 1;
      int tile_index = curr_tile_index - map_cl->width;
      if (map_cl->cl_tiles[tile_index] == BLOCK ||
          (map_cam_tile.y < map_cl->fl_height[tile_index] &&
           curr_cl_tile != UP_SLOPE &&
           curr_cl_tile != WATER) ||
          map_cam_tile.y + 1 < map_cl->fl_height[tile_index]) {
        cl_dir_z = 1;
        z_dist = (map_cam_tile.z << FP) - (map_cam_pos.z - CL_DIST);
      }
    } else
    if (map_cam_tile.z < map_cl->depth - 1 && map_cam_pos.z + CL_DIST > (map_cam_tile.z + 1) << FP) {
      test_dir_z = 2;
      int tile_index = curr_tile_index + map_cl->width;
      if (map_cl->cl_tiles[tile_index] == BLOCK ||
          (map_cam_tile.y < map_cl->fl_height[tile_index] &&
           curr_cl_tile != DOWN_SLOPE &&
           curr_cl_tile != WATER) ||
          map_cam_tile.y + 1 < map_cl->fl_height[tile_index]) {
        cl_dir_z = 2;
        z_dist = map_cam_pos.z + CL_DIST - ((map_cam_tile.z + 1) << FP);
      }
    }
    
    // sprintf_c(dbg_screen_output, "%d", cl_dir_x);
    
    // diagonal collision
    
    if (!cl_dir_x && !cl_dir_z) {
      int cl_diag = 0;
      if (test_dir_x == 1) {
        if (test_dir_z == 1) {
          int tile_index = curr_tile_index - map_cl->width - 1;
          if (map_cl->cl_tiles[tile_index] == BLOCK ||
              // current camera tile height is lower than adyacent tile height
              (map_cam_tile.y < map_cl->fl_height[tile_index] &&
               curr_cl_tile != LEFT_SLOPE &&
               curr_cl_tile != UP_SLOPE &&
               curr_cl_tile != WATER) ||
              // current camera tile height plus one is lower than adyacent tile height
              map_cam_tile.y + 1 < map_cl->fl_height[tile_index]) {
            cl_diag = 1;
          }
        } else
        if (test_dir_z == 2) {
          int tile_index = curr_tile_index + map_cl->width - 1;
          if (map_cl->cl_tiles[tile_index] == BLOCK ||
              (map_cam_tile.y < map_cl->fl_height[tile_index] &&
               curr_cl_tile != LEFT_SLOPE &&
               curr_cl_tile != DOWN_SLOPE &&
               curr_cl_tile != WATER) ||
              map_cam_tile.y + 1 < map_cl->fl_height[tile_index]) {
            cl_diag = 1;
          }
        }
      } else
      if (test_dir_x == 2) {
        if (test_dir_z == 1) {
          int tile_index = curr_tile_index - map_cl->width + 1;
          if (map_cl->cl_tiles[tile_index] == BLOCK ||
              (map_cam_tile.y < map_cl->fl_height[tile_index] &&
               curr_cl_tile != RIGHT_SLOPE &&
               curr_cl_tile != UP_SLOPE &&
               curr_cl_tile != WATER) ||
              map_cam_tile.y + 1 < map_cl->fl_height[tile_index]) {
            cl_diag = 1;
          }
        } else
        if (test_dir_z == 2) {
          int tile_index = curr_tile_index + map_cl->width + 1;
          if (map_cl->cl_tiles[tile_index] == BLOCK ||
              (map_cam_tile.y < map_cl->fl_height[tile_index] &&
               curr_cl_tile != RIGHT_SLOPE &&
               curr_cl_tile != DOWN_SLOPE &&
               curr_cl_tile != WATER) ||
              map_cam_tile.y + 1 < map_cl->fl_height[tile_index]) {
            cl_diag = 1;
          }
        }
      }
      
      // calculate the distance to the edges and select the closer one
      
      if (cl_diag) {
        if (test_dir_x == 1) {
          x_dist = (map_cam_tile.x << FP) - (map_cam_pos.x - CL_DIST);
        } else
        if (test_dir_x == 2) {
          x_dist = map_cam_pos.x + CL_DIST - ((map_cam_tile.x + 1) << FP);
        }
        
        if (test_dir_z == 1) {
          z_dist = (map_cam_tile.z << FP) - (map_cam_pos.z - CL_DIST);
        } else
        if (test_dir_z == 2) {
          z_dist = map_cam_pos.z + CL_DIST - ((map_cam_tile.z + 1) << FP);
        }
        
        if (x_dist < z_dist) {
          if (test_dir_x == 1) {
            cl_dir_x = 1;
          } else
          if (test_dir_x == 2) {
            cl_dir_x = 2;
          }
        } else {
          if (test_dir_z == 1) {
            cl_dir_z = 1;
          } else
          if (test_dir_z == 2) {
            cl_dir_z = 2;
          }
        }
      }
    }
  }
  
  // move the camera outside the wall
  
  if (cl_dir_x == 1) {
    cam.pos.x += x_dist;
  } else
  if (cl_dir_x == 2) {
    cam.pos.x -= x_dist;
  }
  
  if (cl_dir_z == 1) {
    cam.pos.z += z_dist;
  } else
  if (cl_dir_z == 2) {
    cam.pos.z -= z_dist;
  }
}

// return the height for the slope given a certain point on it

fixed calc_slope_height(int test_dir, vec3_t *map_cam_pos, vec3i_t *map_cam_tile, int tile_index) {
  int cl_tile = map_cl->cl_tiles[tile_index];
  
  vec2i_t cam_tile;
  cam_tile.x = map_cam_tile->x;
  cam_tile.y = map_cam_tile->z;
  
  switch (test_dir) {
    case 1:
      cam_tile.x--;
      break;
    case 2:
      cam_tile.x++;
      break;
    case 3:
      cam_tile.y--;
      break;
    case 4:
      cam_tile.y++;
      break;
  }
  
  fixed dist = 0;
  
  switch (cl_tile) {
    case UP_SLOPE:
      dist = ((cam_tile.y + 1) << FP) - map_cam_pos->z;
      break;
    case DOWN_SLOPE:
      dist = map_cam_pos->z - (cam_tile.y << FP);
      break;
    case LEFT_SLOPE:
      dist = ((cam_tile.x + 1) << FP) - map_cam_pos->x;
      break;
    case RIGHT_SLOPE:
      dist = map_cam_pos->x - (cam_tile.x << FP);
      break;
  }
  
  return map_cl->fl_height[tile_index] * map_scn.tile_height + fp_mul(dist, map_scn.slope);
}