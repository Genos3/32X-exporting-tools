#include <SDL3/SDL.h>
#include "common.h"

static char *load_text_file(const char *path);
static GLuint compile_shader(GLenum type, const char *path);
static bool load_presenter(void);
static void updateCursorPos(SDL_Window *window);
static void processKeyDown(SDL_Keycode key, bool repeat);
static void processKeyUp(SDL_Keycode key);

static SDL_Window *window;
static SDL_GLContext gl_context;
static GLuint framebuffer_texture;
static GLuint presenter_program;
static GLuint presenter_vao;
static u8 *framebuffer_rgb;

static bool quit;
static int windows_width, windows_height;
static u8 view_lines_mode;
float delta_time_f;

u8 i_cursor_key_pressed, i_key_pressed, i_modifier_key_pressed, mouse_moved, i_mouse_button_pressed, enable_mouse_cursor;
u16 i_direction_pressed;
int i_cursor_dx, i_cursor_dy;
u8 mouse_capture;
char *file_path;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        SDL_Log("no arguments passed");
        return 1;
    }

    file_path = argv[1];
    windows_width = PC_SCREEN_WIDTH * PC_SCREEN_SCALE_SIZE;
    windows_height = PC_SCREEN_HEIGHT * PC_SCREEN_SCALE_SIZE;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER)) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window = SDL_CreateWindow("test", windows_width, windows_height, SDL_WINDOW_OPENGL);
    if (!window) {
        SDL_Log("SDL_CreateWindow: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        SDL_Log("SDL_GL_CreateContext: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to load OpenGL functions");
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    if (!load_presenter()) {
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    if (!init_menu()) {
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    glGenTextures(1, &framebuffer_texture);
    glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
        PC_SCREEN_WIDTH, PC_SCREEN_HEIGHT, 0,
        GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenVertexArrays(1, &presenter_vao);

    framebuffer_rgb = malloc(PC_SCREEN_WIDTH * PC_SCREEN_HEIGHT * 3);

    SDL_GL_SetSwapInterval(1);
    SDL_SetWindowPosition(window, 500, 100);

    init_memory();
    load_files(file_path);

    view_lines_mode = 0;
    frame_cycles = 0;
    delta_time_f = 0;
    fps = 60;
    redraw_scene = 1;
    dbg_show_poly_num = 0;
    dbg_show_grid_tile_num = 0;
    mouse_capture = 0;
    enable_mouse_cursor = 0;
    mouse_moved = 0;
    i_cursor_dx = 0;
    i_cursor_dy = 0;
    i_mouse_button_pressed = 0;
    i_direction_pressed = 0;
    i_cursor_key_pressed = 0;
    i_key_pressed = 0;
    i_modifier_key_pressed = 0;
    updateCursorPos(window);

    init_3d();
    init_game();
    draw();
    if (cfg.debug) draw_dbg();
    
    for (int i = 0; i < PC_SCREEN_WIDTH * PC_SCREEN_HEIGHT; i++) {
        u16 pixel = screen[i];

        framebuffer_rgb[i * 3]     = (pixel & 0x1F) * 255 / 31;
        framebuffer_rgb[i * 3 + 1] = ((pixel >> 5) & 0x1F) * 255 / 31;
        framebuffer_rgb[i * 3 + 2] = ((pixel >> 10) & 0x1F) * 255 / 31;
    }

    glViewport(0, 0, windows_width, windows_height);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
        PC_SCREEN_WIDTH, PC_SCREEN_HEIGHT,
        GL_RGB, GL_UNSIGNED_BYTE, framebuffer_rgb);

    glUseProgram(presenter_program);
    glBindVertexArray(presenter_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(window);

    const double update_time = 1000.0 / 60.0;
    double delta_time_t = update_time;
    const double performance_frequency = 1000.0 / (double)SDL_GetPerformanceFrequency();
    quit = false;

    while (!quit) {
        key_prev = key_curr;
        Uint64 start_time = SDL_GetPerformanceCounter();
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    quit = true;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    processKeyDown(event.key.key, event.key.repeat);
                    break;
                case SDL_EVENT_KEY_UP:
                    processKeyUp(event.key.key);
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (mouse_capture) {
                        mouse_moved = 1;
                        i_cursor_dx += (int)event.motion.xrel;
                        i_cursor_dy += (int)event.motion.yrel;
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT && !mouse_capture) {
                        handle_gui_mouse((int)event.button.x, (int)event.button.y, window);
                    } else if (mouse_capture && event.button.button == SDL_BUTTON_LEFT) {
                        i_mouse_button_pressed = LEFT_MOUSE_BUTTON;
                    } else if (mouse_capture && event.button.button == SDL_BUTTON_RIGHT) {
                        i_mouse_button_pressed = RIGHT_MOUSE_BUTTON;
                    }
                    break;
                case SDL_EVENT_WINDOW_FOCUS_LOST:
                    if (mouse_capture) {
                        mouse_capture = 0;
                        updateCursorPos(window);
                    }
                    break;
            }
        }

        if (delta_time_t < update_time) delta_time_t = update_time;
        while (delta_time_t > 0) {
            float dt = min_c(delta_time_t, update_time);
            update_state(fix(dt / 1000));
            delta_time_t -= update_time;
        }

        if (mouse_capture && mouse_moved) {
            mouse_moved = 0;
            i_cursor_dx = 0;
            i_cursor_dy = 0;
        }

        if (redraw_scene) {
            draw();
            if (cfg.debug) draw_dbg();

            for (int i = 0; i < PC_SCREEN_WIDTH * PC_SCREEN_HEIGHT; i++) {
                u16 pixel = screen[i];

                framebuffer_rgb[i * 3]     = (pixel & 0x1F) * 255 / 31;
                framebuffer_rgb[i * 3 + 1] = ((pixel >> 5) & 0x1F) * 255 / 31;
                framebuffer_rgb[i * 3 + 2] = ((pixel >> 10) & 0x1F) * 255 / 31;
            }

            glViewport(0, 0, windows_width, windows_height);
            glClear(GL_COLOR_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                PC_SCREEN_WIDTH, PC_SCREEN_HEIGHT,
                GL_RGB, GL_UNSIGNED_BYTE, framebuffer_rgb);

            glUseProgram(presenter_program);
            glBindVertexArray(presenter_vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            
            draw_menu(windows_width, windows_height);

            SDL_GL_SwapWindow(window);
            redraw_scene = 0;
        }

        delta_time_f = (SDL_GetPerformanceCounter() - start_time) * performance_frequency;
        delta_time_t = delta_time_f;
        fps = delta_time_f > 0 ? 1000.0 / delta_time_f : 60;
        if (fps > 60) fps = 60;
        if (delta_time_f < update_time - 1) SDL_Delay((Uint32)(update_time - delta_time_f - 1));
    }

    free_memory();
    free(shared_face_dir);
    free(framebuffer_rgb);
    glDeleteTextures(1, &framebuffer_texture);
    glDeleteProgram(presenter_program);
    glDeleteVertexArrays(1, &presenter_vao);
    shutdown_menu();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

static char *load_text_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        SDL_Log("Failed to open shader: %s", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *source = malloc(size + 1);
    if (!source) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(source, 1, size, file);
    fclose(file);
    source[bytes_read] = '\0';

    return source;
}

static GLuint compile_shader(GLenum type, const char *path) {
    char *source = load_text_file(path);
    if (!source) return 0;

    GLuint shader = glCreateShader(type);
    const GLchar *shader_source = source;
    glShaderSource(shader, 1, &shader_source, NULL);
    free(source);

    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        SDL_Log("Shader compilation failed (%s): %s", path, log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static bool load_presenter(void) {
    GLuint vertex_shader = compile_shader(
        GL_VERTEX_SHADER, "shaders/present.vert");
    GLuint fragment_shader = compile_shader(
        GL_FRAGMENT_SHADER, "shaders/present.frag");

    if (!vertex_shader || !fragment_shader) {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }

    presenter_program = glCreateProgram();
    glAttachShader(presenter_program, vertex_shader);
    glAttachShader(presenter_program, fragment_shader);
    glLinkProgram(presenter_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    GLint success;
    glGetProgramiv(presenter_program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(presenter_program, sizeof(log), NULL, log);
        SDL_Log("Presenter link failed: %s", log);
        glDeleteProgram(presenter_program);
        presenter_program = 0;
        return false;
    }

    glUseProgram(presenter_program);
    glUniform1i(glGetUniformLocation(presenter_program, "framebuffer"), 0);
    glUseProgram(0);

    return true;
}

static void processKeyDown(SDL_Keycode key, bool repeat) {
    if (!repeat) {
        switch (key) {
            case SDLK_w: i_direction_pressed |= KEY_W; break;
            case SDLK_s: i_direction_pressed |= KEY_S; break;
            case SDLK_a: i_direction_pressed |= KEY_A; break;
            case SDLK_d: i_direction_pressed |= KEY_D; break;
            case SDLK_q: i_direction_pressed |= KEY_Q; break;
            case SDLK_e: i_direction_pressed |= KEY_E; break;
            case SDLK_KP_1: i_direction_pressed |= KEY_NUMPAD_1; break;
            case SDLK_KP_2: i_direction_pressed |= KEY_NUMPAD_2; break;
            case SDLK_KP_3: i_direction_pressed |= KEY_NUMPAD_3; break;
            case SDLK_KP_4: i_direction_pressed |= KEY_NUMPAD_4; break;
            case SDLK_KP_5: i_direction_pressed |= KEY_NUMPAD_5; break;
            case SDLK_KP_6: i_direction_pressed |= KEY_NUMPAD_6; break;
            case SDLK_RETURN: i_key_pressed = KEY_RETURN; break;
            case SDLK_SPACE: i_key_pressed = KEY_SPACE; break;
            case SDLK_LCTRL: case SDLK_RCTRL: i_key_pressed = KEY_CONTROL; break;
            case SDLK_BACKSPACE: i_key_pressed = KEY_BACK; break;
            case SDLK_COMMA: i_key_pressed = KEY_COMMA; break;
            case SDLK_PERIOD: i_key_pressed = KEY_POINT; break;
            case SDLK_MINUS: i_key_pressed = KEY_MINUS; break;
            case SDLK_c: i_key_pressed |= KEY_C; break;
            case SDLK_v: i_key_pressed |= KEY_V; break;
            case SDLK_b: i_key_pressed |= KEY_B; break;
            case SDLK_m: i_key_pressed |= KEY_M; break;
            case SDLK_p: i_key_pressed |= KEY_P; break;
            case SDLK_LSHIFT: case SDLK_RSHIFT: i_modifier_key_pressed |= KEY_SHIFT; break;
            case SDLK_f: cfg.debug ^= 1; redraw_scene = 1; break;
            case SDLK_g:
                if (!view_lines_mode) { view_lines_mode = 1; cfg.draw_lines = 1; }
                else if (view_lines_mode == 1) { view_lines_mode = 2; cfg.draw_polys = 0; }
                else { view_lines_mode = 0; cfg.draw_lines = 0; cfg.draw_polys = 1; }
                redraw_scene = 1;
                break;
            case SDLK_h: map_grid_enabled ^= 1; redraw_scene = 1; break;
            case SDLK_r: cfg.tx_perspective_mapping_enabled ^= 1; redraw_scene = 1; break;
            case SDLK_t: cfg.draw_textures ^= 1; redraw_scene = 1; break;
            case SDLK_o: export_map_file(file_path); break;
            case SDLK_ESCAPE: mouse_capture = 0; updateCursorPos(window); quit = true; break;
            case SDLK_F1: mouse_capture ^= 1; updateCursorPos(window); break;
            case SDLK_F2: enable_mouse_cursor ^= 1; updateCursorPos(window); break;
        }
    }

    switch (key) {
        case SDLK_UP: i_cursor_key_pressed |= KEY_UP; break;
        case SDLK_DOWN: i_cursor_key_pressed |= KEY_DOWN; break;
        case SDLK_LEFT: i_cursor_key_pressed |= KEY_LEFT; break;
        case SDLK_RIGHT: i_cursor_key_pressed |= KEY_RIGHT; break;
        case SDLK_z: i_cursor_key_pressed |= KEY_Z; break;
        case SDLK_x: i_cursor_key_pressed |= KEY_X; break;
        case SDLK_COMMA: if (repeat) i_key_pressed = KEY_COMMA; break;
        case SDLK_PERIOD: if (repeat) i_key_pressed = KEY_POINT; break;
    }
}

static void processKeyUp(SDL_Keycode key) {
    switch (key) {
        case SDLK_w: i_direction_pressed &= ~KEY_W; break;
        case SDLK_s: i_direction_pressed &= ~KEY_S; break;
        case SDLK_a: i_direction_pressed &= ~KEY_A; break;
        case SDLK_d: i_direction_pressed &= ~KEY_D; break;
        case SDLK_q: i_direction_pressed &= ~KEY_Q; break;
        case SDLK_e: i_direction_pressed &= ~KEY_E; break;
        case SDLK_KP_1: i_direction_pressed &= ~KEY_NUMPAD_1; break;
        case SDLK_KP_2: i_direction_pressed &= ~KEY_NUMPAD_2; break;
        case SDLK_KP_3: i_direction_pressed &= ~KEY_NUMPAD_3; break;
        case SDLK_KP_4: i_direction_pressed &= ~KEY_NUMPAD_4; break;
        case SDLK_KP_5: i_direction_pressed &= ~KEY_NUMPAD_5; break;
        case SDLK_KP_6: i_direction_pressed &= ~KEY_NUMPAD_6; break;
        case SDLK_LSHIFT: case SDLK_RSHIFT: i_modifier_key_pressed &= ~KEY_SHIFT; break;
    }
}

static void updateCursorPos(SDL_Window *window) {
    SDL_SetWindowRelativeMouseMode(window, mouse_capture);
    SDL_ShowCursor(enable_mouse_cursor || !mouse_capture);
}
