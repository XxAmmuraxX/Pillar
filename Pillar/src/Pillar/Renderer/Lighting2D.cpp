#include "Pillar/Renderer/Lighting2D.h"

#include "Pillar/Logger.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/Renderer/Shader.h"
#include "Pillar/Renderer/Lighting2DGeometry.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <limits>

namespace Pillar
{
	namespace
	{
		struct GLStateSnapshot
		{
			GLint Framebuffer = 0;
			std::array<GLint, 4> Viewport{ 0, 0, 0, 0 };
			std::array<GLint, 4> ScissorBox{ 0, 0, 0, 0 };
			GLboolean BlendEnabled = GL_FALSE;
			GLboolean DepthTestEnabled = GL_FALSE;
			GLboolean ScissorEnabled = GL_FALSE;
			GLboolean StencilEnabled = GL_FALSE;

			GLint BlendSrcRGB = 0;
			GLint BlendDstRGB = 0;
			GLint BlendSrcAlpha = 0;
			GLint BlendDstAlpha = 0;
			GLint BlendEqRGB = 0;
			GLint BlendEqAlpha = 0;
			std::array<GLboolean, 4> ColorMask{ GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };

			GLboolean DepthWriteMask = GL_TRUE;

			GLint StencilFunc = 0;
			GLint StencilRef = 0;
			GLint StencilValueMask = 0;
			GLint StencilWriteMask = 0;
			GLint StencilFail = 0;
			GLint StencilPassDepthFail = 0;
			GLint StencilPassDepthPass = 0;

			GLint CurrentProgram = 0;
			GLint VertexArrayBinding = 0;
			GLint ArrayBufferBinding = 0;
			GLint ElementArrayBufferBinding = 0;

			GLint ActiveTexture = 0;
			GLint Texture2DBinding0 = 0;
			GLint Texture2DBinding1 = 0;
		};

		static GLStateSnapshot CaptureGLState()
		{
			GLStateSnapshot s;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &s.Framebuffer);
			glGetIntegerv(GL_VIEWPORT, s.Viewport.data());
			glGetIntegerv(GL_SCISSOR_BOX, s.ScissorBox.data());
			s.BlendEnabled = glIsEnabled(GL_BLEND);
			s.DepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
			s.ScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
			s.StencilEnabled = glIsEnabled(GL_STENCIL_TEST);

			glGetIntegerv(GL_BLEND_SRC_RGB, &s.BlendSrcRGB);
			glGetIntegerv(GL_BLEND_DST_RGB, &s.BlendDstRGB);
			glGetIntegerv(GL_BLEND_SRC_ALPHA, &s.BlendSrcAlpha);
			glGetIntegerv(GL_BLEND_DST_ALPHA, &s.BlendDstAlpha);
			glGetIntegerv(GL_BLEND_EQUATION_RGB, &s.BlendEqRGB);
			glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &s.BlendEqAlpha);
			glGetBooleanv(GL_COLOR_WRITEMASK, s.ColorMask.data());

			glGetBooleanv(GL_DEPTH_WRITEMASK, &s.DepthWriteMask);

