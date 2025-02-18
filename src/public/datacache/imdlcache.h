//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// model loading and caching
//
//=============================================================================

#ifndef IMDLCACHE_H
#define IMDLCACHE_H

#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystem.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct studiohdr_t;
struct studiohwdata_t;
struct vcollide_t;
struct virtualmodel_t;
struct vertexFileHeader_t;

namespace OptimizedModel
{
	struct FileHeader_t;
}


//-----------------------------------------------------------------------------
// Reference to a loaded studiomdl 
//-----------------------------------------------------------------------------
typedef unsigned short MDLHandle_t;

enum
{
	MDLHANDLE_INVALID = (MDLHandle_t)~0 
};


//-----------------------------------------------------------------------------
// Cache data types
//-----------------------------------------------------------------------------
enum MDLCacheDataType_t
{
	// Callbacks to get called when data is loaded or unloaded for these:
	MDLCACHE_STUDIOHDR = 0,
	MDLCACHE_STUDIOHWDATA,
	MDLCACHE_VCOLLIDE,

	// Callbacks NOT called when data is loaded or unloaded for these:
	MDLCACHE_ANIMBLOCK,
	MDLCACHE_VIRTUALMODEL,
	MDLCACHE_VERTEXES,
	MDLCACHE_DECODEDANIMBLOCK,
};

abstract_class IMDLCacheNotify
{
public:
	// Called right after the data is loaded
	virtual void OnDataLoaded( MDLCacheDataType_t type, MDLHandle_t handle ) = 0;

	// Called right before the data is unloaded
	virtual void OnDataUnloaded( MDLCacheDataType_t type, MDLHandle_t handle ) = 0;
};


//-----------------------------------------------------------------------------
// Flags for flushing 
//-----------------------------------------------------------------------------
enum MDLCacheFlush_t
{
	MDLCACHE_FLUSH_STUDIOHDR		= 0x01,
	MDLCACHE_FLUSH_STUDIOHWDATA		= 0x02,
	MDLCACHE_FLUSH_VCOLLIDE			= 0x04,
	MDLCACHE_FLUSH_ANIMBLOCK		= 0x08,
	MDLCACHE_FLUSH_VIRTUALMODEL		= 0x10,
	MDLCACHE_FLUSH_AUTOPLAY         = 0x20,
	MDLCACHE_FLUSH_VERTEXES         = 0x40,

	MDLCACHE_FLUSH_IGNORELOCK       = 0x80000000,
	MDLCACHE_FLUSH_ALL              = 0xFFFFFFFF
};

