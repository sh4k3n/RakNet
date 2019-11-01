#define NOMINMAX
#include "../DependentExtensions/catch2/single_include/catch2/catch.hpp"

#include "TestEnv.h"
using namespace RakNet;

// Tests to measure and benchmark latency on packet loss conditions.
#include <chrono>

uint32_t SendBigData(size_t DataSize, size_t NumPackets, unsigned short minPing, unsigned short maxPing, float packetLoss)
{
    TestEnvironment env;
    int seed = 12345;

    size_t totalSend = 0;
    size_t totalReceived = 0;
    size_t totalTime = 0;
    size_t totalSendBytes = 0;
    size_t totalRecvBytes = 0;

    env.Setup(2, minPing, maxPing, packetLoss);

    size_t numConnections = 0;
    auto start = std::chrono::high_resolution_clock::now();
    do
    {
        if (numConnections == 2 && totalSend < numConnections*NumPackets)
        {
            BitStream s;
            s.Write((char)(ID_USER_PACKET_ENUM + 1));
            while (s.GetNumberOfBytesUsed() < DataSize)
            {
                s.Write(uint8_t(s.GetNumberOfBytesUsed()));
            }
            for (size_t i = 0; i < 2; ++i)
            {
                SystemAddress target = env.peers[i]->GetSystemAddressFromIndex(0);
                if (target.systemIndex != -1)
                {
                    env.peers[i]->Send(&s, PacketPriority::IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, target, false);
                    totalSend++;
                    totalSendBytes += s.GetNumberOfBytesUsed();
                }
            }
        }

        for (int i = 0; i < 2; i++)
        {
            Packet* packet;
            while (packet = env.peers[i]->Receive())
            {
                BitStream input(packet->data, packet->length, false);
                uint8_t id;
                input.Read(id);
                if (id == ID_USER_PACKET_ENUM + 1)
                {
                    RakNet::TimeMS sendTime;
                    while (input.GetNumberOfUnreadBits() > 0)
                    {
                        uint8_t v;
                        input.Read(v);
                        RNetAssert(v == uint8_t(input.GetReadOffset()/8 - 1));
                    }                 

                    totalReceived++;
                    totalRecvBytes += packet->length;
                    printf("TotalRecv=%lu/%lu\n", totalRecvBytes, 2 * DataSize * NumPackets);
                }
                else
                {
                    if (id == ID_CONNECTION_REQUEST_ACCEPTED || id == ID_ALREADY_CONNECTED ||
                        id == ID_NEW_INCOMING_CONNECTION)
                    {
                        numConnections = 0;
                        for (int j = 0; j < 2; j++)
                        {
                            if (env.peers[j]->NumberOfConnections() == 1)
                            {
                                numConnections++;
                            }
                        }
                    }
                }
                env.peers[i]->DeallocatePacket(packet);
            }
        }
        RakSleep(0);
    } while (totalRecvBytes != 2*DataSize * NumPackets);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000;
    printf("Elapsed=%f\n", elapsed);
    env.Stats();
    REQUIRE(totalRecvBytes == 2*DataSize * NumPackets);
    return elapsed;
}

TEST_CASE("BigData")
{
    unsigned short ping = 40;
    REQUIRE(SendBigData(1024 * 1024, 10, ping, ping + (ping / 10), 0.1f) < 50);  // TODO: Reduce to 20 when window size config done.
}

