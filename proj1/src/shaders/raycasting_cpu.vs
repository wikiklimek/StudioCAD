#version 460 core
layout (location = 0) in vec3 aPos;

out vec2 TexCoords;

void main()
{
    // (-1 do 1) -> (0 do 1)
    TexCoords = aPos.xy * 0.5 + 0.5;
    gl_Position = vec4(aPos, 1.0);
}
