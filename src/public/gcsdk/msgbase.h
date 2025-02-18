//====== Copyright ©, Valve Corporation, All rights reserved. =======
//
// Purpose: Holds the CMsgBase_t class
//
//=============================================================================

#ifndef GCMSGBASE_H
#define GCMSGBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk_types.h"
#include "tier1/tsmultimempool.h"
#include "gclogger.h"
#include "gcconstants.h"
#include "refcount.h"

namespace GCSDK
{

class CNetPacket;

// used for message types in GCSDK where we don't have the actual enum
typedef uint32 MsgType_t;
const uint32 k_EMsgProtoBufFlag = 0x80000000;

//extern ConVar g_ConVarMsgErrorDump;
extern CThreadSafeMultiMemoryPool g_MemPoolMsg;

enum EMsgFormatType
{
	k_EMsgFormatTypeStruct = 0,
	k_EMsgFormatTypeClientStruct = 1,
	k_EMsgFormatTypeClientStructDeprecated = 2,
	k_EMsgFormatTypeProtocolBuffer = 3
};


// 
// Interface that the CNetPacket wrappers for both old/new inter-server 
// message formats must implement.
//
class IMsgNetPacket : public CRefCount
{
public:

	virtual EMsgFormatType GetEMsgFormatType() const = 0;

	virtual CNetPacket *GetCNetPacket() const = 0;
	virtual uint8 *PubData() const = 0;
	virtual uint CubData() const = 0;

	//
	// Inter-server or client messages in both old/new formats
	//
	virtual MsgType_t GetEMsg() const = 0;
	virtual JobID_t GetSourceJobID() const = 0;
	virtual JobID_t GetTargetJobID() const = 0;
	virtual void SetTargetJobID( JobID_t ulJobID ) = 0;

	//
	// Client messages only in the old format, optional in any msg in the new format
	//
	virtual CSteamID GetSteamID() const = 0;
	virtual void SetSteamID( CSteamID steamID ) = 0;

	// Inter-gc messages only
	virtual AppId_t GetSourceAppID() const = 0;
	virtual void SetSourceAppID( AppId_t appId ) = 0;

	//
	// The name of the job type to route this message to. ProtoBuf messages only
	//
	virtual bool BHasTargetJobName() const = 0;
	virtual const char *GetTargetJobName() const = 0;

protected:

	// Needed due to CRefCount inheritance which makes this not a pure interface class
	virtual ~IMsgNetPacket() {}

};

IMsgNetPacket *IMsgNetPacketFromCNetPacket( CNetPacket *pNetPacket );


// Wrapper around IMsgNetPacket which auto-releases
class CIMsgNetPacketAutoRelease
{
public:
	CIMsgNetPacketAutoRelease( CNetPacket *pNetPacket ) { m_pMsgNetPacket = IMsgNetPacketFromCNetPacket( pNetPacket ); }
	~CIMsgNetPacketAutoRelease() { SAFE_RELEASE( m_pMsgNetPacket ); }

	void Replace( CNetPacket *pNetPacket, bool bIsClientMsg ) { SAFE_RELEASE( m_pMsgNetPacket ); m_pMsgNetPacket = IMsgNetPacketFromCNetPacket( pNetPacket ); }

	IMsgNetPacket *Get() { return m_pMsgNetPacket; }
	IMsgNetPacket *operator->()	{ return m_pMsgNetPacket; }

protected:
	void operator=( const CIMsgNetPacketAutoRelease &that ) { AssertMsg( false, "Not safe to copy since releases references on destruction" ); }
	CIMsgNetPacketAutoRelease( const CIMsgNetPacketAutoRelease &that ) { AssertMsg( false, "Not safe to copy since releases references on destruction" ); }

	IMsgNetPacket *m_pMsgNetPacket;
};


//-----------------------------------------------------------------------------
// Purpose: Helper class for incoming and outgoing network packets.
//			IMPORTANT: Note the distinction between pubData and cubData
//			(which refer to the message payload), and pubMsg and cubMsg
//			(which refer to the entire message, with the header).
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
class CMsgBase_t
{
public:
	// Send constructor
	CMsgBase_t( uint32 cubStruct, uint32 cubReserve = 0 );

