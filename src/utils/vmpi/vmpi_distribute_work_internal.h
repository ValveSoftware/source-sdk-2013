//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef VMPI_DISTRIBUTE_WORK_INTERNAL_H
#define VMPI_DISTRIBUTE_WORK_INTERNAL_H
#ifdef _WIN32
#pragma once
#endif


#define VMPI_DISTRIBUTE_WORK_EXTRA_SUBPACKET_BASE 50


typedef uint64 WUIndexType;
class CDSInfo;
extern bool g_bVMPIEarlyExit;


// These classes are overridden to handle the details of communicating and scheduling work units.
class IWorkUnitDistributorMaster
{
public:
	virtual void Release() = 0;

	virtual void DistributeWork_Master( CDSInfo *pInfo ) = 0;

	virtual void OnWorkerReady( int iWorker ) = 0;
	virtual bool HandlePacket( MessageBuffer *pBuf, int iSource, bool bIgnoreContents ) = 0;

	// Called when the results for a work unit arrive. This function must return false if
	// we've already received the results for this work unit.
	virtual bool HandleWorkUnitResults( WUIndexType iWorkUnit ) = 0;

	virtual void DisconnectHandler( int workerID ) = 0;
};

class IWorkUnitDistributorWorker
{
public:
	virtual void Release() = 0;
	
	virtual void Init( CDSInfo *pInfo ) = 0;
	
	// Called by worker threads to get the next work unit to do.
	virtual bool GetNextWorkUnit( WUIndexType *pWUIndex ) = 0;
	
	// Called by the worker threads after a work unit is completed.
	virtual void NoteLocalWorkUnitCompleted( WUIndexType iWU ) = 0;

	// Called by the main thread when a packet comes in.
	virtual bool HandlePacket( MessageBuffer *pBuf, int iSource, bool bIgnoreContents ) = 0;
};



template < typename T, typename Derived >
class CVisibleWindowVectorT : protected CUtlVector < T >
{
	typedef CUtlVector< T > BaseClass;

public:
	CVisibleWindowVectorT() : m_uiBase( 0 ), m_uiTotal( 0 ) {}

public:
	inline void PostInitElement( uint64 uiRealIdx, T &newElement ) { /* do nothing */ return; }
	inline void PostInitElement( uint64 uiRealIdx, T &newElement, T const &x ) { newElement = x; }

public:
	// Resets the content and makes size "uiTotal"
	void Reset( uint64 uiTotal ) {
		BaseClass::RemoveAll();
		BaseClass::EnsureCapacity( min( 100, (int)uiTotal ) );
		m_uiBase = 0;
		m_uiTotal = uiTotal;
	}

	// Gets the element from the window, otherwise NULL
	T & Get( uint64 idx ) {
		T *pElement = (( idx >= m_uiBase && idx < m_uiBase + BaseClass::Count() ) ? ( BaseClass::Base() + size_t( idx - m_uiBase ) ) : NULL );
		Assert( pElement );
		return *pElement;
	}

	// Expands the window to see the element by its index
	void ExpandWindow( uint64 idxAccessible ) {
		Assert( idxAccessible >= m_uiBase );
		if ( idxAccessible >= m_uiBase + BaseClass::Count() ) {
			int iOldCount = BaseClass::Count();
			int iAddElements = int(idxAccessible - m_uiBase) - iOldCount + 1;
			Assert( iOldCount + iAddElements <= 100000000 /* really need 100 Mb at once? */ );
			BaseClass::AddMultipleToTail( iAddElements );
			for ( int iNewCount = BaseClass::Count(); iOldCount < iNewCount; ++ iOldCount )
				static_cast< Derived * >( this )->PostInitElement( m_uiBase + iOldCount, BaseClass::Element( iOldCount ) );
		}
	}

	// Expands the window and initializes the new elements to a given value
	void ExpandWindow( uint64 idxAccessible, T const& x ) {
		Assert( idxAccessible >= m_uiBase );
		if ( idxAccessible >= m_uiBase + BaseClass::Count() ) {
			int iOldCount = BaseClass::Count();
			int iAddElements = int(idxAccessible - m_uiBase) - iOldCount + 1;
			Assert( unsigned(iAddElements) <= 50000000 /* growing 50 Mb at once? */ );
			BaseClass::AddMultipleToTail( iAddElements );
			for ( int iNewCount = BaseClass::Count(); iOldCount < iNewCount; ++ iOldCount )
				static_cast< Derived * >( this )->PostInitElement( m_uiBase + iOldCount, BaseClass::Element( iOldCount ), x );
		}
	}

