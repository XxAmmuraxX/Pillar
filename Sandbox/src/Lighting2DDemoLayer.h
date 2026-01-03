#pragma once

#include "Pillar.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/Renderer/Lighting2D.h"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <vector>

class Lighting2DDemoLayer : public Pillar::Layer
{
public:
	Lighting2DDemoLayer()
		: Layer("Lighting2DDemoLayer"), m_CameraController(16.0f / 9.0f, false)
	{
		m_Light.Position = { -2.0f, 0.5f };
		m_Light.Color = { 1.0f, 0.55f, 0.25f };
		m_Light.Intensity = 3.0f;
		m_Light.Radius = 8.0f;
		m_Light.CastShadows = true;
		m_Light.ShadowStrength = 1.0f;
		m_Light.LayerMask = 0xFFFFFFFFu;
		m_Light.Type = Pillar::Light2DType::Spot;
		m_Light.Direction = { 1.0f, -0.15f };
		m_Light.InnerAngleRadians = glm::radians(18.0f);
		m_Light.OuterAngleRadians = glm::radians(35.0f);
	}

	void OnAttach() override
	{
		Layer::OnAttach();
		PIL_INFO("Lighting2DDemoLayer attached");

		// Dramatic baseline: very low ambient.
		m_Settings.AmbientColor = { 0.45f, 0.55f, 0.8f };
		m_Settings.AmbientIntensity = 0.06f;
		m_Settings.EnableShadows = true;

		BuildScene();
	}

	void OnDetach() override
	{
		Layer::OnDetach();
	}

	void OnUpdate(float dt) override
	{
		m_CameraController.OnUpdate(dt);

		UpdateLightInteraction(dt);

		Pillar::Renderer::SetClearColor({ 0.02f, 0.02f, 0.03f, 1.0f });
		Pillar::Renderer::Clear();

		auto& app = Pillar::Application::Get();
		uint32_t w = app.GetWindow().GetWidth();
		uint32_t h = app.GetWindow().GetHeight();

		Pillar::Lighting2D::BeginScene(m_CameraController.GetCamera(), w, h, m_Settings);

		DrawSceneSprites();
		SubmitShadowCasters();
		Pillar::Lighting2D::SubmitLight(m_Light);

		Pillar::Lighting2D::EndScene();
	}

	void OnEvent(Pillar::Event& event) override
	{
		m_CameraController.OnEvent(event);
	}

	void OnImGuiRender() override
	{
		ImGui::Begin("Lighting2D Demo");
		ImGui::Text("Interaction");
		ImGui::BulletText("LMB drag: move light");
		ImGui::BulletText("Arrow keys: nudge light");
		ImGui::BulletText("Hold Shift: faster nudge");

		ImGui::Separator();
		ImGui::Text("Ambient");
		ImGui::ColorEdit3("Ambient Color", &m_Settings.AmbientColor.x);
		ImGui::SliderFloat("Ambient Intensity", &m_Settings.AmbientIntensity, 0.0f, 0.5f);
		ImGui::Checkbox("Enable Shadows", &m_Settings.EnableShadows);

		ImGui::Separator();
		ImGui::Text("Light");
		int type = (m_Light.Type == Pillar::Light2DType::Spot) ? 1 : 0;
		if (ImGui::Combo("Type", &type, "Point\0Spot\0"))
			m_Light.Type = (type == 1) ? Pillar::Light2DType::Spot : Pillar::Light2DType::Point;

		ImGui::ColorEdit3("Color", &m_Light.Color.x);
		ImGui::SliderFloat("Intensity", &m_Light.Intensity, 0.0f, 8.0f);
		ImGui::SliderFloat("Radius", &m_Light.Radius, 0.5f, 25.0f);
		ImGui::Checkbox("Cast Shadows", &m_Light.CastShadows);
		ImGui::SliderFloat("Shadow Strength", &m_Light.ShadowStrength, 0.0f, 1.0f);

		if (m_Light.Type == Pillar::Light2DType::Spot)
		{
			float innerDeg = glm::degrees(m_Light.InnerAngleRadians);
			float outerDeg = glm::degrees(m_Light.OuterAngleRadians);
			if (ImGui::SliderFloat("Inner Angle (deg)", &innerDeg, 1.0f, 80.0f))
				m_Light.InnerAngleRadians = glm::radians(innerDeg);
			if (ImGui::SliderFloat("Outer Angle (deg)", &outerDeg, 1.0f, 85.0f))
				m_Light.OuterAngleRadians = glm::radians(outerDeg);

			ImGui::SliderFloat2("Direction", &m_Light.Direction.x, -1.0f, 1.0f);
		}

		ImGui::Separator();
		ImGui::Text("Light Position: (%.2f, %.2f)", m_Light.Position.x, m_Light.Position.y);

		if (ImGui::Button("Reset Scene"))
			BuildScene();

		ImGui::End();
	}

private:
	struct OccluderRect
	{
		glm::vec2 Center{ 0.0f };
		glm::vec2 HalfSize{ 0.5f };
		float Rotation = 0.0f;
		glm::vec4 Color{ 0.45f, 0.48f, 0.52f, 1.0f };
	};

