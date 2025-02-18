//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef MSGPROTOBUF_H
#define MSGPROTOBUF_H
#ifdef _WIN32
#pragma once
#endif

#include "msgbase.h"
#include "gcmsg.h"
#include "tier0/tslist.h"

// eliminates a conflict with TYPE_BOOL in OSX
#ifdef TYPE_BOOL
#undef TYPE_BOOL
#endif

#pragma warning(push)
#pragma warning( disable:4512 )
#include <tier0/valve_minmax_off.h>
#include "steammessages.pb.h"
#include <tier0/valve_minmax_on.h>
#pragma warning(pop)


namespace GCSDK
{

//-----------------------------------------------------------------------------
// CProtoBufNetPacket
// Thin wrapper around raw CNetPacket which implements our IMsgNetPacket interface.
//-----------------------------------------------------------------------------
class CProtoBufNetPacket : public IMsgNetPacket
{

public:
	CProtoBufNetPacket( CNetPacket *pNetPacket, GCProtoBufMsgSrc eReplyType, const CSteamID steamID, uint32 nGCDirIndex, MsgType_t msgType );

	//IMsgNetPacket

	virtual EMsgFormatType GetEMsgFormatType() const OVERRIDE { return k_EMsgFormatTypeProtocolBuffer; }
	virtual CNetPacket *GetCNetPacket() const OVERRIDE { return m_pNetPacket; }
	virtual uint8 *PubData() const OVERRIDE { return m_pNetPacket->PubData(); }
	virtual uint CubData() const OVERRIDE { return m_pNetPacket->CubData(); }

	virtual MsgType_t GetEMsg() const OVERRIDE { return m_msgType; }
	virtual JobID_t GetSourceJobID() const OVERRIDE { return m_pHeader->job_id_source(); }
	virtual JobID_t GetTargetJobID() const OVERRIDE { return m_pHeader->job_id_target(); }
	virtual void SetTargetJobID( JobID_t ulJobID ) OVERRIDE { m_pHeader->set_job_id_target( ulJobID ); }

	virtual CSteamID GetSteamID() const OVERRIDE { return m_steamID; }
	virtual void SetSteamID( CSteamID steamID ) OVERRIDE { m_steamID = steamID; }

	virtual AppId_t GetSourceAppID() const OVERRIDE { return m_pHeader->source_app_id(); };
	virtual void SetSourceAppID( AppId_t appId ) OVERRIDE { m_pHeader->set_source_app_id( appId ); }

	virtual bool BHasTargetJobName() const OVERRIDE { return m_pHeader->has_target_job_name(); }
	virtual const char *GetTargetJobName() const OVERRIDE { return m_pHeader->target_job_name().c_str(); }


	bool IsValid() const { return m_bIsValid; }
	ProtoBufMsgHeader_t &GetFixedHeader() const { return *( ( ProtoBufMsgHeader_t * )PubData() ); }
	CMsgProtoBufHeader *GetProtoHeader() const { return m_pHeader; }

	//called to obtain access to the body portion of the net packet associated with this message. Will return NULL if not in a valid state
	bool GetMsgBody( const uint8*& pubData, uint32& cubData ) const;

private:
	virtual ~CProtoBufNetPacket();

	CNetPacket *m_pNetPacket;
	CMsgProtoBufHeader *m_pHeader;
	CSteamID  m_steamID; 
	MsgType_t m_msgType;
	bool m_bIsValid;
};


//-----------------------------------------------------------------------------
// CProtoBufMsgBase - Base class for templated protobuf msgs. As much code is
//	in this class as possible to reduce template copy overhead
//-----------------------------------------------------------------------------
class CProtoBufMsgBase
{
public:
	// allows any kind of destination to be the target of an AsyncSend
	class IProtoBufSendHandler
	{
	public:
		virtual bool BAsyncSend( MsgType_t eMsg, const uint8 *pubMsgBytes, uint32 cubSize ) = 0;
	};

	// Receive constructor. We expect InitFromPacket will be called later
	CProtoBufMsgBase();
	// Send constructor. InitFromPacket must not be called later
	CProtoBufMsgBase( MsgType_t eMsgType );

