#version 460 core
uniform vec3 p[4];
uniform int segmentCount;
uniform int degree;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    float t = float(gl_VertexID) / float(segmentCount);
    float u = 1.0 - t;
    vec3 pos = vec3(0.0);

    if (degree == 3)
    {
        pos = u*u*u*p[0] + 3.0*u*u*t*p[1] + 3.0*u*t*t*p[2] + t*t*t*p[3];
    }
    else if (degree == 2)
    {
        pos = u*u*p[0] + 2.0*u*t*p[1] + t*t*p[2];
    }
    else if (degree == 1)
    {
        pos = u*p[0] + t*p[1];
    }

    gl_Position = projection * view * vec4(pos, 1.0);
}