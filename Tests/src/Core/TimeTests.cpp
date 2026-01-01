#include <gtest/gtest.h>
#include "Pillar/Time.h"
#include <thread>
#include <chrono>

using namespace Pillar;

class TimeTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		Time::Reset();
		Time::SetTimeScale(1.0f);
	}
};

TEST_F(TimeTest, DefaultsAfterReset)
{
	EXPECT_EQ(Time::GetFrameCount(), 0u);
	EXPECT_FLOAT_EQ(Time::GetDeltaTime(), 0.0f);
	EXPECT_FLOAT_EQ(Time::GetUnscaledDeltaTime(), 0.0f);
}

TEST_F(TimeTest, TickAdvancesFrameAndDelta)
{
	Time::Tick(0.016f);
	EXPECT_EQ(Time::GetFrameCount(), 1u);
	EXPECT_NEAR(Time::GetUnscaledDeltaTime(), 0.016f, 1e-6f);
	EXPECT_NEAR(Time::GetDeltaTime(), 0.016f, 1e-6f);
}

TEST_F(TimeTest, TimeScaleAffectsDelta)
{
	Time::SetTimeScale(0.5f);
	Time::Tick(0.020f);
	EXPECT_NEAR(Time::GetUnscaledDeltaTime(), 0.020f, 1e-6f);
	EXPECT_NEAR(Time::GetDeltaTime(), 0.010f, 1e-6f);
}

TEST_F(TimeTest, SetTimeScaleUpdatesDeltaImmediately)
{
	Time::Tick(0.010f);
	Time::SetTimeScale(2.0f);
	EXPECT_NEAR(Time::GetDeltaTime(), 0.020f, 1e-6f);
}

TEST_F(TimeTest, GetTimeSecondsMonotonic)
{
	auto t0 = Time::GetTimeSeconds();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	auto t1 = Time::GetTimeSeconds();
	EXPECT_GE(t1, t0);
}
