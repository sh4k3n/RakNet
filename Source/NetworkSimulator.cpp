#include "NetworkSimulator.h"
#include "Rand.h"
#include "RakNetSocket2.h"
#include "RemoteSystem.h"

void RakNet::NetworkSimulator::Send(BitStream& bitStream, RakNet::TimeMS now, RemoteSystem& remoteSystem)
{
    uint16_t length = uint16_t(bitStream.GetNumberOfBytesUsed());
    if (myPacketloss > 0.0)
    {
        if (frandomMT() < myPacketloss)
        {
            return;
        }
    }
    if (length > myMtu)
    {
        return;
    }

    if (myMinExtraPing > 0 || myExtraPingVariance > 0)
    {
        uint16_t delay = myMinExtraPing +
            (myExtraPingVariance ? (randomMT() % myExtraPingVariance) : 0);
        if (delay > 0)
        {
            auto delayedSend = std::make_unique<DelayedSend>(remoteSystem);
            memcpy(delayedSend->data, bitStream.GetData(), length);
            delayedSend->length = length;
            delayedSend->sendTime = now + delay;
            myDelayList.push(std::move(delayedSend));
            return;
        }
    }
    RNS2_SendParameters bsp;
    bsp.data = (char*)bitStream.GetData();
    bsp.length = length;
    bsp.systemAddress = remoteSystem.systemAddress;
    remoteSystem.rakNetSocket->Send(&bsp, _FILE_AND_LINE_);
}

void RakNet::NetworkSimulator::Update(RakNet::TimeMS now)
{
    while (!myDelayList.empty() &&
        static_cast<int>(now - myDelayList.top()->sendTime) >= 0)
    {
        auto& delayedSend = myDelayList.top();
        RNS2_SendParameters bsp;
        bsp.data = (char*)delayedSend->data;
        bsp.length = delayedSend->length;
        bsp.systemAddress = delayedSend->remoteSystem.systemAddress;
        delayedSend->remoteSystem.rakNetSocket->Send(&bsp, _FILE_AND_LINE_);
        myDelayList.pop();
    }
}