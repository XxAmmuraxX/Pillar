#pragma once

#include "Pillar.h"
#include <memory>

namespace RenderingDemo {

/**
 * @brief Complete Rendering Demo showcasing all Pillar rendering features
 * 
 * This demo demonstrates:
 * - Colored and textured quads
 * - Texture atlas usage (grid and TexturePacker)
 * - Rotation and scaling
 * - Post-processing effects
 * - Render queue with sorting
 * - Performance statistics display
 */
class RenderingDemoLayer : public Pillar::Layer
{
public:
    RenderingDemoLayer()
        : Layer("RenderingDemo")
    {
    }

    void OnAttach() override
    {
        PIL_INFO("=== Pillar Engine - Comprehensive Rendering Demo ===");
        
        // Get window size
        const auto& window = Pillar::Application::Get().GetWindow();
        m_ViewportWidth = window.GetWidth();
        m_ViewportHeight = window.GetHeight();
        
        // Create camera with controller
        float aspectRatio = (float)m_ViewportWidth / (float)m_ViewportHeight;
        m_CameraController = std::make_unique<Pillar::OrthographicCameraController>(
            aspectRatio, 
            true  // Enable rotation
        );
        
        // Set camera properties
        m_CameraController->SetZoomLevel(5.0f);
        m_CameraController->SetTranslationSpeed(2.0f);
        
        // Load textures
        LoadTextures();
        
        // Create texture atlas
        CreateTextureAtlas();
        
        // Initialize post-processing
        InitializePostProcessing();
        
        PIL_INFO("RenderingDemoLayer initialized successfully");
        PIL_INFO("Controls:");
        PIL_INFO("  WASD - Move camera");
        PIL_INFO("  Q/E - Rotate camera");
        PIL_INFO("  Mouse Wheel - Zoom");
        PIL_INFO("  1-5 - Toggle demo modes");
        PIL_INFO("  P - Toggle post-processing");
    }

    void OnDetach() override
    {
        if (m_PostProcessStack)
        {
            m_PostProcessStack->Shutdown();
        }
    }

    void OnUpdate(float deltaTime) override
    {
        // Update camera
        m_CameraController->OnUpdate(deltaTime);
        
        // Update animations
        m_Rotation += m_RotationSpeed * deltaTime;
        m_Time += deltaTime;
        
        // Render
        Render();
    }

    void OnEvent(Pillar::Event& event) override
    {
        m_CameraController->OnEvent(event);
        
        // Handle window resize for framebuffer
        if (event.GetEventType() == Pillar::EventType::WindowResize)
        {
            const auto& resizeEvent = static_cast<const Pillar::WindowResizeEvent&>(event);
            m_ViewportWidth = resizeEvent.GetWidth();
            m_ViewportHeight = resizeEvent.GetHeight();
            
            // Recreate framebuffer and resize post-processing
            if (m_SceneFramebuffer && m_PostProcessStack)
            {
                Pillar::FramebufferSpecification fbSpec;
                fbSpec.Width = m_ViewportWidth;
                fbSpec.Height = m_ViewportHeight;
                fbSpec.Samples = 1;
                m_SceneFramebuffer = Pillar::Framebuffer::Create(fbSpec);
                m_PostProcessStack->Resize(m_ViewportWidth, m_ViewportHeight);
            }
        }
        
        // Handle key presses for demo mode switching
        if (event.GetEventType() == Pillar::EventType::KeyPressed)
        {
            const auto& keyEvent = static_cast<const Pillar::KeyPressedEvent&>(event);
            
            switch (keyEvent.GetKeyCode())
            {
                case PIL_KEY_1: m_DemoMode = DemoMode::BasicQuads; break;
                case PIL_KEY_2: m_DemoMode = DemoMode::Textures; break;
                case PIL_KEY_3: m_DemoMode = DemoMode::Atlas; break;
                case PIL_KEY_4: m_DemoMode = DemoMode::Rotation; break;
                case PIL_KEY_5: m_DemoMode = DemoMode::Performance; break;
                case PIL_KEY_P: m_PostProcessingEnabled = !m_PostProcessingEnabled; break;
                default: break;
            }
        }
    }

