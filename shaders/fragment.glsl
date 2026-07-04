#version 450

layout(set = 3, binding = 0) uniform FragmentUniforms {
    vec4 u_color;
};

layout(location = 0) out vec4 out_color;

void main() {
    out_color = u_color;
}
