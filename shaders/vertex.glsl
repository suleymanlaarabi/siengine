#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec4 in_model_col0;
layout(location = 2) in vec4 in_model_col1;
layout(location = 3) in vec4 in_model_col2;
layout(location = 4) in vec4 in_model_col3;
layout(location = 5) in vec4 in_color;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 u_view_projection;
};

void main() {
    mat4 model = mat4(in_model_col0, in_model_col1, in_model_col2, in_model_col3);
    gl_Position = u_view_projection * model * vec4(in_pos, 1.0);
    out_color = in_color;
}
