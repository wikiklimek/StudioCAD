#version 460 core
out vec4 FragColor;

// Odbieramy kolory i parametr odbicia
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform float shininessM;

void main()
{
    // Na razie mieszamy kolor obiektu ze światłem (bardzo proste oświetlenie).
    // Dodajemy shininessM w losowy sposób tylko po to, by kompilator zmiennej nie usunął.
    vec3 finalColor = objectColor * lightColor * (shininessM);
    FragColor = vec4(finalColor, 1.0);
}