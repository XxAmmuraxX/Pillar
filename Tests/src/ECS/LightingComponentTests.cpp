#include <gtest/gtest.h>

#include "Pillar/ECS/ComponentRegistry.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/SceneSerializer.h"

#include "Pillar/ECS/Components/Rendering/Light2DComponent.h"
#include "Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h"

#include <filesystem>

using namespace Pillar;

TEST(LightingComponentTests, Light2DComponentDefaults)
{
	Light2DComponent light;
	EXPECT_EQ(light.Type, Light2DType::Point);
	EXPECT_FLOAT_EQ(light.Color.r, 1.0f);
	EXPECT_FLOAT_EQ(light.Color.g, 0.85f);
	EXPECT_FLOAT_EQ(light.Color.b, 0.6f);
	EXPECT_FLOAT_EQ(light.Intensity, 1.0f);
	EXPECT_FLOAT_EQ(light.Radius, 6.0f);
	EXPECT_TRUE(light.CastShadows);
	EXPECT_FLOAT_EQ(light.ShadowStrength, 1.0f);
	EXPECT_EQ(light.LayerMask, 0xFFFFFFFFu);
}

TEST(LightingComponentTests, ShadowCaster2DComponentDefaults)
{
	ShadowCaster2DComponent caster;
	EXPECT_TRUE(caster.Closed);
	EXPECT_FALSE(caster.TwoSided);
	EXPECT_TRUE(caster.Points.empty());
	EXPECT_EQ(caster.LayerMask, 0xFFFFFFFFu);
}

TEST(LightingComponentTests, BuiltinRegistry_HasLightingRegistrations)
{
	ComponentRegistry::Get().EnsureBuiltinsRegistered();

	EXPECT_NE(ComponentRegistry::Get().GetRegistration("light2d"), nullptr);
	EXPECT_NE(ComponentRegistry::Get().GetRegistration("shadowCaster2d"), nullptr);
}

TEST(LightingComponentTests, SceneSerializer_RoundTrip_PreservesLightingComponents)
{
	ComponentRegistry::Get().EnsureBuiltinsRegistered();

	auto testFilePath = std::filesystem::temp_directory_path() / "pillar_test_scene_lighting.json";
	if (std::filesystem::exists(testFilePath))
		std::filesystem::remove(testFilePath);

	// Create and save scene
	{
		Scene scene("LightingScene");
		Entity e = scene.CreateEntity("LightEntity");

		auto& light = e.AddComponent<Light2DComponent>();
		light.Type = Light2DType::Spot;
		light.Color = { 0.2f, 0.6f, 1.0f };
		light.Intensity = 2.5f;
		light.Radius = 12.0f;
		light.InnerAngleRadians = 0.3f;
		light.OuterAngleRadians = 0.7f;
		light.CastShadows = true;
		light.ShadowStrength = 0.75f;
		light.LayerMask = 0x00FF00FFu;

		auto& caster = e.AddComponent<ShadowCaster2DComponent>();
		caster.Closed = true;
		caster.TwoSided = false;
		caster.LayerMask = 0xF0F0F0F0u;
		caster.Points = {
			{ -1.0f, -1.0f },
			{  1.0f, -1.0f },
			{  0.0f,  1.0f }
		};

		SceneSerializer serializer(&scene);
		ASSERT_TRUE(serializer.Serialize(testFilePath.string()));
	}

	// Load scene
	Scene loaded;
	SceneSerializer serializer(&loaded);
	ASSERT_TRUE(serializer.Deserialize(testFilePath.string()));

	Entity e = loaded.FindEntityByName("LightEntity");
	ASSERT_TRUE(e);

	ASSERT_TRUE(e.HasComponent<Light2DComponent>());
	ASSERT_TRUE(e.HasComponent<ShadowCaster2DComponent>());

	{
		auto& light = e.GetComponent<Light2DComponent>();
		EXPECT_EQ(light.Type, Light2DType::Spot);
		EXPECT_FLOAT_EQ(light.Color.r, 0.2f);
		EXPECT_FLOAT_EQ(light.Color.g, 0.6f);
		EXPECT_FLOAT_EQ(light.Color.b, 1.0f);
		EXPECT_FLOAT_EQ(light.Intensity, 2.5f);
		EXPECT_FLOAT_EQ(light.Radius, 12.0f);
		EXPECT_FLOAT_EQ(light.InnerAngleRadians, 0.3f);
		EXPECT_FLOAT_EQ(light.OuterAngleRadians, 0.7f);
		EXPECT_TRUE(light.CastShadows);
		EXPECT_FLOAT_EQ(light.ShadowStrength, 0.75f);
		EXPECT_EQ(light.LayerMask, 0x00FF00FFu);
	}

	{
		auto& caster = e.GetComponent<ShadowCaster2DComponent>();
		EXPECT_TRUE(caster.Closed);
		EXPECT_FALSE(caster.TwoSided);
		EXPECT_EQ(caster.LayerMask, 0xF0F0F0F0u);
		ASSERT_EQ(caster.Points.size(), 3u);
		EXPECT_FLOAT_EQ(caster.Points[0].x, -1.0f);
		EXPECT_FLOAT_EQ(caster.Points[0].y, -1.0f);
		EXPECT_FLOAT_EQ(caster.Points[1].x, 1.0f);
		EXPECT_FLOAT_EQ(caster.Points[1].y, -1.0f);
		EXPECT_FLOAT_EQ(caster.Points[2].x, 0.0f);
		EXPECT_FLOAT_EQ(caster.Points[2].y, 1.0f);
	}

	// Cleanup
	if (std::filesystem::exists(testFilePath))
		std::filesystem::remove(testFilePath);
}
