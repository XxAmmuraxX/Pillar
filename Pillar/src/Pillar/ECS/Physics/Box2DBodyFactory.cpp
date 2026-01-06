#include "Box2DBodyFactory.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/Logger.h"

namespace Pillar {

	b2Body* Box2DBodyFactory::CreateBody(
		b2World* world,
		const glm::vec2& position,
		float rotation,
		b2BodyType bodyType,
		bool fixedRotation,
		float gravityScale,
		float linearDamping,
		float angularDamping,
		bool isBullet,
		bool isEnabled)
	{
		b2BodyDef bodyDef;
		bodyDef.type = bodyType;
		bodyDef.position.Set(position.x, position.y);
		bodyDef.angle = rotation;
		bodyDef.fixedRotation = fixedRotation;
		bodyDef.gravityScale = gravityScale;
		bodyDef.linearDamping = linearDamping;
		bodyDef.angularDamping = angularDamping;
		bodyDef.bullet = isBullet;
		bodyDef.enabled = isEnabled;

		return world->CreateBody(&bodyDef);
	}

	b2Fixture* Box2DBodyFactory::CreateFixture(b2Body* body, const ColliderComponent& collider)
	{
		b2FixtureDef fixtureDef = CreateFixtureDef(collider);

		b2Shape* shape = nullptr;
		b2CircleShape circleShape;
		b2PolygonShape polygonShape;

		switch (collider.Type)
		{
		case ColliderType::Circle:
			circleShape = CreateCircleShape(collider);
			fixtureDef.shape = &circleShape;
			break;

		case ColliderType::Box:
			polygonShape = CreateBoxShape(collider);
			fixtureDef.shape = &polygonShape;
			break;

		case ColliderType::Polygon:
			polygonShape = CreatePolygonShape(collider);
			fixtureDef.shape = &polygonShape;
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
		// Ensure minimum radius to prevent Box2D assertion failures
		shape.m_radius = glm::max(collider.Radius, 0.01f);
		shape.m_p.Set(collider.Offset.x, collider.Offset.y);
		return shape;
	}

	b2PolygonShape Box2DBodyFactory::CreateBoxShape(const ColliderComponent& collider)
	{
		b2PolygonShape shape;
		// Ensure minimum half-extents to prevent Box2D assertion failures
		float halfX = glm::max(collider.HalfExtents.x, 0.01f);
		float halfY = glm::max(collider.HalfExtents.y, 0.01f);
		shape.SetAsBox(halfX, halfY,
			b2Vec2(collider.Offset.x, collider.Offset.y), 0.0f);
		return shape;
	}
	b2PolygonShape Box2DBodyFactory::CreatePolygonShape(const ColliderComponent& collider)
	{
		b2PolygonShape shape;

		// Validate vertex count (Box2D requires 3-8 vertices)
		if (collider.Vertices.size() < 3)
		{
			PIL_CORE_WARN("Polygon collider requires at least 3 vertices, got {0}. Creating default triangle.", collider.Vertices.size());
			// Create a small default triangle
			b2Vec2 defaultVerts[3];
			defaultVerts[0].Set(0.0f, 0.5f);
			defaultVerts[1].Set(-0.43f, -0.25f);
			defaultVerts[2].Set(0.43f, -0.25f);
			shape.Set(defaultVerts, 3);
			return shape;
		}

		if (collider.Vertices.size() > b2_maxPolygonVertices)
		{
			PIL_CORE_WARN("Polygon collider has {0} vertices, but Box2D maximum is {1}. Using first {1} vertices.",
				collider.Vertices.size(), b2_maxPolygonVertices);
		}

		// Convert glm vertices to Box2D vertices (apply offset)
		int32 vertexCount = glm::min((int)collider.Vertices.size(), b2_maxPolygonVertices);
		b2Vec2 b2Vertices[b2_maxPolygonVertices];
		
		for (int32 i = 0; i < vertexCount; ++i)
		{
			b2Vertices[i].Set(
				collider.Vertices[i].x + collider.Offset.x,
				collider.Vertices[i].y + collider.Offset.y
			);
		}

		// Set the polygon shape (Box2D will validate convexity and winding order)
		shape.Set(b2Vertices, vertexCount);
		return shape;
	}


} // namespace Pillar
