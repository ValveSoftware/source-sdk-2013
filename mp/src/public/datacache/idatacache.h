//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef IDATACACHE_H
#define IDATACACHE_H

#ifdef _WIN32
#pragma once
#endif


#include "tier0/dbg.h"
#include "appframework/IAppSystem.h"

class IDataCache;

//-----------------------------------------------------------------------------
//
// Shared Data Cache API
//
//-----------------------------------------------------------------------------

#define DATACACHE_INTERFACE_VERSION		"VDataCache003"

//-----------------------------------------------------------------------------
// Support types and enums
//-----------------------------------------------------------------------------

//---------------------------------------------------------
// Unique (per section) identifier for a cache item defined by client
//---------------------------------------------------------
typedef uint32 DataCacheClientID_t;


//---------------------------------------------------------
// Cache-defined handle for a cache item
//---------------------------------------------------------
FORWARD_DECLARE_HANDLE( memhandle_t );
typedef memhandle_t DataCacheHandle_t;
#define DC_INVALID_HANDLE ((DataCacheHandle_t)0)

//---------------------------------------------------------
// Cache Limits
//---------------------------------------------------------
struct DataCacheLimits_t
{
	DataCacheLimits_t( unsigned _nMaxBytes = (unsigned)-1, unsigned _nMaxItems = (unsigned)-1, unsigned _nMinBytes = 0, unsigned _nMinItems = 0 )
		: nMaxBytes(_nMaxBytes), 
		nMaxItems(_nMaxItems), 
		nMinBytes(_nMinBytes),
		nMinItems(_nMinItems)
	{
	}

	// Maximum levels permitted
	unsigned nMaxBytes;
	unsigned nMaxItems;

	// Minimum levels permitted
	unsigned nMinBytes;
	unsigned nMinItems;
};

//---------------------------------------------------------
// Cache status
//---------------------------------------------------------
struct DataCacheStatus_t
{
	// Current state of the cache
	unsigned nBytes;
	unsigned nItems;

	unsigned nBytesLocked;
	unsigned nItemsLocked;

	// Diagnostics
	unsigned nFindRequests;
	unsigned nFindHits;
};

//---------------------------------------------------------
// Cache options
//---------------------------------------------------------
enum DataCacheOptions_t
{
	DC_TRACE_ACTIVITY	= (1 << 0),
	DC_FORCE_RELOCATE	= (1 << 1),
	DC_ALWAYS_MISS		= (1 << 2),
	DC_VALIDATE			= (1 << 3),
};


//---------------------------------------------------------
// Cache report types
//---------------------------------------------------------
enum DataCacheReportType_t
{
	DC_SUMMARY_REPORT,
	DC_DETAIL_REPORT,
	DC_DETAIL_REPORT_LRU,
};


//---------------------------------------------------------
// Notifications to section clients on cache events
//---------------------------------------------------------
enum DataCacheNotificationType_t
{
	// Used internally to prohibit notifications
	DC_NONE,

	// Item is falling off the LRU and should be deleted, return false to block
	DC_AGE_DISCARD,

	// Item is being explicitly flushed and should be deleted, return false to block
	DC_FLUSH_DISCARD,

	// Item is being explicitly removed and should be deleted. Failure is not an option
	DC_REMOVED,

	// Cache is requesting item be relocated for debugging purposes
	DC_RELOCATE,

	// Item info should be output to console, return false to accept default handling 
	DC_PRINT_INF0,
};

//-------------------------------------

struct DataCacheNotification_t
{
	DataCacheNotificationType_t type;
	const char *				pszSectionName;
	DataCacheClientID_t			clientId;
	const void *				pItemData;
	unsigned					nItemSize;
};

//---------------------------------------------------------

const int DC_MAX_CLIENT_NAME = 15;
const int DC_MAX_ITEM_NAME = 511;

//---------------------------------------------------------
// Result codes
//---------------------------------------------------------
enum DataCacheRemoveResult_t
{
	DC_OK,
	DC_NOT_FOUND,
	DC_LOCKED,
};

//---------------------------------------------------------
// Add flags
//---------------------------------------------------------
enum DataCacheAddFlags_t
{
	DCAF_LOCK		= ( 1 << 0 ),
	DCAF_DEFAULT	= 0,
};



