//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This file defines all of the Game Coordinator messages for the
//			current IS-embedded implementation of the GC
//
//=============================================================================

#ifndef GCMSG_H
#define GCMSG_H
#ifdef _WIN32
#pragma once
#endif

#include "msgbase.h"
#include "messagelist.h"

#pragma pack( push, 1 )

namespace GCSDK
{

//-----------------------------------------------------------------------------
// Purpose: Header for messages from a client or gameserver to or from the GC
//-----------------------------------------------------------------------------
struct GCMsgHdr_t
{
	MsgType_t	m_eMsg;					// The message type
	uint64	m_ulSteamID;				// User's SteamID

	CUtlString GetHeaderDescription();
	const char *PchMsgName( ) const { return PchMsgNameFromEMsg( m_eMsg ); }
};


//-----------------------------------------------------------------------------
// Purpose: Header for messages from a client or gameserver to or from the GC
//			That contains source and destination jobs for the purpose of
//			replying messages.
//-----------------------------------------------------------------------------
struct GCMsgHdrEx_t
{
	MsgType_t	m_eMsg;					// The message type
	uint64	m_ulSteamID;				// User's SteamID
	uint16  m_nHdrVersion;
	JobID_t m_JobIDTarget;
	JobID_t m_JobIDSource;

	CUtlString GetHeaderDescription();
	const char *PchMsgName( ) const { return PchMsgNameFromEMsg( m_eMsg ); }
};


#pragma pack( push, 1 )
struct ProtoBufMsgHeader_t
{
	int32			m_EMsgFlagged;			// High bit should be set to indicate this message header type is in use.  The rest of the bits indicate message type.
	uint32			m_cubProtoBufExtHdr;	// Size of the extended header which is a serialized protobuf object.  Indicates where it ends and the serialized body protobuf begins.

	ProtoBufMsgHeader_t() : m_EMsgFlagged( 0 ), m_cubProtoBufExtHdr( 0 ) {}
	ProtoBufMsgHeader_t( MsgType_t eMsg, uint32 cubProtoBufExtHdr ) : m_EMsgFlagged( eMsg | k_EMsgProtoBufFlag ), m_cubProtoBufExtHdr( cubProtoBufExtHdr ) {}
	const char *PchMsgName() const { return PchMsgNameFromEMsg( GetEMsg() ); }
	MsgType_t GetEMsg() const { return (MsgType_t)(m_EMsgFlagged & (~k_EMsgProtoBufFlag) ); }
};
#pragma pack(pop)

//-----------------------------------------------------------------------------
// CStructNetPacket
// Thin wrapper around raw CNetPacket which implements our IMsgNetPacket interface.
//-----------------------------------------------------------------------------
class CStructNetPacket : public IMsgNetPacket
{

public:
	CStructNetPacket( CNetPacket *pNetPacket )
	{
		m_pHeader = (GCMsgHdrEx_t*)pNetPacket->PubData();
		m_pNetPacket = pNetPacket;
		m_pNetPacket->AddRef();
	}

	EMsgFormatType GetEMsgFormatType() const { return k_EMsgFormatTypeStruct; }
	CNetPacket *GetCNetPacket() const { return m_pNetPacket; }
	uint8 *PubData() const { return m_pNetPacket->PubData(); }
	uint CubData() const { return m_pNetPacket->CubData(); }

	MsgType_t GetEMsg() const { return (MsgType_t)m_pHeader->m_eMsg; }
	JobID_t GetSourceJobID() const { return m_pHeader->m_JobIDSource; }
	JobID_t GetTargetJobID() const { return m_pHeader->m_JobIDTarget; }
	void SetTargetJobID( JobID_t ulJobID ) { m_pHeader->m_JobIDTarget = ulJobID; }

	CSteamID GetSteamID() const { return CSteamID( m_pHeader->m_ulSteamID ); }
	void SetSteamID( CSteamID steamID )  { m_pHeader->m_ulSteamID = steamID.ConvertToUint64(); }

	AppId_t GetSourceAppID() const { return k_uAppIdInvalid; }
	void SetSourceAppID( AppId_t appId ) {}

	// Routing to a job name is not permitted with the old packet format
	virtual bool BHasTargetJobName() const { return false; }
	virtual const char *GetTargetJobName() const { return NULL; }
private:

	virtual ~CStructNetPacket()
	{
		m_pNetPacket->Release();
	}

