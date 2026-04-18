#version 460 core
layout (lines_adjacency) in;
layout (line_strip, max_vertices = 200) out;

uniform mat4 view;
uniform mat4 projection;
uniform int segmentCount;

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
        float t2 = t * t;
        float t3 = t2 * t;

        // Baza B-Spline wyliczona bezpośrednio na GPU!
        vec3 pos = (
            (-p0 + 3.0*p1 - 3.0*p2 + p3) * t3 +
            (3.0*p0 - 6.0*p1 + 3.0*p2) * t2 +
            (-3.0*p0 + 3.0*p2) * t +
            (p0 + 4.0*p1 + p2)
        ) / 6.0;

        gl_Position = projection * view * vec4(pos, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}