    void OnImGuiRender() override
    {
        ImGui::Begin("Rendering Demo");
        
        // Demo mode selection
        ImGui::Text("Demo Modes (Press keys 1-5):");
        if (ImGui::RadioButton("1. Basic Colored Quads", m_DemoMode == DemoMode::BasicQuads))
            m_DemoMode = DemoMode::BasicQuads;
        if (ImGui::RadioButton("2. Textured Quads", m_DemoMode == DemoMode::Textures))
            m_DemoMode = DemoMode::Textures;
        if (ImGui::RadioButton("3. Texture Atlas", m_DemoMode == DemoMode::Atlas))
            m_DemoMode = DemoMode::Atlas;
        if (ImGui::RadioButton("4. Rotation & Scaling", m_DemoMode == DemoMode::Rotation))
            m_DemoMode = DemoMode::Rotation;
        if (ImGui::RadioButton("5. Performance Test", m_DemoMode == DemoMode::Performance))
            m_DemoMode = DemoMode::Performance;
        
        ImGui::Separator();
        
        // Post-processing controls
        ImGui::Checkbox("Post-Processing (P)", &m_PostProcessingEnabled);
        if (m_PostProcessStack)
        {
            m_PostProcessStack->SetEnabled(m_PostProcessingEnabled);
        }
        
        ImGui::Separator();
        
        // Camera info
        ImGui::Text("Camera Controls:");
        ImGui::BulletText("WASD - Move");
        ImGui::BulletText("Q/E - Rotate");
        ImGui::BulletText("Mouse Wheel - Zoom");
        
        const auto& camera = m_CameraController->GetCamera();
        ImGui::Text("Position: (%.2f, %.2f)", 
                    camera.GetPosition().x, 
                    camera.GetPosition().y);
        ImGui::Text("Rotation: %.2f deg", glm::degrees(camera.GetRotation()));
        ImGui::Text("Zoom: %.2f", m_CameraController->GetZoomLevel());
        
        ImGui::Separator();
        
        // Rendering statistics
        auto stats = Pillar::Renderer2D::GetStats();
        ImGui::Text("=== Rendering Statistics ===");
        ImGui::Text("Draw Calls: %u", stats.DrawCalls);
        ImGui::Text("Quads: %u", stats.QuadCount);
        ImGui::Text("Vertices: %u", stats.VertexCount);
        ImGui::Text("Batches: %u", stats.BatchCount);
        ImGui::Text("Texture Switches: %u", stats.TextureSwitches);
        ImGui::Text("Buffer Uploads: %u", stats.BufferUploads);
        ImGui::Text("Flush Count: %u", stats.FlushCount);
        
        ImGui::Separator();
        ImGui::Text("Efficiency Metrics:");
        ImGui::Text("Batch Efficiency: %.2f quads/batch", stats.GetBatchEfficiency());
        ImGui::Text("Avg Quads/Draw: %.2f", stats.GetAverageQuadsPerDraw());
        
        ImGui::Separator();
        ImGui::Text("Accumulated Stats:");
        ImGui::Text("Total Quads: %u", stats.TotalQuadsRendered);
        ImGui::Text("Total Draws: %u", stats.TotalDrawCalls);
        ImGui::Text("Peak Quads: %u", stats.PeakQuads);
        ImGui::Text("Peak Vertices: %u", stats.PeakVertices);
        
        ImGui::End();
        
        // Post-processing panel
        if (m_PostProcessingEnabled)
        {
            RenderPostProcessingPanel();
        }
    }

private:
    enum class DemoMode
    {
        BasicQuads,
        Textures,
        Atlas,
        Rotation,
        Performance
    };
    
    void LoadTextures()
    {
        PIL_INFO("Loading demo textures...");
        
        // Load individual textures
        m_TestTexture = Pillar::Texture2D::Create("checkerboard.png");
        
        PIL_INFO("Textures loaded successfully");
    }
    
    void CreateTextureAtlas()
    {
        PIL_INFO("Creating texture atlas...");
        
        // Create atlas from grid (8x8 grid of 32x32 cells)
        m_Atlas = Pillar::TextureAtlas::Create("spritesheet.png");
        if (m_Atlas)
        {
            m_Atlas->LoadFromGrid(8, 8, 32.0f, 32.0f);
            PIL_INFO("Texture atlas created successfully");
        }
        else
        {
            PIL_WARN("Failed to create texture atlas - sprite sheet may be missing");
        }
    }
    
