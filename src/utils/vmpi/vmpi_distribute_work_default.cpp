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


#define DW_SUBPACKETID_WU_ASSIGNMENT	(VMPI_DISTRIBUTE_WORK_EXTRA_SUBPACKET_BASE+0)



static int s_numWusToDeal = -1;

void VMPI_SetWorkUnitsPartitionSize( int numWusToDeal )
{
	s_numWusToDeal = numWusToDeal;
}


class CWorkUnitInfo
{
public:
	WUIndexType m_iWorkUnit;
};


class CWULookupInfo
{
public:
	CWULookupInfo() : m_iWUInfo( -1 ), m_iPartition( -222222 ), m_iPartitionListIndex( -1 ) {}

public:
	int m_iWUInfo;				// Index into m_WUInfo.
	int m_iPartition;			// Which partition it's in.
	int m_iPartitionListIndex;	// Index into its partition's m_WUs.
};


class CPartitionInfo
{
public:
	typedef CUtlLinkedList< WUIndexType, int > PartitionWUs;

public:
	int m_iPartition;	// Index into m_Partitions.
	int m_iWorker; // Who owns this partition?
	PartitionWUs m_WUs;	// Which WUs are in this partition?
};


// Work units tracker to track consecutive finished blocks
class CWorkUnitsTracker
{
public:
	CWorkUnitsTracker() {}

public:
	// Initializes the unit tracker to receive numUnits in future
	void PrepareForWorkUnits( uint64 numUnits );
	// Signals that a work unit has been finished
	// returns a zero-based index of the next pending work unit
	//			up to which the task list has been processed fully now
	//			because the received work unit filled the gap or was the next pending work unit.
	// returns 0 to indicate that this work unit is a "faster processed future work unit".
	uint64 WorkUnitFinished( uint64 iWorkUnit );

public:
	enum WUInfo { kNone, kTrigger, kDone };
	CVisibleWindowVector< uint8 > m_arrInfo;
};

void CWorkUnitsTracker::PrepareForWorkUnits( uint64 numUnits )
{
	m_arrInfo.Reset( numUnits + 1 );

	if ( numUnits )
	{
		m_arrInfo.ExpandWindow( 2ull, kNone );
		m_arrInfo.Get( 0ull ) = kTrigger;
	}
}

uint64 CWorkUnitsTracker::WorkUnitFinished( uint64 iWorkUnit )
{
	uint64 uiResult = uint64( 0 );

	if ( iWorkUnit >= m_arrInfo.FirstPossibleIndex() && iWorkUnit < m_arrInfo.PastPossibleIndex() )
	{
		// Need to access the element
		m_arrInfo.ExpandWindow( iWorkUnit + 1, kNone );

		// Set it done
		uint8 &rchThere = m_arrInfo.Get( iWorkUnit ), chThere = rchThere;
		rchThere = kDone;

		// Should we trigger?
		if ( kTrigger == chThere )
		{
			// Go along all "done" work units and trigger the last found one
			while ( ( ( ++ iWorkUnit ) < m_arrInfo.PastVisibleIndex() ) &&
				( kDone == m_arrInfo.Get( iWorkUnit ) ) )
				continue;

			m_arrInfo.Get( iWorkUnit ) = kTrigger;
			m_arrInfo.ShrinkWindow( iWorkUnit - 1 );
			uiResult = iWorkUnit;
		}
		else if( iWorkUnit == m_arrInfo.FirstPossibleIndex() )
		{
			// Go along all "done" work units and shrink including the last found one
			while ( ( ( ++ iWorkUnit ) < m_arrInfo.PastVisibleIndex() ) &&
				( kDone == m_arrInfo.Get( iWorkUnit ) ) )
				continue;

			m_arrInfo.ShrinkWindow( iWorkUnit - 1 );
		}
	}

	return uiResult;
}

CWorkUnitsTracker g_MasterWorkUnitsTracker;



