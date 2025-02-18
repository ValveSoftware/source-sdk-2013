//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "engine/IEngineSound.h"
#include "mempool.h"
#include "movevars_shared.h"
#include "utlrbtree.h"
#include "tier0/vprof.h"
#include "entitydatainstantiator.h"
#include "positionwatcher.h"
#include "movetype_push.h"
#include "vphysicsupdateai.h"
#include "igamesystem.h"
#include "utlmultilist.h"
#include "tier1/callqueue.h"

#ifdef PORTAL
	#include "portal_util_shared.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// memory pool for storing links between entities
static CUtlMemoryPool g_EdictTouchLinks( sizeof(touchlink_t), MAX_EDICTS, CUtlMemoryPool::GROW_NONE, "g_EdictTouchLinks");
static CUtlMemoryPool g_EntityGroundLinks( sizeof( groundlink_t ), MAX_EDICTS, CUtlMemoryPool::GROW_NONE, "g_EntityGroundLinks");

struct watcher_t
{
	EHANDLE				hWatcher;
	IWatcherCallback	*pWatcherCallback;
};

static CUtlMultiList<watcher_t, unsigned short>	g_WatcherList;
class CWatcherList
{
public:
	//CWatcherList(); NOTE: Dataobj doesn't support constructors - it zeros the memory
	~CWatcherList();	// frees the positionwatcher_t's to the pool
	void Init();

	void NotifyPositionChanged( CBaseEntity *pEntity );
	void NotifyVPhysicsStateChanged( IPhysicsObject *pPhysics, CBaseEntity *pEntity, bool bAwake );

	void AddToList( CBaseEntity *pWatcher );
	void RemoveWatcher( CBaseEntity *pWatcher );

private:
	int GetCallbackObjects( IWatcherCallback **pList, int listMax );

	unsigned short Find( CBaseEntity *pEntity );
	unsigned short m_list;
};

int linksallocated = 0;
int groundlinksallocated = 0;

// Prints warnings if any entity think functions take longer than this many milliseconds
#ifdef _DEBUG
#define DEF_THINK_LIMIT "20"
#else
#define DEF_THINK_LIMIT "10"
#endif

ConVar think_limit( "think_limit", DEF_THINK_LIMIT, FCVAR_REPLICATED, "Maximum think time in milliseconds, warning is printed if this is exceeded." );
#ifndef CLIENT_DLL
ConVar debug_touchlinks( "debug_touchlinks", "0", 0, "Spew touch link activity" );
#define DebugTouchlinks() debug_touchlinks.GetBool()
#else
#define DebugTouchlinks() false
#endif



//-----------------------------------------------------------------------------
// Portal-specific hack designed to eliminate re-entrancy in touch functions
//-----------------------------------------------------------------------------
class CPortalTouchScope
{
public:
	CPortalTouchScope();
	~CPortalTouchScope();

public:
	static int m_nDepth;
	static CCallQueue m_CallQueue;	
};

int CPortalTouchScope::m_nDepth = 0;
CCallQueue CPortalTouchScope::m_CallQueue;	

CCallQueue *GetPortalCallQueue()
{
	return ( CPortalTouchScope::m_nDepth > 0 ) ? &CPortalTouchScope::m_CallQueue : NULL;
}

CPortalTouchScope::CPortalTouchScope()
{
	++m_nDepth;
}

CPortalTouchScope::~CPortalTouchScope()
{
	Assert( m_nDepth >= 1 );
	if ( --m_nDepth == 0 )
	{
		m_CallQueue.CallQueued();
	}
}


//-----------------------------------------------------------------------------
// Purpose: System for hanging objects off of CBaseEntity, etc.
//  Externalized data objects ( see sharreddefs.h for enum )
//-----------------------------------------------------------------------------
class CDataObjectAccessSystem : public CAutoGameSystem
{
public:

	enum
	{
		MAX_ACCESSORS = 32,
	};

	CDataObjectAccessSystem()
	{
		// Cast to int to make it clear that we know we are comparing different enum types.
		COMPILE_TIME_ASSERT( (int)NUM_DATAOBJECT_TYPES <= (int)MAX_ACCESSORS );

		Q_memset( m_Accessors, 0, sizeof( m_Accessors ) );
	}

	virtual bool Init()
	{
		AddDataAccessor( TOUCHLINK, new CEntityDataInstantiator< touchlink_t > );
		AddDataAccessor( GROUNDLINK, new CEntityDataInstantiator< groundlink_t > );
		AddDataAccessor( STEPSIMULATION, new CEntityDataInstantiator< StepSimulationData > );
		AddDataAccessor( MODELSCALE, new CEntityDataInstantiator< ModelScale > );
		AddDataAccessor( POSITIONWATCHER, new CEntityDataInstantiator< CWatcherList > );
		AddDataAccessor( PHYSICSPUSHLIST, new CEntityDataInstantiator< physicspushlist_t > );
		AddDataAccessor( VPHYSICSUPDATEAI, new CEntityDataInstantiator< vphysicsupdateai_t > );
		AddDataAccessor( VPHYSICSWATCHER, new CEntityDataInstantiator< CWatcherList > );
		
		return true;
	}

	virtual void Shutdown()
	{
		for ( int i = 0; i < MAX_ACCESSORS; i++ )
		{
			delete m_Accessors[ i ];
			m_Accessors[ i ]  = 0;
		}
	}

	void *GetDataObject( int type, const CBaseEntity *instance )
	{
		if ( !IsValidType( type ) )
		{
			Assert( !"Bogus type" );
			return NULL;
		}
		return m_Accessors[ type ]->GetDataObject( instance );
	}

	void *CreateDataObject( int type, CBaseEntity *instance )
	{
		if ( !IsValidType( type ) )
		{
			Assert( !"Bogus type" );
			return NULL;
		}

		return m_Accessors[ type ]->CreateDataObject( instance );
	}

	void DestroyDataObject( int type, CBaseEntity *instance )
	{
		if ( !IsValidType( type ) )
		{
			Assert( !"Bogus type" );
			return;
		}

		m_Accessors[ type ]->DestroyDataObject( instance );
	}

private:

	bool IsValidType( int type ) const
	{
		if ( type < 0 || type >= MAX_ACCESSORS )
			return false;

		if ( m_Accessors[ type ] == NULL )
			return false;
		return true;
	}

	void AddDataAccessor( int type, IEntityDataInstantiator *instantiator )
	{
		if ( type < 0 || type >= MAX_ACCESSORS )
		{
			Assert( !"AddDataAccessor with out of range type!!!\n" );
			return;
		}

		Assert( instantiator );

		if ( m_Accessors[ type ] != NULL )
		{
			Assert( !"AddDataAccessor, duplicate adds!!!\n" );
			return;
		}

		m_Accessors[ type ] = instantiator;
	}

	IEntityDataInstantiator *m_Accessors[ MAX_ACCESSORS ];
};

static CDataObjectAccessSystem g_DataObjectAccessSystem;

bool CBaseEntity::HasDataObjectType( int type ) const
{
	Assert( type >= 0 && type < NUM_DATAOBJECT_TYPES );
	return ( m_fDataObjectTypes	& (1<<type) ) ? true : false;
}

void CBaseEntity::AddDataObjectType( int type )
{
	Assert( type >= 0 && type < NUM_DATAOBJECT_TYPES );
	m_fDataObjectTypes |= (1<<type);
}

void CBaseEntity::RemoveDataObjectType( int type )
{
	Assert( type >= 0 && type < NUM_DATAOBJECT_TYPES );
	m_fDataObjectTypes &= ~(1<<type);
}

void *CBaseEntity::GetDataObject( int type )
{
	Assert( type >= 0 && type < NUM_DATAOBJECT_TYPES );
	if ( !HasDataObjectType( type ) )
		return NULL;
	return g_DataObjectAccessSystem.GetDataObject( type, this );
}

