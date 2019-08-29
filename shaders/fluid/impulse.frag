#version 400

out vec4 FragColor;

uniform vec3 Point;
uniform float Radius;
uniform vec3 Intensity;

in float gLayer;

void main()
{
    float d = distance(Point.xy, gl_FragCoord.xy);
    float impulse = 0;

    if (d < Radius && gLayer > Radius + 20 && gLayer < Radius + 25)
    {
        float a = (Radius - d) * 0.5;
        impulse = min(a, 1.0);
    }

    FragColor = vec4(Intensity, impulse);
}