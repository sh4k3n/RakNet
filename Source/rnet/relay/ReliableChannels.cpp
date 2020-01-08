#include <rnet/relay/ReliableChannels.h>

#if RAKNET_ARQ == RAKNET_ARQ_KCP
#include <kcp/ikcp.h>
#include <utility>

#include "remotesystem.h"
#include "BitStream.h"
#include <rnet/SocketLayer.h>
#include <rnet/relay/BigDataBuffer.h>

namespace rnet
{
    int SendKCPPacket(const char *buf, int len, ikcpcb *kcp, void *user)
    {
        RemoteSystem& remoteSystem = *static_cast<RemoteSystem*>(user);
        RakNet::BitStream bitStream((unsigned char*)buf, len, false);
		remoteSystem.metrics.OnSent(remoteSystem.lastUpdate, rnet::PacketType::Raw, len + UDP_HEADER_SIZE);
        rnet::socket_layer::SendTo(*remoteSystem.rakNetSocket, bitStream, remoteSystem.systemAddress);
        return 0;
    }

    ReliableChannels::ReliableChannels()
    {
        std::fill(myIsBigDataActive.begin(), myIsBigDataActive.end(), false);
    }

    ReliableChannels::~ReliableChannels()
    {
        Reset();
    }

    void ReliableChannels::Flush()
    {
        std::for_each(myOrderedChannels.begin(), myOrderedChannels.end(), [](auto& iter)
        {
            ikcp_flush(iter);
        });
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
            for (;;)
            {
                int nextSize = ikcp_peeksize(channel);
                if (nextSize <= 0)
                {
                    break;
                }

                if (!myIsBigDataActive[myReceiveProcessIndex])
                {
                    char* newBuffer = (char*)malloc(nextSize);
                    auto res = ikcp_recv(channel, newBuffer, nextSize);
                    RNetAssert(res >= 0);

                    if (newBuffer[0] != ID_BIG_DATA)
                    {
                        *data = (unsigned char*)newBuffer;
                        return nextSize * 8;
                    }
                    else
                    {
                        if (nextSize == 5)
                        {
                            BitStream stream((unsigned char*)(&newBuffer[1]), nextSize, false);
                            uint32_t totalSize;
                            stream.Read(totalSize);
                            if (myBigDataBuffers[myReceiveProcessIndex].Reserve(totalSize))
                            {
                                myIsBigDataActive[myReceiveProcessIndex] = true;
                            }
                        }
                        free(newBuffer);
                    }
                }
                else
                {
                    BigDataBuffer& buffer = myBigDataBuffers[myReceiveProcessIndex];
                    RNetAssert(nextSize <= buffer.myTotalSize - buffer.myUsedSize, "Invalid size");
                    {
                        int res = ikcp_recv(channel, &buffer.myBuffer[buffer.myUsedSize], nextSize);
                        RNetAssert(res >= 0);
                        buffer.myUsedSize += nextSize;

                        if (buffer.myTotalSize == buffer.myUsedSize)
                        {
                            *data = (unsigned char*)buffer.myBuffer;
                            buffer.myBuffer = nullptr;
                            nextSize = buffer.myTotalSize;
                            myBigDataBuffers[myReceiveProcessIndex].Free();
                            myIsBigDataActive[myReceiveProcessIndex] = false;
                            return nextSize * 8;
                        }
                    }
                }
            }
            myReceiveProcessIndex++;
        }
        myReceiveProcessIndex = 0;
        return 0;
    }

    bool ReliableChannels::Send(TimeMS time,
        RemoteSystem& remoteSystem, char* data, uint32_t numberOfBytesToSend, unsigned char orderingChannel)
    {
        RNetAssert(orderingChannel < NUMBER_OF_ORDERED_STREAMS);
        RNetAssert(numberOfBytesToSend > 0);

        struct IKCPCB* stream = EnsureChannel(orderingChannel, remoteSystem);
        int result = ikcp_send(stream, data, numberOfBytesToSend);
        if (result >= 0)
        {
			remoteSystem.metrics.OnSent(time, rnet::PacketType::UserReliable, numberOfBytesToSend);
            return true;
        }

        // TODO: Configure based on MTU
        const uint32_t SegmentSize = 64 * 1024;
        if (numberOfBytesToSend < SegmentSize)
        {
            RNetAbnormal(false);
            return false;
        }

        {
            BitStream bigDataInd;
            bigDataInd.Write(char(ID_BIG_DATA));
            bigDataInd.Write(numberOfBytesToSend);
            result = ikcp_send(stream, reinterpret_cast<char*>(bigDataInd.GetData()), bigDataInd.GetNumberOfBytesUsed());
            RNetAssert(result >= 0);
        }
        auto splits = (numberOfBytesToSend + (numberOfBytesToSend % (SegmentSize))) / (SegmentSize);
        auto pos = 0;
        while(pos < numberOfBytesToSend - (SegmentSize))
        {
			remoteSystem.metrics.OnSent(time, rnet::PacketType::UserReliable, SegmentSize);
            result = ikcp_send(stream, &data[pos], SegmentSize);
            RNetAssert(result >= 0);
            pos += SegmentSize;
        }
		remoteSystem.metrics.OnSent(time, rnet::PacketType::UserReliable, numberOfBytesToSend - pos);
        result = ikcp_send(stream, &data[pos], numberOfBytesToSend - pos);
        RNetAssert(result >= 0);
        return true;
    }

    struct IKCPCB* ReliableChannels::EnsureChannel(uint32_t channel, RemoteSystem& remoteSystem)
    {
        auto iter = myIdToChannel.find(channel);
        if (iter == myIdToChannel.end())
        {
            struct IKCPCB* ikcp = ikcp_create(channel, &remoteSystem);
            int res = ikcp_setmtu(ikcp, remoteSystem.MTUSize);
            RNetAssert(res == 0, "Invalid MTU %i", remoteSystem.MTUSize);
            const int WorkInterval = 10; // Protocol internal work interval in milliseconds
            const int ResendAckSpans = 2; // Number of ACK spans result in direct retransmission
            res = ikcp_nodelay(ikcp,
                1,  // 1 = "nodelay" mode
                WorkInterval, ResendAckSpans,
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

    void ReliableChannels::Update(const RemoteSystem& remoteSystem)
    {
        for (size_t i = 0; i < myOrderedChannels.size(); ++i)        
        {
            ikcpcb* iter = myOrderedChannels[i];
            int wnd;
            if (!myIsBigDataActive[i])
            {
                // TODO: Implement good way to choose correct window size. Need bigger window size for big data.
                // See. https://www.auvik.com/franklymsp/blog/tcp-window-size/
                const uint16_t MaxRTT = remoteSystem.lowestPing;
                const uint16_t TickRate = 60;
                wnd = TickRate * 2 * MaxRTT / 1000;
                if (wnd < 32)
                {
                    wnd = 32;
                }
                else if (wnd > 256)
                {
                    wnd = 256;
                }
            }
            else
            {
                wnd = 256;
            }

            if (iter->snd_wnd != wnd)
            {
                ikcp_wndsize(iter, wnd, wnd);
            }
            ikcp_update(iter, remoteSystem.lastUpdate);
        }
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
