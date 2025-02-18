//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef _TF_GC_SHARED_H
#define _TF_GC_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/msgprotobuf.h"
#include "gcsdk/gcclientjob.h"
#include "gcsdk/job.h"
#include "tier1/utlqueue.h"
#include "gc_clientsystem.h"
#include "tf_gcmessages.pb.h"

using namespace GCSDK;

class IJobReliableMessage; // Defined below

#define MMLog(...) do { ::Msg( __VA_ARGS__ ); } while(false)

// Timeout upon successfully sending a reliable message before deciding we should try again.  This affects how quickly
// we can detect the Stalled state for a queue in the case of non-responses, as well.
#ifdef CLIENT_DLL
static const unsigned int k_ReliableMsg_unReplyTimeoutSecs  = 10;
#else
static const unsigned int k_ReliableMsg_unReplyTimeoutSecs  = 30;
#endif
// Wait at least this long between attempts, in the case of non-timeout failures, to avoid hammer-retrying
static const unsigned int k_ReliableMsg_unMinRetryDelaySecs = 5;

//-----------------------------------------------------------------------------
// ReliableMessageQueue - A queue of reliable messages
//-----------------------------------------------------------------------------
class CReliableMessageQueue
{
	friend class IJobReliableMessage;
public:
	// Check for pending messages
	int NumPendingMessages() const;

	// If processing of reliable messages is stalled due to GC communications issues, can occur when we still have a GC
	// session but the GC is in logon surge or otherwise not disposed to reply to us.
	bool BStalled() const { return m_bCurrentMessageStalled; }

	void Enqueue( IJobReliableMessage *pReliable );

	const IJobReliableMessage *CurrentMessage() const;

private:
	void OnReliableMessageComplete( IJobReliableMessage *pReliable );
	void OnReliableMessageStalled( IJobReliableMessage *pReliable );

	GCSDK::CGCClientJob *m_pCurrentConfirmJob = nullptr;
	CUtlQueue< GCSDK::CGCClientJob * > m_queuePendingConfirmJobs;
	bool m_bCurrentMessageStalled = false;
};

//-----------------------------------------------------------------------------
// ReliableMessage - A message/job class that retry until confirmed, and be sent
//                   In order with other such messages in the same queue
//-----------------------------------------------------------------------------

class IJobReliableMessage : public GCSDK::CGCClientJob
{
	friend class CReliableMessageQueue;
public:
	IJobReliableMessage( CGCClient *pGCClient )
		: CGCClientJob( pGCClient )
	{}

protected:
	void SetComplete() { Queue().OnReliableMessageComplete( this ); }
	void SetStalled() { Queue().OnReliableMessageStalled( this ); }

	CReliableMessageQueue &Queue() { return *m_pQueue; }

private:
	// Should only be called by the queue in question
	void OnAddedToQueue( CReliableMessageQueue *pQueue ) { Finalize(); Assert( !m_pQueue ); m_pQueue = pQueue; }

	// Called when we are added to a queue for the job class to finalize itself
	virtual void Finalize() = 0;

	CReliableMessageQueue *m_pQueue = nullptr;
};

template < typename RELIABLE_MSG_CLASS, typename MSG_TYPE, ETFGCMsg E_MSG_TYPE, typename REPLY_TYPE, ETFGCMsg E_REPLY_TYPE>
class CJobReliableMessageBase : public IJobReliableMessage
{
public:
	typedef CProtoBufMsg< MSG_TYPE >   Msg_t;
	typedef CProtoBufMsg< REPLY_TYPE > Reply_t;
	typedef MSG_TYPE                   MsgProto_t;
	typedef REPLY_TYPE                 ReplyProto_t;

	CJobReliableMessageBase()
		: IJobReliableMessage( GCClientSystem()->GetGCClient() )
		, m_msg( E_MSG_TYPE )
		, m_msgReply()
	{}

	Msg_t &Msg() { return m_msg; }

	virtual bool BYieldingRunJob( void *pvStartParam ) OVERRIDE
	{
		Assert( Queue().CurrentMessage() == this );
		bool bRet = BYieldingRunJobInternal();

		SetComplete();

		return bRet;
	}

