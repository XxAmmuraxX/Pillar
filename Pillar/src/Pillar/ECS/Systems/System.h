#pragma once

namespace Pillar {

	class Scene; // Forward declaration

	class System
	{
	public:
		virtual ~System() = default;

		virtual void OnAttach(Scene* scene) { m_Scene = scene; }
		virtual void OnDetach() { m_Scene = nullptr; }
		virtual void OnUpdate(float deltaTime) = 0;

	protected:
		Scene* m_Scene = nullptr;
	};

} // namespace Pillar
