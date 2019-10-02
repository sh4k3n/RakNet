#pragma once
#include <rnet/NetworkSimulatorSettings.h>
#include "RakNetTime.h"
#include "BitStream.h"

#include <queue>
#include <memory>
#include <deque>
#include <unordered_map>

namespace RakNet
{
    class RakNetSocket2;
}

namespace rnet
{
    class NetworkSimulator
    {
    public:
        NetworkSimulator() {}
#ifdef RAKNET_NETWORK_SIMULATOR
        NetworkSimulator(const NetworkSimulator& other) = delete;

        bool IsActive() const;

        void Send(RakNet::BitStream& bitStream, RakNet::RakNetSocket2& socket, const RakNet::SystemAddress& address);

        void Update(RakNet::Time now);

        void Configure(const NetworkSimulatorSettings& other);     

        const NetworkSimulatorSettings& Settings() const { return myDefaultSettings; }

    private:
        struct DelayedSend
        {
            DelayedSend(RakNet::RakNetSocket2& socket, const RakNet::SystemAddress& address)
                : socket(socket), systemAddress(address) {}
            RakNet::RakNetSocket2& socket;
            RakNet::SystemAddress systemAddress;
            uint8_t data[MAXIMUM_MTU_SIZE];
            uint16_t length;
            RakNet::Time sendTime;
        };

        struct EarlierSendTime
        {
            bool operator()(std::unique_ptr<DelayedSend>& a, std::unique_ptr<DelayedSend>& b)
            {
                return static_cast<int>(a->sendTime - b->sendTime) > 0;
            }
        };

        std::priority_queue<std::unique_ptr<DelayedSend>,
            std::deque<std::unique_ptr<DelayedSend>>,
            EarlierSendTime> myDelayList;
        // std::vector<RakNet::SystemAddress, NetworkSimulatorSettings> mySystemSettings;
        NetworkSimulatorSettings myDefaultSettings;
#endif
    };
}