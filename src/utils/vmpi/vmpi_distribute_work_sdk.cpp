//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "vmpi.h"
#include "vmpi_distribute_work.h"
#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "utlvector.h"
#include "utllinkedlist.h"
#include "vmpi_dispatch.h"
#include "pacifier.h"
#include "vstdlib/random.h"
#include "mathlib/mathlib.h"
#include "threadhelpers.h"
#include "threads.h"
#include "tier1/strtools.h"
#include "tier1/utlmap.h"
#include "tier1/smartptr.h"
#include "tier0/icommandline.h"
#include "cmdlib.h"
#include "vmpi_distribute_tracker.h"
#include "vmpi_distribute_work_internal.h"



#define DW_SUBPACKETID_SHUFFLE				(VMPI_DISTRIBUTE_WORK_EXTRA_SUBPACKET_BASE+0)
#define DW_SUBPACKETID_REQUEST_SHUFFLE		(VMPI_DISTRIBUTE_WORK_EXTRA_SUBPACKET_BASE+1)
#define DW_SUBPACKETID_WUS_COMPLETED_LIST	(VMPI_DISTRIBUTE_WORK_EXTRA_SUBPACKET_BASE+2)



// This is a pretty simple iterator. Basically, it holds a matrix of numbers.
// Each row is assigned to a worker, and the worker just walks through his row.
//
// When a worker reaches the end of his row, it gets a little trickier.
// They'll start doing their neighbor's row
// starting at the back and continue on. At about this time, the master should reshuffle the
// remaining work units to evenly distribute them amongst the workers.
class CWorkUnitWalker
{
public:
	CWorkUnitWalker()
	{
		m_nWorkUnits = 0;
	}
	
	// This is all that's needed for it to start assigning work units.
	void Init( WUIndexType matrixWidth, WUIndexType matrixHeight, WUIndexType nWorkUnits )
	{
		m_nWorkUnits = nWorkUnits;
		m_MatrixWidth = matrixWidth;
		m_MatrixHeight = matrixHeight;
		Assert( m_MatrixWidth * m_MatrixHeight >= nWorkUnits );
		
		m_WorkerInfos.RemoveAll();
		m_WorkerInfos.EnsureCount( m_MatrixHeight );
		for ( int i=0; i < m_MatrixHeight; i++ )
		{
			m_WorkerInfos[i].m_iStartWorkUnit = matrixWidth * i;
			m_WorkerInfos[i].m_iWorkUnitOffset = 0;
		}
	}

	// This is the main function of the shuffler
	bool GetNextWorkUnit( int iWorker, WUIndexType *pWUIndex, bool *bWorkerFinishedHisColumn )
	{
		if ( iWorker < 0 || iWorker >= m_WorkerInfos.Count() )
		{
			Assert( false );
			return false;
		}

		// If this worker has walked through all the work units, then he's done.
		CWorkerInfo *pWorker = &m_WorkerInfos[iWorker];
		if ( pWorker->m_iWorkUnitOffset >= m_nWorkUnits )
			return false;
		
		// If we've gone past the end of our work unit list, then we start at the BACK of the other rows of work units
		// in the hopes that we won't collide with the guy working there. We also should tell the master to reshuffle.
		WUIndexType iWorkUnitOffset = pWorker->m_iWorkUnitOffset;
		if ( iWorkUnitOffset >= m_MatrixWidth )
		{
			WUIndexType xOffset = iWorkUnitOffset % m_MatrixWidth;
			WUIndexType yOffset = iWorkUnitOffset / m_MatrixWidth;
			xOffset = m_MatrixWidth - xOffset - 1;
			iWorkUnitOffset = yOffset * m_MatrixWidth + xOffset;
			*bWorkerFinishedHisColumn = true;
		}
		else
		{
			*bWorkerFinishedHisColumn = false;
		}

		*pWUIndex = (pWorker->m_iStartWorkUnit + iWorkUnitOffset) % m_nWorkUnits;
		++pWorker->m_iWorkUnitOffset;
		return true;
	}


private:
	class CWorkerInfo
	{
	public:
		WUIndexType m_iStartWorkUnit;
		WUIndexType m_iWorkUnitOffset;	// Which work unit in my list of work units am I working on?
	};
	