	void BuildScene()
	{
		m_Walls.clear();

		// Large "room" blocks
		m_Walls.push_back({ { 0.0f, -2.2f }, { 14.0f, 0.6f }, 0.0f, { 0.30f, 0.30f, 0.33f, 1.0f } }); // floor
		m_Walls.push_back({ { 0.0f,  3.2f }, { 14.0f, 0.8f }, 0.0f, { 0.18f, 0.18f, 0.20f, 1.0f } }); // ceiling
		m_Walls.push_back({ { -8.5f, 0.5f }, { 0.8f, 6.0f }, 0.0f, { 0.20f, 0.20f, 0.23f, 1.0f } }); // left wall
		m_Walls.push_back({ {  8.5f, 0.5f }, { 0.8f, 6.0f }, 0.0f, { 0.20f, 0.20f, 0.23f, 1.0f } }); // right wall

		// Occluders inside the room (casts dramatic shadows)
		m_Walls.push_back({ { 0.8f, 0.2f }, { 1.2f, 0.35f }, glm::radians(8.0f), { 0.55f, 0.56f, 0.60f, 1.0f } });
		m_Walls.push_back({ { -2.3f, -0.4f }, { 0.9f, 0.5f }, glm::radians(-12.0f), { 0.50f, 0.52f, 0.56f, 1.0f } });
		m_Walls.push_back({ { 3.2f, -0.9f }, { 0.7f, 0.7f }, glm::radians(0.0f), { 0.42f, 0.44f, 0.47f, 1.0f } });
		m_Walls.push_back({ { 4.7f, 1.0f }, { 0.45f, 1.2f }, glm::radians(18.0f), { 0.46f, 0.47f, 0.50f, 1.0f } });

		// Put light in a slightly better place for the room
		m_Light.Position = { -4.5f, 1.1f };
		m_Light.Direction = { 1.0f, -0.05f };
		m_Light.Intensity = 3.2f;
		m_Light.Radius = 10.0f;
		m_Light.ShadowStrength = 1.0f;
	}

	glm::mat4 RectModel(const OccluderRect& r) const
	{
		return glm::translate(glm::mat4(1.0f), glm::vec3(r.Center, 0.0f))
			* glm::rotate(glm::mat4(1.0f), r.Rotation, glm::vec3(0, 0, 1))
			* glm::scale(glm::mat4(1.0f), glm::vec3(r.HalfSize * 2.0f, 1.0f));
	}

