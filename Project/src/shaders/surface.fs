#version 410 core
out vec4 FragColor;

in vec2 fsUV;

uniform vec3 objectColor;
uniform bool isStereo;
uniform vec3 stereoColor;

uniform sampler2D trimMap;
uniform bool useTrim;
uniform bool trimFlip;

void main()
{
    if (useTrim) {
        float maskValue = texture(trimMap, fsUV).r;
        bool keep = maskValue > 0.5;
        if (trimFlip) keep = !keep;
        if (!keep) discard;
    }

    if (isStereo) FragColor = vec4(stereoColor, 1.0);
    else FragColor = vec4(objectColor, 1.0);
}