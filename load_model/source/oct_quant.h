typedef struct {
	int w, h;
	unsigned char *pix;
} image_t, *image;

image img_new(int w, int h);
void color_quant(image im, int n_colors, int dither);