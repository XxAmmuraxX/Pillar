#pragma once

namespace Pillar {

	struct XPGemComponent
	{
		int XPValue = 1;
		float AttractionRadius = 3.0f;  // Distance at which gem moves toward player
		float MoveSpeed = 10.0f;        // Speed when attracted

		bool IsAttracted = false;       // State flag

		XPGemComponent() = default;
		XPGemComponent(const XPGemComponent&) = default;
		XPGemComponent(int xp) : XPValue(xp) {}
	};

} // namespace Pillar
