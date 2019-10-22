#pragma once

#include <rnet/RNetConfig.h>

#ifdef _RETAIL
#define RNetAssert(__expected, ...) __assume(__expected)
#else
#define RNetAssert(__expected, ...) assert(__expected)
#endif

#ifdef _RETAIL
#define RNetAbnormal(__expected)
#else
#define RNetAbnormal(__expected) assert(__expected)
#endif
