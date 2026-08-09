#ifndef SIENGINE_BACKEND_WEBGL_SHADERS_H
#define SIENGINE_BACKEND_WEBGL_SHADERS_H

static const char *vertex_shader_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "layout(location = 0) in vec2 unit_position;\n"
    "layout(location = 1) in vec2 instance_position;\n"
    "layout(location = 2) in float instance_rotation;\n"
    "layout(location = 3) in vec2 instance_scale;\n"
    "layout(location = 4) in vec2 instance_size;\n"
    "layout(location = 5) in vec4 instance_color;\n"
    "layout(location = 6) in float instance_frame;\n"
    "layout(location = 7) in vec2 instance_flip;\n"
    "uniform vec4 camera_bounds;\n"
    "uniform vec4 texture_size;\n"
    "uniform vec4 sheet;\n"
    "uniform vec4 sheet_layout;\n"
    "uniform vec4 pivot;\n"
    "out vec2 out_uv;\n"
    "out vec4 out_color;\n"
    "void main() {\n"
    "  vec2 local = (unit_position - pivot.xy) * instance_size * instance_scale;\n"
    "  float c = cos(instance_rotation);\n"
    "  float s = sin(instance_rotation);\n"
    "  vec2 world = instance_position + vec2(local.x * c - local.y * s, local.x * s + local.y * c);\n"
    "  gl_Position = vec4((world.x - camera_bounds.x) / (camera_bounds.z - camera_bounds.x) * 2.0 - 1.0, 1.0 - (world.y - camera_bounds.y) / (camera_bounds.w - camera_bounds.y) * 2.0, 0.0, 1.0);\n"
    "  vec2 uv = unit_position;\n"
    "  if (sheet.x > 0.0) {\n"
    "    float column = mod(instance_frame, sheet.x);\n"
    "    float row = floor(instance_frame / sheet.x);\n"
    "    vec2 origin = sheet_layout.xy + vec2(column * (sheet.z + sheet_layout.z), row * (sheet.w + sheet_layout.w));\n"
    "    uv = (origin + unit_position * sheet.zw) / texture_size.xy;\n"
    "    if (instance_flip.x > 0.5) uv.x = (origin.x + sheet.z) / texture_size.x - (uv.x - origin.x / texture_size.x);\n"
    "    if (instance_flip.y > 0.5) uv.y = (origin.y + sheet.w) / texture_size.y - (uv.y - origin.y / texture_size.y);\n"
    "  } else {\n"
    "    if (instance_flip.x > 0.5) uv.x = 1.0 - uv.x;\n"
    "    if (instance_flip.y > 0.5) uv.y = 1.0 - uv.y;\n"
    "  }\n"
    "  out_uv = uv;\n"
    "  out_color = instance_color;\n"
    "}\n";

static const char *sprite_fragment_shader_source =
    "#version 300 es\nprecision highp float;\n"
    "uniform sampler2D sprite_texture;\n"
    "in vec2 out_uv; in vec4 out_color; out vec4 color;\n"
    "void main() { color = texture(sprite_texture, out_uv) * out_color; }\n";

static const char *shape_fragment_shader_source =
    "#version 300 es\nprecision highp float;\n"
    "in vec4 out_color; out vec4 color;\n"
    "void main() { color = out_color; }\n";

static const char *circle_fragment_shader_source =
    "#version 300 es\nprecision highp float;\n"
    "in vec2 out_uv; in vec4 out_color; out vec4 color;\n"
    "void main() { float d = length(out_uv * 2.0 - 1.0); float e = fwidth(d); float a = 1.0 - smoothstep(1.0 - e, 1.0 + e, d); if (a <= 0.0) discard; color = vec4(out_color.rgb, out_color.a * a); }\n";

#endif