	virtual ~CProtoBufMsgBase();

	bool InitFromPacket( IMsgNetPacket * pNetPacket );
	bool BAsyncSend( IProtoBufSendHandler & pSender ) const;
	bool BAsyncSendWithPreSerializedBody( IProtoBufSendHandler & pSender, const byte *pubBody, uint32 cubBody ) const;
	//free standing version to send a protobuff given a header and pre-serialized body. Primarily used for efficient message routing
	static bool BAsyncSendWithPreSerializedBody( IProtoBufSendHandler& sender, MsgType_t eMsgType, const CMsgProtoBufHeader& hdr, const byte* pubBody, uint32 cubBody );
	//similar to the above, but sends a protobuf object that will be serialized into the buffer
	static bool BAsyncSendProto( IProtoBufSendHandler& sender, MsgType_t eMsgType, const CMsgProtoBufHeader& hdr, const ::google::protobuf::Message& proto );

	CMsgProtoBufHeader		 &Hdr()				{ return *m_pProtoBufHdr; }
	const CMsgProtoBufHeader &Hdr() const		{ return *m_pProtoBufHdr; }
	const CMsgProtoBufHeader &ConstHdr() const	{ return *m_pProtoBufHdr; }

	MsgType_t	GetEMsg()			const		{ return m_eMsg & (~k_EMsgProtoBufFlag); }
	CSteamID	GetClientSteamID()	const		{ return CSteamID( m_pProtoBufHdr->client_steam_id() ); }
	JobID_t		GetJobIDTarget()	const		{ return m_pProtoBufHdr->job_id_target(); }
	JobID_t		GetJobIDSource()	const		{ return m_pProtoBufHdr->job_id_source(); }
	AppId_t		GetSourceAppID()	const		{ return m_pProtoBufHdr->source_app_id(); }
	bool		BIsExpectingReply()	const		{ return GetJobIDSource() != k_GIDNil; }

	void SetJobIDSource( JobID_t jobId )		{ m_pProtoBufHdr->set_job_id_source( jobId ); }
	void SetJobIDTarget( JobID_t jobId )		{ m_pProtoBufHdr->set_job_id_target( jobId ); }
	void SetSourceAppID( AppId_t appId )		{ m_pProtoBufHdr->set_source_app_id( appId ); }
	void ExpectingReply( JobID_t jobId )		{ SetJobIDSource( jobId ); }

	EResult		GetEResult() const { return (EResult)ConstHdr().eresult(); }
	void		SetEResult( EResult eResult ) { Hdr().set_eresult( eResult ); }
	const char *GetErrorMessage() const { return ConstHdr().error_message().c_str(); }
	void		SetErrorMessage( const char *pchErrorMessage ) { Hdr().set_error_message( pchErrorMessage ); }
	void		AppendErrorMessage( const char *pchErrorMessage ) { Hdr().mutable_error_message()->append( pchErrorMessage ); }

	// Must be implemented by subclasses. Returns the body. The templated subclasses have their
	// own body accessor that returns the body as the specific type
	virtual ::google::protobuf::Message *GetGenericBody() const = 0;

protected:

	// Mutex to use when registering a new pool type
	static CThreadMutex s_PoolRegMutex;

private:

	//utility function that handles allocating a memory pool big enough for the provided header and specified body
	//size and writing the header into the pool. This will return a pointer to the memory, as well as the header size.
	static uint8* AllocateMessageMemory( MsgType_t eMsgType, const CMsgProtoBufHeader& hdr, uint32 cubBodySize, uint32* pCubTotalSizeOut );
	//called to free the memory returned by allocate message memory
	static void FreeMessageMemory( uint8* pMemory );
	
	// Pointer to an external net packet if we have one. If we have one then we will
	// not have allocated m_pProtoBufHdr ourselves
	CProtoBufNetPacket *m_pNetPacket;

	// Protobuf objects for extended pb based header
	CMsgProtoBufHeader *m_pProtoBufHdr;

	// Our message type
	MsgType_t m_eMsg;

