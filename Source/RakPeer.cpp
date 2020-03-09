#include "RakPeer.h"
#include "RakSleep.h"

namespace RakNet
{
	STATIC_FACTORY_DEFINITIONS(RakPeerInterface, RakPeer)

		RAK_THREAD_DECLARATION(UpdateNetworkLoop);

	RakPeer::RakPeer() : BasePeer()
	{
	}

	RakPeer::~RakPeer()
	{
		Shutdown();
	}

	// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	RAK_THREAD_DECLARATION(RakNet::UpdateNetworkLoop)
	{
		RakPeer * rakPeer = (RakPeer *)arguments;

		BitStream updateBitStream(MAXIMUM_MTU_SIZE
#if LIBCAT_SECURITY==1
			+ cat::AuthenticatedEncryption::OVERHEAD_BYTES
#endif
		);
		// 
		rakPeer->isMainLoopThreadActive = true;

		while (rakPeer->endThreads == false)
		{
			// #ifdef _DEBUG
			// 		// Sanity check, make sure RunUpdateCycle does not block or not otherwise get called for a long time
			// 		RakNetTime thisCall=RakNet::GetTime();
			// 		RakAssert(thisCall-lastCall<250);
			// 		lastCall=thisCall;
			// #endif
			if (rakPeer->userUpdateThreadPtr)
				rakPeer->userUpdateThreadPtr(rakPeer, rakPeer->userUpdateThreadData);

			rakPeer->RunUpdateCycle(updateBitStream);

			// Pending sends go out this often, unless quitAndDataEvents is set
			rakPeer->quitAndDataEvents.WaitOnEvent(10);
		}

		rakPeer->isMainLoopThreadActive = false;
		return 0;

	}

	bool RakPeer::StartThreads(int threadPriority)
	{
		BasePeer::StartThreads(threadPriority);
		if (isMainLoopThreadActive == false)
		{
			int errorCode;
			errorCode = RakNet::RakThread::Create(UpdateNetworkLoop, this, threadPriority);
			if (errorCode != 0)
			{
				Shutdown(0, 0);
				return false;
			}
		}
		// Wait for the threads to activate.  When they are active they will set these variables to true
		while (isMainLoopThreadActive == false)
		{
			RakSleep(10);
		}
		return true;
	}

	void RakPeer::StopThreads()
	{
		BasePeer::StopThreads();


#if !defined(__native_client__) && !defined(WINDOWS_STORE_RT)
		for (unsigned int i = 0; i < socketList.Size(); i++)
		{
			if (socketList[i]->IsBerkleySocket())
			{
				((RNS2_Berkley *)socketList[i])->SignalStopRecvPollingThread();
			}
		}
#endif

		/*
		// Get recvfrom to unblock
		for (i=0; i < socketList.Size(); i++)
		{
			if (SocketLayer::SendTo(socketList[i], (const char*) &i,1,socketList[i]->GetBoundAddress(), _FILE_AND_LINE_)!=0)
				break;
		}
		*/

		while (isMainLoopThreadActive)
		{
			endThreads = true;
			RakSleep(15);
		}

		/*
		timeout = RakNet::GetTimeMS()+1000;
		while ( isRecvFromLoopThreadActive.GetValue()>0 && RakNet::GetTimeMS()<timeout )
		{
			// Get recvfrom to unblock
			for (i=0; i < socketList.Size(); i++)
			{
				SocketLayer::SendTo(socketList[i], (const char*) &i,1,socketList[i]->GetBoundAddress(), _FILE_AND_LINE_);
			}

			RakSleep(30);
		}
		*/

#if !defined(__native_client__) && !defined(WINDOWS_STORE_RT)
		for (unsigned int i = 0; i < socketList.Size(); i++)
		{
			if (socketList[i]->IsBerkleySocket())
			{
				((RNS2_Berkley *)socketList[i])->BlockOnStopRecvPollingThread();
			}
		}
#endif
	}
}
