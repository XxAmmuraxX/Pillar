#include "GameLayer.h"

#include "Pillar/Logger.h"
#include "Pillar/Renderer/Renderer.h"

GameLayer::GameLayer()
    : Pillar::Layer("GameLayer")
{
}

GameLayer::~GameLayer() = default;

void GameLayer::OnAttach()
{
    PIL_INFO("GameLayer attached");
}

void GameLayer::OnDetach()
{
    PIL_INFO("GameLayer detached");
}

void GameLayer::OnUpdate(float deltaTime)
{
    (void)deltaTime;

    Pillar::Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    Pillar::Renderer::Clear();
}
