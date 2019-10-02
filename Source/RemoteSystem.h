#pragma once

#include "ReliabilityLayer.h"
#include "rnet/NetworkSimulator.h"

namespace RakNet
{
    /// \brief Holds the clock differences between systems, along with the ping
    struct PingAndClockDifferential
    {
        unsigned short pingTime;
        RakNet::Time clockDifferential;
    };

    /// \internal
    /// \brief All the information representing a connected system
    struct RemoteSystem
    {
        bool isActive; // Is this structure in use?
        SystemAddress systemAddress;  /// Their external IP on the internet
        SystemAddress myExternalSystemAddress;  /// Your external IP on the internet, from their perspective
        SystemAddress theirInternalSystemAddress[MAXIMUM_NUMBER_OF_INTERNAL_IDS];  /// Their internal IP, behind the LAN
        ReliabilityLayer reliabilityLayer;  /// The reliability layer associated with this player
        bool weInitiatedTheConnection; /// True if we started this connection via Connect.  False if someone else connected to us.
        PingAndClockDifferential pingAndClockDifferential[PING_TIMES_ARRAY_SIZE];  /// last x ping times and calculated clock differentials with it
        RakNet::Time pingAndClockDifferentialWriteIndex;  /// The index we are writing into the pingAndClockDifferential circular buffer
        unsigned short lowestPing; ///The lowest ping value encountered
        RakNet::Time nextPingTime;  /// When to next ping this player
        RakNet::Time lastReliableSend; /// When did the last reliable send occur.  Reliable sends must occur at least once every timeoutTime/2 units to notice disconnects
        RakNet::Time connectionTime; /// connection time, if active.
    //		int connectionSocketIndex; // index into connectionSockets to send back on.
        RakNetGUID guid;
        uint16_t MTUSize;
        // Reference counted socket to send back on
        RakNetSocket2* rakNetSocket;
        SystemIndex remoteSystemIndex;

#if LIBCAT_SECURITY==1
        // Cached answer used internally by RakPeer to prevent DoS attacks based on the connexion handshake
        char answer[cat::EasyHandshake::ANSWER_BYTES];

        // If the server has bRequireClientKey = true, then this is set to the validated public key of the connected client
        // Valid after connectMode reaches HANDLING_CONNECTION_REQUEST
        char client_public_key[cat::EasyHandshake::PUBLIC_KEY_BYTES];
#endif

        enum ConnectMode { NO_ACTION, DISCONNECT_ASAP, DISCONNECT_ASAP_SILENTLY, DISCONNECT_ON_NO_ACK, REQUESTED_CONNECTION, HANDLING_CONNECTION_REQUEST, UNVERIFIED_SENDER, CONNECTED } connectMode;
    };
}