	WUIndexType m_nWorkUnits;
	WUIndexType m_MatrixWidth;
	WUIndexType m_MatrixHeight;
	CUtlVector<CWorkerInfo> m_WorkerInfos;
};


class IShuffleRequester
{
public:
	virtual void RequestShuffle() = 0;
};


// This is updated every time the master decides to reshuffle.
// In-between shuffles, you can call NoteWorkUnitCompleted when a work unit is completed
// and it'll avoid returning that work unit from GetNextWorkUnit again, but it WON'T 
class CShuffledWorkUnitWalker
{
public:
	void Init( WUIndexType nWorkUnits, IShuffleRequester *pRequester )
	{
		m_iLastShuffleRequest = 0;
		m_iCurShuffle = 1;
		m_flLastShuffleTime = Plat_FloatTime();
		m_pShuffleRequester = pRequester;
		
		int nBytes = PAD_NUMBER( nWorkUnits, 8 ) / 8;
		m_CompletedWUBits.SetSize( nBytes );
		m_LocalCompletedWUBits.SetSize( nBytes );
		for ( WUIndexType i=0; i < m_CompletedWUBits.Count(); i++ )
			m_LocalCompletedWUBits[i] = m_CompletedWUBits[i] = 0;		
	
		// Setup our list of work units remaining.
		for ( WUIndexType iWU=0; iWU < nWorkUnits; iWU++ )
		{
			// Note: we're making an assumption here that if we add entries to a CUtlLinkedList in ascending order, their indices
			// will be ascending 1-by-1 as well. If that assumption breaks, we can create an extra array here to map WU indices to the linked list indices.
			WUIndexType index = m_WorkUnitsRemaining.AddToTail( iWU );
			if ( index != iWU )
			{
				Error( "CShuffledWorkUnitWalker: assumption on CUtlLinkedList indexing failed.\n" );
			}
		}
	}
	
	void Shuffle( int nWorkers )
	{
		if ( nWorkers == 0 )
			return;

		++m_iCurShuffle;
		m_flLastShuffleTime = Plat_FloatTime();
		
		CVMPICriticalSectionLock csLock( &m_CS );
		csLock.Lock();
		
		m_WorkUnitsMap.RemoveAll();
		m_WorkUnitsMap.EnsureCount( m_WorkUnitsRemaining.Count() );
		
		// Here's the shuffle. The CWorkUnitWalker is going to walk each worker through its own group from 0-W,
		// and our job is to interleave it so when worker 0 goes [0,1,2] and worker 1 goes [100,101,102], they're actually 
		// doing [0,N,2N] and [1,N+1,2N+1] where N=# of workers.
		
		// The grid is RxW long, and R*W is >= nWorkUnits.
		// R = # units per worker = width of the matrix
		// W = # workers          = height of the matrix
		WUIndexType matrixHeight = nWorkers;
		WUIndexType matrixWidth = m_WorkUnitsRemaining.Count() / matrixHeight;
		if ( (m_WorkUnitsRemaining.Count() % matrixHeight) != 0 )
			++matrixWidth;

		Assert( matrixWidth * matrixHeight >= m_WorkUnitsRemaining.Count() );
		
		WUIndexType iWorkUnit = 0;
		FOR_EACH_LL( m_WorkUnitsRemaining, i )
		{
			WUIndexType xCoord = iWorkUnit / matrixHeight;
			WUIndexType yCoord = iWorkUnit % matrixHeight;
			Assert( xCoord < matrixWidth );
			Assert( yCoord < matrixHeight );
			
			m_WorkUnitsMap[yCoord*matrixWidth+xCoord] = m_WorkUnitsRemaining[i];
			++iWorkUnit;
		}

		m_Walker.Init( matrixWidth, matrixHeight, m_WorkUnitsRemaining.Count() );
	}	
	
