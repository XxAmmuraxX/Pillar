#include "ComponentRegistry.h"
#include "Components/Core/TagComponent.h"
#include "Components/Core/TransformComponent.h"
#include "Components/Core/UUIDComponent.h"
#include "Components/Physics/RigidbodyComponent.h"
#include "Components/Physics/ColliderComponent.h"
#include "Components/Physics/VelocityComponent.h"
#include "Components/Gameplay/XPGemComponent.h"
#include "Components/Gameplay/BulletComponent.h"
#include "Components/Gameplay/ParticleComponent.h"
#include "Components/Gameplay/ParticleEmitterComponent.h"
#include "Components/Rendering/SpriteComponent.h"
#include <box2d/box2d.h>

namespace Pillar {

	// Helper functions for GLM serialization
	namespace JsonHelpers {
		inline json SerializeVec2(const glm::vec2& vec)
		{
			return json::array({ vec.x, vec.y });
		}

		inline glm::vec2 DeserializeVec2(const json& j)
		{
			return glm::vec2(j[0].get<float>(), j[1].get<float>());
		}

		inline json SerializeVec4(const glm::vec4& vec)
		{
			return json::array({ vec.x, vec.y, vec.z, vec.w });
		}

		inline glm::vec4 DeserializeVec4(const json& j)
		{
			return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
		}
	}