    void InitializePostProcessing()
    {
        PIL_INFO("Initializing post-processing...");
        
        // Create scene framebuffer for rendering with actual viewport size
        Pillar::FramebufferSpecification fbSpec;
        fbSpec.Width = m_ViewportWidth;
        fbSpec.Height = m_ViewportHeight;
        fbSpec.Samples = 1;
        m_SceneFramebuffer = Pillar::Framebuffer::Create(fbSpec);
        
        // Create post-process stack
        m_PostProcessStack = std::make_unique<Pillar::PostProcessStack>();
        m_PostProcessStack->Init(m_ViewportWidth, m_ViewportHeight);
        
        // Add vignette effect
        m_VignetteEffect = std::make_shared<Pillar::VignetteEffect>();
        m_VignetteEffect->Init();  // Initialize the effect!
        m_VignetteEffect->SetRadius(m_VignetteRadius);
        m_VignetteEffect->SetSoftness(m_VignetteSoftness);
        m_VignetteEffect->SetIntensity(0.6f);
        m_VignetteEffect->SetEnabled(true);
        m_PostProcessStack->AddEffect(m_VignetteEffect);
        
        // Add chromatic aberration effect
        m_ChromaEffect = std::make_shared<Pillar::ChromaticAberrationEffect>();
        m_ChromaEffect->Init();  // Initialize the effect!
        m_ChromaEffect->SetOffset(m_ChromaOffset);
        m_ChromaEffect->SetIntensity(0.3f);
        m_ChromaEffect->SetEnabled(false);
        m_PostProcessStack->AddEffect(m_ChromaEffect);
        
        // Add grayscale effect
        m_GrayscaleEffect = std::make_shared<Pillar::GrayscaleEffect>();
        m_GrayscaleEffect->Init();  // Initialize the effect!
        m_GrayscaleEffect->SetIntensity(0.0f);
        m_GrayscaleEffect->SetEnabled(false);
        m_PostProcessStack->AddEffect(m_GrayscaleEffect);
        
        m_PostProcessStack->SetEnabled(m_PostProcessingEnabled);
        
        PIL_INFO("Post-processing initialized");
    }
    
