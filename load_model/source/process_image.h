void process_image(char str[]);
void quantize_palette(list_u16 *palette, int n_colors);
void index_palette(list_u16 *palette, u8 type);
void set_alpha_cr();
void set_tx_index();
void set_untextured_colors();
void set_palette_gradients();
void dup_textures_color_index();

extern byte tx_has_alpha, image_is_rgba;