#version 400

in vec4 a_vertex;

uniform mat4 ModelviewProjection;

void main()
{
    gl_Position = a_vertex;
}
