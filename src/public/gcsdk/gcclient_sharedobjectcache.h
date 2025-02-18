//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Additional shared object cache functionality for the GC
//
//=============================================================================

#ifndef GCCLIENT_SHAREDOBJECTCACHE_H
#define GCCLIENT_SHAREDOBJECTCACHE_H
#ifdef _WIN32
#pragma once
#endif

#include "sharedobjectcache.h"

class CMsgSOCacheSubscribed;
class CMsgSOCacheSubscribed_SubscribedType;

namespace GCSDK
{

/// Enumerate different events that might trigger a callback to an ISharedObjectListener
enum ESOCacheEvent
{

	/// Dummy sentinel value
	eSOCacheEvent_None = 0,

	/// We received a our first update from the GC and are subscribed
	eSOCacheEvent_Subscribed = 1,

	/// We lost connection to GC or GC notified us that we are no longer subscribed.
	/// Objects stay in the cache, but we no longer receive updates
	eSOCacheEvent_Unsubscribed = 2,

	/// We received a full update from the GC on a cache for which we were already subscribed.
	/// This can happen if connectivity is lost, and then restored before we realized it was lost.
	eSOCacheEvent_Resubscribed = 3,		

	/// We received an incremental update from the GC about specific object(s) being
	/// added, updated, or removed from the cache
	eSOCacheEvent_Incremental = 4,

	/// A lister was added to the cache
	/// @see CGCClientSharedObjectCache::AddListener
	eSOCacheEvent_ListenerAdded = 5,		

	/// A lister was removed from the cache
	/// @see CGCClientSharedObjectCache::RemoveListener
	eSOCacheEvent_ListenerRemoved = 6,		
};

//----------------------------------------------------------------------------
// Purpose: Allow game components to register themselves to hear about inventory
//			changes when they are received from the server
//----------------------------------------------------------------------------
class ISharedObjectListener
{
public:

	/// Called when a new object is created in a cache we are currently subscribed to, or when we are added
	/// as a listener to a cache which already has objects in it
	///
	/// eEvent will be one of:
	/// - eSOCacheEvent_Subscribed
	/// - eSOCacheEvent_Resubscribed
	/// - eSOCacheEvent_Incremental
	/// - eSOCacheEvent_ListenerAdded
	virtual void SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) = 0;

	/// Called when we are about to update objects in our cache but haven't done any of the work yet.
	///
	/// eEvent will be one of:
	/// - eSOCacheEvent_Resubscribed
	/// - eSOCacheEvent_Incremental
	virtual void PreSOUpdate( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) = 0;

	/// Called when an object is updated in a cache we are currently subscribed to.
	///
	/// eEvent will be one of:
	/// - eSOCacheEvent_Resubscribed
	/// - eSOCacheEvent_Incremental
	virtual void SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) = 0;

	/// Called when we're finished updating objects in our cache for this batch.
	///
	/// eEvent will be one of:
	/// - eSOCacheEvent_Resubscribed
	/// - eSOCacheEvent_Incremental
	virtual void PostSOUpdate( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) = 0;

	/// Called when an object is about to be deleted in a cache we are currently subscribed to.
	/// The object will have already been removed from the cache, but is still valid.
	///
	/// eEvent will be one of:
	/// - eSOCacheEvent_Incremental
	/// - eSOCacheEvent_Resubscribed
	virtual void SODestroyed( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) = 0;

	/// Called to notify a listener that he is subscribed to the cache.
	///
	/// eEvent will be one of:
	/// - eSOCacheEvent_Subscribed
	/// - eSOCacheEvent_Resubscribed
	/// - eSOCacheEvent_ListenerAdded
	///
	/// A listener is guaranteed that it will not receive incremental updates (SOCreated,
	/// SOUpdated, SODestroyed) while not subscribed.  (Before the SOCacheSubscribed or
	/// after SOCacheUnsubscribed.)  However, note that it may be possible to receive
	/// an SOCacheSubscribed message while already subscribed.  This can happen if the
	/// GC loses and restores connection, or otherwise decides that a full update is
	/// necessary.
	virtual void SOCacheSubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) = 0;

	/// Called to notify a listener that he is no longer subscribed to the cache.
	/// if he is being removed as a listener, then he will no longer receive
	/// updates.  Otherwise, he might receive another SOCacheSubscribed
	/// message, followed by further update notifications.
	///
	/// eEvent will be one of:
	/// - eSOCacheEvent_Unsubscribed
	/// - eSOCacheEvent_ListenerRemoved
	virtual void SOCacheUnsubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) = 0;
};


//----------------------------------------------------------------------------
// Purpose: The part of a shared object cache that handles all objects of a 
//			single type.
//----------------------------------------------------------------------------
class CGCClientSharedObjectContext
{
public:
	CGCClientSharedObjectContext( const CSteamID & steamIDOwner );

	bool BAddListener( ISharedObjectListener *pListener );
	bool BRemoveListener( ISharedObjectListener *pListener );

