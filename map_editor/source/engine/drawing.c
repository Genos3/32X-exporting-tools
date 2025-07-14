#include "common.h"

#if !ENABLE_ASM
  RAM_CODE void draw_face_tr(g_poly_t *poly, int face_id) { //draw face transformed
    if (!poly->num_vertices) return;
    
    // view 3d clipping
    
    if (poly->frustum_clip_sides) {
      if (poly->frustum_clip_sides & NEAR_PLANE) {
        clip_poly_plane(poly, 0);
      }
      
      #if ENABLE_FAR_PLANE_CLIPPING
        if (poly->frustum_clip_sides & FAR_PLANE) {
          clip_poly_plane(poly, 1);
        }
      #endif
      
      if (poly->frustum_clip_sides & LEFT_PLANE) {
        clip_poly_plane(poly, 2);
      }
      
      if (poly->frustum_clip_sides & RIGHT_PLANE) {
        clip_poly_plane(poly, 3);
      }
      
      if (poly->frustum_clip_sides & TOP_PLANE) {
        clip_poly_plane(poly, 4);
      }
      
      if (poly->frustum_clip_sides & BOTTOM_PLANE) {
        clip_poly_plane(poly, 5);
      }
      
      if (!poly->num_vertices) return;
    }
    
    // perspective division
    
    for (int i = 0; i < poly->num_vertices; i++) {
      project_vertex(&poly->vertices[i]);
    }
    
    if (cfg.draw_polys && poly->flags.is_visible) {
      if (!poly->flags.has_texture) { // no texture
        #if ENABLE_Z_BUFFER
          draw_poly_zb(poly);
        #else
          draw_poly(poly);
        #endif
      } else if (poly->face_type & SPRITE) { // sprite
        #if ENABLE_Z_BUFFER
          draw_sprite_zb(poly);
        #else
          draw_sprite(poly);
        #endif
      } else { // normal face
        #if TX_PERSP_MODE == 1
          #if ENABLE_Z_BUFFER
            if (cfg.tx_perspective_mapping_enabled) {
              draw_poly_tx_ps_zb(poly);
            } else {
              draw_poly_tx_affine_zb(poly);
            }
          #else
            if (cfg.tx_perspective_mapping_enabled) {
              draw_poly_tx_sub_ps(poly);
            } else {
              draw_poly_tx_affine(poly);
            }
          #endif
        #else
          #if ENABLE_Z_BUFFER
            draw_poly_tx_affine_zb(poly);
          #else
            draw_poly_tx_affine(poly);
          #endif
        #endif
      }
    }
    
    if (curr_selected_tile) {
      draw_lines_poly(poly, PAL_RED);
    } else if (cfg.draw_lines) {
      u16 color;
      if (VIEW_TEST_DIST && cfg.face_enable_persp_tx) {
        color = PAL_BLUE;
      } else {
        color = PAL_MAGENTA;
      }
      #if !defined(PC) && PALETTE_MODE
        color = dup8(color);
      #endif
      draw_lines_poly(poly, color);
    }
    
    if (dbg_show_poly_num) {
      if (dbg_num_poly_dsp == face_id) {
        draw_lines_poly(poly, PAL_RED);
      }
    }
    
    r_scene_num_polys++;
  }
#endif

RAM_CODE void draw_lines_poly(g_poly_t *poly, u16 color) {
  #if ENABLE_Z_BUFFER
    for (int i = 0; i < poly->num_vertices - 1; i++) {
      poly->vertices[i].z *= 0.99;
    }
  #endif
  
  for (int i = 0; i < poly->num_vertices - 1; i++) {
    #if ENABLE_ASM
      draw_line_asm(poly->vertices[i].x, poly->vertices[i].y, poly->vertices[i + 1].x, poly->vertices[i + 1].y, color);
    #else
      #if ENABLE_Z_BUFFER
        draw_line_zb(poly->vertices[i].x, poly->vertices[i].y, poly->vertices[i].z, poly->vertices[i + 1].x, poly->vertices[i + 1].y, poly->vertices[i + 1].z, color);
      #else
        draw_line(poly->vertices[i].x, poly->vertices[i].y, poly->vertices[i + 1].x, poly->vertices[i + 1].y, color);
      #endif
    #endif
  }
  
  #if ENABLE_ASM
    draw_line_asm(poly->vertices[poly->num_vertices - 1].x, poly->vertices[poly->num_vertices - 1].y, poly->vertices[0].x, poly->vertices[0].y, color);
  #else
    #if ENABLE_Z_BUFFER
      draw_line_zb(poly->vertices[poly->num_vertices - 1].x, poly->vertices[poly->num_vertices - 1].y, poly->vertices[poly->num_vertices - 1].z, poly->vertices[0].x, poly->vertices[0].y, poly->vertices[0].z, color);
    #else
      draw_line(poly->vertices[poly->num_vertices - 1].x, poly->vertices[poly->num_vertices - 1].y, poly->vertices[0].x, poly->vertices[0].y , color);
    #endif
  #endif
}

void draw_axis(){
  line_t line;
  line.p0.x = camera_matrix[0];
  line.p0.y = camera_matrix[4];
  line.p0.z = camera_matrix[8];
  
  for (int i = 1; i <= 3; i++) {
    u16 color;
    if (i == 1) {
      color = PAL_RED;
    } else
    if (i == 2) {
      color = PAL_GREEN;
    } else {
      color = PAL_BLUE;
    }
    
    #if !defined(PC) && PALETTE_MODE
      color = dup8(color);
    #endif
    
    line.p1.x = line.p0.x + fp_mul(camera_matrix[i], AXIS_VECTOR_SIZE);
    line.p1.y = line.p0.y + fp_mul(camera_matrix[i + 4], AXIS_VECTOR_SIZE);
    line.p1.z = line.p0.z + fp_mul(camera_matrix[i + 8], AXIS_VECTOR_SIZE);
    
    draw_line_3d(line, color);
  }
}

