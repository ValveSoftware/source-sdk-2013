//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/threadtools.h"
#include "utlmultilist.h"
#include "utlvector.h"

FORWARD_DECLARE_HANDLE( memhandle_t );

#define INVALID_MEMHANDLE ((memhandle_t)(intp)-1)

class CDataManagerBase
{
public:

	// public API
	// -----------------------------------------------------------------------------
	// memhandle_t			CreateResource( params ) // implemented by derived class
	void					DestroyResource( memhandle_t handle );

	// type-safe implementation in derived class
	//void					*LockResource( memhandle_t handle );
	int						UnlockResource( memhandle_t handle );
	void					TouchResource( memhandle_t handle );
	void					MarkAsStale( memhandle_t handle );		// move to head of LRU

	int						LockCount( memhandle_t handle );
	int						BreakLock( memhandle_t handle );
	int						BreakAllLocks();

	// HACKHACK: For convenience - offers no lock protection 
	// type-safe implementation in derived class
	//void					*GetResource_NoLock( memhandle_t handle );

	unsigned int			TargetSize();
	unsigned int			AvailableSize();
	unsigned int			UsedSize();

	void					NotifySizeChanged( memhandle_t handle, unsigned int oldSize, unsigned int newSize );

	void					SetTargetSize( unsigned int targetSize );

	// NOTE: flush is equivalent to Destroy
	unsigned int			FlushAllUnlocked();
	unsigned int			FlushToTargetSize();
	unsigned int			FlushAll();
	unsigned int			Purge( unsigned int nBytesToPurge );
	unsigned int			EnsureCapacity( unsigned int size );

	// Thread lock
	virtual void			Lock() {}
	virtual bool			TryLock() { return true; }
	virtual void			Unlock() {}

	// Iteration

	// -----------------------------------------------------------------------------

	// Debugging only!!!!
	void					GetLRUHandleList( CUtlVector< memhandle_t >& list );
	void					GetLockHandleList( CUtlVector< memhandle_t >& list );


protected:
	// derived class must call these to implement public API
	unsigned short			CreateHandle( bool bCreateLocked );
	memhandle_t				StoreResourceInHandle( unsigned short memoryIndex, void *pStore, unsigned int realSize );
	void					*GetResource_NoLock( memhandle_t handle );
	void					*GetResource_NoLockNoLRUTouch( memhandle_t handle );
	void					*LockResource( memhandle_t handle );

	// NOTE: you must call this from the destructor of the derived class! (will assert otherwise)
	void					FreeAllLists()	{ FlushAll(); m_listsAreFreed = true; }

							CDataManagerBase( unsigned int maxSize );
	virtual					~CDataManagerBase();
	
	
	inline unsigned int		MemTotal_Inline() const { return m_targetMemorySize; }
	inline unsigned int		MemAvailable_Inline() const { return m_targetMemorySize - m_memUsed; }
	inline unsigned int		MemUsed_Inline() const { return m_memUsed; }

// Implemented by derived class:
	virtual void			DestroyResourceStorage( void * ) = 0;
	virtual unsigned int	GetRealSize( void * ) = 0;

	memhandle_t				ToHandle( unsigned short index );
	unsigned short			FromHandle( memhandle_t handle );
	
	void					TouchByIndex( unsigned short memoryIndex );
	void *					GetForFreeByIndex( unsigned short memoryIndex );

	// One of these is stored per active allocation
	struct resource_lru_element_t
	{
		resource_lru_element_t()
		{
			lockCount = 0;
			serial = 1;
			pStore = 0;
		}

		unsigned short lockCount;
		unsigned short serial;
		void	*pStore;
	};

	unsigned int m_targetMemorySize;
	unsigned int m_memUsed;
	
	CUtlMultiList< resource_lru_element_t, unsigned short >  m_memoryLists;
	
	unsigned short m_lruList;
	unsigned short m_lockList;
	unsigned short m_freeList;
	unsigned short m_listsAreFreed : 1;
	unsigned short m_unused : 15;

};

template< class STORAGE_TYPE, class CREATE_PARAMS, class LOCK_TYPE = STORAGE_TYPE *, class MUTEX_TYPE = CThreadNullMutex>
class CDataManager : public CDataManagerBase
{
	typedef CDataManagerBase BaseClass;
public:

	CDataManager( unsigned int size = (unsigned)-1 ) : BaseClass(size) {}
	

