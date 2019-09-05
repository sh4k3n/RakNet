#pragma once

#include "RakNetTime.h"
#include "BitStream.h"
#include "MTUSize.h"

#include <queue>
#include <memory>
#include <deque>

namespace RakNet
{
    struct RemoteSystem;
    class NetworkSimulator
    {
    public:
        NetworkSimulator() {}

        void Send(BitStream& bitStream, RakNet::TimeMS now, RemoteSystem& remoteSystem);

        void Update(RakNet::TimeMS now);

        void Configure(float packetLoss, uint16_t minExtraPing, uint16_t extraPingVariance)
        {
            myPacketloss = packetLoss;
            myMinExtraPing = minExtraPing;
            myExtraPingVariance = extraPingVariance;
        }

    private:
        struct DelayedSend
        {
            DelayedSend(RemoteSystem& rs) : remoteSystem(rs) {}
            RemoteSystem& remoteSystem;
            uint8_t data[MAXIMUM_MTU_SIZE];
            uint16_t length;
            RakNet::TimeMS sendTime;
            // unsigned int extraSocketOptions;
        };

        struct EarlierSendTime
        {
            bool operator()(std::unique_ptr<DelayedSend>& a, std::unique_ptr<DelayedSend>& b)
            {
                return static_cast<int>(a->sendTime - b->sendTime) > 0;
            }
        };

        std::priority_queue<std::unique_ptr<DelayedSend>, std::deque<std::unique_ptr<DelayedSend>>, 
            EarlierSendTime> myDelayList;
        double myPacketloss = 0;
        uint16_t myMtu = 0xFFFF;
        uint16_t myMinExtraPing = 0;
        uint16_t myExtraPingVariance = 0;
    };
}