void draw_line_3d(line_t line, u16 color) {
  if (check_clip_line(&line)) return;
  
  project_vertex(&line.p0);
  project_vertex(&line.p1);
  
  #if !ENABLE_Z_BUFFER
    #if ENABLE_ASM
      draw_line_asm(line.p0.x, line.p0.y, line.p1.x, line.p1.y, color);
    #else
      draw_line(line.p0.x, line.p0.y, line.p1.x, line.p1.y, color);
    #endif
  #else
    draw_line_zb(line.p0.x, line.p0.y, line.p0.z, line.p1.x, line.p1.y, line.p1.z, color);
  #endif
}

void draw_point_3d(vec3_t pt, u16 color) {
  // frustum culling
  
  fixed w = fp_mul(pt.z, vp.screen_side_x_dt);
  fixed h = fp_mul(pt.z, vp.screen_side_y_dt);
  
  if (ENABLE_FAR_PLANE_CLIPPING && pt.z > Z_FAR) return;
  if (pt.z < Z_NEAR || pt.x < -w || pt.x < -w || pt.x > w || pt.y > h) return;
  
  project_vertex(&pt);
  
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      set_pixel(pt.x + j - 1, pt.y + i - 1, color);
    }
  }
}

#if ENABLE_Z_BUFFER
  void draw_z_buffer() {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
      u32 color;
      if (z_buffer[i]) {
        color = fp_div(1 << FP, z_buffer[i]) >> (FP + 2);
      } else {
        color = 31;
      }
      
      if (color > 31) {
        color = 31;
      }
      
      color = 31 - color;
      screen[i] = RGB15(color, color, color);
    }
  }
#endif

void draw_debug_buffers() {
  #if ENABLE_Z_BUFFER && DRAW_Z_BUFFER
    draw_z_buffer();
  #endif
  
  if (cfg.draw_lines) {
    draw_axis();
    #if ENABLE_GRID_FRUSTUM_CULLING && DRAW_FRUSTUM
      draw_frustum(PAL_RED);
    #endif
  }
  /* #if ENABLE_GRID_FRUSTUM_CULLING && DRAW_GRID // && ENABLE_Z_BUFFER
    r_color = PAL_RED;
    draw_map_vis();
  #endif */
  
  // draw the binary coverage buffer grid
  
  #if DRAW_OC_BIN_BUFFER_GRID
    int pos = OC_SEG_SIZE_S;
    
    for (int i = 0; i < OC_MAP_SCREEN_WIDTH; i++) {
      draw_line(pos, 0, pos, SCREEN_HEIGHT, PAL_RED);
      pos += OC_SEG_SIZE;
    }
  #endif
  
  // draw the edge buffer
  
  #if DRAW_OC_PNT_BUFFER
    if (cfg.draw_lines) {
      for (int i = 0; i < FRAME_HEIGHT; i++) {
        for (int j = 0; j < OC_MAP_SCREEN_WIDTH; j++) {
          u16 ocl_pnt_buffer = occlusion_buffer_pnt[i * OC_MAP_SCREEN_WIDTH + j];
          
          if (ocl_pnt_buffer & SEG_PNT_SET) {
            u16 ocl_pnt_offset = ocl_pnt_buffer & SEG_PNT_OFFSET; // offset of the current endpoint (0-7)
            
            u16 color;
            if (ocl_pnt_buffer & SEG_PNT_START) {
              color = PAL_MAGENTA;
            } else {
              color = PAL_BLUE;
            }
            
            screen[i * SCREEN_WIDTH + (j << OC_SEG_SIZE_BITS) + ocl_pnt_offset] = color;
          }
        }
      }
    }
  #endif
  
  // draw the occlusion map
  
  #if DRAW_OC_MAP
    if (cfg.draw_lines) {
      for (int i = 0; i < OC_MAP_SCREEN_HEIGHT; i++) {
        for (int j = 0; j < OC_MAP_SCREEN_WIDTH; j++) {
          int tile = occlusion_map[i * OC_MAP_SCREEN_WIDTH + j];
          
          if (tile) {
            u16 color;
            if (tile < OC_MAP_FULL_BLOCK) {
              color = dup8(PAL_RED);
            } else {
              color = dup8(PAL_BLUE);
            }
            
            fill_rect(j * OC_MAP_WIDTH, i * OC_MAP_WIDTH, OC_MAP_WIDTH, OC_MAP_WIDTH, color);
          }
        }
      }
    }
  #endif
  
  // draw the occlusion map grid
  
  #if DRAW_OC_MAP_GRID
    u16 color = dup8(PAL_RED);
    int pos = OC_MAP_WIDTH - 1;
    
    for (int i = 0; i < OC_MAP_SCREEN_HEIGHT; i++) {
      #if ENABLE_ASM
        draw_line_asm(0, pos << FP, SCREEN_WIDTH << FP, pos << FP, color);
      #else
        draw_line(0, pos << FP, SCREEN_WIDTH << FP, pos << FP, color);
      #endif
      
      pos += OC_MAP_WIDTH;
    }
    
    pos = OC_MAP_WIDTH - 1;
    
    for (int i = 0; i < OC_MAP_SCREEN_WIDTH; i++) {
      #if ENABLE_ASM
        draw_line_asm(pos << FP, 0, pos << FP, SCREEN_HEIGHT << FP, color);
      #else
        draw_line(pos << FP, 0, pos << FP, SCREEN_HEIGHT << FP, color);
      #endif
      
      pos += OC_MAP_WIDTH;
    }
  #endif
}