	~CDataManager()
	{
		// NOTE: This must be called in all implementations of CDataManager
		FreeAllLists();
	}

	// Use GetData() to translate pointer to LOCK_TYPE
	LOCK_TYPE LockResource( memhandle_t hMem )
	{
		void *pLock = BaseClass::LockResource( hMem );
		if ( pLock )
		{
			return StoragePointer(pLock)->GetData();
		}

		return NULL;
	}

	// Use GetData() to translate pointer to LOCK_TYPE
	LOCK_TYPE GetResource_NoLock( memhandle_t hMem )
	{
		void *pLock = const_cast<void *>(BaseClass::GetResource_NoLock( hMem ));
		if ( pLock )
		{
			return StoragePointer(pLock)->GetData();
		}
		return NULL;
	}

	// Use GetData() to translate pointer to LOCK_TYPE
	// Doesn't touch the memory LRU
	LOCK_TYPE GetResource_NoLockNoLRUTouch( memhandle_t hMem )
	{
		void *pLock = const_cast<void *>(BaseClass::GetResource_NoLockNoLRUTouch( hMem ));
		if ( pLock )
		{
			return StoragePointer(pLock)->GetData();
		}
		return NULL;
	}

	// Wrapper to match implementation of allocation with typed storage & alloc params.
	memhandle_t CreateResource( const CREATE_PARAMS &createParams, bool bCreateLocked = false )
	{
		BaseClass::EnsureCapacity((unsigned int)STORAGE_TYPE::EstimatedSize(createParams));
		unsigned short memoryIndex = BaseClass::CreateHandle( bCreateLocked );
		STORAGE_TYPE *pStore = STORAGE_TYPE::CreateResource( createParams );
		return BaseClass::StoreResourceInHandle( memoryIndex, pStore, (unsigned int) pStore->Size() );
	}

	// Iteration. Must lock first
	memhandle_t GetFirstUnlocked()
	{
		unsigned node = m_memoryLists.Head(m_lruList);
		if ( node == m_memoryLists.InvalidIndex() )
		{
			return INVALID_MEMHANDLE;
		}
		return ToHandle( node );
	}

	memhandle_t GetFirstLocked()
	{
		unsigned node = m_memoryLists.Head(m_lockList);
		if ( node == m_memoryLists.InvalidIndex() )
		{
			return INVALID_MEMHANDLE;
		}
		return ToHandle( node );
	}

	memhandle_t GetNext( memhandle_t hPrev )
	{
		if ( hPrev == INVALID_MEMHANDLE )
		{
			return INVALID_MEMHANDLE;
		}

		unsigned short iNext = m_memoryLists.Next( FromHandle( hPrev ) );
		if ( iNext == m_memoryLists.InvalidIndex() ) 
		{
			return INVALID_MEMHANDLE;
		}

		return ToHandle( iNext );
	}

	MUTEX_TYPE &AccessMutex()	{ return m_mutex; }
	virtual void Lock() { m_mutex.Lock(); }
	virtual bool TryLock() { return m_mutex.TryLock(); }
	virtual void Unlock() { m_mutex.Unlock(); }

private:
	STORAGE_TYPE *StoragePointer( void *pMem )
	{
		return static_cast<STORAGE_TYPE *>(pMem);
	}

	virtual void DestroyResourceStorage( void *pStore )
	{
		StoragePointer(pStore)->DestroyResource();
	}
	
	virtual unsigned int GetRealSize( void *pStore )
	{
		return (unsigned int) StoragePointer(pStore)->Size();
	}

	MUTEX_TYPE m_mutex;
};

//-----------------------------------------------------------------------------

inline unsigned short CDataManagerBase::FromHandle( memhandle_t handle )
{
	unsigned int fullWord = (unsigned int)reinterpret_cast<uintp>( handle );
	unsigned short serial = fullWord>>16;
	unsigned short index = fullWord & 0xFFFF;
	index--;
	if ( m_memoryLists.IsValidIndex(index) && m_memoryLists[index].serial == serial )
		return index;
	return m_memoryLists.InvalidIndex();
}

inline int CDataManagerBase::LockCount( memhandle_t handle )
{
	Lock();
	int result = 0;
	unsigned short memoryIndex = FromHandle(handle);
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		result = m_memoryLists[memoryIndex].lockCount;
	}
	Unlock();
	return result;
}


#endif // RESOURCEMANAGER_H
