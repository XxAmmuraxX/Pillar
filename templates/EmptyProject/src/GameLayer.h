#pragma once

#include "Pillar/Layer.h"

class GameLayer : public Pillar::Layer
{
public:
    GameLayer();
    ~GameLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float deltaTime) override;
};
