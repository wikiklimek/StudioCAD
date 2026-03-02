#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

// To jest nasz "ekran" policzony na procesorze i przesłany jako tekstura
uniform sampler2D screenTexture;

void main()
{
    FragColor = texture(screenTexture, TexCoords);
}