#version 400

in float gLayer;
out float FragColor;

uniform sampler3D Density;
uniform vec3 LightPosition = vec3(1.0, 1.0, 2.0);
uniform float LightIntensity = 10.0;
uniform float Absorption = 10.0;
uniform float LightStep;
uniform int LightSamples;
uniform vec3 InverseSize;

float GetDensity(vec3 pos)
{
    return texture(Density, pos).x;
}

void main()
{
    vec3 pos = InverseSize * vec3(gl_FragCoord.xy, gLayer);
    vec3 lightDir = normalize(LightPosition-pos) * LightStep;
    float Tl = 1.0;
    vec3 lpos = pos + lightDir;
    
    for (int s = 0; s < LightSamples; ++s) {
        float ld = GetDensity(lpos);
        Tl *= 1.0 - Absorption * LightStep * ld;
        if (Tl <= 0.01)
            break;

        // Would be faster if this coniditional is replaced with a tighter loop
        if (lpos.x < 0 || lpos.y < 0 || lpos.z < 0 ||
            lpos.x > 1 || lpos.y > 1 || lpos.z > 1)
            break;

        lpos += lightDir;
    }

    float Li = LightIntensity*Tl;
    FragColor = Li;
}