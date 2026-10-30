#include "common.h"

// calculates the gradient for the sky and updates the palette

void set_sky_gradient() {
    u32_fix r0 = sky.start_color_ch.r << FP;
    u32_fix g0 = sky.start_color_ch.g << FP;
    u32_fix b0 = sky.start_color_ch.b << FP;
    
    u32_fix r1 = sky.end_color_ch.r << FP;
    u32_fix g1 = sky.end_color_ch.g << FP;
    u32_fix b1 = sky.end_color_ch.b << FP;
    
    // calculate the deltas for each color
    
    s32_fix dt_r = (r1 - r0) >> SKY_NUM_COLOR_GRADIENT_BITS;
    s32_fix dt_g = (g1 - g0) >> SKY_NUM_COLOR_GRADIENT_BITS;
    s32_fix dt_b = (b1 - b0) >> SKY_NUM_COLOR_GRADIENT_BITS;
    
    for (int i = 0; i < SKY_NUM_COLOR_GRADIENT; i++) {
        u16 color = ((b0 >> FP) << 10) | ((g0 >> FP) << 5) | (r0 >> FP);
        
        hw_palette[PAL_SKY_GRD_OFFSET + i] = color;
        
        r0 += dt_r;
        g0 += dt_g;
        b0 += dt_b;
    }
}

// draws a sky gradient

void draw_sky_pal() {
    // obtain the start and end points
    
    u16 cam_start_angle = cam.rot.x - vp.half_vfov - (128 << 8);
    // u16 cam_end_angle = cam.rot.x + vp.half_vfov - (128 << 8);
    
    s32_fix grd_y0 = (u16)(sky.start_angle - (128 << 8)) - cam_start_angle;
    s32_fix grd_y1 = (u16)(sky.end_angle - (128 << 8)) - cam_start_angle;
    
    // turn the angles to screen coords
    
    grd_y0 = grd_y0 * sky.fp_deg2px; // .16
    grd_y1 = grd_y1 * sky.fp_deg2px; // .16
    
    int grd_y0i = grd_y0 >> FP;
    int grd_y1i = grd_y1 >> FP;
    
    // calculate the gradient delta for the height
    
    s32_fix grd_dy;
    if (grd_y1i > 0 && grd_y0i < SCREEN_HEIGHT) {
        s32_fix t_grd_dy = grd_y1i - grd_y0i;
        
        s32_fix rdy;
        if (t_grd_dy) {
            // signed integer division
            #if !DIV_LUT_ENABLE
                rdy = fp_div(1, t_grd_dy);
            #else
                rdy = div_luts_s(t_grd_dy); // .16 result
            #endif
        } else {
            rdy = 0;
        }
        
        grd_dy = SKY_NUM_COLOR_GRADIENT * rdy;
    } else {
        grd_dy = 0;
    }
    
    s32_fix scr_grd_offset = 0; // screen gradient offset
    
    // clipping
    
    if (grd_y0i < 0 && grd_y1i > 0) {
        scr_grd_offset += -grd_y0i * grd_dy;
        grd_y0i = 0;
    }
    else if (grd_y1i <= 0) {
        scr_grd_offset = (SKY_NUM_COLOR_GRADIENT - 1) << FP;
        grd_y1i = 0;
    }
    
    if (grd_y0i < SCREEN_HEIGHT && grd_y1i > SCREEN_HEIGHT) {
        grd_y1i = SCREEN_HEIGHT - 1;
    }
    else if (grd_y0i >= SCREEN_HEIGHT) {
        grd_y0i = SCREEN_HEIGHT - 1;
    }
    
    // draw the gradient
    
    int scr_grd_offset_i = scr_grd_offset >> FP;
    
    u16 pal_color_idx = PAL_SKY_GRD_OFFSET + scr_grd_offset_i;
    
    #ifdef PC
        u16 color_data = pal_color_idx;
    #else
        u16 color_data = dup8(pal_color_idx);
    #endif
    
    u16 *vid_y_offset = (u16 *)screen;
    
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        
        // memset32(vid_y_offset, color_data, SCREEN_HALF_WIDTH);
        
        u16 *vid_pnt = vid_y_offset;
        
        #pragma GCC unroll 4
        for (int j = 0; j < FRAME_WIDTH_W; j++) {
            #ifdef PC
                *vid_pnt++ = pc_palette[(u8)color_data];
            #else
                *vid_pnt++ = color_data;
            #endif
        }
        
        if (i >= grd_y0i && i < grd_y1i) {
            scr_grd_offset += grd_dy;
            
            if (scr_grd_offset >> FP != scr_grd_offset_i) {
                scr_grd_offset_i = scr_grd_offset >> FP;
                pal_color_idx = PAL_SKY_GRD_OFFSET + scr_grd_offset_i;
                #ifdef PC
                    color_data = pal_color_idx;
                #else
                    color_data = dup8(pal_color_idx);
                #endif
            }
        }
        
        vid_y_offset += FRAME_WIDTH_W;
    }
}