	CNetPacket *m_pNetPacket;
	GCMsgHdrEx_t *m_pHeader;
};




#define GCMSG_EX_HEADER_SIZE	( sizeof( GCSDK::GCMsgHdrEx_t ) - sizeof( GCSDK::GCMsgHdr_t ) )

static const uint16 k_nHdrVersion = 0x1;


//-----------------------------------------------------------------------------
// Purpose: Header for messages from one GC to another
//-----------------------------------------------------------------------------
struct GCMsgInterHdr_t
{
	MsgType_t	m_eMsg;					// The message type
	uint32 m_unSourceAppId;				// App ID of the source GC
	uint16  m_nHdrVersion;
	JobID_t m_JobIDTarget;
	JobID_t m_JobIDSource;

	CUtlString GetHeaderDescription();
	const char *PchMsgName( ) const { return PchMsgNameFromEMsg( m_eMsg ); }
};



#pragma pack( pop )

//-----------------------------------------------------------------------------
// CGCMsgBase
// Message class for messages between the GC and a GS or client
// Handles a message with a header of type GCMsgHdrEx_t, a payload of type MSG_BODY_TYPE, and optional variable length data
//-----------------------------------------------------------------------------
typedef CMsgBase_t<GCMsgHdrEx_t> CGCMsgBase;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

template <typename MSG_BODY_TYPE>
class CGCMsg : public CGCMsgBase
{
public:
	// Client send constructor
	CGCMsg( MsgType_t eMsg, uint32 cubReserve = 64 ) : 
	  CGCMsgBase( sizeof( MSG_BODY_TYPE ), cubReserve ) 
	  { 
		  // Fill out the message header
		  Hdr().m_eMsg = eMsg;
		  Hdr().m_nHdrVersion = k_nHdrVersion;
		  Hdr().m_JobIDSource = k_GIDNil;
		  Hdr().m_JobIDTarget = k_GIDNil;
	  }

	  // reply constructor
	  CGCMsg( MsgType_t eMsg, const CGCMsgBase &msg, uint32 cubReserve = 64 ) : 
	  CGCMsgBase( sizeof( MSG_BODY_TYPE ), cubReserve ) 
	  { 
		  // Fill out the message header
		  Hdr().m_eMsg = eMsg;
		  Hdr().m_ulSteamID = msg.Hdr().m_ulSteamID;
		  Hdr().m_nHdrVersion = k_nHdrVersion;
		  Hdr().m_JobIDSource = k_GIDNil;
		  Hdr().m_JobIDTarget = msg.Hdr().m_JobIDSource;
	  }

	  CGCMsg( uint8 *pubPkt, uint32 cubPkt ) :
	  CGCMsgBase( sizeof( GCMsgHdrEx_t ), sizeof( MSG_BODY_TYPE ), pubPkt, cubPkt )
	  {
	  }

	  // Receive constructor
	  // Use this constructor when creating a message from a received network packet
	  CGCMsg( CIMsgNetPacketAutoRelease &refNetPacket )
		  :CGCMsgBase( sizeof( GCMsgHdrEx_t ), sizeof( MSG_BODY_TYPE ), refNetPacket->PubData(), refNetPacket->CubData() )
	  {
	  }

	  // Receive constructor
	  // Use this constructor when creating a message from a received network packet
	  CGCMsg( IMsgNetPacket *pNetPacket )
		  :CGCMsgBase( sizeof( GCMsgHdrEx_t ), sizeof( MSG_BODY_TYPE ), pNetPacket->PubData(), pNetPacket->CubData() )
	  {
	  }


	  // Receive constructor
	  // Use this constructor when creating a message from a received network packet
	  CGCMsg( CNetPacket *pNetPacket )
		  :CGCMsgBase( sizeof( GCMsgHdrEx_t ), sizeof( MSG_BODY_TYPE ), pNetPacket->PubData(), pNetPacket->CubData() )
	  {
	  }

	  // empty constructor
	  CGCMsg() :
	  CGCMsgBase( sizeof( GCMsgHdrEx_t ), sizeof( MSG_BODY_TYPE ), NULL, 0 )
	  {
	  }

	  ~CGCMsg()
	  {
	  }

	  // Accessors
	  MSG_BODY_TYPE &Body() { return * ( MSG_BODY_TYPE * ) ( m_pubPkt + m_cubMsgHdr ); }
	  const MSG_BODY_TYPE &Body() const { return * ( MSG_BODY_TYPE * ) ( m_pubPkt + m_cubMsgHdr ); }
	  GCMsgHdrEx_t * PGCMsgHdr() { return ( ( GCMsgHdrEx_t * ) m_pubPkt ); }
	  MsgType_t GetEMsg() const { return Hdr().m_eMsg; }

	  // Called to set the JobID that will be expecting
	  // a reply to this message.
	  void ExpectingReply( JobID_t jobIDSource )
	  {
		  Hdr().m_JobIDSource = jobIDSource;
	  }

	  bool BIsExpectingReply()  { return Hdr().m_JobIDSource != k_GIDNil; }
};

} // namespace GCSDK

#endif // GCMSG_H