	// Private and unimplemented. Implement these if you want to be able to copy
	// these objects.
	CProtoBufMsgBase( const CProtoBufMsgBase& );
	CProtoBufMsgBase& operator=( const CProtoBufMsgBase& );
};


//-----------------------------------------------------------------------------
// CProtoBufMsgMemoryPoolBase - Interface to allocation pools for each protobufmsg type
//-----------------------------------------------------------------------------
class CProtoBufMsgMemoryPoolBase
{
public:
	CProtoBufMsgMemoryPoolBase( uint32 unTargetLow, uint32 unTargetHigh );
	virtual ~CProtoBufMsgMemoryPoolBase();

	// Memory interface
	::google::protobuf::Message *Alloc();
	void Free( ::google::protobuf::Message *pMsg );

	// Stats
	uint32 GetEstimatedSize();
	uint32 GetAllocated()			{ return m_unAllocated; }
	uint32 GetFree()				{ return m_pTSQueueFreeObjects->Count(); }
	uint32 GetAllocHitCount()		{ return m_unAllocHitCounter; }
	uint32 GetAllocMissCount()		{ return m_unAllocMissCounter; }

	// To be overriden by the templated class
	virtual CUtlString GetName() = 0;

protected:
	// The actual memory management. Must be overriden by the templated class
	virtual google::protobuf::Message *InternalAlloc() = 0;
	virtual void InternalFree( google::protobuf::Message *pMsg ) = 0;
	
	// Called by the derived destructor to deallocate the outstanding messages
	bool PopItem( google::protobuf::Message **ppMsg );

private:
	CTSQueue<google::protobuf::Message *> *m_pTSQueueFreeObjects;

	// These counters are important to get correct, so interlocked in case of allocating on threads
	CInterlockedInt m_unAllocHitCounter;
	CInterlockedInt m_unAllocMissCounter;
	CInterlockedInt m_unAllocated;

	// Only set at construction, so not needed to be thread safe
	uint32 m_unTargetCountLow;
	uint32 m_unTargetCountHigh;
};

} // namespace GCDSK

// The rest of the file needs memdbgon because the code in the templates do actual allocation
#include "tier0/memdbgon.h"

namespace GCSDK
{

//-----------------------------------------------------------------------------
// CProtoBufMsgMemoryPool - Implementation for allocation pools for protobufmsgs.
// We create one of these per protobuf msg type, created on first construction of
// an object of that type.
//-----------------------------------------------------------------------------
template< typename PB_OBJECT_TYPE > 
class CProtoBufMsgMemoryPool : public CProtoBufMsgMemoryPoolBase
{
public:
	CProtoBufMsgMemoryPool()
		: CProtoBufMsgMemoryPoolBase( PB_OBJECT_TYPE::descriptor()->options().GetExtension( msgpool_soft_limit ), 
									  PB_OBJECT_TYPE::descriptor()->options().GetExtension( msgpool_hard_limit ) ) {}
	virtual ~CProtoBufMsgMemoryPool()
	{
		google::protobuf::Message *pObject = NULL;
		while ( PopItem( &pObject ) )
		{
			InternalFree( pObject );
		}
	}

	virtual CUtlString GetName() OVERRIDE
	{ 
		return PB_OBJECT_TYPE::default_instance().GetTypeName().c_str(); 
	}

private:
	virtual ::google::protobuf::Message *InternalAlloc()
	{
		PB_OBJECT_TYPE *pObject = (PB_OBJECT_TYPE *)malloc( sizeof( PB_OBJECT_TYPE ) );
		Construct( pObject );
		return pObject;
	}

	virtual void InternalFree( google::protobuf::Message *pMsg )
	{
		if ( NULL == pMsg )
		{
			Assert( NULL != pMsg );
			return;
		}

		PB_OBJECT_TYPE *pObject = (PB_OBJECT_TYPE *)pMsg;
		Destruct( pObject );
		FreePv( pObject );
	}
};


//-----------------------------------------------------------------------------
// CProtoBufMsgMemoryPoolMgr - Manages all the message pools for protobufmsgs.  
// Should have one global singleton instance of this which tracks all the pools
// for individual message types.
//-----------------------------------------------------------------------------
class CProtoBufMsgMemoryPoolMgr
{
public:
	CProtoBufMsgMemoryPoolMgr();
	~CProtoBufMsgMemoryPoolMgr();

