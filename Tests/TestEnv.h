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
            float packetLoss = 0, float duplicates = 0, uint16_t mtu = MAXIMUM_MTU_SIZE)
        {
            unsigned short basePort = 50000;
            for (uint16_t i = 0; i < numPeers; i++)
            {
                peers[i] = static_cast<RakPeer*>(RakNet::RakPeerInterface::GetInstance());
                rnet::NetworkSimulatorSettings settings;
                settings.packetloss = packetLoss;
                settings.minExtraPing = minPing;
                settings.extraPingVariance = maxPing - minPing;
                settings.mtu = mtu;
                settings.duplicates = duplicates;
                peers[i]->ApplyNetworkSimulator(settings);
                peers[i]->SetMaximumIncomingConnections(CONNECTIONS_PER_SYSTEM);
                RakNet::SocketDescriptor socketDescriptor(basePort + i, 0);
                peers[i]->Startup(numPeers, &socketDescriptor, 1);
                peers[i]->SetOfflinePingResponse("Offline Ping Data", (int)strlen("Offline Ping Data") + 1);
            }

            for (uint16_t i = 0; i < numPeers; i++)
            {
                for (uint16_t j = 0; j < numPeers; j++)
                {
                    if (i != j)
                    {
                        peers[i]->Connect("127.0.0.1", basePort + j, 0, 0);
                    }
                }
            }
        }

        ~TestEnvironment()
        {
            for (uint16_t i = 0; i < NUM_PEERS; i++)
            {
                if (peers[i])
                {
                    peers[i]->Shutdown(500);
                    RakNet::RakPeerInterface::DestroyInstance(peers[i]);
                }
            }
        }

        void Stats()
        {
            char data[1024];
            for (uint16_t i = 0; i < NUM_PEERS; i++)
            {
                for (uint16_t j = 0; j < NUM_PEERS; j++)
                {
                    if (i != j && peers[i] && peers[j])
                    {
                        RakNetStatistics rss;
                        if (peers[i]->GetStatistics(0, &rss))
                        {
                            StatisticsToString(&rss, data, 1);
                        }
                    }
                }
            }
        }
    };
}
