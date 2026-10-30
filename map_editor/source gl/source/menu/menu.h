bool init_menu(void);
void draw_menu(int width, int height);
void shutdown_menu(void);
void handle_gui_mouse(int x, int y, SDL_Window *window);
GLuint compile_shader(GLenum type, const char *path);