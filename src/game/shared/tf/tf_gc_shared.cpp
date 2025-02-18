//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_gc_shared.h"

//-----------------------------------------------------------------------------
void CReliableMessageQueue::Enqueue( IJobReliableMessage *pReliable )
{
	pReliable->OnAddedToQueue( this );

	if ( !m_pCurrentConfirmJob )
	{
		m_pCurrentConfirmJob = pReliable;
		pReliable->StartJobDelayed( nullptr );
	}
	else
	{
		// Queue, confirm jobs will kick next in queue as necessary
		m_queuePendingConfirmJobs.Insert( pReliable );
	}
}

//-----------------------------------------------------------------------------
int CReliableMessageQueue::NumPendingMessages() const
{
	Assert( !m_queuePendingConfirmJobs.Count() || m_pCurrentConfirmJob );
	return ( m_pCurrentConfirmJob ? 1 : 0 ) + m_queuePendingConfirmJobs.Count();
}

//-----------------------------------------------------------------------------
void CReliableMessageQueue::OnReliableMessageStalled( IJobReliableMessage *pReliable )
{
	if ( m_pCurrentConfirmJob != pReliable )
	{
		AssertMsg( m_pCurrentConfirmJob == pReliable, "OnReliableMessageStalled from non-current message, is bad" );
		return;
	}

	m_bCurrentMessageStalled = true;
}

//-----------------------------------------------------------------------------
void CReliableMessageQueue::OnReliableMessageComplete( IJobReliableMessage *pReliable )
{
	if ( m_pCurrentConfirmJob != pReliable )
	{
		AssertMsg( m_pCurrentConfirmJob == pReliable,
		           "OnReliableMessageComplete from a job that isn't head of queue, something very wrong" );
		return;
	}

	m_bCurrentMessageStalled = false;

	if ( m_queuePendingConfirmJobs.Count() )
	{
		// Kick off next job
		m_pCurrentConfirmJob = m_queuePendingConfirmJobs.RemoveAtHead();
		m_pCurrentConfirmJob->StartJob( nullptr );
	}
	else
	{
		m_pCurrentConfirmJob = nullptr;
	}
}

//-----------------------------------------------------------------------------
const IJobReliableMessage *CReliableMessageQueue::CurrentMessage() const
{
	return static_cast< const IJobReliableMessage *>( m_pCurrentConfirmJob );
}
