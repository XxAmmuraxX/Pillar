#include <gtest/gtest.h>
// PhysicsSystemTests: verifies Box2D integration, body creation, gravity,
// static/dynamic behavior, fixed rotation, and gravity scaling.
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Systems/PhysicsSystem.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"

using namespace Pillar;

class PhysicsSystemTests : public ::testing::Test {
protected:
    void SetUp() override {
        m_Scene = std::make_unique<Scene>("PhysicsTest");
        m_PhysicsSystem = std::make_unique<PhysicsSystem>(glm::vec2(0.0f, -9.81f));
        m_PhysicsSystem->OnAttach(m_Scene.get());
    }

    void TearDown() override {
        m_PhysicsSystem->OnDetach();
        m_PhysicsSystem.reset();
        m_Scene.reset();
    }

    std::unique_ptr<Scene> m_Scene;
    std::unique_ptr<PhysicsSystem> m_PhysicsSystem;
};

TEST_F(PhysicsSystemTests, SystemAttach_CreatesWorld) {
    EXPECT_NE(m_PhysicsSystem->GetWorld(), nullptr);
}

TEST_F(PhysicsSystemTests, Update_WithNoEntities_DoesNotCrash) {
    EXPECT_NO_THROW(m_PhysicsSystem->OnUpdate(0.016f));
}

TEST_F(PhysicsSystemTests, Update_CreatesBodyForRigidbodyEntity) {
    auto entity = m_Scene->CreateEntity("PhysicsEntity");
    auto& rb = entity.AddComponent<RigidbodyComponent>();
    entity.AddComponent<ColliderComponent>(ColliderComponent::Circle(0.5f));

    // Need to accumulate enough time to trigger a fixed update (1/60 = 0.0167s)
    m_PhysicsSystem->OnUpdate(0.02f);

    EXPECT_NE(rb.Body, nullptr);
}

TEST_F(PhysicsSystemTests, Gravity_AffectsDynamicBodies) {
    auto entity = m_Scene->CreateEntity("FallingEntity");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(0.0f, 10.0f);
    
    auto& rb = entity.AddComponent<RigidbodyComponent>();
    rb.BodyType = b2_dynamicBody;
    entity.AddComponent<ColliderComponent>(ColliderComponent::Circle(0.5f));

    // First update creates the body
    m_PhysicsSystem->OnUpdate(0.017f);
    
    float initialY = rb.Body->GetPosition().y;
    
    // Simulate several frames
    for (int i = 0; i < 60; i++) {
        m_PhysicsSystem->OnUpdate(0.016f);
    }

    // Body should have fallen due to gravity
    EXPECT_LT(rb.Body->GetPosition().y, initialY);
}

TEST_F(PhysicsSystemTests, StaticBody_DoesNotMove) {
    auto entity = m_Scene->CreateEntity("StaticEntity");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(0.0f, 5.0f);
    
    auto& rb = entity.AddComponent<RigidbodyComponent>();
    rb.BodyType = b2_staticBody;
    entity.AddComponent<ColliderComponent>(ColliderComponent::Box(glm::vec2(1.0f, 1.0f)));

    m_PhysicsSystem->OnUpdate(0.017f);
    
    b2Vec2 initialPos = rb.Body->GetPosition();
    
    for (int i = 0; i < 60; i++) {
        m_PhysicsSystem->OnUpdate(0.016f);
    }

    EXPECT_FLOAT_EQ(rb.Body->GetPosition().y, initialPos.y);
}

TEST_F(PhysicsSystemTests, FixedRotation_PreventsBodyRotation) {
    auto entity = m_Scene->CreateEntity("NonRotating");
    auto& rb = entity.AddComponent<RigidbodyComponent>();
    rb.BodyType = b2_dynamicBody;
    rb.FixedRotation = true;
    entity.AddComponent<ColliderComponent>(ColliderComponent::Circle(0.5f));

    m_PhysicsSystem->OnUpdate(0.017f);

    EXPECT_TRUE(rb.Body->IsFixedRotation());
}

TEST_F(PhysicsSystemTests, GravityScale_ZeroDisablesGravity) {
    auto entity = m_Scene->CreateEntity("NoGravityEntity");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(0.0f, 10.0f);
    
    auto& rb = entity.AddComponent<RigidbodyComponent>();
    rb.BodyType = b2_dynamicBody;
    rb.GravityScale = 0.0f;
    entity.AddComponent<ColliderComponent>(ColliderComponent::Circle(0.5f));

    m_PhysicsSystem->OnUpdate(0.017f);
    
    float initialY = rb.Body->GetPosition().y;
    
    for (int i = 0; i < 60; i++) {
        m_PhysicsSystem->OnUpdate(0.016f);
    }

    // Should not have fallen (no gravity)
    EXPECT_FLOAT_EQ(rb.Body->GetPosition().y, initialY);
}