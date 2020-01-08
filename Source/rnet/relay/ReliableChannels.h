#pragma once
#include <rnet/RNet.h>
#include "RakNetTime.h"
#include "MessageIdentifiers.h"

#include <rnet/relay/BigDataBuffer.h>

#if RAKNET_ARQ == RAKNET_ARQ_KCP
#include <unordered_map>
#include <vector>
#include <array>

/// Number of ordered streams available. You can use up to 32 ordered streams
// TODO: Currently limited to first message id (ID_CONNECTED_PING=32)
// TODO: KCP supports more streams.
#define NUMBER_OF_ORDERED_STREAMS ID_CONNECTED_PING // 32 // 2^5

struct IKCPCB;
namespace RakNet
{
    struct RemoteSystem;
    class BitStream;
}

namespace rnet
{
    using RemoteSystem = struct RakNet::RemoteSystem;
    using BitStream = RakNet::BitStream;
    using TimeMS = RakNet::TimeMS;
    class BigDataBuffer;

    class ReliableChannels
    {
    public:
        ReliableChannels();
        ~ReliableChannels();
        void Flush();
        void Reset();
        bool Input(RemoteSystem& remoteSystem, const char *buffer, unsigned int length);
        uint32_t Receive(unsigned char**data);
        bool Send(TimeMS time, RemoteSystem& remoteSystem, char *data, uint32_t numberOfBytesToSend,
            unsigned char orderingChannel);
        void Update(const RemoteSystem& remoteSystem);
        bool IsOutgoingDataWaiting(void);
        bool AreAcksWaiting(void);

    private:
        struct IKCPCB* EnsureChannel(uint32_t channel, RemoteSystem& remoteSystem);

        std::vector<struct IKCPCB*> myOrderedChannels;
        uint32_t myReceiveProcessIndex = 0;
        std::unordered_map<uint32_t, uint8_t> myIdToChannel;
        std::array<BigDataBuffer, NUMBER_OF_ORDERED_STREAMS> myBigDataBuffers;
        std::array<bool, NUMBER_OF_ORDERED_STREAMS> myIsBigDataActive;
    };

}

#endif