void *CBaseEntity::CreateDataObject( int type )
{
	Assert( type >= 0 && type < NUM_DATAOBJECT_TYPES );
	AddDataObjectType( type );
	return g_DataObjectAccessSystem.CreateDataObject( type, this );
}

void CBaseEntity::DestroyDataObject( int type )
{
	Assert( type >= 0 && type < NUM_DATAOBJECT_TYPES );
	if ( !HasDataObjectType( type ) )
		return;
	g_DataObjectAccessSystem.DestroyDataObject( type, this );
	RemoveDataObjectType( type );
}

void CWatcherList::Init()
{
	m_list = g_WatcherList.CreateList();
}

CWatcherList::~CWatcherList()
{
	g_WatcherList.DestroyList( m_list );
}

int CWatcherList::GetCallbackObjects( IWatcherCallback **pList, int listMax )
{
	int index = 0;
	unsigned short next = g_WatcherList.InvalidIndex();
	for ( unsigned short node = g_WatcherList.Head( m_list ); node != g_WatcherList.InvalidIndex(); node = next )
	{
		next = g_WatcherList.Next( node );
		watcher_t *pNode = &g_WatcherList.Element(node);
		if ( pNode->hWatcher.Get() )
		{
			pList[index] = pNode->pWatcherCallback;
			index++;
			if ( index >= listMax )
			{
				Assert(0);
				return index;
			}
		}
		else
		{
			g_WatcherList.Remove( m_list, node );
		}
	}
	return index;
}

void CWatcherList::NotifyPositionChanged( CBaseEntity *pEntity )
{
	IWatcherCallback *pCallbacks[1024]; // HACKHACK: Assumes this list is big enough
	int count = GetCallbackObjects( pCallbacks, ARRAYSIZE(pCallbacks) );
	for ( int i = 0; i < count; i++ )
	{
		IPositionWatcher *pWatcher = assert_cast<IPositionWatcher *>(pCallbacks[i]);
		if ( pWatcher )
		{
			pWatcher->NotifyPositionChanged(pEntity);
		}
	}
}

void CWatcherList::NotifyVPhysicsStateChanged( IPhysicsObject *pPhysics, CBaseEntity *pEntity, bool bAwake )
{
	IWatcherCallback *pCallbacks[1024];	// HACKHACK: Assumes this list is big enough!
	int count = GetCallbackObjects( pCallbacks, ARRAYSIZE(pCallbacks) );
	for ( int i = 0; i < count; i++ )
	{
		IVPhysicsWatcher *pWatcher = assert_cast<IVPhysicsWatcher *>(pCallbacks[i]);
		if ( pWatcher )
		{
			pWatcher->NotifyVPhysicsStateChanged(pPhysics, pEntity, bAwake);
		}
	}
}

unsigned short CWatcherList::Find( CBaseEntity *pEntity )
{
	unsigned short next = g_WatcherList.InvalidIndex();
	for ( unsigned short node = g_WatcherList.Head( m_list ); node != g_WatcherList.InvalidIndex(); node = next )
	{
		next = g_WatcherList.Next( node );
		watcher_t *pNode = &g_WatcherList.Element(node);
		if ( pNode->hWatcher.Get() == pEntity )
		{
			return node;
		}
	}
	return g_WatcherList.InvalidIndex();
}

void CWatcherList::RemoveWatcher( CBaseEntity *pEntity )
{
	unsigned short node = Find( pEntity );
	if ( node != g_WatcherList.InvalidIndex() )
	{
		g_WatcherList.Remove( m_list, node );
	}
}


void CWatcherList::AddToList( CBaseEntity *pWatcher )
{
	unsigned short node = Find( pWatcher );
	if ( node == g_WatcherList.InvalidIndex() )
	{
		watcher_t watcher;
		watcher.hWatcher = pWatcher;
			// save this separately so we can use the EHANDLE to test for deletion
		watcher.pWatcherCallback = dynamic_cast<IWatcherCallback *> (pWatcher);

		if ( watcher.pWatcherCallback )
		{
			g_WatcherList.AddToTail( m_list, watcher );
		}
	}
}

static void AddWatcherToEntity( CBaseEntity *pWatcher, CBaseEntity *pEntity, int watcherType )
{
	CWatcherList *pList = (CWatcherList *)pEntity->GetDataObject(watcherType);
	if ( !pList )
	{
		pList = ( CWatcherList * )pEntity->CreateDataObject( watcherType );
		pList->Init();
	}

	pList->AddToList( pWatcher );
}

static void RemoveWatcherFromEntity( CBaseEntity *pWatcher, CBaseEntity *pEntity, int watcherType )
{
	CWatcherList *pList = (CWatcherList *)pEntity->GetDataObject(watcherType);
	if ( pList )
	{
		pList->RemoveWatcher( pWatcher );
	}
}

void WatchPositionChanges( CBaseEntity *pWatcher, CBaseEntity *pMovingEntity )
{
	AddWatcherToEntity( pWatcher, pMovingEntity, POSITIONWATCHER );
}

void RemovePositionWatcher( CBaseEntity *pWatcher, CBaseEntity *pMovingEntity )
{
	RemoveWatcherFromEntity( pWatcher, pMovingEntity, POSITIONWATCHER );
}

void ReportPositionChanged( CBaseEntity *pMovedEntity )
{
	CWatcherList *pList = (CWatcherList *)pMovedEntity->GetDataObject(POSITIONWATCHER);
	if ( pList )
	{
		pList->NotifyPositionChanged( pMovedEntity );
	}
}

void WatchVPhysicsStateChanges( CBaseEntity *pWatcher, CBaseEntity *pPhysicsEntity )
{
	AddWatcherToEntity( pWatcher, pPhysicsEntity, VPHYSICSWATCHER );
}

void RemoveVPhysicsStateWatcher( CBaseEntity *pWatcher, CBaseEntity *pPhysicsEntity )
{
	AddWatcherToEntity( pWatcher, pPhysicsEntity, VPHYSICSWATCHER );
}