			glGetIntegerv(GL_STENCIL_FUNC, &s.StencilFunc);
			glGetIntegerv(GL_STENCIL_REF, &s.StencilRef);
			glGetIntegerv(GL_STENCIL_VALUE_MASK, &s.StencilValueMask);
			glGetIntegerv(GL_STENCIL_WRITEMASK, &s.StencilWriteMask);
			glGetIntegerv(GL_STENCIL_FAIL, &s.StencilFail);
			glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &s.StencilPassDepthFail);
			glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &s.StencilPassDepthPass);

			glGetIntegerv(GL_CURRENT_PROGRAM, &s.CurrentProgram);
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &s.VertexArrayBinding);
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &s.ArrayBufferBinding);
			glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &s.ElementArrayBufferBinding);

			glGetIntegerv(GL_ACTIVE_TEXTURE, &s.ActiveTexture);
			// Capture bindings for the texture units we touch.
			glActiveTexture(GL_TEXTURE0);
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &s.Texture2DBinding0);
			glActiveTexture(GL_TEXTURE1);
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &s.Texture2DBinding1);
			glActiveTexture((GLenum)s.ActiveTexture);
			return s;
		}

		static void RestoreGLState(const GLStateSnapshot& s)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)s.Framebuffer);
			glViewport(s.Viewport[0], s.Viewport[1], s.Viewport[2], s.Viewport[3]);
			glScissor(s.ScissorBox[0], s.ScissorBox[1], s.ScissorBox[2], s.ScissorBox[3]);
			if (s.BlendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
			if (s.DepthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
			if (s.ScissorEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
			if (s.StencilEnabled) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);

			glBlendFuncSeparate((GLenum)s.BlendSrcRGB, (GLenum)s.BlendDstRGB, (GLenum)s.BlendSrcAlpha, (GLenum)s.BlendDstAlpha);
			glBlendEquationSeparate((GLenum)s.BlendEqRGB, (GLenum)s.BlendEqAlpha);
			glColorMask(s.ColorMask[0], s.ColorMask[1], s.ColorMask[2], s.ColorMask[3]);
			glDepthMask(s.DepthWriteMask);

			glStencilFunc((GLenum)s.StencilFunc, s.StencilRef, (GLuint)s.StencilValueMask);
			glStencilMask((GLuint)s.StencilWriteMask);
			glStencilOp((GLenum)s.StencilFail, (GLenum)s.StencilPassDepthFail, (GLenum)s.StencilPassDepthPass);

			glUseProgram((GLuint)s.CurrentProgram);
			glBindVertexArray((GLuint)s.VertexArrayBinding);
			glBindBuffer(GL_ARRAY_BUFFER, (GLuint)s.ArrayBufferBinding);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)s.ElementArrayBufferBinding);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, (GLuint)s.Texture2DBinding0);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, (GLuint)s.Texture2DBinding1);
			glActiveTexture((GLenum)s.ActiveTexture);
		}

		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		struct Lighting2DData
		{
			bool Initialized = false;

			Lighting2DSettings Settings;
			glm::mat4 ViewProjection{ 1.0f };

			uint32_t ViewportWidth = 0;
			uint32_t ViewportHeight = 0;

			std::shared_ptr<Framebuffer> OutputFramebuffer;
			std::shared_ptr<Framebuffer> SceneColorFramebuffer;
			std::shared_ptr<Framebuffer> LightAccumFramebuffer;

			std::vector<Light2DSubmit> Lights;
			std::vector<ShadowCaster2DSubmit> Casters;
			std::vector<glm::vec2> ShadowTrianglesScratch;

			struct LightUniformLocations
			{
				GLint Program = 0;
				GLint u_ViewProjection = -1;
				GLint u_Model = -1;
				GLint u_LightType = -1;
				GLint u_LightPos = -1;
				GLint u_Direction = -1;
				GLint u_InnerCos = -1;
				GLint u_OuterCos = -1;
				GLint u_Color = -1;
				GLint u_Intensity = -1;
				GLint u_Radius = -1;
			};
			LightUniformLocations LightUniforms;

			struct ShadowUniformLocations
			{
				GLint Program = 0;
				GLint u_ViewProjection = -1;
			};
			ShadowUniformLocations ShadowUniforms;

			Shader* LightShader = nullptr;
			Shader* ShadowShader = nullptr;
			Shader* CompositeShader = nullptr;

			GLuint QuadVAO = 0;
			GLuint QuadVBO = 0;
			GLuint QuadIBO = 0;

			GLuint FullscreenVAO = 0;
			GLuint FullscreenVBO = 0;
			GLuint FullscreenIBO = 0;

			GLuint ShadowVAO = 0;
			GLuint ShadowVBO = 0;

			GLStateSnapshot StateBefore{};
			bool InScene = false;
		};

		static Lighting2DData s_Data;

		static void EnsureFramebuffers(uint32_t width, uint32_t height)
		{
			if (!s_Data.SceneColorFramebuffer)
			{
				FramebufferSpecification spec;
				spec.Width = width;
				spec.Height = height;
				s_Data.SceneColorFramebuffer = Framebuffer::Create(spec);
			}
			else if (s_Data.SceneColorFramebuffer->GetWidth() != width || s_Data.SceneColorFramebuffer->GetHeight() != height)
			{
				s_Data.SceneColorFramebuffer->Resize(width, height);
			}

			if (!s_Data.LightAccumFramebuffer)
			{
				FramebufferSpecification spec;
				spec.Width = width;
				spec.Height = height;
				s_Data.LightAccumFramebuffer = Framebuffer::Create(spec);
			}
			else if (s_Data.LightAccumFramebuffer->GetWidth() != width || s_Data.LightAccumFramebuffer->GetHeight() != height)
			{
				s_Data.LightAccumFramebuffer->Resize(width, height);
			}
		}

		static void EnsureGLResources()
		{
			if (s_Data.QuadVAO != 0)
				return;

			// World-space quad (model matrix places it)
			QuadVertex quadVerts[4] = {
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f } },
				{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f } },
				{ {  0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f } },
				{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f } },
			};
			uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

			glGenVertexArrays(1, &s_Data.QuadVAO);
			glBindVertexArray(s_Data.QuadVAO);

			glGenBuffers(1, &s_Data.QuadVBO);
			glBindBuffer(GL_ARRAY_BUFFER, s_Data.QuadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

			glGenBuffers(1, &s_Data.QuadIBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Data.QuadIBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, Position));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, TexCoord));

			glBindVertexArray(0);

			// Fullscreen quad (NDC)
			QuadVertex fsVerts[4] = {
				{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
				{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
				{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f } },
				{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f } },
			};

			glGenVertexArrays(1, &s_Data.FullscreenVAO);
			glBindVertexArray(s_Data.FullscreenVAO);

			glGenBuffers(1, &s_Data.FullscreenVBO);
			glBindBuffer(GL_ARRAY_BUFFER, s_Data.FullscreenVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(fsVerts), fsVerts, GL_STATIC_DRAW);

			glGenBuffers(1, &s_Data.FullscreenIBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Data.FullscreenIBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, Position));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, TexCoord));

			glBindVertexArray(0);

			// Shadow triangles (vec2 positions)
			glGenVertexArrays(1, &s_Data.ShadowVAO);
			glBindVertexArray(s_Data.ShadowVAO);

			glGenBuffers(1, &s_Data.ShadowVBO);
			glBindBuffer(GL_ARRAY_BUFFER, s_Data.ShadowVBO);
			glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

			glBindVertexArray(0);
		}

		static void EnsureShaders()
		{
			if (s_Data.LightShader && s_Data.ShadowShader && s_Data.CompositeShader)
				return;

			const std::string lightVert = R"(
#version 410 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;

out vec2 v_WorldPos;

void main()
{
    vec4 world = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = world.xy;
    gl_Position = u_ViewProjection * world;
}
)";

			const std::string lightFrag = R"(
