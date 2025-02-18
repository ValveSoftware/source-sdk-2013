//====== Copyright (c), Valve Corporation, All rights reserved. =======
//
// Purpose: Holds the CNetPacket class
//
//=============================================================================

#ifndef GCNETPACKET_H
#define GCNETPACKET_H

namespace GCSDK
{

//-----------------------------------------------------------------------------
// Purpose: reference-counted received network packet
//-----------------------------------------------------------------------------
class CNetPacket
{
public:
	CNetPacket();
	~CNetPacket();

	//called to allocate a buffer for the net packet of the specified size. This takes an optional pointer
	//of which it will copy into the data if appropriate
	void Init( uint32 cubData, const void* pCopyData = NULL );

	//called when working with a net packet that you want to reference a separate buffer
	void InitAdoptBuffer( uint32 cubData, uint8* pubData );
	void OrphanBuffer();

	// data
	uint8 *PubData() const				{ return m_pubData; }
	uint CubData() const 				{ return m_cubData; }

	// ownership
	void AddRef();
	void Release();
	
private:
	int m_cRef;					// reference count, deletes self when 0
	uint m_cubData;
	uint8 *m_pubData;
};



} // namespace GCSDK

#endif // GCNETPACKET_H