	// Threadsafe.
	bool Thread_IsWorkUnitCompleted( WUIndexType iWU )
	{
		CVMPICriticalSectionLock csLock( &m_CS );
		csLock.Lock();
		
		byte val = m_CompletedWUBits[iWU >> 3] & (1 << (iWU & 7));
		return (val != 0);
	}
	
	WUIndexType Thread_NumWorkUnitsRemaining()
	{
		CVMPICriticalSectionLock csLock( &m_CS );
		csLock.Lock();
		
		return m_WorkUnitsRemaining.Count();
	}
	
	bool Thread_GetNextWorkUnit( int iWorker, WUIndexType *pWUIndex )
	{
		CVMPICriticalSectionLock csLock( &m_CS );
		csLock.Lock();
		
		while ( 1 )
		{
			WUIndexType iUnmappedWorkUnit;
			bool bWorkerFinishedHisColumn;
			if ( !m_Walker.GetNextWorkUnit( iWorker, &iUnmappedWorkUnit, &bWorkerFinishedHisColumn ) )
				return false;
				
			// If we've done all the work units assigned to us in the last shuffle, then request a reshuffle.
			if ( bWorkerFinishedHisColumn )
				HandleWorkerFinishedColumn();
			
			// Check the pending list.
			*pWUIndex = m_WorkUnitsMap[iUnmappedWorkUnit];
			byte bIsCompleted = m_CompletedWUBits[*pWUIndex >> 3] & (1 << (*pWUIndex & 7));
			byte bIsCompletedLocally = m_LocalCompletedWUBits[*pWUIndex >> 3] & (1 << (*pWUIndex & 7));
			if ( !bIsCompleted && !bIsCompletedLocally )
				return true;
		}			
	}
	
	void HandleWorkerFinishedColumn()
	{
		if ( m_iLastShuffleRequest != m_iCurShuffle )
		{
			double flCurTime = Plat_FloatTime();
			if ( flCurTime - m_flLastShuffleTime > 2.0f )
			{
				m_pShuffleRequester->RequestShuffle();
				m_iLastShuffleRequest = m_iCurShuffle;
			}
		}
	}
	
	void Thread_NoteWorkUnitCompleted( WUIndexType iWU )
	{
		CVMPICriticalSectionLock csLock( &m_CS );
		csLock.Lock();

		byte val = m_CompletedWUBits[iWU >> 3] & (1 << (iWU & 7));
		if ( val == 0 )
		{
			m_WorkUnitsRemaining.Remove( iWU );
			m_CompletedWUBits[iWU >> 3] |= (1 << (iWU & 7));
		}
	}
	
	void Thread_NoteLocalWorkUnitCompleted( WUIndexType iWU )
	{
		CVMPICriticalSectionLock csLock( &m_CS );
		csLock.Lock();
		m_LocalCompletedWUBits[iWU >> 3] |= (1 << (iWU & 7));
	}
	
