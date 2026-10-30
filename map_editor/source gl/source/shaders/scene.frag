#version 450 core

uniform sampler2D texture_sampler;

in vec2 uv;
in vec3 color;
in float textured;
in float lighting;

out vec4 frag_color;

void main() {
    if (textured > 0.5) {
        vec4 texel = texture(texture_sampler, uv);
        frag_color = vec4(texel.rgb * lighting, texel.a);
    } else {
        frag_color = vec4(color * lighting, 1.0);
    }
}