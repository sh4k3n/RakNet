#pragma once

#include "BasePeer.h"

namespace RakNet
{
	class RakPeer : public BasePeer
	{
	public:
		RakPeer();

		~RakPeer();

	protected:
		friend RAK_THREAD_DECLARATION(UpdateNetworkLoop);

		virtual bool StartThreads(int threadPriority) override;

		virtual void StopThreads() override;

		virtual bool RunUpdateCycle(BitStream& bitStream) override
		{
			PreUpdate(bitStream);
			PostUpdate();
			return true;
		}

	};
}