static bool CompareSoonestWorkUnitSets( CPartitionInfo::PartitionWUs * const &x, CPartitionInfo::PartitionWUs * const &y )
{
	// Compare by fourth/second/first job in the partitions
	WUIndexType missing = ~WUIndexType(0);
	WUIndexType jobsX[4] = { missing, missing, missing, missing };
	WUIndexType jobsY[4] = { missing, missing, missing, missing };
	int counter = 0;

	counter = 0;
	FOR_EACH_LL( (*x), i )
	{
		jobsX[ counter ++ ] = (*x)[i];
		if ( counter >= 4 )
			break;
	}

	counter = 0;
	FOR_EACH_LL( (*y), i )
	{
		jobsY[ counter ++ ] = (*y)[i];
		if ( counter >= 4 )
			break;
	}

	// Compare
	if ( jobsX[3] != jobsY[3] )
		return ( jobsX[3] < jobsY[3] );

	if ( jobsX[1] != jobsY[1] )
		return ( jobsX[1] < jobsY[1] );

	return jobsX[0] < jobsY[0];
}



class CDistributor_DefaultMaster : public IWorkUnitDistributorMaster
{
public:
	virtual void Release()
	{
		delete this;
	}
	
	virtual void DistributeWork_Master( CDSInfo *pInfo )
	{
		m_pInfo = pInfo;
		g_MasterWorkUnitsTracker.PrepareForWorkUnits( m_pInfo->m_nWorkUnits );
		
		m_WULookup.Reset( pInfo->m_nWorkUnits );
		while ( m_WULookup.FirstPossibleIndex() < m_WULookup.PastPossibleIndex() )
		{
			VMPI_DispatchNextMessage( 200 );

			VMPITracker_HandleDebugKeypresses();
		
			if ( g_pDistributeWorkCallbacks && g_pDistributeWorkCallbacks->Update() )
				break;
		}
	}

	virtual void OnWorkerReady( int iSource )
	{
		AssignWUsToWorker( iSource );
	}

	virtual bool HandleWorkUnitResults( WUIndexType iWorkUnit )
	{
		CWULookupInfo *pLookup = NULL;
		if ( iWorkUnit >= m_WULookup.FirstPossibleIndex() && iWorkUnit < m_WULookup.PastVisibleIndex() )
			pLookup = &m_WULookup.Get( iWorkUnit );

		if ( !pLookup || pLookup->m_iWUInfo == -1 )
			return false;
		
		// Mark this WU finished and remove it from the list of pending WUs.
		m_WUInfo.Remove( pLookup->m_iWUInfo );
		pLookup->m_iWUInfo = -1;	


		// Get rid of the WU from its partition.
		int iPartition = pLookup->m_iPartition;
		CPartitionInfo *pPartition = m_Partitions[iPartition];
		pPartition->m_WUs.Remove( pLookup->m_iPartitionListIndex );

		// Shrink the window of the lookup work units
		if ( iWorkUnit == m_WULookup.FirstPossibleIndex() )
		{
			WUIndexType kwu = iWorkUnit;
			for ( WUIndexType kwuEnd = m_WULookup.PastVisibleIndex(); kwu < kwuEnd; ++ kwu )
			{
				if ( -1 != m_WULookup.Get( kwu ).m_iWUInfo && kwu > iWorkUnit )
					break;
			}
			m_WULookup.ShrinkWindow( kwu - 1 );
		}

		// Give the worker some new work if need be.
		if ( pPartition->m_WUs.Count() == 0 )
		{
			int iPartitionWorker = pPartition->m_iWorker;
			delete pPartition;
			m_Partitions.Remove( iPartition );
	
			// If there are any more WUs remaining, give the worker from this partition some more of them.
			if ( m_WULookup.FirstPossibleIndex() < m_WULookup.PastPossibleIndex() )
			{
				AssignWUsToWorker( iPartitionWorker );
			}
		}

		uint64 iDoneWorkUnits = g_MasterWorkUnitsTracker.WorkUnitFinished( iWorkUnit );
		if ( iDoneWorkUnits && g_pDistributeWorkCallbacks )
		{
			g_pDistributeWorkCallbacks->OnWorkUnitsCompleted( iDoneWorkUnits );
		}

		return true;
	}

