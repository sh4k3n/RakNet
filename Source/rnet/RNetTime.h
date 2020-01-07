#pragma once

#include <rnet/RNet.h>
#include <limits>
#include <type_traits>

// TODO: Get rid of these
#undef min
#undef max

namespace rnet
{
	enum class TimeSyncState : uint8_t
	{
		NoSync,
		InitialSync,
		Sync,
		Precise
	};

	// Define __GET_TIME_64BIT if you want to use large types for GetTime (takes more bandwidth when you transmit time though!)
	// You would want to do this if your system is going to run long enough to overflow the millisecond counter (over a month)
#if __GET_TIME_64BIT==1
	// NOTE: Check does RakNet work correcty if type of Time and TimMS are different
	using Time = uint64_t;
	using TimeMS = uint32_t ;
	using TimeDeltaMS = int32_t;
	using TimeUS = uint64_t;
#else
	using Time = uint32_t;
	using TimeMS = uint32_t;
	using TimeUS = uint32_t;
#endif

	using TimeDeltaMS = std::make_signed<TimeMS>::type;
	using TimeDeltaUS = std::make_signed<TimeUS>::type;

	constexpr TimeDeltaMS DeltaTime(TimeMS a, TimeMS b) { return static_cast<TimeDeltaMS>(a - b); }

	constexpr bool IsTimeInRange(TimeMS a, TimeMS b, TimeDeltaMS range)
	{
		auto delta = static_cast<TimeDeltaMS>(a - b);
		return delta < 0 ? ((-delta) <= range) : (delta <= range);
	}

	constexpr TimeDeltaUS DeltaTime(TimeUS a, TimeUS b)
	{
		auto delta = static_cast<TimeDeltaUS>(a - b);
		RNetAssert( // Check values are in safe range for compare
			delta <= std::numeric_limits<TimeDeltaUS>::max() / 2 &&
			delta >= std::numeric_limits<TimeDeltaUS>::min() / 2, "Too large delta");
		return delta;
	}

	template<typename TTime = TimeMS>
	constexpr TTime TimeSince(TTime a, TTime b)
	{
		auto delta = DeltaTime(a, b);
		RNetAssert(delta >= 0, "Negative delta");
		return static_cast<TTime>(delta);
	}

	static constexpr TimeMS ConnectFloodTimeout = 100;

	static constexpr TimeMS FailureConditionTimeout = 10 * 1000;

	static constexpr TimeMS DefaultTimeout = 10 * 1000;

	using PingTime = uint16_t;

	static constexpr PingTime DefaultPingFrequency = 500;

	static constexpr PingTime OccasionalPingFrequency = 30 * 1000;
}

