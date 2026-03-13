#version 460 core
out vec4 FragColor;

uniform float a, b, c;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform float m;

uniform float pixelSize;
uniform vec2 resolution;

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 steppedCoord = floor(fragCoord / pixelSize) * pixelSize + (pixelSize * 0.5);

    vec2 uv = (steppedCoord / resolution) * 2.0 - 1.0;

    vec3 rayOrigin = vec3(0.0, 0.0, 3.0);
    vec3 rayDir = normalize(vec3(uv.x, uv.y, -1.0));

    float Aq = a * rayDir.x * rayDir.x + b * rayDir.y * rayDir.y + c * rayDir.z * rayDir.z;
    float Bq = 2.0 * (a * rayOrigin.x * rayDir.x + b * rayOrigin.y * rayDir.y + c * rayOrigin.z * rayDir.z);
    float Cq = a * rayOrigin.x * rayOrigin.x + b * rayOrigin.y * rayOrigin.y + c * rayOrigin.z * rayOrigin.z - 1.0;

    float delta = Bq * Bq - 4.0 * Aq * Cq;
    if (delta < 0.0) discard;

    float t = (-Bq - sqrt(delta)) / (2.0 * Aq);
    if (t < 0.0) discard;

    vec3 hitPoint = rayOrigin + t * rayDir;

    vec3 n = normalize(vec3(2.0 * a * hitPoint.x, 2.0 * b * hitPoint.y, 2.0 * c * hitPoint.z));

    vec3 w = normalize(rayOrigin - hitPoint);

    float w_dot_n = max(dot(w, n), 0.0);

    float specularTerm = pow(w_dot_n, m);

    vec3 finalColor = (lightColor * specularTerm) * objectColor;

    FragColor = vec4(finalColor, 1.0);
}