// draws a gradient for the sky

void draw_sky() {
    // obtain the start and end points
    
    u16 cam_start_angle = cam.rot.x - vp.half_vfov - (128 << 8);
    // u16 cam_end_angle = cam.rot.x + vp.half_vfov - (128 << 8);
    
    int grd_y0 = (u16)(sky.start_angle - (128 << 8)) - cam_start_angle;
    int grd_y1 = (u16)(sky.end_angle - (128 << 8)) - cam_start_angle;
    
    // turn the angles to screen coords
    
    grd_y0 = grd_y0 * sky.fp_deg2px; // .16
    grd_y1 = grd_y1 * sky.fp_deg2px; // .16
    
    int grd_y0i = grd_y0 >> FP;
    int grd_y1i = grd_y1 >> FP;
    
    u32_fix r0 = sky.start_color_ch.r;
    u32_fix g0 = sky.start_color_ch.g;
    u32_fix b0 = sky.start_color_ch.b;
    
    u32_fix r1 = sky.end_color_ch.r;
    u32_fix g1 = sky.end_color_ch.g;
    u32_fix b1 = sky.end_color_ch.b;
    
    // determine the number of shading levels by obtaining the channel with the largest difference
    
    int num_levels = max_c(max_c(r1 - r0, g1 - g0), b1 - b0);
    
    // calculate the deltas for each color and for the gradient
    
    s32_fix dr_grd, dg_grd, db_grd, grd_dy;
    if (grd_y1i > 0 && grd_y0i < SCREEN_HEIGHT) {
        // s32_fix rdy = rc_fp_div_luts_a(1 >> 20, grd_y1i - grd_y0i);
        // unsigned integer division
        s32_fix rc_v = div_luts(num_levels); // .16 result
        dr_grd = (r1 - r0) * rc_v;
        dg_grd = (g1 - g0) * rc_v;
        db_grd = (b1 - b0) * rc_v;
        // grd_dy = rc_fp_div_luts_a(num_levels, grd_y1i - grd_y0i);
        // signed integer division
        grd_dy = div_luts_s(grd_y1i - grd_y0i); // .16 result
        grd_dy = num_levels * grd_dy;
    } else {
        dr_grd = 0;
        dg_grd = 0;
        db_grd = 0;
        grd_dy = 0;
    }
    
    r0 <<= FP;
    g0 <<= FP;
    b0 <<= FP;
    
    s32_fix grd_offset = 0;
    
    // clipping
    
    if (grd_y0i < 0 && grd_y1i > 0) {
        //s32_fix s = rc_fp_div_luts_a(grd_y1 - 0, grd_y1 - grd_y0);
        //r0 = r0 + fp_mul(s, r1 - r0);
        grd_offset = -grd_y0i * grd_dy;
        r0 = r0 + fp_mul(dr_grd, grd_offset);
        g0 = g0 + fp_mul(dg_grd, grd_offset);
        b0 = b0 + fp_mul(db_grd, grd_offset);
        grd_offset &= (1 << FP) - 1;
        grd_y0i = 0;
    } else
    if (grd_y1i <= 0) {
        grd_y1i = 0;
        r0 = r1 << FP;
        g0 = g1 << FP;
        b0 = b1 << FP;
    }
    
    if (grd_y0i < SCREEN_HEIGHT && grd_y1i > SCREEN_HEIGHT) {
        grd_y1i = SCREEN_HEIGHT - 1;
    }
    
    // draw the gradient
    
    u32_fix sr = r0;
    u32_fix sg = g0;
    u32_fix sb = b0;
    
    u32 color = dup16(((sb >> FP) << 10) | ((sg >> FP) << 5) | (sr >> FP));
    u16 *vid_y_offset = screen;
    
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        memset32(vid_y_offset, color, SCREEN_HALF_WIDTH);
        
        /* u16 *vid_pnt = vid_y_offset;
        
        for (int j = 0; j < SCREEN_WIDTH; j++) {
            *vid_pnt++ = color;
        } */
        
        if (i >= grd_y0i && i < grd_y1i) {
            if (grd_offset >= 1 << FP) {

                sr += dr_grd;
                sg += dg_grd;
                sb += db_grd;
                
                color = dup16(((sb >> FP) << 10) | ((sg >> FP) << 5) | (sr >> FP));
                grd_offset -= 1 << FP;
            }
            
            grd_offset += grd_dy;
        }
        
        vid_y_offset += SCREEN_WIDTH;
    }
}