//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef GCCLIENTJOB_H
#define GCCLIENTJOB_H
#ifdef _WIN32
#pragma once
#endif

namespace GCSDK
{
class CGCClient;

//-----------------------------------------------------------------------------
// Purpose: handles a network message job from the client
//-----------------------------------------------------------------------------
class CGCClientJob : public CJob
{
public:
	CGCClientJob( CGCClient *pGCClient ) : CJob( pGCClient->GetJobMgr() ), m_pGCClient( pGCClient ), m_cHeartbeatsBeforeTimeout( k_cJobHeartbeatsBeforeTimeoutDefault ) {}

	// all GCClient jobs must implement one of these
	virtual bool BYieldingRunGCJob( IMsgNetPacket *pNetPacket )	{ return false; }
	virtual bool BYieldingRunGCJob()							{ return false; }

	virtual EServerType GetServerType() { return k_EServerTypeGCClient; }

protected:
	CGCClient *m_pGCClient;

	bool BYldSendMessageAndGetReply( CGCMsgBase &msgOut, uint nTimeoutSec, CGCMsgBase *pMsgIn, MsgType_t eMsg )
	{
		IMsgNetPacket *pNetPacket = NULL;

		if ( !BYldSendMessageAndGetReply( msgOut, nTimeoutSec, &pNetPacket ) )
			return false;

		pMsgIn->SetPacket( pNetPacket );

		if ( pMsgIn->Hdr().m_eMsg != eMsg )
			return false;

		return true;
	}

	bool BYldSendMessageAndGetReply( CGCMsgBase &msgOut, uint nTimeoutSec, IMsgNetPacket **ppNetPacket )
	{
		msgOut.ExpectingReply( GetJobID() );

		if ( !m_pGCClient->BSendMessage( msgOut ) )
			return false;

		SetJobTimeout( nTimeoutSec );
		return BYieldingWaitForMsg( ppNetPacket );
	}

	enum BYldSendMessageAndGetReply_t
	{
		BYLDREPLY_SUCCESS,
		BYLDREPLY_SEND_FAILED,
		BYLDREPLY_TIMEOUT,
		BYLDREPLY_MSG_TYPE_MISMATCH,
	};
	BYldSendMessageAndGetReply_t BYldSendMessageAndGetReplyEx( CProtoBufMsgBase &msgOut, uint nTimeoutSec, CProtoBufMsgBase *pMsgIn, MsgType_t eMsg )
	{
		IMsgNetPacket *pNetPacket = NULL;

		msgOut.ExpectingReply( GetJobID() );

		if ( !m_pGCClient->BSendMessage( msgOut ) )
			return BYLDREPLY_SEND_FAILED;

		SetJobTimeout( nTimeoutSec );
		if( !BYieldingWaitForMsg( &pNetPacket ) )
			return BYLDREPLY_TIMEOUT;

		pMsgIn->InitFromPacket( pNetPacket );

		if ( pMsgIn->GetEMsg() != eMsg )
			return BYLDREPLY_MSG_TYPE_MISMATCH;

		return BYLDREPLY_SUCCESS;
	}

	bool BYldSendMessageAndGetReply( CProtoBufMsgBase &msgOut, uint nTimeoutSec, CProtoBufMsgBase *pMsgIn, MsgType_t eMsg )
	{
		if( BYldSendMessageAndGetReplyEx( msgOut, nTimeoutSec, pMsgIn, eMsg ) != BYLDREPLY_SUCCESS )
			return false;
		return true;
	}

	bool BYldSendMessageAndGetReply( CProtoBufMsgBase &msgOut, uint nTimeoutSec, IMsgNetPacket **ppNetPacket )
	{
		msgOut.ExpectingReply( GetJobID() );

		if ( !m_pGCClient->BSendMessage( msgOut ) )
			return false;

		SetJobTimeout( nTimeoutSec );
		return BYieldingWaitForMsg( ppNetPacket );
	}

	virtual uint32 CHeartbeatsBeforeTimeout() { return m_cHeartbeatsBeforeTimeout; }
	void SetJobTimeout( uint nTimeoutSec ) {  m_cHeartbeatsBeforeTimeout = 1 + ((nTimeoutSec * k_nMillion) / k_cMicroSecJobHeartbeat); }

private:
	virtual bool BYieldingRunJobFromMsg( IMsgNetPacket *pNetPacket )
	{
		// Protection against a NULL GCClient. Yields so the job is not deleted instantly
		if ( !m_pGCClient )
		{
			BYieldingWaitOneFrame();
			return false;
		}

		return BYieldingRunGCJob( pNetPacket );
	}

	virtual bool BYieldingRunJob( void *pvStartParam )
	{
		// Protection against a NULL GCClient. Yields so the job is not deleted instantly
		if ( !m_pGCClient )
		{
			BYieldingWaitOneFrame();
			return false;
		}

		return BYieldingRunGCJob();
	}

	uint32 m_cHeartbeatsBeforeTimeout;
};

} // namespace GCSDK

#endif // GCCLIENTJOB_H
