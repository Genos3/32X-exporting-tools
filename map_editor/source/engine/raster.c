#include "common.h"

#if !ENABLE_ASM
  RAM_CODE void draw_line(fixed x0, fixed y0, fixed x1, fixed y1, u16 color) {
    // if x0 > x1 exchange the vertices in order to always draw from left to right
    
    if (x0 > x1) {
      int t = x1;
      x1 = x0;
      x0 = t;
      t = y1;
      y1 = y0;
      y0 = t;
    }
    
    if (x0 >= SCREEN_WIDTH_FP) x0--;
    if (x1 >= SCREEN_WIDTH_FP) x1--;
    if (y0 >= SCREEN_HEIGHT_FP) y0--;
    if (y1 >= SCREEN_HEIGHT_FP) y1--;
    
    uint adx_i = (x1 >> FP) - (x0 >> FP);
    int dy_i = (y1 >> FP) - (y0 >> FP);
    
    if (!adx_i && !dy_i) return;
    
    int vid_y_inc;
    if (dy_i >= 0) {
      vid_y_inc = SCREEN_WIDTH;
    } else {
      vid_y_inc = -SCREEN_WIDTH;
    }
    
    uint ady_i = abs_c(dy_i);
    
    int y0_i = y0 >> FP;
    
    u16 *vid_y_offset = screen + y0_i * SCREEN_WIDTH;
    
    if (ady_i >= adx_i) { // dy > dx
      fixed dxdy;
      if (ady_i && adx_i) {
        // unsigned integer division
        // fixed rdy = div_lut[ady_i]; // .16 result
        // dxdy = adx_i * rdy; // always positive
        dxdy = fp_div(adx_i, ady_i);
      } else {
        dxdy = 0;
      }
      
      int height = ady_i;
      
      fixed dty = y0 - fp_trunc(y0); // sub-pixel precision
      fixed sx = x0 + fp_mul(dxdy, dty);
      
      while (height >= 0) {
        uint sx_i = sx >> FP;
        
        if (sx_i >= SCREEN_WIDTH) sx_i = SCREEN_WIDTH - 1;
        
        *(vid_y_offset + sx_i) = color;
        
        sx += dxdy;
        vid_y_offset += vid_y_inc;
        
        height--;
      }
    } else { // dx > dy
      fixed dydx;
      if (ady_i && adx_i) {
        // unsigned integer division
        // fixed rdx = div_lut[adx_i]; // .16 result
        // dydx = ady_i * rdx; // always positive
        dydx = fp_div(ady_i, adx_i);
      } else {
        dydx = 0;
      }
      
      int width = adx_i;
      
      fixed dtx = x0 - fp_trunc(x0); // sub-pixel precision
      fixed sy = 0 + fp_mul(dydx, dtx);
      
      int x0_i = x0 >> FP;
      
      u16 *vid_pnt = vid_y_offset + x0_i;
      
      while (width > 0) {
        *vid_pnt++ = color;
        
        sy += dydx;
        
        if (sy >= 1 << FP) {
          sy -= 1 << FP;
          vid_pnt += vid_y_inc;
        }
        
        width--;
      }
    }
  }
#endif

void set_pixel(int x, int y, u16 color) {
  // if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  
  // u32 screen_offset = y * SCREEN_WIDTH + x;
  #if defined(PC) && PALETTE_MODE
    screen[y * SCREEN_WIDTH + x] = pc_palette[color];
  #else
    screen[y * SCREEN_WIDTH + x] = color;
  #endif
}

RAM_CODE void fill_rect(int x, int y, int width, int height, u16 color) {
  u16 *vid_y_offset = screen + y * SCREEN_WIDTH + x;
  
  for (int i = 0; i < height; i++) {
    memset16(vid_y_offset, color, width);
    vid_y_offset += SCREEN_WIDTH;
  }
}

