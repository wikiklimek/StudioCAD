#version 460 core
layout (points) in;
layout (line_strip, max_vertices = 200) out;

uniform mat4 view;
uniform mat4 projection;
uniform int segmentCount;

uniform vec3 a_coeff;
uniform vec3 b_coeff;
uniform vec3 c_coeff;
uniform vec3 d_coeff;
uniform float d_len;

void main() {
    int segs = clamp(segmentCount, 1, 199);
    for (int i = 0; i <= segs; ++i) {
        float tau = (float(i) / float(segs)) * d_len;
        vec3 pos = a_coeff + b_coeff * tau + c_coeff * (tau * tau) + d_coeff * (tau * tau * tau);
        gl_Position = projection * view * vec4(pos, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}