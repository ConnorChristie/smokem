#version 400

in vec4 a_vertex;
in vec2 a_tex_coord;

out vec2 TexCoord;
smooth out vec4 VertexPosition;

uniform mat4 ModelviewProjection;

void main()
{
    gl_Position = vec4(a_vertex.xyz, 1);
    VertexPosition = a_vertex;
    TexCoord = a_tex_coord;
}
