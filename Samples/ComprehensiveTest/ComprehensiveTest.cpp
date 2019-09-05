/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#define CATCH_CONFIG_MAIN
#include "catch2/single_include/catch2/catch.hpp"

#include "RakPeerInterface.h"

#include "BitStream.h"
#include <stdlib.h> // For atoi
#include <cstring> // For strlen
#include "Rand.h"
#include "RakNetStatistics.h"
#include "MessageIdentifiers.h"
#include <stdio.h>
#include "GetTime.h"
using namespace RakNet;

#ifdef _WIN32
#include "WindowsIncludes.h" // Sleep
#else
#include <unistd.h> // usleep
#include <cstdio>
#endif

//#define _VERIFY_RECIPIENTS
#define _DO_PRINTF

#define NUM_PEERS 10
#define CONNECTIONS_PER_SYSTEM 4

struct TestEnvironment
{
    RakPeerInterface *peers[NUM_PEERS] = { nullptr };

    void Setup(size_t numPeers, unsigned short minPing = 0, unsigned short maxPing = 0, float packetLoss = 0)
    {
        unsigned short basePort = 50000;
        for (uint16_t i = 0; i < numPeers; i++)
        {
            peers[i] = RakNet::RakPeerInterface::GetInstance();
            peers[i]->ApplyNetworkSimulator(packetLoss, minPing, maxPing-minPing);
            peers[i]->SetMaximumIncomingConnections(CONNECTIONS_PER_SYSTEM);
            RakNet::SocketDescriptor socketDescriptor(basePort + i, 0);
            peers[i]->Startup(numPeers, &socketDescriptor, 1);
            peers[i]->SetOfflinePingResponse("Offline Ping Data", (int)strlen("Offline Ping Data") + 1);
        }

        for (uint16_t i = 0; i < numPeers; i++)
        {
            for (uint16_t j = 0; j < numPeers; j++)
            {
                if (i != j)
                {
                    peers[i]->Connect("127.0.0.1", basePort + j, 0, 0);
                }
            }
        }
    }

    ~TestEnvironment()
    {
        for (uint16_t i = 0; i < NUM_PEERS; i++)
        {
            if (peers[i])
            {
                peers[i]->Shutdown(500);
                RakNet::RakPeerInterface::DestroyInstance(peers[i]); 
            }
        }
    }

    void Stats()
    {
        char data[1024];
        for (uint16_t i = 0; i < NUM_PEERS; i++)
        {
            for (uint16_t j = 0; j < NUM_PEERS; j++)
            {
                if (i != j && peers[i] && peers[j])
                {
                    //SystemAddress target, mySystemAddress;
                    RakNetStatistics rss;
                    //mySystemAddress = peers[j]->GetInternalID();
                    if (peers[i]->GetStatistics(0, &rss))
                        //if (rss)
                        {
                            StatisticsToString(&rss, data, 1);
#ifdef _DO_PRINTF
                            printf("Statistics for local system %i:\n%s", j, data);
#endif
                        }
                }
            }
        }
    }

};