	virtual void DisconnectHandler( int workerID )
	{
		int iPartitionLookup = FindPartitionByWorker( workerID );
		if ( iPartitionLookup != -1 )
		{
			// Mark this guy's partition as unowned so another worker can get it.
			CPartitionInfo *pPartition = m_Partitions[iPartitionLookup];
			pPartition->m_iWorker = -1;
		}
	}

	CPartitionInfo* AddPartition( int iWorker )
	{
		CPartitionInfo *pNew = new CPartitionInfo;
		pNew->m_iPartition = m_Partitions.AddToTail( pNew );
		pNew->m_iWorker = iWorker;
		return pNew;
	}

	bool SplitWUsPartition( CPartitionInfo *pPartitionLarge,
						CPartitionInfo **ppFirstHalf, CPartitionInfo **ppSecondHalf,
						int iFirstHalfWorker, int iSecondHalfWorker )
	{
		int nCount = pPartitionLarge->m_WUs.Count();
		
		if ( nCount > 1 )	// Allocate the partitions for the two workers
		{
			*ppFirstHalf = AddPartition( iFirstHalfWorker );
			*ppSecondHalf = AddPartition( iSecondHalfWorker );
		}
		else				// Specially transfer a partition with too few work units
		{
			*ppFirstHalf = NULL;
			*ppSecondHalf = AddPartition( iSecondHalfWorker );
		}

		// Prepare for transfer
		CPartitionInfo *arrNewParts[2] = { *ppFirstHalf ? *ppFirstHalf : *ppSecondHalf, *ppSecondHalf };
		
		// Transfer the work units:
		// alternate first/second halves
		// don't put more than "half deal units" tasks into the second half
		// e.g.               { 1, 2, 3, 4 }
		// becomes: 1st half { 1, 2 }, 2nd half { 3, 4 }
		for ( int k = 0; k < nCount; ++ k )
		{
			int iHead = pPartitionLarge->m_WUs.Head();
			WUIndexType iWU = pPartitionLarge->m_WUs[ iHead ];
			pPartitionLarge->m_WUs.Remove( iHead );

			/*
			int nHalf = !!( ( k % 2 ) || ( k >= nCount - 1 ) );
			if ( k == 5 ) // no more than 2 jobs to branch off
				arrNewParts[ 1 ] = arrNewParts[ 0 ];
				*/
			int nHalf = !( k < nCount/2 );
			CPartitionInfo *pTo = arrNewParts[ nHalf ];

			CWULookupInfo &li = m_WULookup.Get( iWU );
			li.m_iPartition = pTo->m_iPartition;
			li.m_iPartitionListIndex = pTo->m_WUs.AddToTail( iWU );
		}

		// LogPartitionsWorkUnits( pInfo );
		return true;
	}

