/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "TestHelpers.h"

TestHelpers::TestHelpers(void)
{
}

TestHelpers::~TestHelpers(void)
{
}

void TestHelpers::StandardServerPrep(RakPeerInterface *&server)
{

	server=RakPeerInterface::GetInstance();
	server->Startup(1,&SocketDescriptor(60000,0),1);
	server->SetMaximumIncomingConnections(1);

}

void TestHelpers::StandardClientPrep(RakPeerInterface *&client)
{

	client=RakPeerInterface::GetInstance();

	client->Startup(1,&SocketDescriptor(),1);

}

void TestHelpers::StandardServerPrep(RakPeerInterface *&server,DataStructures::List <RakPeerInterface *> &destroyList)
{

	StandardServerPrep(server);
	destroyList.Push(server,_FILE_AND_LINE_);

}

void TestHelpers::StandardClientPrep(RakPeerInterface *&client,DataStructures::List <RakPeerInterface *> &destroyList)
{

	StandardClientPrep(client);
	destroyList.Push(client,_FILE_AND_LINE_);

}

//returns false if not connected
bool TestHelpers::WaitAndConnectTwoPeersLocally(RakPeerInterface *connector,RakPeerInterface *connectee,int millisecondsToWait)
{

	SystemAddress connecteeAdd=connectee->GetInternalID();
    return CommonFunctions::WaitAndConnect(connector, "127.0.0.1", connecteeAdd.GetPort(), millisecondsToWait);

}

//returns false if connect fails
bool TestHelpers::ConnectTwoPeersLocally(RakPeerInterface *connector,RakPeerInterface *connectee)
{
	SystemAddress connecteeAdd=connectee->GetInternalID();
    return connector->Connect("127.0.0.1", connecteeAdd.GetPort(), 0, 0);
}

bool TestHelpers::BroadCastTestPacket(RakPeerInterface *sender,PacketReliability rel,PacketPriority pr,int typeNum)//returns send return value
{

	char str2[]="AAAAAAAAAA";
	str2[0]=typeNum;
	return sender->Send(str2,(int) strlen(str2)+1, pr, rel  ,0, UNASSIGNED_SYSTEM_ADDRESS, true)>0;
}

bool TestHelpers::SendTestPacketDirected(RakPeerInterface *sender,char * ip,int port,PacketReliability rel,PacketPriority pr,int typeNum)//returns send return value
{
	SystemAddress recAddress(ip, port);
	char str2[]="AAAAAAAAAA";
	str2[0]=typeNum;
	return sender->Send(str2,(int) strlen(str2)+1, pr, rel  ,0, recAddress, false)>0;
}

bool TestHelpers::WaitForTestPacket(RakPeerInterface *reciever,int millisecondsToWait)
{

	RakTimer timer(millisecondsToWait);

	Packet *packet;
	while(!timer.IsExpired())
	{
		for (packet=reciever->Receive(); packet;reciever->DeallocatePacket(packet), packet=reciever->Receive())
		{

			if (packet->data[0]==ID_USER_PACKET_ENUM+1)
			{
				reciever->DeallocatePacket(packet);
				return true;
			}

		}

	}

	return false;

}

void RecieveForXTime(RakPeerInterface *reciever,int millisecondsToWait)
{

	RakTimer timer(millisecondsToWait);

	Packet *packet;
	while(!timer.IsExpired())
	{
		for (packet=reciever->Receive(); packet;reciever->DeallocatePacket(packet), packet=reciever->Receive())
		{

		}

	}
}

