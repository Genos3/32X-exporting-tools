#include "common.h"

#if ENABLE_OCCLUSION_CULLING && ENABLE_Z_BUFFER
  RAM_CODE void draw_sprite_occ_zb(g_poly_t *poly) {
    vec4_tx_t vt[2];
    
    // obtain the top/left vertex
    
    int sup_vt = 0;
    
    for (int i = 1; i < poly->num_vertices; i++) {
      if (poly->vertices[i].y <= poly->vertices[sup_vt].y && poly->vertices[i].x <= poly->vertices[sup_vt].x) {
        sup_vt = i;
      }
    }
    
    // set the vertices starting with the top/left vertex in clockwise order
    
    fixed sz = poly->vertices[0].z;
    
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
    
    // fixed dty = fp_ceil(vt[0].y) - vt[0].y;
    int dx = vt[1].x - vt[0].x;
    // unsigned integer division
    fixed rdx = div_luts(dx) >> 12; // 12 bit result
    fixed dudx = fp_mul32(vt[1].u - vt[0].u, rdx);
    
    int dy = vt[1].y - vt[0].y;
    // unsigned integer division
    fixed rdy = div_luts(dy) >> 12; // 12 bit result
    fixed dvdy = fp_mul32(vt[1].v - vt[0].v, rdy);
    
    // initialize the side variables
    
    #if 0 && ENABLE_SUB_TEXEL_ACC
      fixed dtx = fp_ceil(vt[0].x) - vt[0].x;
      // fixed dty = fp_ceil(vt[0].y) - vt[0].y;
      fixed su_l = vt[0].u + fp_mul32(dudx, dtx);
    #else
      fixed su_l = vt[0].u;
    #endif
    fixed sv_l = vt[0].v; // + fp_mul32(dvdy, dty);
    
    int height = dy;
    int i = vt[0].y;
    int ocl_buffer_y_offset = i * OC_MAP_SCREEN_WIDTH;
    
    // set the screen offset for the start of the first scanline
    
    u16 *vid_y_offset = screen + i * FRAME_WIDTH;
    
    // Y loop
    
    while (height) {
      fixed su = su_l;
      int sv_i = sv_l >> FP;
      
      // set the pointers for the start and the end of the scanline
      
      u16 *vid_pnt = vid_y_offset + vt[0].x;
      u16 *end_pnt = vid_y_offset + vt[0].x + dx;
      int screen_offset = i * FRAME_WIDTH + vt[0].x;
      
      // scanline loop
      
      // int ocl_buffer_offset = (i * vp.screen_width + vt[0].x) >> OC_SEG_SIZE_BITS;
      int ocl_buffer_offset = ocl_buffer_y_offset + (vt[0].x >> OC_SEG_SIZE_BITS);
      int ocl_bit_offset = 1 << (vt[0].x & OC_SEG_SIZE_S);
      int ocl_map_y_offset = (i >> OC_MAP_SIZE_BITS) * OC_MAP_SCREEN_WIDTH;
      int sx = vt[0].x;
      ocl_buffer_y_offset += OC_MAP_SCREEN_WIDTH;
      u16 *seg_end_pnt = vid_y_offset + (vt[0].x | OC_SEG_SIZE_S) + 1; // end of the segment
      
      while (vid_pnt < end_pnt) {
        u16 ocl_seg_buffer = occlusion_buffer_bin[ocl_buffer_offset];
        int ocl_map_offset = ocl_map_y_offset + (sx >> OC_MAP_SIZE_BITS);
        
        if (seg_end_pnt > end_pnt) {
          seg_end_pnt = end_pnt;
        }
        
        if (!ocl_seg_buffer) { // empty segment
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
              z_buffer[screen_offset] = sz;
              
              ocl_seg_buffer |= ocl_bit_offset;
              num_pixels_map++;
            }
            
            vid_pnt++;
            screen_offset++;
            su += dudx;
            ocl_bit_offset <<= 1;
          }
          
          occlusion_buffer_bin[ocl_buffer_offset] = ocl_seg_buffer;
          occlusion_map[ocl_map_offset] += num_pixels_map;
        } else
        if (ocl_seg_buffer == (1 << OC_SEG_SIZE) - 1) { // full segment
          while (vid_pnt < seg_end_pnt) {
            if (sz < z_buffer[screen_offset]) {
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
                z_buffer[screen_offset] = sz;
              }
            }
            
            vid_pnt++;
            screen_offset++;
            su += dudx;
          }
        } else { // partially filled segment
          int num_pixels_map = 0;
          while (vid_pnt < seg_end_pnt) {
            if (sz < z_buffer[screen_offset]) {
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
                z_buffer[screen_offset] = sz;
                
                if (!(ocl_seg_buffer & ocl_bit_offset)) {
                  ocl_seg_buffer |= ocl_bit_offset;
                  num_pixels_map++;
                }
              }
            }
            
            vid_pnt++;
            screen_offset++;
            su += dudx;
            ocl_bit_offset <<= 1;
          }
          
          occlusion_buffer_bin[ocl_buffer_offset] = ocl_seg_buffer;
          occlusion_map[ocl_map_offset] += num_pixels_map;
        }
        
        ocl_buffer_offset++;
        seg_end_pnt += OC_SEG_SIZE;
        sx = (sx | OC_SEG_SIZE_S) + 1;
        ocl_bit_offset = 1;
      }
      
      sv_l += dvdy;
      vid_y_offset += FRAME_WIDTH;
      height--;
      i++;
    }
  }
  
  RAM_CODE void draw_poly_occ_zb(g_poly_t *poly) {
    typedef struct {
      fixed dxdy, dzdy;
      int dy;
    } dt_t;
    
    vec3_t vt[12];
    dt_t dt[12];
    int sup_vt = 0;
    int inf_vt = 0;
    u16 color = poly->color;
    
    // obtain the top and bottom vertices
    
    for (int i = 1; i < poly->num_vertices; i++) {
      if (poly->vertices[i].y < poly->vertices[sup_vt].y) {
        sup_vt = i;
      }
      if (poly->vertices[i].y > poly->vertices[inf_vt].y) {
        inf_vt = i;
      }
    }
    
    // if the polygon doesn't has height return
    
    if (poly->vertices[sup_vt].y == poly->vertices[inf_vt].y) return;
    
    // set the vertices starting with the top vertex in clockwise order
    
    int n = sup_vt; // next vt
    if (!poly->flags.is_backface) {
      for (int i = 0; i < poly->num_vertices; i++) {
        vt[i].x = poly->vertices[n].x;
        vt[i].y = poly->vertices[n].y;
        vt[i].z = poly->vertices[n].z;
        
        n++;
        if (n == poly->num_vertices) {
          n = 0;
        }
      }
    } else {
      for (int i = 0; i < poly->num_vertices; i++) {
        vt[i].x = poly->vertices[n].x;
        vt[i].y = poly->vertices[n].y;
        vt[i].z = poly->vertices[n].z;
        
        n--;
        if (n < 0) {
          n = poly->num_vertices - 1;
        }
      }
    }
    
    // calculate the edge deltas
    
    n = 1;
    for (int i = 0; i < poly->num_vertices; i++) {
      int dy = vt[n].y - vt[i].y;
      
      if (dy) {
        // dt[i].dxdy = fp_div(vt[i].x - vt[n].x, dy);
        // signed integer division
        fixed rdy = div_luts_s(dy) >> 12; // 24 bit result
        dt[i].dxdy = ((vt[n].x - vt[i].x) * rdy); // .24 - 12 = .12
        dt[i].dzdy = fp_mul32(vt[n].z - vt[i].z, rdy);
        dt[i].dy = dy;
      } else {
        dt[i].dxdy = 0;
        dt[i].dzdy = 0;
        dt[i].dy = 0;
      }
      
      n++;
      if (n == poly->num_vertices) {
        n = 0;
      }
    }
    
    // initialize the side variables
    
    int next_vt_l = poly->num_vertices - 1;
    int next_vt_r = 1;
    
    fixed dxdy_l = dt[next_vt_l].dxdy;
    fixed dzdy_l = dt[next_vt_l].dzdy;
    int height_l = -dt[next_vt_l].dy;
    fixed dxdy_r = dt[0].dxdy;
    fixed dzdy_r = dt[0].dzdy;
    int height_r = dt[0].dy;
    
    #if ENABLE_SUB_PIXEL_ACC
      fixed dty = fp_ceil(vt[0].y) - vt[0].y; // sub-pixel precision
      fixed sx_l = vt[0].x + fp_mul32(dxdy_l, dty);
      fixed sx_r = vt[0].x + fp_mul32(dxdy_r, dty);
      fixed sz_l = vt[0].z + fp_mul32(dzdy_l, dty);
      fixed sz_r = vt[0].z + fp_mul32(dzdy_r, dty);
    #else
      fixed sx_l = vt[0].x << FP;
      fixed sx_r = vt[0].x << FP;
      fixed sz_l = vt[0].z;
      fixed sz_r = vt[0].z;
    #endif

    int i = vt[0].y;
    int ocl_buffer_y_offset = i * OC_MAP_SCREEN_WIDTH;
    
    // set the screen offset for the start of the first scanline
    
    u16 *vid_y_offset = screen + i * FRAME_WIDTH;
    
    // main Y loop
    
    while (1) {
      // next edge found on the left side
      
      if (!height_l) {
        dxdy_l = dt[next_vt_l - 1].dxdy;
        dzdy_l = dt[next_vt_l - 1].dzdy;
        height_l = -dt[next_vt_l - 1].dy;
        
        if (height_l < 0) return;
        
        #if ENABLE_SUB_PIXEL_ACC
          dty = fp_ceil(vt[next_vt_l].y) - vt[next_vt_l].y; // sub-pixel precision
          sx_l = vt[next_vt_l].x + fp_mul32(dxdy_l, dty);
          sz_l = vt[next_vt_l].z + fp_mul32(dzdy_l, dty);
        #else
          sx_l = vt[next_vt_l].x << FP;
          sz_l = vt[next_vt_l].z;
        #endif
        
        next_vt_l--;
      }
      
      // next edge found on the right side
      
      if (!height_r) {
        dxdy_r = dt[next_vt_r].dxdy;
        dzdy_r = dt[next_vt_r].dzdy;
        height_r = dt[next_vt_r].dy;
        
        if (height_r < 0) return;
        
        #if ENABLE_SUB_PIXEL_ACC
          dty = fp_ceil(vt[next_vt_r].y) - vt[next_vt_r].y; // sub-pixel precision
          sx_r = vt[next_vt_r].x + fp_mul32(dxdy_r, dty);
          sz_r = vt[next_vt_r].z + fp_mul32(dzdy_r, dty);
        #else
          sx_r = vt[next_vt_r].x << FP;
          sz_r = vt[next_vt_r].z;
        #endif
        
        next_vt_r++;
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
        
        // calculate the scanline deltas
        
        fixed dzdx;
        int dx = sx_r_i - sx_l_i;
        if (dx) {
          // fixed rdy = fp_div(1 << FP, dx);
          // unsigned integer division
          fixed rdx = div_luts(dx) >> 12; // 24 bit result
          dzdx = fp_mul32(sz_r - sz_l, rdx);
        } else {
          dzdx = 0;
        }
        
        // initialize the scanline variables
        
        #if ENABLE_SUB_TEXEL_ACC
          fixed dt = fp_ceil(sx_l) - sx_l; // sub-texel precision
          fixed sz = sz_l + fp_mul(dzdx, dt);
        #else
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
        
        // set the pointers for the start and the end of the scanline
        
        u16 *vid_pnt = vid_y_offset + sx_l_i;
        u16 *end_pnt = vid_y_offset + sx_r_i;
        int screen_offset = i * FRAME_WIDTH + sx_l_i;
        
        // scanline loop
        
        //int ocl_buffer_offset = (i * vp.screen_width + sx_l_i) >> OC_SEG_SIZE_BITS;
        int ocl_buffer_offset = ocl_buffer_y_offset + (sx_l_i >> OC_SEG_SIZE_BITS);
        int ocl_bit_offset = 1 << (sx_l_i & OC_SEG_SIZE_S);
        int ocl_map_y_offset = (i >> OC_MAP_SIZE_BITS) * OC_MAP_SCREEN_WIDTH;
        int sx = sx_l_i;
        ocl_buffer_y_offset += OC_MAP_SCREEN_WIDTH;
        u16 *seg_end_pnt = vid_y_offset + (sx_l_i | OC_SEG_SIZE_S) + 1; // end of the segment
        
        while (vid_pnt < end_pnt) {
          u16 ocl_seg_buffer = occlusion_buffer_bin[ocl_buffer_offset];
          int ocl_map_offset = ocl_map_y_offset + (sx >> OC_MAP_SIZE_BITS);
          
          if (!ocl_seg_buffer) { // empty segment
            if (ocl_bit_offset == 1 && seg_end_pnt < end_pnt) { // the line covers the whole segment
              occlusion_buffer_bin[ocl_buffer_offset] = (1 << OC_SEG_SIZE) - 1;
              occlusion_map[ocl_map_offset] += OC_SEG_SIZE;
            } else { // the line is part of the start or the end of the scanline
              if (seg_end_pnt > end_pnt) {
                seg_end_pnt = end_pnt;
              }
              occlusion_buffer_bin[ocl_buffer_offset] = ((1 << (seg_end_pnt - vid_pnt)) - 1) << (sx & OC_SEG_SIZE_S);
              occlusion_map[ocl_map_offset] += seg_end_pnt - vid_pnt;
            }
            
            while (vid_pnt < seg_end_pnt) {
              *vid_pnt++ = color;
              z_buffer[screen_offset] = sz;
              screen_offset++;
              sz += dzdx;
            }
          } else
          if (ocl_seg_buffer == (1 << OC_SEG_SIZE) - 1) { // full segment
            if (seg_end_pnt > end_pnt) {
              seg_end_pnt = end_pnt;
            }
            
            while (vid_pnt < seg_end_pnt) {
              if (sz < z_buffer[screen_offset]) {
                *vid_pnt = color;
                z_buffer[screen_offset] = sz;
              }
              
              vid_pnt++;
              screen_offset++;
              sz += dzdx;
            }
          } else { // partially filled segment
            if (seg_end_pnt > end_pnt) {
              seg_end_pnt = end_pnt;
            }
            
            int num_pixels_map = 0;
            while (vid_pnt < seg_end_pnt) {
              if (sz < z_buffer[screen_offset]) {
                *vid_pnt = color;
                z_buffer[screen_offset] = sz;
                
                if (!(ocl_seg_buffer & ocl_bit_offset)) { //ror & 1
                  ocl_seg_buffer |= ocl_bit_offset;
                  num_pixels_map++;
                }
              }
              
              vid_pnt++;
              screen_offset++;
              sz += dzdx;
              ocl_bit_offset <<= 1;
            }
            
            occlusion_buffer_bin[ocl_buffer_offset] = ocl_seg_buffer;
            occlusion_map[ocl_map_offset] += num_pixels_map;
          }
          
          ocl_buffer_offset++;
          seg_end_pnt += OC_SEG_SIZE;
          sx = (sx | OC_SEG_SIZE_S) + 1;
          ocl_bit_offset = 1;
        }
        
        // increment the left and right side variables
        
        sx_l += dxdy_l;
        sx_r += dxdy_r;
        sz_l += dzdy_l;
        sz_r += dzdy_r;
        vid_y_offset += FRAME_WIDTH;
        height--;
        i++;
      }
    }
  }
  
  RAM_CODE void draw_poly_tx_affine_occ_zb(g_poly_t *poly) {
    typedef struct {
      fixed dxdy, dudy, dvdy, dzdy;
      int dy;
    } dt_t;
    
    vec5_t vt[12];
    dt_t dt[12];
    int sup_vt = 0;
    int inf_vt = 0;
    
    // obtain the top and bottom vertices
    
    for (int i = 1; i < poly->num_vertices; i++) {
      if (poly->vertices[i].y < poly->vertices[sup_vt].y) {
        sup_vt = i;
      }
      if (poly->vertices[i].y > poly->vertices[inf_vt].y) {
        inf_vt = i;
      }
    }
    
    // if the polygon doesn't has height return
    
    if (poly->vertices[sup_vt].y == poly->vertices[inf_vt].y) return;
    
    // set the vertices starting with the top vertex in clockwise order
    
    int n = sup_vt; // next vt
    if (!poly->flags.is_backface) {
      for (int i = 0; i < poly->num_vertices; i++) {
        vt[i].x = poly->vertices[n].x;
        vt[i].y = poly->vertices[n].y;
        vt[i].z = poly->vertices[n].z;
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
        vt[i].z = poly->vertices[n].z;
        vt[i].u = poly->vertices[n].u;
        vt[i].v = poly->vertices[n].v;
        
        n--;
        if (n < 0) {
          n = poly->num_vertices - 1;
        }
      }
    }
    
    // calculate the edge deltas
    
    n = 1;
    for (int i = 0; i < poly->num_vertices; i++) {
      int dy = vt[n].y - vt[i].y;
      
      if (dy) {
        // fixed rdy = fp_div(1 << FP, dy);
        // signed integer division
        fixed rdy = div_luts_s(dy) >> 12; // 24 bit result
        dt[i].dxdy = ((vt[n].x - vt[i].x) * rdy);
        dt[i].dudy = fp_mul32(vt[n].u - vt[i].u, rdy);
        dt[i].dvdy = fp_mul32(vt[n].v - vt[i].v, rdy);
        dt[i].dzdy = fp_mul32(vt[n].z - vt[i].z, rdy);
        dt[i].dy = dy;
      } else {
        dt[i].dxdy = 0;
        dt[i].dudy = 0;
        dt[i].dvdy = 0;
        dt[i].dzdy = 0;
        dt[i].dy = 0;
      }
      
      n++;
      if (n == poly->num_vertices) {
        n = 0;
      }
    }
    
    // initialize the side variables
    
    int next_vt_l = poly->num_vertices - 1;
    int next_vt_r = 1;
    
    fixed dxdy_l = dt[next_vt_l].dxdy;
    fixed dudy_l = dt[next_vt_l].dudy;
    fixed dvdy_l = dt[next_vt_l].dvdy;
    fixed dzdy_l = dt[next_vt_l].dzdy;
    int height_l = -dt[next_vt_l].dy;
    fixed dxdy_r = dt[0].dxdy;
    fixed dudy_r = dt[0].dudy;
    fixed dvdy_r = dt[0].dvdy;
    fixed dzdy_r = dt[0].dzdy;
    int height_r = dt[0].dy;
    
    #if ENABLE_SUB_PIXEL_ACC
      fixed dty = fp_ceil(vt[0].y) - vt[0].y; // sub-pixel precision
      fixed sx_l = vt[0].x + fp_mul32(dxdy_l, dty);
      fixed su_l = vt[0].u + fp_mul(dudy_l, dty);
      fixed sv_l = vt[0].v + fp_mul(dvdy_l, dty);
      fixed sz_l = vt[0].z + fp_mul32(dzdy_l, dty);
      fixed sx_r = vt[0].x + fp_mul32(dxdy_r, dty);
      fixed su_r = vt[0].u + fp_mul(dudy_r, dty);
      fixed sv_r = vt[0].v + fp_mul(dvdy_r, dty);
      fixed sz_r = vt[0].z + fp_mul32(dzdy_r, dty);
    #else
      fixed sx_l = vt[0].x << FP;
      fixed su_l = vt[0].u;
      fixed sv_l = vt[0].v;
      fixed sz_l = vt[0].z;
      fixed sx_r = vt[0].x << FP;
      fixed su_r = vt[0].u;
      fixed sv_r = vt[0].v;
      fixed sz_r = vt[0].z;
    #endif
    
    int i = vt[0].y;
    int ocl_buffer_y_offset = i * OC_MAP_SCREEN_WIDTH;
    
    // set the screen offset for the start of the first scanline
    
    u16 *vid_y_offset = screen + i * FRAME_WIDTH;
    
    // main Y loop
    
    while (1) {
      // next edge found on the left side
      
      if (!height_l) {
        dxdy_l = dt[next_vt_l - 1].dxdy;
        dudy_l = dt[next_vt_l - 1].dudy;
        dvdy_l = dt[next_vt_l - 1].dvdy;
        dzdy_l = dt[next_vt_l - 1].dzdy;
        height_l = -dt[next_vt_l - 1].dy;
        
        if (height_l < 0) return;
        
        #if ENABLE_SUB_PIXEL_ACC
          dty = fp_ceil(vt[next_vt_l].y) - vt[next_vt_l].y; // sub-pixel precision
          sx_l = vt[next_vt_l].x + fp_mul32(dxdy_l, dty);
          su_l = vt[next_vt_l].u + fp_mul(dudy_l, dty);
          sv_l = vt[next_vt_l].v + fp_mul(dvdy_l, dty);
          sz_l = vt[next_vt_l].z + fp_mul32(dzdy_l, dty);
        #else
          sx_l = vt[next_vt_l].x << FP;
          su_l = vt[next_vt_l].u;
          sv_l = vt[next_vt_l].v;
          sz_l = vt[next_vt_l].z;
        #endif
        
        next_vt_l--;
      }
      
      // next edge found on the right side
      
      if (!height_r) {
        dxdy_r = dt[next_vt_r].dxdy;
        dudy_r = dt[next_vt_r].dudy;
        dvdy_r = dt[next_vt_r].dvdy;
        dzdy_r = dt[next_vt_r].dzdy;
        height_r = dt[next_vt_r].dy;
        
        if (height_r < 0) return;
        
        #if ENABLE_SUB_PIXEL_ACC
          dty = fp_ceil(vt[next_vt_r].y) - vt[next_vt_r].y; // sub-pixel precision
          sx_r = vt[next_vt_r].x + fp_mul32(dxdy_r, dty);
          su_r = vt[next_vt_r].u + fp_mul(dudy_r, dty);
          sv_r = vt[next_vt_r].v + fp_mul(dvdy_r, dty);
          sz_r = vt[next_vt_r].z + fp_mul32(dzdy_r, dty);
        #else
          sx_r = vt[next_vt_r].x << FP;
          su_r = vt[next_vt_r].u;
          sv_r = vt[next_vt_r].v;
          sz_r = vt[next_vt_r].z;
        #endif
        
        next_vt_r++;
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
        int sx_r_i = sx_r >> FP;
        
        // calculate the scanline deltas
        
        fixed dudx, dvdx, dzdx;
        int dx = sx_r_i - sx_l_i;
        if (dx) {
          // fixed rdy = fp_div(1 << FP, dx);
          // unsigned integer division
          fixed rdx = div_luts(dx) >> 12; // 24 bit result
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
          fixed dt = fp_ceil(sx_l) - sx_l; // sub-texel precision
          fixed su = su_l + fp_mul(dudx, dt);
          fixed sv = sv_l + fp_mul(dvdx, dt);
          fixed sz = sz_l + fp_mul(dzdx, dt);
        #else
          fixed su = su_l;
          fixed sv = sv_l;
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
        
        // set the pointers for the start and the end of the scanline
        
        u16 *vid_pnt = vid_y_offset + sx_l_i;
        u16 *end_pnt = vid_y_offset + sx_r_i;
        int screen_offset = i * FRAME_WIDTH + sx_l_i;
        
        // scanline loop
        
        // int ocl_buffer_offset = (i * vp.screen_width + sx_l_i) >> OC_SEG_SIZE_BITS;
        int ocl_buffer_offset = ocl_buffer_y_offset + (sx_l_i >> OC_SEG_SIZE_BITS);
        int ocl_bit_offset = 1 << (sx_l_i & OC_SEG_SIZE_S);
        int ocl_map_y_offset = (i >> OC_MAP_SIZE_BITS) * OC_MAP_SCREEN_WIDTH;
        int sx = sx_l_i;
        ocl_buffer_y_offset += OC_MAP_SCREEN_WIDTH;
        u16 *seg_end_pnt = vid_y_offset + (sx_l_i | OC_SEG_SIZE_S) + 1; // end of the segment
        
        if (!poly->flags.has_transparency) {
          while (vid_pnt < end_pnt) {
            u16 ocl_seg_buffer = occlusion_buffer_bin[ocl_buffer_offset];
            int ocl_map_offset = ocl_map_y_offset + (sx >> OC_MAP_SIZE_BITS);
            
            if (!ocl_seg_buffer) { // empty segment
              if (ocl_bit_offset == 1 && seg_end_pnt < end_pnt) { // the line covers the whole segment
                occlusion_buffer_bin[ocl_buffer_offset] = (1 << OC_SEG_SIZE) - 1;
                occlusion_map[ocl_map_offset] += OC_SEG_SIZE;
              } else { // the line is part of the start or the end of the scanline
                if (seg_end_pnt > end_pnt) {
                  seg_end_pnt = end_pnt;
                }
                occlusion_buffer_bin[ocl_buffer_offset] = ((1 << (seg_end_pnt - vid_pnt)) - 1) << (sx & OC_SEG_SIZE_S);
                occlusion_map[ocl_map_offset] += seg_end_pnt - vid_pnt;
              }
              
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
                z_buffer[screen_offset] = sz;
                
                screen_offset++;
                su += dudx;
                sv += dvdx;
                sz += dzdx;
              }
            } else
            if (ocl_seg_buffer == (1 << OC_SEG_SIZE) - 1) { // full segment
              if (sz - z_buffer[screen_offset] > FP >> 3) { // skip it
                screen_offset += seg_end_pnt - vid_pnt;
                su += dudx * (seg_end_pnt - vid_pnt);
                sv += dvdx * (seg_end_pnt - vid_pnt);
                sz += dzdx * (seg_end_pnt - vid_pnt);
                vid_pnt = seg_end_pnt;
              } else {
                if (seg_end_pnt > end_pnt) {
                  seg_end_pnt = end_pnt;
                }
                
                while (vid_pnt < seg_end_pnt) {
                  if (sz < z_buffer[screen_offset]) {
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
                    z_buffer[screen_offset] = sz;
                  }
                  
                  vid_pnt++;
                  screen_offset++;
                  su += dudx;
                  sv += dvdx;
                  sz += dzdx;
                }
              }
            } else { // partially filled segment
              if (seg_end_pnt > end_pnt) {
                seg_end_pnt = end_pnt;
              }
              
              int num_pixels_map = 0;
              while (vid_pnt < seg_end_pnt) {
                if (sz < z_buffer[screen_offset]) {
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
                  z_buffer[screen_offset] = sz;
                  
                  if (!(ocl_seg_buffer & ocl_bit_offset)) {
                    ocl_seg_buffer |= ocl_bit_offset;
                    num_pixels_map++;
                  }
                }
                
                vid_pnt++;
                screen_offset++;
                su += dudx;
                sv += dvdx;
                sz += dzdx;
                ocl_bit_offset <<= 1;
              }
              
              occlusion_buffer_bin[ocl_buffer_offset] = ocl_seg_buffer;
              occlusion_map[ocl_map_offset] += num_pixels_map;
            }
            
            ocl_buffer_offset++;
            seg_end_pnt += OC_SEG_SIZE;
            sx = (sx | OC_SEG_SIZE_S) + 1;
            ocl_bit_offset = 1;
          }
        } else {
          while (vid_pnt < end_pnt) {
            u16 ocl_seg_buffer = occlusion_buffer_bin[ocl_buffer_offset];
            int ocl_map_offset = ocl_map_y_offset + (sx >> OC_MAP_SIZE_BITS);
            
            if (seg_end_pnt > end_pnt) {
              seg_end_pnt = end_pnt;
            }
            
            if (!ocl_seg_buffer) { // empty segment
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
                  z_buffer[screen_offset] = sz;
                  
                  ocl_seg_buffer |= ocl_bit_offset;
                  num_pixels_map++;
                }
                
                vid_pnt++;
                screen_offset++;
                su += dudx;
                sv += dvdx;
                sz += dzdx;
                ocl_bit_offset <<= 1;
              }
              
              occlusion_buffer_bin[ocl_buffer_offset] = ocl_seg_buffer;
              occlusion_map[ocl_map_offset] += num_pixels_map;
            } else
            if (ocl_seg_buffer == (1 << OC_SEG_SIZE) - 1) { // full segment
              while (vid_pnt < seg_end_pnt) {
                if (sz < z_buffer[screen_offset]) {
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
                    z_buffer[screen_offset] = sz;
                  }
                }
                
                vid_pnt++;
                screen_offset++;
                su += dudx;
                sv += dvdx;
                sz += dzdx;
              }
            } else { // partially filled segment
              int num_pixels_map = 0;
              while (vid_pnt < seg_end_pnt) {
                if (sz < z_buffer[screen_offset]) {
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
                    z_buffer[screen_offset] = sz;
                    
                    if (!(ocl_seg_buffer & ocl_bit_offset)) {
                      ocl_seg_buffer |= ocl_bit_offset;
                      num_pixels_map++;
                    }
                  }
                }
                
                vid_pnt++;
                screen_offset++;
                su += dudx;
                sv += dvdx;
                sz += dzdx;
                ocl_bit_offset <<= 1;
              }
              
              occlusion_buffer_bin[ocl_buffer_offset] = ocl_seg_buffer;
              occlusion_map[ocl_map_offset] += num_pixels_map;
            }
            
            ocl_buffer_offset++;
            seg_end_pnt += OC_SEG_SIZE;
            sx = (sx | OC_SEG_SIZE_S) + 1;
            ocl_bit_offset = 1;
          }
        }
        
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
        i++;
      }
    }
  }
#endif