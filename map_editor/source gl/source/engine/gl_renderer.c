#include <stddef.h>
#include "gl_renderer.h"

#define GL_BATCH_MAX_VERTICES 4096

typedef struct {
    float x, y;
    float u, v;
    float r, g, b;
    float textured;
} gl_vertex_t;

static gl_vertex_t batch[GL_BATCH_MAX_VERTICES];
static int batch_count;

static GLuint scene_program;
static GLuint scene_vao;
static GLuint scene_vbo;
static GLuint current_texture;

static GLint screen_size_location;

void gl_renderer_init(void) {
    GLuint vertex_shader = compile_shader(
        GL_VERTEX_SHADER, "shaders/scene.vert");
    GLuint fragment_shader = compile_shader(
        GL_FRAGMENT_SHADER, "shaders/scene.frag");
    
    scene_program = glCreateProgram();
    glAttachShader(scene_program, vertex_shader);
    glAttachShader(scene_program, fragment_shader);
    glLinkProgram(scene_program);
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    glGenVertexArrays(1, &scene_vao);
    glGenBuffers(1, &scene_vbo);
    
    glBindVertexArray(scene_vao);
    glBindBuffer(GL_ARRAY_BUFFER, scene_vbo);
    
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(batch),
        NULL,
        GL_STREAM_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        sizeof(gl_vertex_t),
        (void *)offsetof(gl_vertex_t, x));
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        sizeof(gl_vertex_t),
        (void *)offsetof(gl_vertex_t, u));
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 3, GL_FLOAT, GL_FALSE,
        sizeof(gl_vertex_t),
        (void *)offsetof(gl_vertex_t, r));
    
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3, 1, GL_FLOAT, GL_FALSE,
        sizeof(gl_vertex_t),
        (void *)offsetof(gl_vertex_t, textured));
    
    glBindVertexArray(0);
    
    screen_size_location = glGetUniformLocation(
        scene_program, "screen_size");
    
    glUseProgram(scene_program);
    glUniform1i(
        glGetUniformLocation(scene_program, "texture_sampler"), 0);
    glUseProgram(0);
}

void gl_renderer_begin(void) {
    batch_count = 0;
    current_texture = 0;
    
    glUseProgram(scene_program);
    glBindVertexArray(scene_vao);
}

void gl_renderer_flush(void) {
    if (!batch_count) {
        return;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, scene_vbo);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        batch_count * sizeof(gl_vertex_t),
        batch);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, current_texture);
    
    glDrawArrays(GL_TRIANGLES, 0, batch_count);
    
    batch_count = 0;
}

void gl_renderer_submit(g_poly_t *poly, GLuint texture) {
    if (texture != current_texture ||
        batch_count + (poly->num_vertices - 2) * 3 >
            GL_BATCH_MAX_VERTICES) {
        gl_renderer_flush();
        current_texture = texture;
    }
    
    const float fixed_scale = 1.0f / (1 << FP);
    const float red = (poly->color & 31) / 31.0f;
    const float green = ((poly->color >> 5) & 31) / 31.0f;
    const float blue = ((poly->color >> 10) & 31) / 31.0f;
    const float texture_width = (float)(poly->texture_width_s + 1);
    const float texture_height = (float)(poly->texture_height_s + 1);
    
    // triangulate the face
    
    for (int i = 1; i < poly->num_vertices - 1; i++) {
        int indices[3] = {0, i, i + 1};
        
        for (int j = 0; j < 3; j++) {
            vec5_t *vertex = &poly->vertices[indices[j]];
            gl_vertex_t *out = &batch[batch_count++];
            
            out->x = vertex->x * fixed_scale;
            out->y = vertex->y * fixed_scale;
            
            out->u = texture ? vertex->u / texture_width : 0.0f;
            out->v = texture ? vertex->v / texture_height : 0.0f;
            
            out->r = red;
            out->g = green;
            out->b = blue;
            out->textured = texture != 0;
        }
    }
}

void gl_renderer_shutdown(void) {
    gl_renderer_flush();
    
    glDeleteBuffers(1, &scene_vbo);
    glDeleteVertexArrays(1, &scene_vao);
    glDeleteProgram(scene_program);
}

void gl_load_model_textures(textures_t *textures) {
    textures->gl_texture_ids = malloc(
        textures->num_textures * LIGHT_GRD * sizeof(GLuint)
    );
    
    for (int tx_id = 0; tx_id < textures->num_textures; tx_id++) {
        int width = textures->texture_sizes_padded[tx_id].w + 1;
        int height = textures->texture_sizes_padded[tx_id].h + 1;
        
        u16 *texture_data = textures->texture_data + textures->tx_index[tx_id];
        u8 *rgba = malloc(width * height * 4);
        
        for (int light = 0; light < LIGHT_GRD; light++) {
            for (int i = 0; i < width * height; i++) {
                u16 texel = texture_data[i];
                u8 *pixel = &rgba[i * 4];
                
                if (!texel) {
                    pixel[0] = 0;
                    pixel[1] = 0;
                    pixel[2] = 0;
                    pixel[3] = 0;
                    continue;
                }
                
                u8 color = textures->cr_palette_tx_idx[
                    (texel << LIGHT_GRD_BITS) + light
                ] & 255;
                
                u8 b = color & 31;
                u8 g = (color >> 5) & 31;
                u8 r = (color >> 10) & 31;
                
                pixel[0] = (r << 3) | (r >> 2);
                pixel[1] = (g << 3) | (g >> 2);
                pixel[2] = (b << 3) | (b >> 2);
                pixel[3] = 255;
            }
            
            GLuint *texture_id =
                &textures->gl_texture_ids[tx_id * LIGHT_GRD + light];
            
            glGenTextures(1, texture_id);
            glBindTexture(GL_TEXTURE_2D, *texture_id);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA8,
                width,
                height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                rgba
            );
        }
        
        free(rgba);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
}