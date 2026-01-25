#include "GameLayer.h"

#include "Pillar/Logger.h"
#include "Pillar/Renderer/Renderer.h"
#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/Input.h"

GameLayer::GameLayer()
    : Pillar::Layer("GameLayer")
{
}

GameLayer::~GameLayer() = default;

void GameLayer::OnAttach()
{
    PIL_INFO("GameLayer attached - Welcome to Pillar Engine!");
    
    // Enable keyboard camera controls for this demo
    // In a real game, you might want to disable this and control the camera programmatically
    m_CameraController.SetKeyboardControlEnabled(true);
    
    // Load a placeholder texture (create a checkerboard if missing)
    // You can replace this with your own texture in assets/textures/
    m_CheckerboardTexture = Pillar::Texture2D::Create("checkerboard.png");
    
    // Camera starts at origin
    m_CameraController.SetZoomLevel(5.0f);
    
    PIL_INFO("Camera controls: WASD = Move, Q/E = Rotate, Mouse Wheel = Zoom");
}

void GameLayer::OnDetach()
{
    PIL_INFO("GameLayer detached");
}

void GameLayer::OnUpdate(float deltaTime)
{
    // Update camera based on input
    m_CameraController.OnUpdate(deltaTime);
    
    // Clear screen
    Pillar::Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    Pillar::Renderer::Clear();
    
    // Begin rendering
    Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());
    
    // Example: Draw colored square at origin
    Pillar::Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, m_SquareColor);
    
    // Example: Draw textured square offset from origin
    if (m_CheckerboardTexture)
    {
        Pillar::Renderer2D::DrawQuad({ 2.0f, 0.0f }, { 1.0f, 1.0f }, m_CheckerboardTexture.get());
    }
    
    // Example: Draw rotating quad
    static float rotation = 0.0f;
    rotation += deltaTime * 45.0f; // 45 degrees per second
    Pillar::Renderer2D::DrawRotatedQuad({ -2.0f, 0.0f }, { 1.0f, 1.0f }, 
                                        glm::radians(rotation), 
                                        glm::vec4(1.0f, 0.5f, 0.2f, 1.0f));
    
    // End rendering
    Pillar::Renderer2D::EndScene();
}

void GameLayer::OnEvent(Pillar::Event& event)
{
    // Pass events to camera controller for input handling
    m_CameraController.OnEvent(event);
    
    // Add your custom event handling here
}

void GameLayer::OnImGuiRender()
{
    // Debug UI panel
    ImGui::Begin("Game Debug");
    
    ImGui::Text("Welcome to Pillar Engine!");
    ImGui::Separator();
    
    ImGui::Text("Camera Position: (%.1f, %.1f)", 
                m_CameraController.GetCamera().GetPosition().x,
                m_CameraController.GetCamera().GetPosition().y);
    ImGui::Text("Zoom Level: %.2f", m_CameraController.GetZoomLevel());
    
    ImGui::Separator();
    ImGui::ColorEdit4("Square Color", &m_SquareColor.r);
    
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD - Move camera");
    ImGui::BulletText("Q/E - Rotate camera");
    ImGui::BulletText("Mouse Wheel - Zoom");
    
    ImGui::End();
}
