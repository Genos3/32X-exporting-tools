#version 450 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_color;
layout(location = 3) in float in_textured;
layout(location = 4) in float in_lighting;

uniform vec2 screen_size;

out vec2 uv;
out vec3 color;
out float textured;
out float lighting;

void main() {
    vec2 position = in_position / screen_size * 2.0 - 1.0;

    gl_Position = vec4(position.x, -position.y, 0.0, 1.0);

    uv = in_uv;
    color = in_color;
    textured = in_textured;
    lighting = in_lighting;
}