#include <math.h>
#include "common.h"

void calc_fov() {
  float hfov_deg = 70; // 70
  float aspect_ratio = (float)SCREEN_WIDTH / SCREEN_HEIGHT;
  // vp.hfov = (hfov_deg << 8) * 256 / 360; // .8
  // vp.half_hfov = vp.hfov >> 1;
  // -vp.hfov = fp_mul(vp.vfov, vp.aspect_ratio);
  // -vp.hfov = (float)atan(SCREEN_HALF_HEIGHT / vp.focal_length) * 180 / PI_fp * 2;
  // vp.half_hfov = atan(shift_fp(fp_mul(tan_c(vp.half_vfov), vp.aspect_ratio), FP, 14));
  // float half_vfov = atan((float)tan(hfov_deg / 2 * PI_FL / 180) / aspect_ratio) / PI_FL * 180;
  float half_vfov = atan_deg(tan_deg(hfov_deg / 2) / aspect_ratio);
  vp.half_vfov = fix8(half_vfov);
  vp.vfov = vp.half_vfov * 2;
  
  // tan(x) = op / adj
  // adj = op / tan(x)
  // sin(x) / cos(x) = op / adj
  // adj = op * cos(x) / sin(x)
  
  // fixed z_near_dist = fp_div(lu_cos(vp.half_hfov), lu_sin(vp.half_hfov));
  // vp.focal_length = SCREEN_HALF_WIDTH * z_near_dist; // z_dist_screen, 115 (fov 70, screen_height 160)
  // vp.focal_length = SCREEN_HALF_WIDTH / tan_c(vp.half_hfov);
  float focal_length = SCREEN_HALF_WIDTH / tan(hfov_deg / 2 * PI_FL / 180);
  vp.focal_length = fix(focal_length);
  // vp.focal_length = fp_mul(vp.focal_length, vp.aspect_ratio);
  vp.focal_length_i = vp.focal_length >> FP;
  vp.screen_side_x_dt = fix(SCREEN_HALF_WIDTH / focal_length) + fix(0.01);
  vp.screen_side_y_dt = fix(SCREEN_HALF_HEIGHT / focal_length) + fix(0.01);
}

#if PALETTE_MODE
  void init_palette() {
    hw_palette[PAL_BG] = CLR_BG;
    hw_palette[PAL_WHITE] = CLR_WHITE;
    hw_palette[PAL_BLACK] = CLR_BLACK;
    hw_palette[PAL_RED] = CLR_RED;
    hw_palette[PAL_GREEN] = CLR_GREEN;
    hw_palette[PAL_BLUE] = CLR_BLUE;
    hw_palette[PAL_YELLOW] = CLR_YELLOW;
    hw_palette[PAL_MAGENTA] = CLR_MAGENTA;
  }
#endif

void init_structs() {
  dl.pl_list = dl_pl_list;
  dl.vt_list = dl_vt_list;
  dl.obj_list = dl_obj_list;
  dl.tr_vt_index = dl_tr_vt_index;
  dl.visible_vt_list = dl_visible_vt_list;
  
  g_ot_list.pnt = ordering_table;
  g_ot_list.pl_list = ot_pl_list;
  g_ot_list.size = SCN_OT_SIZE;
}

void init_obj(object_t *obj) {
  obj->pos.x = 0;
  obj->pos.y = 0;
  obj->pos.z = 0;
  obj->rot.x = 0;
  obj->rot.y = 0;
  obj->rot.z = 0;
  obj->static_pos = 1;
  obj->static_rot = 1;
}