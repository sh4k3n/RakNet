#pragma once

#include "RakNetSocket2.h"

namespace rnet
{
    namespace socket_layer
    {
        namespace detail
        {
            static void SendTo(RakNet::RakNetSocket2& rakNetSocket, RakNet::BitStream& bitStream,
                const RakNet::SystemAddress &systemAddress)
            {
#ifdef USE_THREADED_SEND
                SendToThread::SendToThreadBlock *block = SendToThread::AllocateBlock();
                memcpy(block->data, bitStream.GetData(), bitStream.GetNumberOfBytesUsed());
                block->dataWriteOffset = bitStream.GetNumberOfBytesUsed();
                //block->extraSocketOptions = extraSocketOptions;
                //block->remotePortRakNetWasStartedOn_PS3 = remotePortRakNetWasStartedOn_PS3;
                //block->s = s;
                block->systemAddress = remoteSystem.systemAddress;
                SendToThread::ProcessBlock(block);
#else
                RakNet::RNS2_SendParameters bsp;
                bsp.data = (char*)bitStream.GetData();
                bsp.length = bitStream.GetNumberOfBytesUsed();
                bsp.systemAddress = systemAddress;
                rakNetSocket.Send(&bsp, _FILE_AND_LINE_);
#endif
            }
        }

        static void SendTo(RakNet::RakNetSocket2& rakNetSocket, RakNet::BitStream& bitStream,
            const RakNet::SystemAddress &systemAddress)
        {
#ifdef RAKNET_NETWORK_SIMULATOR
            rakNetSocket.SendToSimulator(bitStream, systemAddress);
#else
            detail::SendTo(rakNetSocket, bitStream, systemAddress);
#endif
            bitStream.GetNumberOfBytesUsed();
        }
    }
}

