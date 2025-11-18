#pragma once

#include "Pillar.h"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

class ExampleLayer : public Pillar::Layer
{
public:
	ExampleLayer() 
		: Layer("ExampleLayer"), m_SquarePosition(0.0f, 0.0f) {}

	void OnAttach() override 
	{ 
		Layer::OnAttach();
		PIL_INFO("ExampleLayer attached - Renderer API: OpenGL");
		
		// Create a 2D square (2 triangles)
		float vertices[] = {
			-0.5f, -0.5f,  // Bottom-left
			 0.5f, -0.5f,  // Bottom-right
			 0.5f,  0.5f,  // Top-right
			-0.5f,  0.5f   // Top-left
		};

		uint32_t indices[] = {
			0, 1, 2,  // First triangle
			2, 3, 0   // Second triangle
		};

		// Create vertex array and buffers using factory methods
		m_VertexArray = Pillar::VertexArray::Create();
		
		m_VertexBuffer = Pillar::VertexBuffer::Create(vertices, sizeof(vertices));
		m_VertexArray->AddVertexBuffer(m_VertexBuffer);

		m_IndexBuffer = Pillar::IndexBuffer::Create(indices, 6);
		m_VertexArray->SetIndexBuffer(m_IndexBuffer);

		// Create shader for 2D rendering
		std::string vertexSrc = R"(
			#version 410 core
			
			layout(location = 0) in vec2 a_Position;
			
			uniform mat4 u_Transform;
			
			void main()
			{
				gl_Position = u_Transform * vec4(a_Position, 0.0, 1.0);
			}
		)";

		std::string fragmentSrc = R"(
			#version 410 core
			
			layout(location = 0) out vec4 color;
			
			uniform vec4 u_Color;
			
			void main()
			{
				color = u_Color;
			}
		)";

		m_Shader = Pillar::Shader::Create(vertexSrc, fragmentSrc);
		
		PIL_INFO("2D Square geometry and shader created!");
	}
	
	void OnDetach() override 
	{ 
		Layer::OnDetach();
		
		delete m_Shader;
		delete m_IndexBuffer;
		delete m_VertexBuffer;
		delete m_VertexArray;
	}
	
    void OnUpdate(float dt) override
    {
		// Update animation time
		m_Time += dt;
		
		// Animate background color
		float r = (sin(m_Time) + 1.0f) * 0.5f * 0.1f + 0.05f;
		float g = (cos(m_Time * 0.7f) + 1.0f) * 0.5f * 0.1f + 0.05f;
		float b = (sin(m_Time * 0.5f) + 1.0f) * 0.5f * 0.1f + 0.05f;
		
		Pillar::Renderer::SetClearColor({ r, g, b, 1.0f });
		Pillar::Renderer::Clear();

		// Update square position with keyboard
		float speed = 1.0f * dt;
		if (Pillar::Input::IsKeyPressed(PIL_KEY_W))
			m_SquarePosition.y += speed;
		if (Pillar::Input::IsKeyPressed(PIL_KEY_S))
			m_SquarePosition.y -= speed;
		if (Pillar::Input::IsKeyPressed(PIL_KEY_A))
			m_SquarePosition.x -= speed;
		if (Pillar::Input::IsKeyPressed(PIL_KEY_D))
			m_SquarePosition.x += speed;

		// Create 2D transformation matrix
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(m_SquarePosition, 0.0f));
		
		// Add rotation animation
		float rotation = m_Time * 50.0f; // Rotate 50 degrees per second
		transform = glm::rotate(transform, glm::radians(rotation), glm::vec3(0, 0, 1));
		
		// Add scale pulsing effect
		float scale = 0.5f + sin(m_Time * 2.0f) * 0.2f;
		transform = glm::scale(transform, glm::vec3(scale, scale, 1.0f));

		// Animate color
		float colorR = (sin(m_Time * 2.0f) + 1.0f) * 0.5f;
		float colorG = (cos(m_Time * 1.5f) + 1.0f) * 0.5f;
		float colorB = (sin(m_Time * 3.0f) + 1.0f) * 0.5f;

		// Render the 2D square
		Pillar::Renderer::BeginScene();
		
		m_Shader->Bind();
		m_Shader->SetMat4("u_Transform", transform);
		m_Shader->SetFloat4("u_Color", { colorR, colorG, colorB, 1.0f });
		
		Pillar::Renderer::Submit(m_VertexArray);
		
		Pillar::Renderer::EndScene();
    }
    
    void OnEvent(Pillar::Event& event) override
    {
        if (event.GetEventType() == Pillar::EventType::KeyPressed)
        {
			Pillar::KeyPressedEvent& keyEvent = static_cast<Pillar::KeyPressedEvent&>(event);
			if (keyEvent.GetRepeatCount() == 0)  // Only log on first press
			{
				PIL_INFO("Key Pressed: {0}", keyEvent.GetKeyCode());
			}
		}
	}
	
	void OnImGuiRender() override 
	{

	}

private:
	Pillar::Shader* m_Shader = nullptr;
	Pillar::VertexArray* m_VertexArray = nullptr;
	Pillar::VertexBuffer* m_VertexBuffer = nullptr;
	Pillar::IndexBuffer* m_IndexBuffer = nullptr;
	
	glm::vec2 m_SquarePosition;
	float m_Time = 0.0f;
};
