#version 460 core
layout (location = 0) in vec3 aPos;

uniform float a;
uniform float b;
uniform float c;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}