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

namespace RakNet {

// Define __GET_TIME_64BIT if you want to use large types for GetTime (takes more bandwidth when you transmit time though!)
// You would want to do this if your system is going to run long enough to overflow the millisecond counter (over a month)
#if __GET_TIME_64BIT==1
typedef uint64_t Time;
typedef uint32_t TimeMS;
typedef uint64_t TimeUS;
#else
typedef uint32_t Time;
typedef uint32_t TimeMS;
typedef uint64_t TimeUS;
#endif

#if RAKNET_ARQ == RAKNET_ARQ_KCP
#define CC_TIME_TYPE_BYTES 4
#else
/// Set to 4 if you are using the iPod Touch TG. See http://www.jenkinssoftware.com/forum/index.php?topic=2717.0
#define CC_TIME_TYPE_BYTES 8
#endif

#if CC_TIME_TYPE_BYTES==8
typedef RakNet::TimeUS CCTimeType;
#else
typedef RakNet::TimeMS CCTimeType;
#endif

} // namespace RakNet

