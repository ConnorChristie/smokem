#version 400

out vec4 FragColor;

uniform sampler3D Density;
uniform sampler3D LightCache;

in vec2 TexCoord;
in vec4 VertexPosition;

uniform mat4 ModelMatrix;
uniform float FocalLength;
uniform vec2 WindowSize;
uniform float StepSize;
uniform int ViewSamples;
uniform vec3 CameraPosition;
uniform vec3 RayOrigin;

uniform vec3 Ambient = vec3(0.15, 0.15, 0.20);
uniform float Absorption = 10.0;

struct Ray {
    vec3 Origin;
    vec3 Dir;
};

struct AABB {
    vec3 Min;
    vec3 Max;
};

bool IntersectBox(Ray r, AABB aabb, out float tNear, out float tFar)
{
    vec3 tMin = (aabb.Min - r.Origin) / r.Dir;
    vec3 tMax = (aabb.Max - r.Origin) / r.Dir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    tNear = max(max(t1.x, t1.y), t1.z);
    tFar = min(min(t2.x, t2.y), t2.z);
    return tNear > 0.0 && tNear < tFar;
}

void main()
{
    vec3 rayDirection;
    rayDirection.xy = 2.0 * gl_FragCoord.xy / WindowSize - 1.0;
    rayDirection.x /= WindowSize.y / WindowSize.x;
    rayDirection.z = -FocalLength;
    rayDirection = normalize((vec4(rayDirection, 0) * ModelMatrix).xyz);

    Ray eye = Ray(RayOrigin, rayDirection);
    AABB aabb = AABB(vec3(-1), vec3(1));

    float tnear, tfar;
    if (!IntersectBox(eye, aabb, tnear, tfar))
    {
        return;
    }

    vec3 rayStart = eye.Origin + eye.Dir * tnear;
    vec3 rayStop = eye.Origin + eye.Dir * tfar;

    vec3 pos = rayStart;
    vec3 viewDir = normalize(rayStop - rayStart) * StepSize;

    float remainingLength = distance(rayStop, rayStart);
    float T = 1.0;

    vec3 Lo = Ambient;

    for (int i = 0; i < ViewSamples && remainingLength > 0.0; ++i, pos += viewDir, remainingLength -= StepSize)
    {
        float density = texture(Density, pos).x;
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

    // vec4 tex = texture(Density, vec3(2.0 * gl_FragCoord.xy / WindowSize - 1.0, 1));
    // FragColor = vec4(tex.xyz, 1);

    // FragColor = vec4(texture(Density, VertexPosition.xyz).xxx, 1);
}