#version 460 core
uniform vec3 p[4];
uniform int segmentCount;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    float t = float(gl_VertexID) / float(segmentCount);
    float t2 = t * t;
    float t3 = t2 * t;

    vec3 pos = (
        (-p[0] + 3.0*p[1] - 3.0*p[2] + p[3]) * t3 +
        (3.0*p[0] - 6.0*p[1] + 3.0*p[2]) * t2 +
        (-3.0*p[0] + 3.0*p[2]) * t +
        (p[0] + 4.0*p[1] + p[2])
    ) / 6.0;

    gl_Position = projection * view * vec4(pos, 1.0);
}