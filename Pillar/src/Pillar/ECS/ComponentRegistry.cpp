#include "ComponentRegistry.h"

namespace Pillar {

	ComponentRegistry& ComponentRegistry::Get()
	{
		static ComponentRegistry instance;
		return instance;
	}

	void ComponentRegistry::EnsureBuiltinsRegistered()
	{
		if (!m_BuiltinsRegistered)
		{
			RegisterBuiltinComponents();
			m_BuiltinsRegistered = true;
		}
	}

} // namespace Pillar
