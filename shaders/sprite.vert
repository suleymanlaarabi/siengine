#version 450

layout(location = 0) in vec2 unit_position;
layout(location = 1) in vec2 instance_position;
layout(location = 2) in float instance_rotation;
layout(location = 3) in vec2 instance_scale;
layout(location = 4) in vec2 instance_size;
layout(location = 5) in vec4 instance_color;
layout(location = 6) in float instance_frame;
layout(location = 7) in vec2 instance_flip;

layout(set = 1, binding = 0, std140) uniform Scene {
    vec4 bounds;
    vec4 texture_size;
    vec4 sheet;
    vec4 sheet_layout;
    vec4 pivot;
};

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

void main() {
    vec2 local = (unit_position - pivot.xy) * instance_size * instance_scale;
    float c = cos(instance_rotation);
    float s = sin(instance_rotation);
    vec2 world = instance_position + vec2(local.x * c - local.y * s, local.x * s + local.y * c);
    gl_Position = vec4(
        (world.x - bounds.x) / (bounds.z - bounds.x) * 2.0 - 1.0,
        1.0 - (world.y - bounds.y) / (bounds.w - bounds.y) * 2.0,
        0.0,
        1.0
    );

    vec2 uv = unit_position;
    if (sheet.x > 0.0) {
        float column = mod(instance_frame, sheet.x);
        float row = floor(instance_frame / sheet.x);
        vec2 origin = sheet_layout.xy + vec2(
            column * (sheet.z + sheet_layout.z),
            row * (sheet.w + sheet_layout.w)
        );
        uv = (origin + unit_position * sheet.zw) / texture_size.xy;
    }
    if (instance_flip.x > 0.5)
        uv.x = 2.0 * (sheet.x > 0.0 ? (sheet_layout.x + unit_position.x * sheet.z) / texture_size.x : unit_position.x) - uv.x;
    if (instance_flip.y > 0.5)
        uv.y = 2.0 * (sheet.x > 0.0 ? (sheet_layout.y + unit_position.y * sheet.w) / texture_size.y : unit_position.y) - uv.y;
    out_uv = uv;
    out_color = instance_color;
}