	void RegisterPool( CProtoBufMsgMemoryPoolBase *pPool );
	void DumpPoolInfo();

	CMsgProtoBufHeader *AllocProtoBufHdr()					{ return (CMsgProtoBufHeader *)m_PoolHeaders.Alloc(); }
	void FreeProtoBufHdr( CMsgProtoBufHeader *pObject )		{ m_PoolHeaders.Free( pObject ); }

private:
	CProtoBufMsgMemoryPool<CMsgProtoBufHeader> m_PoolHeaders;
	CUtlVector< CProtoBufMsgMemoryPoolBase * > m_vecMsgPools;
};

extern CProtoBufMsgMemoryPoolMgr *GProtoBufMsgMemoryPoolMgr();


//-----------------------------------------------------------------------------
// CProtoBufPtrMsg
// Similar to a CProtoBufMsg, but the constructor simply takes in a pointer which is a
// pointer to the protobuf object that is being wrapped by a message. This memory is managed
// by the caller, this object does nothing to free the memory
//-----------------------------------------------------------------------------
class CProtoBufPtrMsg : public CProtoBufMsgBase
{
public:
	CProtoBufPtrMsg( google::protobuf::Message *pProto ) : m_pProtoBufBody( pProto )	{}

private:
	virtual google::protobuf::Message *GetGenericBody() const OVERRIDE { return m_pProtoBufBody; }

	// Protobuf object for the message body
	google::protobuf::Message *m_pProtoBufBody;

	// Private and unimplemented. Implement these if you want to be able to copy
	// these objects.
	CProtoBufPtrMsg( const CProtoBufPtrMsg& );
	CProtoBufPtrMsg& operator=( const CProtoBufPtrMsg& );
};


//-----------------------------------------------------------------------------
// CProtoBufMsg
// New style steam inter-server message class based on Google Protocol Buffers
// Handles a message with a header of type MsgHdr_t, a body of type T, and optional variable length data
//-----------------------------------------------------------------------------
template< typename PB_OBJECT_TYPE > 
class CProtoBufMsg : public CProtoBufMsgBase
{
private:
	static bool s_bRegisteredWithMemoryPoolMgr;
	static CProtoBufMsgMemoryPool< PB_OBJECT_TYPE > *s_pMemoryPool;

public:

	// Used to alloc a protobuf of this type from the pool. Can be used by functions
	// working with protobufs that aren't messages to take advantage of pooling
	static PB_OBJECT_TYPE *AllocProto()
	{
		// If we haven't done registration do so now
		// Called on construction of each object of this type, but only does work
		// once to setup memory pools for the class type.
		if ( !s_bRegisteredWithMemoryPoolMgr )
		{
			// Get the lock and make sure we still haven't
			s_PoolRegMutex.Lock();
			if ( !s_bRegisteredWithMemoryPoolMgr )
			{
				s_pMemoryPool = new CProtoBufMsgMemoryPool< PB_OBJECT_TYPE >();
				GProtoBufMsgMemoryPoolMgr()->RegisterPool( s_pMemoryPool );
				s_bRegisteredWithMemoryPoolMgr = true;
			}
			s_PoolRegMutex.Unlock();
		}

		return static_cast<PB_OBJECT_TYPE *>( s_pMemoryPool->Alloc() );
	}

	// Frees a protobuf allocated with AllocProto()
	static void FreeProto( PB_OBJECT_TYPE *pbObj )
	{
		s_pMemoryPool->Free( pbObj );
	}


	// Constructor for an empty message
	CProtoBufMsg( MsgType_t eMsg ) 
		: CProtoBufMsgBase( eMsg )
		, m_pProtoBufBody( NULL )
	{ 
		VPROF_BUDGET( "CProtoBufMsg::CProtoBufMsg( MsgType_t )", VPROF_BUDGETGROUP_OTHER_NETWORKING );
		m_pProtoBufBody = AllocProto();
	}

