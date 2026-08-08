#version 450

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;
layout(location = 0) out vec4 out_color;

void main() {
    float distance = length(in_uv * 2.0 - 1.0);
    float edge = fwidth(distance);
    float alpha = 1.0 - smoothstep(1.0 - edge, 1.0 + edge, distance);
    if (alpha <= 0.0)
        discard;
    out_color = vec4(in_color.rgb, in_color.a * alpha);
}