#if !ENABLE_ASM
  #ifdef PC
    #define set_pixel_sprite() \
      u32 su_i = su >> FP; \
      u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i]; \
      \
      if (tx_color) { \
        tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor; \
        \
        *vid_pnt = poly->cr_palette_tx_idx[(u8)tx_color]; \
      } \
      \
      vid_pnt++; \
      \
      su += dudx;
  #else
    #define set_pixel_sprite(shift) \
      u32 su_i = su >> FP; \
      u16 tx_color = poly->texture_image[(sv_i << (shift)) + su_i]; \
      \
      if (tx_color) { \
        tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor; \
        \
        *vid_pnt = tx_color; \
      } \
      \
      vid_pnt++; \
      \
      su += dudx;
  #endif
  
  RAM_CODE void draw_sprite(g_poly_t *poly) {
    // obtain the top/left vertex
    
    u32 vt_0 = 0;
    
    for (int i = 1; i < 4; i++) {
      if (poly->vertices[i].y <= poly->vertices[vt_0].y && poly->vertices[i].x <= poly->vertices[vt_0].x) {
        vt_0 = i;
      }
    }
    
    u32 vt_1 = vt_0 + 2;
    if (vt_1 >= 4) {
      vt_1 -= 4;
    }
    
    // calculate the deltas
    
    u32 dx = (poly->vertices[vt_1].x >> FP) - (poly->vertices[vt_0].x >> FP);
    
    // unsigned integer division
    fixed rdx = fp_div(1, dx);
    // fixed rdx = div_lut[dx]; // + 1 // .16 result
    fixed dudx = fp_mul((poly->vertices[vt_1].u - poly->vertices[vt_0].u), rdx);
    
    u32 dy = (poly->vertices[vt_1].y >> FP) - (poly->vertices[vt_0].y >> FP);
    
    // unsigned integer division
    fixed rdy = fp_div(1, dy);
    // fixed rdy = div_lut[dy]; // .16 result
    fixed dvdy = fp_mul((poly->vertices[vt_1].v - poly->vertices[vt_0].v), rdy);
    
    // initialize the side variables
    
    #if ENABLE_SUB_TEXEL_ACC
      fixed dtx = poly->vertices[vt_0].x - fp_trunc(poly->vertices[vt_0].x);
      fixed_u su_l = poly->vertices[vt_0].u + fp_mul(dudx, dtx);
    #else
      fixed_u su_l = poly->vertices[vt_0].u;
    #endif
    fixed_u sv_l = poly->vertices[vt_0].v; // + fp_mul(dvdy, dty);
    
    u32 height = dy;
    
    #if 0 && POLY_X_CLIPPING
      if (poly->vertices[vt_0].x < 0) {
        dx += poly->vertices[vt_0].x >> FP;
        poly->vertices[vt_0].x = 0;
      }
      if (poly->vertices[vt_1].x > SCREEN_WIDTH_FP) {
        dx -= (poly->vertices[vt_1].x >> FP) - SCREEN_WIDTH;
      }
    #endif
    
    // set the screen offset for the start of the first scanline
    
    u32 y0_i = poly->vertices[vt_0].y >> FP;
    u32 x0_i = poly->vertices[vt_0].x >> FP;
    
    u16 *vid_y_offset = screen + y0_i * SCREEN_WIDTH + x0_i;
    
    // Y loop
    
    while (height > 0) {
      fixed_u su = su_l;
      u32 sv_i = sv_l >> FP;
      
      // set the pointers for the start and the end of the scanline
      
      u16 *vid_pnt = vid_y_offset;
      u16 *end_pnt = vid_y_offset + dx;
      
      // scanline loop
      
      #ifdef PC
        while (vid_pnt < end_pnt) {
          set_pixel_sprite();
        }
      #else
        switch (poly->texture_width_bits) {
          case 3:
            while (vid_pnt < end_pnt) {
              set_pixel_sprite(3); // 8
            }
            break;
          
          case 4:
            while (vid_pnt < end_pnt) {
              set_pixel_sprite(4); // 16
            }
            break;
          
          case 5:
            while (vid_pnt < end_pnt) {
              set_pixel_sprite(5); // 32
            }
            break;
        }
      #endif
      
      sv_l += dvdy;
      vid_y_offset += SCREEN_WIDTH;
      height--;
    }
  }
  
  RAM_CODE void draw_poly(g_poly_t *poly) {
    u16 color = poly->color;
    
    // obtain the top and bottom vertices
    
    int sup_vt = 0;
    #if CHECK_POLY_HEIGHT
      int inf_vt = 0;
    #endif
    
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
    
    // if the polygon doesn't have height return
    
    #if CHECK_POLY_HEIGHT
      if (poly->vertices[sup_vt].y >> FP == poly->vertices[inf_vt].y >> FP) return;
    #endif
    
    // initialize the edge variables
    
    fixed sx_l, sx_r;
    fixed dxdy_l, dxdy_r;
    
    int curr_vt_l = sup_vt;
    int curr_vt_r = sup_vt;
    
    int height_l = 0;
    int height_r = 0;
    
    // set the screen offset for the start of the first scanline
    
    u32 y0_i = poly->vertices[sup_vt].y >> FP;
    
    u16 *vid_y_offset = screen + y0_i * SCREEN_WIDTH;
    
    // main Y loop
    
    while (1) {
      // next edge found on the left side
      
      while (!height_l) {
        int next_vt_l;
        
        if (!poly->flags.is_backface) {
          next_vt_l = curr_vt_l - 1;
        
          if (next_vt_l < 0) {
            next_vt_l = poly->num_vertices - 1;
          }
        } else {
          next_vt_l = curr_vt_l + 1;
          
          if (next_vt_l == poly->num_vertices) {
            next_vt_l = 0;
          }
        }
        
        height_l = (poly->vertices[next_vt_l].y >> FP) - (poly->vertices[curr_vt_l].y >> FP);
        
        if (height_l < 0) return;
        
        if (height_l) {
          // calculate the edge deltas
          
          fixed dx = poly->vertices[next_vt_l].x - poly->vertices[curr_vt_l].x;
          
          // unsigned integer division
          // fixed rdy = div_lut[height_l]; // .16 result
          // dxdy_l = fp_mul(dx, rdy);
          dxdy_l = fp_div(dx, height_l);
          
          // initialize the side variables
          
          fixed dty = poly->vertices[curr_vt_l].y - fp_trunc(poly->vertices[curr_vt_l].y); // sub-pixel precision
          sx_l = poly->vertices[curr_vt_l].x + fp_mul(dxdy_l, dty);
        }
        
        curr_vt_l = next_vt_l;
      }
      
      // next edge found on the right side
      
      while (!height_r) {
        int next_vt_r;
        
        if (!poly->flags.is_backface) {
          next_vt_r = curr_vt_r + 1;
          
          if (next_vt_r == poly->num_vertices) {
            next_vt_r = 0;
          }
        } else {
          next_vt_r = curr_vt_r - 1;
        
          if (next_vt_r < 0) {
            next_vt_r = poly->num_vertices - 1;
          }
        }
        
        height_r = (poly->vertices[next_vt_r].y >> FP) - (poly->vertices[curr_vt_r].y >> FP);
        
        if (height_r < 0) return;
        
        if (height_r) {
          // calculate the edge deltas
          
          fixed dx = poly->vertices[next_vt_r].x - poly->vertices[curr_vt_r].x;
          
          // unsigned integer division
          // fixed rdy = div_lut[height_r]; // .16 result
          // dxdy_r = fp_mul(dx, rdy);
          dxdy_r = fp_div(dx, height_r);
          
          // initialize the side variables
          
          fixed dty = poly->vertices[curr_vt_r].y - fp_trunc(poly->vertices[curr_vt_r].y); // sub-pixel precision
          sx_r = poly->vertices[curr_vt_r].x + fp_mul(dxdy_r, dty);
        }
        
        curr_vt_r = next_vt_r;
      }
      
      // if the polygon doesn't have height return
      
      if (!height_l && !height_r) return;
      
      // obtain the height to the next vertex on Y
      
      u32 height = min_c(height_l, height_r);
      
      height_l -= height;
      height_r -= height;
      
      // second Y loop
      
      while (height > 0) {
        int sx_l_i = sx_l >> FP;
        int sx_r_i = sx_r >> FP;
        
        if (sx_l_i >= sx_r_i) goto segment_loop_exit;
        
        // X clipping
        
        #if POLY_X_CLIPPING
          if (sx_l_i < 0) {
            sx_l_i = 0;
          }
          if (sx_r_i > SCREEN_WIDTH) {
            sx_r_i = SCREEN_WIDTH;
          }
        #endif
        
        // set the pointers for the start and the end of the scanline
        
        u16 *vid_pnt = vid_y_offset + sx_l_i;
        
        // scanline loop
        
        #if AUTO_FILL
          u32 length = sx_r_i - sx_l_i;
          
          if (length) {
            u16 offset = (u16)(vid_pnt - screen + 0x100);
            while (MARS_VDP_FBCTL & MARS_VDP_FEN);
            MARS_VDP_FILLEN = length - 1;
            MARS_VDP_FILADR = offset;
            MARS_VDP_FILDAT = color;
          }
        #else
          u16 *end_pnt = vid_y_offset + sx_r_i;
          
          #pragma GCC unroll 4
          while (vid_pnt < end_pnt) {
            *vid_pnt++ = color;
          }
        #endif
        
        segment_loop_exit:
        
        // increment the left and right side variables
        
        sx_l += dxdy_l;
        sx_r += dxdy_r;
        vid_y_offset += SCREEN_WIDTH;
        height--;
      }
    }
  }
  
  #ifdef PC
    #define set_pixel_tx_affine() \
      u32 su_i = su >> FP; \
      u32 sv_i = sv >> FP; \
      su_i &= poly->texture_width_s; \
      sv_i &= poly->texture_height_s; \
      \
      u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i]; \
      tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor; \
      \
      *vid_pnt++ = poly->cr_palette_tx_idx[(u8)tx_color]; \
      \
      su += dudx; \
      sv += dvdx;
  #else
    #define set_pixel_tx_affine(shift) \
      u32 su_i = su >> FP; \
      u32 sv_i = sv >> FP; \
      su_i &= poly->texture_width_s; \
      sv_i &= poly->texture_height_s; \
      \
      u16 tx_color = poly->texture_image[(sv_i << (shift)) + su_i]; \
      tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor; \
      \
      *vid_pnt++ = tx_color; \
      \
      su += dudx; \
      sv += dvdx;
  #endif
  
  #ifdef PC
    #define set_pixel_tx_affine_tr() \
      u32 su_i = su >> FP; \
      u32 sv_i = sv >> FP; \
      su_i &= poly->texture_width_s; \
      sv_i &= poly->texture_height_s; \
      \
      u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i]; \
      \
      if (tx_color) { \
        tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor; \
        \
        *vid_pnt = poly->cr_palette_tx_idx[(u8)tx_color]; \
      } \
      \
      vid_pnt++; \
      \
      su += dudx; \
      sv += dvdx;
  #else
    #define set_pixel_tx_affine_tr(shift) \
      u32 su_i = su >> FP; \
      u32 sv_i = sv >> FP; \
      su_i &= poly->texture_width_s; \
      sv_i &= poly->texture_height_s; \
      \
      u16 tx_color = poly->texture_image[(sv_i << (shift)) + su_i]; \
      \
      if (tx_color) { \
        tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor; \
        \
        *vid_pnt = tx_color; \
      } \
      \
      vid_pnt++; \
      \
      su += dudx; \
      sv += dvdx;
  #endif
  
  RAM_CODE void draw_poly_tx_affine(g_poly_t *poly) {
    // obtain the top and bottom vertices
    
    int sup_vt = 0;
    #if CHECK_POLY_HEIGHT
      int inf_vt = 0;
    #endif
    
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
    
    // if the polygon doesn't have height return
    
    #if CHECK_POLY_HEIGHT
      if (poly->vertices[sup_vt].y >> FP == poly->vertices[inf_vt].y >> FP) return;
    #endif
    
    // initialize the edge variables
    
    fixed sx_l, su_l, sv_l, sx_r, su_r, sv_r;
    fixed dxdy_l, dudy_l, dvdy_l, dxdy_r, dudy_r, dvdy_r;
    
    int curr_vt_l = sup_vt;
    int curr_vt_r = sup_vt;
    
    int height_l = 0;
    int height_r = 0;
    
    // set the screen offset for the start of the first scanline
    
    u32 y0_i = poly->vertices[sup_vt].y >> FP;
    
    u16 *vid_y_offset = screen + y0_i * SCREEN_WIDTH;
    
    // main Y loop
    
    while (1) {
      // next edge found on the left side
      
      while (!height_l) {
        int next_vt_l;
        
        if (!poly->flags.is_backface) {
          next_vt_l = curr_vt_l - 1;
        
          if (next_vt_l < 0) {
            next_vt_l = poly->num_vertices - 1;
          }
        } else {
          next_vt_l = curr_vt_l + 1;
          
          if (next_vt_l == poly->num_vertices) {
            next_vt_l = 0;
          }
        }
        
        height_l = (poly->vertices[next_vt_l].y >> FP) - (poly->vertices[curr_vt_l].y >> FP);
        
        if (height_l < 0) return;
        
        if (height_l) {
          // calculate the edge deltas
          
          fixed dx = poly->vertices[next_vt_l].x - poly->vertices[curr_vt_l].x;
          fixed du = poly->vertices[next_vt_l].u - poly->vertices[curr_vt_l].u;
          fixed dv = poly->vertices[next_vt_l].v - poly->vertices[curr_vt_l].v;
          
          // unsigned integer division
          fixed rdy = fp_div(1, height_l);
          // fixed rdy = div_lut[height_l]; // .16 result
          dxdy_l = fp_mul(dx, rdy);
          dudy_l = fp_mul(du, rdy);
          dvdy_l = fp_mul(dv, rdy);
          
          // initialize the side variables
          
          fixed dty = poly->vertices[curr_vt_l].y - fp_trunc(poly->vertices[curr_vt_l].y); // sub-pixel precision
          sx_l = poly->vertices[curr_vt_l].x + fp_mul(dxdy_l, dty);
          su_l = poly->vertices[curr_vt_l].u + fp_mul(dudy_l, dty);
          sv_l = poly->vertices[curr_vt_l].v + fp_mul(dvdy_l, dty);
        }
        
        curr_vt_l = next_vt_l;
      }
      
      // next edge found on the right side
      
      while (!height_r) {
        int next_vt_r;
        
        if (!poly->flags.is_backface) {
          next_vt_r = curr_vt_r + 1;
          
          if (next_vt_r == poly->num_vertices) {
            next_vt_r = 0;
          }
        } else {
          next_vt_r = curr_vt_r - 1;
        
          if (next_vt_r < 0) {
            next_vt_r = poly->num_vertices - 1;
          }
        }
        
        height_r = (poly->vertices[next_vt_r].y >> FP) - (poly->vertices[curr_vt_r].y >> FP);
        
        if (height_r < 0) return;
        
        if (height_r) {
          // calculate the edge deltas
          
          fixed dx = poly->vertices[next_vt_r].x - poly->vertices[curr_vt_r].x;
          fixed du = poly->vertices[next_vt_r].u - poly->vertices[curr_vt_r].u;
          fixed dv = poly->vertices[next_vt_r].v - poly->vertices[curr_vt_r].v;
          
          // unsigned integer division
          fixed rdy = fp_div(1, height_r);
          // fixed rdy = div_lut[height_r]; // .16 result
          dxdy_r = fp_mul(dx, rdy);
          dudy_r = fp_mul(du, rdy);
          dvdy_r = fp_mul(dv, rdy);
          
          // initialize the side variables
          
          fixed dty = poly->vertices[curr_vt_r].y - fp_trunc(poly->vertices[curr_vt_r].y); // sub-pixel precision
          sx_r = poly->vertices[curr_vt_r].x + fp_mul(dxdy_r, dty);
          su_r = poly->vertices[curr_vt_r].u + fp_mul(dudy_r, dty);
          sv_r = poly->vertices[curr_vt_r].v + fp_mul(dvdy_r, dty);
        }
        
        curr_vt_r = next_vt_r;
      }
      
      // if the polygon doesn't have height return
      
      if (!height_l && !height_r) return;
      
      // obtain the height to the next vertex on Y
      
      u32 height = min_c(height_l, height_r);
      
      height_l -= height;
      height_r -= height;
      
      // second Y loop
      
      while (height > 0) {
        int sx_l_i = sx_l >> FP; // ceil
        int sx_r_i = sx_r >> FP;
        
        if (sx_l_i >= sx_r_i) goto segment_loop_exit;
        
        // calculate the scanline deltas
        
        fixed dudx, dvdx;
        u32 dx = sx_r_i - sx_l_i - 1;
        if (dx > 0) {
          // unsigned integer division
          fixed rdx = fp_div(1, dx);
          // fixed rdx = div_lut[dx]; // .16 result
          dudx = fp_mul((su_r - su_l), rdx);
          dvdx = fp_mul((sv_r - sv_l), rdx);
        } else {
          dudx = 0;
          dvdx = 0;
        }
        
        // initialize the scanline variables
        
        #if ENABLE_SUB_TEXEL_ACC
          fixed dtx = sx_l - fp_trunc(sx_l); // sub-texel precision
          fixed_u su = su_l + fp_mul(dudx, dtx);
          fixed_u sv = sv_l + fp_mul(dvdx, dtx);
        #else
          fixed_u su = su_l;
          fixed_u sv = sv_l;
        #endif
        
        // X clipping
        
        #if POLY_X_CLIPPING
          if (sx_l_i < 0) {
            // su += dudx * (-sx_l_i);
            // sv += dvdx * (-sx_l_i);
            sx_l_i = 0;
          }
          if (sx_r_i > SCREEN_WIDTH) {
            sx_r_i = SCREEN_WIDTH;
          }
        #endif
        
        // set the pointers for the start and the end of the scanline
        
        u16 *vid_pnt = vid_y_offset + sx_l_i;
        u16 *end_pnt = vid_y_offset + sx_r_i;
        
        // scanline loop
        
        if (!poly->flags.has_texture) {
          #ifdef PC
            while (vid_pnt < end_pnt) {
              set_pixel_tx_affine();
            }
          #else
            switch (poly->texture_width_bits) {
              case 3:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine(3); // 8
                }
                break;
              
              case 4:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine(4); // 16
                }
                break;
              
              case 5:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine(5); // 32
                }
                break;
            }
          #endif
        } else {
          #ifdef PC
            while (vid_pnt < end_pnt) {
              set_pixel_tx_affine_tr();
            }
          #else
            switch (poly->texture_width_bits) {
              case 3:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine_tr(3); // 8
                }
                break;
              
              case 4:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine_tr(4); // 16
                }
                break;
              
              case 5:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine_tr(5); // 32
                }
                break;
            }
          #endif
        }
        
        segment_loop_exit:
        
        // increment the left and right side variables
        
        sx_l += dxdy_l;
        su_l += dudy_l;
        sv_l += dvdy_l;
        sx_r += dxdy_r;
        su_r += dudy_r;
        sv_r += dvdy_r;
        vid_y_offset += SCREEN_WIDTH;
        height--;
      }
    }
  }