	CRC32_t GetShuffleCRC()
	{
#ifdef _DEBUG
		static bool bCalcShuffleCRC = true;
#else
		static bool bCalcShuffleCRC = VMPI_IsParamUsed( mpi_CalcShuffleCRC );
#endif
		if ( bCalcShuffleCRC )
		{
			CVMPICriticalSectionLock csLock( &m_CS );
			csLock.Lock();

			CRC32_t ret;
			CRC32_Init( &ret );
			
			FOR_EACH_LL( m_WorkUnitsRemaining, i )
			{
				WUIndexType iWorkUnit = m_WorkUnitsRemaining[i];
				CRC32_ProcessBuffer( &ret, &iWorkUnit, sizeof( iWorkUnit ) );
			}
			
			for ( int i=0; i < m_WorkUnitsMap.Count(); i++ )
			{
				WUIndexType iWorkUnit = m_WorkUnitsMap[i];
				CRC32_ProcessBuffer( &ret, &iWorkUnit, sizeof( iWorkUnit ) );
			}
			
			CRC32_Final( &ret );
			return ret;
		}
		else
		{
			return false;
		}
	}

private:
	// These are PENDING WU completions until we call Shuffle() again, at which point we actually reorder the list
	// based on the completed WUs.
	CUtlVector<byte> m_CompletedWUBits;					// Bit vector of completed WUs.
	CUtlLinkedList<WUIndexType, WUIndexType> m_WorkUnitsRemaining;
	CUtlVector<WUIndexType> m_WorkUnitsMap;				// Maps the 0-N indices in the CWorkUnitWalker to the list of remaining work units.

	// Helps us avoid some duplicates that happen during shuffling if we've completed some WUs and sent them
	// to the master, but the master hasn't included them in the DW_SUBPACKETID_WUS_COMPLETED_LIST yet.
	CUtlVector<byte> m_LocalCompletedWUBits;					// Bit vector of completed WUs.

	// Used to control how frequently we request a reshuffle.
	unsigned int m_iCurShuffle;
	unsigned int m_iLastShuffleRequest;	// The index of the shuffle we last requested a reshuffle on (don't request a reshuffle on the same one).
	double m_flLastShuffleTime;
	IShuffleRequester *m_pShuffleRequester;
	
	CWorkUnitWalker m_Walker;
	CVMPICriticalSection m_CS;
};



class CDistributor_SDKMaster : public IWorkUnitDistributorMaster, public IShuffleRequester
{
public:
	virtual void Release()
	{
		delete this;
	}

	static void Master_WorkerThread_Static( int iThread, void *pUserData )
	{
		((CDistributor_SDKMaster*)pUserData)->Master_WorkerThread( iThread );
	}
	
	void Master_WorkerThread( int iThread )
	{
		while ( m_WorkUnitWalker.Thread_NumWorkUnitsRemaining() > 0 && !g_bVMPIEarlyExit )
		{
			WUIndexType iWU;
			if ( !m_WorkUnitWalker.Thread_GetNextWorkUnit( 0, &iWU ) )
			{
				// Wait until there are some WUs to do.
				VMPI_Sleep( 10 );
				continue;
			}
			
			// Do this work unit.
			m_WorkUnitWalker.Thread_NoteLocalWorkUnitCompleted( iWU );	// We do this before it's completed because otherwise if a Shuffle() occurs,
																		// the other thread might happen to pickup this work unit and we don't want that.
			m_pInfo->m_WorkerInfo.m_pProcessFn( iThread, iWU, NULL );
			NotifyLocalMasterCompletedWorkUnit( iWU );
		}
	}

	virtual void DistributeWork_Master( CDSInfo *pInfo )
	{
		m_pInfo = pInfo;
		m_bForceShuffle = false;
		m_bShuffleRequested = false;
		m_flLastShuffleRequestServiceTime = Plat_FloatTime();
		
		// Spawn idle-priority worker threads right here.
		m_bUsingMasterLocalThreads = (pInfo->m_WorkerInfo.m_pProcessFn != 0);
		if ( VMPI_IsParamUsed( mpi_NoMasterWorkerThreads ) )
		{
			Msg( "%s found. No worker threads will be created.\n", VMPI_GetParamString( mpi_NoMasterWorkerThreads ) );
			m_bUsingMasterLocalThreads = false;
		}
		m_WorkUnitWalker.Init( pInfo->m_nWorkUnits, this );
		Shuffle();

		if ( m_bUsingMasterLocalThreads )
			RunThreads_Start( Master_WorkerThread_Static, this, k_eRunThreadsPriority_Idle );

		uint64 lastShuffleTime = Plat_MSTime();
		while ( m_WorkUnitWalker.Thread_NumWorkUnitsRemaining() > 0 )
		{
			VMPI_DispatchNextMessage( 200 );
			CheckLocalMasterCompletedWorkUnits();
			
			VMPITracker_HandleDebugKeypresses();
			if ( g_pDistributeWorkCallbacks && g_pDistributeWorkCallbacks->Update() )
				break;

			// Reshuffle the work units optimally every certain interval.
			if ( m_bForceShuffle || CheckShuffleRequest() )
			{
				Shuffle();				
				lastShuffleTime = Plat_MSTime();
				m_bForceShuffle = false;
			}
		}
		
		RunThreads_End();
	}