#version 410 core

in vec2 v_WorldPos;

uniform int u_LightType; // 0 = point, 1 = spot
uniform vec2 u_LightPos;
uniform vec2 u_Direction;
uniform float u_InnerCos;
uniform float u_OuterCos;
uniform vec3 u_Color;
uniform float u_Intensity;
uniform float u_Radius;

out vec4 o_Color;

void main()
{
	vec2 toFrag = v_WorldPos - u_LightPos;
	float d = length(toFrag);
	float t = clamp(1.0 - (d / u_Radius), 0.0, 1.0);
    // smoother falloff
    float a = t * t * (3.0 - 2.0 * t);

	float cone = 1.0;
	if (u_LightType == 1)
	{
		vec2 dir = normalize(u_Direction);
		vec2 toN = (d > 1e-6) ? (toFrag / d) : vec2(0.0);
		float cd = dot(dir, toN);
		// smoothstep(outer..inner) in cosine space
		cone = clamp((cd - u_OuterCos) / max(u_InnerCos - u_OuterCos, 1e-6), 0.0, 1.0);
		cone = cone * cone * (3.0 - 2.0 * cone);
	}

    vec3 rgb = u_Color * (u_Intensity * a);
	o_Color = vec4(rgb * cone, 1.0);
}
)";

			const std::string shadowVert = R"(