	// copies data from pubPkt
	CMsgBase_t( const uint8 *pubPkt, uint32 cubPkt );

	// Receive constructor - aliases pubPkt
	CMsgBase_t( uint32 cubHdr, uint32 cubStruct, uint8 *pubPkt, uint32 cubPkt/*, HCONNECTION hConnection = NULL*/ );

	// set packet after using empty constructor
	void SetPacket( IMsgNetPacket *pNetPacket );

	// Destructor
	virtual ~CMsgBase_t();

	// Accessors
	uint8 *PubVarData() { return ( m_pubPkt + m_cubMsgHdr + m_cubStruct ); }
	const uint8 *PubVarData() const { return ( m_pubPkt + m_cubMsgHdr + m_cubStruct ); }
	uint32 CubVarData() const 
	{
		if ( m_cubPkt >= ( m_cubMsgHdr + m_cubStruct ) )
			return m_cubPkt - m_cubMsgHdr - m_cubStruct; 
		else
			return 0;
	}
	uint8 *PubPkt() { return m_pubPkt; }
	const uint8 *PubPkt() const { return m_pubPkt; }
	uint32 CubPkt() const { return m_cubPkt; }
	MSG_HEADER_TYPE &Hdr() { return * ( MSG_HEADER_TYPE * ) ( m_pubPkt ); }
	const MSG_HEADER_TYPE &Hdr() const { return * ( MSG_HEADER_TYPE * ) ( m_pubPkt ); }
	uint32 CubHdr() const { return m_cubMsgHdr; }

	uint8* PubBody() { return m_pubBody; }
	const uint8* PubBody() const { return m_pubBody; }
	uint32 CubBody() const { return CubPkt() - CubHdr(); }

	// Add additional data
	int DubWriteCur() { return m_cubPkt - m_cubMsgHdr - m_cubStruct; }				// Our current offset within the var data block
	void AddBoolData( bool bData );
	void AddUint8Data( uint8 ubData );
	void AddUintData( uint32 unData );
	void AddIntData( int32 nData );
	void AddInt16Data( int16 sData );
	void AddUint16Data( uint16 usData );
	void AddUint64Data( uint64 ulData );
	void AddInt64Data( int64 lData );
	void AddFloatData( float lData );
	void AddVariableLenData( const void *pvData, uint cubLen );	
	void AddStrData( const char *pchIn );
	template<typename T>	void AddStructure(T&	structure ) ;

	// Read variable-length data (can also read manually using PubVarData(), CubVarData()
	void ResetReadPtr() { m_pubVarRead = PubVarData(); }
	uint8 *PubReadCur() { return m_pubVarRead; }
	const uint8 *PubReadCur() const { return m_pubVarRead; }
	uint32 CubReadRemaining() const { return (uint32)(m_pubPkt + m_cubPkt - m_pubVarRead); }
	void AdvanceReadPtr( int cubSkip ) { m_pubVarRead += cubSkip; }
	bool BReadBoolData( bool *pbData );
	bool BReadUint8Data( uint8 *pbData );
	bool BReadUintData( uint32 *punData );
	bool BReadIntData( int32 *pnData );
	bool BReadInt16Data( int16 *psData );
	bool BReadUint16Data( uint16 *pusData );
	bool BReadUint64Data( uint64 *pulData );
	bool BReadInt64Data( int64 *plData );
	bool BReadFloatData( float *pflData );
	bool BReadVariableLenData( void *pvBuff, uint32 cubRead );
	bool BReadStr( char *pchBuff, int cchBuff );
	bool BReadStr( CUtlString *pstr );
	template<typename T> bool BReadStructure(T& structure) ;

