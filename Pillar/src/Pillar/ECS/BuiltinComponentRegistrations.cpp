#include "ComponentRegistry.h"
#include "Components/Core/TagComponent.h"
#include "Components/Core/TransformComponent.h"
#include "Components/Core/UUIDComponent.h"
#include "Components/Physics/RigidbodyComponent.h"
#include "Components/Physics/ColliderComponent.h"
#include "Components/Physics/VelocityComponent.h"
#include "Components/Gameplay/XPGemComponent.h"
#include "Components/Gameplay/BulletComponent.h"
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
	}

} // namespace Pillar