	// Constructor for an empty message responding to a client
	CProtoBufMsg( MsgType_t eMsg, CSteamID steamIDClient, int32 nSessionIDClient ) 
		: CProtoBufMsgBase( eMsg )
		, m_pProtoBufBody( NULL )
	{ 
		VPROF_BUDGET( "CProtoBufMsg::CProtoBufMsg( MsgType_t, CSteamID, int32 )", VPROF_BUDGETGROUP_OTHER_NETWORKING );

		m_pProtoBufBody = AllocProto();
		Hdr().set_client_steam_id( steamIDClient.ConvertToUint64() );
		Hdr().set_client_session_id( nSessionIDClient );
	}

	// Constructor from an incoming netpacket
	CProtoBufMsg( IMsgNetPacket *pNetPacket )
		: CProtoBufMsgBase()
		, m_pProtoBufBody( NULL )
	{
		m_pProtoBufBody = AllocProto();
		InitFromPacket( pNetPacket );
	}

	// constructor for use in catching replies or any other place where you have nothing to stuff in
	// the message at construct time
	CProtoBufMsg()
		: CProtoBufMsgBase()
		, m_pProtoBufBody( NULL )
	{
		m_pProtoBufBody = AllocProto();
	}

	// Constructor for replying to another protobuf message
	CProtoBufMsg( MsgType_t eMsg, const CProtoBufMsgBase & msgReplyingTo )
		: CProtoBufMsgBase( eMsg )
		, m_pProtoBufBody( NULL )
	{
		VPROF_BUDGET( "CProtoBufMsg::CProtoBufMsg( EMsg, CProtoBufMsgMemoryPoolBase )", VPROF_BUDGETGROUP_OTHER_NETWORKING );
		m_pProtoBufBody = AllocProto();

		// set up the actual reply
		SetJobIDTarget( msgReplyingTo.GetJobIDSource() );
	}

	// Destructor
	virtual ~CProtoBufMsg()
	{
		if ( m_pProtoBufBody )
		{
			FreeProto( m_pProtoBufBody );
			m_pProtoBufBody = NULL;
		}
	}

	// Accessors
	PB_OBJECT_TYPE &Body() { return *m_pProtoBufBody; }
	const PB_OBJECT_TYPE &Body() const { return *m_pProtoBufBody; }

private:
	virtual google::protobuf::Message *GetGenericBody() const OVERRIDE { return m_pProtoBufBody; }

	// Protobuf object for the message body
	PB_OBJECT_TYPE *m_pProtoBufBody;

	// Private and unimplemented. Implement these if you want to be able to copy
    // these objects.
    CProtoBufMsg( const CProtoBufMsg& );
    CProtoBufMsg& operator=( const CProtoBufMsg& );
};

// Statics
template< typename PB_OBJECT_TYPE > bool CProtoBufMsg< PB_OBJECT_TYPE>::s_bRegisteredWithMemoryPoolMgr = false;
template< typename PB_OBJECT_TYPE > CProtoBufMsgMemoryPool< PB_OBJECT_TYPE > *CProtoBufMsg< PB_OBJECT_TYPE>::s_pMemoryPool = NULL;


//-----------------------------------------------------------------------------
// Purpose: Wrapper class to handle alloc/free using the pool allocators
//-----------------------------------------------------------------------------
template <class TMsg>
class CProtoBufPoolObj
{
private:
	TMsg *m_pMsg;

private: // Disallow copying/assignment
	CProtoBufPoolObj( CProtoBufPoolObj const &x );
	CProtoBufPoolObj & operator = ( CProtoBufPoolObj const &x );

public:
	CProtoBufPoolObj()				{ m_pMsg = CProtoBufMsg<TMsg>::AllocProto(); }
	~CProtoBufPoolObj()				{ CProtoBufMsg<TMsg>::FreeProto( m_pMsg ); }

	operator TMsg & () { return *m_pMsg; }
};


} // namespace GCSDK

// memdbgon is only supposed to be on in cpp files, turn it off in case the next thing includes has a conflict with it
#include "tier0/memdbgoff.h"

#endif // MSGPROTOBUF_H
