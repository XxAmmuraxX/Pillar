#include <gtest/gtest.h>
#include "Pillar/Application.h"
#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/Renderer/OrthographicCamera.h"
#include "Pillar/Renderer/Texture.h"
#include <chrono>
#include <memory>
#include <functional>

namespace Pillar {

    /**
     * @brief Helper struct for benchmark results
     */
    struct BenchmarkResult
    {
        long long TotalTimeMs;
        float AvgTimePerFrame;
        float EstimatedFPS;
    };

    /**
     * @brief Run a rendering benchmark and measure performance
     * @param camera The camera to use for rendering
     * @param iterations Number of iterations to run
     * @param renderFunc Function that renders a single frame
     * @return BenchmarkResult with timing statistics
     */
    inline BenchmarkResult RunRenderBenchmark(
        const OrthographicCamera& camera,
        int iterations,
        const std::function<void()>& renderFunc)
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < iterations; ++iter)
        {
            Renderer2D::BeginScene(camera);
            renderFunc();
            Renderer2D::EndScene();
            Renderer2D::ResetStats();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        BenchmarkResult result;
        result.TotalTimeMs = duration.count();
        result.AvgTimePerFrame = duration.count() / static_cast<float>(iterations);
        result.EstimatedFPS = 1000.0f / result.AvgTimePerFrame;
        return result;
    }

    /**
     * @brief Log benchmark results with a descriptive title
     */
    inline void LogBenchmarkResult(const char* title, const BenchmarkResult& result)
    {
        PIL_CORE_INFO("{} Performance:", title);
        PIL_CORE_INFO("  Total Time: {} ms", result.TotalTimeMs);
        PIL_CORE_INFO("  Avg Time Per Frame: {:.2f} ms", result.AvgTimePerFrame);
        PIL_CORE_INFO("  Estimated FPS: {:.0f}", result.EstimatedFPS);
    }

    class Renderer2DPerformanceTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Initialize renderer (requires OpenGL context from Application)
            // Note: This assumes Application::Get() is valid
            if (!Application::GetInstancePtr())
            {
                GTEST_SKIP() << "Renderer2D performance tests require Application instance";
                return;
            }

            Renderer2D::Init();
            
            // Create test textures
            uint32_t whitePixel = 0xffffffff;
            m_WhiteTexture = std::shared_ptr<Texture2D>(Texture2D::Create(1, 1));
            m_WhiteTexture->SetData(&whitePixel, sizeof(uint32_t));
            
            // Create camera
            m_Camera = std::make_unique<OrthographicCamera>(-10.0f, 10.0f, -10.0f, 10.0f);
        }

        void TearDown() override
        {
            if (Application::GetInstancePtr())
            {
                Renderer2D::Shutdown();
            }
        }

        std::shared_ptr<Texture2D> m_WhiteTexture;
        std::unique_ptr<OrthographicCamera> m_Camera;
    };

    // Benchmark: 1000 colored quads (single texture)
    TEST_F(Renderer2DPerformanceTest, Render1000ColoredQuads)
    {
        if (!Application::GetInstancePtr())
        {
            GTEST_SKIP();
            return;
        }

        constexpr int QuadCount = 1000;
        constexpr int Iterations = 100;

        auto result = RunRenderBenchmark(*m_Camera, Iterations, [&]() {
            for (int i = 0; i < QuadCount; ++i)
            {
                float x = (i % 32) * 0.5f - 8.0f;
                float y = (i / 32) * 0.5f - 8.0f;
                glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
                Renderer2D::DrawQuad({ x, y }, { 0.4f, 0.4f }, color);
            }
        });

        LogBenchmarkResult("1000 Colored Quads", result);

        // Should easily hit 60 FPS (16.67ms per frame)
        EXPECT_LT(result.AvgTimePerFrame, 16.67f) << "Failed to achieve 60 FPS with 1000 quads";
    }

    // Benchmark: 10000 colored quads (single texture)
    TEST_F(Renderer2DPerformanceTest, Render10000ColoredQuads)
    {
        if (!Application::GetInstancePtr())
        {
            GTEST_SKIP();
            return;
        }

        constexpr int QuadCount = 10000;
        constexpr int Iterations = 100;

        auto result = RunRenderBenchmark(*m_Camera, Iterations, [&]() {
            for (int i = 0; i < QuadCount; ++i)
            {
                float x = (i % 100) * 0.2f - 10.0f;
                float y = (i / 100) * 0.2f - 10.0f;
                glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
                Renderer2D::DrawQuad({ x, y }, { 0.15f, 0.15f }, color);
            }
        });

        LogBenchmarkResult("10000 Colored Quads", result);

        // Should easily hit 60 FPS (16.67ms per frame)
        EXPECT_LT(result.AvgTimePerFrame, 16.67f) << "Failed to achieve 60 FPS with 10000 quads";
    }

    // Benchmark: 1000 textured quads (10 different textures)
    TEST_F(Renderer2DPerformanceTest, Render1000TexturedQuadsMultipleTextures)
    {
        if (!Application::GetInstancePtr())
        {
            GTEST_SKIP();
            return;
        }

        constexpr int QuadCount = 1000;
        constexpr int TextureCount = 10;
        constexpr int Iterations = 100;

        // Create multiple textures with different colors
        std::vector<std::shared_ptr<Texture2D>> textures;
        for (int t = 0; t < TextureCount; ++t)
        {
            uint32_t color = 0xff000000 | ((t * 25) << 16) | ((t * 25) << 8) | (t * 25);
            auto texture = std::shared_ptr<Texture2D>(Texture2D::Create(1, 1));
            texture->SetData(&color, sizeof(uint32_t));
            textures.push_back(texture);
        }

        auto result = RunRenderBenchmark(*m_Camera, Iterations, [&]() {
            for (int i = 0; i < QuadCount; ++i)
            {
                float x = (i % 32) * 0.5f - 8.0f;
                float y = (i / 32) * 0.5f - 8.0f;
                glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
                
                // Cycle through textures
                const auto& texture = textures[i % TextureCount];
                Renderer2D::DrawQuad({ x, y }, { 0.4f, 0.4f }, color, texture);
            }
        });

        LogBenchmarkResult("1000 Textured Quads (10 Textures)", result);

        // Should still hit 60 FPS even with multiple textures
        EXPECT_LT(result.AvgTimePerFrame, 16.67f) << "Failed to achieve 60 FPS with 1000 quads and 10 textures";
    }

    // Benchmark: Statistics tracking overhead
    TEST_F(Renderer2DPerformanceTest, DrawCallStatisticsAccuracy)
    {
        if (!Application::GetInstancePtr())
        {
            GTEST_SKIP();
            return;
        }

        Renderer2D::ResetStats();
        Renderer2D::BeginScene(*m_Camera);

        // Draw 100 colored quads (should be 1 draw call)
        for (int i = 0; i < 100; ++i)
        {
            Renderer2D::DrawQuad({ (float)i * 0.1f, 0.0f }, { 0.05f, 0.05f }, { 1.0f, 1.0f, 1.0f, 1.0f });
        }

        Renderer2D::EndScene();

        uint32_t drawCalls = Renderer2D::GetDrawCallCount();
        uint32_t quadCount = Renderer2D::GetQuadCount();

        PIL_CORE_INFO("Statistics Test:");
        PIL_CORE_INFO("  Draw Calls: {}", drawCalls);
        PIL_CORE_INFO("  Quad Count: {}", quadCount);

        EXPECT_EQ(quadCount, 100u) << "Incorrect quad count";
        EXPECT_LE(drawCalls, 2u) << "Too many draw calls for single-texture batch";
    }

} // namespace Pillar
