#version 400

layout (location = 0) in vec3 Position;
layout (location = 1) in vec2 TexCoord;
// layout (location = 2) in vec3 Normal;

uniform mat4 ModelviewProjection;

out vec2 vTexCoord;
out vec3 vNormal;

void main()
{
    gl_Position = ModelviewProjection * vec4(Position - vec3(0, 0, 0), 1);
    vTexCoord = TexCoord;
    vNormal = vec3(0);
}