//-----------------------------------------------------------------------------
// IDataCacheSection
//
// Purpose: Implements a sub-section of the global cache. Subsections are
//			areas of the cache with thier own memory constraints and common
//			management.
//-----------------------------------------------------------------------------
abstract_class IDataCacheSection
{
public:
	//--------------------------------------------------------

	virtual IDataCache *GetSharedCache() = 0;
	virtual const char *GetName() = 0;

	//--------------------------------------------------------
	// Purpose: Controls cache size & options
	//--------------------------------------------------------
	virtual void SetLimits( const DataCacheLimits_t &limits ) = 0;
	virtual void SetOptions( unsigned options ) = 0;


	//--------------------------------------------------------
	// Purpose: Get the current state of the section
	//--------------------------------------------------------
	virtual void GetStatus( DataCacheStatus_t *pStatus, DataCacheLimits_t *pLimits = NULL ) = 0;


	//--------------------------------------------------------
	// Purpose: Add an item to the cache.  Purges old items if over budget, returns false if item was already in cache.
	//--------------------------------------------------------
	virtual void EnsureCapacity( unsigned nBytes, unsigned nItems = 1 ) = 0;


	//--------------------------------------------------------
	// Purpose: Add an item to the cache.  Purges old items if over budget, returns false if item was already in cache.
	//--------------------------------------------------------
	virtual bool Add( DataCacheClientID_t clientId, const void *pItemData, unsigned size, DataCacheHandle_t *pHandle ) = 0;

	//--------------------------------------------------------
	// Purpose: Finds an item in the cache, returns NULL if item is not in cache. Not a cheap operation if section not configured for fast find.
	//--------------------------------------------------------
	virtual DataCacheHandle_t Find( DataCacheClientID_t clientId ) = 0;


	//--------------------------------------------------------
	// Purpose: Get an item out of the cache and remove it. No callbacks are executed unless explicity specified.
	//--------------------------------------------------------
	virtual DataCacheRemoveResult_t Remove( DataCacheHandle_t handle, const void **ppItemData, unsigned *pItemSize = NULL, bool bNotify = false ) = 0;
	DataCacheRemoveResult_t Remove( DataCacheHandle_t handle, bool bNotify = false )	{ return Remove( handle, NULL, NULL, bNotify ); }


	//--------------------------------------------------------
	// Purpose: Returns if the data is currently in memory, but does *not* change its location in the LRU
	//--------------------------------------------------------
	virtual bool IsPresent( DataCacheHandle_t handle ) = 0;


	//--------------------------------------------------------
	// Purpose: Lock an item in the cache, returns NULL if item is not in the cache.
	//--------------------------------------------------------
	virtual void *Lock( DataCacheHandle_t handle ) = 0;


	//--------------------------------------------------------
	// Purpose: Unlock a previous lock.
	//--------------------------------------------------------
	virtual int Unlock( DataCacheHandle_t handle ) = 0;


	//--------------------------------------------------------
	// Purpose: Get an item without locking it, returns NULL if item is not in the cache. Use with care!
	//--------------------------------------------------------
	virtual void *Get( DataCacheHandle_t handle, bool bFrameLock = false ) = 0;
	virtual void *GetNoTouch( DataCacheHandle_t handle, bool bFrameLock = false ) = 0;

	//--------------------------------------------------------
	// Purpose: "Frame locking" (not game frame). A crude way to manage locks over relatively 
	//			short periods. Does not affect normal locks/unlocks
	//--------------------------------------------------------
	virtual int BeginFrameLocking() = 0;
	virtual bool IsFrameLocking() = 0;
	virtual void *FrameLock( DataCacheHandle_t handle ) = 0;
	virtual int EndFrameLocking() = 0;
	virtual int *GetFrameUnlockCounterPtr() = 0;


	//--------------------------------------------------------
	// Purpose: Lock management, not for the feint of heart
	//--------------------------------------------------------
	virtual int GetLockCount( DataCacheHandle_t handle ) = 0;
	virtual int BreakLock( DataCacheHandle_t handle ) = 0;


	//--------------------------------------------------------
	// Purpose: Explicitly mark an item as "recently used"
	//--------------------------------------------------------
	virtual bool Touch( DataCacheHandle_t handle ) = 0;


	//--------------------------------------------------------
	// Purpose: Explicitly mark an item as "least recently used". 
	//--------------------------------------------------------
	virtual bool Age( DataCacheHandle_t handle ) = 0;


	//--------------------------------------------------------
	// Purpose: Empty the cache. Returns bytes released, will remove locked items if force specified
	//--------------------------------------------------------
	virtual unsigned Flush( bool bUnlockedOnly = true, bool bNotify = true ) = 0;


	//--------------------------------------------------------
	// Purpose: Dump the oldest items to free the specified amount of memory. Returns amount actually freed
	//--------------------------------------------------------
	virtual unsigned Purge( unsigned nBytes ) = 0;


	//--------------------------------------------------------
	// Purpose: Output the state of the section
	//--------------------------------------------------------
	virtual void OutputReport( DataCacheReportType_t reportType = DC_SUMMARY_REPORT ) = 0;

	//--------------------------------------------------------
	// Purpose: Updates the size used by a specific item (locks the item, kicks
	//  other items out to make room as necessary, unlocks the item).
	//--------------------------------------------------------
	virtual void UpdateSize( DataCacheHandle_t handle, unsigned int nNewSize ) = 0;


	//--------------------------------------------------------
	// Purpose: Access to the mutex. More explicit control during get-then-lock sequences
	// to ensure object stays valid during "then"
	//--------------------------------------------------------
	virtual void LockMutex() = 0;
	virtual void UnlockMutex() = 0;

	//--------------------------------------------------------
	// Purpose: Add an item to the cache.  Purges old items if over budget, returns false if item was already in cache.
	//--------------------------------------------------------
	virtual bool AddEx( DataCacheClientID_t clientId, const void *pItemData, unsigned size, unsigned flags, DataCacheHandle_t *pHandle ) = 0;
};