	virtual void RequestShuffle()
	{
		m_bShuffleRequested = true;
	}
	
	bool CheckShuffleRequest()
	{
		if ( m_bShuffleRequested )
		{
			double flCurTime = Plat_FloatTime();
			if ( flCurTime - m_flLastShuffleRequestServiceTime > 2.0f ) // Only handle shuffle requests every so often.
			{
				m_flLastShuffleRequestServiceTime = flCurTime;
				m_bShuffleRequested = false;
				return true;
			}
		}
		
		return false;		
	}
	
	void Shuffle()
	{
		// Build a list of who's working.
		CUtlVector<unsigned short> whosWorking;
		if ( m_bUsingMasterLocalThreads )
		{
			whosWorking.AddToTail( VMPI_MASTER_ID );
			Assert( VMPI_MASTER_ID == 0 );
		}
		
		{
			CWorkersReady *pWorkersReady = m_WorkersReadyCS.Lock();
			for ( int i=0; i < pWorkersReady->m_WorkersReady.Count(); i++ )
			{
				int iWorker = pWorkersReady->m_WorkersReady[i];
				if ( VMPI_IsProcConnected( iWorker ) )
					whosWorking.AddToTail( iWorker );
			}
			m_WorkersReadyCS.Unlock();
		}

		// Before sending the shuffle command, tell any of these active workers about the pending WUs completed.
		CWUsCompleted *pWUsCompleted = m_WUsCompletedCS.Lock();
			
			m_WUSCompletedMessageBuffer.setLen( 0 );
			if ( BuildWUsCompletedMessage( pWUsCompleted->m_Pending, m_WUSCompletedMessageBuffer ) > 0 )
			{
				for ( int i=m_bUsingMasterLocalThreads; i < whosWorking.Count(); i++ )
				{
					VMPI_SendData( m_WUSCompletedMessageBuffer.data, m_WUSCompletedMessageBuffer.getLen(), whosWorking[i] );
				}
			}
			pWUsCompleted->m_Completed.AddMultipleToTail( pWUsCompleted->m_Pending.Count(), pWUsCompleted->m_Pending.Base() ); // Add the pending ones to the full list now.
			pWUsCompleted->m_Pending.RemoveAll();
		
		m_WUsCompletedCS.Unlock();
		
		// Shuffle ourselves.
		m_WorkUnitWalker.Shuffle( whosWorking.Count() );

		// Send the shuffle command to the workers.
		MessageBuffer mb;
		PrepareDistributeWorkHeader( &mb, DW_SUBPACKETID_SHUFFLE );

		unsigned short nWorkers = whosWorking.Count();
		mb.write( &nWorkers, sizeof( nWorkers ) );

		CRC32_t shuffleCRC = m_WorkUnitWalker.GetShuffleCRC();
		mb.write( &shuffleCRC, sizeof( shuffleCRC ) );

		// Now for each worker, assign him an index in the shuffle and send the shuffle command.		
		int workerIDPos = mb.getLen();
		unsigned short id = 0;
		mb.write( &id, sizeof( id ) );
		for ( int i=m_bUsingMasterLocalThreads; i < whosWorking.Count(); i++ )
		{
			id = (unsigned short)i;
			mb.update( workerIDPos, &id, sizeof( id ) );
			VMPI_SendData( mb.data, mb.getLen(), whosWorking[i] );
		}
	}

