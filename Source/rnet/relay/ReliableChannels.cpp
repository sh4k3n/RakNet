#include <rnet/relay/ReliableChannels.h>

#if RAKNET_ARQ == RAKNET_ARQ_KCP
#include <kcp/ikcp.h>
#include <utility>

#include "remotesystem.h"
#include "BitStream.h"
#include <rnet/SocketLayer.h>

namespace rnet
{
    int SendKCPPacket(const char *buf, int len, ikcpcb *kcp, void *user)
    {
        RemoteSystem& remoteSystem = *static_cast<RemoteSystem*>(user);
        RakNet::BitStream bitStream((unsigned char*)buf, len, false);
        rnet::socket_layer::SendTo(*remoteSystem.rakNetSocket, bitStream, remoteSystem.systemAddress);
        return 0;
    }

    ReliableChannels::ReliableChannels()
    {
    }

    ReliableChannels::~ReliableChannels()
    {
        Reset();
    }

    void ReliableChannels::Flush()
    {
        Reset();
    }

    void ReliableChannels::Reset()
    {
        std::for_each(myOrderedChannels.begin(), myOrderedChannels.end(), [](auto& iter)
        {
            ikcp_release(iter);
        });
        myOrderedChannels.clear();
        myIdToChannel.clear();
    }

    bool ReliableChannels::Input(RemoteSystem& remoteSystem, const char *buffer, unsigned int length)
    {
        RNetAssert(buffer);
        if (length <= 4)
        {
            // Length of 1 is a connection request resend that we just ignore
            // KCP needs at least 4 bytes to read ordering channel.
            return true;
        }

        auto orderingChannel = ikcp_getconv(buffer);
        if (orderingChannel < NUMBER_OF_ORDERED_STREAMS)
        {
            struct IKCPCB* stream = EnsureChannel(orderingChannel, remoteSystem);
            if (ikcp_input(stream, buffer, long(length)) >= 0)
            {
                return true;
            }
        }
        return false;
    }

    uint32_t ReliableChannels::Receive(unsigned char **data)
    {
        while (myReceiveProcessIndex < myOrderedChannels.size())
        {
            auto& channel = myOrderedChannels[myReceiveProcessIndex];
            int nextSize = ikcp_peeksize(channel);
            if (nextSize > 0)
            {
                char* newBuffer = (char*)malloc(nextSize);
                auto res = ikcp_recv(channel, newBuffer, nextSize);
                RNetAssert(res >= 0);
                *data = (unsigned char*)newBuffer;
                return nextSize * 8;
            }
            myReceiveProcessIndex++;
        }
        myReceiveProcessIndex = 0;
        return 0;
    }

    bool ReliableChannels::Send(
        RemoteSystem& remoteSystem, char* data, uint32_t numberOfBytesToSend, unsigned char orderingChannel)
    {
        RNetAssert(orderingChannel < NUMBER_OF_ORDERED_STREAMS);
        RNetAssert(numberOfBytesToSend > 0);

        struct IKCPCB* stream = EnsureChannel(orderingChannel, remoteSystem);
        int result = ikcp_send(stream, data, numberOfBytesToSend);
        if (result >= 0)
        {
            return true;
        }
        RNetAbnormal(false);// Too many segments?
        return false;
    }

    struct IKCPCB* ReliableChannels::EnsureChannel(uint32_t channel, RemoteSystem& remoteSystem)
    {
        auto iter = myIdToChannel.find(channel);
        if (iter == myIdToChannel.end())
        {
            struct IKCPCB* ikcp = ikcp_create(channel, &remoteSystem);
            int res = ikcp_setmtu(ikcp, remoteSystem.MTUSize);
            RNetAssert(res == 0, "Invalid MTU %i", remoteSystem.MTUSize);
            res = ikcp_nodelay(ikcp,
                1,  // 1 = "nodelay" mode
                10, // 10 = Protocol internal work interval 10 milliseconds
                2,  // 2 = 2 ACK spans will result in direct retransmission
                1   // 1 = Non-concessional Flow Control
            );
            RNetAssert(res == 0, "Invalid KCP config");
            ikcp->output = SendKCPPacket;
            myOrderedChannels.emplace_back(ikcp);
            myIdToChannel.insert(std::pair<uint32_t, uint8_t>(channel, uint8_t(myOrderedChannels.size()-1)));
            return ikcp;
        }
        return myOrderedChannels[iter->second];
    }

    void ReliableChannels::Update(TimeMS time)
    {
        std::for_each(myOrderedChannels.begin(), myOrderedChannels.end(), [&time](auto& iter)
        {
            ikcp_update(iter, time);
        });
    }

    bool ReliableChannels::IsOutgoingDataWaiting(void)
    {
        for (auto iter = myOrderedChannels.begin(); iter != myOrderedChannels.end(); ++iter)
        {
            if (ikcp_waitsnd(*iter) > 0)
            {
                return true;
            }
        }
        return false;
    }

    bool ReliableChannels::AreAcksWaiting(void)
    {
        return IsOutgoingDataWaiting();
    }
}

#endif
