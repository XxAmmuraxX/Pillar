#include <gtest/gtest.h>
#include "Pillar/ECS/ComponentRegistry.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include <nlohmann/json.hpp>

using namespace Pillar;
using json = nlohmann::json;

// ============================================================================
// ComponentRegistry Tests
// ============================================================================

// Custom test component for registration tests
struct TestCustomComponent
{
	int Health = 100;
	std::string Name = "TestEntity";
	float Speed = 5.0f;
};

class ComponentRegistryTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Ensure built-ins are registered
		ComponentRegistry::Get().EnsureBuiltinsRegistered();
	}
};

TEST_F(ComponentRegistryTests, Get_ReturnsSingleton)
{
	ComponentRegistry& reg1 = ComponentRegistry::Get();
	ComponentRegistry& reg2 = ComponentRegistry::Get();

	EXPECT_EQ(&reg1, &reg2);
}

TEST_F(ComponentRegistryTests, EnsureBuiltinsRegistered_RegistersStandardComponents)
{
	ComponentRegistry::Get().EnsureBuiltinsRegistered();

	// Check for common built-in components
	// Note: TagComponent and UUIDComponent are not registered for serialization
	// because they are handled specially by the scene serializer
	EXPECT_TRUE(ComponentRegistry::Get().IsRegistered<TransformComponent>());
	EXPECT_TRUE(ComponentRegistry::Get().IsRegistered<VelocityComponent>());
}

TEST_F(ComponentRegistryTests, Register_AddsComponent)
{
	ComponentRegistry::Get().Register<TestCustomComponent>(
		"testCustom",
		[](Entity e) -> json {
			if (!e.HasComponent<TestCustomComponent>()) return nullptr;
			auto& comp = e.GetComponent<TestCustomComponent>();
			return json{
				{ "health", comp.Health },
				{ "name", comp.Name },
				{ "speed", comp.Speed }
			};
		},
		[](Entity e, const json& j) {
			auto& comp = e.AddComponent<TestCustomComponent>();
			comp.Health = j.value("health", 100);
			comp.Name = j.value("name", "Unknown");
			comp.Speed = j.value("speed", 5.0f);
		},
		[](Entity src, Entity dst) {
			if (!src.HasComponent<TestCustomComponent>()) return;
			auto& s = src.GetComponent<TestCustomComponent>();
			auto& d = dst.AddComponent<TestCustomComponent>();
			d.Health = s.Health;
			d.Name = s.Name;
			d.Speed = s.Speed;
		}
	);

	EXPECT_TRUE(ComponentRegistry::Get().IsRegistered<TestCustomComponent>());
}

TEST_F(ComponentRegistryTests, GetJsonKey_ReturnsCorrectKey)
{
	// Transform should be registered as "transform"
	const std::string& key = ComponentRegistry::Get().GetJsonKey<TransformComponent>();
	EXPECT_EQ(key, "transform");
}

TEST_F(ComponentRegistryTests, GetJsonKey_ReturnsEmptyForUnregistered)
{
	struct UnregisteredComponent {};
	const std::string& key = ComponentRegistry::Get().GetJsonKey<UnregisteredComponent>();
	EXPECT_TRUE(key.empty());
}

TEST_F(ComponentRegistryTests, GetRegistration_ReturnsCorrectRegistration)
{
	const ComponentRegistration* reg = ComponentRegistry::Get().GetRegistration("transform");

	EXPECT_NE(reg, nullptr);
	EXPECT_EQ(reg->Name, "transform");
	EXPECT_TRUE(reg->Serialize != nullptr);
	EXPECT_TRUE(reg->Deserialize != nullptr);
}

TEST_F(ComponentRegistryTests, GetRegistration_ReturnsNullForUnknown)
{
	const ComponentRegistration* reg = ComponentRegistry::Get().GetRegistration("unknown_component");

	EXPECT_EQ(reg, nullptr);
}

TEST_F(ComponentRegistryTests, GetRegistrations_ReturnsAllRegistered)
{
	const auto& registrations = ComponentRegistry::Get().GetRegistrations();

	// Should have multiple registrations
	EXPECT_GT(registrations.size(), 0);

	// Should contain transform
	EXPECT_TRUE(registrations.find("transform") != registrations.end());
}