//-----------------------------------------------------------------------------
// IDataCacheClient
//
// Purpose: Connection between the cache and the owner of a cache section
//
//-----------------------------------------------------------------------------
abstract_class IDataCacheClient
{
public:
	//--------------------------------------------------------
	// 
	//--------------------------------------------------------
	virtual bool HandleCacheNotification( const DataCacheNotification_t &notification  ) = 0;


	//--------------------------------------------------------
	// 
	//--------------------------------------------------------
	virtual bool GetItemName( DataCacheClientID_t clientId, const void *pItem, char *pDest, unsigned nMaxLen  ) = 0;
};

//-------------------------------------

class CDefaultDataCacheClient : public IDataCacheClient
{
public:
	virtual bool HandleCacheNotification( const DataCacheNotification_t &notification  )
	{
		switch ( notification.type )
		{
		case DC_AGE_DISCARD:
		case DC_FLUSH_DISCARD:
		case DC_REMOVED:
		default:
			Assert ( 0 );
			return false;
		}
		return false;
	}

	virtual bool GetItemName( DataCacheClientID_t clientId, const void *pItem, char *pDest, unsigned nMaxLen  )
	{
		return false;
	}
};


//-----------------------------------------------------------------------------
// IDataCache
//
// Purpose: The global shared cache. Manages sections and overall budgets.
//
//-----------------------------------------------------------------------------
abstract_class IDataCache : public IAppSystem
{
public:
	//--------------------------------------------------------
	// Purpose: Controls cache size.
	//--------------------------------------------------------
	virtual void SetSize( int nMaxBytes ) = 0;
	virtual void SetOptions( unsigned options ) = 0;
	virtual void SetSectionLimits( const char *pszSectionName, const DataCacheLimits_t &limits ) = 0;


	//--------------------------------------------------------
	// Purpose: Get the current state of the cache
	//--------------------------------------------------------
	virtual void GetStatus( DataCacheStatus_t *pStatus, DataCacheLimits_t *pLimits = NULL ) = 0;


	//--------------------------------------------------------
	// Purpose: Add a section to the cache
	//--------------------------------------------------------
	virtual IDataCacheSection *AddSection( IDataCacheClient *pClient, const char *pszSectionName, const DataCacheLimits_t &limits = DataCacheLimits_t(), bool bSupportFastFind = false ) = 0;


	//--------------------------------------------------------
	// Purpose: Remove a section from the cache
	//--------------------------------------------------------
	virtual void RemoveSection( const char *pszClientName, bool bCallFlush = true ) = 0;
	void RemoveSection( IDataCacheSection *pSection, bool bCallFlush = true )	{ if ( pSection) RemoveSection( pSection->GetName() ); }


	//--------------------------------------------------------
	// Purpose: Find a section of the cache
	//--------------------------------------------------------
	virtual IDataCacheSection *FindSection( const char *pszClientName ) = 0;


	//--------------------------------------------------------
	// Purpose: Dump the oldest items to free the specified amount of memory. Returns amount actually freed
	//--------------------------------------------------------
	virtual unsigned Purge( unsigned nBytes ) = 0;


	//--------------------------------------------------------
	// Purpose: Empty the cache. Returns bytes released, will remove locked items if force specified
	//--------------------------------------------------------
	virtual unsigned Flush( bool bUnlockedOnly = true, bool bNotify = true ) = 0;


	//--------------------------------------------------------
	// Purpose: Output the state of the cache
	//--------------------------------------------------------
	virtual void OutputReport( DataCacheReportType_t reportType = DC_SUMMARY_REPORT, const char *pszSection = NULL ) = 0;
};

