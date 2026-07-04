#version 450

layout(location = 0) in vec3 in_pos;

layout(set = 1, binding = 0) uniform VertexUniforms {
    mat4 u_mvp;
};

void main() {
    gl_Position = u_mvp * vec4(in_pos, 1.0);
}