	int BuildWUsCompletedMessage( CUtlVector<WUIndexType> &wusCompleted, MessageBuffer &mb )
	{
		PrepareDistributeWorkHeader( &mb, DW_SUBPACKETID_WUS_COMPLETED_LIST );
		m_pInfo->WriteWUIndex( wusCompleted.Count(), &mb );
		for ( int i=0; i < wusCompleted.Count(); i++ )
		{
			m_pInfo->WriteWUIndex( wusCompleted[i], &mb );
		}
		return wusCompleted.Count();
	}

	virtual void OnWorkerReady( int iSource )
	{
		CWorkersReady *pWorkersReady = m_WorkersReadyCS.Lock();
		if ( pWorkersReady->m_WorkersReady.Find( iSource ) == -1 )
		{
			pWorkersReady->m_WorkersReady.AddToTail( iSource );
			
			// Get this guy up to speed on which WUs are done.
			{
				CWUsCompleted *pWUsCompleted = m_WUsCompletedCS.Lock();
				m_WUSCompletedMessageBuffer.setLen( 0 );
				BuildWUsCompletedMessage( pWUsCompleted->m_Completed, m_WUSCompletedMessageBuffer );
				m_WUsCompletedCS.Unlock();
			}
			
			VMPI_SendData( m_WUSCompletedMessageBuffer.data, m_WUSCompletedMessageBuffer.getLen(), iSource );
			m_bForceShuffle = true;
		}
		m_WorkersReadyCS.Unlock();
	}

	virtual bool HandleWorkUnitResults( WUIndexType iWorkUnit )
	{
		return Thread_HandleWorkUnitResults( iWorkUnit );
	}
	
	bool Thread_HandleWorkUnitResults( WUIndexType iWorkUnit )
	{
		if ( m_WorkUnitWalker.Thread_IsWorkUnitCompleted( iWorkUnit ) )
		{
			return false;
		}
		else
		{
			m_WorkUnitWalker.Thread_NoteWorkUnitCompleted( iWorkUnit );
			
			// We need the lock on here because our own worker threads can call into here.
			CWUsCompleted *pWUsCompleted = m_WUsCompletedCS.Lock();
			pWUsCompleted->m_Pending.AddToTail( iWorkUnit );
			m_WUsCompletedCS.Unlock();
			return true;
		}
	}

	virtual bool HandlePacket( MessageBuffer *pBuf, int iSource, bool bIgnoreContents )
	{
		if ( pBuf->data[1] == DW_SUBPACKETID_REQUEST_SHUFFLE )
		{
			if ( bIgnoreContents )
				return true;
		
			m_bShuffleRequested = true;
		}
		return false;
	}

	virtual void DisconnectHandler( int workerID )
	{
		CWorkersReady *pWorkersReady = m_WorkersReadyCS.Lock();
		
		if ( pWorkersReady->m_WorkersReady.Find( workerID ) != -1 )
			m_bForceShuffle = true;
			
		m_WorkersReadyCS.Unlock();
	}

public:
	CDSInfo *m_pInfo;
	
	class CWorkersReady
	{
	public:
		CUtlVector<int> m_WorkersReady; // The list of workers who have said they're ready to participate.
	};
	CCriticalSectionData<CWorkersReady> m_WorkersReadyCS;

	class CWUsCompleted
	{
	public:
		CUtlVector<WUIndexType> m_Completed;					// WUs completed that we have sent to workers.
		CUtlVector<WUIndexType> m_Pending;			// WUs completed that we haven't sent to workers.
	};
	CCriticalSectionData<CWUsCompleted> m_WUsCompletedCS;
	MessageBuffer						m_WUSCompletedMessageBuffer;	// Used to send lists of completed WUs.
	int m_bUsingMasterLocalThreads;
	
