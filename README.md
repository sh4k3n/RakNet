RakNet/KCP
============

This is a heavily improved RakNet fork. The biggest change is replacing RakNet ARQ protocol with KCP as 
RakNet's Sliding Window has bugs and UDT is mediocre for real time multiplayer games. 
KCP provides better latency than Sliding Window or UDT by sacrificing some bandwidth.

Key features
------------------------------------------
- New ARQ protocol: KCP
- Unit tested
- Various RakNet fixes


Issues and deviations from RakNet
-----------------------------------------
- Supports only "unreliable" and "reliable ordered" messaging. 
	- For reliable unordered data use "reliable ordered" messaging, but send messages in different channels, when they do not have to be in order
	- It should be trivial to create own protocol for "unreliable with ack" type of messaging
- Several minor issues marked with To-Do comments in code
- Encryption and plugin supports are broken

Package notes
------------------------------------------
The Help directory contains index.html, which is full help documentation in HTML format
The Source directory contain all files required for the core of Raknet and is used if you want to use the source in your program or create your own dll
The Samples directory contains code samples and one game using an older version of Raknet.  The code samples each demonstrate one feature of Raknet.  The game samples cover several features.
The Tests directory contains unit tests
The lib directory contains libs for debug and release versions of RakNet and RakVoice

C# support
------------------------------------------

See Help\swigtutorial.html

How to setup
-----------------------------------------
Create a work directy and from the work directory call "cmake [RakNet root directory]"