#endif

#if TX_PERSP_MODE == 1
  RAM_CODE void draw_poly_tx_sub_ps(g_poly_t *poly) {
    // obtain the top and bottom vertices
    
    int sup_vt = 0;
    #if CHECK_POLY_HEIGHT
      int inf_vt = 0;
    #endif
    
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
    
    // if the polygon doesn't have height return
    
    #if CHECK_POLY_HEIGHT
      if (poly->vertices[sup_vt].y >> FP == poly->vertices[inf_vt].y >> FP) return;
    #endif
    
    // calculate the reciprocal of Z and divide U and V by it
    
    for (int i = 0; i < poly->num_vertices; i++) {
      // unsigned fixed point division
      poly->vertices[i].z = fp_div(1 << FP, poly->vertices[i].z);
      // poly->vertices[i].z = div_lut[(fixed_u)poly->vertices[i].z >> 8] << 8; // .16 result
      poly->vertices[i].u = fp_mul(poly->vertices[i].u, poly->vertices[i].z);
      poly->vertices[i].v = fp_mul(poly->vertices[i].v, poly->vertices[i].z);
    }
    
    // initialize the edge variables
    
    fixed sx_l, su_l, sv_l, sz_l, sx_r, su_r, sv_r, sz_r;
    fixed dxdy_l, dudy_l, dvdy_l, dzdy_l, dxdy_r, dudy_r, dvdy_r, dzdy_r;
    
    int curr_vt_l = sup_vt;
    int curr_vt_r = sup_vt;
    
    int height_l = 0;
    int height_r = 0;
    
    // set the screen offset for the start of the first scanline
    
    u32 y0_i = poly->vertices[sup_vt].y >> FP;
    
    u16 *vid_y_offset = screen + y0_i * SCREEN_WIDTH;
    
    // main Y loop
    
    while (1) {
      // next edge found on the left side
      
      while (!height_l) {
        int next_vt_l;
        
        if (!poly->flags.is_backface) {
          next_vt_l = curr_vt_l - 1;
        
          if (next_vt_l < 0) {
            next_vt_l = poly->num_vertices - 1;
          }
        } else {
          next_vt_l = curr_vt_l + 1;
          
          if (next_vt_l == poly->num_vertices) {
            next_vt_l = 0;
          }
        }
        
        height_l = (poly->vertices[next_vt_l].y >> FP) - (poly->vertices[curr_vt_l].y >> FP);
        
        if (height_l < 0) return;
        
        if (height_l) {
          // calculate the edge deltas
          
          fixed dx = poly->vertices[next_vt_l].x - poly->vertices[curr_vt_l].x;
          fixed du = poly->vertices[next_vt_l].u - poly->vertices[curr_vt_l].u;
          fixed dv = poly->vertices[next_vt_l].v - poly->vertices[curr_vt_l].v;
          fixed dz = poly->vertices[next_vt_l].z - poly->vertices[curr_vt_l].z;
          
          // unsigned integer division
          fixed rdy = fp_div(1, height_l);
          // fixed rdy = div_lut[height_l]; // .16 result
          dxdy_l = fp_mul(dx, rdy);
          dudy_l = fp_mul(du, rdy);
          dvdy_l = fp_mul(dv, rdy);
          dzdy_l = fp_mul(dz, rdy);
          
          // initialize the side variables
          
          fixed dty = poly->vertices[curr_vt_l].y - fp_trunc(poly->vertices[curr_vt_l].y); // sub-pixel precision
          sx_l = poly->vertices[curr_vt_l].x + fp_mul(dxdy_l, dty);
          su_l = poly->vertices[curr_vt_l].u + fp_mul(dudy_l, dty);
          sv_l = poly->vertices[curr_vt_l].v + fp_mul(dvdy_l, dty);
          sz_l = poly->vertices[curr_vt_l].z + fp_mul(dzdy_l, dty);
        }
        
        curr_vt_l = next_vt_l;
      }
      
      // next edge found on the right side
      
      while (!height_r) {
        int next_vt_r;
        
        if (!poly->flags.is_backface) {
          next_vt_r = curr_vt_r + 1;
          
          if (next_vt_r == poly->num_vertices) {
            next_vt_r = 0;
          }
        } else {
          next_vt_r = curr_vt_r - 1;
        
          if (next_vt_r < 0) {
            next_vt_r = poly->num_vertices - 1;
          }
        }
        
        height_r = (poly->vertices[next_vt_r].y >> FP) - (poly->vertices[curr_vt_r].y >> FP);
        
        if (height_r < 0) return;
        
        if (height_r) {
          // calculate the edge deltas
          
          fixed dx = poly->vertices[next_vt_r].x - poly->vertices[curr_vt_r].x;
          fixed du = poly->vertices[next_vt_r].u - poly->vertices[curr_vt_r].u;
          fixed dv = poly->vertices[next_vt_r].v - poly->vertices[curr_vt_r].v;
          fixed dz = poly->vertices[next_vt_r].z - poly->vertices[curr_vt_r].z;
          
          // unsigned integer division
          fixed rdy = fp_div(1, height_r);
          // fixed rdy = div_lut[height_r]; // .16 result
          dxdy_r = fp_mul(dx, rdy);
          dudy_r = fp_mul(du, rdy);
          dvdy_r = fp_mul(dv, rdy);
          dzdy_r = fp_mul(dz, rdy);
          
          // initialize the side variables
          
          fixed dty = poly->vertices[curr_vt_r].y - fp_trunc(poly->vertices[curr_vt_r].y); // sub-pixel precision
          sx_r = poly->vertices[curr_vt_r].x + fp_mul(dxdy_r, dty);
          su_r = poly->vertices[curr_vt_r].u + fp_mul(dudy_r, dty);
          sv_r = poly->vertices[curr_vt_r].v + fp_mul(dvdy_r, dty);
          sz_r = poly->vertices[curr_vt_r].z + fp_mul(dzdy_r, dty);
        }
        
        curr_vt_r = next_vt_r;
      }
      
      // if the polygon doesn't have height return
      
      if (!height_l && !height_r) return;
      
      // obtain the height to the next vertex on Y
      
      u32 height = min_c(height_l, height_r);
      
      height_l -= height;
      height_r -= height;
      
      // second Y loop
      
      while (height > 0) {
        uint sx_l_i = sx_l >> FP; // ceil
        uint sx_r_i = sx_r >> FP;
        
        if (sx_l_i >= sx_r_i) goto segment_loop_exit;
        
        // calculate the scanline deltas
        
        fixed dudx, dvdx, dzdx;
        int dx = sx_r_i - sx_l_i;
        if (dx) {
          // unsigned integer division
          fixed rdx = fp_div(1, dx);
          // fixed rdx = div_lut[dx >> 8] << 8; // .16 result
          dudx = fp_mul(su_r - su_l, rdx);
          dvdx = fp_mul(sv_r - sv_l, rdx);
          dzdx = fp_mul(sz_r - sz_l, rdx);
        } else {
          dudx = 0;
          dvdx = 0;
          dzdx = 0;
        }
        
        // initialize the scanline variables
        
        // screen u and v pre-stepped at the segment end storing the start
        #if ENABLE_SUB_TEXEL_ACC
          fixed dt = sx_l - fp_trunc(sx_l); // sub-texel precision
          fixed s_su_r = su_l + fp_mul(dudx, dt);
          fixed s_sv_r = sv_l + fp_mul(dvdx, dt);
          fixed sz = sz_l + fp_mul(dzdx, dt);
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
          if (sx_r_i > SCREEN_WIDTH) {
            sx_r_i = SCREEN_WIDTH;
          }
        #endif
        
        // set the pointers for the start and the end of the scanline
        
        u16 *vid_pnt = vid_y_offset + sx_l_i;
        u16 *end_pnt = vid_y_offset + sx_r_i;
        
        u16 *seg_end_pnt = vid_y_offset + (sx_l_i | OC_SEG_SIZE_S) + 1; // end of the segment
        
        // unsigned fixed point division
        fixed rsz = fp_div(1 << FP, sz);
        // fixed rsz = div_lut[(fixed_u)sz >> 8] << 8; // .16 result
        // perspective correct u and v pre-stepped at the segment end storing the start
        fixed p_su_r = fp_mul(s_su_r, rsz);
        fixed p_sv_r = fp_mul(s_sv_r, rsz);
        
        // segment loop
        
        while (vid_pnt < end_pnt) {
          #if VIEW_PERSP_SUB
            if (cfg.debug){ //view perspective subdivision
              *(vid_pnt - 1) = PAL_BLUE;
              //set_pixel(j, i);
            }
          #endif
          
          if (seg_end_pnt > end_pnt) {
            seg_end_pnt = end_pnt;
          }
          
          int length;
          fixed p_su, p_sv, p_duln, p_dvln;
          if (seg_end_pnt - vid_pnt < OC_SEG_SIZE) { // the segment is not full
            length = seg_end_pnt - vid_pnt;
            s_su_r += dudx * length;
            s_sv_r += dvdx * length;
            sz += dzdx * length;
            
            p_su = p_su_r;
            p_sv = p_sv_r;
            
            // unsigned fixed point division
            fixed rsz = fp_div(1, sz);
            // fixed rsz = div_lut[(fixed_u)sz]; // .16 result
            p_su_r = fp_mul(s_su_r, rsz);
            p_sv_r = fp_mul(s_sv_r, rsz);
            
            // unsigned integer division
            fixed rc_length = fp_div(1, length);
            // fixed rc_length = div_lut[length]; // .16 result
            p_duln = fp_mul(p_su_r - p_su, rc_length);
            p_dvln = fp_mul(p_sv_r - p_sv, rc_length);
          } else { // full segment
            length = OC_SEG_SIZE;
            s_su_r += dudx << OC_SEG_SIZE_BITS;
            s_sv_r += dvdx << OC_SEG_SIZE_BITS;
            sz += dzdx << OC_SEG_SIZE_BITS;
            
            p_su = p_su_r;
            p_sv = p_sv_r;
            
            // unsigned fixed point division
            fixed rsz = fp_div(1 << FP, sz);
            // fixed rsz = div_lut[sz >> 8] << 8; // .16 result
            p_su_r = fp_mul(s_su_r, rsz);
            p_sv_r = fp_mul(s_sv_r, rsz);
            
            p_duln = (p_su_r - p_su) >> OC_SEG_SIZE_BITS;
            p_dvln = (p_sv_r - p_sv) >> OC_SEG_SIZE_BITS;
          }
          
          // segment inner loop
          
          if (!poly->flags.has_texture) {
            while (vid_pnt < seg_end_pnt) {
              int su_i = p_su >> FP;
              int sv_i = p_sv >> FP;
              su_i &= poly->texture_width_s;
              sv_i &= poly->texture_height_s;
              
              u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
              tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
              
              #if defined(PC)
                *vid_pnt++ = poly->cr_palette_tx_idx[(u8)tx_color];
              #else
                *vid_pnt++ = tx_color;
              #endif
              
              p_su += p_duln;
              p_sv += p_dvln;
            }
          } else {
            while (vid_pnt < seg_end_pnt) {
              int su_i = p_su >> FP;
              int sv_i = p_sv >> FP;
              su_i &= poly->texture_width_s;
              sv_i &= poly->texture_height_s;
              
              u16 tx_color = poly->texture_image[(sv_i << poly->texture_width_bits) + su_i];
              
              if (tx_color) { // tx_color != TX_ALPHA_CR
                tx_color = (tx_color << LIGHT_GRD_BITS) + poly->final_light_factor;
                
                #if defined(PC)
                  *vid_pnt = poly->cr_palette_tx_idx[(u8)tx_color];
                #else
                  *vid_pnt = tx_color;
                #endif
              }
              
              vid_pnt++;
              p_su += p_duln;
              p_sv += p_dvln;
            }
          }
          
          seg_end_pnt += OC_SEG_SIZE;
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
        vid_y_offset += SCREEN_WIDTH;
        height--;
      }
    }
  }
#endif

#if DRAW_TRIANGLES
  #define CONSTANT_DELTAS 1
  
  RAM_CODE void draw_tri_tx_affine(g_poly_t *poly) {
    // obtain the top and bottom vertices
    
    int sup_vt = 0;
    #if CHECK_POLY_HEIGHT
      int inf_vt = 0;
    #endif
    
    for (int i = 1; i < 3; i++) {
      if (poly->vertices[i].y < poly->vertices[sup_vt].y) {
        sup_vt = i;
      }
      #if CHECK_POLY_HEIGHT
        if (poly->vertices[i].y > poly->vertices[inf_vt].y) {
          inf_vt = i;
        }
      #endif
    }
    
    // if the polygon doesn't have height return
    
    #if CHECK_POLY_HEIGHT
      if (poly->vertices[sup_vt].y >> FP == poly->vertices[inf_vt].y >> FP) return;
    #endif
    
    // set the vertices starting with the top vertex in clockwise order
    
    // initialize the edge variables
    
    fixed sx_l, su_l, sv_l, sx_r, su_r, sv_r;
    fixed dxdy_l, dudy_l, dvdy_l, dxdy_r, dudy_r, dvdy_r;
    
    int curr_vt_l = sup_vt;
    int curr_vt_r = sup_vt;
    
    int height_l = 0;
    int height_r = 0;
    
    #if CONSTANT_DELTAS
      // constant deltas
      
      u32 cxy1 = (poly->vertices[0].x - poly->vertices[2].x) * (poly->vertices[1].y - poly->vertices[2].y);
      u32 cxy2 = (poly->vertices[1].x - poly->vertices[2].x) * (poly->vertices[0].y - poly->vertices[2].y);
      
      fixed cuy1 = fp_mul((poly->vertices[0].u - poly->vertices[2].u), (poly->vertices[1].y - poly->vertices[2].y));
      fixed cuy2 = fp_mul((poly->vertices[1].u - poly->vertices[2].u), (poly->vertices[0].y - poly->vertices[2].y));
      fixed cvy1 = fp_mul((poly->vertices[0].v - poly->vertices[2].v), (poly->vertices[1].y - poly->vertices[2].y));
      fixed cvy2 = fp_mul((poly->vertices[1].v - poly->vertices[2].v), (poly->vertices[0].y - poly->vertices[2].y));
      
      // unsigned integer division
      
      fixed rdx = div_lut[cxy1 - cxy2]; // .16 result
      
      fixed dudx = fp_mul(cuy1 - cuy2, rdx);
      fixed dvdx = fp_mul(cvy1 - cvy2, rdx);
    #endif
    
    // set the screen offset for the start of the first scanline
    
    u32 y0_i = poly->vertices[0].y >> FP;
    
    u16 *vid_y_offset = screen + y0_i * SCREEN_WIDTH;
    
    // main Y loop
    
    while (1) {
      // next edge found on the left side
      
      if (!height_l) {
        int next_vt_l;
        
        if (!poly->flags.is_backface) {
          next_vt_l = curr_vt_l - 1;
          
          if (next_vt_l < 0) {
            next_vt_l = 2;
          }
        } else {
          next_vt_l = curr_vt_l + 1;
          
          if (next_vt_l == 3) {
            next_vt_l = 0;
          }
        }
        
        height_l = (poly->vertices[next_vt_l].y >> FP) - (poly->vertices[curr_vt_l].y >> FP);
        
        if (height_l < 0) return;
        
        if (height_l) {
          fixed dx = poly->vertices[next_vt_l].x - poly->vertices[curr_vt_l].x;
          fixed du = poly->vertices[next_vt_l].u - poly->vertices[curr_vt_l].u;
          fixed dv = poly->vertices[next_vt_l].v - poly->vertices[curr_vt_l].v;
          
          // unsigned integer division
          fixed rdy = div_lut[height_l]; // .16 result
          dxdy_l = fp_mul(dx, rdy);
          dudy_l = fp_mul(du, rdy);
          dvdy_l = fp_mul(dv, rdy);
          
          // initialize the side variables
          
          fixed dty = poly->vertices[curr_vt_l].y - fp_trunc(poly->vertices[curr_vt_l].y); // sub-pixel precision
          sx_l = poly->vertices[curr_vt_l].x + fp_mul(dxdy_l, dty);
          su_l = poly->vertices[curr_vt_l].u + fp_mul(dudy_l, dty);
          sv_l = poly->vertices[curr_vt_l].v + fp_mul(dvdy_l, dty);
        }
        
        curr_vt_l = next_vt_l;
      }
      
      // next edge found on the right side
      
      if (!height_r) {
        int next_vt_r;
        
        if (!poly->flags.is_backface) {
          next_vt_r = curr_vt_r + 1;
          
          if (next_vt_r == 3) {
            next_vt_r = 0;
          }
        } else {
          next_vt_r = curr_vt_r - 1;
        
          if (next_vt_r < 0) {
            next_vt_r = 2;
          }
        }
        
        height_r = (poly->vertices[next_vt_r].y >> FP) - (poly->vertices[curr_vt_r].y >> FP);
        
        if (height_r < 0) return;
        
        if (height_r) {
          fixed dx = poly->vertices[next_vt_r].x - poly->vertices[curr_vt_r].x;
          fixed du = poly->vertices[next_vt_r].u - poly->vertices[curr_vt_r].u;
          fixed dv = poly->vertices[next_vt_r].v - poly->vertices[curr_vt_r].v;
          
          // unsigned integer division
          fixed rdy = div_lut[height_r]; // .16 result
          dxdy_r = fp_mul(dx, rdy);
          dudy_r = fp_mul(du, rdy);
          dvdy_r = fp_mul(dv, rdy);
          
          // initialize the side variables
          
          fixed dty = poly->vertices[curr_vt_r].y - fp_trunc(poly->vertices[curr_vt_r].y); // sub-pixel precision
          sx_r = poly->vertices[curr_vt_r].x + fp_mul(dxdy_r, dty);
          su_r = poly->vertices[curr_vt_r].u + fp_mul(dudy_r, dty);
          sv_r = poly->vertices[curr_vt_r].v + fp_mul(dvdy_r, dty);
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
      
      while (height > 0) {
        int sx_l_i = sx_l >> FP; // ceil
        int sx_r_i = sx_r >> FP;
        
        if (sx_l_i >= sx_r_i) goto segment_loop_exit;
        
        #if !CONSTANT_DELTAS
          // calculate the scanline deltas
          
          fixed dudx, dvdx;
          u32 dx = sx_r_i - sx_l_i;
          if (dx) {
            // unsigned integer division
            //fixed dx = fp_div(1 << FP, sx_r - sx_l);
            fixed rdx = div_lut[dx]; // .16 result
            dudx = fp_mul(su_r - su_l, rdx);
            dvdx = fp_mul(sv_r - sv_l, rdx);
          } else {
            dudx = 0;
            dvdx = 0;
          }
        #endif
        
        // initialize the scanline variables
        
        #if ENABLE_SUB_TEXEL_ACC
          fixed dtx = sx_l) - sx_l; // sub-texel precision
          fixed su = su_l + fp_mul(dudx, dtx);
          fixed sv = sv_l + fp_mul(dvdx, dtx);
        #else
          fixed su = su_l;
          fixed sv = sv_l;
        #endif
        
        // X clipping
        
        #if POLY_X_CLIPPING
          if (sx_l_i < 0) {
            sx_l_i = 0;
          }
          if (sx_r_i > SCREEN_WIDTH) {
            sx_r_i = SCREEN_WIDTH;
          }
        #endif
        
        // set the pointers for the start and the end of the scanline
        
        u16 *vid_pnt = vid_y_offset + sx_l_i;
        u16 *end_pnt = vid_y_offset + sx_r_i;
        
        // scanline loop
        
        if (!poly->flags.has_texture) {
          #ifdef PC
            while (vid_pnt < end_pnt) {
              set_pixel_tx_affine();
            }
          #else
            switch (poly->texture_width_bits) {
              case 3:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine(3); // 8
                }
                break;
              
              case 4:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine(4); // 16
                }
                break;
              
              case 5:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine(5); // 32
                }
                break;
            }
          #endif
        } else {
          #ifdef PC
            while (vid_pnt < end_pnt) {
              set_pixel_tx_affine_tr();
            }
          #else
            switch (poly->texture_width_bits) {
              case 3:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine_tr(3); // 8
                }
                break;
              
              case 4:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine_tr(4); // 16
                }
                break;
              
              case 5:
                while (vid_pnt < end_pnt) {
                  set_pixel_tx_affine_tr(5); // 32
                }
                break;
            }
          #endif
        }
        
        segment_loop_exit:
        
        // increment the left and right side variables
        
        sx_l += dxdy_l;
        su_l += dudy_l;
        sv_l += dvdy_l;
        sx_r += dxdy_r;
        su_r += dudy_r;
        sv_r += dvdy_r;
        vid_y_offset += SCREEN_WIDTH;
        height--;
      }
    }
  }
#endif