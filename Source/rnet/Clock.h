#pragma once

#include "rnet/RNetTime.h"
#include <chrono>

namespace rnet
{
	class SteadyClock
	{
	public:
		static TimeUS GetTimeUS()
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count();
		}

		static TimeMS GetTimeMS()
		{
			return static_cast<TimeMS>(std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count() + 0.5);
		}
	};
}