	void DrawSceneSprites()
	{
		// Background gradient-ish (two quads)
		Pillar::Renderer2DBackend::DrawQuad({ 0.0f, 0.0f, -0.5f }, { 32.0f, 18.0f }, { 0.18f, 0.20f, 0.28f, 1.0f });
		Pillar::Renderer2DBackend::DrawQuad({ 0.0f, -1.5f, -0.49f }, { 32.0f, 12.0f }, { 0.10f, 0.10f, 0.14f, 1.0f });

		for (const auto& wall : m_Walls)
		{
			glm::vec2 size = wall.HalfSize * 2.0f;
			Pillar::Renderer2DBackend::DrawRotatedQuad({ wall.Center.x, wall.Center.y, 0.0f }, size, wall.Rotation, wall.Color);
		}

		// Small "torch" marker
		Pillar::Renderer2DBackend::DrawQuad({ m_Light.Position.x, m_Light.Position.y, 0.05f }, { 0.25f, 0.25f }, { 1.0f, 0.75f, 0.25f, 1.0f });
	}

	void SubmitShadowCasters()
	{
		for (const auto& wall : m_Walls)
		{
			Pillar::ShadowCaster2DSubmit caster;
			caster.Closed = true;
			// These are closed, solid rectangles. Submitting them as two-sided causes
			// back-facing edges to cast, which can look like "shadows inside the wall"
			// when the light moves to the other side.
			caster.TwoSided = false;
			caster.LayerMask = 0xFFFFFFFFu;

			glm::mat4 model = RectModel(wall);
			std::vector<glm::vec2> local = {
				{ -0.5f, -0.5f },
				{  0.5f, -0.5f },
				{  0.5f,  0.5f },
				{ -0.5f,  0.5f },
			};

			caster.WorldPoints.reserve(local.size());
			for (const auto& p : local)
			{
				glm::vec4 w = model * glm::vec4(p.x, p.y, 0.0f, 1.0f);
				caster.WorldPoints.push_back({ w.x, w.y });
			}

			Pillar::Lighting2D::SubmitShadowCaster(caster);
		}
	}

	glm::vec2 MouseToWorld() const
	{
		auto [mx, my] = Pillar::Input::GetMousePosition();
		auto& window = Pillar::Application::Get().GetWindow();
		float w = (float)window.GetWidth();
		float h = (float)window.GetHeight();
		if (w <= 0.0f || h <= 0.0f)
			return { 0.0f, 0.0f };

		float x = (mx / w) * 2.0f - 1.0f;
		float y = 1.0f - (my / h) * 2.0f;

		glm::mat4 invVP = glm::inverse(m_CameraController.GetCamera().GetViewProjectionMatrix());
		glm::vec4 world = invVP * glm::vec4(x, y, 0.0f, 1.0f);
		if (std::abs(world.w) > 1e-6f)
			world /= world.w;
		return { world.x, world.y };
	}

	void UpdateLightInteraction(float dt)
	{
		const float baseSpeed = 4.0f;
		float speed = baseSpeed * ((Pillar::Input::IsKeyDown(PIL_KEY_LEFT_SHIFT) || Pillar::Input::IsKeyDown(PIL_KEY_RIGHT_SHIFT)) ? 3.0f : 1.0f);

		glm::vec2 delta{ 0.0f };
		if (Pillar::Input::IsKeyDown(PIL_KEY_LEFT)) delta.x -= 1.0f;
		if (Pillar::Input::IsKeyDown(PIL_KEY_RIGHT)) delta.x += 1.0f;
		if (Pillar::Input::IsKeyDown(PIL_KEY_DOWN)) delta.y -= 1.0f;
		if (Pillar::Input::IsKeyDown(PIL_KEY_UP)) delta.y += 1.0f;

		if (delta.x != 0.0f || delta.y != 0.0f)
		{
			float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
			delta /= (len > 1e-6f ? len : 1.0f);
			m_Light.Position += delta * speed * dt;
		}

		// Click-drag to move light in world space.
		if (Pillar::Input::IsMouseButtonDown(PIL_MOUSE_BUTTON_LEFT))
		{
			m_Light.Position = MouseToWorld();
		}
	}

private:
	Pillar::OrthographicCameraController m_CameraController;

	Pillar::Lighting2DSettings m_Settings;
	Pillar::Light2DSubmit m_Light;

	std::vector<OccluderRect> m_Walls;
};
