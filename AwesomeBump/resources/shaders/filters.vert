#version 330 core

layout(location = 0) in vec3 positionIn;

uniform int  quad_draw_mode;
uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewMatrix;

out vec2 v2QuadCoords;

void main()
{
    vec2 pos = sign(positionIn.xy);
    v2QuadCoords = (pos + 1) * 0.5 ;

    if(quad_draw_mode == 0)
    {
        gl_Position = vec4(pos, 0, 1);
    }
    else if(quad_draw_mode == 1)
    {
        gl_Position = ProjectionMatrix * ModelViewMatrix * vec4(positionIn, 1);
    }
}
