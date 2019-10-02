/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#define NOMINMAX
#include "../DependentExtensions/catch2/single_include/catch2/catch.hpp"

#include "RakPeerInterface.h"

#include "BitStream.h"
#include <stdlib.h> // For atoi
#include <cstring> // For strlen
#include "Rand.h"

#include <stdio.h>
#include <algorithm>
#include "GetTime.h"
#include "RakSleep.h"
#include "TestEnv.h"
using namespace RakNet;

//#define _VERIFY_RECIPIENTS
// #define _DO_PRINTF


// run a bit of everything to test for crashes
TEST_CASE("StressRandom")
{
    constexpr RakNet::TimeMS milliseconds = 15 * 1000;

    TestEnvironment env;
    char data[8096];
    int seed = 12345;

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
			data[0]=char(ID_USER_PACKET_ENUM);
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

        for (int i = 0; i < NUM_PEERS; i++)
        {
            env.peers[i]->DeallocatePacket(env.peers[i]->Receive());
        }

        RakSleep(0);
	}
}


