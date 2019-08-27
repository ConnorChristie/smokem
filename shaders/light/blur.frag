#version 400

in float gLayer;
out float FragColor;
uniform sampler3D Density;
uniform vec3 InverseSize;
uniform float StepSize;
uniform float DensityScale;

float GetDensity(vec3 pos)
{
    return texture(Density, pos).x * DensityScale;
}

// This implements a super stupid filter in 3-Space that takes 7 samples.
// A three-pass seperable Gaussian would be way better.

void main()
{
    vec3 pos = InverseSize * vec3(gl_FragCoord.xy, gLayer);
    float e = StepSize;
    float z = e;
    float density = GetDensity(pos);
    density += GetDensity(pos + vec3(e,e,0));
    density += GetDensity(pos + vec3(-e,e,0));
    density += GetDensity(pos + vec3(e,-e,0));
    density += GetDensity(pos + vec3(-e,-e,0));
    density += GetDensity(pos + vec3(0,0,-z));
    density += GetDensity(pos + vec3(0,0,+z));
    density /= 7;
    FragColor =  density;
}