	bool BYieldingRunJobInternal()
	{
		MMLog( "[ReliableMsg] %s started for %s\n", GetMsgName(), DebugString() );

		// Trigger OnPrepare
		ReliableMsg()->OnPrepare();

		for ( ;; )
		{
			BYieldingWaitOneFrame();

			// Create and load the message
			// continuously attempt to send the message to the GC

			double flTimeStart = Plat_FloatTime();
			if ( GCClientSystem()->BConnectedtoGC() )
			{
				BYldSendMessageAndGetReply_t result = BYldSendMessageAndGetReplyEx( m_msg,
				                                                                    k_ReliableMsg_unReplyTimeoutSecs,
				                                                                    &m_msgReply, E_REPLY_TYPE );

				switch ( result )
				{
					case BYLDREPLY_SUCCESS:
						MMLog( "[ReliableMsg] %s successfully sent for %s\n",
						       GetMsgName(), DebugString() );
						// Trigger OnReply
						ReliableMsg()->OnReply( m_msgReply );
						return true;
					case BYLDREPLY_SEND_FAILED:
						MMLog( "[ReliableMsg] %s send FAILED for %s -- retrying\n",
						       GetMsgName(), DebugString() );
						break;
					case BYLDREPLY_TIMEOUT:
						MMLog( "[ReliableMsg] %s send TIMEOUT for %s -- retrying\n",
						       GetMsgName(), DebugString() );
						break;
					case BYLDREPLY_MSG_TYPE_MISMATCH:
						MMLog( "[ReliableMsg] %s send TYPE MISMATCH for %s\n",
						       GetMsgName(), DebugString() );
						Assert( !"Mismatched response type in reliable message" );
						return true;
				}
			}
			else
			{
				MMLog( "[ReliableMsg] %s send STALLED for %s -- No GC session, retrying\n",
				       GetMsgName(), DebugString() );
			}

			// Looping to retry, inform queue we are stalled
			SetStalled();

			// If we are looping to retry, enforce minimum retry delay
			double flWait = (double)k_ReliableMsg_unMinRetryDelaySecs - Max( Plat_FloatTime() - flTimeStart, 0. );
			if ( flWait > 0. && flWait <= (double)k_ReliableMsg_unMinRetryDelaySecs )
				{ BYieldingWaitTime( (uint32_t)(flWait * k_nMillion) ); }
		}
	}

protected:
	// Overrides

	// Must be overridden by reliable message implementers. Debug string is e.g. "Match 12345, Lobby 4"
	void InitDebugString( CUtlString &debugStr ) {}
	const char *MsgName() { return "<unknown>"; }

	// Optionally overridden
	void OnReply( Reply_t &msgReply ) {}
	// Called before sending, after previous messages in queue have flushed
	void OnPrepare() {}

private:
	// Upcast ourselves to the message
	RELIABLE_MSG_CLASS *ReliableMsg() { return static_cast<RELIABLE_MSG_CLASS *>(this); }

	virtual void Finalize() OVERRIDE
	{
		ReliableMsg()->InitDebugString( m_strDebug );
		MMLog( "[ReliableMsg] %s queued for %s\n", GetMsgName(), DebugString() );
	}

	const char *DebugString() { return m_strDebug.Get(); }

	// Forward to override
	const char *GetMsgName() { return ReliableMsg()->MsgName(); }

	Msg_t   m_msg;
	Reply_t m_msgReply;
	CUtlString m_strDebug;

	void _static_asserts() {
		// Ensure we passed an override and provided provided these
		// (ifdef OSX because I don't have time to figure out what criteria the OS X toolchain has to not blow this)
#if __cplusplus >= 201103L && !defined ( OSX )
		static_assert( std::is_base_of< decltype( *this ), RELIABLE_MSG_CLASS >::value,
		               "RELIABLE_MSG_CLASS Must be an override of this base" );
		static_assert( !std::is_same< decltype( &(decltype( *this )::InitDebugString) ),
		               decltype( &RELIABLE_MSG_CLASS::InitDebugString ) >::value && \
		               !std::is_same< decltype( &(decltype( *this )::MsgName) ),
		               decltype( &RELIABLE_MSG_CLASS::MsgName ) >::value,
		               "RELIABLE_MSG_CLASS class must override DebugString and MsgName" );
#endif // __cplusplus >= 201103L && !defined ( OSX )
	}
};

#endif // _TF_GC_SHARED_H
