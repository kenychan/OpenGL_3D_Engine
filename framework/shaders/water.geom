#version 330 core

layout(points) in;
layout(line_strip, max_vertices = 300) out;

in vec3 vColor[];
out vec3 fColor;
in float vSides[];
//in float vRadius[];

const float PI = 3.1415926;

uniform float radius;

void main()
{
    fColor = vColor[0];

   for (int i = 0; i <= vSides[0]; i++) {
        // Angle between each side in radians
        float ang = PI * 2.0/ vSides[0] * i;

        // Offset from center of point (0.3 to accomodate for aspect ratio)
        vec4 offset = vec4(cos(ang) * radius, sin(ang) * radius, 0.0, 0.0);
        gl_Position = gl_in[0].gl_Position + offset;

        EmitVertex();
    }

    EndPrimitive();
}