	// returns pointer to data (and size of data in out ptr), and NULLs out own pointer;
	// caller now owns this memory and must free
	uint8 * DetachPkt( int * pcubPkt )
	{
		Assert( pcubPkt );
		*pcubPkt = m_cubPkt;
		uint8 * pRetVal = m_pubPkt;
		m_pubPkt = NULL;
		m_pubBody = NULL;
		m_cubPkt = 0;
		return pRetVal;
	}

	void ResetWritePtr()
	{
		m_cubPkt = m_cubMsgHdr + m_cubStruct;	
	}

	uint32 GetWriteOffset() const { return m_cubPkt; }
	void SetWriteOffset( uint32 nWriteOffset )	{ m_cubPkt = nWriteOffset; }

	// Called to set the JobID that will be expecting
	// a reply to this message.
	void ExpectingReply( JobID_t jobIDSource )
	{
		Hdr().m_JobIDSource = jobIDSource;
	}

	bool BIsExpectingReply() const  { return Hdr().m_JobIDSource != k_GIDNil; }

	// make sure the buffer can hold this extra amount of data
	void EnsurePacketSize( uint32 cubNewsize ); 
	//HCONNECTION GetHConnection() const { return m_hConnection; }

	void ReportBufferOverflow();
	void PacketDump(); // spews complete packet content to console

protected:
	// Shared by send & receive
	uint8 *m_pubPkt;				// Raw packet data
	uint8 *m_pubBody;				// pointer to body; always equal to m_pubPkt + m_cubMsgHdr
	uint32 m_cubPkt;				// Raw packet size
	const uint32 m_cubMsgHdr;		// Size of our message header
	uint32 m_cubStruct;				// Size of our message-specific struct
	//HCONNECTION m_hConnection;		// Connection on which we received the message
private:
	// stop people from hurting themselves
	CMsgBase_t( const CMsgBase_t &rhs ) {};
	CMsgBase_t &operator=( const CMsgBase_t &rhs ) {};

	bool m_bAlloced;				// Did we allocate this buffer or does someone else own it