#if RAKNET_ARQ == RAKNET_ARQ_KCP
namespace RakNet
{
	// Verbosity level currently supports 0 (low), 1 (medium), 2 (high)
	// Buffer must be hold enough to hold the output string.  See the source to get an idea of how many bytes will be output
	void StatisticsToString(rnet::Stats *s, char *buffer, int verbosityLevel)
	{
		if (s == 0)
		{
			sprintf(buffer, "stats is a NULL pointer in statsToString\n");
			return;
		}

		if (verbosityLevel == 0)
		{
			sprintf(buffer,
				"MBytes per second sent     %.1f%\n"
				"MBytes per second received %.1f%\n"
				"Current packetloss        %.1f%\n",
				s->RawBytesPerSecondSent() / (1000.0*1000.0),
				s->RawBytesPerSecondReceived() / (1000.0*1000.0),
				s->PacketLossPerSecond()*100.0f
			);
		}
		else //if (verbosityLevel == 1)
		{
			sprintf(buffer,
				"MBytes per second sent       %.1f%\n"
				"MBytes per second received   %.1f%\n"
				"Total actual MBytes sent      %.1f%\n"
				"Total actual MBytes received  %.1f%\n"
				"Total user MBytes sent       %.1f%\n"
				"Current packetloss           %.1f%\n",
				s->RawBytesPerSecondSent() / (1000.0*1000.0),
				s->RawBytesPerSecondReceived() / (1000.0*1000.0),
				s->RawBytesSent() / (1000.0*1000.0),
				s->RawBytesReceived() / (1000.0*1000.0),
				s->UserBytesSent() / (1000.0*1000.0),
				s->PacketLossPerSecond()*100.0f
			);
			/*sprintf(buffer,
				"Actual bytes per second sent       %" PRINTF_64_BIT_MODIFIER "u\n"
				"Actual bytes per second received   %" PRINTF_64_BIT_MODIFIER "u\n"
				"Message bytes per second pushed    %" PRINTF_64_BIT_MODIFIER "u\n"
				"Total actual bytes sent            %" PRINTF_64_BIT_MODIFIER "u\n"
				"Total actual bytes received        %" PRINTF_64_BIT_MODIFIER "u\n"
				"Total message bytes pushed         %" PRINTF_64_BIT_MODIFIER "u\n"
				"Protocol overhead                  %.1f%%\n"
				"Current packetloss                 %.1f%%\n"
				"Average packetloss                 %.1f%%\n"
				"Elapsed connection time in seconds %" PRINTF_64_BIT_MODIFIER "u\n",
				(long long unsigned int) s->valueOverLastSecond[ACTUAL_BYTES_SENT],
				(long long unsigned int) s->valueOverLastSecond[ACTUAL_BYTES_RECEIVED],
				(long long unsigned int) s->valueOverLastSecond[USER_MESSAGE_BYTES_PUSHED],
				(long long unsigned int) s->runningTotal[ACTUAL_BYTES_SENT],
				(long long unsigned int) s->runningTotal[ACTUAL_BYTES_RECEIVED],
				(long long unsigned int) s->runningTotal[USER_MESSAGE_BYTES_PUSHED],

				s->runningTotal[USER_MESSAGE_BYTES_PUSHED] > 0 ?
				float(s->runningTotal[ACTUAL_BYTES_SENT] - s->runningTotal[USER_MESSAGE_BYTES_PUSHED]) * 100.0f /
				s->runningTotal[USER_MESSAGE_BYTES_PUSHED] : 0.0f,

				s->packetlossLastSecond*100.0f,
				s->packetlossTotal*100.0f,
				(long long unsigned int) (uint64_t)((RakNet::GetTimeUS() - s->connectionStartTime) / 1000000)
			);

			if (s->BPSLimitByCongestionControl != 0)
			{
				char buff2[128];
				sprintf(buff2,
					"Send capacity                    %" PRINTF_64_BIT_MODIFIER "u bytes per second (%.0f%%)\n",
					(long long unsigned int) s->BPSLimitByCongestionControl,
					100.0f * s->valueOverLastSecond[ACTUAL_BYTES_SENT] / s->BPSLimitByCongestionControl
				);
				strcat(buffer, buff2);
			}
			if (s->BPSLimitByOutgoingBandwidthLimit != 0)
			{
				char buff2[128];
				sprintf(buff2,
					"Send limit                       %" PRINTF_64_BIT_MODIFIER "u (%.0f%%)\n",
					(long long unsigned int) s->BPSLimitByOutgoingBandwidthLimit,
					100.0f * s->valueOverLastSecond[ACTUAL_BYTES_SENT] / s->BPSLimitByOutgoingBandwidthLimit
				);
				strcat(buffer, buff2);
			}*/
		}
		/*else
		{
			sprintf(buffer,
				"Actual bytes per second sent         %" PRINTF_64_BIT_MODIFIER "u\n"
				"Actual bytes per second received     %" PRINTF_64_BIT_MODIFIER "u\n"
				"Message bytes per second sent        %" PRINTF_64_BIT_MODIFIER "u\n"
				"Message bytes per second resent      %" PRINTF_64_BIT_MODIFIER "u\n"
				"Message bytes per second pushed      %" PRINTF_64_BIT_MODIFIER "u\n"
				"Message bytes per second returned	  %" PRINTF_64_BIT_MODIFIER "u\n"
				"Message bytes per second ignored     %" PRINTF_64_BIT_MODIFIER "u\n"
				"Total bytes sent                     %" PRINTF_64_BIT_MODIFIER "u\n"
				"Total bytes received                 %" PRINTF_64_BIT_MODIFIER "u\n"
				"Total message bytes sent             %" PRINTF_64_BIT_MODIFIER "u\n"
				"Total message bytes resent           %" PRINTF_64_BIT_MODIFIER "u\n"
				"Total message bytes pushed           %" PRINTF_64_BIT_MODIFIER "u\n"
				"Total message bytes returned		  %" PRINTF_64_BIT_MODIFIER "u\n"
				"Total message bytes ignored          %" PRINTF_64_BIT_MODIFIER "u\n"
				"Messages in send buffer, by priority %i,%i,%i,%i\n"
				"Bytes in send buffer, by priority    %i,%i,%i,%i\n"
				"Messages in resend buffer            %i\n"
				"Bytes in resend buffer               %" PRINTF_64_BIT_MODIFIER "u\n"
				"Current packetloss                   %.1f%%\n"
				"Average packetloss                   %.1f%%\n"
				"Elapsed connection time in seconds   %" PRINTF_64_BIT_MODIFIER "u\n",
				(long long unsigned int) s->valueOverLastSecond[ACTUAL_BYTES_SENT],
				(long long unsigned int) s->valueOverLastSecond[ACTUAL_BYTES_RECEIVED],
				(long long unsigned int) s->valueOverLastSecond[USER_MESSAGE_BYTES_SENT],
				(long long unsigned int) s->valueOverLastSecond[USER_MESSAGE_BYTES_RESENT],
				(long long unsigned int) s->valueOverLastSecond[USER_MESSAGE_BYTES_PUSHED],
				(long long unsigned int) s->valueOverLastSecond[USER_MESSAGE_BYTES_RECEIVED_PROCESSED],
				(long long unsigned int) s->valueOverLastSecond[USER_MESSAGE_BYTES_RECEIVED_IGNORED],
				(long long unsigned int) s->runningTotal[ACTUAL_BYTES_SENT],
				(long long unsigned int) s->runningTotal[ACTUAL_BYTES_RECEIVED],
				(long long unsigned int) s->runningTotal[USER_MESSAGE_BYTES_SENT],
				(long long unsigned int) s->runningTotal[USER_MESSAGE_BYTES_RESENT],
				(long long unsigned int) s->runningTotal[USER_MESSAGE_BYTES_PUSHED],
				(long long unsigned int) s->runningTotal[USER_MESSAGE_BYTES_RECEIVED_PROCESSED],
				(long long unsigned int) s->runningTotal[USER_MESSAGE_BYTES_RECEIVED_IGNORED],
				s->messageInSendBuffer[IMMEDIATE_PRIORITY], s->messageInSendBuffer[HIGH_PRIORITY], s->messageInSendBuffer[MEDIUM_PRIORITY], s->messageInSendBuffer[LOW_PRIORITY],
				(unsigned int)s->bytesInSendBuffer[IMMEDIATE_PRIORITY], (unsigned int)s->bytesInSendBuffer[HIGH_PRIORITY], (unsigned int)s->bytesInSendBuffer[MEDIUM_PRIORITY], (unsigned int)s->bytesInSendBuffer[LOW_PRIORITY],
				s->messagesInResendBuffer,
				(long long unsigned int) s->bytesInResendBuffer,
				s->packetlossLastSecond*100.0f,
				s->packetlossTotal*100.0f,
				(long long unsigned int) (uint64_t)((RakNet::GetTimeUS() - s->connectionStartTime) / 1000000)
			);

			if (s->BPSLimitByCongestionControl != 0)
			{
				char buff2[128];
				sprintf(buff2,
					"Send capacity                    %" PRINTF_64_BIT_MODIFIER "u bytes per second (%.0f%%)\n",
					(long long unsigned int) s->BPSLimitByCongestionControl,
					100.0f * s->valueOverLastSecond[ACTUAL_BYTES_SENT] / s->BPSLimitByCongestionControl
				);
				strcat(buffer, buff2);
			}
			if (s->BPSLimitByOutgoingBandwidthLimit != 0)
			{
				char buff2[128];
				sprintf(buff2,
					"Send limit                       %" PRINTF_64_BIT_MODIFIER "u (%.0f%%)\n",
					(long long unsigned int) s->BPSLimitByOutgoingBandwidthLimit,
					100.0f * s->valueOverLastSecond[ACTUAL_BYTES_SENT] / s->BPSLimitByOutgoingBandwidthLimit
				);
				strcat(buffer, buff2);
			}
		}*/
	}
}
#endif