int test(RakNet::TimeMS milliseconds)
{
    TestEnvironment env;
    char data[8096];
    int seed = 12345;

    printf("This is just a test app to run a bit of everything to test for crashes.\n");
	printf("Difficulty: Intermediate\n\n");
    printf("Using seed %i\n", seed);
    seedMT(seed);

    env.Setup(NUM_PEERS);


	RakNet::TimeMS endTime = RakNet::GetTimeMS()+ milliseconds;
	while (RakNet::GetTimeMS()<endTime)
	{
        float nextAction = frandomMT();
        int peerIndex = 0;

		if (nextAction < .04f)
		{
			// Initialize
			int peerIndex=randomMT()%NUM_PEERS;
			RakNet::SocketDescriptor socketDescriptor(60000+peerIndex, 0);
			env.peers[peerIndex]->Startup(NUM_PEERS, &socketDescriptor, 1);
            env.peers[peerIndex]->Connect("127.0.0.1", 60000+randomMT() % NUM_PEERS, 0, 0);
		}
		else if (nextAction < .09f)
		{
			// Connect
			peerIndex=randomMT()%NUM_PEERS;
            env.peers[peerIndex]->Connect("127.0.0.1", 60000+randomMT() % NUM_PEERS, 0, 0);
		}
		else if (nextAction < .10f)
		{
			// Disconnect
			peerIndex=randomMT()%NUM_PEERS;
		//	peers[peerIndex]->Shutdown(randomMT() % 100);
		}
		else if (nextAction < .12f)
		{
			// GetConnectionList
			peerIndex=randomMT()%NUM_PEERS;
			SystemAddress remoteSystems[NUM_PEERS];
			unsigned short numSystems=NUM_PEERS;
			env.peers[peerIndex]->GetConnectionList(remoteSystems, &numSystems);
			if (numSystems>0)
			{
#ifdef _DO_PRINTF
				printf("%i: ", 60000+numSystems);
				for (int i=0; i < numSystems; i++)
				{
					printf("%i: ", remoteSystems[i].GetPort());
				}
				printf("\n");
#endif
			}			
		}
		else if (nextAction < .14f)
		{
			// Send
			int dataLength;
			PacketPriority priority;
			PacketReliability reliability;
			unsigned char orderingChannel;
			SystemAddress target;
			bool broadcast;

		//	data[0]=ID_RESERVED1+(randomMT()%10);
			data[0]=ID_USER_PACKET_ENUM;
			dataLength=3+(randomMT()%8000);
//			dataLength=600+(randomMT()%7000);
			priority=(PacketPriority)(randomMT()%(int)NUMBER_OF_PRIORITIES);
			reliability=(PacketReliability)(randomMT()%((int)RELIABLE_SEQUENCED+1));
			orderingChannel=randomMT()%32;
			if ((randomMT()%NUM_PEERS)==0)
				target=RakNet::UNASSIGNED_SYSTEM_ADDRESS;
			else
				target=env.peers[peerIndex]->GetSystemAddressFromIndex(randomMT()%NUM_PEERS);

			broadcast=(bool)(randomMT()%2);
#ifdef _VERIFY_RECIPIENTS
			broadcast=false; // Temporarily in so I can check recipients
#endif

			peerIndex=randomMT()%NUM_PEERS;
			sprintf(data+3, "dataLength=%i priority=%i reliability=%i orderingChannel=%i target=%i broadcast=%i\n", dataLength, priority, reliability, orderingChannel, target.GetPort(), broadcast);
			//unsigned short localPort=60000+i;
#ifdef _VERIFY_RECIPIENTS
			memcpy((char*)data+1, (char*)&target.port, sizeof(unsigned short));
#endif
            data[dataLength-1]=0;
            env.peers[peerIndex]->Send(data, dataLength, priority, reliability, orderingChannel, target, broadcast);
		}
		else if (nextAction < .18f)
		{
			int dataLength;
			PacketPriority priority;
			PacketReliability reliability;
			unsigned char orderingChannel;
			SystemAddress target;
			bool broadcast;

			data[0]=ID_USER_PACKET_ENUM+(randomMT()%10);
			dataLength=3+(randomMT()%8000);
//			dataLength=600+(randomMT()%7000);
			priority=(PacketPriority)(randomMT()%(int)NUMBER_OF_PRIORITIES);
			reliability=(PacketReliability)(randomMT()%((int)RELIABLE_SEQUENCED+1));
			orderingChannel=randomMT()%32;
			peerIndex=randomMT()%NUM_PEERS;
			if ((randomMT()%NUM_PEERS)==0)
				target=RakNet::UNASSIGNED_SYSTEM_ADDRESS;
			else
				target=env.peers[peerIndex]->GetSystemAddressFromIndex(randomMT()%NUM_PEERS);
			broadcast=(bool)(randomMT()%2);
#ifdef _VERIFY_RECIPIENTS
			broadcast=false; // Temporarily in so I can check recipients
#endif

			sprintf(data+3, "dataLength=%i priority=%i reliability=%i orderingChannel=%i target=%i broadcast=%i\n", dataLength, priority, reliability, orderingChannel, target.GetPort(), broadcast);
#ifdef _VERIFY_RECIPIENTS
			memcpy((char*)data, (char*)&target.port, sizeof(unsigned short));
#endif
			data[dataLength-1]=0;
		}
		else if (nextAction < .181f)
		{
			// CloseConnection
			SystemAddress target;
			peerIndex=randomMT()%NUM_PEERS;
			target=env.peers[peerIndex]->GetSystemAddressFromIndex(randomMT()%NUM_PEERS);
            env.peers[peerIndex]->CloseConnection(target, (bool)(randomMT()%2), 0);
		}
		else if (nextAction < .20f)
		{
			// Offline Ping
			peerIndex=randomMT()%NUM_PEERS;
            env.peers[peerIndex]->Ping("127.0.0.1", 60000+(randomMT()%NUM_PEERS), (bool)(randomMT()%2));
		}
		else if (nextAction < .21f)
		{
			// Online Ping
			SystemAddress target;
			target=env.peers[peerIndex]->GetSystemAddressFromIndex(randomMT()%NUM_PEERS);
			peerIndex=randomMT()%NUM_PEERS;
            env.peers[peerIndex]->Ping(target);
		}
		else if (nextAction < .24f)
		{

		}
		else if (nextAction < .25f)
		{
			// GetStatistics
			SystemAddress target, mySystemAddress;
			RakNetStatistics *rss;
			mySystemAddress= env.peers[peerIndex]->GetInternalID();
			target= env.peers[peerIndex]->GetSystemAddressFromIndex(randomMT()%NUM_PEERS);
			peerIndex=randomMT()%NUM_PEERS;
			rss= env.peers[peerIndex]->GetStatistics(mySystemAddress);
			if (rss)
			{
				StatisticsToString(rss, data, 0);
#ifdef _DO_PRINTF
				printf("Statistics for local system %i:\n%s", mySystemAddress.GetPort(), data);
#endif
			}
			
			rss= env.peers[peerIndex]->GetStatistics(target);
			if (rss)
			{
				StatisticsToString(rss, data, 0);
#ifdef _DO_PRINTF
				printf("Statistics for target system %i:\n%s", target.GetPort(), data);
#endif
			}			
		}

		for (int i=0; i < NUM_PEERS; i++)
            env.peers[i]->DeallocatePacket(env.peers[i]->Receive());

#ifdef _WIN32
		Sleep(0);
#else
		usleep(0);
#endif
	}


	for (int i=0; i < NUM_PEERS; i++)
		RakNet::RakPeerInterface::DestroyInstance(env.peers[i]);

	return 0;
}

