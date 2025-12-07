#pragma once

#include <cstdint>

namespace Pillar {

	// Forward declaration
	class Entity;

	struct HierarchyComponent
	{
		uint64_t ParentUUID = 0;
		
		HierarchyComponent() = default;
		HierarchyComponent(const HierarchyComponent&) = default;
		HierarchyComponent(uint64_t parent)
			: ParentUUID(parent) {}
	};

} // namespace Pillar