//-----------------------------------------------------------------------------
// Helper class to support usage pattern similar to CDataManager
//-----------------------------------------------------------------------------

template< class STORAGE_TYPE, class CREATE_PARAMS, class LOCK_TYPE = STORAGE_TYPE * >
class CManagedDataCacheClient : public CDefaultDataCacheClient
{
public:
	typedef CManagedDataCacheClient<STORAGE_TYPE, CREATE_PARAMS, LOCK_TYPE> CCacheClientBaseClass;

	CManagedDataCacheClient()
		: m_pCache( NULL )
	{
	}

	void Init( IDataCache *pSharedCache, const char *pszSectionName, const DataCacheLimits_t &limits = DataCacheLimits_t(), bool bSupportFastFind = false )
	{
		if ( !m_pCache )
		{
			m_pCache = pSharedCache->AddSection( this, pszSectionName, limits, bSupportFastFind );
		}
	}

	void Shutdown()
	{
		if ( m_pCache )
		{
			m_pCache->GetSharedCache()->RemoveSection( m_pCache );
			m_pCache = NULL;
		}
	}

	LOCK_TYPE CacheGet( DataCacheHandle_t handle, bool bFrameLock = true )
	{ 
		return (LOCK_TYPE)(((STORAGE_TYPE *)m_pCache->Get( handle, bFrameLock ))->GetData()); 
	}

	LOCK_TYPE CacheGetNoTouch( DataCacheHandle_t handle )	
	{ 
		return (LOCK_TYPE)(((STORAGE_TYPE *)m_pCache->GetNoTouch( handle ))->GetData()); 
	}

	LOCK_TYPE CacheLock( DataCacheHandle_t handle )				
	{ 
		return (LOCK_TYPE)(((STORAGE_TYPE *)m_pCache->Lock( handle ))->GetData()); 
	}

	int CacheUnlock( DataCacheHandle_t handle )
	{ 
		return m_pCache->Unlock( handle ); 
	}

	void CacheTouch( DataCacheHandle_t handle )
	{ 
		m_pCache->Touch( handle ); 
	}

	void CacheRemove( DataCacheHandle_t handle, bool bNotify = true )
	{ 
		m_pCache->Remove( handle, bNotify ); 
	}

	void CacheFlush()
	{ 
		m_pCache->Flush(); 
	}

	DataCacheHandle_t CacheCreate( const CREATE_PARAMS &createParams, unsigned flags = DCAF_DEFAULT )
	{
		m_pCache->EnsureCapacity(STORAGE_TYPE::EstimatedSize(createParams));
		STORAGE_TYPE *pStore = STORAGE_TYPE::CreateResource( createParams );
		DataCacheHandle_t handle;
		m_pCache->AddEx( (DataCacheClientID_t)pStore, pStore, pStore->Size(), flags, &handle);
		return handle;
	}

	void CacheLockMutex()
	{
		m_pCache->LockMutex();
	}

	void CacheUnlockMutex()
	{
		m_pCache->UnlockMutex();
	}

	bool HandleCacheNotification( const DataCacheNotification_t &notification )
	{
		switch ( notification.type )
		{
		case DC_AGE_DISCARD:
		case DC_FLUSH_DISCARD:
		case DC_REMOVED:
			{
				STORAGE_TYPE *p = (STORAGE_TYPE *)notification.clientId;
				p->DestroyResource();
			}
			return true;
		default:
			return CDefaultDataCacheClient::HandleCacheNotification( notification );
		}
	}


protected:

	~CManagedDataCacheClient()
	{
		Shutdown();
	}

	IDataCacheSection *GetCacheSection()
	{
		return m_pCache;
	}

private:
	IDataCacheSection *m_pCache;

};

//-----------------------------------------------------------------------------

#endif // IDataCache
