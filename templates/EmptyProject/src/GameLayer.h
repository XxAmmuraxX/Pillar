#pragma once

#include "Pillar.h"

class GameLayer : public Pillar::Layer
{
public:
    GameLayer();
    ~GameLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float deltaTime) override;
    void OnEvent(Pillar::Event& event) override;
    void OnImGuiRender() override;

private:
    // Camera
    Pillar::OrthographicCameraController m_CameraController;
    
    // Example rendering
    std::shared_ptr<Pillar::Texture2D> m_CheckerboardTexture;
    glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };
    
    // Add your game state here
};
