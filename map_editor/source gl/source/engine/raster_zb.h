void draw_line_zb(s32_fix x0, s32_fix y0, s32_fix z0, s32_fix x1, s32_fix y1, s32_fix z1, u16 color);
void set_pixel_zb(int x, int y, s32_fix z, u16 color);
void draw_sprite_zb(g_poly_t *poly);
void draw_poly_zb(g_poly_t *poly);
void draw_poly_tx_affine_zb(g_poly_t *poly);
void draw_poly_tx_ps_zb(g_poly_t *poly);