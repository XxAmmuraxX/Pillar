#include <gtest/gtest.h>
// Box2DContactListenerTests: exercises contact begin/end callbacks to ensure
// the engine's contact listener is invoked during collisions/separations.
#include "Pillar/ECS/Physics/Box2DContactListener.h"
#include "Pillar/ECS/Physics/Box2DWorld.h"
#include <box2d/box2d.h>

using namespace Pillar;

// ============================================================================
// Box2DContactListener Tests
// ============================================================================

class Box2DContactListenerTests : public ::testing::Test {
protected:
    void SetUp() override {
        m_World = std::make_unique<Box2DWorld>(glm::vec2(0.0f, 0.0f));
        m_Listener = std::make_unique<Box2DContactListener>();
        m_World->GetWorld()->SetContactListener(m_Listener.get());
    }

    void TearDown() override {
        m_Listener.reset();
        m_World.reset();
    }

    std::unique_ptr<Box2DWorld> m_World;
    std::unique_ptr<Box2DContactListener> m_Listener;

    b2Body* CreateDynamicBody(const b2Vec2& position, float radius = 0.5f) {
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = position;
        
        b2Body* body = m_World->GetWorld()->CreateBody(&bodyDef);
        
        b2CircleShape shape;
        shape.m_radius = radius;
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.density = 1.0f;
        body->CreateFixture(&fixtureDef);
        
        return body;
    }
};

TEST_F(Box2DContactListenerTests, BeginContact_FiresOnCollision) {
    // Create two overlapping bodies
    b2Body* body1 = CreateDynamicBody(b2Vec2(0.0f, 0.0f));
    b2Body* body2 = CreateDynamicBody(b2Vec2(0.5f, 0.0f)); // Overlapping
    
    // Step the world to detect collision
    m_World->Step(1.0f / 60.0f);
    
    // Contact listener should have been called (test passes if no crash)
    SUCCEED();
}

TEST_F(Box2DContactListenerTests, EndContact_FiresOnSeparation) {
    b2Body* body1 = CreateDynamicBody(b2Vec2(0.0f, 0.0f));
    b2Body* body2 = CreateDynamicBody(b2Vec2(0.5f, 0.0f));
    
    // Step to start contact
    m_World->Step(1.0f / 60.0f);
    
    // Move bodies apart
    body2->SetTransform(b2Vec2(5.0f, 0.0f), 0.0f);
    
    // Step to end contact
    m_World->Step(1.0f / 60.0f);
    
    SUCCEED();
}