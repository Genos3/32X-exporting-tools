void draw_line(s32_fix x0, s32_fix y0, s32_fix x1, s32_fix y1, u16 color);
void draw_line_zb(s32_fix x0, s32_fix y0, s32_fix z0, s32_fix x1, s32_fix y1, s32_fix z1, u16 color);
void set_pixel(int x, int y, u16 color);
void fill_rect(int x, int y, int width, int height, u16 color);
void draw_sprite(g_poly_t *poly);
void draw_poly(g_poly_t *poly);
void draw_poly_tx_affine(g_poly_t *poly);
void draw_poly_tx_sub_ps(g_poly_t *poly);
void draw_tri_tx_affine(g_poly_t *poly);