#pragma once

#include "RakPeer.h"
#include "RakNetStatistics.h"
#include "GetTime.h"
#include "MessageIdentifiers.h"
#include "RakSleep.h"

namespace RakNet
{
    static constexpr unsigned int NUM_PEERS = 10;
    static constexpr uint16_t CONNECTIONS_PER_SYSTEM = 4;

    struct TestEnvironment
    {
        RakPeer *peers[NUM_PEERS] = { nullptr };

		void Setup(size_t numPeers, unsigned short minPing = 0, unsigned short maxPing = 0,
			float packetLoss = 0, float duplicates = 0, uint16_t mtu = MAXIMUM_MTU_SIZE);

		~TestEnvironment();

		void Stats();
    };
}
