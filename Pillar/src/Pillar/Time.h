#pragma once

#include "Pillar/Core.h"

#include <cstdint>

namespace Pillar
{
	class PIL_API Time
	{
	public:
		static float GetTimeSeconds();
		static float GetDeltaTime();
		static float GetUnscaledDeltaTime();
		static uint64_t GetFrameCount();

		static void SetTimeScale(float timeScale);
		static float GetTimeScale();

		// Called once per frame by the application loop to advance timing state.
		static void Tick(float unscaledDeltaTime);
		static void Reset();

	private:
		static double GetNowSeconds();

		static double s_StartupSeconds;
		static float s_TimeScale;
		static float s_UnscaledDeltaTime;
		static float s_DeltaTime;
		static uint64_t s_FrameCount;
	};
}
