#version 450 core

uniform sampler2D text_texture;
uniform vec4 color;
uniform bool use_texture;

in vec2 tex_coord;
out vec4 frag_color;

void main() {
    frag_color = use_texture
        ? texture(text_texture, tex_coord) * color
        : color;
}