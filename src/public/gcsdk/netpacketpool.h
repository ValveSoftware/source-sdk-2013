//====== Copyright (c), Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef GCNETPACKETPOOL_H
#define GCNETPACKETPOOL_H

#include "netpacket.h"

namespace GCSDK
{

class CNetPacket;

extern int g_cNetPacket;
extern CThreadSafeMultiMemoryPool g_MemPoolMsg;


class CNetPacketPool
{
public:

#ifdef _SERVER
	static int  CMBPacketMemPool() { return ( CNetPacketPool::sm_MemPoolNetPacket.CubTotalSize() / k_nMegabyte ); }
	static int  CMBPacketMemPoolInUse() { return ( CNetPacketPool::sm_MemPoolNetPacket.CubSizeInUse() / k_nMegabyte ); }
#endif // _SERVER

	static CNetPacket *AllocNetPacket() { g_cNetPacket++; return sm_MemPoolNetPacket.Alloc(); }

private:
	friend class CNetPacket;

	static CClassMemoryPool<CNetPacket> sm_MemPoolNetPacket;

};


} // namespace GCSDK

#endif // GCNETPACKETPOOL_H