	// Receive only
	uint8 *m_pubVarRead;			// Our current read pointer in the variable-length data
};

template <typename MSG_HEADER_TYPE>
CMsgBase_t<MSG_HEADER_TYPE>::CMsgBase_t( uint32 cubStruct, uint32 cubReserve )
: m_cubMsgHdr( sizeof( MSG_HEADER_TYPE ) )
{
	m_cubStruct = cubStruct;

	// Alloc a buffer
	m_cubPkt = m_cubMsgHdr + m_cubStruct;	
	m_pubPkt = (uint8 *) g_MemPoolMsg.Alloc( m_cubPkt + cubReserve );
	m_pubBody = m_pubPkt + m_cubMsgHdr;
	memset(m_pubPkt, 0, m_cubPkt );
	m_bAlloced = true;
	m_pubVarRead = NULL;
}


template <typename MSG_HEADER_TYPE>
CMsgBase_t<MSG_HEADER_TYPE>::CMsgBase_t( const uint8 *pubPkt, uint32 cubPkt )
: m_cubMsgHdr( 0 )
{
	m_cubStruct = 0;

	// Alloc a buffer
	m_cubPkt = cubPkt;	
	m_pubPkt = (uint8 *) g_MemPoolMsg.Alloc( m_cubPkt ); 
	m_pubBody = m_pubPkt + m_cubMsgHdr;
	Q_memcpy(m_pubPkt, pubPkt, cubPkt );
	m_bAlloced = true;
	m_pubVarRead = NULL;
}


template <typename MSG_HEADER_TYPE>
CMsgBase_t<MSG_HEADER_TYPE>::CMsgBase_t( uint32 cubHdr, uint32 cubStruct,  uint8 *pubPkt, uint32 cubPkt )
: m_cubMsgHdr( cubHdr )
{
	Assert( cubHdr != 0 );
	Assert( !cubPkt || ( cubPkt >= ( cubHdr + cubStruct ) ) );
	m_cubStruct = cubStruct;
	m_pubPkt = pubPkt;
	m_pubBody = m_pubPkt + m_cubMsgHdr;
	m_cubPkt = cubPkt;
	m_bAlloced = false;
	m_pubVarRead = PubVarData();
}


template <typename MSG_HEADER_TYPE>
CMsgBase_t<MSG_HEADER_TYPE>::~CMsgBase_t()
{
	// if we allocated memory, free it
	if ( m_bAlloced && m_pubPkt )
		g_MemPoolMsg.Free( m_pubPkt );
}


//-----------------------------------------------------------------------------
// Purpose: ensure the packet can contain at least this much extra data
// Input: cubExtraSize - the amount of bytes to have room for
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::SetPacket( IMsgNetPacket *pNetPacket )
{
	m_pubPkt = pNetPacket->PubData();
	m_pubBody = m_pubPkt + m_cubMsgHdr;
	m_cubPkt = pNetPacket->CubData();
	Assert( !m_cubPkt || ( m_cubPkt >= ( m_cubMsgHdr + m_cubStruct ) ) );
	m_bAlloced = false;
	m_pubVarRead = PubVarData();
}


//-----------------------------------------------------------------------------
// Purpose: ensure the packet can contain at least this much extra data
// Input: cubExtraSize - the amount of bytes to have room for
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::EnsurePacketSize( uint32 cubExtraSize )
{
	m_pubPkt = (uint8 *) g_MemPoolMsg.ReAlloc( m_pubPkt, m_cubPkt + cubExtraSize );
	m_pubBody = m_pubPkt + m_cubMsgHdr;
}


//-----------------------------------------------------------------------------
// Purpose: Appends a bool to the variable length data associated with this message
// Input:	bData -	data to append
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::AddBoolData( bool ubData )
{
	return AddUint8Data( static_cast< uint8 >( ubData ) );
}


//-----------------------------------------------------------------------------
// Purpose: Appends a uint8 to the variable length data associated with this message
// Input:	ubData -	data to append
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::AddUint8Data( uint8 ubData )
{
	EnsurePacketSize( sizeof( uint8 ) );
	*( ( uint8 * ) ( m_pubPkt + m_cubPkt ) ) = ubData;
	m_cubPkt += sizeof( uint8 );
}


//-----------------------------------------------------------------------------
// Purpose: Appends a uint to the variable length data associated with this message
// Input:	unData -	data to append
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>	
void CMsgBase_t<MSG_HEADER_TYPE>::AddUintData( uint32 unData )
{
	EnsurePacketSize( sizeof( uint32 ) );
	*( ( uint32 * ) ( m_pubPkt + m_cubPkt ) ) = unData;
	m_cubPkt += sizeof( uint32 );
}


//-----------------------------------------------------------------------------
// Purpose: Appends an int to the variable length data associated with this message
// Input:	nData -	data to append
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::AddIntData( int32 nData )
{
	EnsurePacketSize( sizeof( int32 ) );
	*( ( int32 * ) ( m_pubPkt + m_cubPkt ) ) = nData;
	m_cubPkt += sizeof( int32 );
}


//-----------------------------------------------------------------------------
// Purpose: Appends an int16 to the variable length data associated with this message
// Input:	nData -	data to append
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::AddInt16Data( int16 sData )
{
	EnsurePacketSize( sizeof( int16 ) );
	*( ( int16 * ) ( m_pubPkt + m_cubPkt ) ) = sData;
	m_cubPkt += sizeof( int16 );
}


//-----------------------------------------------------------------------------
// Purpose: Appends an int16 to the variable length data associated with this message
// Input:	nData -	data to append
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::AddUint16Data( uint16 usData )
{
	EnsurePacketSize( sizeof( uint16 ) );
	*( ( uint16 * ) ( m_pubPkt + m_cubPkt ) ) = usData;
	m_cubPkt += sizeof( uint16 );
}


//-----------------------------------------------------------------------------
// Purpose: Appends a uint64 to the variable length data associated with this message
// Input:	ulData -	data to append
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::AddUint64Data( uint64 ulData )
{
	EnsurePacketSize( sizeof( uint64 ) );
	*( ( uint64 * ) ( m_pubPkt + m_cubPkt ) ) = ulData;
	m_cubPkt += sizeof( uint64 );
}


//-----------------------------------------------------------------------------
// Purpose: Appends an int64 to the variable length data associated with this message
// Input:	lData -	data to append
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::AddInt64Data( int64 lData )
{
	EnsurePacketSize( sizeof( int64 ) );
	*( ( int64 * ) ( m_pubPkt + m_cubPkt ) ) = lData;
	m_cubPkt += sizeof( int64 );
}


//-----------------------------------------------------------------------------
// Purpose: Appends variable length data to this message.  (Can be called
//			repeatedly to append multiple data blocks.)
// Input:	pvData -		pointer to data to append
//			cubData	-		size of data to append
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::AddVariableLenData( const void *pvData, uint cubLen )
{
	if ( cubLen > 0 )
	{
		EnsurePacketSize( cubLen );
		memcpy( ( m_pubPkt + m_cubPkt ), pvData, cubLen );
		m_cubPkt += cubLen;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Appends a float to the variable length data associated with this message
// Input:	nData -	data to append
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::AddFloatData( float flData )
{
	EnsurePacketSize( sizeof( float ) );
	*( ( float * ) ( m_pubPkt + m_cubPkt ) ) = flData;
	m_cubPkt += sizeof( float );
}


//-----------------------------------------------------------------------------
// Purpose: Appends a string to the variable-length portion of this message.
// Input:	pchIn -			String to append to the message
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::AddStrData( const char *pchIn )
{
	if ( !pchIn )
	{
		Assert( pchIn ); // passing a null string here is a code bug
		return;
	}

	int cchIn = Q_strlen( pchIn );

	EnsurePacketSize( cchIn + 1 );

	Q_strncpy( ( char * ) ( m_pubPkt + m_cubPkt ), pchIn, cchIn + 1 );
	m_cubPkt += ( cchIn + 1 );
}

template <typename MSG_HEADER_TYPE>
template <typename T>
void CMsgBase_t<MSG_HEADER_TYPE>::AddStructure( T& structure )
{

	EnsurePacketSize(sizeof(structure)) ;
	*( reinterpret_cast<T*>(m_pubPkt+m_cubPkt)) = structure ;
	m_cubPkt += sizeof(structure) ;
}


//-----------------------------------------------------------------------------
// Purpose: Read a bool from the variable-length part of the message
// Input:	pbData -		[return] The value we read goes here
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadBoolData( bool *pbData )
{
	return BReadUint8Data( ( uint8* ) pbData );
}


//-----------------------------------------------------------------------------
// Purpose: Read a uint8 from the variable-length part of the message
// Input:	pbData -		[return] The value we read goes here
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadUint8Data( uint8 *pbData )
{
	if ( m_pubVarRead + sizeof( uint8 ) > m_pubPkt + m_cubPkt )
	{
		ReportBufferOverflow();
		return false;
	}

	*pbData = * ( ( uint8 * ) m_pubVarRead );
	m_pubVarRead += sizeof( uint8 );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Read a uint32 from the variable-length part of the message
// Input:	punData -		[return] The value we read goes here
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadUintData( uint32 *punData )
{
	if ( m_pubVarRead + sizeof( uint32 ) > m_pubPkt + m_cubPkt )
	{
		ReportBufferOverflow();
		return false;
	}

	*punData = * ( ( uint32 * ) m_pubVarRead );
	m_pubVarRead += sizeof( uint32 );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Reads an int32 from the variable-length part of the message
// Input:	pnData -		[return] The value we read goes here
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadIntData( int32 *pnData )
{
	if ( m_pubVarRead + sizeof( int32 ) > m_pubPkt + m_cubPkt )
	{
		ReportBufferOverflow();
		return false;
	}

	*pnData = * ( ( int32 * ) m_pubVarRead );
	m_pubVarRead += sizeof( int32 );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Reads an int16 from the variable-length part of the message
// Input:	pnData -		[return] The value we read goes here
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadInt16Data( int16 *psData )
{
	if ( m_pubVarRead + sizeof( int16 ) > m_pubPkt + m_cubPkt )
	{
		ReportBufferOverflow();
		return false;
	}

	*psData = * ( ( int16 * ) m_pubVarRead );
	m_pubVarRead += sizeof( int16 );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Reads an uint16 from the variable-length part of the message
// Input:	pnData -		[return] The value we read goes here
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadUint16Data( uint16 *pusData )
{
	if ( m_pubVarRead + sizeof( uint16 ) > m_pubPkt + m_cubPkt )
	{
		ReportBufferOverflow();
		return false;
	}

	*pusData = * ( ( uint16 * ) m_pubVarRead );
	m_pubVarRead += sizeof( uint16 );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Read a uint64 from the variable-length part of the message
// Input:	pulData -		[return] The value we read goes here
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadUint64Data( uint64 *pulData )
{
	if ( m_pubVarRead + sizeof( uint64 ) > m_pubPkt + m_cubPkt )
	{
		ReportBufferOverflow();
		return false;
	}

	*pulData = * ( ( uint64 * ) m_pubVarRead );
	m_pubVarRead += sizeof( uint64 );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Reads an int64 from the variable-length part of the message
// Input:	plData -		[return] The value we read goes here
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadInt64Data( int64 *plData )
{
	if ( m_pubVarRead + sizeof( int64 ) > m_pubPkt + m_cubPkt )
	{
		ReportBufferOverflow();
		return false;
	}

	*plData = * ( ( int64 * ) m_pubVarRead );
	m_pubVarRead += sizeof( int64 );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Reads a float from the variable-length part of the message
// Input:	pflData -		[return] The value we read goes here
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadFloatData( float *pflData )
{
	if ( m_pubVarRead + sizeof( float ) > m_pubPkt + m_cubPkt )
	{
		ReportBufferOverflow();
		return false;
	}

	*pflData = * ( ( float * ) m_pubVarRead );
	m_pubVarRead += sizeof( float );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Reads a block of data from the variable-length part of the message
// Input:	pvBuff -		[return] Buffer to copy the data into
//			cubRead -		Amount of data to read
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadVariableLenData( void *pvBuff, uint32 cubRead )
{
	if ( m_pubVarRead + cubRead > m_pubPkt + m_cubPkt )
	{
		ReportBufferOverflow();
		return false;
	}

	Q_memcpy( pvBuff, m_pubVarRead, cubRead );
	m_pubVarRead += cubRead;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Reads a string from the variable-length part of the message
// Input:	pchBuff -		[return] Buffer to copy the string into
//			cchBuff -		Size of the buffer
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadStr( char *pchBuff, int cchBuff )
{
	int cchRead = 0;
	int cchLeft = CubReadRemaining();	// get bytes left in message

	// search for string end in rest of message
	while ( cchRead < cchLeft )
	{
		// if we hit the 0, stop
		if ( *(m_pubVarRead+cchRead) == 0 )
			break;

		cchRead++;
	}

	cchRead++; // add the 0

	// check if string fits into buffer and was found within packet bounds
	if ( ( cchRead > cchBuff ) || (cchRead > cchLeft) )
	{
		// at least return an empty string since most code doesn't check the return value
		if ( cchBuff > 0 )
			pchBuff[0] = 0;

		ReportBufferOverflow();

		return false;
	}

	// copy the string to output buffer
	Q_memcpy( pchBuff, m_pubVarRead, cchRead );
	m_pubVarRead += ( cchRead * sizeof( char ) );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Reads a string from the variable-length part of the message
// Input:	pchString -		[return] copied string
// Output:	true if we were able to read, false if we overran the buffer
//-----------------------------------------------------------------------------
template <typename MSG_HEADER_TYPE>
bool CMsgBase_t<MSG_HEADER_TYPE>::BReadStr( CUtlString *pstr )
{
	if ( pstr == NULL )
		return false;

	int cchRead = 0;
	int cchLeft = CubReadRemaining();	// get bytes left in message

	// search for string end in rest of message
	while ( cchRead < cchLeft )
	{
		// if we hit the 0, stop
		if ( *(m_pubVarRead+cchRead) == 0 )
			break;

		cchRead++;
	}

	cchRead++; // add the 0

	// check if string was found within packet bounds
	if ( cchRead > cchLeft )
	{
		ReportBufferOverflow();
		return false;
	}

	// copy the string
	*pstr = (const char*)m_pubVarRead;
	m_pubVarRead += ( cchRead * sizeof( char ) );

	return true;
}

template <typename MSG_HEADER_TYPE>
template <typename T>
bool	CMsgBase_t<MSG_HEADER_TYPE>::BReadStructure( T& structure )
{
	int		cbLeft = CubReadRemaining() ;

	if( cbLeft >= sizeof(structure))
	{
		structure = *( reinterpret_cast<T*>(m_pubVarRead)) ;
		m_pubVarRead += sizeof(structure) ;
		return true ;
	}
	return false ;
}



template <typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::PacketDump()
{
	//if ( !g_ConVarMsgErrorDump.GetBool() )
	//	return;

	EmitInfo( SPEW_NETWORK, SPEW_NEVER, LOG_ALWAYS, "Packet dump: raw size %u, header size %u, body size %u, var size %u\n", m_cubPkt, m_cubMsgHdr, m_cubStruct, CubVarData() );

	EmitInfo( SPEW_NETWORK, SPEW_NEVER, LOG_ALWAYS, "Header dump: %s\n", Hdr().GetHeaderDescription().String() );

	EmitInfo( SPEW_NETWORK, SPEW_NEVER, LOG_ALWAYS, "Struct dump: %u bytes\n", m_cubStruct );
	char szLine[100] = "";
	char szText[32] = "";

	for ( uint i=0; i<m_cubStruct; i++ )
	{
		byte nValue = PubBody()[i];
		uint nIndex = i%16;

		Q_snprintf( szLine+3*nIndex, 8, "%02X ", nValue );

		if ( nValue > 31 && nValue != '%' )
			szText[nIndex] = nValue;
		else
			szText[nIndex] = '.';

		if ( nIndex == 15 || i==(m_cubStruct-1))
		{
			szText[nIndex+1] = '\n';
			szText[nIndex+2] = 0;
			Q_strcat( szLine, "; ", sizeof(szLine) );
			Q_strcat( szLine, szText, sizeof(szLine) );
			EmitInfo( SPEW_NETWORK, SPEW_NEVER, LOG_ALWAYS, "%s", szLine );
			szLine[0]=0;
		}
	}

	uint cubVarData = MIN( CubVarData(), 1024u );

	EmitInfo( SPEW_NETWORK, SPEW_NEVER, LOG_ALWAYS, "VarData dump: %u bytes\n", cubVarData );

	for ( uint i=0; i<cubVarData; i++ )
	{
		byte nValue = PubVarData()[i];
		uint nIndex = i%16;

		Q_snprintf( szLine+3*nIndex, 8, "%02X ", nValue );

		if ( nValue > 31 && nValue != '%' )
			szText[nIndex] = nValue;
		else
			szText[nIndex] = '.';

		if ( nIndex == 15 || i==(cubVarData-1))
		{
			szText[nIndex+1] = '\n';
			szText[nIndex+2] = 0;
			Q_strcat( szLine, " ; ", sizeof(szLine) );
			Q_strcat( szLine, szText, sizeof(szLine) );
			EmitInfo( SPEW_NETWORK, SPEW_NEVER, LOG_ALWAYS, "%s", szLine );
			szLine[0]=0;
		}
	}
}

template<typename MSG_HEADER_TYPE>
void CMsgBase_t<MSG_HEADER_TYPE>::ReportBufferOverflow()
{
	EmitWarning( SPEW_NETWORK, SPEW_ALWAYS, "Read buffer overflowed on incoming %s packet\n", Hdr().PchMsgName( ) );
	PacketDump();
}

} // namespace GCSDK

#endif // GCMSGBASE_H
