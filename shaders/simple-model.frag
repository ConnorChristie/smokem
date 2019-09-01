#version 400

const int numberOfLights = 1;

const float attnConst = 0.98;
const float attnLinear = 0.025;
const float attnQuad = 0.01;

uniform vec3 lightPositions[numberOfLights]; // positions of each light source
uniform vec3 lightAmbients[numberOfLights]; // ambient values for each light source
uniform vec3 lightDiffuses[numberOfLights]; // diffuse values for each light source
uniform vec3 lightSpeculars[numberOfLights]; // specular values for each light source
uniform float lightBrightnesses[numberOfLights]; // brightnesses of each light.

uniform float mtl_ambient; // ambient material value
uniform float mtl_diffuse; // diffuse material value
uniform float mtl_specular; // specular material value
uniform float mtl_emission; // emission colour for the material
uniform float mtl_shininess; // shininess of the material

in mat4 lightPosMatrix;
in vec4 vertex;
in vec3 normal;
in vec2 texCoord;

out vec4 fragColor;

vec3 phongLight(in vec4 position, in vec3 norm, in vec4 light_pos, in vec3 light_ambient, in vec3 light_diffuse, in vec3 light_specular, float light_brightness)
{
    vec3 s;

    if (light_pos.w == 0.0)
    {
        // s is the direction from the light to the vertex
        s = normalize(light_pos.xyz);
    }
    else
    {
        s = normalize(vec3(light_pos - position));
    }

    // v is the direction from the eye to the vertex
    vec3 v = normalize(-position.xyz);

    // r is the direction of light reflected from the vertex
    vec3 r = reflect(-s, norm);

    // The diffuse component
    float sDotN = max(dot(s,norm), 0.0);

    vec3 ambient = clamp(light_ambient * mtl_ambient, 0.0, 1.0);
    vec3 diffuse = clamp(light_diffuse * mtl_diffuse * sDotN, 0.0, 1.0);

    // Specular component
    vec3 spec = vec3(0.0);
    if (sDotN > 0.0)
    {
        spec = clamp(light_specular * mtl_specular * pow(max(dot(r,v), 0.0), mtl_shininess), 0.0, 1.0);
    }

    // distance between fragment and light
    float dist = distance(position, light_pos);

    // attenuation factor, so that light fades with distance
    float attenuation = light_brightness / (attnConst + attnLinear * dist + attnQuad * dist * dist);

    return ambient + attenuation * (diffuse + spec);
}

void main()
{
    fragColor = vec4(0.8f, 0.1f, 0.0f, 1.0f);

    // Normal Map
    // vec3 NN = texture(tex_norm, texCoord.st).xyz;
    // vec3 N = normal + normalize(2.0 * NN.xyz - 1.0);

    for (int i = 0; i < numberOfLights; i++)
    {
        fragColor.xyz += phongLight(
            vertex,
            normalize(normal),
            lightPosMatrix * vec4(lightPositions[i], 1.0),
            lightAmbients[i],
            lightDiffuses[i],
            lightSpeculars[i],
            lightBrightnesses[i]
        );
    }

    fragColor = vec4(fragColor.xyz + mtl_emission, 1.0f);
}