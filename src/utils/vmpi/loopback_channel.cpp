//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "loopback_channel.h"
#include "utllinkedlist.h"
#include "iphelpers.h"


// -------------------------------------------------------------------------------- //
// CLoopbackChannel.
// -------------------------------------------------------------------------------- //

typedef struct
{
	int				m_Len;
	unsigned char	m_Data[1];
} LoopbackMsg_t;


class CLoopbackChannel : public IChannel
{
public:

	virtual				~CLoopbackChannel()
	{
		FOR_EACH_LL( m_Messages, i )
		{
			free( m_Messages[i] );
		}
		m_Messages.Purge();
	}

	virtual void		Release()
	{
		delete this;
	}

	virtual bool	Send( const void *pData, int len )
	{
		const void *pChunks[1] = { pData };
		int chunkLengths[1] = { len };
		return SendChunks( pChunks, chunkLengths, 1 );
	}
	
	virtual bool	SendChunks( void const * const *pChunks, const int *pChunkLengths, int nChunks )
	{
		CChunkWalker walker( pChunks, pChunkLengths, nChunks );

		LoopbackMsg_t *pMsg = (LoopbackMsg_t*)malloc( sizeof( LoopbackMsg_t ) - 1 + walker.GetTotalLength() );
		walker.CopyTo( pMsg->m_Data, walker.GetTotalLength() );
		pMsg->m_Len = walker.GetTotalLength();
		m_Messages.AddToTail( pMsg );
		return true;
	}
	
	virtual bool	Recv( CUtlVector<unsigned char> &data, double flTimeout )
	{
		int iNext = m_Messages.Head();
		if ( iNext == m_Messages.InvalidIndex() )
		{
			return false;
		}
		else
		{
			LoopbackMsg_t *pMsg = m_Messages[iNext];
			
			data.CopyArray( pMsg->m_Data, pMsg->m_Len );
			
			free( pMsg );
			m_Messages.Remove( iNext );

			return true;
		}
	}
	
	virtual bool	IsConnected()
	{
		return true;
	}

	virtual void	GetDisconnectReason( CUtlVector<char> &reason )
	{
	}

private:
	CUtlLinkedList<LoopbackMsg_t*,int>	m_Messages;	// FIFO for messages we've sent.
};


IChannel* CreateLoopbackChannel()
{
	return new CLoopbackChannel;
}

