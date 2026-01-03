#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/OrthographicCamera.h"
#include "Pillar/Renderer/Framebuffer.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace Pillar
{
	enum class Light2DType : uint8_t
	{
		Point = 0,
		Spot = 1,
	};

	struct Light2DSubmit
	{
		Light2DType Type = Light2DType::Point;
		glm::vec2 Position{ 0.0f };
		glm::vec2 Direction{ 1.0f, 0.0f }; // Used for spot
		glm::vec3 Color{ 1.0f };
		float Intensity = 1.0f;
		float Radius = 5.0f;
		float InnerAngleRadians = 0.25f;
		float OuterAngleRadians = 0.5f;
		bool CastShadows = true;
		float ShadowStrength = 1.0f;
		uint32_t LayerMask = 0xFFFFFFFFu;
	};

	struct ShadowCaster2DSubmit
	{
		std::vector<glm::vec2> WorldPoints;
		bool Closed = true;
		bool TwoSided = false;
		uint32_t LayerMask = 0xFFFFFFFFu;
	};

	struct Lighting2DSettings
	{
		glm::vec3 AmbientColor{ 1.0f, 1.0f, 1.0f };
		float AmbientIntensity = 0.15f;
		bool EnableShadows = true;
	};

	class PIL_API Lighting2D
	{
	public:
		struct ScissorRect
		{
			int X = 0;
			int Y = 0;
			int Width = 0;
			int Height = 0;
			bool Valid = false;
		};

		static void Init();
		static void Shutdown();

		// Begins a lit 2D frame. This binds an internal scene-color framebuffer and
		// calls Renderer2DBackend::BeginScene(camera) for you.
		static void BeginScene(const OrthographicCamera& camera,
			uint32_t viewportWidth,
			uint32_t viewportHeight,
			const Lighting2DSettings& settings = {});

		// Same as above, but composites into the provided output framebuffer.
		static void BeginScene(const OrthographicCamera& camera,
			const std::shared_ptr<Framebuffer>& outputFramebuffer,
			const Lighting2DSettings& settings = {});

		static void SubmitLight(const Light2DSubmit& light);
		static void SubmitShadowCaster(const ShadowCaster2DSubmit& caster);

		// Ends the lit frame: finishes sprite batching, renders light accumulation
		// (with optional stencil shadows), then composites to output.
		static void EndScene();

		// Deterministic helper for tests and culling.
		static ScissorRect ComputeScissorRect(const glm::mat4& viewProjection,
			const glm::vec2& lightPosition,
			float radius,
			uint32_t viewportWidth,
			uint32_t viewportHeight);
	};
}
