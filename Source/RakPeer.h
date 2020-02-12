#pragma once

#include "BasePeer.h"

namespace RakNet
{
	class RakPeer : public BasePeer
	{
	public:
		RakPeer();
	protected:
		friend RAK_THREAD_DECLARATION(UpdateNetworkLoop);

		virtual bool StartThreads(int threadPriority) override;
	};
}
