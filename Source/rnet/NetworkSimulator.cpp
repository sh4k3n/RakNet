#include "NetworkSimulator.h"

#ifdef RAKNET_NETWORK_SIMULATOR
#include "Rand.h"
#include "RakNetSocket2.h"
#include "RemoteSystem.h"
#include "GetTime.h"

#include <memory>
#include "rnet/SocketLayer.h"

void rnet::NetworkSimulator::Configure(const NetworkSimulatorSettings& settings)
{
    myDefaultSettings = settings;
}

bool rnet::NetworkSimulator::IsActive() const
{
    return myDefaultSettings.duplicates > 0.0f || myDefaultSettings.packetloss > 0.0f ||
        myDefaultSettings.extraPingVariance > 0 ||
        myDefaultSettings.minExtraPing > 0 ||
        myDefaultSettings.mtu != MAXIMUM_MTU_SIZE;
}

void rnet::NetworkSimulator::Send(RakNet::BitStream& bitStream, RakNet::RakNetSocket2& socket, const RakNet::SystemAddress& systemAddress)
{
    auto& settings = myDefaultSettings;
    uint16_t length = uint16_t(bitStream.GetNumberOfBytesUsed());
    if (length > settings.mtu)
    {
        return;
    }

    const size_t numCopies = settings.duplicates > 0.0f && frandomMT() < settings.duplicates ? 2 : 1;
    for (size_t i = 0; i < numCopies; ++i)
    {        
        if (settings.packetloss > 0.0f)
        {
            if (frandomMT() < settings.packetloss)
            {
                continue;
            }
        }

        if (settings.minExtraPing > 0 || settings.extraPingVariance > 0)
        {
            uint16_t delay = settings.minExtraPing +
                (settings.extraPingVariance ? (randomMT() % settings.extraPingVariance) : 0);
            if (delay > 0)
            {
                auto delayedSend = std::make_unique<DelayedSend>(socket, systemAddress);
                RakAssert(length <= MAXIMUM_MTU_SIZE);
                memcpy(delayedSend->data, bitStream.GetData(), length);
                delayedSend->length = length;
                delayedSend->sendTime = RakNet::GetTime() + delay;
                myDelayList.push(std::move(delayedSend));
                continue;
            }
        }

        socket_layer::detail::SendTo(socket, bitStream, systemAddress);
    }
}

void rnet::NetworkSimulator::Update(RakNet::Time now)
{
    while (!myDelayList.empty() &&
        static_cast<int>(now - myDelayList.top()->sendTime) >= 0)
    {
        auto& delayedSend = myDelayList.top();
        RakNet::RNS2_SendParameters bsp;
        bsp.data = (char*)delayedSend->data;
        bsp.length = delayedSend->length;
        bsp.systemAddress = delayedSend->systemAddress;
        delayedSend->socket.Send(&bsp, _FILE_AND_LINE_);
        myDelayList.pop();
    }
}
#endif