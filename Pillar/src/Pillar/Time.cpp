#include "Pillar/Time.h"

#include <chrono>

namespace Pillar
{
	double Time::s_StartupSeconds = Time::GetNowSeconds();
	float Time::s_TimeScale = 1.0f;
	float Time::s_UnscaledDeltaTime = 0.0f;
	float Time::s_DeltaTime = 0.0f;
	uint64_t Time::s_FrameCount = 0;

	double Time::GetNowSeconds()
	{
		using clock = std::chrono::steady_clock;
		return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
	}

	float Time::GetTimeSeconds()
	{
		return static_cast<float>(GetNowSeconds() - s_StartupSeconds);
	}

	float Time::GetDeltaTime()
	{
		return s_DeltaTime;
	}

	float Time::GetUnscaledDeltaTime()
	{
		return s_UnscaledDeltaTime;
	}

	uint64_t Time::GetFrameCount()
	{
		return s_FrameCount;
	}

	void Time::SetTimeScale(float timeScale)
	{
		s_TimeScale = timeScale;
		s_DeltaTime = s_UnscaledDeltaTime * s_TimeScale;
	}

	float Time::GetTimeScale()
	{
		return s_TimeScale;
	}

	void Time::Tick(float unscaledDeltaTime)
	{
		s_UnscaledDeltaTime = unscaledDeltaTime;
		s_DeltaTime = s_UnscaledDeltaTime * s_TimeScale;
		++s_FrameCount;
	}

	void Time::Reset()
	{
		s_StartupSeconds = GetNowSeconds();
		s_UnscaledDeltaTime = 0.0f;
		s_DeltaTime = 0.0f;
		s_FrameCount = 0;
	}
}
