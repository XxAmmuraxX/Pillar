#include <gtest/gtest.h>
#include "Pillar/ECS/Physics/Box2DBodyFactory.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include <box2d/box2d.h>

using namespace Pillar;

class Box2DBodyFactoryTests : public ::testing::Test {
protected:
    void SetUp() override {
        m_World = new b2World(b2Vec2(0.0f, -9.81f));
    }

    void TearDown() override {
        delete m_World;
    }

    b2World* m_World;
};

TEST_F(Box2DBodyFactoryTests, CreateBody_DynamicBody) {
    b2Body* body = Box2DBodyFactory::CreateBody(
        m_World,
        glm::vec2(5.0f, 10.0f),
        0.5f, // rotation
        b2_dynamicBody,
        false, // fixedRotation
        1.0f   // gravityScale
    );

    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->GetType(), b2_dynamicBody);
    EXPECT_FLOAT_EQ(body->GetPosition().x, 5.0f);
    EXPECT_FLOAT_EQ(body->GetPosition().y, 10.0f);
    EXPECT_FLOAT_EQ(body->GetAngle(), 0.5f);
    EXPECT_FLOAT_EQ(body->GetGravityScale(), 1.0f);
}

TEST_F(Box2DBodyFactoryTests, CreateBody_StaticBody) {
    b2Body* body = Box2DBodyFactory::CreateBody(
        m_World,
        glm::vec2(0.0f, 0.0f),
        0.0f,
        b2_staticBody
    );

    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->GetType(), b2_staticBody);
}

TEST_F(Box2DBodyFactoryTests, CreateBody_KinematicBody) {
    b2Body* body = Box2DBodyFactory::CreateBody(
        m_World,
        glm::vec2(0.0f, 0.0f),
        0.0f,
        b2_kinematicBody
    );

    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->GetType(), b2_kinematicBody);
}

TEST_F(Box2DBodyFactoryTests, CreateBody_FixedRotation) {
    b2Body* body = Box2DBodyFactory::CreateBody(
        m_World,
        glm::vec2(0.0f, 0.0f),
        0.0f,
        b2_dynamicBody,
        true // fixedRotation
    );

    ASSERT_NE(body, nullptr);
    EXPECT_TRUE(body->IsFixedRotation());
}

TEST_F(Box2DBodyFactoryTests, CreateFixture_CircleCollider) {
    b2Body* body = Box2DBodyFactory::CreateBody(
        m_World, glm::vec2(0.0f, 0.0f), 0.0f, b2_dynamicBody
    );

    ColliderComponent collider = ColliderComponent::Circle(1.5f);
    collider.Density = 2.0f;
    collider.Friction = 0.5f;
    collider.Restitution = 0.3f;

    b2Fixture* fixture = Box2DBodyFactory::CreateFixture(body, collider);

    ASSERT_NE(fixture, nullptr);
    EXPECT_EQ(fixture->GetShape()->GetType(), b2Shape::e_circle);
    EXPECT_FLOAT_EQ(fixture->GetDensity(), 2.0f);
    EXPECT_FLOAT_EQ(fixture->GetFriction(), 0.5f);
    EXPECT_FLOAT_EQ(fixture->GetRestitution(), 0.3f);
}

TEST_F(Box2DBodyFactoryTests, CreateFixture_BoxCollider) {
    b2Body* body = Box2DBodyFactory::CreateBody(
        m_World, glm::vec2(0.0f, 0.0f), 0.0f, b2_dynamicBody
    );

    ColliderComponent collider = ColliderComponent::Box(glm::vec2(2.0f, 3.0f));
    
    b2Fixture* fixture = Box2DBodyFactory::CreateFixture(body, collider);

    ASSERT_NE(fixture, nullptr);
    EXPECT_EQ(fixture->GetShape()->GetType(), b2Shape::e_polygon);
}

TEST_F(Box2DBodyFactoryTests, CreateFixture_SensorCollider) {
    b2Body* body = Box2DBodyFactory::CreateBody(
        m_World, glm::vec2(0.0f, 0.0f), 0.0f, b2_dynamicBody
    );

    ColliderComponent collider = ColliderComponent::Circle(1.0f);
    collider.IsSensor = true;

    b2Fixture* fixture = Box2DBodyFactory::CreateFixture(body, collider);

    ASSERT_NE(fixture, nullptr);
    EXPECT_TRUE(fixture->IsSensor());
}

TEST_F(Box2DBodyFactoryTests, CreateFixture_CollisionFiltering) {
    b2Body* body = Box2DBodyFactory::CreateBody(
        m_World, glm::vec2(0.0f, 0.0f), 0.0f, b2_dynamicBody
    );

    ColliderComponent collider = ColliderComponent::Circle(1.0f);
    collider.CategoryBits = 0x0002;
    collider.MaskBits = 0x0004;
    collider.GroupIndex = -1;

    b2Fixture* fixture = Box2DBodyFactory::CreateFixture(body, collider);

    ASSERT_NE(fixture, nullptr);
    EXPECT_EQ(fixture->GetFilterData().categoryBits, 0x0002);
    EXPECT_EQ(fixture->GetFilterData().maskBits, 0x0004);
    EXPECT_EQ(fixture->GetFilterData().groupIndex, -1);
}

TEST_F(Box2DBodyFactoryTests, CreateFixture_WithOffset) {
    b2Body* body = Box2DBodyFactory::CreateBody(
        m_World, glm::vec2(0.0f, 0.0f), 0.0f, b2_dynamicBody
    );

    ColliderComponent collider = ColliderComponent::Circle(1.0f);
    collider.Offset = glm::vec2(2.0f, 3.0f);

    b2Fixture* fixture = Box2DBodyFactory::CreateFixture(body, collider);

    ASSERT_NE(fixture, nullptr);
    // Circle shape stores offset in m_p
    const b2CircleShape* circle = static_cast<const b2CircleShape*>(fixture->GetShape());
    EXPECT_FLOAT_EQ(circle->m_p.x, 2.0f);
    EXPECT_FLOAT_EQ(circle->m_p.y, 3.0f);
}