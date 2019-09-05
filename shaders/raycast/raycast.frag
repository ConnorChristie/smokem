#version 400

out vec4 FragColor;

uniform sampler3D Density;
uniform sampler3D LightCache;

uniform mat4 InverseProjectionMatrix;
uniform mat4 InverseViewMatrix;
uniform float FocalLength;
uniform vec2 WindowSize;
uniform vec3 RayOrigin;
uniform vec3 Ambient = vec3(0.15, 0.15, 0.20);
uniform vec3 LightColor = vec3(1, 1, 1);
uniform float Absorption = 10.0;
uniform float LightSamples;
uniform int ViewSamples;

float GetDensity(vec3 pos)
{
    return texture(Density, pos).x;
}

struct Ray
{
    vec3 Origin;
    vec3 Dir;
};

struct AABB
{
    vec3 Min;
    vec3 Max;
};

bool IntersectBox(Ray r, AABB aabb, out float t0, out float t1)
{
    vec3 invR = 1.0 / r.Dir;
    vec3 tbot = invR * (aabb.Min - r.Origin);
    vec3 ttop = invR * (aabb.Max - r.Origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);

    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);

    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);

    return t0 <= t1;
}

void main()
{
    vec3 rayDirection = vec3((gl_FragCoord.xy * 2.0) / WindowSize - 1.0, -FocalLength);

    vec4 ray_clip = vec4(rayDirection, 1.0);
    vec4 ray_eye = vec4((InverseProjectionMatrix * ray_clip).xyz, 0.0);
    vec3 ray_wor = normalize((InverseViewMatrix * ray_eye).xyz);

    vec3 objPos = vec3(20, 0, 20);

    Ray eye = Ray(RayOrigin, ray_wor);
    AABB aabb = AABB(objPos, objPos + vec3(1) * 8);

    float tnear, tfar;
    if (!IntersectBox(eye, aabb, tnear, tfar)) return;

    float T = 1.0;
    vec3 Lo = Ambient;

    float stepSize = (tfar - tnear) / ViewSamples;

    for (int i = 0; i < ViewSamples; i++)
    {
        vec3 newPos = eye.Origin + eye.Dir * (tnear + i * stepSize);

        // pos is the global position but we need the local when sampling the texture
        vec3 localPos = (newPos - aabb.Min) / 8;
        vec3 lightColor = LightColor;

        float density = texture(Density, localPos).x;

        if (localPos.z < 0.1)
        {
            // Draw a dark colored floor so we can see the shadow
            density = 10;
            lightColor = 3 * Ambient;
        }
        else if (density <= 0.01)
        {
            continue;
        }

        T *= 1.0 - density * LightSamples * Absorption;
        if (T <= 0.01) break;

        vec3 Li = lightColor * texture(LightCache, localPos).xxx;
        Lo += Li * T * density * LightSamples;
    }

    FragColor.rgb = Lo;
    FragColor.a = 1 - T;

    // FragColor = vec4((pos - aabb.Min) / 8, 1);
}