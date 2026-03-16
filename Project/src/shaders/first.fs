#version 460 core
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform float shininessM;

void main()
{
    vec3 finalColor = objectColor * lightColor * (shininessM);
    FragColor = vec4(finalColor, 1.0);
}