	void SOCreated( const CSharedObject *pObject, ESOCacheEvent eEvent ) const;
	void PreSOUpdate( GCSDK::ESOCacheEvent eEvent ) const;
	void SOUpdated( const CSharedObject *pObject, ESOCacheEvent eEvent ) const;
	void PostSOUpdate( GCSDK::ESOCacheEvent eEvent ) const;
	void SODestroyed( const CSharedObject *pObject, ESOCacheEvent eEvent ) const;
	void SOCacheSubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) const;
	void SOCacheUnsubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) const;

	const CSteamID & GetOwner() const { return m_steamIDOwner; }
	const CUtlVector< ISharedObjectListener * > & GetListeners() const { return m_vecListeners; }
private:
	CSteamID m_steamIDOwner;
	CUtlVector<ISharedObjectListener *> m_vecListeners;
};


//----------------------------------------------------------------------------
// Purpose: The part of a shared object cache that handles all objects of a 
//			single type.
//----------------------------------------------------------------------------
class CGCClientSharedObjectTypeCache : public CSharedObjectTypeCache
{
public:
	CGCClientSharedObjectTypeCache( int nTypeID, const CGCClientSharedObjectContext & context );
	virtual ~CGCClientSharedObjectTypeCache();

	bool BParseCacheSubscribedMsg( const CMsgSOCacheSubscribed_SubscribedType & msg, CUtlVector<CSharedObject*> &vecCreatedObjects, CUtlVector<CSharedObject*> &vecUpdatedObjects, CUtlVector<CSharedObject*> &vecObjectsToDestroy );

	CSharedObject *BCreateFromMsg( const void *pvData, uint32 unSize, bool *bUpdatedExisting );
	bool BDestroyFromMsg( const void *pvData, uint32 unSize );
	bool BUpdateFromMsg( const void *pvData, uint32 unSize );

	void RemoveAllObjects( CUtlVector<CSharedObject*> &vecObjects );

private:
	const CGCClientSharedObjectContext & m_context;
};


//----------------------------------------------------------------------------
// Purpose: A cache of a bunch of shared objects of different types. This class
//			is shared between clients, gameservers, and the GC and is 
//			responsible for sending messages from the GC to cause object 
//			creation/destruction/updating on the clients/gameservers.
//----------------------------------------------------------------------------
class CGCClientSharedObjectCache : public CSharedObjectCache
{
	friend class CGCSOUpdateMultipleJob;

public:
	CGCClientSharedObjectCache( const CSteamID & steamIDOwner = CSteamID() );
	virtual ~CGCClientSharedObjectCache();

	/// Have we received at least one update from the GC?
	bool BIsInitialized() const { return m_bInitialized; }

	/// Are we currently subscribed to updates from the GC?
	bool BIsSubscribed() const { return m_bSubscribed; }

	/// Who owns this cache?
	virtual const CSteamID & GetOwner() const { return m_context.GetOwner(); }

	/// Adds a listener interface to the object.  A call to re-add
	/// a listener that's already listening is harmlessly ignored.
	///
	/// If the cache is currently subscribed, then the listener will
	/// immediately receive a SOCacheSubscribed call.
	void AddListener( ISharedObjectListener *pListener );

	/// Removes a listener interface on the object.  Returns true
	/// if we were listening and removed OK, false if we were not
	/// previously listening and the call was ignored
	///
	/// If the cache is currently subscribed and we were listening,
	/// then the listener will immediately receive a SOCacheUnsubscribed
	/// call.
	bool RemoveListener( ISharedObjectListener *pListener );

	CGCClientSharedObjectTypeCache *FindTypeCache( int nClassID ) { return (CGCClientSharedObjectTypeCache *)FindBaseTypeCache( nClassID ); }
	CGCClientSharedObjectTypeCache *CreateTypeCache( int nClassID ) { return (CGCClientSharedObjectTypeCache *)CreateBaseTypeCache( nClassID ); }

	bool BParseCacheSubscribedMsg( const CMsgSOCacheSubscribed & msg, bool bLocal = false );
	void NotifyUnsubscribe();
	void NotifyResubscribedUpToDate();

	bool BCreateFromMsg( int nTypeID, const void *pvData, uint32 unSize );
	bool BDestroyFromMsg( int nTypeID, const void *pvData, uint32 unSize );
	bool BUpdateFromMsg( int nTypeID, const void *pvData, uint32 unSize );

#ifdef DBGFLAG_VALIDATE
	virtual void Validate( CValidator &validator, const char *pchName );
#endif

	bool BIsLocal() const { return m_bLocal; }

protected:
private:
	virtual CSharedObjectTypeCache *AllocateTypeCache( int nClassID ) const OVERRIDE { return new CGCClientSharedObjectTypeCache( nClassID, m_context ); }
	CGCClientSharedObjectTypeCache *GetTypeCacheByIndex( int nIndex ) { return (CGCClientSharedObjectTypeCache *)CSharedObjectCache::GetTypeCacheByIndex( nIndex ); }

	CGCClientSharedObjectContext m_context;
	bool m_bInitialized;
	bool m_bSubscribed;
	bool m_bLocal; // Not subscribed via the GC
};



} // namespace GCSDK


#endif //GCCLIENT_SHAREDOBJECTCACHE_H