	void AssignWUsToWorker( int iWorker )
	{
		// Get rid of this worker's old partition.
		int iPrevious = FindPartitionByWorker( iWorker );
		if ( iPrevious != -1 )
		{
			delete m_Partitions[iPrevious];
			m_Partitions.Remove( iPrevious );
		}

		if ( g_iVMPIVerboseLevel >= 1 )
			Msg( "A" );


		CVisibleWindowVector< CWULookupInfo > &vlkup = m_WULookup;
		if ( CommandLine()->FindParm( "-mpi_NoScheduler" ) )
		{
			Warning( "\n\n-mpi_NoScheduler found: Warning - this should only be used for testing and with 1 worker!\n\n" );
			vlkup.ExpandWindow( m_pInfo->m_nWorkUnits );
			CPartitionInfo *pPartition = AddPartition( iWorker );
			for ( int i=0; i < m_pInfo->m_nWorkUnits; i++ )
			{
				CWorkUnitInfo info;
				info.m_iWorkUnit = i;

				CWULookupInfo &li = vlkup.Get( i );
				li.m_iPartition = pPartition->m_iPartition;
				li.m_iPartitionListIndex = pPartition->m_WUs.AddToTail( i );
				li.m_iWUInfo = m_WUInfo.AddToTail( info );
			}

			SendPartitionToWorker( pPartition, iWorker );
			return;
		}
		

		// Any partitions abandoned by workers?
		int iAbandonedPartition = FindPartitionByWorker( -1 );
		if ( -1 != iAbandonedPartition )
		{
			CPartitionInfo *pPartition = m_Partitions[ iAbandonedPartition ];
			pPartition->m_iWorker = iWorker;
			SendPartitionToWorker( pPartition, iWorker );
		}
		
		// Any absolutely untouched partitions yet?
		else if ( vlkup.PastVisibleIndex() < vlkup.PastPossibleIndex() )
		{
			// Figure out how many WUs to include in a batch
			int numWusToDeal = s_numWusToDeal;
			if ( numWusToDeal <= 0 )
			{
				uint64 uiFraction = vlkup.PastPossibleIndex() / g_nMaxWorkerCount;
				Assert( uiFraction < INT_MAX/2 );
				
				numWusToDeal = int( uiFraction );
				if ( numWusToDeal <= 0 )
					numWusToDeal = 8;
			}

			// Allocate room for upcoming work units lookup
			WUIndexType iBegin = vlkup.PastVisibleIndex();
			WUIndexType iEnd = min( iBegin + g_nMaxWorkerCount * numWusToDeal, vlkup.PastPossibleIndex() );
			vlkup.ExpandWindow( iEnd - 1 );

			// Allocate a partition
			size_t numPartitions = min( ( size_t )(iEnd - iBegin), ( size_t )g_nMaxWorkerCount );
			CArrayAutoPtr< CPartitionInfo * > spArrPartitions( new CPartitionInfo* [ numPartitions ] );
			CPartitionInfo **arrPartitions = spArrPartitions.Get();

			arrPartitions[0] = AddPartition( iWorker );
			for ( size_t k = 1; k < numPartitions; ++ k )
				arrPartitions[k] = AddPartition( -1 );

			// Assign upcoming work units to the partitions.
			for ( WUIndexType i = iBegin ; i < iEnd; ++ i )
			{
				CWorkUnitInfo info;
				info.m_iWorkUnit = i;

				CPartitionInfo *pPartition = arrPartitions[ size_t( (i - iBegin) % numPartitions ) ];

				CWULookupInfo &li = vlkup.Get( i );
				li.m_iPartition = pPartition->m_iPartition;
				li.m_iPartitionListIndex = pPartition->m_WUs.AddToTail( i );
				li.m_iWUInfo = m_WUInfo.AddToTail( info );
			}

			// Now send this guy the WU list in his partition.
			SendPartitionToWorker( arrPartitions[0], iWorker );
		}

		// Split one of the last partitions to finish sooner
		else
		{
			// Find a partition to split.
			int iPartToSplit = FindSoonestPartition();
			if ( iPartToSplit >= 0 )
			{
				CPartitionInfo *pPartition = m_Partitions[ iPartToSplit ];

				CPartitionInfo *pOldHalf = NULL, *pNewHalf = NULL;
				int iOldWorker = pPartition->m_iWorker, iNewWorker = iWorker;
				if ( SplitWUsPartition( pPartition, &pOldHalf, &pNewHalf, iOldWorker, iNewWorker ) )
				{
					if ( pOldHalf )
						SendPartitionToWorker( pOldHalf, iOldWorker );
					if ( pNewHalf )
						SendPartitionToWorker( pNewHalf, iNewWorker );

					// Delete the partition that got split
					Assert( pPartition->m_WUs.Count() == 0 );
					delete pPartition;
					m_Partitions.Remove( iPartToSplit );
				}
			}
		}
	}

	int FindSoonestPartition()
	{
		CUtlLinkedList < CPartitionInfo *, int > &lst = m_Partitions;

		// Sorted partitions
		CUtlMap< CPartitionInfo::PartitionWUs *, int > sortedPartitions ( CompareSoonestWorkUnitSets );
		sortedPartitions.EnsureCapacity( lst.Count() );
		FOR_EACH_LL( lst, i )
		{
			sortedPartitions.Insert( &lst[i]->m_WUs, i );
		}

		if ( sortedPartitions.Count() )
		{
			return sortedPartitions.Element( sortedPartitions.FirstInorder() );
		}

		return lst.Head();
	}

