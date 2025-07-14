#include "common.h"

#if ENABLE_OCCLUSION_CULLING && !EDGE_BUFFER
  RAM_CODE void draw_sprite_occ(g_poly_t *poly) {
    vec4_tx_t vt[2];
    
    int sup_vt = 0;
    
    // obtain the top/left vertex
    
    for (int i = 1; i < poly->num_vertices; i++) {
      if (poly->vertices[i].y <= poly->vertices[sup_vt].y && poly->vertices[i].x <= poly->vertices[sup_vt].x) {
        sup_vt = i;
      }
    }
    
    // set the vertices starting with the top/left vertex in clockwise order
    
    int n = sup_vt; //next vt
    
    vt[0].x = poly->vertices[n].x;
    vt[0].y = poly->vertices[n].y;
    vt[0].u = poly->vertices[n].u;
    vt[0].v = poly->vertices[n].v;
    
    n += 2;
    if (n >= 4) {
      n -= 4;
    }
    
    vt[1].x = poly->vertices[n].x;
    vt[1].y = poly->vertices[n].y;
    vt[1].u = poly->vertices[n].u;
    vt[1].v = poly->vertices[n].v;
    
    // calculate the deltas
    
    fixed dx = (vt[1].x >> FP) - (vt[0].x >> FP);
    // unsigned integer division
    fixed rdx = div_luts(dx) >> 12; // 12 bit result
    fixed dudx = fp_mul32(vt[1].u - vt[0].u, rdx);
    
    fixed dy = (vt[1].y >> FP) - (vt[0].y >> FP);
    // unsigned integer division
    fixed rdy = div_luts(dy) >> 12; // 12 bit result
    fixed dvdy = fp_mul32(vt[1].v - vt[0].v, rdy);
    
    // initialize the side variables
    
    #if ENABLE_SUB_TEXEL_ACC
      fixed dtx = vt[0].x - fp_trunc(vt[0].x);
      // fixed dty = vt[0].y - fp_trunc(vt[0].y);
      fixed su_l = vt[0].u + fp_mul32(dudx, dtx);
    #else
      fixed su_l = vt[0].u;
    #endif
    fixed sv_l = vt[0].v; // + fp_mul32(dvdy, dty);
    
    int sx_l_i = vt[0].x >> FP;
    int sx_r_i = vt[1].x >> FP;
    
    // X clipping
    
    #if POLY_X_CLIPPING
      if (sx_l_i < 0) {
        sx_l_i = 0;
      }
      if (sx_r_i > vp.screen_width) {
        sx_r_i = vp.screen_width;
      }
    #endif
    
    int height = dy;
    int screen_y_offset = vt[0].y >> FP;
    int ocl_buffer_y_offset = screen_y_offset * OC_MAP_SCREEN_WIDTH;
    
    // set the screen offset for the start of the first scanline
    
    u16 *vid_y_offset = screen + screen_y_offset * FRAME_WIDTH;
    
    // Y loop
    
    while (height) {
      fixed su = su_l;
      int sv_i = sv_l >> FP;
      
      int start_seg_x = sx_l_i;
      
      // set the pointers for the start and the end of the scanline
      
      u16 *vid_pnt = vid_y_offset + sx_l_i;
      u16 *end_pnt = vid_y_offset + sx_r_i;
      
      u16 *seg_end_pnt = vid_y_offset + (start_seg_x | OC_SEG_SIZE_S) + 1; // end of the segment
      
      // int ocl_buffer_offset = (screen_y_offset * vp.screen_width + vt[0].x) >> OC_SEG_SIZE_BITS;
      int ocl_buffer_offset = ocl_buffer_y_offset + (start_seg_x >> OC_SEG_SIZE_BITS);
      int ocl_map_y_offset = (screen_y_offset >> OC_MAP_SIZE_BITS) * OC_MAP_SCREEN_WIDTH;
      int ocl_map_offset = ocl_map_y_offset + (start_seg_x >> OC_MAP_SIZE_BITS);
      start_seg_x &= OC_SEG_SIZE_S;
      int ocl_bit_offset = 1 << start_seg_x;
      
      u16 *occ_buf_pnt = occlusion_buffer_bin + ocl_buffer_offset;
      u16 *occ_map_pnt = occlusion_map + ocl_map_offset;
      
      u16 ocl_seg_buffer;
      
      // if the line is part of the start of the scanline
      if (start_seg_x) {
        ocl_seg_buffer = *occ_buf_pnt;
        
        if (ocl_seg_buffer == OC_SEG_FULL_SIZE_S) { // full segment, skip it
          // if the end of the scanline is not in the current segment
          if (seg_end_pnt >= end_pnt) goto segment_loop_exit;
          
          su += dudx * (seg_end_pnt - vid_pnt);
          vid_pnt = seg_end_pnt;
          
          occ_buf_pnt++;
          occ_map_pnt++;
          seg_end_pnt += OC_SEG_SIZE;
          ocl_bit_offset = 1;
        }
      }
      
      // segment loop
      
      while (vid_pnt < end_pnt) {
        ocl_seg_buffer = *occ_buf_pnt;
        
        if (!ocl_seg_buffer) { // empty segment
          if (seg_end_pnt > end_pnt) {
            seg_end_pnt = end_pnt;
          }
          
          int num_pixels_map = 0;
          
          while (vid_pnt < seg_end_pnt) {
            int su_i = su >> FP;
            u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
            
            if (tx_color) { // tx_color != cfg.tx_alpha_cr
              tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
              
              #if PALETTE_MODE
                #if defined(PC)
                  *vid_pnt = pc_palette[tx_color];
                #else
                  *vid_pnt = dup8(tx_color);
                #endif
              #else
                *vid_pnt = poly->cr_palette_tx_idx[tx_color];
              #endif
              
              ocl_seg_buffer |= ocl_bit_offset;
              num_pixels_map++;
            }
            vid_pnt++;
            
            su += dudx;
            ocl_bit_offset <<= 1;
          }
          
          *occ_buf_pnt = ocl_seg_buffer;
          *occ_map_pnt += num_pixels_map;
        } else
        if (ocl_seg_buffer == OC_SEG_FULL_SIZE_S) { // full segment, skip it
          su += dudx << OC_SEG_SIZE_BITS;
          vid_pnt = seg_end_pnt;
        } else { // partially filled segment
          if (seg_end_pnt > end_pnt) {
            seg_end_pnt = end_pnt;
          }
          
          int num_pixels_map = 0;
          while (vid_pnt < seg_end_pnt) {
            if (!(ocl_seg_buffer & ocl_bit_offset)) {
              int su_i = su >> FP;
              u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
              
              if (tx_color) { // tx_color != cfg.tx_alpha_cr
                tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
                
                #if PALETTE_MODE
                  #if defined(PC)
                    *vid_pnt = pc_palette[tx_color];
                  #else
                    *vid_pnt = dup8(tx_color);
                  #endif
                #else
                  *vid_pnt = poly->cr_palette_tx_idx[tx_color];
                #endif
                
                ocl_seg_buffer |= ocl_bit_offset;
                num_pixels_map++;
              }
            }
            vid_pnt++;
            
            su += dudx;
            ocl_bit_offset <<= 1;
          }
          
          *occ_buf_pnt = ocl_seg_buffer;
          *occ_map_pnt += num_pixels_map;
        }
        
        occ_buf_pnt++;
        occ_map_pnt++;
        seg_end_pnt += OC_SEG_SIZE;
        ocl_bit_offset = 1;
      }
      
      segment_loop_exit:
      
      sv_l += dvdy;
      vid_y_offset += FRAME_WIDTH;
      height--;
      ocl_buffer_y_offset += OC_MAP_SCREEN_WIDTH;
      screen_y_offset++;
    }
  }
  
  RAM_CODE void draw_poly_occ(g_poly_t *poly) {
    vec2_t vt[12];
    
    int sup_vt = 0;
    #if CHECK_POLY_HEIGHT
      int inf_vt = 0;
    #endif
    u16 color = poly->color;
    
    // obtain the top and bottom vertices
    
    for (int i = 1; i < poly->num_vertices; i++) {
      if (poly->vertices[i].y < poly->vertices[sup_vt].y) {
        sup_vt = i;
      }
      #if CHECK_POLY_HEIGHT
        if (poly->vertices[i].y > poly->vertices[inf_vt].y) {
          inf_vt = i;
        }
      #endif
    }
    
    // if the polygon doesn't has height return
    
    #if CHECK_POLY_HEIGHT
      if (poly->vertices[sup_vt].y >> FP == poly->vertices[inf_vt].y >> FP) return;
    #endif
    
    // set the vertices starting with the top vertex in clockwise order
    
    int n = sup_vt; // next vt
    if (!poly->flags.is_backface) {
      for (int i = 0; i < poly->num_vertices; i++) {
        vt[i].x = poly->vertices[n].x;
        vt[i].y = poly->vertices[n].y;
        
        n++;
        if (n == poly->num_vertices) {
          n = 0;
        }
      }
    } else {
      for (int i = 0; i < poly->num_vertices; i++) {
        vt[i].x = poly->vertices[n].x;
        vt[i].y = poly->vertices[n].y;
        
        n--;
        if (n < 0) {
          n = poly->num_vertices - 1;
        }
      }
    }
    
    // initialize the edge variables
    
    fixed sx_l, sx_r;
    fixed dxdy_l, dxdy_r;
    
    int curr_vt_l = 0;
    int curr_vt_r = 0;
    
    int height_l = 0;
    int height_r = 0;

    int screen_y_offset = vt[0].y >> FP;
    int ocl_buffer_y_offset = screen_y_offset * OC_MAP_SCREEN_WIDTH;
    
    // set the screen offset for the start of the first scanline
    
    u16 *vid_y_offset = screen + screen_y_offset * FRAME_WIDTH;
    
    // main Y loop
    
    while (1) {
      // next edge found on the left side
      
      while (!height_l) {
        // calculate the edge delta
        
        int next_vt_l = curr_vt_l - 1;
        
        if (next_vt_l < 0) {
          next_vt_l = poly->num_vertices - 1;
        }
        
        height_l = (vt[next_vt_l].y >> FP) - (vt[curr_vt_l].y >> FP);
        
        if (height_l < 0) return;
        
        if (height_l) {
          fixed dx = vt[next_vt_l].x - vt[curr_vt_l].x;
          
          // unsigned integer division
          fixed rdy = div_luts(height_l) >> 12; // 12 bit result
          dxdy_l = fp_mul32(dx, rdy);
        
        // initialize the side variables
        
        fixed dty = vt[curr_vt_l].y - fp_trunc(vt[curr_vt_l].y); // sub-pixel precision
        sx_l = vt[curr_vt_l].x + fp_mul32(dxdy_l, dty);
        }
        
        curr_vt_l = next_vt_l;
      }
      
      // next edge found on the right side
      
      while (!height_r) {
        // calculate the edge delta
        
        int next_vt_r = curr_vt_r + 1;
        
        if (next_vt_r == poly->num_vertices) {
          next_vt_r = 0;
        }
        
        height_r = (vt[next_vt_r].y >> FP) - (vt[curr_vt_r].y >> FP);
        
        if (height_r < 0) return;
        
        if (height_r) {
          fixed dx = vt[next_vt_r].x - vt[curr_vt_r].x;
          
          // unsigned integer division
          fixed rdy = div_luts(height_r) >> 12; // 12 bit result
          dxdy_r = fp_mul32(dx, rdy);
          
          // initialize the side variables
          
          fixed dty = vt[curr_vt_r].y - fp_trunc(vt[curr_vt_r].y); // sub-pixel precision
          sx_r = vt[curr_vt_r].x + fp_mul32(dxdy_r, dty);
        }
        
        curr_vt_r = next_vt_r;
      }
      
      // if the polygon doesn't have height return
      
      if (!height_l && !height_r) return;
      
      // obtain the height to the next vertex on Y
      
      int height = min_c(height_l, height_r);
      
      height_l -= height;
      height_r -= height;
      
      // second Y loop
      
      while (height) {
        int sx_l_i = sx_l >> FP;
        int sx_r_i = sx_r >> FP;
        
        if (sx_l_i >= sx_r_i) goto segment_loop_exit;
        
        // X clipping
        
        #if POLY_X_CLIPPING
          if (sx_l_i < 0) {
            sx_l_i = 0;
          }
          if (sx_r_i > vp.screen_width) {
            sx_r_i = vp.screen_width;
          }
        #endif
        
        int start_seg_x = sx_l_i;
        
        // set the pointers for the start and the end of the scanline
        
        u16 *vid_pnt = vid_y_offset + sx_l_i;
        u16 *end_pnt = vid_y_offset + sx_r_i;
        
        u16 *seg_end_pnt = vid_y_offset + (start_seg_x | OC_SEG_SIZE_S) + 1; // end of the segment
        
        //int ocl_buffer_offset = (screen_y_offset * vp.screen_width + sx_l_i) >> OC_SEG_SIZE_BITS;
        int ocl_buffer_offset = ocl_buffer_y_offset + (start_seg_x >> OC_SEG_SIZE_BITS);
        int ocl_map_y_offset = (screen_y_offset >> OC_MAP_SIZE_BITS) * OC_MAP_SCREEN_WIDTH;
        int ocl_map_offset = ocl_map_y_offset + (start_seg_x >> OC_MAP_SIZE_BITS);
        start_seg_x &= OC_SEG_SIZE_S;
        int ocl_bit_offset = 1 << start_seg_x;
        
        u16 *occ_buf_pnt = occlusion_buffer_bin + ocl_buffer_offset;
        u16 *occ_map_pnt = occlusion_map + ocl_map_offset;
        
        u16 ocl_seg_buffer;
        
        if (start_seg_x) { // the line is part of the start of the scanline
          ocl_seg_buffer = *occ_buf_pnt;
          
          if (!ocl_seg_buffer) { // empty segment
          // if the end of the scanline is in the current segment
            if (seg_end_pnt > end_pnt) {
              seg_end_pnt = end_pnt;
            }
            
            *occ_buf_pnt = ((1 << (seg_end_pnt - vid_pnt)) - 1) << start_seg_x;
            *occ_map_pnt += seg_end_pnt - vid_pnt;
            
            goto empty_seg_scanline_start;
          }
          else if (ocl_seg_buffer == OC_SEG_FULL_SIZE_S) { // full segment, skip it
            if (seg_end_pnt >= end_pnt) goto segment_loop_exit;
            
            vid_pnt = seg_end_pnt;
            
            occ_buf_pnt++;
            occ_map_pnt++;
            seg_end_pnt += OC_SEG_SIZE;
            ocl_bit_offset = 1;
          } else goto partially_filled_segment;
        }
        
        // segment loop
        
        while (vid_pnt < end_pnt) {
          ocl_seg_buffer = *occ_buf_pnt;
          
          if (!ocl_seg_buffer) { // empty segment
            if (seg_end_pnt > end_pnt) { // the line is part of the end of the scanline
              seg_end_pnt = end_pnt;
              *occ_buf_pnt = ((1 << (seg_end_pnt - vid_pnt)) - 1);
              *occ_map_pnt += seg_end_pnt - vid_pnt;
            } else { // the line covers the whole segment
              *occ_buf_pnt = OC_SEG_FULL_SIZE_S;
              *occ_map_pnt += OC_SEG_SIZE;
            }
            
            empty_seg_scanline_start:
            
            while (vid_pnt < seg_end_pnt) {
              *vid_pnt++ = color;
            }
          } else if (ocl_seg_buffer == OC_SEG_FULL_SIZE_S) { // full segment, skip it
            vid_pnt = seg_end_pnt;
          } else { // partially filled segment
            partially_filled_segment:
            
            if (seg_end_pnt > end_pnt) {
              seg_end_pnt = end_pnt;
            }
            
            int num_pixels_map = 0;
            
            while (vid_pnt < seg_end_pnt) {
              if (!(ocl_seg_buffer & ocl_bit_offset)) { //ror & 1
                *vid_pnt = color;
                ocl_seg_buffer |= ocl_bit_offset;
                num_pixels_map++;
              }
              vid_pnt++;
              ocl_bit_offset <<= 1;
              //ocl_seg_buffer >>= 1;
            }
            
            *occ_buf_pnt = ocl_seg_buffer;
            *occ_map_pnt += num_pixels_map;
          }
          
          occ_buf_pnt++;
          occ_map_pnt++;
          seg_end_pnt += OC_SEG_SIZE;
          ocl_bit_offset = 1;
        }
        
        segment_loop_exit:
        
        // increment the left and right side variables
        
        sx_l += dxdy_l;
        sx_r += dxdy_r;
        vid_y_offset += FRAME_WIDTH;
        height--;
        ocl_buffer_y_offset += OC_MAP_SCREEN_WIDTH;
        screen_y_offset++;
      }
    }
  }
  
  RAM_CODE void draw_poly_tx_affine_occ(g_poly_t *poly) {
    vec4_tx_t vt[12];
    
    int sup_vt = 0;
    #if CHECK_POLY_HEIGHT
      int inf_vt = 0;
    #endif
    
    // obtain the top and bottom vertices
    
    for (int i = 1; i < poly->num_vertices; i++) {
      if (poly->vertices[i].y < poly->vertices[sup_vt].y) {
        sup_vt = i;
      }
      #if CHECK_POLY_HEIGHT
        if (poly->vertices[i].y > poly->vertices[inf_vt].y) {
          inf_vt = i;
        }
      #endif
    }
    
    // if the polygon doesn't has height return
    
    #if CHECK_POLY_HEIGHT
      if (poly->vertices[sup_vt].y >> FP == poly->vertices[inf_vt].y >> FP) return;
    #endif
    
    // set the vertices starting with the top vertex in clockwise order
    
    int n = sup_vt; // next vt
    if (!poly->flags.is_backface) {
      for (int i = 0; i < poly->num_vertices; i++) {
        vt[i].x = poly->vertices[n].x;
        vt[i].y = poly->vertices[n].y;
        vt[i].u = poly->vertices[n].u;
        vt[i].v = poly->vertices[n].v;
        
        n++;
        if (n == poly->num_vertices) {
          n = 0;
        }
      }
    } else {
      for (int i = 0; i < poly->num_vertices; i++) {
        vt[i].x = poly->vertices[n].x;
        vt[i].y = poly->vertices[n].y;
        vt[i].u = poly->vertices[n].u;
        vt[i].v = poly->vertices[n].v;
        
        n--;
        if (n < 0) {
          n = poly->num_vertices - 1;
        }
      }
    }
    
    // initialize the edge variables
    
    fixed sx_l, su_l, sv_l, sx_r, su_r, sv_r;
    fixed dxdy_l, dudy_l, dvdy_l, dxdy_r, dudy_r, dvdy_r;
    
    int curr_vt_l = 0;
    int curr_vt_r = 0;
    
    int height_l = 0;
    int height_r = 0;
    
    int screen_y_offset = vt[0].y >> FP;
    int ocl_buffer_y_offset = screen_y_offset * OC_SEG_SCREEN_WIDTH;
    
    // set the screen offset for the start of the first scanline
    
    u16 *vid_y_offset = screen + screen_y_offset * FRAME_WIDTH;
    
    // main Y loop
    
    while (1) {
      // next edge found on the left side
      
      while (!height_l) {
        // calculate the edge delta
        
        int next_vt_l = curr_vt_l - 1;
        
        if (next_vt_l < 0) {
          next_vt_l = poly->num_vertices - 1;
        }
        
        height_l = (vt[next_vt_l].y >> FP) - (vt[curr_vt_l].y >> FP);
        
        if (height_l < 0) return;
        
        if (height_l) {
          fixed dx = vt[next_vt_l].x - vt[curr_vt_l].x;
          fixed du = vt[next_vt_l].u - vt[curr_vt_l].u;
          fixed dv = vt[next_vt_l].v - vt[curr_vt_l].v;
          
          // unsigned integer division
          fixed rdy = div_luts(height_l) >> 12; // 12 bit result
          dxdy_l = fp_mul32(dx, rdy);
          dudy_l = fp_mul32(du, rdy);
          dvdy_l = fp_mul32(dv, rdy);
          
          // initialize the side variables
          
          fixed dty = vt[curr_vt_l].y - fp_trunc(vt[curr_vt_l].y); // sub-pixel precision
          sx_l = vt[curr_vt_l].x + fp_mul32(dxdy_l, dty);
          su_l = vt[curr_vt_l].u + fp_mul32(dudy_l, dty);
          sv_l = vt[curr_vt_l].v + fp_mul32(dvdy_l, dty);
        }
        
        curr_vt_l = next_vt_l;
      }
      
      // next edge found on the right side
      
      while (!height_r) {
        // calculate the edge delta
        
        int next_vt_r = curr_vt_r + 1;
        
        if (next_vt_r == poly->num_vertices) {
          next_vt_r = 0;
        }
        
        height_r = (vt[next_vt_r].y >> FP) - (vt[curr_vt_r].y >> FP);
        
        if (height_r < 0) return;
        
        if (height_r) {
          fixed dx = vt[next_vt_r].x - vt[curr_vt_r].x;
          fixed du = vt[next_vt_r].u - vt[curr_vt_r].u;
          fixed dv = vt[next_vt_r].v - vt[curr_vt_r].v;
          
          // unsigned integer division
          fixed rdy = div_luts(height_r) >> 12; // 12 bit result
          dxdy_r = fp_mul32(dx, rdy);
          dudy_r = fp_mul32(du, rdy);
          dvdy_r = fp_mul32(dv, rdy);
          
          // initialize the side variables
          
          fixed dty = vt[curr_vt_r].y - fp_trunc(vt[curr_vt_r].y); // sub-pixel precision
          sx_r = vt[curr_vt_r].x + fp_mul32(dxdy_r, dty);
          su_r = vt[curr_vt_r].u + fp_mul32(dudy_r, dty);
          sv_r = vt[curr_vt_r].v + fp_mul32(dvdy_r, dty);
        }
        
        curr_vt_r = next_vt_r;
      }
      
      // if the polygon doesn't have height return
      
      if (!height_l && !height_r) return;
      
      // obtain the height to the next vertex on Y
      
      int height = min_c(height_l, height_r);
      
      height_l -= height;
      height_r -= height;
      
      // second Y loop
      
      while (height) {
        int sx_l_i = sx_l >> FP; // ceil
        // int sx_r_i = sx_r >> FP;
        int sx_r_i = fp_ceil_i(sx_r);
        
        if (sx_l_i >= sx_r_i) goto segment_loop_exit;
        
        // calculate the scanline deltas
        
        fixed dudx, dvdx;
        int dx = sx_r_i - sx_l_i;
        if (dx) {
          // fixed rdx = fp_div(1 << FP, dx);
          // unsigned integer division
          fixed rdx = div_luts(dx) >> 12; // 12 bit result
          dudx = fp_mul32(su_r - su_l, rdx);
          dvdx = fp_mul32(sv_r - sv_l, rdx);
        } else {
          dudx = 0;
          dvdx = 0;
        }
        
        // initialize the scanline variables
        
        #if ENABLE_SUB_TEXEL_ACC
          fixed dt = sx_l - fp_trunc(sx_l); // sub-texel precision
          fixed su = su_l + fp_mul32(dudx, dt);
          fixed sv = sv_l + fp_mul32(dvdx, dt);
        #else
          fixed su = su_l;
          fixed sv = sv_l;
        #endif
        
        // X clipping
        
        #if POLY_X_CLIPPING
          if (sx_l_i < 0) {
            sx_l_i = 0;
          }
          if (sx_r_i > vp.screen_width) {
            sx_r_i = vp.screen_width;
          }
        #endif
        
        int start_seg_x = sx_l_i;
        
        // set the pointers for the start and the end of the scanline
        
        u16 *vid_pnt = vid_y_offset + sx_l_i;
        u16 *end_pnt = vid_y_offset + sx_r_i;
        
        u16 *seg_end_pnt = vid_y_offset + (start_seg_x | OC_SEG_SIZE_S) + 1; // end of the segment
        
        // int ocl_buffer_offset = (screen_y_offset * vp.screen_width + sx_l_i) >> OC_SEG_SIZE_BITS;
        int ocl_buffer_offset = ocl_buffer_y_offset + (start_seg_x >> OC_SEG_SIZE_BITS);
        int ocl_map_y_offset = (screen_y_offset >> OC_MAP_SIZE_BITS) * OC_MAP_SCREEN_WIDTH;
        int ocl_map_offset = ocl_map_y_offset + (start_seg_x >> OC_MAP_SIZE_BITS);
        start_seg_x &= OC_SEG_SIZE_S;
        int ocl_bit_offset = 1 << start_seg_x;
        
        u16 *occ_buf_pnt = occlusion_buffer_bin + ocl_buffer_offset;
        u16 *occ_map_pnt = occlusion_map + ocl_map_offset;
        
        u16 ocl_seg_buffer;
        
        // the start of the line is not at the start of the segment
        if (start_seg_x) {
          ocl_seg_buffer = *occ_buf_pnt;
          
          if (ocl_seg_buffer == OC_SEG_FULL_SIZE_S) { // full segment, skip it
            // if the end of the scanline is in the current segment
            if (seg_end_pnt >= end_pnt) goto segment_loop_exit;
            
            su += dudx * (seg_end_pnt - vid_pnt);
            sv += dvdx * (seg_end_pnt - vid_pnt);
            vid_pnt = seg_end_pnt;
            
            occ_buf_pnt++;
            occ_map_pnt++;
            seg_end_pnt += OC_SEG_SIZE;
            ocl_bit_offset = 1;
            
            start_seg_x = 0;
          }
        }
        
        if (!poly->flags.has_transparency) { // face doesn't have transparency
          
          if (start_seg_x) { // the line is part of the start of the scanline
            ocl_seg_buffer = *occ_buf_pnt;
            
            if (!ocl_seg_buffer) { // empty segment
              // if the end of the scanline is in the current segment
              if (seg_end_pnt > end_pnt) {
                seg_end_pnt = end_pnt;
              }
              
              *occ_buf_pnt = ((1 << (seg_end_pnt - vid_pnt)) - 1) << start_seg_x;
              *occ_map_pnt += seg_end_pnt - vid_pnt;
              
              goto empty_seg_scanline_start;
            } else goto partially_filled_segment; // partially filled segment
            // {
            //   ror(ocl_seg_buffer, start_seg_x);
            // }
          }
          
          // segment loop
          
          while (vid_pnt < end_pnt) {
            ocl_seg_buffer = *occ_buf_pnt;
            
            if (!ocl_seg_buffer) { // empty segment
              if (seg_end_pnt > end_pnt) { // the line is part of the end of the scanline
                seg_end_pnt = end_pnt;
                *occ_buf_pnt = ((1 << (seg_end_pnt - vid_pnt)) - 1);
                *occ_map_pnt += seg_end_pnt - vid_pnt;
              } else { // the line covers the whole segment
                *occ_buf_pnt = OC_SEG_FULL_SIZE_S;
                *occ_map_pnt += OC_SEG_SIZE;
              }
              
              empty_seg_scanline_start:
              
              while (vid_pnt < seg_end_pnt) {
                int su_i = su >> FP;
                int sv_i = sv >> FP;
                su_i &= poly->texture_width_s;
                sv_i &= poly->texture_height_s;
                
                u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
                tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
                
                #if PALETTE_MODE
                  #if defined(PC)
                    *vid_pnt++ = pc_palette[tx_color];
                  #else
                    *vid_pnt++ = dup8(tx_color);
                  #endif
                #else
                  *vid_pnt++ = poly->cr_palette_tx_idx[tx_color];
                #endif
                
                su += dudx;
                sv += dvdx;
              }
            } else
            if (ocl_seg_buffer == OC_SEG_FULL_SIZE_S) { // full segment, skip it
              su += dudx << OC_SEG_SIZE_BITS;
              sv += dvdx << OC_SEG_SIZE_BITS;
              vid_pnt = seg_end_pnt;
            } else { // partially filled segment
              partially_filled_segment:
              
              if (seg_end_pnt > end_pnt) {
                // end_pnt_diff = seg_end_pnt - end_pnt;
                seg_end_pnt = end_pnt;
              }
              
              int num_pixels_map = 0;
              
              while (vid_pnt < seg_end_pnt) {
                if (!(ocl_seg_buffer & ocl_bit_offset)) {
                  int su_i = su >> FP;
                  int sv_i = sv >> FP;
                  su_i &= poly->texture_width_s;
                  sv_i &= poly->texture_height_s;
                  
                  u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
                  tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
                  
                  #if PALETTE_MODE
                    #if defined(PC)
                      *vid_pnt = pc_palette[tx_color];
                    #else
                      *vid_pnt = dup8(tx_color);
                    #endif
                  #else
                    *vid_pnt = poly->cr_palette_tx_idx[tx_color];
                  #endif
                  
                  ocl_seg_buffer |= ocl_bit_offset;
                  num_pixels_map++;
                }
                vid_pnt++;
                
                su += dudx;
                sv += dvdx;
                ocl_bit_offset <<= 1;
              }
              
              // if (end_pnt_diff) {
              //   ror(ocl_seg_buffer, end_pnt_diff);
              // }
              
              *occ_buf_pnt = ocl_seg_buffer;
              *occ_map_pnt += num_pixels_map;
            }
            
            occ_buf_pnt++;
            occ_map_pnt++;
            seg_end_pnt += OC_SEG_SIZE;
            ocl_bit_offset = 1;
          }
        } else { // face has transparency
          // if the line is part of the start of the scanline
          // if (start_seg_x) {
          //   ror(ocl_seg_buffer, start_seg_x);
          // }
          
          // segment loop
          
          while (vid_pnt < end_pnt) {
            u16 ocl_seg_buffer = *occ_buf_pnt;
            
            if (!ocl_seg_buffer) { // empty segment
              if (seg_end_pnt > end_pnt) {
                // end_pnt_diff = seg_end_pnt - end_pnt;
                seg_end_pnt = end_pnt;
              }
              
              int num_pixels_map = 0;
              
              while (vid_pnt < seg_end_pnt) {
                int su_i = su >> FP;
                int sv_i = sv >> FP;
                su_i &= poly->texture_width_s;
                sv_i &= poly->texture_height_s;
                
                u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
                
                if (tx_color) { // tx_color != cfg.tx_alpha_cr
                  tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
                  
                  #if PALETTE_MODE
                    #if defined(PC)
                      *vid_pnt = pc_palette[tx_color];
                    #else
                      *vid_pnt = dup8(tx_color);
                    #endif
                  #else
                    *vid_pnt = poly->cr_palette_tx_idx[tx_color];
                  #endif
                  
                  ocl_seg_buffer |= ocl_bit_offset;
                  num_pixels_map++;
                }
                vid_pnt++;
                
                su += dudx;
                sv += dvdx;
                ocl_bit_offset <<= 1;
              }
              
              // if (end_pnt_diff) {
              //   ror(ocl_seg_buffer, end_pnt_diff);
              // }
              
              *occ_buf_pnt = ocl_seg_buffer;
              *occ_map_pnt += num_pixels_map;
            } else
            if (ocl_seg_buffer == OC_SEG_FULL_SIZE_S) { // full segment, skip it
              su += dudx << OC_SEG_SIZE_BITS;
              sv += dvdx << OC_SEG_SIZE_BITS;
              vid_pnt = seg_end_pnt;
            } else { // partially filled segment
              if (seg_end_pnt > end_pnt) {
                // end_pnt_diff = seg_end_pnt - end_pnt;
                seg_end_pnt = end_pnt;
              }
              
              int num_pixels_map = 0;
              
              while (vid_pnt < seg_end_pnt) {
                if (!(ocl_seg_buffer & ocl_bit_offset)) {
                  int su_i = su >> FP;
                  int sv_i = sv >> FP;
                  su_i &= poly->texture_width_s;
                  sv_i &= poly->texture_height_s;
                  
                  u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
                  
                  if (tx_color) { // tx_color != cfg.tx_alpha_cr
                    tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
                    
                    #if PALETTE_MODE
                      #if defined(PC)
                        *vid_pnt = pc_palette[tx_color];
                      #else
                        *vid_pnt = dup8(tx_color);
                      #endif
                    #else
                      *vid_pnt = poly->cr_palette_tx_idx[tx_color];
                    #endif
                    
                    ocl_seg_buffer |= ocl_bit_offset;
                    num_pixels_map++;
                  }
                }
                vid_pnt++;
                
                su += dudx;
                sv += dvdx;
                ocl_bit_offset <<= 1;
              }
              
              // if (end_pnt_diff) {
              //   ror(ocl_seg_buffer, end_pnt_diff);
              // }
              
              *occ_buf_pnt = ocl_seg_buffer;
              *occ_map_pnt += num_pixels_map;
            }
            
            occ_buf_pnt++;
            occ_map_pnt++;
            seg_end_pnt += OC_SEG_SIZE;
            ocl_bit_offset = 1;
          }
        }
        
        segment_loop_exit:
        
        // increment the left and right side variables
        
        sx_l += dxdy_l;
        su_l += dudy_l;
        sv_l += dvdy_l;
        sx_r += dxdy_r;
        su_r += dudy_r;
        sv_r += dvdy_r;
        vid_y_offset += FRAME_WIDTH;
        height--;
        ocl_buffer_y_offset += OC_MAP_SCREEN_WIDTH;
        screen_y_offset++;
      }
    }
  }
  
  #if TX_PERSP_MODE
    RAM_CODE void draw_poly_tx_ps_occ(g_poly_t *poly) {
      vec5_t vt[12];
      
      int sup_vt = 0;
      #if CHECK_POLY_HEIGHT
        int inf_vt = 0;
      #endif
      
      // obtain the top and bottom vertices
      
      for (int i = 1; i < poly->num_vertices; i++) {
        if (poly->vertices[i].y < poly->vertices[sup_vt].y) {
          sup_vt = i;
        }
        #if CHECK_POLY_HEIGHT
          if (poly->vertices[i].y > poly->vertices[inf_vt].y) {
            inf_vt = i;
          }
        #endif
      }
      
      // if the polygon doesn't has height return
      
      #if CHECK_POLY_HEIGHT
        if (poly->vertices[sup_vt].y >> FP == poly->vertices[inf_vt].y >> FP) return;
      #endif
      
      // set the vertices starting with the top vertex in clockwise order
      
      int n = sup_vt; //next vt
      if (!poly->flags.is_backface) {
        for (int i = 0; i < poly->num_vertices; i++) {
          vt[i].x = poly->vertices[n].x;
          vt[i].y = poly->vertices[n].y;
          // calculate the reciprocal of Z and divide U and V by it
          // unsigned fixed point division
          // vt[i].z = fp_div(1 << 8, poly->vertices[n].z);
          vt[i].z = div_luts(poly->vertices[n].z >> 8) << 8; // 16 bit result
          vt[i].u = fp_mul32(poly->vertices[n].u, vt[i].z);
          vt[i].v = fp_mul32(poly->vertices[n].v, vt[i].z);
          
          n++;
          if (n == poly->num_vertices) {
            n = 0;
          }
        }
      } else {
        for (int i = 0; i < poly->num_vertices; i++) {
          vt[i].x = poly->vertices[n].x;
          vt[i].y = poly->vertices[n].y;
          // calculate the reciprocal of Z and divide U and V by it
          // unsigned fixed point division
          // vt[i].z = fp_div(1 << 8, poly->vertices[n].z);
          vt[i].z = div_luts(poly->vertices[n].z >> 8) << 8; // 16 bit result
          vt[i].u = fp_mul32(poly->vertices[n].u, vt[i].z);
          vt[i].v = fp_mul32(poly->vertices[n].v, vt[i].z);
          
          n--;
          if (n < 0) {
            n = poly->num_vertices - 1;
          }
        }
      }
      
      // initialize the edge variables
    
    fixed sx_l, su_l, sv_l, sz_l, sx_r, su_r, sv_r, sz_r;
    fixed dxdy_l, dudy_l, dvdy_l, dzdy_l, dxdy_r, dudy_r, dvdy_r, dzdy_r;
      
      int curr_vt_l = 0;
      int curr_vt_r = 0;
      
      int height_l = 0;
      int height_r = 0;
      
      int screen_y_offset = vt[0].y >> FP;
      int ocl_buffer_y_offset = screen_y_offset * OC_SEG_SCREEN_WIDTH;
      
      // set the screen offset for the start of the first scanline
      
      u16 *vid_y_offset = screen + screen_y_offset * FRAME_WIDTH;
      
      // main Y loop
      
      while (1) {
        // next edge found on the left side
        
        while (!height_l) {
          // calculate the edge delta
          
          int next_vt_l = curr_vt_l - 1;
          
          if (next_vt_l < 0) {
            next_vt_l = poly->num_vertices - 1;
          }
          
          height_l = (vt[next_vt_l].y >> FP) - (vt[curr_vt_l].y >> FP);
          
          if (height_l < 0) return;
          
          if (height_l) {
            fixed dx = vt[next_vt_l].x - vt[curr_vt_l].x;
            fixed du = vt[next_vt_l].u - vt[curr_vt_l].u;
            fixed dv = vt[next_vt_l].v - vt[curr_vt_l].v;
            fixed dz = vt[next_vt_l].z - vt[curr_vt_l].z;
            
            // unsigned integer division
            fixed rdy = div_luts(height_l) >> 12; // 12 bit result
            dxdy_l = fp_mul32(dx, rdy);
            dudy_l = fp_mul32(du, rdy);
            dvdy_l = fp_mul32(dv, rdy);
            dzdy_l = fp_mul32(dz, rdy);
            
            // initialize the side variables
            
            fixed dty = vt[curr_vt_l].y - fp_trunc(vt[curr_vt_l].y); // sub-pixel precision
            sx_l = vt[curr_vt_l].x + fp_mul32(dxdy_l, dty);
            su_l = vt[curr_vt_l].u + fp_mul32(dudy_l, dty);
            sv_l = vt[curr_vt_l].v + fp_mul32(dvdy_l, dty);
            sz_l = vt[curr_vt_l].z + fp_mul32(dzdy_l, dty);
          }
          
          curr_vt_l = next_vt_l;
        }
        
        // next edge found on the right side
        
        while (!height_r) {
          // calculate the edge delta
          
          int next_vt_r = curr_vt_r + 1;
          
          if (next_vt_r == poly->num_vertices) {
            next_vt_r = 0;
          }
          
          height_r = (vt[next_vt_r].y >> FP) - (vt[curr_vt_r].y >> FP);
          
          if (height_r < 0) return;
          
          if (height_r) {
            fixed dx = vt[next_vt_r].x - vt[curr_vt_r].x;
            fixed du = vt[next_vt_r].u - vt[curr_vt_r].u;
            fixed dv = vt[next_vt_r].v - vt[curr_vt_r].v;
            fixed dz = vt[next_vt_r].z - vt[curr_vt_r].z;
            
            // unsigned integer division
            fixed rdy = div_luts(height_r) >> 12; // 12 bit result
            dxdy_r = fp_mul32(dx, rdy);
            dudy_r = fp_mul32(du, rdy);
            dvdy_r = fp_mul32(dv, rdy);
            dzdy_r = fp_mul32(dz, rdy);
            
            // initialize the side variables
            
            fixed dty = vt[curr_vt_r].y - fp_trunc(vt[curr_vt_r].y); // sub-pixel precision
            sx_r = vt[curr_vt_r].x + fp_mul32(dxdy_r, dty);
            su_r = vt[curr_vt_r].u + fp_mul32(dudy_r, dty);
            sv_r = vt[curr_vt_r].v + fp_mul32(dvdy_r, dty);
            sz_r = vt[curr_vt_r].z + fp_mul32(dzdy_r, dty);
          }
          
          curr_vt_r = next_vt_r;
        }
        
        // if the polygon doesn't have height return
        
        if (!height_l && !height_r) return;
        
        // obtain the height to the next vertex on Y
        
        int height = min_c(height_l, height_r);
        
        height_l -= height;
        height_r -= height;
        
        // second Y loop
        
        while (height) {
          int sx_l_i = sx_l >> FP; // ceil
          // int sx_r_i = sx_r >> FP;
          int sx_r_i = fp_ceil_i(sx_r);
          
          if (sx_l_i >= sx_r_i) goto segment_loop_exit;
          
          // calculate the scanline deltas
          
          fixed dudx, dvdx, dzdx;
          int dx = sx_r_i - sx_l_i;
          if (dx) {
            //fixed rdx = fp_div(1 << FP, dx);
            // unsigned integer division
            fixed rdx = div_luts(dx) >> 12; // 12 bit result
            dudx = fp_mul32(su_r - su_l, rdx);
            dvdx = fp_mul32(sv_r - sv_l, rdx);
            dzdx = fp_mul32(sz_r - sz_l, rdx);
          } else {
            dudx = 0;
            dvdx = 0;
            dzdx = 0;
          }
          
          // initialize the scanline variables
          
          #if ENABLE_SUB_TEXEL_ACC
            fixed dt = sx_l - fp_trunc(sx_l); // sub-texel precision
            // screen u, v and z pre-stepped at the segment end storing the start
            fixed s_su_r = su_l + fp_mul32(dudx, dt);
            fixed s_sv_r = sv_l + fp_mul32(dvdx, dt);
            fixed sz = sz_l + fp_mul32(dzdx, dt);
          #else
            fixed s_su_r = su_l;
            fixed s_sv_r = sv_l;
            fixed sz = sz_l;
          #endif
          
          // X clipping
          
          #if POLY_X_CLIPPING
            if (sx_l_i < 0) {
              sx_l_i = 0;
            }
            if (sx_r_i > vp.screen_width) {
              sx_r_i = vp.screen_width;
            }
          #endif
          
          int start_seg_x = sx_l_i;
          
          // set the pointers for the start and the end of the scanline
          
          u16 *vid_pnt = vid_y_offset + sx_l_i;
          u16 *end_pnt = vid_y_offset + sx_r_i;
          
          u16 *seg_end_pnt = vid_y_offset + (start_seg_x | OC_SEG_SIZE_S) + 1; // end of the segment
          
          // int ocl_buffer_offset = (screen_y_offset * vp.screen_width + sx_l_i) >> OC_SEG_SIZE_BITS;
          int ocl_buffer_offset = ocl_buffer_y_offset + (start_seg_x >> OC_SEG_SIZE_BITS);
          int ocl_map_y_offset = (screen_y_offset >> OC_MAP_SIZE_BITS) * OC_MAP_SCREEN_WIDTH;
          int ocl_map_offset = ocl_map_y_offset + (start_seg_x >> OC_MAP_SIZE_BITS);
          start_seg_x &= OC_SEG_SIZE_S;
          int ocl_bit_offset = 1 << start_seg_x;
          
          u16 *occ_buf_pnt = occlusion_buffer_bin + ocl_buffer_offset;
          u16 *occ_map_pnt = occlusion_map + ocl_map_offset;
          
          u16 ocl_seg_buffer = *occ_buf_pnt;
          
          // indicates if the current segment is full to avoid calculating the perspective projection
          int segment_fully_occluded = 0;
          
          fixed p_su_r, p_sv_r;
          if (ocl_seg_buffer == OC_SEG_FULL_SIZE_S) { // full segment, skip it
            // if the end of the scanline is in the current segment
            if (seg_end_pnt >= end_pnt) goto segment_loop_exit;
            
            // if the start of the line is not at the start of the segment
            if (start_seg_x) {
              s_su_r += dudx * (seg_end_pnt - vid_pnt);
              s_sv_r += dvdx * (seg_end_pnt - vid_pnt);
              sz += dzdx * (seg_end_pnt - vid_pnt);
            } else {
              s_su_r += dudx << OC_SEG_SIZE_BITS;
              s_sv_r += dvdx << OC_SEG_SIZE_BITS;
              sz += dzdx << OC_SEG_SIZE_BITS;
            }
            
            vid_pnt = seg_end_pnt;
            occ_buf_pnt++;
            occ_map_pnt++;
            seg_end_pnt += OC_SEG_SIZE;
            ocl_bit_offset = 1;
            
            start_seg_x = 0;
            segment_fully_occluded = 1;
          } else {
            // perspective correct u and v pre-stepped at the segment end storing the start
            // rz = fp_div(1 << FP, sz);
            // unsigned fixed point division
            fixed rsz = div_luts(sz >> 8) << 8;
            p_su_r = fp_mul32(s_su_r, rsz);
            p_sv_r = fp_mul32(s_sv_r, rsz);
          }
          
          int seg_length;
          
          if (!poly->flags.has_transparency) { // face doesn't have transparency
            // if the line is part of the start of the scanline and it's an empty segment
            if (start_seg_x && !ocl_seg_buffer) {
              // if the end of the scanline is in the current segment
              if (seg_end_pnt > end_pnt) {
                seg_end_pnt = end_pnt;
              }
              
              seg_length = seg_end_pnt - vid_pnt;
              
              *occ_buf_pnt = ((1 << seg_length) - 1) << start_seg_x;
              *occ_map_pnt += seg_length;
              
              goto segment_scanline_start;
            }// else {
            //   ror(ocl_seg_buffer, start_seg_x);
            // }
          }
          
          // segment loop
          
          while (vid_pnt < end_pnt) {
            ocl_seg_buffer = *occ_buf_pnt;
            
            if (ocl_seg_buffer == OC_SEG_FULL_SIZE_S) { // full segment, skip it
              s_su_r += dudx << OC_SEG_SIZE_BITS;
              s_sv_r += dvdx << OC_SEG_SIZE_BITS;
              sz += dzdx << OC_SEG_SIZE_BITS;
              vid_pnt = seg_end_pnt;
              
              segment_fully_occluded = 1;
            } else {
              #if VIEW_PERSP_SUB
                if (cfg.debug){ //view perspective subdivision
                  *(vid_pnt - 1) = PAL_BLUE;
                  //set_pixel(j, screen_y_offset);
                }
              #endif
              
              if (segment_fully_occluded) { // the previous segment was fully occluded
                // rz = fp_div(1 << FP, sz);
                // unsigned fixed point division
                fixed rsz = div_luts(sz >> 8) << 8;
                p_su_r = fp_mul32(s_su_r, rsz);
                p_sv_r = fp_mul32(s_sv_r, rsz);
                
                segment_fully_occluded = 0;
              }
              
              if (seg_end_pnt > end_pnt) {
                // end_pnt_diff = seg_end_pnt - end_pnt;
                seg_end_pnt = end_pnt;
              }
              
              seg_length = seg_end_pnt - vid_pnt;
              
              if (!poly->flags.has_transparency && !ocl_seg_buffer) { // empty segment
                if (seg_length < OC_SEG_SIZE) { // the line is part of the end of the scanline
                  *occ_buf_pnt = (1 << seg_length) - 1;
                  *occ_map_pnt += seg_length;
                } else { // the line covers the whole segment
                  *occ_buf_pnt = OC_SEG_FULL_SIZE_S;
                  *occ_map_pnt += OC_SEG_SIZE;
                }
              }
              
              segment_scanline_start:
              
              fixed p_su, p_sv, p_duln, p_dvln;
              if (seg_length < OC_SEG_SIZE) { // line is smaller than the segment size
                s_su_r += dudx * seg_length;
                s_sv_r += dvdx * seg_length;
                sz += dzdx * seg_length;
                
                p_su = p_su_r;
                p_sv = p_sv_r;
                
                // rz = fp_div(1 << FP, sz);
                // unsigned fixed point division
                fixed rsz = div_luts(sz >> 8) << 8;
                p_su_r = fp_mul32(s_su_r, rsz);
                p_sv_r = fp_mul32(s_sv_r, rsz);
                
                // fixed rl = fp_div(1 << FP, seg_length);
                // unsigned integer division
                fixed rc_seg_length = div_luts(seg_length) >> 12;
                p_duln = fp_mul32(p_su_r - p_su, rc_seg_length);
                p_dvln = fp_mul32(p_sv_r - p_sv, rc_seg_length);
              } else { // full segment
                s_su_r += dudx << OC_SEG_SIZE_BITS;
                s_sv_r += dvdx << OC_SEG_SIZE_BITS;
                sz += dzdx << OC_SEG_SIZE_BITS;
                
                p_su = p_su_r;
                p_sv = p_sv_r;
                
                // rz = fp_div(1 << FP, sz);
                // unsigned fixed point division
                fixed rsz = div_luts(sz >> 8) << 8;
                p_su_r = fp_mul32(s_su_r, rsz);
                p_sv_r = fp_mul32(s_sv_r, rsz);
                
                p_duln = (p_su_r - p_su) >> OC_SEG_SIZE_BITS;
                p_dvln = (p_sv_r - p_sv) >> OC_SEG_SIZE_BITS;
              }
              
              if (!poly->flags.has_transparency) { // face doesn't have transparency
                if (!ocl_seg_buffer) { // empty segment
                  while (vid_pnt < seg_end_pnt) {
                    int su_i = p_su >> FP;
                    int sv_i = p_sv >> FP;
                    su_i &= poly->texture_width_s;
                    sv_i &= poly->texture_height_s;
                    
                    u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
                    tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
                    
                    #if PALETTE_MODE
                      #if defined(PC)
                        *vid_pnt++ = pc_palette[tx_color];
                      #else
                        *vid_pnt++ = dup8(tx_color);
                      #endif
                    #else
                      *vid_pnt++ = poly->cr_palette_tx_idx[tx_color];
                    #endif
                    
                    p_su += p_duln;
                    p_sv += p_dvln;
                  }
                } else { // partially filled segment                  
                  int num_pixels_map = 0;
                  
                  while (vid_pnt < seg_end_pnt) {
                    if (!(ocl_seg_buffer & ocl_bit_offset)) {
                      int su_i = p_su >> FP;
                      int sv_i = p_sv >> FP;
                      su_i &= poly->texture_width_s;
                      sv_i &= poly->texture_height_s;
                      
                      u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
                      tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
                      
                      #if PALETTE_MODE
                        #if defined(PC)
                          *vid_pnt = pc_palette[tx_color];
                        #else
                          *vid_pnt = dup8(tx_color);
                        #endif
                      #else
                        *vid_pnt = poly->cr_palette_tx_idx[tx_color];
                      #endif
                      
                      ocl_seg_buffer |= ocl_bit_offset;
                      num_pixels_map++;
                    }
                    vid_pnt++;
                    
                    p_su += p_duln;
                    p_sv += p_dvln;
                    ocl_bit_offset <<= 1;
                  }
                  
                  // if (end_pnt_diff) {
                  //   ror(ocl_seg_buffer, end_pnt_diff);
                  // }
                  
                  *occ_buf_pnt = ocl_seg_buffer;
                  *occ_map_pnt += num_pixels_map;
                }
              } else { // face has transparency
                if (!ocl_seg_buffer) { // empty segment
                  int num_pixels_map = 0;
                  
                  while (vid_pnt < seg_end_pnt) {
                    int su_i = p_su >> FP;
                    int sv_i = p_sv >> FP;
                    su_i &= poly->texture_width_s;
                    sv_i &= poly->texture_height_s;
                    
                    u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
                    
                    if (tx_color) { // tx_color != cfg.tx_alpha_cr
                      tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
                      
                      #if PALETTE_MODE
                        #if defined(PC)
                          *vid_pnt = pc_palette[tx_color];
                        #else
                          *vid_pnt = dup8(tx_color);
                        #endif
                      #else
                        *vid_pnt = poly->cr_palette_tx_idx[tx_color];
                      #endif
                      
                      ocl_seg_buffer |= ocl_bit_offset;
                      num_pixels_map++;
                    }
                    vid_pnt++;
                    
                    p_su += p_duln;
                    p_sv += p_dvln;
                    ocl_bit_offset <<= 1;
                  }
                  
                  // if (end_pnt_diff) {
                  //   ror(ocl_seg_buffer, end_pnt_diff);
                  // }
                  
                  *occ_buf_pnt = ocl_seg_buffer;
                  *occ_map_pnt += num_pixels_map;
                } else { // partially filled segment
                  int num_pixels_map = 0;
                  
                  while (vid_pnt < seg_end_pnt) {
                    if (!(ocl_seg_buffer & ocl_bit_offset)) {
                      int su_i = p_su >> FP;
                      int sv_i = p_sv >> FP;
                      su_i &= poly->texture_width_s;
                      sv_i &= poly->texture_height_s;
                      
                      u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
                      
                      if (tx_color) { // tx_color != cfg.tx_alpha_cr
                        tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
                        
                        #if PALETTE_MODE
                          #if defined(PC)
                            *vid_pnt = pc_palette[tx_color];
                          #else
                            *vid_pnt = dup8(tx_color);
                          #endif
                        #else
                          *vid_pnt = poly->cr_palette_tx_idx[tx_color];
                        #endif
                        
                        ocl_seg_buffer |= ocl_bit_offset;
                        num_pixels_map++;
                      }
                    }
                    vid_pnt++;
                    
                    p_su += p_duln;
                    p_sv += p_dvln;
                    ocl_bit_offset <<= 1;
                  }
                  
                  // if (end_pnt_diff) {
                  //   ror(ocl_seg_buffer, end_pnt_diff);
                  // }
                  
                  *occ_buf_pnt = ocl_seg_buffer;
                  *occ_map_pnt += num_pixels_map;
                }
              }
            }
            
            occ_buf_pnt++;
            occ_map_pnt++;
            seg_end_pnt += OC_SEG_SIZE;
            ocl_bit_offset = 1;
          }
          
          segment_loop_exit:
          
          // increment the left and right side variables
          
          sx_l += dxdy_l;
          su_l += dudy_l;
          sv_l += dvdy_l;
          sz_l += dzdy_l;
          sx_r += dxdy_r;
          su_r += dudy_r;
          sv_r += dvdy_r;
          sz_r += dzdy_r;
          vid_y_offset += FRAME_WIDTH;
          height--;
          ocl_buffer_y_offset += OC_MAP_SCREEN_WIDTH;
          screen_y_offset++;
        }
      }
    }
  #endif
#endif