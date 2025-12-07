#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/Texture.h"
#include "Pillar/Renderer/OrthographicCamera.h"
#include <glm/glm.hpp>

namespace Pillar {

	/**
	 * @brief 2D Renderer Interface
	 * 
	 * Provides a unified API for both immediate and batch rendering.
	 * Implementation can be swapped between:
	 * - BasicRenderer2D (one draw call per quad)
	 * - BatchRenderer2D (batched draw calls)
	 */
	class PIL_API IRenderer2D
	{
	public:
		virtual ~IRenderer2D() = default;

		// Scene management
		virtual void BeginScene(const OrthographicCamera& camera) = 0;
		virtual void EndScene() = 0;

		// Quad rendering (core primitive for sprites)
		virtual void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
		                     const glm::vec4& color) = 0;
		
		virtual void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
		                     const glm::vec4& color, Texture2D* texture) = 0;
		
		virtual void DrawQuad(const glm::vec3& position, const glm::vec2& size, 
		                     const glm::vec4& color, Texture2D* texture,
		                     const glm::vec2& texCoordMin, const glm::vec2& texCoordMax) = 0;

		// Rotated quad (for sprite rotation)
		virtual void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
		                            float rotation, const glm::vec4& color) = 0;
		
		virtual void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
		                            float rotation, const glm::vec4& color, Texture2D* texture) = 0;

		// Stats (for debugging/profiling)
		virtual uint32_t GetDrawCallCount() const = 0;
		virtual uint32_t GetQuadCount() const = 0;
		virtual void ResetStats() = 0;
	};

	/**
	 * @brief Static renderer API (facade pattern)
	 * 
	 * Allows switching between basic and batch renderer at runtime.
	 * Both teams can code against this interface.
	 */
	class PIL_API Renderer2DBackend
	{
	public:
		enum class API
		{
			Basic,  // Team A: Simple immediate mode
			Batch   // Team B: Batched rendering
		};

		static void Init(API api = API::Basic);
		static void Shutdown();
		static void SetAPI(API api);
		
		// Forward to active implementation
		static void BeginScene(const OrthographicCamera& camera);
		static void EndScene();
		
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
		                    const glm::vec4& color);
		
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
		                    const glm::vec4& color, Texture2D* texture);
		
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, 
		                    const glm::vec4& color, Texture2D* texture,
		                    const glm::vec2& texCoordMin, const glm::vec2& texCoordMax);

		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
		                           float rotation, const glm::vec4& color);
		
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
		                           float rotation, const glm::vec4& color, Texture2D* texture);

		// Stats
		static uint32_t GetDrawCallCount();
		static uint32_t GetQuadCount();
		static void ResetStats();

	private:
		static IRenderer2D* s_ActiveRenderer;
		static IRenderer2D* s_BasicRenderer;
		static IRenderer2D* s_BatchRenderer;
	};

} // namespace Pillar
