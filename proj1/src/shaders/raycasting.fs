#version 460 core
out vec4 FragColor;

// Parametry geometrii
uniform float a, b, c;

// Parametry Phonga
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform float m;

// Rysowanie adaptacyjne
uniform float pixelSize;
uniform vec2 resolution;

void main()
{
    // ===============================================================
    // 1. RYSOWANIE ADAPTACYJNE (Sub-sampling)
    // Zaokrąglamy współrzędne ekranu do siatki 'pixelSize'.
    // Jeśli pixelSize = 10, piksele od 0 do 9 dostaną tę samą wartość uv!
    // ===============================================================
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 steppedCoord = floor(fragCoord / pixelSize) * pixelSize + (pixelSize * 0.5);

    // Przeliczamy to z powrotem na układ -1.0 do 1.0
    vec2 uv = (steppedCoord / resolution) * 2.0 - 1.0;

    // Kamera
    vec3 rayOrigin = vec3(0.0, 0.0, 3.0);
    vec3 rayDir = normalize(vec3(uv.x, uv.y, -1.0));

    // ===============================================================
    // 2. RAY CASTING (Bez zmian, równanie elipsoidy)
    // ===============================================================
    float Aq = a * rayDir.x * rayDir.x + b * rayDir.y * rayDir.y + c * rayDir.z * rayDir.z;
    float Bq = 2.0 * (a * rayOrigin.x * rayDir.x + b * rayOrigin.y * rayDir.y + c * rayOrigin.z * rayDir.z);
    float Cq = a * rayOrigin.x * rayOrigin.x + b * rayOrigin.y * rayOrigin.y + c * rayOrigin.z * rayOrigin.z - 1.0;

    float delta = Bq * Bq - 4.0 * Aq * Cq;
    if (delta < 0.0) discard;

    float t = (-Bq - sqrt(delta)) / (2.0 * Aq);
    if (t < 0.0) discard;

    vec3 hitPoint = rayOrigin + t * rayDir;

    // ===============================================================
    // 3. OŚWIETLENIE PHONGA Z TWOJEGO WZORU
    // ===============================================================

    // Wektor normalny 'n' na powierzchni bryły (znormalizowany gradient)
    vec3 n = normalize(vec3(2.0 * a * hitPoint.x, 2.0 * b * hitPoint.y, 2.0 * c * hitPoint.z));

    // Twój wektor 'w' - wersor do obserwatora z punktu uderzenia
    // Skoro obserwator to rayOrigin, wektor to rayOrigin - hitPoint
    vec3 w = normalize(rayOrigin - hitPoint);

    // Iloczyn skalarny (w dot n) - musi być zabezpieczony przed wartościami ujemnymi
    float w_dot_n = max(dot(w, n), 0.0);

    // Twój wzór na Specular: (w dot n) ^ m
    float specularTerm = pow(w_dot_n, m);

    // Mieszanie kolorów:
    // Kolor bazowy obiektu + błysk światła
    //vec3 finalColor = (objectColor * w_dot_n) + (lightColor * specularTerm);
    vec3 finalColor = (lightColor * specularTerm) * objectColor;

    FragColor = vec4(finalColor, 1.0);
}