TEST_F(ComponentRegistryTests, GetRegistrationCount_ReturnsCorrectCount)
{
	size_t count = ComponentRegistry::Get().GetRegistrationCount();

	// Should have multiple built-in components registered
	EXPECT_GT(count, 5);
}

TEST_F(ComponentRegistryTests, Serialize_TransformComponent)
{
	Scene scene;
	Entity entity = scene.CreateEntity();
	auto& transform = entity.GetComponent<TransformComponent>();
	transform.Position = glm::vec2(10.0f, 20.0f);
	transform.Rotation = 1.57f;
	transform.Scale = glm::vec2(2.0f, 3.0f);

	const ComponentRegistration* reg = ComponentRegistry::Get().GetRegistration("transform");
	ASSERT_NE(reg, nullptr);

	json serialized = reg->Serialize(entity);

	EXPECT_FALSE(serialized.is_null());
	// Position/scale are serialized as JSON arrays [x, y]
	EXPECT_FLOAT_EQ(serialized["position"][0].get<float>(), 10.0f);
	EXPECT_FLOAT_EQ(serialized["position"][1].get<float>(), 20.0f);
	EXPECT_FLOAT_EQ(serialized["rotation"].get<float>(), 1.57f);
	EXPECT_FLOAT_EQ(serialized["scale"][0].get<float>(), 2.0f);
	EXPECT_FLOAT_EQ(serialized["scale"][1].get<float>(), 3.0f);
}

TEST_F(ComponentRegistryTests, Deserialize_TransformComponent)
{
	Scene scene;
	Entity entity = scene.CreateEntity();

	// Remove existing transform to test deserialization
	auto& existingTransform = entity.GetComponent<TransformComponent>();
	existingTransform.Position = glm::vec2(0.0f);
	existingTransform.Rotation = 0.0f;
	existingTransform.Scale = glm::vec2(1.0f);

	// Position/scale are expected as JSON arrays [x, y]
	json data = {
		{ "position", json::array({ 50.0f, 100.0f }) },
		{ "rotation", 3.14f },
		{ "scale", json::array({ 0.5f, 0.5f }) }
	};

	const ComponentRegistration* reg = ComponentRegistry::Get().GetRegistration("transform");
	ASSERT_NE(reg, nullptr);

	reg->Deserialize(entity, data);

	auto& transform = entity.GetComponent<TransformComponent>();
	EXPECT_FLOAT_EQ(transform.Position.x, 50.0f);
	EXPECT_FLOAT_EQ(transform.Position.y, 100.0f);
	EXPECT_FLOAT_EQ(transform.Rotation, 3.14f);
	EXPECT_FLOAT_EQ(transform.Scale.x, 0.5f);
	EXPECT_FLOAT_EQ(transform.Scale.y, 0.5f);
}

TEST_F(ComponentRegistryTests, IsRegistered_ReturnsFalseForUnregistered)
{
	struct CompletelyNewComponent {};

	EXPECT_FALSE(ComponentRegistry::Get().IsRegistered<CompletelyNewComponent>());
}

TEST_F(ComponentRegistryTests, CopyFunction_CopiesComponent)
{
	Scene scene;
	Entity src = scene.CreateEntity("Source");
	Entity dst = scene.CreateEntity("Destination");

	auto& srcTransform = src.GetComponent<TransformComponent>();
	srcTransform.Position = glm::vec2(100.0f, 200.0f);
	srcTransform.Rotation = 2.0f;
	srcTransform.Scale = glm::vec2(5.0f, 5.0f);

	const ComponentRegistration* reg = ComponentRegistry::Get().GetRegistration("transform");
	ASSERT_NE(reg, nullptr);
	ASSERT_NE(reg->Copy, nullptr);

	reg->Copy(src, dst);

	auto& dstTransform = dst.GetComponent<TransformComponent>();
	EXPECT_FLOAT_EQ(dstTransform.Position.x, 100.0f);
	EXPECT_FLOAT_EQ(dstTransform.Position.y, 200.0f);
	EXPECT_FLOAT_EQ(dstTransform.Rotation, 2.0f);
	EXPECT_FLOAT_EQ(dstTransform.Scale.x, 5.0f);
	EXPECT_FLOAT_EQ(dstTransform.Scale.y, 5.0f);
}
