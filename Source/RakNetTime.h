#pragma once
/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "NativeTypes.h"
#include "RakNetDefines.h"
#include <rnet/RNetTime.h>

namespace RakNet {

	using TimeMS = rnet::TimeMS;
	using Time = rnet::Time;
	using TimeUS = rnet::TimeUS;	
	using TimeDeltaMS = rnet::TimeDeltaMS;
	using TimeDeltaUS = rnet::TimeDeltaUS;

	constexpr TimeDeltaMS DeltaTime(TimeMS a, TimeMS b)
	{
		return rnet::DeltaTime(a, b);
	}

	constexpr bool IsTimeInRange(TimeMS a, TimeMS b, TimeDeltaMS range)
	{
		return rnet::IsTimeInRange(a, b, range);
	}

	constexpr TimeDeltaUS DeltaTime(TimeUS a, TimeUS b)
	{
		return rnet::DeltaTime(a, b);
	}

	template<typename TTime = TimeMS>
	/*constexpr*/ TTime TimeSince(TTime a, TTime b)
	{
		return rnet::TimeSince(a, b);
	}

} // namespace RakNet
