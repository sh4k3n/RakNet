#define NOMINMAX
#include "../DependentExtensions/catch2/single_include/catch2/catch.hpp"

#include "TestEnv.h"
using namespace RakNet;

// Tests to measure and benchmark latency on packet loss conditions.

float MeasurePacketDelay(RakNet::TimeMS milliseconds, unsigned short minPing, unsigned short maxPing, float packetLoss)
{
    TestEnvironment env;
    int seed = 12345;

    size_t totalSend = 0;
    size_t totalReceived = 0;
    size_t totalTime = 0;
    size_t totalSendBytes = 0;
    size_t totalRecvBytes = 0;
    uint16_t measuredMinPing = UINT16_MAX;
    uint16_t measuredMaxPing = 0;

    env.Setup(2, minPing, maxPing, packetLoss);
    RakNet::TimeMS lastSend = RakNet::GetTimeMS();
    RakNet::TimeMS endTime = RakNet::GetTimeMS() + milliseconds;
    constexpr RakNet::TimeMS timeBetweenPackets = 16;
    constexpr size_t PacketSize = 128;

    RakNet::TimeMS currentTime;
    size_t numConnections = 0;


    do
    {
        do
        {
            currentTime = RakNet::GetTimeMS();
            if (numConnections == 2 && static_cast<int32_t>(endTime - currentTime) > 0 &&
                static_cast<int>(currentTime - lastSend) > timeBetweenPackets)
            {
                lastSend = currentTime;
                BitStream s;
                s.Write((char)(ID_USER_PACKET_ENUM + 1));
                s.Write(currentTime);
                s.Write(totalSend);
                while (s.GetNumberOfBytesUsed() < PacketSize)
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
                        input.Read(sendTime);
                        size_t readIndex;
                        input.Read(readIndex);
                        totalReceived++;
                        auto pingTime = uint16_t(currentTime - sendTime);
                        measuredMinPing = std::min(pingTime, measuredMinPing);
                        measuredMaxPing = std::max(pingTime, measuredMaxPing);
                        totalTime += pingTime;
                        totalRecvBytes += packet->length;
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
                        else
                        {
                            printf("Received %u\n", id);
                        }
                    }
                    env.peers[i]->DeallocatePacket(packet);
                }
            }
            RakSleep(0);
        } while (static_cast<int32_t>(endTime - currentTime) > 0);
        // printf("Sent=%u Received=%u\n", totalSend, totalReceived);
    } while (totalReceived != totalSend && static_cast<int32_t>(currentTime - endTime) < 2000);
    env.Stats();
    REQUIRE(totalReceived == totalSend);
    float mean = 1000.0f;
    if (totalReceived > 0)
    {
        mean = float(totalTime) / totalReceived;
        printf("Ping mean=%f ms min=%u max=%u recv=%lu(%lu bytes) sent=%lu(%lu bytes) packetLoss=%f\n",
            mean, measuredMinPing, measuredMaxPing,
            totalReceived, totalRecvBytes, totalSend, totalSendBytes, packetLoss);
    }
    return mean;
}

static constexpr RakNet::TimeMS TestLength = 15 * 1000;

TEST_CASE("PacketDelay40msNoPacketLoss")
{
    unsigned short ping = 40;
    REQUIRE(MeasurePacketDelay(TestLength, ping, ping + (ping / 10), 0.0f) < 50.0f);
}

TEST_CASE("PacketDelay40msLightPacketLoss")
{
    unsigned short ping = 40;
    REQUIRE(MeasurePacketDelay(TestLength, ping, ping + (ping / 10), 0.02f) < 80.0f);
}

TEST_CASE("PacketDelay40msHeavyPacketLoss")
{
    unsigned short ping = 40;
    REQUIRE(MeasurePacketDelay(TestLength, ping, ping + (ping / 10), 0.07f) < 100.0f);
}

TEST_CASE("PacketDelay250msNoPacketLoss")
{
    unsigned short ping = 250;
    REQUIRE(MeasurePacketDelay(TestLength, ping, ping + (ping / 10), 0.0f) < 350.0f);
}

TEST_CASE("PacketDelay150msHeavyPacketLoss")
{
    unsigned short ping = 150;
    REQUIRE(MeasurePacketDelay(TestLength, ping, ping + (ping / 10), 0.05f) < 350.0f);
}

TEST_CASE("PacketDelay250msLightPacketLoss")
{
    unsigned short ping = 250;
    REQUIRE(MeasurePacketDelay(TestLength, ping, ping + (ping / 10), 0.01f) < 400.0f);
}

TEST_CASE("PacketDelayVarying")
{
    REQUIRE(MeasurePacketDelay(TestLength, 50, 500, 0.00f) < 500.0f);
}

TEST_CASE("PacketDelay500msNoPacketLoss")
{
    unsigned short ping = 500;
    REQUIRE(MeasurePacketDelay(TestLength, ping, ping + (ping / 10), 0.0f) < 600.0f);
}