void ReportVPhysicsStateChanged( IPhysicsObject *pPhysics, CBaseEntity *pEntity, bool bAwake )
{
	CWatcherList *pList = (CWatcherList *)pEntity->GetDataObject(VPHYSICSWATCHER);
	if ( pList )
	{
		pList->NotifyVPhysicsStateChanged( pPhysics, pEntity, bAwake );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseEntity::DestroyAllDataObjects( void )
{
	int i;
	for ( i = 0; i < NUM_DATAOBJECT_TYPES; i++ )
	{
		if ( HasDataObjectType( i ) )
		{
			DestroyDataObject( i );
		}
	}
}

//-----------------------------------------------------------------------------
// For debugging
//-----------------------------------------------------------------------------

#ifdef GAME_DLL

void SpewLinks()
{
	int nCount = 0;
	for ( CBaseEntity *pClass = gEntList.FirstEnt(); pClass != NULL; pClass = gEntList.NextEnt(pClass) )
	{
		if ( pClass /*&& !pClass->IsDormant()*/ )
		{
			touchlink_t *root = ( touchlink_t * )pClass->GetDataObject( TOUCHLINK );
			if ( root )
			{

				// check if the edict is already in the list
				for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
				{
					++nCount;
					Msg("[%d] (%d) Link %d (%s) -> %d (%s)\n", nCount, pClass->IsDormant(),
						pClass->entindex(), pClass->GetClassname(),
						link->entityTouched->entindex(), link->entityTouched->GetClassname() );
				}
			}
		}
	}
}

#endif

//-----------------------------------------------------------------------------
// Returns the actual gravity
//-----------------------------------------------------------------------------
static inline float GetActualGravity( CBaseEntity *pEnt )
{
	float ent_gravity = pEnt->GetGravity();
	if ( ent_gravity == 0.0f )
	{
		ent_gravity = 1.0f;
	}

	return ent_gravity * GetCurrentGravity();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : inline touchlink_t
//-----------------------------------------------------------------------------
inline touchlink_t *AllocTouchLink( void )
{
	touchlink_t *link = (touchlink_t*)g_EdictTouchLinks.Alloc( sizeof(touchlink_t) );
	if ( link )
	{
		++linksallocated;
	}
	else
	{
		DevWarning( "AllocTouchLink: failed to allocate touchlink_t.\n" );
	}

	return link;
}

static touchlink_t *g_pNextLink = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *link - 
// Output : inline void
//-----------------------------------------------------------------------------
inline void FreeTouchLink( touchlink_t *link )
{
	if ( link )
	{
		if ( link == g_pNextLink )
		{
			g_pNextLink = link->nextLink;
		}
		--linksallocated;
		link->prevLink = link->nextLink = NULL;
	}

	// Necessary to catch crashes
	g_EdictTouchLinks.Free( link );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : inline groundlink_t
//-----------------------------------------------------------------------------
inline groundlink_t *AllocGroundLink( void )
{
	groundlink_t *link = (groundlink_t*)g_EntityGroundLinks.Alloc( sizeof(groundlink_t) );
	if ( link )
	{
		++groundlinksallocated;
	}
	else
	{
		DevMsg( "AllocGroundLink: failed to allocate groundlink_t.!!!  groundlinksallocated=%d g_EntityGroundLinks.Count()=%d\n", groundlinksallocated, g_EntityGroundLinks.Count() );
	}


	return link;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *link - 
// Output : inline void
//-----------------------------------------------------------------------------
inline void FreeGroundLink( groundlink_t *link )
{

	if ( link )
	{
		--groundlinksallocated;
	}

	g_EntityGroundLinks.Free( link );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::IsCurrentlyTouching( void ) const
{
	if ( HasDataObjectType( TOUCHLINK ) )
	{
		return true;
	}

	return false;
}

static bool g_bCleanupDatObject = true;

//-----------------------------------------------------------------------------
// Purpose: Checks to see if any entities that have been touching this one
//			have stopped touching it, and notify the entity if so.
//			Called at the end of a frame, after all the entities have run
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsCheckForEntityUntouch( void )
{
	Assert( g_pNextLink == NULL );

	touchlink_t *link;

	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
#ifdef PORTAL
		CPortalTouchScope scope;
#endif
		bool saveCleanup = g_bCleanupDatObject;
		g_bCleanupDatObject = false;

		link = root->nextLink;
		while ( link != root )
		{
			g_pNextLink = link->nextLink;

			// these touchlinks are not polled.  The ents are touching due to an outside
			// system that will add/delete them as necessary (vphysics in this case)
			if ( link->touchStamp == TOUCHSTAMP_EVENT_DRIVEN )
			{
				// refresh the touch call
				PhysicsTouch( link->entityTouched );
			}
			else
			{    
				// check to see if the touch stamp is up to date
				if ( link->touchStamp != touchStamp )
				{
					// stamp is out of data, so entities are no longer touching
					// remove self from other entities touch list
					PhysicsNotifyOtherOfUntouch( this, link->entityTouched );

					// remove other entity from this list
					PhysicsRemoveToucher( this, link );
				}
			}

			link = g_pNextLink;
		}

		g_bCleanupDatObject = saveCleanup;

		// Nothing left in list, destroy root
		if ( root->nextLink == root &&
			 root->prevLink == root )
		{
			DestroyDataObject( TOUCHLINK );
		}
	}

	g_pNextLink = NULL;

	SetCheckUntouch( false );
}

//-----------------------------------------------------------------------------
// Purpose: notifies an entity than another touching entity has moved out of contact.
// Input  : *other - the entity to be acted upon
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsNotifyOtherOfUntouch( CBaseEntity *ent, CBaseEntity *other )
{
	if ( !other )
		return;

	// loop through ed's touch list, looking for the notifier
	// remove and call untouch if found
	touchlink_t *root = ( touchlink_t * )other->GetDataObject( TOUCHLINK );
	if ( root )
	{
		touchlink_t *link = root->nextLink;
		while ( link != root )
		{
			if ( link->entityTouched == ent )
			{
				PhysicsRemoveToucher( other, link );

				// Check for complete removal
				if ( g_bCleanupDatObject &&
					 root->nextLink == root && 
					 root->prevLink == root )
				{
					other->DestroyDataObject( TOUCHLINK );
				}
				return;
			}

			link = link->nextLink;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: removes a toucher from the list
// Input  : *link - the link to remove
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsRemoveToucher( CBaseEntity *otherEntity, touchlink_t *link )
{
	// Every start Touch gets a corresponding end touch
	if ( (link->flags & FTOUCHLINK_START_TOUCH) && 
		link->entityTouched != NULL &&
		otherEntity != NULL )
	{
		otherEntity->EndTouch( link->entityTouched );
	}

	link->nextLink->prevLink = link->prevLink;
	link->prevLink->nextLink = link->nextLink;

	if ( DebugTouchlinks() )
		Msg( "remove 0x%p: %s-%s (%d-%d) [%d in play, %d max]\n", link, link->entityTouched->GetDebugName(), otherEntity->GetDebugName(), link->entityTouched->entindex(), otherEntity->entindex(), linksallocated, g_EdictTouchLinks.PeakCount() );
	FreeTouchLink( link );
}

//-----------------------------------------------------------------------------
// Purpose: Clears all touches from the list
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsRemoveTouchedList( CBaseEntity *ent )
{
#ifdef PORTAL
	CPortalTouchScope scope;
#endif

	touchlink_t *link, *nextLink;

	touchlink_t *root = ( touchlink_t * )ent->GetDataObject( TOUCHLINK );
	if ( root )
	{
		link = root->nextLink;
		bool saveCleanup = g_bCleanupDatObject;
		g_bCleanupDatObject = false;
		while ( link && link != root )
		{
			nextLink = link->nextLink;

			// notify the other entity that this ent has gone away
			PhysicsNotifyOtherOfUntouch( ent, link->entityTouched );

			// kill it
			if ( DebugTouchlinks() )
				Msg( "remove 0x%p: %s-%s (%d-%d) [%d in play, %d max]\n", link, ent->GetDebugName(), link->entityTouched->GetDebugName(), ent->entindex(), link->entityTouched->entindex(), linksallocated, g_EdictTouchLinks.PeakCount() );
			FreeTouchLink( link );
			link = nextLink;
		}

		g_bCleanupDatObject = saveCleanup;
		ent->DestroyDataObject( TOUCHLINK );
	}

	ent->touchStamp = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *other - 
// Output : groundlink_t
//-----------------------------------------------------------------------------
groundlink_t *CBaseEntity::AddEntityToGroundList( CBaseEntity *other )
{
	groundlink_t *link;

	if ( this == other )
		return NULL;

	// check if the edict is already in the list
	groundlink_t *root = ( groundlink_t * )GetDataObject( GROUNDLINK );
	if ( root )
	{
		for ( link = root->nextLink; link != root; link = link->nextLink )
		{
			if ( link->entity == other )
			{
				// no more to do
				return link;
			}
		}
	}
	else
	{
		root = ( groundlink_t * )CreateDataObject( GROUNDLINK );
		root->prevLink = root->nextLink = root;
	}

	// entity is not in list, so it's a new touch
	// add it to the touched list and then call the touch function

	// build new link
	link = AllocGroundLink();
	if ( !link )
		return NULL;

	link->entity = other;
	// add it to the list
	link->nextLink = root->nextLink;
	link->prevLink = root;
	link->prevLink->nextLink = link;
	link->nextLink->prevLink = link;

	PhysicsStartGroundContact( other );

	return link;
}

//-----------------------------------------------------------------------------
// Purpose: Called whenever two entities come in contact
// Input  : *pentOther - the entity who it has touched
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsStartGroundContact( CBaseEntity *pentOther )
{
	if ( !pentOther )
		return;

	if ( !(IsMarkedForDeletion() || pentOther->IsMarkedForDeletion()) )
	{
		pentOther->StartGroundContact( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: notifies an entity than another touching entity has moved out of contact.
// Input  : *other - the entity to be acted upon
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsNotifyOtherOfGroundRemoval( CBaseEntity *ent, CBaseEntity *other )
{
	if ( !other )
		return;

	// loop through ed's touch list, looking for the notifier
	// remove and call untouch if found
	groundlink_t *root = ( groundlink_t * )other->GetDataObject( GROUNDLINK );
	if ( root )
	{
		groundlink_t *link = root->nextLink;
		while ( link != root )
		{
			if ( link->entity == ent )
			{
				PhysicsRemoveGround( other, link );

				if ( root->nextLink == root && 
					 root->prevLink == root )
				{
					other->DestroyDataObject( GROUNDLINK );
				}
				return;
			}

			link = link->nextLink;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: removes a toucher from the list
// Input  : *link - the link to remove
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsRemoveGround( CBaseEntity *other, groundlink_t *link )
{
	// Every start Touch gets a corresponding end touch
	if ( link->entity != NULL )
	{
		CBaseEntity *linkEntity = link->entity;
		CBaseEntity *otherEntity = other;
		if ( linkEntity && otherEntity )
		{
			linkEntity->EndGroundContact( otherEntity );
		}
	}

	link->nextLink->prevLink = link->prevLink;
	link->prevLink->nextLink = link->nextLink;
	FreeGroundLink( link );
}

//-----------------------------------------------------------------------------
// Purpose: static method to remove ground list for an entity
// Input  : *ent - 
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsRemoveGroundList( CBaseEntity *ent )
{
	groundlink_t *link, *nextLink;

	groundlink_t *root = ( groundlink_t * )ent->GetDataObject( GROUNDLINK );
	if ( root )
	{
		link = root->nextLink;
		while ( link && link != root )
		{
			nextLink = link->nextLink;

			// notify the other entity that this ent has gone away
			PhysicsNotifyOtherOfGroundRemoval( ent, link->entity );

			// kill it
			FreeGroundLink( link );

			link = nextLink;
		}

		ent->DestroyDataObject( GROUNDLINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame that two entities are touching
// Input  : *pentOther - the entity who it has touched
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsTouch( CBaseEntity *pentOther )
{
	if ( pentOther )
	{
		if ( !(IsMarkedForDeletion() || pentOther->IsMarkedForDeletion()) )
		{
			Touch( pentOther );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called whenever two entities come in contact
// Input  : *pentOther - the entity who it has touched
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsStartTouch( CBaseEntity *pentOther )
{
	if ( pentOther )
	{
		if ( !(IsMarkedForDeletion() || pentOther->IsMarkedForDeletion()) )
		{
			StartTouch( pentOther );
			Touch( pentOther );
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: Marks in an entity that it is touching another entity, and calls
//			it's Touch() function if it is a new touch.
//			Stamps the touch link with the new time so that when we check for
//			untouch we know things haven't changed.
// Input  : *other - entity that it is in contact with
//-----------------------------------------------------------------------------
touchlink_t *CBaseEntity::PhysicsMarkEntityAsTouched( CBaseEntity *other )
{
	touchlink_t *link;

	if ( this == other )
		return NULL;

	// Entities in hierarchy should not interact
	if ( (this->GetMoveParent() == other) || (this == other->GetMoveParent()) )
		return NULL;

	// check if either entity doesn't generate touch functions
	if ( (GetFlags() | other->GetFlags()) & FL_DONTTOUCH )
		return NULL;

	// Pure triggers should not touch each other
	if ( IsSolidFlagSet( FSOLID_TRIGGER ) && other->IsSolidFlagSet( FSOLID_TRIGGER ) )
	{
		if (!IsSolid() && !other->IsSolid())
			return NULL;
	}

	// Don't do touching if marked for deletion
	if ( other->IsMarkedForDeletion() )
	{
		return NULL;
	}

	if ( IsMarkedForDeletion() )
	{
		return NULL;
	}

#ifdef PORTAL
	CPortalTouchScope scope;
#endif

	// check if the edict is already in the list
	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( link = root->nextLink; link != root; link = link->nextLink )
		{
			if ( link->entityTouched == other )
			{
				// update stamp
				link->touchStamp = touchStamp;
				
				if ( !CBaseEntity::sm_bDisableTouchFuncs )
				{
					PhysicsTouch( other );
				}

				// no more to do
				return link;
			}
		}
	}
	else
	{
		// Allocate the root object
		root = ( touchlink_t * )CreateDataObject( TOUCHLINK );
		root->nextLink = root->prevLink = root;
	}

	// entity is not in list, so it's a new touch
	// add it to the touched list and then call the touch function

	// build new link
	link = AllocTouchLink();
	if ( DebugTouchlinks() )
		Msg( "add 0x%p: %s-%s (%d-%d) [%d in play, %d max]\n", link, GetDebugName(), other->GetDebugName(), entindex(), other->entindex(), linksallocated, g_EdictTouchLinks.PeakCount() );
	if ( !link )
		return NULL;

	link->touchStamp = touchStamp;
	link->entityTouched = other;
	link->flags = 0;
	// add it to the list
	link->nextLink = root->nextLink;
	link->prevLink = root;
	link->prevLink->nextLink = link;
	link->nextLink->prevLink = link;

	// non-solid entities don't get touched
	bool bShouldTouch = (IsSolid() && !IsSolidFlagSet(FSOLID_VOLUME_CONTENTS)) || IsSolidFlagSet(FSOLID_TRIGGER);
	if ( bShouldTouch && !other->IsSolidFlagSet(FSOLID_TRIGGER) )
	{
		link->flags |= FTOUCHLINK_START_TOUCH;
		if ( !CBaseEntity::sm_bDisableTouchFuncs )
		{
			PhysicsStartTouch( other );
		}
	}

	return link;
}

static trace_t g_TouchTrace;
const trace_t &CBaseEntity::GetTouchTrace( void )
{
	return g_TouchTrace;
}


//-----------------------------------------------------------------------------
// Purpose: Marks the fact that two edicts are in contact
// Input  : *other - other entity
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsMarkEntitiesAsTouching( CBaseEntity *other, trace_t &trace )
{
	g_TouchTrace = trace;
	PhysicsMarkEntityAsTouched( other );
	other->PhysicsMarkEntityAsTouched( this );
}

void CBaseEntity::PhysicsMarkEntitiesAsTouchingEventDriven( CBaseEntity *other, trace_t &trace )
{
	g_TouchTrace = trace;
	g_TouchTrace.m_pEnt = other;

	touchlink_t *link;
	link = this->PhysicsMarkEntityAsTouched( other );
	if ( link )
	{
		// mark these links as event driven so they aren't untouched the next frame
		// when the physics doesn't refresh them
		link->touchStamp = TOUCHSTAMP_EVENT_DRIVEN;
	}
	g_TouchTrace.m_pEnt = this;
	link = other->PhysicsMarkEntityAsTouched( this );
	if ( link )
	{
		link->touchStamp = TOUCHSTAMP_EVENT_DRIVEN;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Two entities have touched, so run their touch functions
// Input  : *other - 
//			*ptrace - 
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsImpact( CBaseEntity *other, trace_t &trace )
{
	if ( !other )
	{
		return;
	}

	// If either of the entities is flagged to be deleted, 
	//  don't call the touch functions
	if ( ( GetFlags() | other->GetFlags() ) & FL_KILLME )
	{
		return;
	}

	PhysicsMarkEntitiesAsTouching( other, trace );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the mask of what is solid for the given entity
// Output : unsigned int
//-----------------------------------------------------------------------------
unsigned int CBaseEntity::PhysicsSolidMaskForEntity( void ) const
{
	return MASK_SOLID;
}


//-----------------------------------------------------------------------------
// Computes the water level + type
//-----------------------------------------------------------------------------
void CBaseEntity::UpdateWaterState()
{
	// FIXME: This computation is nonsensical for rigid child attachments
	// Should we just grab the type + level of the parent?
	// Probably for rigid children anyways...

	// Compute the point to check for water state
	Vector	point;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &point );

	SetWaterLevel( 0 );
	SetWaterType( CONTENTS_EMPTY );
	int cont = UTIL_PointContents (point);

	if (( cont & MASK_WATER ) == 0)
		return;

	SetWaterType( cont );
	SetWaterLevel( 1 );

	// point sized entities are always fully submerged
	if ( IsPointSized() )
	{
		SetWaterLevel( 3 );
	}
	else
	{
		// Check the exact center of the box
		point[2] = WorldSpaceCenter().z;

		int midcont = UTIL_PointContents (point);
		if ( midcont & MASK_WATER )
		{
			// Now check where the eyes are...
			SetWaterLevel( 2 );
			point[2] = EyePosition().z;

			int eyecont = UTIL_PointContents (point);
			if ( eyecont & MASK_WATER )
			{
				SetWaterLevel( 3 );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Check if entity is in the water and applies any current to velocity
// and sets appropriate water flags
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::PhysicsCheckWater( void )
{
	if (GetMoveParent())
		return GetWaterLevel() > 1;

	int cont = GetWaterType();

	// If we're not in water + don't have a current, we're done
	if ( ( cont & (MASK_WATER | MASK_CURRENT) ) != (MASK_WATER | MASK_CURRENT) )
		return GetWaterLevel() > 1;

	// Compute current direction
	Vector v( 0, 0, 0 );
	if ( cont & CONTENTS_CURRENT_0 )
	{
		v[0] += 1;
	}
	if ( cont & CONTENTS_CURRENT_90 )
	{
		v[1] += 1;
	}
	if ( cont & CONTENTS_CURRENT_180 )
	{
		v[0] -= 1;
	}
	if ( cont & CONTENTS_CURRENT_270 )
	{
		v[1] -= 1;
	}
	if ( cont & CONTENTS_CURRENT_UP )
	{
		v[2] += 1;
	}
	if ( cont & CONTENTS_CURRENT_DOWN )
	{
		v[2] -= 1;
	}

	// The deeper we are, the stronger the current.
	Vector newBaseVelocity;
	VectorMA (GetBaseVelocity(), 50.0*GetWaterLevel(), v, newBaseVelocity);
	SetBaseVelocity( newBaseVelocity );
	
	return GetWaterLevel() > 1;
}


//-----------------------------------------------------------------------------
// Purpose: Bounds velocity
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsCheckVelocity( void )
{
	Vector origin = GetAbsOrigin();
	Vector vecAbsVelocity = GetAbsVelocity();

	bool bReset = false;
	for ( int i=0 ; i<3 ; i++ )
	{
		if ( IS_NAN(vecAbsVelocity[i]) )
		{
			Msg( "Got a NaN velocity on %s\n", GetClassname() );
			vecAbsVelocity[i] = 0;
			bReset = true;
		}
		if ( IS_NAN(origin[i]) )
		{
			Msg( "Got a NaN origin on %s\n", GetClassname() );
			origin[i] = 0;
			bReset = true;
		}

		if ( vecAbsVelocity[i] > sv_maxvelocity.GetFloat() ) 
		{
#ifdef _DEBUG
			DevWarning( 2, "Got a velocity too high on %s\n", GetClassname() );
#endif
			vecAbsVelocity[i] = sv_maxvelocity.GetFloat();
			bReset = true;
		}
		else if ( vecAbsVelocity[i] < -sv_maxvelocity.GetFloat() )
		{
#ifdef _DEBUG
			DevWarning( 2, "Got a velocity too low on %s\n", GetClassname() );
#endif
			vecAbsVelocity[i] = -sv_maxvelocity.GetFloat();
			bReset = true;
		}
	}

	if (bReset)
	{
		SetAbsOrigin( origin );
		SetAbsVelocity( vecAbsVelocity );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Applies gravity to falling objects
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsAddGravityMove( Vector &move )
{
	Vector vecAbsVelocity = GetAbsVelocity();

	move.x = (vecAbsVelocity.x + GetBaseVelocity().x ) * gpGlobals->frametime;
	move.y = (vecAbsVelocity.y + GetBaseVelocity().y ) * gpGlobals->frametime;

	if ( GetFlags() & FL_ONGROUND )
	{
		move.z = GetBaseVelocity().z * gpGlobals->frametime;
		return;
	}

	// linear acceleration due to gravity
	float newZVelocity = vecAbsVelocity.z - GetActualGravity( this ) * gpGlobals->frametime;

	move.z = ((vecAbsVelocity.z + newZVelocity) / 2.0 + GetBaseVelocity().z ) * gpGlobals->frametime;

	Vector vecBaseVelocity = GetBaseVelocity();
	vecBaseVelocity.z = 0.0f;
	SetBaseVelocity( vecBaseVelocity );
	
	vecAbsVelocity.z = newZVelocity;
	SetAbsVelocity( vecAbsVelocity );

	// Bound velocity
	PhysicsCheckVelocity();
}


#define	STOP_EPSILON	0.1
//-----------------------------------------------------------------------------
// Purpose: Slide off of the impacting object.  Returns the blocked flags (1 = floor, 2 = step / wall)
// Input  : in - 
//			normal - 
//			out - 
//			overbounce - 
// Output : int
//-----------------------------------------------------------------------------
int CBaseEntity::PhysicsClipVelocity( const Vector& in, const Vector& normal, Vector& out, float overbounce )
{
	float	backoff;
	float	change;
	float angle;
	int		i, blocked;
	
	blocked = 0;

	angle = normal[ 2 ];

	if ( angle > 0 )
	{
		blocked |= 1;		// floor
	}
	if ( !angle )
	{
		blocked |= 2;		// step
	}
	
	backoff = DotProduct (in, normal) * overbounce;

	for ( i=0 ; i<3 ; i++ )
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
		{
			out[i] = 0;
		}
	}
	
	return blocked;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseEntity::ResolveFlyCollisionBounce( trace_t &trace, Vector &vecVelocity, float flMinTotalElasticity )
{
#ifdef HL1_DLL
	flMinTotalElasticity = 0.3f;
#endif//HL1_DLL

	// Get the impact surface's elasticity.
	float flSurfaceElasticity;
	physprops->GetPhysicsProperties( trace.surface.surfaceProps, NULL, NULL, NULL, &flSurfaceElasticity );
	
	float flTotalElasticity = GetElasticity() * flSurfaceElasticity;
	if ( flMinTotalElasticity > 0.9f )
	{
		flMinTotalElasticity = 0.9f;
	}
	flTotalElasticity = clamp( flTotalElasticity, flMinTotalElasticity, 0.9f );

	// NOTE: A backoff of 2.0f is a reflection
	Vector vecAbsVelocity;
	PhysicsClipVelocity( GetAbsVelocity(), trace.plane.normal, vecAbsVelocity, 2.0f );
	vecAbsVelocity *= flTotalElasticity;

	// Get the total velocity (player + conveyors, etc.)
	VectorAdd( vecAbsVelocity, GetBaseVelocity(), vecVelocity );
	float flSpeedSqr = DotProduct( vecVelocity, vecVelocity );

	// Stop if on ground.
	if ( trace.plane.normal.z > 0.7f )			// Floor
	{
		// Verify that we have an entity.
		CBaseEntity *pEntity = trace.m_pEnt;
		Assert( pEntity );

		// Are we on the ground?
		if ( vecVelocity.z < ( GetActualGravity( this ) * gpGlobals->frametime ) )
		{
			vecAbsVelocity.z = 0.0f;

			// Recompute speedsqr based on the new absvel
			VectorAdd( vecAbsVelocity, GetBaseVelocity(), vecVelocity );
			flSpeedSqr = DotProduct( vecVelocity, vecVelocity );
		}

		SetAbsVelocity( vecAbsVelocity );

		if ( flSpeedSqr < ( 30 * 30 ) )
		{
			if ( pEntity->IsStandable() )
			{
				SetGroundEntity( pEntity );
			}

			// Reset velocities.
			SetAbsVelocity( vec3_origin );
			SetLocalAngularVelocity( vec3_angle );
		}
		else
		{
			Vector vecDelta = GetBaseVelocity() - vecAbsVelocity;	
			Vector vecBaseDir = GetBaseVelocity();
			VectorNormalize( vecBaseDir );
			float flScale = vecDelta.Dot( vecBaseDir );

			VectorScale( vecAbsVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, vecVelocity ); 
			VectorMA( vecVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, GetBaseVelocity() * flScale, vecVelocity );
			PhysicsPushEntity( vecVelocity, &trace );
		}
	}
	else
	{
		// If we get *too* slow, we'll stick without ever coming to rest because
		// we'll get pushed down by gravity faster than we can escape from the wall.
		if ( flSpeedSqr < ( 30 * 30 ) )
		{
			// Reset velocities.
			SetAbsVelocity( vec3_origin );
			SetLocalAngularVelocity( vec3_angle );
		}
		else
		{
			SetAbsVelocity( vecAbsVelocity );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseEntity::ResolveFlyCollisionSlide( trace_t &trace, Vector &vecVelocity )
{
	// Get the impact surface's friction.
	float flSurfaceFriction;
	physprops->GetPhysicsProperties( trace.surface.surfaceProps, NULL, NULL, &flSurfaceFriction, NULL );

	// A backoff of 1.0 is a slide.
	float flBackOff = 1.0f;	
	Vector vecAbsVelocity;
	PhysicsClipVelocity( GetAbsVelocity(), trace.plane.normal, vecAbsVelocity, flBackOff );

	if ( trace.plane.normal.z <= 0.7 )			// Floor
	{
		SetAbsVelocity( vecAbsVelocity );
		return;
	}

	// Stop if on ground.
	// Get the total velocity (player + conveyors, etc.)
	VectorAdd( vecAbsVelocity, GetBaseVelocity(), vecVelocity );
	float flSpeedSqr = DotProduct( vecVelocity, vecVelocity );

	// Verify that we have an entity.
	CBaseEntity *pEntity = trace.m_pEnt;
	Assert( pEntity );

	// Are we on the ground?
	if ( vecVelocity.z < ( GetActualGravity( this ) * gpGlobals->frametime ) )
	{
		vecAbsVelocity.z = 0.0f;

		// Recompute speedsqr based on the new absvel
		VectorAdd( vecAbsVelocity, GetBaseVelocity(), vecVelocity );
		flSpeedSqr = DotProduct( vecVelocity, vecVelocity );
	}
	SetAbsVelocity( vecAbsVelocity );

	if ( flSpeedSqr < ( 30 * 30 ) )
	{
		if ( pEntity->IsStandable() )
		{
			SetGroundEntity( pEntity );
		}

		// Reset velocities.
		SetAbsVelocity( vec3_origin );
		SetLocalAngularVelocity( vec3_angle );
	}
	else
	{
		vecAbsVelocity += GetBaseVelocity();
		vecAbsVelocity *= ( 1.0f - trace.fraction ) * gpGlobals->frametime * flSurfaceFriction;
		PhysicsPushEntity( vecAbsVelocity, &trace );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseEntity::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
{
	// Stop if on ground.
	if ( trace.plane.normal.z > 0.7 )			// Floor
	{
		// Get the total velocity (player + conveyors, etc.)
		VectorAdd( GetAbsVelocity(), GetBaseVelocity(), vecVelocity );

		// Verify that we have an entity.
		CBaseEntity *pEntity = trace.m_pEnt;
		Assert( pEntity );

		// Are we on the ground?
		if ( vecVelocity.z < ( GetActualGravity( this ) * gpGlobals->frametime ) )
		{
			Vector vecAbsVelocity = GetAbsVelocity();
			vecAbsVelocity.z = 0.0f;
			SetAbsVelocity( vecAbsVelocity );
		}

		if ( pEntity->IsStandable() )
		{
			SetGroundEntity( pEntity );
		}
	}
}

//-----------------------------------------------------------------------------
// Performs the collision resolution for fliers.
//-----------------------------------------------------------------------------
void CBaseEntity::PerformFlyCollisionResolution( trace_t &trace, Vector &move )
{
	switch( GetMoveCollide() )
	{
	case MOVECOLLIDE_FLY_CUSTOM:
		{
			ResolveFlyCollisionCustom( trace, move );
			break;
		}

	case MOVECOLLIDE_FLY_BOUNCE:
		{
			ResolveFlyCollisionBounce( trace, move );
			break;
		}

	case MOVECOLLIDE_FLY_SLIDE:
	case MOVECOLLIDE_DEFAULT:
	// NOTE: The default fly collision state is the same as a slide (for backward capatability).
		{
			ResolveFlyCollisionSlide( trace, move );
			break;
		}

	default:
		{
			// Invalid MOVECOLLIDE_<type>
			Assert( 0 );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks if an object has passed into or out of water and sets water info, alters velocity, plays splash sounds, etc.
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsCheckWaterTransition( void )
{
	int oldcont = GetWaterType();
	UpdateWaterState();
	int cont = GetWaterType();

	// We can exit right out if we're a child... don't bother with this...
	if (GetMoveParent())
		return;

	if ( cont & MASK_WATER )
	{
		if (oldcont == CONTENTS_EMPTY)
		{
#ifndef CLIENT_DLL
			Splash();
#endif // !CLIENT_DLL

			// just crossed into water
			EmitSound( "BaseEntity.EnterWater" );

			if ( !IsEFlagSet( EFL_NO_WATER_VELOCITY_CHANGE ) )
			{
				Vector vecAbsVelocity = GetAbsVelocity();
				vecAbsVelocity[2] *= 0.5;
				SetAbsVelocity( vecAbsVelocity );
			}
		}
	}
	else
	{
		if ( oldcont != CONTENTS_EMPTY )
		{	
			// just crossed out of water
			EmitSound( "BaseEntity.ExitWater" );
		}		
	}
}

//-----------------------------------------------------------------------------
// Computes new angles based on the angular velocity
//-----------------------------------------------------------------------------
void CBaseEntity::SimulateAngles( float flFrameTime )
{
	// move angles
	QAngle angles;
	VectorMA ( GetLocalAngles(), flFrameTime, GetLocalAngularVelocity(), angles );
	SetLocalAngles( angles );
}


//-----------------------------------------------------------------------------
// Purpose: Toss, bounce, and fly movement.  When onground, do nothing.
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsToss( void )
{
	trace_t	trace;
	Vector	move;

	PhysicsCheckWater();

	// regular thinking
	if ( !PhysicsRunThink() )
		return;

	// Moving upward, off the ground, or  resting on a client/monster, remove FL_ONGROUND
	if ( GetAbsVelocity()[2] > 0 || !GetGroundEntity() || !GetGroundEntity()->IsStandable() )
	{
		SetGroundEntity( NULL );
	}

	// Check to see if entity is on the ground at rest
	if ( GetFlags() & FL_ONGROUND )
	{
		if ( VectorCompare( GetAbsVelocity(), vec3_origin ) )
		{
			// Clear rotation if not moving (even if on a conveyor)
			SetLocalAngularVelocity( vec3_angle );
			if ( VectorCompare( GetBaseVelocity(), vec3_origin ) )
				return;
		}
	}

	PhysicsCheckVelocity();

	// add gravity
	if ( GetMoveType() == MOVETYPE_FLYGRAVITY && !(GetFlags() & FL_FLY) )
	{
		PhysicsAddGravityMove( move );
	}
	else
	{
		// Base velocity is not properly accounted for since this entity will move again after the bounce without
		// taking it into account
		Vector vecAbsVelocity = GetAbsVelocity();
		vecAbsVelocity += GetBaseVelocity();
		VectorScale(vecAbsVelocity, gpGlobals->frametime, move);
		PhysicsCheckVelocity( );
	}

	// move angles
	SimulateAngles( gpGlobals->frametime );

	// move origin
	PhysicsPushEntity( move, &trace );

#if !defined( CLIENT_DLL )
	if ( VPhysicsGetObject() )
	{
		VPhysicsGetObject()->UpdateShadow( GetAbsOrigin(), vec3_angle, true, gpGlobals->frametime );
	}
#endif

	PhysicsCheckVelocity();

	if (trace.allsolid )
	{	
		// entity is trapped in another solid
		// UNDONE: does this entity needs to be removed?
		SetAbsVelocity(vec3_origin);
		SetLocalAngularVelocity(vec3_angle);
		return;
	}
	
#if !defined( CLIENT_DLL )
	if (IsEdictFree())
		return;
#endif

	if (trace.fraction != 1.0f)
	{
		PerformFlyCollisionResolution( trace, move );
	}
	
	// check for in water
	PhysicsCheckWaterTransition();
}


//-----------------------------------------------------------------------------
// Simulation in local space of rigid children
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsRigidChild( void )
{
	VPROF("CBaseEntity::PhysicsRigidChild");
	// NOTE: rigidly attached children do simulation in local space
	// Collision impulses will be handled either not at all, or by
	// forwarding the information to the highest move parent

	Vector vecPrevOrigin = GetAbsOrigin();

	// regular thinking
	if ( !PhysicsRunThink() )
		return;

	VPROF_SCOPE_BEGIN("CBaseEntity::PhysicsRigidChild-2");

#if !defined( CLIENT_DLL )
	// Cause touch functions to be called
	PhysicsTouchTriggers( &vecPrevOrigin );

	// We have to do this regardless owing to hierarchy
	if ( VPhysicsGetObject() )
	{
		int solidType = GetSolid();
		bool bAxisAligned = ( solidType == SOLID_BBOX || solidType == SOLID_NONE ) ? true : false;
		VPhysicsGetObject()->UpdateShadow( GetAbsOrigin(), bAxisAligned ? vec3_angle : GetAbsAngles(), true, gpGlobals->frametime );
	}
#endif

	VPROF_SCOPE_END();
}


//-----------------------------------------------------------------------------
// Computes the base velocity
//-----------------------------------------------------------------------------
void CBaseEntity::UpdateBaseVelocity( void )
{
#if !defined( CLIENT_DLL )
	if ( GetFlags() & FL_ONGROUND )
	{
		CBaseEntity	*groundentity = GetGroundEntity();
		if ( groundentity )
		{
			// On conveyor belt that's moving?
			if ( groundentity->GetFlags() & FL_CONVEYOR )
			{
				Vector vecNewBaseVelocity;
				groundentity->GetGroundVelocityToApply( vecNewBaseVelocity );
				if ( GetFlags() & FL_BASEVELOCITY )
				{
					vecNewBaseVelocity += GetBaseVelocity();
				}
				AddFlag( FL_BASEVELOCITY );
				SetBaseVelocity( vecNewBaseVelocity );
			}
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Runs a frame of physics for a specific edict (and all it's children)
// Input  : *ent - the thinking edict
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsSimulate( void )
{
	VPROF( "CBaseEntity::PhysicsSimulate" );
	// NOTE:  Players override PhysicsSimulate and drive through their CUserCmds at that point instead of
	//  processng through this function call!!!  They shouldn't chain to here ever.
	// Make sure not to simulate this guy twice per frame
	if (m_nSimulationTick == gpGlobals->tickcount)
		return;

	m_nSimulationTick = gpGlobals->tickcount;

	Assert( !IsPlayer() );

	// If we've got a moveparent, we must simulate that first.
	CBaseEntity *pMoveParent = GetMoveParent();

	if ( (GetMoveType() == MOVETYPE_NONE && !pMoveParent) || (GetMoveType() == MOVETYPE_VPHYSICS ) )
	{
		PhysicsNone();
		return;
	}

	// If ground entity goes away, make sure FL_ONGROUND is valid
	if ( !GetGroundEntity() )
	{
		RemoveFlag( FL_ONGROUND );
	}

	if (pMoveParent)
	{
		VPROF( "CBaseEntity::PhysicsSimulate-MoveParent" );
		pMoveParent->PhysicsSimulate();
	}
	else
	{
		VPROF( "CBaseEntity::PhysicsSimulate-BaseVelocity" );

		UpdateBaseVelocity();

		if ( ((GetFlags() & FL_BASEVELOCITY) == 0) && (GetBaseVelocity() != vec3_origin) )
		{
			// Apply momentum (add in half of the previous frame of velocity first)
			// BUGBUG: This will break with PhysicsStep() because of the timestep difference
			Vector vecAbsVelocity;
			VectorMA( GetAbsVelocity(), 1.0 + (gpGlobals->frametime*0.5), GetBaseVelocity(), vecAbsVelocity );
			SetAbsVelocity( vecAbsVelocity );
			SetBaseVelocity( vec3_origin );
		}
		RemoveFlag( FL_BASEVELOCITY );
	}

	switch( GetMoveType() )
	{
	case MOVETYPE_PUSH:
		{
			VPROF( "CBaseEntity::PhysicsSimulate-MOVETYPE_PUSH" );
			PhysicsPusher();
		}
		break;


	case MOVETYPE_VPHYSICS:
		{
		}
		break;

	case MOVETYPE_NONE:
		{
			VPROF( "CBaseEntity::PhysicsSimulate-MOVETYPE_NONE" );
			Assert(pMoveParent);
			PhysicsRigidChild();
		}
		break;

	case MOVETYPE_NOCLIP:
		{
			VPROF( "CBaseEntity::PhysicsSimulate-MOVETYPE_NOCLIP" );
			PhysicsNoclip();
		}
		break;

	case MOVETYPE_STEP:
		{
			VPROF( "CBaseEntity::PhysicsSimulate-MOVETYPE_STEP" );
			PhysicsStep();
		}
		break;

	case MOVETYPE_FLY:
	case MOVETYPE_FLYGRAVITY:
		{
			VPROF( "CBaseEntity::PhysicsSimulate-MOVETYPE_FLY" );
			PhysicsToss();
		}
		break;

	case MOVETYPE_CUSTOM:
		{
			VPROF( "CBaseEntity::PhysicsSimulate-MOVETYPE_CUSTOM" );
			PhysicsCustom();
		}
		break;

	default:
		Warning( "PhysicsSimulate: %s bad movetype %d", GetClassname(), GetMoveType() );
		Assert(0);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Runs thinking code if time.  There is some play in the exact time the think
//  function will be called, because it is called before any movement is done
//  in a frame.  Not used for pushmove objects, because they must be exact.
//  Returns false if the entity removed itself.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::PhysicsRunThink( thinkmethods_t thinkMethod )
{
	if ( IsEFlagSet( EFL_NO_THINK_FUNCTION ) )
		return true;
	
	bool bAlive = true;

	// Don't fire the base if we're avoiding it
	if ( thinkMethod != THINK_FIRE_ALL_BUT_BASE )
	{
		bAlive = PhysicsRunSpecificThink( -1, &CBaseEntity::Think );
		if ( !bAlive )
			return false;
	}

	// Are we just firing the base think?
	if ( thinkMethod == THINK_FIRE_BASE_ONLY )
		return bAlive;

	// Fire the rest of 'em
	for ( int i = 0; i < m_aThinkFunctions.Count(); i++ )
	{
#ifdef _DEBUG
		// Set the context
		m_iCurrentThinkContext = i;
#endif

		bAlive = PhysicsRunSpecificThink( i, m_aThinkFunctions[i].m_pfnThink );

#ifdef _DEBUG
		// Clear our context
		m_iCurrentThinkContext = NO_THINK_CONTEXT;
#endif

		if ( !bAlive )
			return false;
	}
	
	return bAlive;
}

//-----------------------------------------------------------------------------
// Purpose: For testing if all thinks are occuring at the same time
//-----------------------------------------------------------------------------
struct ThinkSync
{
	float					thinktime;
	int						thinktick;
	CUtlVector< EHANDLE >	entities;

	ThinkSync()
	{
		thinktime = 0;
	}

	ThinkSync( const ThinkSync& src )
	{
		thinktime = src.thinktime;
		thinktick = src.thinktick;
		int c = src.entities.Count();
		for ( int i = 0; i < c; i++ )
		{
			entities.AddToTail( src.entities[ i ] );
		}
	}
};

#if !defined( CLIENT_DLL )
static ConVar sv_thinktimecheck( "sv_thinktimecheck", "0", 0, "Check for thinktimes all on same timestamp." );
#endif

//-----------------------------------------------------------------------------
// Purpose: For testing if all thinks are occuring at the same time
//-----------------------------------------------------------------------------
class CThinkSyncTester
{
public:
	CThinkSyncTester() :
	  m_Thinkers( 0, 0, ThinkLessFunc )
	{
		  m_nLastFrameCount = -1;
		  m_bShouldCheck = false;
	}

	void EntityThinking( int framecount, CBaseEntity *ent, float thinktime, int thinktick )
	{
#if !defined( CLIENT_DLL )
		if ( m_nLastFrameCount != framecount )
		{
			if ( m_bShouldCheck )
			{
				// Report
				Report();
				m_Thinkers.RemoveAll();
				m_nLastFrameCount = framecount;
			}

			m_bShouldCheck = sv_thinktimecheck.GetBool();
		}

		if ( !m_bShouldCheck )
			return;

		ThinkSync *p = FindOrAddItem( ent, thinktime );
		if ( !p )
		{
			Assert( 0 );
		}

		p->thinktime = thinktime;
		p->thinktick = thinktick;
		EHANDLE h;
		h = ent;
		p->entities.AddToTail( h );
#endif
	}

private:

	static bool ThinkLessFunc( const ThinkSync& item1, const ThinkSync& item2 )
	{
		return item1.thinktime < item2.thinktime;
	}

	ThinkSync	*FindOrAddItem( CBaseEntity *ent, float thinktime )
	{
		ThinkSync item;
		item.thinktime = thinktime;

		int idx = m_Thinkers.Find( item );
		if ( idx == m_Thinkers.InvalidIndex() )
		{
			idx = m_Thinkers.Insert( item );
		}
		
		return &m_Thinkers[ idx ];
	}

	void Report()
	{
		if ( m_Thinkers.Count() == 0 )
			return;

		Msg( "-----------------\nThink report frame %i\n", gpGlobals->tickcount );

		for ( int i = m_Thinkers.FirstInorder(); 
			i != m_Thinkers.InvalidIndex(); 
			i = m_Thinkers.NextInorder( i ) )
		{
			ThinkSync *p = &m_Thinkers[ i ];
			Assert( p );
			if ( !p )
				continue;

			int ecount = p->entities.Count();
			if ( !ecount )
			{
				continue;
			}

			Msg( "thinktime %f, %i entities\n", p->thinktime, ecount );
			for ( int j =0; j < ecount; j++ )
			{
				EHANDLE h = p->entities[ j ];
				int lastthinktick = 0;
				int nextthinktick = 0;
				CBaseEntity *e = h.Get();
				if ( e )
				{
					lastthinktick = e->m_nLastThinkTick;
					nextthinktick = e->m_nNextThinkTick;
				}

				Msg( "  %p : %30s (last %5i/next %5i)\n", h.Get(), h.Get() ? h->GetClassname() : "NULL",
					lastthinktick, nextthinktick );
			}
		}
	}

	CUtlRBTree< ThinkSync >	m_Thinkers;
	int			m_nLastFrameCount;
	bool		m_bShouldCheck;
};

static CThinkSyncTester g_ThinkChecker;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseEntity::PhysicsRunSpecificThink( int nContextIndex, BASEPTR thinkFunc )
{
	int thinktick = GetNextThinkTick( nContextIndex );

	if ( thinktick <= 0 || thinktick > gpGlobals->tickcount )
		return true;
	
	float thinktime = thinktick * TICK_INTERVAL;

	// Don't let things stay in the past.
	//  it is possible to start that way
	//  by a trigger with a local time.
	if ( thinktime < gpGlobals->curtime )
	{
		thinktime = gpGlobals->curtime;	
	}
	
	// Only do this on the game server
#if !defined( CLIENT_DLL )
	g_ThinkChecker.EntityThinking( gpGlobals->tickcount, this, thinktime, m_nNextThinkTick );
#endif

	SetNextThink( nContextIndex, TICK_NEVER_THINK );

	PhysicsDispatchThink( thinkFunc );

	SetLastThink( nContextIndex, gpGlobals->curtime );

	// Return whether entity is still valid
	return ( !IsMarkedForDeletion() );
}

void CBaseEntity::SetGroundEntity( CBaseEntity *ground )
{
	if ( m_hGroundEntity.Get() == ground )
		return;

#ifdef GAME_DLL
	// this can happen in-between updates to the held object controller (physcannon, +USE)
	// so trap it here and release held objects when they become player ground
	if ( ground && IsPlayer() && ground->GetMoveType()== MOVETYPE_VPHYSICS )
	{
		CBasePlayer *pPlayer = ToBasePlayer(this);
		IPhysicsObject *pPhysGround = ground->VPhysicsGetObject();
		if ( pPhysGround && pPlayer )
		{
			if ( pPhysGround->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
			{
				pPlayer->ForceDropOfCarriedPhysObjects( ground );
			}
		}
	}
#endif

	CBaseEntity *oldGround = m_hGroundEntity;
	m_hGroundEntity = ground;

	// Just starting to touch
	if ( !oldGround && ground )
	{
		ground->AddEntityToGroundList( this );
	}
	// Just stopping touching
	else if ( oldGround && !ground )
	{
		PhysicsNotifyOtherOfGroundRemoval( this, oldGround );
	}
	// Changing out to new ground entity
	else
	{
		PhysicsNotifyOtherOfGroundRemoval( this, oldGround );
		ground->AddEntityToGroundList( this );
	}

	// HACK/PARANOID:  This is redundant with the code above, but in case we get out of sync groundlist entries ever, 
	//  this will force the appropriate flags
	if ( ground )
	{
		AddFlag( FL_ONGROUND );
	}
	else
	{
		RemoveFlag( FL_ONGROUND );
	}
}

CBaseEntity *CBaseEntity::GetGroundEntity( void )
{
	return m_hGroundEntity;
}

void CBaseEntity::StartGroundContact( CBaseEntity *ground )
{
	AddFlag( FL_ONGROUND );
//	Msg( "+++ %s starting contact with ground %s\n", GetClassname(), ground->GetClassname() );
}

void CBaseEntity::EndGroundContact( CBaseEntity *ground )
{
	RemoveFlag( FL_ONGROUND );
//	Msg( "--- %s ending contact with ground %s\n", GetClassname(), ground->GetClassname() );
}


void CBaseEntity::SetGroundChangeTime( float flTime )
{
	m_flGroundChangeTime = flTime;
}

float CBaseEntity::GetGroundChangeTime( void )
{
	return m_flGroundChangeTime;
}



// Remove this as ground entity for all object resting on this object
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseEntity::WakeRestingObjects()
{
	// Unset this as ground entity for everything resting on this object
	//  This calls endgroundcontact for everything on the list
	PhysicsRemoveGroundList( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ent - 
//-----------------------------------------------------------------------------
bool CBaseEntity::HasNPCsOnIt( void )
{
	groundlink_t *link;
	groundlink_t *root = ( groundlink_t * )GetDataObject( GROUNDLINK );
	if ( root )
	{
		for ( link = root->nextLink; link != root; link = link->nextLink )
		{
			if ( link->entity && link->entity->MyNPCPointer() )
				return true;
		}
	}

	return false;
}
