#include "Box2DBodyFactory.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"

namespace Pillar {

	b2Body* Box2DBodyFactory::CreateBody(
		b2World* world,
		const glm::vec2& position,
		float rotation,
		b2BodyType bodyType,
		bool fixedRotation,
		float gravityScale)
	{
		b2BodyDef bodyDef;
		bodyDef.type = bodyType;
		bodyDef.position.Set(position.x, position.y);
		bodyDef.angle = rotation;
		bodyDef.fixedRotation = fixedRotation;
		bodyDef.gravityScale = gravityScale;

		return world->CreateBody(&bodyDef);
	}

	b2Fixture* Box2DBodyFactory::CreateFixture(b2Body* body, const ColliderComponent& collider)
	{
		b2FixtureDef fixtureDef = CreateFixtureDef(collider);

		b2Shape* shape = nullptr;
		b2CircleShape circleShape;
		b2PolygonShape boxShape;

		switch (collider.Type)
		{
		case ColliderType::Circle:
			circleShape = CreateCircleShape(collider);
			fixtureDef.shape = &circleShape;
			break;

		case ColliderType::Box:
			boxShape = CreateBoxShape(collider);
			fixtureDef.shape = &boxShape;
			break;

		case ColliderType::Polygon:
			// TODO: Implement polygon shape
			break;
		}

		return body->CreateFixture(&fixtureDef);
	}

	b2FixtureDef Box2DBodyFactory::CreateFixtureDef(const ColliderComponent& collider)
	{
		b2FixtureDef fixtureDef;
		fixtureDef.density = collider.Density;
		fixtureDef.friction = collider.Friction;
		fixtureDef.restitution = collider.Restitution;
		fixtureDef.isSensor = collider.IsSensor;
		fixtureDef.filter.categoryBits = collider.CategoryBits;
		fixtureDef.filter.maskBits = collider.MaskBits;
		fixtureDef.filter.groupIndex = collider.GroupIndex;

		return fixtureDef;
	}

	b2CircleShape Box2DBodyFactory::CreateCircleShape(const ColliderComponent& collider)
	{
		b2CircleShape shape;
		shape.m_radius = collider.Radius;
		shape.m_p.Set(collider.Offset.x, collider.Offset.y);
		return shape;
	}

	b2PolygonShape Box2DBodyFactory::CreateBoxShape(const ColliderComponent& collider)
	{
		b2PolygonShape shape;
		shape.SetAsBox(collider.HalfExtents.x, collider.HalfExtents.y,
			b2Vec2(collider.Offset.x, collider.Offset.y), 0.0f);
		return shape;
	}

} // namespace Pillar
