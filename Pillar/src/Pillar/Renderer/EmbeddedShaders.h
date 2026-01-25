#pragma once

// ============================================================================
// EmbeddedShaders.h - Pre-compiled shader source code embedded in the library
// ============================================================================
// This file contains all engine shaders as constexpr strings to eliminate
// the dependency on external shader files for SDK distribution.
// 
// Benefits:
// - No external shader files needed at runtime
// - Faster startup (no file I/O)
// - Simpler SDK packaging
// 
// The ShaderLibrary still supports file-based loading for hot-reload during
// development. Set PIL_USE_EMBEDDED_SHADERS=0 to force file-based loading.
// ============================================================================

namespace Pillar::EmbeddedShaders {

    // ========================================================================
    // BatchQuad Shader - Used by BatchRenderer2D for batched quad rendering
    // ========================================================================

    constexpr const char* BatchQuadVertex = R"(
#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;

uniform mat4 u_ViewProjection;

out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;

void main()
{
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    v_TexIndex = a_TexIndex;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}
)";

    constexpr const char* BatchQuadFragment = R"(
#version 410 core

layout(location = 0) out vec4 color;

in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;

uniform sampler2D u_Textures[32];

void main()
{
    int texIndex = int(v_TexIndex);
    color = texture(u_Textures[texIndex], v_TexCoord) * v_Color;
}
)";

    // ========================================================================
    // Post-Processing: Fullscreen Quad Vertex Shader (shared by all effects)
    // ========================================================================

    constexpr const char* PostProcessVertex = R"(
#version 410 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord = a_TexCoord;
    gl_Position = vec4(a_Position, 0.0, 1.0);
}
)";

    // ========================================================================
    // Post-Processing: Grayscale Effect
    // ========================================================================

    constexpr const char* GrayscaleFragment = R"(
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
)";

    // ========================================================================
    // Post-Processing: Vignette Effect
    // ========================================================================

    constexpr const char* VignetteFragment = R"(
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
)";

    // ========================================================================
    // Post-Processing: Chromatic Aberration Effect
    // ========================================================================

    constexpr const char* ChromaticAberrationFragment = R"(
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
)";

} // namespace Pillar::EmbeddedShaders
