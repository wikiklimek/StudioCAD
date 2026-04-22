#version 460 core
layout (lines_adjacency) in;
layout (line_strip, max_vertices = 200) out;

uniform mat4 view;
uniform mat4 projection;
uniform int segmentCount;
uniform int degree;

void main()
{
    int segs = clamp(segmentCount, 1, 199);

    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 p3 = gl_in[3].gl_Position.xyz;

    for (int i = 0; i <= segs; ++i)
    {
        float t = float(i) / float(segs);
        float u = 1.0 - t;
        vec3 pos = vec3(0.0);

        if (degree == 3)
        {
            pos = u*u*u*p0 + 3.0*u*u*t*p1 + 3.0*u*t*t*p2 + t*t*t*p3;
        }
        else if (degree == 2)
        {
            pos = u*u*p0 + 2.0*u*t*p1 + t*t*p2;
        }
        else if (degree == 1)
        {
            pos = u*p0 + t*p1;
        }

        // Dopiero teraz mnożymy przez kamerę!
        gl_Position = projection * view * vec4(pos, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}