#version 410 core
layout(location = 0) in vec2 a_Position;

uniform mat4 u_ViewProjection;

void main()
{
    gl_Position = u_ViewProjection * vec4(a_Position.xy, 0.0, 1.0);
}
)";

			const std::string shadowFrag = R"(
#version 410 core
out vec4 o_Color;
void main()
{
    o_Color = vec4(0.0, 0.0, 0.0, 1.0);
}
)";

			const std::string compositeVert = R"(
#version 410 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord = a_TexCoord;
    gl_Position = vec4(a_Position, 1.0);
}
)";

			const std::string compositeFrag = R"(
#version 410 core

in vec2 v_TexCoord;

uniform sampler2D u_SceneColor;
uniform sampler2D u_LightAccum;

out vec4 o_Color;

void main()
{
    vec4 scene = texture(u_SceneColor, v_TexCoord);
    vec3 light = texture(u_LightAccum, v_TexCoord).rgb;
    o_Color = vec4(scene.rgb * light, scene.a);
}
)";

			s_Data.LightShader = Shader::Create(lightVert, lightFrag);
			s_Data.ShadowShader = Shader::Create(shadowVert, shadowFrag);
			s_Data.CompositeShader = Shader::Create(compositeVert, compositeFrag);

			PIL_CORE_ASSERT(s_Data.LightShader && s_Data.ShadowShader && s_Data.CompositeShader, "Lighting2D shaders must compile");
			// Invalidate cached uniform locations in case programs changed.
			s_Data.LightUniforms = {};
			s_Data.ShadowUniforms = {};
		}

		static void EnsureLightUniformLocationsBound()
		{
			GLint program = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &program);
			if (program == 0)
				return;

			if (s_Data.LightUniforms.Program == program)
				return;

			s_Data.LightUniforms = {};
			s_Data.LightUniforms.Program = program;
			s_Data.LightUniforms.u_ViewProjection = glGetUniformLocation(program, "u_ViewProjection");
			s_Data.LightUniforms.u_Model = glGetUniformLocation(program, "u_Model");
			s_Data.LightUniforms.u_LightType = glGetUniformLocation(program, "u_LightType");
			s_Data.LightUniforms.u_LightPos = glGetUniformLocation(program, "u_LightPos");
			s_Data.LightUniforms.u_Direction = glGetUniformLocation(program, "u_Direction");
			s_Data.LightUniforms.u_InnerCos = glGetUniformLocation(program, "u_InnerCos");
			s_Data.LightUniforms.u_OuterCos = glGetUniformLocation(program, "u_OuterCos");
			s_Data.LightUniforms.u_Color = glGetUniformLocation(program, "u_Color");
			s_Data.LightUniforms.u_Intensity = glGetUniformLocation(program, "u_Intensity");
			s_Data.LightUniforms.u_Radius = glGetUniformLocation(program, "u_Radius");
		}

		static void EnsureShadowUniformLocationsBound()
		{
			GLint program = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &program);
			if (program == 0)
				return;

			if (s_Data.ShadowUniforms.Program == program)
				return;

			s_Data.ShadowUniforms = {};
			s_Data.ShadowUniforms.Program = program;
			s_Data.ShadowUniforms.u_ViewProjection = glGetUniformLocation(program, "u_ViewProjection");
		}

		static void RenderLightAccumulation()
		{
			PIL_CORE_ASSERT(s_Data.SceneColorFramebuffer && s_Data.LightAccumFramebuffer, "Lighting2D requires internal framebuffers");

			s_Data.LightAccumFramebuffer->Bind();
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);

			// Clear to ambient
			glm::vec3 ambient = s_Data.Settings.AmbientColor * s_Data.Settings.AmbientIntensity;
			glClearColor(ambient.r, ambient.g, ambient.b, 1.0f);
			glClearStencil(0);
			glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);

			EnsureShaders();
			EnsureGLResources();

			s_Data.ShadowTrianglesScratch.reserve(2048);

			// Bind and set per-pass uniforms once.
			s_Data.LightShader->Bind();
			EnsureLightUniformLocationsBound();
			if (s_Data.LightUniforms.u_ViewProjection >= 0)
				glUniformMatrix4fv(s_Data.LightUniforms.u_ViewProjection, 1, GL_FALSE, &s_Data.ViewProjection[0][0]);

			for (const auto& light : s_Data.Lights)
			{
				if (light.Radius <= 0.0f || light.Intensity <= 0.0f)
					continue;

				// Scissor to light bounds for performance and for scissored stencil clears.
				Lighting2D::ScissorRect scissor = Lighting2D::ComputeScissorRect(s_Data.ViewProjection, light.Position, light.Radius, s_Data.ViewportWidth, s_Data.ViewportHeight);
				if (!scissor.Valid)
					continue;

				glEnable(GL_SCISSOR_TEST);
				glScissor(scissor.X, scissor.Y, scissor.Width, scissor.Height);

				// Clear stencil within scissor
				glEnable(GL_STENCIL_TEST);
				glStencilMask(0xFF);
				glClearStencil(0);
				glClear(GL_STENCIL_BUFFER_BIT);

				if (s_Data.Settings.EnableShadows && light.CastShadows)
				{
					// Build shadow triangles
					auto& shadowTriangles = s_Data.ShadowTrianglesScratch;
					shadowTriangles.clear();

					Lighting2DGeometry::Light2D gLight;
					gLight.Position = light.Position;
					gLight.Radius = light.Radius;
					gLight.LayerMask = light.LayerMask;

					for (const auto& caster : s_Data.Casters)
					{
						Lighting2DGeometry::ShadowCaster2D gCaster;
						gCaster.WorldPoints = caster.WorldPoints;
						gCaster.Closed = caster.Closed;
						gCaster.TwoSided = caster.TwoSided;
						gCaster.LayerMask = caster.LayerMask;

						if (!Lighting2DGeometry::IsCasterInRange(gLight, gCaster))
							continue;

						Lighting2DGeometry::BuildShadowVolumeTriangles(gLight, gCaster, shadowTriangles);
					}

					if (!shadowTriangles.empty())
					{
						// Write to stencil where shadow volumes are drawn
						glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
						glStencilFunc(GL_ALWAYS, 1, 0xFF);
						glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

						s_Data.ShadowShader->Bind();
						EnsureShadowUniformLocationsBound();
						if (s_Data.ShadowUniforms.u_ViewProjection >= 0)
							glUniformMatrix4fv(s_Data.ShadowUniforms.u_ViewProjection, 1, GL_FALSE, &s_Data.ViewProjection[0][0]);

						glBindVertexArray(s_Data.ShadowVAO);
						glBindBuffer(GL_ARRAY_BUFFER, s_Data.ShadowVBO);
						glBufferData(GL_ARRAY_BUFFER, shadowTriangles.size() * sizeof(glm::vec2), shadowTriangles.data(), GL_STREAM_DRAW);

						glDrawArrays(GL_TRIANGLES, 0, (GLsizei)shadowTriangles.size());
						glBindVertexArray(0);
						glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
					}
				}

				// Render the light with stencil test (exclude shadowed pixels)
				bool useStencil = (s_Data.Settings.EnableShadows && light.CastShadows);
				if (useStencil)
				{
					glStencilMask(0x00);
					glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
					glStencilFunc(GL_EQUAL, 0, 0xFF);
				}
				else
				{
					glDisable(GL_STENCIL_TEST);
				}

				s_Data.LightShader->Bind();
			EnsureLightUniformLocationsBound();
			if (s_Data.LightUniforms.u_ViewProjection >= 0)
				glUniformMatrix4fv(s_Data.LightUniforms.u_ViewProjection, 1, GL_FALSE, &s_Data.ViewProjection[0][0]);

				// Bind quad
				glBindVertexArray(s_Data.QuadVAO);

				if (s_Data.LightUniforms.Program != 0)
				{
					glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(light.Position, 0.0f))
						* glm::scale(glm::mat4(1.0f), glm::vec3(light.Radius * 2.0f, light.Radius * 2.0f, 1.0f));

					int typeInt = (light.Type == Light2DType::Spot) ? 1 : 0;
					glm::vec2 dir = light.Direction;
					float dirLenSq = dir.x * dir.x + dir.y * dir.y;
					if (dirLenSq > 1e-6f) dir = dir / std::sqrt(dirLenSq);
					float innerCos = std::cos(light.InnerAngleRadians);
					float outerCos = std::cos(light.OuterAngleRadians);
					if (outerCos > innerCos) std::swap(outerCos, innerCos);

					if (s_Data.LightUniforms.u_LightType >= 0) glUniform1i(s_Data.LightUniforms.u_LightType, typeInt);
					if (s_Data.LightUniforms.u_LightPos >= 0) glUniform2f(s_Data.LightUniforms.u_LightPos, light.Position.x, light.Position.y);
					if (s_Data.LightUniforms.u_Direction >= 0) glUniform2f(s_Data.LightUniforms.u_Direction, dir.x, dir.y);
					if (s_Data.LightUniforms.u_InnerCos >= 0) glUniform1f(s_Data.LightUniforms.u_InnerCos, innerCos);
					if (s_Data.LightUniforms.u_OuterCos >= 0) glUniform1f(s_Data.LightUniforms.u_OuterCos, outerCos);
					if (s_Data.LightUniforms.u_Color >= 0) glUniform3f(s_Data.LightUniforms.u_Color, light.Color.r, light.Color.g, light.Color.b);
					if (s_Data.LightUniforms.u_Intensity >= 0) glUniform1f(s_Data.LightUniforms.u_Intensity, light.Intensity);
					if (s_Data.LightUniforms.u_Radius >= 0) glUniform1f(s_Data.LightUniforms.u_Radius, light.Radius);
					if (s_Data.LightUniforms.u_Model >= 0) glUniformMatrix4fv(s_Data.LightUniforms.u_Model, 1, GL_FALSE, &model[0][0]);
				}

				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

				// ShadowStrength support: if ShadowStrength < 1, render a reduced-intensity pass inside the stencil.
				if (useStencil)
				{
					float strength = std::clamp(light.ShadowStrength, 0.0f, 1.0f);
					float shadowIntensity = light.Intensity * (1.0f - strength);
					if (shadowIntensity > 0.0f && strength < 1.0f)
					{
						glStencilFunc(GL_EQUAL, 1, 0xFF);
						if (s_Data.LightUniforms.u_Intensity >= 0) glUniform1f(s_Data.LightUniforms.u_Intensity, shadowIntensity);
						glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
					}
				}

				glBindVertexArray(0);

				glDisable(GL_SCISSOR_TEST);
				glDisable(GL_STENCIL_TEST);
			}

			glDisable(GL_BLEND);
		}

		static void CompositeToOutput()
		{
			uint32_t w = s_Data.ViewportWidth;
			uint32_t h = s_Data.ViewportHeight;

			if (s_Data.OutputFramebuffer)
			{
				s_Data.OutputFramebuffer->Bind();
				w = s_Data.OutputFramebuffer->GetWidth();
				h = s_Data.OutputFramebuffer->GetHeight();
			}
			else
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, (GLsizei)w, (GLsizei)h);
			}

			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glDisable(GL_BLEND);

			EnsureShaders();
			EnsureGLResources();

			s_Data.CompositeShader->Bind();

			// Bind textures
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, s_Data.SceneColorFramebuffer->GetColorAttachmentRendererID());
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, s_Data.LightAccumFramebuffer->GetColorAttachmentRendererID());

			// Set sampler uniforms via OpenGL (Shader interface lacks SetInt for both?) it has SetInt.
			s_Data.CompositeShader->SetInt("u_SceneColor", 0);
			s_Data.CompositeShader->SetInt("u_LightAccum", 1);

			glBindVertexArray(s_Data.FullscreenVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 0);

			if (s_Data.OutputFramebuffer)
				s_Data.OutputFramebuffer->Unbind();
		}
	}

	void Lighting2D::Init()
	{
		if (s_Data.Initialized)
			return;

		PIL_CORE_INFO("Initializing Lighting2D...");
		EnsureShaders();
		EnsureGLResources();
		s_Data.Initialized = true;
	}

	void Lighting2D::Shutdown()
	{
		if (!s_Data.Initialized)
			return;

		PIL_CORE_INFO("Shutting down Lighting2D...");

		delete s_Data.LightShader;
		delete s_Data.ShadowShader;
		delete s_Data.CompositeShader;
		s_Data.LightShader = nullptr;
		s_Data.ShadowShader = nullptr;
		s_Data.CompositeShader = nullptr;

		if (s_Data.QuadVAO) glDeleteVertexArrays(1, &s_Data.QuadVAO);
		if (s_Data.QuadVBO) glDeleteBuffers(1, &s_Data.QuadVBO);
		if (s_Data.QuadIBO) glDeleteBuffers(1, &s_Data.QuadIBO);
		if (s_Data.FullscreenVAO) glDeleteVertexArrays(1, &s_Data.FullscreenVAO);
		if (s_Data.FullscreenVBO) glDeleteBuffers(1, &s_Data.FullscreenVBO);
		if (s_Data.FullscreenIBO) glDeleteBuffers(1, &s_Data.FullscreenIBO);
		if (s_Data.ShadowVAO) glDeleteVertexArrays(1, &s_Data.ShadowVAO);
		if (s_Data.ShadowVBO) glDeleteBuffers(1, &s_Data.ShadowVBO);

		s_Data = Lighting2DData{};
	}

	void Lighting2D::BeginScene(const OrthographicCamera& camera,
		uint32_t viewportWidth,
		uint32_t viewportHeight,
		const Lighting2DSettings& settings)
	{
		PIL_CORE_ASSERT(s_Data.Initialized, "Lighting2D::Init must be called before BeginScene");
		PIL_CORE_ASSERT(!s_Data.InScene, "Lighting2D::BeginScene called while already in scene");

		s_Data.StateBefore = CaptureGLState();
		s_Data.Settings = settings;
		s_Data.OutputFramebuffer.reset();
		s_Data.ViewProjection = camera.GetViewProjectionMatrix();
		s_Data.ViewportWidth = viewportWidth;
		s_Data.ViewportHeight = viewportHeight;

		PIL_CORE_ASSERT(s_Data.ViewportWidth > 0 && s_Data.ViewportHeight > 0, "Lighting2D requires a valid viewport size");
		EnsureFramebuffers(s_Data.ViewportWidth, s_Data.ViewportHeight);

		s_Data.Lights.clear();
		s_Data.Casters.clear();

		// SceneColor pass
		s_Data.SceneColorFramebuffer->Bind();
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		Renderer2DBackend::BeginScene(camera);
		s_Data.InScene = true;
	}

	void Lighting2D::BeginScene(const OrthographicCamera& camera,
		const std::shared_ptr<Framebuffer>& outputFramebuffer,
		const Lighting2DSettings& settings)
	{
		PIL_CORE_ASSERT(s_Data.Initialized, "Lighting2D::Init must be called before BeginScene");
		PIL_CORE_ASSERT(!s_Data.InScene, "Lighting2D::BeginScene called while already in scene");

		s_Data.StateBefore = CaptureGLState();

		s_Data.Settings = settings;
		s_Data.OutputFramebuffer = outputFramebuffer;
		s_Data.ViewProjection = camera.GetViewProjectionMatrix();

		uint32_t w = 0;
		uint32_t h = 0;
		if (outputFramebuffer)
		{
			w = outputFramebuffer->GetWidth();
			h = outputFramebuffer->GetHeight();
		}
		else
		{
			// Keep current viewport size if user will set it via the overload with explicit size.
			w = s_Data.ViewportWidth;
			h = s_Data.ViewportHeight;
		}

		PIL_CORE_ASSERT(w > 0 && h > 0, "Lighting2D requires a valid viewport size");
		s_Data.ViewportWidth = w;
		s_Data.ViewportHeight = h;

		EnsureFramebuffers(w, h);

		s_Data.Lights.clear();
		s_Data.Casters.clear();

		// SceneColor pass
		s_Data.SceneColorFramebuffer->Bind();
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		Renderer2DBackend::BeginScene(camera);
		s_Data.InScene = true;
	}

	void Lighting2D::SubmitLight(const Light2DSubmit& light)
	{
		PIL_CORE_ASSERT(s_Data.InScene, "Lighting2D::SubmitLight called outside BeginScene/EndScene");
		if (!s_Data.InScene)
			return;
		s_Data.Lights.push_back(light);
	}

	void Lighting2D::SubmitShadowCaster(const ShadowCaster2DSubmit& caster)
	{
		PIL_CORE_ASSERT(s_Data.InScene, "Lighting2D::SubmitShadowCaster called outside BeginScene/EndScene");
		if (!s_Data.InScene)
			return;
		if (caster.WorldPoints.size() < 2)
			return;
		s_Data.Casters.push_back(caster);
	}

	void Lighting2D::EndScene()
	{
		PIL_CORE_ASSERT(s_Data.InScene, "Lighting2D::EndScene called without BeginScene");

		Renderer2DBackend::EndScene();
		s_Data.SceneColorFramebuffer->Unbind();

		RenderLightAccumulation();
		CompositeToOutput();

		s_Data.InScene = false;
		s_Data.OutputFramebuffer.reset();

		RestoreGLState(s_Data.StateBefore);
	}

	Lighting2D::ScissorRect Lighting2D::ComputeScissorRect(const glm::mat4& viewProjection,
		const glm::vec2& lightPosition,
		float radius,
		uint32_t viewportWidth,
		uint32_t viewportHeight)
	{
		ScissorRect r;
		if (radius <= 0.0f || viewportWidth == 0 || viewportHeight == 0)
			return r;

		// Conservative screen-space bounds by projecting the world AABB of the circle.
		glm::vec2 minW = lightPosition - glm::vec2(radius);
		glm::vec2 maxW = lightPosition + glm::vec2(radius);

		auto project = [&](const glm::vec2& p) -> glm::vec2 {
			glm::vec4 clip = viewProjection * glm::vec4(p.x, p.y, 0.0f, 1.0f);
			if (std::abs(clip.w) <= std::numeric_limits<float>::epsilon())
				return glm::vec2(0.0f);
			glm::vec3 ndc = glm::vec3(clip) / clip.w;
			// Convert NDC [-1..1] to pixel coords (origin bottom-left for glScissor)
			float px = (ndc.x * 0.5f + 0.5f) * (float)viewportWidth;
			float py = (ndc.y * 0.5f + 0.5f) * (float)viewportHeight;
			return { px, py };
		};

		glm::vec2 p0 = project({ minW.x, minW.y });
		glm::vec2 p1 = project({ maxW.x, minW.y });
		glm::vec2 p2 = project({ maxW.x, maxW.y });
		glm::vec2 p3 = project({ minW.x, maxW.y });

		float minX = std::min({ p0.x, p1.x, p2.x, p3.x });
		float maxX = std::max({ p0.x, p1.x, p2.x, p3.x });
		float minY = std::min({ p0.y, p1.y, p2.y, p3.y });
		float maxY = std::max({ p0.y, p1.y, p2.y, p3.y });

		// Clamp
		int x0 = (int)std::floor(minX);
		int y0 = (int)std::floor(minY);
		int x1 = (int)std::ceil(maxX);
		int y1 = (int)std::ceil(maxY);

		x0 = std::clamp(x0, 0, (int)viewportWidth);
		y0 = std::clamp(y0, 0, (int)viewportHeight);
		x1 = std::clamp(x1, 0, (int)viewportWidth);
		y1 = std::clamp(y1, 0, (int)viewportHeight);

		int w = x1 - x0;
		int h = y1 - y0;
		if (w <= 0 || h <= 0)
			return r;

		r.X = x0;
		r.Y = y0;
		r.Width = w;
		r.Height = h;
		r.Valid = true;
		return r;
	}
}
