
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "basetypes.h"
#include "datamanager.h"

DECLARE_POINTER_HANDLE( memhandle_t );

CDataManagerBase::CDataManagerBase( unsigned int maxSize )
{
	m_targetMemorySize = maxSize;
	m_memUsed = 0;
	m_lruList = m_memoryLists.CreateList();
	m_lockList = m_memoryLists.CreateList();
	m_freeList = m_memoryLists.CreateList();
	m_listsAreFreed = 0;
}

CDataManagerBase::~CDataManagerBase() 
{
	Assert( m_listsAreFreed );
}

void CDataManagerBase::NotifySizeChanged( memhandle_t handle, unsigned int oldSize, unsigned int newSize )
{
	Lock();
	m_memUsed += (int)newSize - (int)oldSize;
	Unlock();
}

void CDataManagerBase::SetTargetSize( unsigned int targetSize )
{
	m_targetMemorySize = targetSize;
}

unsigned int CDataManagerBase::FlushAllUnlocked()
{
	Lock();

	int nFlush = m_memoryLists.Count( m_lruList );
	void **pScratch = (void **)_alloca( nFlush * sizeof(void *) );
	CUtlVector<void *> destroyList( pScratch, nFlush );

	unsigned nBytesInitial = MemUsed_Inline();

	int node = m_memoryLists.Head(m_lruList);
	while ( node != m_memoryLists.InvalidIndex() )
	{
		int next = m_memoryLists.Next(node);
		m_memoryLists.Unlink( m_lruList, node );
		destroyList.AddToTail( GetForFreeByIndex( node ) );
		node = next;
	}

	Unlock();

	for ( int i = 0; i < nFlush; i++ )
	{
		DestroyResourceStorage( destroyList[i] );
	}

	return ( nBytesInitial - MemUsed_Inline() );
}

unsigned int CDataManagerBase::FlushToTargetSize()
{
	return EnsureCapacity(0);
}

// Frees everything!  The LRU AND the LOCKED items.  This is only used to forcibly free the resources,
// not to make space.

unsigned int CDataManagerBase::FlushAll()
{
	Lock();

	int nFlush = m_memoryLists.Count( m_lruList ) + m_memoryLists.Count( m_lockList );
	void **pScratch = (void **)_alloca( nFlush * sizeof(void *) );
	CUtlVector<void *> destroyList( pScratch, nFlush );

	unsigned result = MemUsed_Inline();
	int node;
	int nextNode;

	node = m_memoryLists.Head(m_lruList);
	while ( node != m_memoryLists.InvalidIndex() )
	{
		nextNode = m_memoryLists.Next(node);
		m_memoryLists.Unlink( m_lruList, node );
		destroyList.AddToTail( GetForFreeByIndex( node ) );
		node = nextNode;
	}

	node = m_memoryLists.Head(m_lockList);
	while ( node != m_memoryLists.InvalidIndex() )
	{
		nextNode = m_memoryLists.Next(node);
		m_memoryLists.Unlink( m_lockList, node );
		m_memoryLists[node].lockCount = 0;
		destroyList.AddToTail( GetForFreeByIndex( node ) );
		node = nextNode;
	}

	m_listsAreFreed = false;
	Unlock();

	for ( int i = 0; i < nFlush; i++ )
	{
		DestroyResourceStorage( destroyList[i] );
	}

	return result;
}

unsigned int CDataManagerBase::Purge( unsigned int nBytesToPurge )
{
	unsigned int nTargetSize = MemUsed_Inline() - nBytesToPurge;
	// Check for underflow
	if ( MemUsed_Inline() < nBytesToPurge )
		nTargetSize = 0;
	unsigned int nImpliedCapacity = MemTotal_Inline() - nTargetSize;
	return EnsureCapacity( nImpliedCapacity );
}


void CDataManagerBase::DestroyResource( memhandle_t handle )
{
	Lock();
	unsigned short index = FromHandle( handle );
	if ( !m_memoryLists.IsValidIndex(index) )
	{
		Unlock();
		return;
	}
	
	Assert( m_memoryLists[index].lockCount == 0  );
	if ( m_memoryLists[index].lockCount )
		BreakLock( handle );
	m_memoryLists.Unlink( m_lruList, index );
	void *p = GetForFreeByIndex( index );
	Unlock();

	DestroyResourceStorage( p );
}


