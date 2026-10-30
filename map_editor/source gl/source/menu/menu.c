#include <SDL3_ttf/SDL_ttf.h>
#include "common.h"

static void draw_menu_rect(float x, float y, float width, float height,
    float red, float green, float blue);
static void draw_menu_text(int index, float x, float y);
static void save_file_callback(void *userdata, const char * const *filelist, int filter);
static void save_file_as(SDL_Window *window);

static bool file_menu_open;
static char save_path[4096];

static TTF_Font *menu_font;
static GLuint menu_program;
static GLuint menu_vao;
static GLuint menu_textures[3];
static int menu_text_width[3];
static int menu_text_height[3];

static const char *menu_text[] = {
    "File",
    "Save",
    "Save As..."
};

bool init_menu(void) {
    if (!TTF_Init()) {
        SDL_Log("TTF_Init: %s", SDL_GetError());
        return false;
    }

    char *base_path = SDL_GetBasePath();
    char font_path[1024];

    SDL_snprintf(font_path, sizeof(font_path),
        "%sassets/fonts/arial.ttf", base_path);

    menu_font = TTF_OpenFont(font_path, 14.0f);

    SDL_free(base_path);

    if (!menu_font) {
        SDL_Log("TTF_OpenFont: %s", SDL_GetError());
        return false;
    }

    GLuint vertex_shader = compile_shader(
        GL_VERTEX_SHADER, "shaders/menu.vert");
    GLuint fragment_shader = compile_shader(
        GL_FRAGMENT_SHADER, "shaders/menu.frag");

    if (!vertex_shader || !fragment_shader) {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }

    menu_program = glCreateProgram();
    glAttachShader(menu_program, vertex_shader);
    glAttachShader(menu_program, fragment_shader);
    glLinkProgram(menu_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    GLint success;
    glGetProgramiv(menu_program, GL_LINK_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetProgramInfoLog(menu_program, sizeof(log), NULL, log);
        SDL_Log("Menu shader link failed: %s", log);
        glDeleteProgram(menu_program);
        menu_program = 0;
        return false;
    }

    glGenVertexArrays(1, &menu_vao);
    glGenTextures(3, menu_textures);

    for (int i = 0; i < 3; i++) {
        SDL_Surface *surface = TTF_RenderText_Blended(
            menu_font, menu_text[i], 0,
            (SDL_Color){230, 230, 230, 255});

        if (!surface) {
            SDL_Log("TTF_RenderText_Blended: %s", SDL_GetError());
            return false;
        }

        SDL_Surface *rgba_surface = SDL_ConvertSurface(
            surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);

        if (!rgba_surface) {
            SDL_Log("SDL_ConvertSurface: %s", SDL_GetError());
            return false;
        }

        menu_text_width[i] = rgba_surface->w;
        menu_text_height[i] = rgba_surface->h;

        glBindTexture(GL_TEXTURE_2D, menu_textures[i]);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA8,
            rgba_surface->w, rgba_surface->h, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, rgba_surface->pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        SDL_DestroySurface(rgba_surface);
    }

    return true;
}

static void draw_menu_rect(float x, float y, float width, float height,
    float red, float green, float blue) {
    glUniform4f(
        glGetUniformLocation(menu_program, "rect"),
        x, y, width, height);

    glUniform4f(
        glGetUniformLocation(menu_program, "color"),
        red, green, blue, 1.0f);

    glUniform1i(
        glGetUniformLocation(menu_program, "use_texture"), GL_FALSE);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void draw_menu_text(int index, float x, float y) {
    glUniform4f(
        glGetUniformLocation(menu_program, "rect"),
        x, y, menu_text_width[index], menu_text_height[index]);

    glUniform4f(
        glGetUniformLocation(menu_program, "color"),
        1.0f, 1.0f, 1.0f, 1.0f);

    glUniform1i(
        glGetUniformLocation(menu_program, "use_texture"), GL_TRUE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, menu_textures[index]);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void draw_menu(int width, int height) {
    glUseProgram(menu_program);
    glBindVertexArray(menu_vao);

    glUniform2f(
        glGetUniformLocation(menu_program, "screen_size"),
        (float)width, (float)height);

    glUniform1i(
        glGetUniformLocation(menu_program, "text_texture"), 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    draw_menu_rect(0, 0, (float)width, 26,
        40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f);

    draw_menu_rect(0, 0, 48, 26,
        (file_menu_open ? 75.0f : 55.0f) / 255.0f,
        55.0f / 255.0f, 55.0f / 255.0f);

    draw_menu_text(0, 8, 6);

    if (file_menu_open) {
        draw_menu_rect(0, 26, 170, 52,
            45.0f / 255.0f, 45.0f / 255.0f, 45.0f / 255.0f);

        draw_menu_text(1, 8, 32);
        draw_menu_text(2, 8, 58);
    }

    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glUseProgram(0);
}

void handle_gui_mouse(int x, int y, SDL_Window *window) {
    if (y < 26 && x < 48) {
        file_menu_open = !file_menu_open;
        redraw_scene = 1;
        return;
    }

    if (file_menu_open && x >= 0 && x < 170 && y >= 26 && y < 78) {
        if (y < 52) {
            export_map_file(file_path);
        } else {
            save_file_as(window);
        }

        file_menu_open = false;
        return;
    }

    file_menu_open = false;
}

static void save_file_callback(void *userdata, const char * const *filelist, int filter) {
    if (!filelist || !filelist[0]) return;

    SDL_strlcpy(save_path, filelist[0], sizeof(save_path));
    file_path = save_path;
    export_map_file(file_path);
}

static void save_file_as(SDL_Window *window) {
    const SDL_DialogFileFilter filters[] = {
        {"Map files", "map"}
    };

    SDL_ShowSaveFileDialog(save_file_callback, NULL, window, filters, 1, NULL);
}

void shutdown_menu(void) {
    glDeleteTextures(3, menu_textures);
    glDeleteVertexArrays(1, &menu_vao);
    glDeleteProgram(menu_program);

    TTF_CloseFont(menu_font);
    TTF_Quit();
}