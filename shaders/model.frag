#version 400

in vec3 vNormal;
in vec2 vTexCoord;

out vec4 Color;

uniform sampler2D Texture;

void main()
{
    Color = texture(Texture, vTexCoord);
    // Color = vec4(vTexCoord.x, vTexCoord.y, 0, 0);
}