	void RegisterBuiltinComponents()
	{
		auto& registry = ComponentRegistry::Get();

		// Skip if already registered
		if (registry.GetRegistrationCount() > 0)
			return;

		// ============================================================
		// Core Components
		// ============================================================

		// TransformComponent
		registry.Register<TransformComponent>("transform",
			// Serialize
			[](Entity e) -> json {
				if (!e.HasComponent<TransformComponent>()) return nullptr;
				auto& t = e.GetComponent<TransformComponent>();
				return json{
					{ "position", JsonHelpers::SerializeVec2(t.Position) },
					{ "rotation", t.Rotation },
					{ "scale", JsonHelpers::SerializeVec2(t.Scale) }
				};
			},
			// Deserialize
			[](Entity e, const json& j) {
				// TransformComponent is always added by CreateEntity, so we get it
				auto& t = e.GetComponent<TransformComponent>();
				if (j.contains("position"))
					t.Position = JsonHelpers::DeserializeVec2(j["position"]);
				if (j.contains("rotation"))
					t.Rotation = j["rotation"].get<float>();
				if (j.contains("scale"))
					t.Scale = JsonHelpers::DeserializeVec2(j["scale"]);
				t.Dirty = true;
			},
			// Copy
			[](Entity src, Entity dst) {
				if (!src.HasComponent<TransformComponent>()) return;
				auto& s = src.GetComponent<TransformComponent>();
				auto& d = dst.GetComponent<TransformComponent>();
				d.Position = s.Position;
				d.Rotation = s.Rotation;
				d.Scale = s.Scale;
				d.Dirty = true;
			}
		);

		// ============================================================
		// Physics Components
		// ============================================================

		// RigidbodyComponent
		registry.Register<RigidbodyComponent>("rigidbody",
			// Serialize
			[](Entity e) -> json {
				if (!e.HasComponent<RigidbodyComponent>()) return nullptr;
				auto& rb = e.GetComponent<RigidbodyComponent>();
				std::string bodyType;
				switch (rb.BodyType)
				{
					case b2_staticBody: bodyType = "static"; break;
					case b2_kinematicBody: bodyType = "kinematic"; break;
					case b2_dynamicBody: bodyType = "dynamic"; break;
				}
				return json{
					{ "bodyType", bodyType },
					{ "fixedRotation", rb.FixedRotation },
					{ "gravityScale", rb.GravityScale }
				};
			},
			// Deserialize
			[](Entity e, const json& j) {
				b2BodyType bodyType = b2_dynamicBody;
				if (j.contains("bodyType"))
				{
					std::string typeStr = j["bodyType"].get<std::string>();
					if (typeStr == "static") bodyType = b2_staticBody;
					else if (typeStr == "kinematic") bodyType = b2_kinematicBody;
				}
				auto& rb = e.AddComponent<RigidbodyComponent>(bodyType);
				if (j.contains("fixedRotation"))
					rb.FixedRotation = j["fixedRotation"].get<bool>();
				if (j.contains("gravityScale"))
					rb.GravityScale = j["gravityScale"].get<float>();
			},
			// Copy - RigidbodyComponent has unique b2Body*, so we only copy data
			[](Entity src, Entity dst) {
				if (!src.HasComponent<RigidbodyComponent>()) return;
				auto& s = src.GetComponent<RigidbodyComponent>();
				auto& d = dst.AddComponent<RigidbodyComponent>(s.BodyType);
				d.FixedRotation = s.FixedRotation;
				d.GravityScale = s.GravityScale;
				// Note: Body pointer stays null, will be created by physics system
			}
		);

		// ColliderComponent
		registry.Register<ColliderComponent>("collider",
			// Serialize
			[](Entity e) -> json {
				if (!e.HasComponent<ColliderComponent>()) return nullptr;
				auto& c = e.GetComponent<ColliderComponent>();
				json j;
				
				if (c.Type == ColliderType::Box)
				{
					j["type"] = "box";
					j["halfExtents"] = JsonHelpers::SerializeVec2(c.HalfExtents);
				}
				else if (c.Type == ColliderType::Circle)
				{
					j["type"] = "circle";
					j["radius"] = c.Radius;
				}
				
				j["offset"] = JsonHelpers::SerializeVec2(c.Offset);
				j["friction"] = c.Friction;
				j["restitution"] = c.Restitution;
				j["density"] = c.Density;
				j["isSensor"] = c.IsSensor;
				
				return j;
			},
			// Deserialize
			[](Entity e, const json& j) {
				std::string type = j.value("type", "box");
				
				ColliderComponent collider;
				if (type == "circle")
				{
					collider = ColliderComponent::Circle(j.value("radius", 0.5f));
				}
				else
				{
					glm::vec2 halfExtents = j.contains("halfExtents") ?
						JsonHelpers::DeserializeVec2(j["halfExtents"]) :
						glm::vec2(0.5f);
					collider = ColliderComponent::Box(halfExtents);
				}
				
				if (j.contains("offset"))
					collider.Offset = JsonHelpers::DeserializeVec2(j["offset"]);
				collider.Friction = j.value("friction", 0.3f);
				collider.Restitution = j.value("restitution", 0.0f);
				collider.Density = j.value("density", 1.0f);
				collider.IsSensor = j.value("isSensor", false);
				
				e.AddComponent<ColliderComponent>(collider);
			},
			// Copy
			[](Entity src, Entity dst) {
				if (!src.HasComponent<ColliderComponent>()) return;
				auto& s = src.GetComponent<ColliderComponent>();
				ColliderComponent d;
				d.Type = s.Type;
				d.Offset = s.Offset;
				if (s.Type == ColliderType::Circle)
					d.Radius = s.Radius;
				else
					d.HalfExtents = s.HalfExtents;
				d.Density = s.Density;
				d.Friction = s.Friction;
				d.Restitution = s.Restitution;
				d.IsSensor = s.IsSensor;
				dst.AddComponent<ColliderComponent>(d);
			}
		);

		// VelocityComponent
		registry.Register<VelocityComponent>("velocity",
			// Serialize
			[](Entity e) -> json {
				if (!e.HasComponent<VelocityComponent>()) return nullptr;
				auto& v = e.GetComponent<VelocityComponent>();
				return json{
					{ "velocity", JsonHelpers::SerializeVec2(v.Velocity) },
					{ "acceleration", JsonHelpers::SerializeVec2(v.Acceleration) },
					{ "drag", v.Drag },
					{ "maxSpeed", v.MaxSpeed }
				};
			},
			// Deserialize
			[](Entity e, const json& j) {
				auto& v = e.AddComponent<VelocityComponent>();
				if (j.contains("velocity"))
					v.Velocity = JsonHelpers::DeserializeVec2(j["velocity"]);
				if (j.contains("acceleration"))
					v.Acceleration = JsonHelpers::DeserializeVec2(j["acceleration"]);
				if (j.contains("drag"))
					v.Drag = j["drag"].get<float>();
				if (j.contains("maxSpeed"))
					v.MaxSpeed = j["maxSpeed"].get<float>();
			},
			// Copy
			[](Entity src, Entity dst) {
				if (!src.HasComponent<VelocityComponent>()) return;
				auto& s = src.GetComponent<VelocityComponent>();
				auto& d = dst.AddComponent<VelocityComponent>();
				d.Velocity = s.Velocity;
				d.Acceleration = s.Acceleration;
				d.Drag = s.Drag;
				d.MaxSpeed = s.MaxSpeed;
			}
		);

		// ============================================================
		// Gameplay Components
		// ============================================================

		// XPGemComponent
		registry.Register<XPGemComponent>("xpGem",
			// Serialize
			[](Entity e) -> json {
				if (!e.HasComponent<XPGemComponent>()) return nullptr;
				auto& g = e.GetComponent<XPGemComponent>();
				return json{
					{ "xpValue", g.XPValue },
					{ "attractionRadius", g.AttractionRadius },
					{ "moveSpeed", g.MoveSpeed }
				};
			},
			// Deserialize
			[](Entity e, const json& j) {
				int xpValue = j.value("xpValue", 1);
				auto& g = e.AddComponent<XPGemComponent>(xpValue);
				if (j.contains("attractionRadius"))
					g.AttractionRadius = j["attractionRadius"].get<float>();
				if (j.contains("moveSpeed"))
					g.MoveSpeed = j["moveSpeed"].get<float>();
			},
			// Copy
			[](Entity src, Entity dst) {
				if (!src.HasComponent<XPGemComponent>()) return;
				auto& s = src.GetComponent<XPGemComponent>();
				auto& d = dst.AddComponent<XPGemComponent>(s.XPValue);
				d.AttractionRadius = s.AttractionRadius;
				d.MoveSpeed = s.MoveSpeed;
			}
		);

		// BulletComponent - typically not serialized (runtime-only), but for completeness
		registry.Register<BulletComponent>("bullet",
			// Serialize
			[](Entity e) -> json {
				if (!e.HasComponent<BulletComponent>()) return nullptr;
				auto& b = e.GetComponent<BulletComponent>();
				return json{
					{ "damage", b.Damage },
					{ "lifetime", b.Lifetime },
					{ "timeAlive", b.TimeAlive },
					{ "pierce", b.Pierce },
					{ "maxHits", b.MaxHits },
					{ "hitsRemaining", b.HitsRemaining }
				};
			},
			// Deserialize
			[](Entity e, const json& j) {
				Entity owner; // Owner reference is lost during serialization
				float damage = j.value("damage", 10.0f);
				auto& b = e.AddComponent<BulletComponent>(owner, damage);
				if (j.contains("lifetime"))
					b.Lifetime = j["lifetime"].get<float>();
				if (j.contains("timeAlive"))
					b.TimeAlive = j["timeAlive"].get<float>();
				if (j.contains("pierce"))
					b.Pierce = j["pierce"].get<bool>();
				if (j.contains("maxHits"))
					b.MaxHits = j["maxHits"].get<uint32_t>();
				if (j.contains("hitsRemaining"))
					b.HitsRemaining = j["hitsRemaining"].get<uint32_t>();
			},
			// Copy
			[](Entity src, Entity dst) {
				if (!src.HasComponent<BulletComponent>()) return;
				auto& s = src.GetComponent<BulletComponent>();
				auto& d = dst.AddComponent<BulletComponent>(Entity(), s.Damage);
				d.Lifetime = s.Lifetime;
				d.TimeAlive = s.TimeAlive;
				d.Pierce = s.Pierce;
				d.MaxHits = s.MaxHits;
				d.HitsRemaining = s.HitsRemaining;
			}
		);

		// ============================================================
		// Rendering Components
		// ============================================================

		// SpriteComponent
		registry.Register<SpriteComponent>("sprite",
			// Serialize
			[](Entity e) -> json {
				if (!e.HasComponent<SpriteComponent>()) return nullptr;
				auto& s = e.GetComponent<SpriteComponent>();
				return json{
					{ "color", JsonHelpers::SerializeVec4(s.Color) },
					{ "size", JsonHelpers::SerializeVec2(s.Size) },
					{ "texCoordMin", JsonHelpers::SerializeVec2(s.TexCoordMin) },
					{ "texCoordMax", JsonHelpers::SerializeVec2(s.TexCoordMax) },
					{ "zIndex", s.ZIndex }
					// Note: Texture not serialized (requires asset management)
				};
			},
			// Deserialize
			[](Entity e, const json& j) {
				auto& s = e.AddComponent<SpriteComponent>();
				if (j.contains("color"))
					s.Color = JsonHelpers::DeserializeVec4(j["color"]);
				if (j.contains("size"))
					s.Size = JsonHelpers::DeserializeVec2(j["size"]);
				if (j.contains("texCoordMin"))
					s.TexCoordMin = JsonHelpers::DeserializeVec2(j["texCoordMin"]);
				if (j.contains("texCoordMax"))
					s.TexCoordMax = JsonHelpers::DeserializeVec2(j["texCoordMax"]);
				if (j.contains("zIndex"))
					s.ZIndex = j["zIndex"].get<float>();
			},
			// Copy
			[](Entity src, Entity dst) {
				if (!src.HasComponent<SpriteComponent>()) return;
				auto& s = src.GetComponent<SpriteComponent>();
				auto& d = dst.AddComponent<SpriteComponent>();
				d.Texture = s.Texture;  // Shared pointer, shallow copy is fine
				d.Color = s.Color;
				d.Size = s.Size;
				d.TexCoordMin = s.TexCoordMin;
				d.TexCoordMax = s.TexCoordMax;
				d.ZIndex = s.ZIndex;
			}
		);

	// ParticleComponent
	registry.Register<ParticleComponent>(
		"ParticleComponent",
		// Serialize
		[](Entity e) -> json {
			if (!e.HasComponent<ParticleComponent>()) return nullptr;
			auto& p = e.GetComponent<ParticleComponent>();
			json j;
			j["lifetime"] = p.Lifetime;
			j["age"] = p.Age;
			j["dead"] = p.Dead;
			j["startSize"] = JsonHelpers::SerializeVec2(p.StartSize);
			j["endSize"] = JsonHelpers::SerializeVec2(p.EndSize);
			j["startColor"] = JsonHelpers::SerializeVec4(p.StartColor);
			j["endColor"] = JsonHelpers::SerializeVec4(p.EndColor);
			j["startRotation"] = p.StartRotation;
			j["endRotation"] = p.EndRotation;
			j["fadeOut"] = p.FadeOut;
			j["scaleOverTime"] = p.ScaleOverTime;
			j["rotateOverTime"] = p.RotateOverTime;
			return j;
		},
		// Deserialize
		[](Entity e, const json& j) {
			auto& p = e.AddComponent<ParticleComponent>();
			if (j.contains("lifetime"))
				p.Lifetime = j["lifetime"].get<float>();
			if (j.contains("age"))
				p.Age = j["age"].get<float>();
			if (j.contains("dead"))
				p.Dead = j["dead"].get<bool>();
			if (j.contains("startSize"))
				p.StartSize = JsonHelpers::DeserializeVec2(j["startSize"]);
			if (j.contains("endSize"))
				p.EndSize = JsonHelpers::DeserializeVec2(j["endSize"]);
			if (j.contains("startColor"))
				p.StartColor = JsonHelpers::DeserializeVec4(j["startColor"]);
			if (j.contains("endColor"))
				p.EndColor = JsonHelpers::DeserializeVec4(j["endColor"]);
			if (j.contains("startRotation"))
				p.StartRotation = j["startRotation"].get<float>();
			if (j.contains("endRotation"))
				p.EndRotation = j["endRotation"].get<float>();
			if (j.contains("fadeOut"))
				p.FadeOut = j["fadeOut"].get<bool>();
			if (j.contains("scaleOverTime"))
				p.ScaleOverTime = j["scaleOverTime"].get<bool>();
			if (j.contains("rotateOverTime"))
				p.RotateOverTime = j["rotateOverTime"].get<bool>();
		},
		// Copy
		[](Entity src, Entity dst) {
			if (!src.HasComponent<ParticleComponent>()) return;
			auto& s = src.GetComponent<ParticleComponent>();
			auto& d = dst.AddComponent<ParticleComponent>();
			d.Lifetime = s.Lifetime;
			d.Age = s.Age;
			d.Dead = s.Dead;
			d.StartSize = s.StartSize;
			d.EndSize = s.EndSize;
			d.StartColor = s.StartColor;
			d.EndColor = s.EndColor;
			d.StartRotation = s.StartRotation;
			d.EndRotation = s.EndRotation;
			d.FadeOut = s.FadeOut;
			d.ScaleOverTime = s.ScaleOverTime;
			d.RotateOverTime = s.RotateOverTime;
		}
	);

	// ParticleEmitterComponent
	registry.Register<ParticleEmitterComponent>("particleEmitter",
		// Serialize
		[](Entity e) -> json {
			if (!e.HasComponent<ParticleEmitterComponent>()) return nullptr;
			auto& pe = e.GetComponent<ParticleEmitterComponent>();
			json j;
			j["enabled"] = pe.Enabled;
			j["emissionRate"] = pe.EmissionRate;
			j["burstMode"] = pe.BurstMode;
			j["burstCount"] = pe.BurstCount;
			
			// Emission shape
			std::string shapeStr = "point";
			switch (pe.Shape)
			{
				case EmissionShape::Point: shapeStr = "point"; break;
				case EmissionShape::Circle: shapeStr = "circle"; break;
				case EmissionShape::Box: shapeStr = "box"; break;
				case EmissionShape::Cone: shapeStr = "cone"; break;
			}
			j["shape"] = shapeStr;
			j["shapeSize"] = JsonHelpers::SerializeVec2(pe.ShapeSize);
			
			// Direction & speed
			j["direction"] = JsonHelpers::SerializeVec2(pe.Direction);
			j["directionSpread"] = pe.DirectionSpread;
			j["speed"] = pe.Speed;
			j["speedVariance"] = pe.SpeedVariance;
			
			// Particle properties
			j["lifetime"] = pe.Lifetime;
			j["lifetimeVariance"] = pe.LifetimeVariance;
			j["size"] = pe.Size;
			j["sizeVariance"] = pe.SizeVariance;
			j["startColor"] = JsonHelpers::SerializeVec4(pe.StartColor);
			j["colorVariance"] = JsonHelpers::SerializeVec4(pe.ColorVariance);
			
			// Visual effects
			j["fadeOut"] = pe.FadeOut;
			j["scaleOverTime"] = pe.ScaleOverTime;
			j["rotateOverTime"] = pe.RotateOverTime;
			j["endScale"] = pe.EndScale;
			j["rotationSpeed"] = pe.RotationSpeed;
			
			// Gravity
			j["gravity"] = JsonHelpers::SerializeVec2(pe.Gravity);
			
			return j;
		},
		// Deserialize
		[](Entity e, const json& j) {
			auto& pe = e.AddComponent<ParticleEmitterComponent>();
			
			if (j.contains("enabled"))
				pe.Enabled = j["enabled"].get<bool>();
			if (j.contains("emissionRate"))
				pe.EmissionRate = j["emissionRate"].get<float>();
			if (j.contains("burstMode"))
				pe.BurstMode = j["burstMode"].get<bool>();
			if (j.contains("burstCount"))
				pe.BurstCount = j["burstCount"].get<int>();
			
			// Emission shape
			if (j.contains("shape"))
			{
				std::string shapeStr = j["shape"].get<std::string>();
				if (shapeStr == "circle") pe.Shape = EmissionShape::Circle;
				else if (shapeStr == "box") pe.Shape = EmissionShape::Box;
				else if (shapeStr == "cone") pe.Shape = EmissionShape::Cone;
				else pe.Shape = EmissionShape::Point;
			}
			if (j.contains("shapeSize"))
				pe.ShapeSize = JsonHelpers::DeserializeVec2(j["shapeSize"]);
			
			// Direction & speed
			if (j.contains("direction"))
				pe.Direction = JsonHelpers::DeserializeVec2(j["direction"]);
			if (j.contains("directionSpread"))
				pe.DirectionSpread = j["directionSpread"].get<float>();
			if (j.contains("speed"))
				pe.Speed = j["speed"].get<float>();
			if (j.contains("speedVariance"))
				pe.SpeedVariance = j["speedVariance"].get<float>();
			
			// Particle properties
			if (j.contains("lifetime"))
				pe.Lifetime = j["lifetime"].get<float>();
			if (j.contains("lifetimeVariance"))
				pe.LifetimeVariance = j["lifetimeVariance"].get<float>();
			if (j.contains("size"))
				pe.Size = j["size"].get<float>();
			if (j.contains("sizeVariance"))
				pe.SizeVariance = j["sizeVariance"].get<float>();
			if (j.contains("startColor"))
				pe.StartColor = JsonHelpers::DeserializeVec4(j["startColor"]);
			if (j.contains("colorVariance"))
				pe.ColorVariance = JsonHelpers::DeserializeVec4(j["colorVariance"]);
			
			// Visual effects
			if (j.contains("fadeOut"))
				pe.FadeOut = j["fadeOut"].get<bool>();
			if (j.contains("scaleOverTime"))
				pe.ScaleOverTime = j["scaleOverTime"].get<bool>();
			if (j.contains("rotateOverTime"))
				pe.RotateOverTime = j["rotateOverTime"].get<bool>();
			if (j.contains("endScale"))
				pe.EndScale = j["endScale"].get<float>();
			if (j.contains("rotationSpeed"))
				pe.RotationSpeed = j["rotationSpeed"].get<float>();
			
			// Gravity
			if (j.contains("gravity"))
				pe.Gravity = JsonHelpers::DeserializeVec2(j["gravity"]);
		},
		// Copy
		[](Entity src, Entity dst) {
			if (!src.HasComponent<ParticleEmitterComponent>()) return;
			auto& s = src.GetComponent<ParticleEmitterComponent>();
			auto& d = dst.AddComponent<ParticleEmitterComponent>();
			
			d.Enabled = s.Enabled;
			d.EmissionRate = s.EmissionRate;
			d.BurstMode = s.BurstMode;
			d.BurstCount = s.BurstCount;
			d.BurstFired = false; // Reset burst state
			
			d.Shape = s.Shape;
			d.ShapeSize = s.ShapeSize;
			
			d.Direction = s.Direction;
			d.DirectionSpread = s.DirectionSpread;
			d.Speed = s.Speed;
			d.SpeedVariance = s.SpeedVariance;
			
			d.Lifetime = s.Lifetime;
			d.LifetimeVariance = s.LifetimeVariance;
			d.Size = s.Size;
			d.SizeVariance = s.SizeVariance;
			d.StartColor = s.StartColor;
			d.ColorVariance = s.ColorVariance;
			
			d.FadeOut = s.FadeOut;
			d.ScaleOverTime = s.ScaleOverTime;
			d.RotateOverTime = s.RotateOverTime;
			d.EndScale = s.EndScale;
			d.RotationSpeed = s.RotationSpeed;
			
			d.Gravity = s.Gravity;
		}
	);
}

} // namespace Pillar