	int FindPartitionByWorker( int iWorker )
	{
		FOR_EACH_LL( m_Partitions, i )
		{
			if ( m_Partitions[i]->m_iWorker == iWorker )
				return i;
		}
		return -1;
	}

	void SendPartitionToWorker( CPartitionInfo *pPartition, int iWorker )
	{
		// Stuff the next nWUs work units into the buffer.
		MessageBuffer mb;
		PrepareDistributeWorkHeader( &mb, DW_SUBPACKETID_WU_ASSIGNMENT );
		
		FOR_EACH_LL( pPartition->m_WUs, i )
		{
			WUIndexType iWU = pPartition->m_WUs[i];
			mb.write( &iWU, sizeof( iWU ) );
			VMPITracker_WorkUnitSentToWorker( ( int ) iWU, iWorker );
		}

		VMPI_SendData( mb.data, mb.getLen(), iWorker );
	}

	virtual bool HandlePacket( MessageBuffer *pBuf, int iSource, bool bIgnoreContents )
	{
		return false;
	}
	
private:
	
	CDSInfo *m_pInfo;

	CUtlLinkedList<CPartitionInfo*,int> m_Partitions;	
	CVisibleWindowVector<CWULookupInfo> m_WULookup;		// Map work unit index to CWorkUnitInfo.
	CUtlLinkedList<CWorkUnitInfo,int> m_WUInfo;			// Sorted with most elegible WU at the head.
};



class CDistributor_DefaultWorker : public IWorkUnitDistributorWorker
{
public:
	virtual void Release()
	{
		delete this;
	}

	virtual void Init( CDSInfo *pInfo )
	{
	}
	
	virtual bool GetNextWorkUnit( WUIndexType *pWUIndex )
	{
		CVMPICriticalSectionLock csLock( &m_CS );
		csLock.Lock();

		// NOTE: this is called from INSIDE worker threads.
		if ( m_WorkUnits.Count() == 0 )
		{
			return false;
		}
		else
		{
			*pWUIndex = m_WorkUnits[ m_WorkUnits.Head() ];
			m_WorkUnits.Remove( m_WorkUnits.Head() );
			return true;
		}
	}

	virtual void NoteLocalWorkUnitCompleted( WUIndexType iWU )
	{
	}

	virtual bool HandlePacket( MessageBuffer *pBuf, int iSource, bool bIgnoreContents )
	{
		if ( pBuf->data[1] == DW_SUBPACKETID_WU_ASSIGNMENT )
		{
			// If the message wasn't even related to the current DistributeWork() call we're on, ignore it.
			if ( bIgnoreContents )
				return true;

			if ( ((pBuf->getLen() - pBuf->getOffset()) % sizeof( WUIndexType )) != 0 )
			{
				Error( "DistributeWork: invalid work units packet from master" );
			}

			// Parse out the work unit indices.
			CVMPICriticalSectionLock csLock( &m_CS );
			csLock.Lock();

				m_WorkUnits.Purge();

				int nIndices = (pBuf->getLen() - pBuf->getOffset()) / sizeof( WUIndexType );
				for ( int i=0; i < nIndices; i++ )
				{
					WUIndexType iWU;
					pBuf->read( &iWU, sizeof( iWU ) );

					// Add the index to the list.
					m_WorkUnits.AddToTail( iWU );
				}
			
			csLock.Unlock();

			return true;
		}
		else
		{
			return false;
		}
	}

	// Threads eat up the list of WUs in here.
	CVMPICriticalSection m_CS;
	CUtlLinkedList<WUIndexType, int> m_WorkUnits;			// A list of work units assigned to this worker
};




IWorkUnitDistributorMaster* CreateWUDistributor_DefaultMaster()
{
	return new CDistributor_DefaultMaster;
}
IWorkUnitDistributorWorker* CreateWUDistributor_DefaultWorker()
{
	return new CDistributor_DefaultWorker;
}