/*
#define MDLCACHE_INTERFACE_VERSION_4 "MDLCache004"

namespace MDLCacheV4
{

abstract_class IMDLCache : public IAppSystem
{
public:
	// Used to install callbacks for when data is loaded + unloaded
	virtual void SetCacheNotify( IMDLCacheNotify *pNotify ) = 0;
	// NOTE: This assumes the "GAME" path if you don't use
	// the UNC method of specifying files. This will also increment
	// the reference count of the MDL
	virtual MDLHandle_t FindMDL( const char *pMDLRelativePath ) = 0;

	// Reference counting
	virtual int AddRef( MDLHandle_t handle ) = 0;
	virtual int Release( MDLHandle_t handle ) = 0;

	// Gets at the various data associated with a MDL
	virtual studiohdr_t *GetStudioHdr( MDLHandle_t handle ) = 0;
	virtual studiohwdata_t *GetHardwareData( MDLHandle_t handle ) = 0;
	virtual vcollide_t *GetVCollide( MDLHandle_t handle ) = 0;
	virtual unsigned char *GetAnimBlock( MDLHandle_t handle, int nBlock ) = 0;
	virtual virtualmodel_t *GetVirtualModel( MDLHandle_t handle ) = 0;
	virtual int GetAutoplayList( MDLHandle_t handle, unsigned short **pOut ) = 0;
	virtual vertexFileHeader_t *GetVertexData( MDLHandle_t handle ) = 0;

	// Brings all data associated with an MDL into memory
	virtual void TouchAllData( MDLHandle_t handle ) = 0;

	// Gets/sets user data associated with the MDL
	virtual void SetUserData( MDLHandle_t handle, void* pData ) = 0;
	virtual void *GetUserData( MDLHandle_t handle ) = 0;

	// Is this MDL using the error model?
	virtual bool IsErrorModel( MDLHandle_t handle ) = 0;

	// Flushes the cache, force a full discard
	virtual void Flush( int nFlushFlags = MDLCACHE_FLUSH_ALL ) = 0;

	// Flushes a particular model out of memory
	virtual void Flush( MDLHandle_t handle, int nFlushFlags = MDLCACHE_FLUSH_ALL ) = 0;

	// Returns the name of the model (its relative path)
	virtual const char *GetModelName( MDLHandle_t handle ) = 0;

	// faster access when you already have the studiohdr
	virtual virtualmodel_t *GetVirtualModelFast( const studiohdr_t *pStudioHdr, MDLHandle_t handle ) = 0;

	// all cache entries that subsequently allocated or successfully checked 
	// are considered "locked" and will not be freed when additional memory is needed
	virtual void BeginLock() = 0;

	// reset all protected blocks to normal
	virtual void EndLock() = 0;

	// returns a pointer to a counter that is incremented every time the cache has been out of the locked state (EVIL)
	virtual int *GetFrameUnlockCounterPtr()  = 0;

	// Finish all pending async operations
	virtual void FinishPendingLoads() = 0;	
};


}
*/

//-----------------------------------------------------------------------------
// The main MDL cacher 
//-----------------------------------------------------------------------------
#define MDLCACHE_INTERFACE_VERSION "MDLCache004"
 
