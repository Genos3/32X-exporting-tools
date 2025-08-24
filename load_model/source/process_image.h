void process_image(char *str, textures_t *textures);
void quantize_palette(list_u16 *palette, int n_colors, textures_t *textures);
void index_palette(list_u16 *palette, u8 textured, textures_t *textures);
void set_alpha_color(textures_t *textures);
void set_tx_index(textures_t *textures);
void set_untextured_colors(textures_t *textures);
void set_palette_gradients(textures_t *textures);
void dup_textures_color_index(textures_t *textures);

extern byte tx_has_alpha, image_is_rgba;