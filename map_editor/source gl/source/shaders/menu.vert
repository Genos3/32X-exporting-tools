#version 450 core

uniform vec2 screen_size;
uniform vec4 rect;

out vec2 tex_coord;

void main() {
    const vec2 positions[6] = vec2[](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    );

    vec2 position = positions[gl_VertexID];
    vec2 pixel_position = rect.xy + position * rect.zw;

    gl_Position = vec4(
        pixel_position.x / screen_size.x * 2.0 - 1.0,
        1.0 - pixel_position.y / screen_size.y * 2.0,
        0.0,
        1.0
    );

    tex_coord = position;
}