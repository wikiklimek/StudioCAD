#version 410 core
out vec4 FragColor;

uniform vec3 objectColor;
uniform bool isStereo;
uniform vec3 stereoColor;

void main()
{
    if (isStereo)
        FragColor = vec4(stereoColor, 1.0);
    else
        FragColor = vec4(objectColor, 1.0);
}