void *CDataManagerBase::LockResource( memhandle_t handle )
{
	AUTO_LOCK( *this );

	unsigned short memoryIndex = FromHandle(handle);
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		if ( m_memoryLists[memoryIndex].lockCount == 0 )
		{
			m_memoryLists.Unlink( m_lruList, memoryIndex );
			m_memoryLists.LinkToTail( m_lockList, memoryIndex );
		}
		Assert(m_memoryLists[memoryIndex].lockCount != (unsigned short)-1);
		m_memoryLists[memoryIndex].lockCount++;
		return m_memoryLists[memoryIndex].pStore;
	}

	return NULL;
}

int CDataManagerBase::UnlockResource( memhandle_t handle )
{
	AUTO_LOCK( *this );
	unsigned short memoryIndex = FromHandle(handle);
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		Assert( m_memoryLists[memoryIndex].lockCount > 0 );
		if ( m_memoryLists[memoryIndex].lockCount > 0 )
		{
			m_memoryLists[memoryIndex].lockCount--;
			if ( m_memoryLists[memoryIndex].lockCount == 0 )
			{
				m_memoryLists.Unlink( m_lockList, memoryIndex );
				m_memoryLists.LinkToTail( m_lruList, memoryIndex );
			}
		}
		return m_memoryLists[memoryIndex].lockCount;
	}

	return 0;
}

void *CDataManagerBase::GetResource_NoLockNoLRUTouch( memhandle_t handle )
{
	AUTO_LOCK( *this );
	unsigned short memoryIndex = FromHandle(handle);
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		return m_memoryLists[memoryIndex].pStore;
	}
	return NULL;
}


void *CDataManagerBase::GetResource_NoLock( memhandle_t handle )
{
	AUTO_LOCK( *this );
	unsigned short memoryIndex = FromHandle(handle);
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		TouchByIndex( memoryIndex );
		return m_memoryLists[memoryIndex].pStore;
	}
	return NULL;
}

void CDataManagerBase::TouchResource( memhandle_t handle )
{
	AUTO_LOCK( *this );
	TouchByIndex( FromHandle(handle) );
}

void CDataManagerBase::MarkAsStale( memhandle_t handle )
{
	AUTO_LOCK( *this );
	unsigned short memoryIndex = FromHandle(handle);
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		if ( m_memoryLists[memoryIndex].lockCount == 0 )
		{
			m_memoryLists.Unlink( m_lruList, memoryIndex );
			m_memoryLists.LinkToHead( m_lruList, memoryIndex );
		}
	}
}

int CDataManagerBase::BreakLock( memhandle_t handle )
{
	AUTO_LOCK( *this );
	unsigned short memoryIndex = FromHandle(handle);
	if ( memoryIndex != m_memoryLists.InvalidIndex() && m_memoryLists[memoryIndex].lockCount )
	{
		int nBroken = m_memoryLists[memoryIndex].lockCount;
		m_memoryLists[memoryIndex].lockCount = 0;
		m_memoryLists.Unlink( m_lockList, memoryIndex );
		m_memoryLists.LinkToTail( m_lruList, memoryIndex );

		return nBroken;
	}
	return 0;
}

int CDataManagerBase::BreakAllLocks()
{
	AUTO_LOCK( *this );
	int nBroken = 0;
	int node;
	int nextNode;

	node = m_memoryLists.Head(m_lockList);
	while ( node != m_memoryLists.InvalidIndex() )
	{
		nBroken++;
		nextNode = m_memoryLists.Next(node);
		m_memoryLists[node].lockCount = 0;
		m_memoryLists.Unlink( m_lockList, node );
		m_memoryLists.LinkToTail( m_lruList, node );
		node = nextNode;
	}

	return nBroken;

}