void TestPacketDelay(RakNet::TimeMS milliseconds, unsigned short minPing, unsigned short maxPing, float packetLoss)
{
    TestEnvironment env;
    char data[8096];
    int seed = 12345;

    size_t totalSend = 0;
    size_t totalReceived = 0;
    size_t totalTime = 0;
    size_t totalSendBytes = 0;
    size_t totalRecvBytes = 0;
    uint16_t measuredMinPing = UINT16_MAX;
    uint16_t measuredMaxPing = 0;

    env.Setup(2, minPing, maxPing, packetLoss);
    RakNet::TimeMS lastSend = RakNet::GetTimeMS() + 2000;
    RakNet::TimeMS endTime = RakNet::GetTimeMS() + milliseconds;
    RakNet::TimeMS currentTime;
    
    do
    {
        do
        {
            currentTime = RakNet::GetTimeMS();
            if (static_cast<int>(currentTime - lastSend) > 100 && static_cast<int>(endTime - currentTime) > 0)
            {
                lastSend = currentTime;
                BitStream s;
                s.Write((char)(ID_USER_PACKET_ENUM));
                s.Write(currentTime);
                s.Write(totalSend);
                while (s.GetNumberOfBytesUsed() < 128)
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
                    if (id == ID_USER_PACKET_ENUM)
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
                        printf("Received=%u\n", id);
                    }
                    env.peers[i]->DeallocatePacket(packet);
                }
            }
#ifdef _WIN32
            Sleep(0);
#else
            usleep(0);
#endif
        } while (static_cast<int>(endTime - currentTime) > 0);
        printf("Sent=%u Received=%u\n", totalSend, totalReceived);
    } while (totalReceived != totalSend && static_cast<int>(currentTime - endTime) < 1000);
    env.Stats();
    if (totalReceived > 0)
    {        
        printf("Ping mean=%f ms min=%u max=%u recv=%lu(%lu) sent=%lu(%lu) packetLoss=%f\n", 
            float(totalTime) / totalReceived, measuredMinPing, measuredMaxPing,
            totalReceived, totalRecvBytes, totalSend, totalSendBytes, packetLoss);
    }
}


TEST_CASE("Stress")
{
    unsigned short ping = 40;
    TestPacketDelay(15 * 1000, ping, ping + (ping / 10), 0.0f);
    /*TestPacketDelay(15 * 1000, ping, ping + (ping / 10), 0.01f);
    TestPacketDelay(15 * 1000, ping, ping + (ping / 10), 0.025f);*/
    TestPacketDelay(120 * 1000, ping, ping + (ping / 10), 0.1f);
}


