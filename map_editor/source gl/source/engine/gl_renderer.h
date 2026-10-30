void gl_renderer_init(void);
void gl_renderer_begin(void);
void gl_renderer_submit(g_poly_t *poly, GLuint texture);
void gl_renderer_flush(void);
void gl_renderer_shutdown(void);
void gl_load_model_textures(textures_t *textures);