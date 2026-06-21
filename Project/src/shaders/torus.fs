#version 460 core
out vec4 FragColor;

in vec3 localPos;

uniform vec3 objectColor;
uniform bool isStereo;
uniform vec3 stereoColor;

uniform float torusR;
uniform sampler2D trimMap;
uniform bool useTrim;
uniform bool trimFlip;

void main()
{
    if (useTrim) {
        float PI = 3.14159265359;

        // Liczymy UV idealnie, niezależnie dla KAŻDEGO PIKSELA
        float u = atan(localPos.y, localPos.x);
        if (u < 0.0)
        {
            u += 2.0 * PI;
        }

        float v = atan(localPos.z, length(localPos.xy) - torusR);
        if (v < 0.0)
        {
            v += 2.0 * PI;
        }

        vec2 fsUV = vec2(u / (2.0 * PI), v / (2.0 * PI));

        float maskValue = texture(trimMap, fsUV).r;
        bool keep = maskValue > 0.5;
        if (trimFlip) keep = !keep;
        if (!keep) discard;
    }

    if(isStereo) FragColor = vec4(stereoColor, 1.0);
    else FragColor = vec4(objectColor, 1.0);
}