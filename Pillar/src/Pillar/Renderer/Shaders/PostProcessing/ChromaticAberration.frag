#version 410 core

layout(location = 0) out vec4 o_Color;

in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform float u_Intensity;
uniform float u_Offset;

void main()
{
    vec2 dir = v_TexCoord - vec2(0.5);
    vec2 offset = dir * u_Offset * u_Intensity;
    
    float r = texture(u_Texture, v_TexCoord + offset).r;
    float g = texture(u_Texture, v_TexCoord).g;
    float b = texture(u_Texture, v_TexCoord - offset).b;
    float a = texture(u_Texture, v_TexCoord).a;
    
    o_Color = vec4(r, g, b, a);
}
