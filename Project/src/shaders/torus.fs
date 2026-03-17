#version 460 core
out vec4 FragColor;

uniform vec3 objectColor;

void main()
{
    // Podstawowy kolor obiektu - w kolejnych zadaniach dodasz tu oświetlenie
    FragColor = vec4(objectColor, 1.0);
}