	bool m_bForceShuffle;
	bool m_bShuffleRequested;
	double m_flLastShuffleRequestServiceTime;
	
	CShuffledWorkUnitWalker m_WorkUnitWalker;
};


class CDistributor_SDKWorker : public IWorkUnitDistributorWorker, public IShuffleRequester
{
public:
	virtual void Init( CDSInfo *pInfo )
	{
		m_iMyWorkUnitWalkerID = -1;
		m_pInfo = pInfo;
		m_WorkUnitWalker.Init( pInfo->m_nWorkUnits, this );
	}
	
	virtual void Release()
	{
		delete this;
	}
	
	virtual bool GetNextWorkUnit( WUIndexType *pWUIndex )
	{
		// If we don't have an ID yet, we haven't received a Shuffle() command, so we're waiting for that before working.
		// TODO: we could do some random WUs here while we're waiting, although that could suck if the WUs take forever to do
		//       and they're duplicates.
		if ( m_iMyWorkUnitWalkerID == -1 )
			return false;
			
		// Look in our current shuffled list of work units for the next one.
		return m_WorkUnitWalker.Thread_GetNextWorkUnit( m_iMyWorkUnitWalkerID, pWUIndex );
	}
	
	virtual void NoteLocalWorkUnitCompleted( WUIndexType iWU )
	{
		m_WorkUnitWalker.Thread_NoteLocalWorkUnitCompleted( iWU );
	}	

	virtual bool HandlePacket( MessageBuffer *pBuf, int iSource, bool bIgnoreContents )
	{
		// If it's a SHUFFLE message, then shuffle..
		if ( pBuf->data[1] == DW_SUBPACKETID_SHUFFLE )
		{
			if ( bIgnoreContents )
				return true;
			
			unsigned short nWorkers, myID;
			CRC32_t shuffleCRC;
			pBuf->read( &nWorkers, sizeof( nWorkers ) );
			pBuf->read( &shuffleCRC, sizeof( shuffleCRC ) );
			pBuf->read( &myID, sizeof( myID ) );
			m_iMyWorkUnitWalkerID = myID;
		
			m_WorkUnitWalker.Shuffle( nWorkers );
			if ( m_WorkUnitWalker.GetShuffleCRC() != shuffleCRC )
			{
				static int nWarnings = 1;
				if ( ++nWarnings <= 2 )
					Warning( "\nShuffle CRC mismatch\n" );
			}
			return true;
		}
		else if ( pBuf->data[1] == DW_SUBPACKETID_WUS_COMPLETED_LIST )
		{
			if ( bIgnoreContents )
				return true;
			
			WUIndexType nCompleted;
 			m_pInfo->ReadWUIndex( &nCompleted, pBuf );
			for ( WUIndexType i=0; i < nCompleted; i++ )
			{
				WUIndexType iWU;
				m_pInfo->ReadWUIndex( &iWU, pBuf );
				m_WorkUnitWalker.Thread_NoteWorkUnitCompleted( iWU );
			}
			
			return true;
		}
		
		return false;
	}

	virtual void RequestShuffle()
	{
		// Ok.. request a reshuffle.
		MessageBuffer mb;
		PrepareDistributeWorkHeader( &mb, DW_SUBPACKETID_REQUEST_SHUFFLE );
		VMPI_SendData( mb.data, mb.getLen(), VMPI_MASTER_ID );
	}
	
private:
	CDSInfo *m_pInfo;
	CShuffledWorkUnitWalker m_WorkUnitWalker;
	int m_iMyWorkUnitWalkerID;
};



IWorkUnitDistributorMaster* CreateWUDistributor_SDKMaster()
{
	return new CDistributor_SDKMaster;
}

IWorkUnitDistributorWorker* CreateWUDistributor_SDKWorker()
{
	return new CDistributor_SDKWorker;
}

