#version 330 core

in vec2 pos;
in vec3 color;
in float sides;
//in float radius;

out vec3 vColor;
out float vSides;
//out float vRadius;



void main()
{
    gl_Position = vec4(pos, 0.0, 1.0);
    vColor = color;
    vSides = sides;
//  vRadius = radius;
}