#version 400

out vec4 FragColor;

uniform sampler3D Density;
uniform sampler3D LightCache;

uniform mat4 Modelview;
uniform float FocalLength;
uniform vec2 WindowSize;
uniform float StepSize;
uniform int ViewSamples;

uniform vec3 RayOrigin;

uniform vec3 LightPosition = vec3(1.0, 1.0, 2.0);
uniform vec3 LightIntensity = vec3(10.0);
uniform vec3 Ambient = vec3(0.15, 0.15, 0.20);
uniform float Absorption = 10.0;

float GetDensity(vec3 pos)
{
    return texture(Density, pos).x;
}

struct Ray {
    vec3 Origin;
    vec3 Dir;
};

struct AABB {
    vec3 Min;
    vec3 Max;
};

bool IntersectBox(Ray r, AABB aabb, out float near, out float far)
{
    vec3 invR = 1.0 / r.Dir;
    vec3 tbot = invR * (aabb.Min - r.Origin);
    vec3 ttop = invR * (aabb.Max - r.Origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    near = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    far = min(t.x, t.y);
    return near <= far;
}

void main()
{
    vec3 rayDirection;
    rayDirection.xy = 2.0 * gl_FragCoord.xy / WindowSize - 1.0;
    rayDirection.x /= WindowSize.y / WindowSize.x;
    rayDirection.z = -FocalLength;
    rayDirection = (vec4(rayDirection, 0) * Modelview).xyz;

    Ray eye = Ray( RayOrigin, normalize(rayDirection) );
    AABB aabb = AABB(vec3(-1), vec3(1));

    float tnear, tfar;
    IntersectBox(eye, aabb, tnear, tfar);
    if (tnear < 0.0) tnear = 0.0;

    vec3 rayStart = eye.Origin + eye.Dir * tnear;
    vec3 rayStop = eye.Origin + eye.Dir * tfar;

    vec3 pos = rayStart;
    vec3 viewDir = normalize(rayStop - rayStart) * StepSize;
    float T = 1.0;
    vec3 Lo = Ambient;

    float remainingLength = distance(rayStop, rayStart);

    for (int i = 0; i < ViewSamples && remainingLength > 0.0; ++i, pos += viewDir, remainingLength -= StepSize)
    {
        float density = GetDensity(pos);
        vec3 lightColor = vec3(1);
        if (pos.z < 0.1)
        {
            // density = 10;
            // lightColor = 3 * Ambient;
            continue;
        }
        else if (density <= 0.01)
        {
            continue;
        }

        T *= 1.0 - density * StepSize * Absorption;
        if (T <= 0.01)
            break;

        vec3 Li = lightColor * texture(LightCache, pos).xxx;
        Lo += Li * T * density * StepSize;
    }

    FragColor.rgb = Lo;
    FragColor.a = 1 - T;
}