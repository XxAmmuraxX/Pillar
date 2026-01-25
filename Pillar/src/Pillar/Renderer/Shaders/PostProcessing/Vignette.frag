#version 410 core

layout(location = 0) out vec4 o_Color;

in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform float u_Intensity;
uniform float u_Radius;
uniform float u_Softness;

void main()
{
    vec4 color = texture(u_Texture, v_TexCoord);
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(v_TexCoord, center);
    float vignette = smoothstep(u_Radius, u_Radius - u_Softness, dist);
    o_Color = vec4(mix(color.rgb, color.rgb * vignette, u_Intensity), color.a);
}