abstract_class IMDLCache : public IAppSystem
{
public:
	// Used to install callbacks for when data is loaded + unloaded
	// Returns the prior notify
	virtual void SetCacheNotify( IMDLCacheNotify *pNotify ) = 0;

	// NOTE: This assumes the "GAME" path if you don't use
	// the UNC method of specifying files. This will also increment
	// the reference count of the MDL
	virtual MDLHandle_t FindMDL( const char *pMDLRelativePath ) = 0;

	// Reference counting
	virtual int AddRef( MDLHandle_t handle ) = 0;
	virtual int Release( MDLHandle_t handle ) = 0;
	virtual int GetRef( MDLHandle_t handle ) = 0;

	// Gets at the various data associated with a MDL
	virtual studiohdr_t *GetStudioHdr( MDLHandle_t handle ) = 0;
	virtual studiohwdata_t *GetHardwareData( MDLHandle_t handle ) = 0;
	virtual vcollide_t *GetVCollide( MDLHandle_t handle ) = 0;
	virtual unsigned char *GetAnimBlock( MDLHandle_t handle, int nBlock ) = 0;
	virtual virtualmodel_t *GetVirtualModel( MDLHandle_t handle ) = 0;
	virtual int GetAutoplayList( MDLHandle_t handle, unsigned short **pOut ) = 0;
	virtual vertexFileHeader_t *GetVertexData( MDLHandle_t handle ) = 0;

	// Brings all data associated with an MDL into memory
	virtual void TouchAllData( MDLHandle_t handle ) = 0;

	// Gets/sets user data associated with the MDL
	virtual void SetUserData( MDLHandle_t handle, void* pData ) = 0;
	virtual void *GetUserData( MDLHandle_t handle ) = 0;

	// Is this MDL using the error model?
	virtual bool IsErrorModel( MDLHandle_t handle ) = 0;

	// Flushes the cache, force a full discard
	virtual void Flush( MDLCacheFlush_t nFlushFlags = MDLCACHE_FLUSH_ALL ) = 0;

	// Flushes a particular model out of memory
	virtual void Flush( MDLHandle_t handle, int nFlushFlags = MDLCACHE_FLUSH_ALL ) = 0;

	// Returns the name of the model (its relative path)
	virtual const char *GetModelName( MDLHandle_t handle ) = 0;

	// faster access when you already have the studiohdr
	virtual virtualmodel_t *GetVirtualModelFast( const studiohdr_t *pStudioHdr, MDLHandle_t handle ) = 0;

	// all cache entries that subsequently allocated or successfully checked 
	// are considered "locked" and will not be freed when additional memory is needed
	virtual void BeginLock() = 0;

	// reset all protected blocks to normal
	virtual void EndLock() = 0;

	// returns a pointer to a counter that is incremented every time the cache has been out of the locked state (EVIL)
	virtual int *GetFrameUnlockCounterPtrOLD() = 0;

	// Finish all pending async operations
	virtual void FinishPendingLoads() = 0;

	virtual vcollide_t *GetVCollideEx( MDLHandle_t handle, bool synchronousLoad = true ) = 0;
	virtual bool GetVCollideSize( MDLHandle_t handle, int *pVCollideSize ) = 0;

	virtual bool GetAsyncLoad( MDLCacheDataType_t type ) = 0;
	virtual bool SetAsyncLoad( MDLCacheDataType_t type, bool bAsync ) = 0;

	virtual void BeginMapLoad() = 0;
	virtual void EndMapLoad() = 0;
	virtual void MarkAsLoaded( MDLHandle_t handle ) = 0;

	virtual void InitPreloadData( bool rebuild ) = 0;
	virtual void ShutdownPreloadData() = 0;

	virtual bool IsDataLoaded( MDLHandle_t handle, MDLCacheDataType_t type ) = 0;

	virtual int *GetFrameUnlockCounterPtr( MDLCacheDataType_t type ) = 0;

	virtual studiohdr_t *LockStudioHdr( MDLHandle_t handle ) = 0;
	virtual void UnlockStudioHdr( MDLHandle_t handle ) = 0;

	virtual bool PreloadModel( MDLHandle_t handle ) = 0;

	// Hammer uses this. If a model has an error loading in GetStudioHdr, then it is flagged
	// as an error model and any further attempts to load it will just get the error model.
	// That is, until you call this function. Then it will load the correct model.
	virtual void ResetErrorModelStatus( MDLHandle_t handle ) = 0;

	virtual void MarkFrame() = 0;
};


//-----------------------------------------------------------------------------
// Critical section helper code
//-----------------------------------------------------------------------------
class CMDLCacheCriticalSection
{
public:
	CMDLCacheCriticalSection( IMDLCache *pCache ) : m_pCache( pCache )
	{
		m_pCache->BeginLock();
	}

	~CMDLCacheCriticalSection()
	{
		m_pCache->EndLock();
	}

private:
	IMDLCache *m_pCache;
};

#define MDCACHE_FINE_GRAINED 1

#if defined(MDCACHE_FINE_GRAINED)
#define MDLCACHE_CRITICAL_SECTION_( pCache ) CMDLCacheCriticalSection cacheCriticalSection(pCache)
#define MDLCACHE_COARSE_LOCK_( pCache ) ((void)(0))
#elif defined(MDLCACHE_LEVEL_LOCKED)
#define MDLCACHE_CRITICAL_SECTION_( pCache )  ((void)(0))
#define MDLCACHE_COARSE_LOCK_( pCache ) ((void)(0))
#else
#define MDLCACHE_CRITICAL_SECTION_( pCache ) ((void)(0))
#define MDLCACHE_COARSE_LOCK_( pCache ) CMDLCacheCriticalSection cacheCriticalSection(pCache)
#endif
#define MDLCACHE_CRITICAL_SECTION() MDLCACHE_CRITICAL_SECTION_(mdlcache)
#define MDLCACHE_COARSE_LOCK() MDLCACHE_COARSE_LOCK_(mdlcache)

#endif // IMDLCACHE_H

