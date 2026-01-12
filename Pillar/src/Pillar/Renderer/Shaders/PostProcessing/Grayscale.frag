#version 410 core

layout(location = 0) out vec4 o_Color;

in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform float u_Intensity;

void main()
{
    vec4 color = texture(u_Texture, v_TexCoord);
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    vec3 grayscale = vec3(gray);
    o_Color = vec4(mix(color.rgb, grayscale, u_Intensity), color.a);
}
