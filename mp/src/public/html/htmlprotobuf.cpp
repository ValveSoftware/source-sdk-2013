//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//=============================================================================//


#include "html/htmlprotobuf.h"
#include "tier0/vprof.h"
#include "tier0/valve_minmax_off.h"


#ifdef _WIN64
// disable 64-bit warnings for the google headers
#pragma warning(push)
#pragma warning(disable:4244) // warning C4244: 'return' : conversion from '__int64' to 'int', possible loss of data
#pragma warning(disable:4267) // warning C4267: 'argument' : conversion from 'size_t' to 'int', possible loss of data
#endif

#include "tier0/memdbgoff.h"
#include "protobuf-2.3.0/src/google/protobuf/message_lite.h"
#include "tier0/memdbgon.h"

#ifdef _WIN64
#pragma warning(pop)
#endif

//-----------------------------------------------------------------------------
// Purpose: serialize a protobuf into a utlbuffer
//-----------------------------------------------------------------------------
void CHTMLBaseProtoBufMsg::SerializeCrossProc( CUtlBuffer *pBuffer ) const
{
	VPROF_BUDGET( "CUIProtoBufMsg::SerializeCrossProc", VPROF_BUDGETGROUP_OTHER_VGUI );
	uint32 unSize = ((google::protobuf::MessageLite *)m_pMsg)->ByteSize();

	// Ensure enough for type, size, and serialized data
	pBuffer->EnsureCapacity( pBuffer->TellPut() + sizeof(uint32) * 3 + unSize ); // bugbug cboyd - drop to * 2 whenpassthrough is removed below

	pBuffer->PutUnsignedInt( unSize );

	if ( unSize == 0 )
		return;

	uint8 *pBody = (uint8*)pBuffer->Base()+pBuffer->TellPut();
	((google::protobuf::MessageLite *)m_pMsg)->SerializeWithCachedSizesToArray( pBody );
	pBuffer->SeekPut( CUtlBuffer::SEEK_CURRENT, unSize );
}


//-----------------------------------------------------------------------------
// Purpose: grab a previously serialized protobuf
//-----------------------------------------------------------------------------
bool CHTMLBaseProtoBufMsg::BDeserializeCrossProc( CUtlBuffer *pBuffer )
{
	VPROF_BUDGET( "CUIProtoBufMsg::BDeserialize", VPROF_BUDGETGROUP_OTHER_VGUI );
	if ( pBuffer->GetBytesRemaining() < (int)sizeof(uint32) )
		return false;
	uint32 unSize = pBuffer->GetUnsignedInt();

	if ( unSize == 0 )
		return true;

	if ( pBuffer->GetBytesRemaining() < (int)unSize )
		return false;

	bool bSucccess = ((google::protobuf::MessageLite *)m_pMsg)->ParseFromArray( (uint8*)pBuffer->Base()+pBuffer->TellGet(), unSize );
	pBuffer->SeekGet( CUtlBuffer::SEEK_CURRENT, unSize );

	return bSucccess;
}