    void Render()
    {
        // If post-processing is enabled, render to framebuffer first
        if (m_PostProcessingEnabled && m_SceneFramebuffer && m_PostProcessStack)
        {
            // Render scene to framebuffer
            m_SceneFramebuffer->Bind();
            Pillar::Renderer2D::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
            Pillar::Renderer2D::Clear();
            
            Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());
            RenderSceneContent();
            Pillar::Renderer2D::EndScene();
            
            m_SceneFramebuffer->Unbind();
            
            // Apply post-processing to screen
            Pillar::Renderer2D::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
            Pillar::Renderer2D::Clear();
            m_PostProcessStack->Process(m_SceneFramebuffer->GetColorAttachmentRendererID());
        }
        else
        {
            // Render directly to screen without post-processing
            Pillar::Renderer2D::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
            Pillar::Renderer2D::Clear();
            
            Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());
            RenderSceneContent();
            Pillar::Renderer2D::EndScene();
        }
    }
    
    void RenderSceneContent()
    {
        // Render based on demo mode
        switch (m_DemoMode)
        {
            case DemoMode::BasicQuads:
                RenderBasicQuads();
                break;
                
            case DemoMode::Textures:
                RenderTexturedQuads();
                break;
                
            case DemoMode::Atlas:
                RenderAtlasDemo();
                break;
                
            case DemoMode::Rotation:
                RenderRotationDemo();
                break;
                
            case DemoMode::Performance:
                RenderPerformanceTest();
                break;
        }
    }
    
    void RenderBasicQuads()
    {
        // Rainbow of colored quads
        float spacing = 1.5f;
        float startX = -4.5f;
        
        // Red
        Pillar::Renderer2D::DrawQuad(
            { startX + 0 * spacing, 0.0f },
            { 1.0f, 1.0f },
            { 1.0f, 0.0f, 0.0f, 1.0f }
        );
        
        // Orange
        Pillar::Renderer2D::DrawQuad(
            { startX + 1 * spacing, 0.0f },
            { 1.0f, 1.0f },
            { 1.0f, 0.5f, 0.0f, 1.0f }
        );
        
        // Yellow
        Pillar::Renderer2D::DrawQuad(
            { startX + 2 * spacing, 0.0f },
            { 1.0f, 1.0f },
            { 1.0f, 1.0f, 0.0f, 1.0f }
        );
        
        // Green
        Pillar::Renderer2D::DrawQuad(
            { startX + 3 * spacing, 0.0f },
            { 1.0f, 1.0f },
            { 0.0f, 1.0f, 0.0f, 1.0f }
        );
        
        // Blue
        Pillar::Renderer2D::DrawQuad(
            { startX + 4 * spacing, 0.0f },
            { 1.0f, 1.0f },
            { 0.0f, 0.0f, 1.0f, 1.0f }
        );
        
        // Indigo
        Pillar::Renderer2D::DrawQuad(
            { startX + 5 * spacing, 0.0f },
            { 1.0f, 1.0f },
            { 0.3f, 0.0f, 0.5f, 1.0f }
        );
        
        // Violet
        Pillar::Renderer2D::DrawQuad(
            { startX + 6 * spacing, 0.0f },
            { 1.0f, 1.0f },
            { 0.5f, 0.0f, 1.0f, 1.0f }
        );
    }
    
    void RenderTexturedQuads()
    {
        if (!m_TestTexture)
        {
            // Fallback to colored quad
            Pillar::Renderer2D::DrawQuad(
                { 0.0f, 0.0f },
                { 2.0f, 2.0f },
                { 1.0f, 0.0f, 1.0f, 1.0f }  // Magenta indicates missing texture
            );
            return;
        }
        
        // Normal texture
        Pillar::Renderer2D::DrawQuad(
            { -3.0f, 0.0f, 0.0f },
            { 2.0f, 2.0f },
            m_TestTexture
        );
        
        // Red tint
        Pillar::Renderer2D::DrawQuad(
            { 0.0f, 0.0f },
            { 2.0f, 2.0f },
            { 1.0f, 0.0f, 0.0f, 1.0f },
            m_TestTexture
        );
        
        // Green tint
        Pillar::Renderer2D::DrawQuad(
            { 3.0f, 0.0f },
            { 2.0f, 2.0f },
            { 0.0f, 1.0f, 0.0f, 1.0f },
            m_TestTexture
        );
    }
    
    void RenderAtlasDemo()
    {
        if (!m_Atlas)
        {
            // Fallback message
            Pillar::Renderer2D::DrawQuad(
                { 0.0f, 0.0f },
                { 2.0f, 2.0f },
                { 1.0f, 0.0f, 1.0f, 1.0f }
            );
            return;
        }
        
        // Draw sprites from atlas in a grid
        float spacing = 1.2f;
        int gridSize = 5;
        float startX = -gridSize * spacing / 2.0f;
        float startY = -gridSize * spacing / 2.0f;
        
        for (int row = 0; row < gridSize; ++row)
        {
            for (int col = 0; col < gridSize; ++col)
            {
                // Generate sprite name (sequential index as generated by LoadFromGrid)
                int spriteIndex = row * gridSize + col;
                std::string spriteName = std::to_string(spriteIndex);
                
                glm::vec2 position(
                    startX + col * spacing,
                    startY + row * spacing
                );
                
                Pillar::Renderer2D::DrawQuad(
                    position,
                    { 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f },
                    m_Atlas,
                    spriteName
                );
            }
        }
    }
    
    void RenderRotationDemo()
    {
        // Rotating squares at different speeds
        float baseRotation = m_Rotation;
        
        // Center rotating square
        Pillar::Renderer2D::DrawRotatedQuad(
            { 0.0f, 0.0f },
            { 2.0f, 2.0f },
            baseRotation,
            { 1.0f, 0.5f, 0.2f, 1.0f }
        );
        
        // Orbiting squares
        int numOrbits = 6;
        float orbitRadius = 4.0f;
        
        for (int i = 0; i < numOrbits; ++i)
        {
            float angle = (float)i / numOrbits * glm::two_pi<float>() + baseRotation * 0.5f;
            glm::vec2 position(
                glm::cos(angle) * orbitRadius,
                glm::sin(angle) * orbitRadius
            );
            
            float hue = (float)i / numOrbits;
            glm::vec4 color(
                glm::sin(hue * glm::pi<float>()),
                glm::sin((hue + 0.33f) * glm::pi<float>()),
                glm::sin((hue + 0.66f) * glm::pi<float>()),
                1.0f
            );
            
            Pillar::Renderer2D::DrawRotatedQuad(
                position,
                { 1.0f, 1.0f },
                -baseRotation * 2.0f,
                color
            );
        }
    }
    
    void RenderPerformanceTest()
    {
        // Render large number of quads to test batch renderer
        int gridSize = 50;  // 50x50 = 2500 quads
        float spacing = 0.5f;
        float startX = -gridSize * spacing / 2.0f;
        float startY = -gridSize * spacing / 2.0f;
        
        for (int row = 0; row < gridSize; ++row)
        {
            for (int col = 0; col < gridSize; ++col)
            {
                glm::vec2 position(
                    startX + col * spacing,
                    startY + row * spacing
                );
                
                // Color based on position
                float hue = (float)(row + col) / (gridSize * 2);
                glm::vec4 color(
                    glm::sin(hue * glm::pi<float>()),
                    glm::sin((hue + 0.33f) * glm::pi<float>()),
                    glm::sin((hue + 0.66f) * glm::pi<float>()),
                    1.0f
                );
                
                Pillar::Renderer2D::DrawQuad(
                    position,
                    { 0.4f, 0.4f },
                    color
                );
            }
        }
    }
    
    void RenderPostProcessingPanel()
    {
        ImGui::Begin("Post-Processing Effects");
        
        if (m_VignetteEffect)
        {
            ImGui::Text("Vignette");
            bool enabled = m_VignetteEffect->IsEnabled();
            if (ImGui::Checkbox("##VignetteEnabled", &enabled))
                m_VignetteEffect->SetEnabled(enabled);
            
            float intensity = m_VignetteEffect->GetIntensity();
            if (ImGui::SliderFloat("Intensity##Vignette", &intensity, 0.0f, 1.0f))
                m_VignetteEffect->SetIntensity(intensity);
            
            if (ImGui::SliderFloat("Radius##Vignette", &m_VignetteRadius, 0.0f, 1.0f))
                m_VignetteEffect->SetRadius(m_VignetteRadius);
            
            if (ImGui::SliderFloat("Softness##Vignette", &m_VignetteSoftness, 0.0f, 1.0f))
                m_VignetteEffect->SetSoftness(m_VignetteSoftness);
            
            ImGui::Separator();
        }
        
        if (m_ChromaEffect)
        {
            ImGui::Text("Chromatic Aberration");
            bool enabled = m_ChromaEffect->IsEnabled();
            if (ImGui::Checkbox("##ChromaEnabled", &enabled))
                m_ChromaEffect->SetEnabled(enabled);
            
            float intensity = m_ChromaEffect->GetIntensity();
            if (ImGui::SliderFloat("Intensity##Chroma", &intensity, 0.0f, 1.0f))
                m_ChromaEffect->SetIntensity(intensity);
            
            if (ImGui::SliderFloat("Offset##Chroma", &m_ChromaOffset, 0.0f, 0.02f))
                m_ChromaEffect->SetOffset(m_ChromaOffset);
            
            ImGui::Separator();
        }
        
        if (m_GrayscaleEffect)
        {
            ImGui::Text("Grayscale");
            bool enabled = m_GrayscaleEffect->IsEnabled();
            if (ImGui::Checkbox("##GrayscaleEnabled", &enabled))
                m_GrayscaleEffect->SetEnabled(enabled);
            
            float intensity = m_GrayscaleEffect->GetIntensity();
            if (ImGui::SliderFloat("Intensity##Grayscale", &intensity, 0.0f, 1.0f))
                m_GrayscaleEffect->SetIntensity(intensity);
        }
        
        ImGui::End();
    }