	// Shrinks the window to drop some of the elements from the head
	void ShrinkWindow( uint64 idxDrop ) {
		Assert( idxDrop >= m_uiBase && idxDrop <= m_uiBase + BaseClass::Count() );
		if ( idxDrop >= m_uiBase && idxDrop <= m_uiBase + BaseClass::Count() ) {
			int iDropElements = int( idxDrop - m_uiBase ) + 1;
			m_uiBase += iDropElements;
			BaseClass::RemoveMultiple( 0, min( iDropElements, BaseClass::Count() ) );
		}
	}

	// First possible index in this vector (only past dropped items)
	uint64 FirstPossibleIndex() const { return m_uiBase; }

	// Last possible index in this vector
	uint64 PastPossibleIndex() const { return m_uiTotal; }
	// Past visible window index in this vector
	uint64 PastVisibleIndex() const { return m_uiBase + BaseClass::Count(); }

protected:
	uint64 m_uiBase, m_uiTotal;
};

template < typename T >
class CVisibleWindowVector : public CVisibleWindowVectorT < T, CVisibleWindowVector< T > >
{
public:
	CVisibleWindowVector() {}
};


template < typename T >
T const * GenericFind( T const *pBegin, T const *pEnd, T const &x )
{
	for ( ; pBegin != pEnd; ++ pBegin )
		if ( *pBegin == x )
			break;
	return pBegin;
}


template < typename T >
T * GenericFind( T *pBegin, T *pEnd, T const &x )
{
	for ( ; pBegin != pEnd; ++ pBegin )
		if ( *pBegin == x )
			break;
	return pBegin;
}


class CWorkerInfo
{
public:
	ProcessWorkUnitFn m_pProcessFn;
	CVisibleWindowVector<WUIndexType> m_WorkUnitsRunning;	// A list of work units currently running, index is the thread index
	CVMPICriticalSection m_WorkUnitsRunningCS;
};


class CMasterInfo
{
public:

	// Only used by the master.
	ReceiveWorkUnitFn m_ReceiveFn;
};


class CDSInfo
{
public:
	inline void WriteWUIndex( WUIndexType iWU, MessageBuffer *pBuf )
	{
		if ( m_nWorkUnits <= 0xFFFF )
		{
			Assert( iWU <= 0xFFFF );
			unsigned short val = (unsigned short)iWU;
			pBuf->write( &val, sizeof( val ) );
		}
		else if ( m_nWorkUnits <= 0xFFFFFFFF )
		{
			Assert( iWU <= 0xFFFF );
			unsigned int val = (unsigned int)iWU;
			pBuf->write( &val, sizeof( val ) );
		}
		else
		{
			pBuf->write( &iWU, sizeof( iWU ) );
		}
	}
	
	inline void ReadWUIndex( WUIndexType *pWU, MessageBuffer *pBuf )
	{
		if ( m_nWorkUnits <= 0xFFFF )
		{
			unsigned short val;
			pBuf->read( &val, sizeof( val ) );
			*pWU = val;
		}
		else if ( m_nWorkUnits <= 0xFFFFFFFF )
		{
			unsigned int val;
			pBuf->read( &val, sizeof( val ) );
			*pWU = val;
		}
		else
		{
			pBuf->read( pWU, sizeof( *pWU ) );
		}
	}

public:
	CWorkerInfo m_WorkerInfo;
	CMasterInfo m_MasterInfo;

	bool m_bMasterReady;	// Set to true when the master is ready to go.
	bool m_bMasterFinished;
	WUIndexType m_nWorkUnits;
	char m_cPacketID;
};


// Called to write the packet ID, the subpacket ID, and the ID of which DistributeWork() call we're at.
void PrepareDistributeWorkHeader( MessageBuffer *pBuf, unsigned char cSubpacketID );

// Called from threads on the master to process a completed work unit.
void NotifyLocalMasterCompletedWorkUnit( WUIndexType iWorkUnit );
void CheckLocalMasterCompletedWorkUnits();


// Creation functions for different distributors.
IWorkUnitDistributorMaster* CreateWUDistributor_DefaultMaster();
IWorkUnitDistributorWorker* CreateWUDistributor_DefaultWorker();

IWorkUnitDistributorMaster* CreateWUDistributor_SDKMaster();
IWorkUnitDistributorWorker* CreateWUDistributor_SDKWorker();


#endif // VMPI_DISTRIBUTE_WORK_INTERNAL_H
