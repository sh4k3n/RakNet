#pragma once

#include <cstdint>
#include "RakNetDefines.h"
#include "MTUSize.h"
#include "RakNetTypes.h"

namespace rnet
{
    struct NetworkSimulatorSettings
    {
        double packetloss = 0.0; // Chance to lose a packet. Ranges from 0 to 1.
        double duplicates = 0.0; // Chance to duplicate a packet. Ranges from 0 to 1.
        uint16_t mtu = MAXIMUM_MTU_SIZE; // Maximum MTU        
        uint16_t minExtraPing = 0; //  The minimum time to delay sends.
        uint16_t extraPingVariance = 0; //  Jitter
    };
}