private:
    // Camera
    std::unique_ptr<Pillar::OrthographicCameraController> m_CameraController;
    
    // Textures
    std::shared_ptr<Pillar::Texture2D> m_TestTexture;
    std::shared_ptr<Pillar::TextureAtlas> m_Atlas;
    
    // Post-processing
    std::unique_ptr<Pillar::PostProcessStack> m_PostProcessStack;
    std::shared_ptr<Pillar::VignetteEffect> m_VignetteEffect;
    std::shared_ptr<Pillar::ChromaticAberrationEffect> m_ChromaEffect;
    std::shared_ptr<Pillar::GrayscaleEffect> m_GrayscaleEffect;
    std::shared_ptr<Pillar::Framebuffer> m_SceneFramebuffer;  // Framebuffer for post-processing
    bool m_PostProcessingEnabled = false;
    
    // Post-processing parameters
    float m_VignetteRadius = 0.7f;
    float m_VignetteSoftness = 0.5f;
    float m_ChromaOffset = 0.005f;
    
    // Viewport
    uint32_t m_ViewportWidth = 1280;
    uint32_t m_ViewportHeight = 720;
    
    // Demo state
    DemoMode m_DemoMode = DemoMode::BasicQuads;
    float m_Rotation = 0.0f;
    float m_RotationSpeed = 1.0f;  // radians per second
    float m_Time = 0.0f;
};

} // namespace RenderingDemo