unsigned short CDataManagerBase::CreateHandle( bool bCreateLocked )
{
	AUTO_LOCK( *this );
	int memoryIndex = m_memoryLists.Head(m_freeList);
	unsigned short list = ( bCreateLocked ) ? m_lockList : m_lruList;
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		m_memoryLists.Unlink( m_freeList, memoryIndex );
		m_memoryLists.LinkToTail( list, memoryIndex );
	}
	else
	{
		memoryIndex = m_memoryLists.AddToTail( list );
	}

	if ( bCreateLocked )
	{
		m_memoryLists[memoryIndex].lockCount++;
	}

	return memoryIndex;
}

memhandle_t CDataManagerBase::StoreResourceInHandle( unsigned short memoryIndex, void *pStore, unsigned int realSize )
{
	AUTO_LOCK( *this );
	resource_lru_element_t &mem = m_memoryLists[memoryIndex];
	mem.pStore = pStore;
	m_memUsed += realSize;
	return ToHandle(memoryIndex);
}

void CDataManagerBase::TouchByIndex( unsigned short memoryIndex )
{
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		if ( m_memoryLists[memoryIndex].lockCount == 0 )
		{
			m_memoryLists.Unlink( m_lruList, memoryIndex );
			m_memoryLists.LinkToTail( m_lruList, memoryIndex );
		}
	}
}

memhandle_t CDataManagerBase::ToHandle( unsigned short index )
{
	unsigned int hiword = m_memoryLists.Element(index).serial;
	hiword <<= 16;
	index++;
	return reinterpret_cast< memhandle_t >( (uintp)( hiword|index ) );
}

unsigned int CDataManagerBase::TargetSize() 
{ 
	return MemTotal_Inline(); 
}

unsigned int CDataManagerBase::AvailableSize()
{ 
	return MemAvailable_Inline(); 
}


unsigned int CDataManagerBase::UsedSize()
{ 
	return MemUsed_Inline(); 
}

// free resources until there is enough space to hold "size"
unsigned int CDataManagerBase::EnsureCapacity( unsigned int size )
{
	unsigned nBytesInitial = MemUsed_Inline();
	while ( MemUsed_Inline() > MemTotal_Inline() || MemAvailable_Inline() < size )
	{
		Lock();
		int lruIndex = m_memoryLists.Head( m_lruList );
		if ( lruIndex == m_memoryLists.InvalidIndex() )
		{
			Unlock();
			break;
		}
		m_memoryLists.Unlink( m_lruList, lruIndex );
		void *p = GetForFreeByIndex( lruIndex );
		Unlock();
		DestroyResourceStorage( p );
	}
	return ( nBytesInitial - MemUsed_Inline() );
}

// free this resource and move the handle to the free list
void *CDataManagerBase::GetForFreeByIndex( unsigned short memoryIndex )
{
	void *p = NULL;
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		Assert( m_memoryLists[memoryIndex].lockCount == 0 );

		resource_lru_element_t &mem = m_memoryLists[memoryIndex];
		unsigned size = GetRealSize( mem.pStore );
		if ( size > m_memUsed )
		{
			ExecuteOnce( Warning( "Data manager 'used' memory incorrect\n" ) );
			size = m_memUsed;
		}
		m_memUsed -= size;
		p = mem.pStore;
		mem.pStore = NULL;
		mem.serial++;
		m_memoryLists.LinkToTail( m_freeList, memoryIndex );
	}
	return p;
}

// get a list of everything in the LRU
void CDataManagerBase::GetLRUHandleList( CUtlVector< memhandle_t >& list )
{
	for ( int node = m_memoryLists.Tail(m_lruList);
			node != m_memoryLists.InvalidIndex();
			node = m_memoryLists.Previous(node) )
	{
		list.AddToTail( ToHandle( node ) );
	}
}

// get a list of everything locked
void CDataManagerBase::GetLockHandleList( CUtlVector< memhandle_t >& list )
{
	for ( int node = m_memoryLists.Head(m_lockList);
			node != m_memoryLists.InvalidIndex();
			node = m_memoryLists.Next(node) )
	{
		list.AddToTail( ToHandle( node ) );
	}
}

