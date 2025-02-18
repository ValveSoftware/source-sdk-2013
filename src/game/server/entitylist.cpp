//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "entitylist.h"
#include "utlvector.h"
#include "igamesystem.h"
#include "collisionutils.h"
#include "UtlSortVector.h"
#include "tier0/vprof.h"
#include "mapentities.h"
#include "client.h"
#include "ai_initutils.h"
#include "globalstate.h"
#include "datacache/imdlcache.h"

#ifdef HL2_DLL
#include "npc_playercompanion.h"
#endif // HL2_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
void SceneManager_ClientActive( CBasePlayer *player );

static CUtlVector<IServerNetworkable*> g_DeleteList;

CGlobalEntityList gEntList;
CBaseEntityList *g_pEntityList = &gEntList;

class CAimTargetManager : public IEntityListener
{
public:
	// Called by CEntityListSystem
	void LevelInitPreEntity() 
	{ 
		gEntList.AddListenerEntity( this );
		Clear(); 
	}
	void LevelShutdownPostEntity()
	{
		gEntList.RemoveListenerEntity( this );
		Clear();
	}

	void Clear() 
	{ 
		m_targetList.Purge(); 
	}

	void ForceRepopulateList()
	{
		Clear();

		CBaseEntity *pEnt = gEntList.FirstEnt();

		while( pEnt )
		{
			if( ShouldAddEntity(pEnt) )
				AddEntity(pEnt);

			pEnt = gEntList.NextEnt( pEnt );
		}
	}
	
	bool ShouldAddEntity( CBaseEntity *pEntity )
	{
		return ((pEntity->GetFlags() & FL_AIMTARGET) != 0);
	}

	// IEntityListener
	virtual void OnEntityCreated( CBaseEntity *pEntity ) {}
	virtual void OnEntityDeleted( CBaseEntity *pEntity )
	{
		if ( !(pEntity->GetFlags() & FL_AIMTARGET) )
			return;
		RemoveEntity(pEntity);
	}
	void AddEntity( CBaseEntity *pEntity )
	{
		if ( pEntity->IsMarkedForDeletion() )
			return;
		m_targetList.AddToTail( pEntity );
	}
	void RemoveEntity( CBaseEntity *pEntity )
	{
		int index = m_targetList.Find( pEntity );
		if ( m_targetList.IsValidIndex(index) )
		{
			m_targetList.FastRemove( index );
		}
	}
	int ListCount() { return m_targetList.Count(); }
	int ListCopy( CBaseEntity *pList[], int listMax )
	{
		int count = MIN(listMax, ListCount() );
		memcpy( pList, m_targetList.Base(), sizeof(CBaseEntity *) * count );
		return count;
	}

private:
	CUtlVector<CBaseEntity *>	m_targetList;
};

static CAimTargetManager g_AimManager;

int AimTarget_ListCount()
{
	return g_AimManager.ListCount();
}
int AimTarget_ListCopy( CBaseEntity *pList[], int listMax )
{
	return g_AimManager.ListCopy( pList, listMax );
}
void AimTarget_ForceRepopulateList()
{
	g_AimManager.ForceRepopulateList();
}


// Manages a list of all entities currently doing game simulation or thinking
// NOTE: This is usually a small subset of the global entity list, so it's
// an optimization to maintain this list incrementally rather than polling each
// frame.
struct simthinkentry_t
{
	unsigned short	entEntry;
	unsigned short	unused0;
	int				nextThinkTick;
};
class CSimThinkManager : public IEntityListener
{
public:
	CSimThinkManager()
	{
		Clear();
	}
	void Clear()
	{
		m_simThinkList.Purge();
		for ( int i = 0; i < ARRAYSIZE(m_entinfoIndex); i++ )
		{
			m_entinfoIndex[i] = 0xFFFF;
		}
	}
	void LevelInitPreEntity()
	{
		gEntList.AddListenerEntity( this );
	}

	void LevelShutdownPostEntity()
	{
		gEntList.RemoveListenerEntity( this );
		Clear();
	}

	void OnEntityCreated( CBaseEntity *pEntity )
	{
		Assert( m_entinfoIndex[pEntity->GetRefEHandle().GetEntryIndex()] == 0xFFFF );
	}
	void OnEntityDeleted( CBaseEntity *pEntity )
	{
		RemoveEntinfoIndex( pEntity->GetRefEHandle().GetEntryIndex() );
	}

	void RemoveEntinfoIndex( int index )
	{
		int listHandle = m_entinfoIndex[index];
		// If this guy is in the active list, remove him
		if ( listHandle != 0xFFFF )
		{
			Assert(m_simThinkList[listHandle].entEntry == index);
			m_simThinkList.FastRemove( listHandle );
			m_entinfoIndex[index] = 0xFFFF;
			
			// fast remove shifted someone, update that someone
			if ( listHandle < m_simThinkList.Count() )
			{
				m_entinfoIndex[m_simThinkList[listHandle].entEntry] = listHandle;
			}
		}
	}
	int ListCount()
	{
		return m_simThinkList.Count();
	}

	int ListCopy( CBaseEntity *pList[], int listMax )
	{
		int count = MIN(listMax, ListCount());
		int out = 0;
		for ( int i = 0; i < count; i++ )
		{
			// only copy out entities that will simulate or think this frame
			if ( m_simThinkList[i].nextThinkTick <= gpGlobals->tickcount )
			{
				Assert(m_simThinkList[i].nextThinkTick>=0);
				int entinfoIndex = m_simThinkList[i].entEntry;
				const CEntInfo *pInfo = gEntList.GetEntInfoPtrByIndex( entinfoIndex );
				pList[out] = (CBaseEntity *)pInfo->m_pEntity;
				Assert(m_simThinkList[i].nextThinkTick==0 || pList[out]->GetFirstThinkTick()==m_simThinkList[i].nextThinkTick);
				Assert( gEntList.IsEntityPtr( pList[out] ) );
				out++;
			}
		}

		return out;
	}

	void EntityChanged( CBaseEntity *pEntity )
	{
		// might change after deletion, don't put back into the list
		if ( pEntity->IsMarkedForDeletion() )
			return;
		
		const CBaseHandle &eh = pEntity->GetRefEHandle();
		if ( !eh.IsValid() )
			return;

		int index = eh.GetEntryIndex();
		if ( pEntity->IsEFlagSet( EFL_NO_THINK_FUNCTION ) && pEntity->IsEFlagSet( EFL_NO_GAME_PHYSICS_SIMULATION ) )
		{
			RemoveEntinfoIndex( index );
		}
		else
		{
			// already in the list? (had think or sim last time, now has both - or had both last time, now just one)
			if ( m_entinfoIndex[index] == 0xFFFF )
			{
				MEM_ALLOC_CREDIT();
				m_entinfoIndex[index] = m_simThinkList.AddToTail();
				m_simThinkList[m_entinfoIndex[index]].entEntry = (unsigned short)index;
				m_simThinkList[m_entinfoIndex[index]].nextThinkTick = 0;
				if ( pEntity->IsEFlagSet(EFL_NO_GAME_PHYSICS_SIMULATION) )
				{
					m_simThinkList[m_entinfoIndex[index]].nextThinkTick = pEntity->GetFirstThinkTick();
					Assert(m_simThinkList[m_entinfoIndex[index]].nextThinkTick>=0);
				}
			}
			else
			{
				// updating existing entry - if no sim, reset think time
				if ( pEntity->IsEFlagSet(EFL_NO_GAME_PHYSICS_SIMULATION) )
				{
					m_simThinkList[m_entinfoIndex[index]].nextThinkTick = pEntity->GetFirstThinkTick();
					Assert(m_simThinkList[m_entinfoIndex[index]].nextThinkTick>=0);
				}
				else
				{
					m_simThinkList[m_entinfoIndex[index]].nextThinkTick = 0;
				}
			}
		}
	}

private:
	unsigned short m_entinfoIndex[NUM_ENT_ENTRIES];
	CUtlVector<simthinkentry_t>	m_simThinkList;
};

CSimThinkManager g_SimThinkManager;

int SimThink_ListCount()
{
	return g_SimThinkManager.ListCount();
}

int SimThink_ListCopy( CBaseEntity *pList[], int listMax )
{
	return g_SimThinkManager.ListCopy( pList, listMax );
}

void SimThink_EntityChanged( CBaseEntity *pEntity )
{
	g_SimThinkManager.EntityChanged( pEntity );
}

static CBaseEntityClassList *s_pClassLists = NULL;
CBaseEntityClassList::CBaseEntityClassList()
{
	m_pNextClassList = s_pClassLists;
	s_pClassLists = this;
}
CBaseEntityClassList::~CBaseEntityClassList()
{
}

CGlobalEntityList::CGlobalEntityList()
{
	m_iHighestEnt = m_iNumEnts = m_iNumEdicts = 0;
	m_bClearingEntities = false;
}


// removes the entity from the global list
// only called from with the CBaseEntity destructor
static bool g_fInCleanupDelete;


// mark an entity as deleted
void CGlobalEntityList::AddToDeleteList( IServerNetworkable *ent )
{
	if ( ent && ent->GetEntityHandle()->GetRefEHandle() != CBaseHandle( INVALID_EHANDLE ) )
	{
		g_DeleteList.AddToTail( ent );
	}
}

extern bool g_bDisableEhandleAccess;
// call this before and after each frame to delete all of the marked entities.
void CGlobalEntityList::CleanupDeleteList( void )
{
	VPROF( "CGlobalEntityList::CleanupDeleteList" );
	g_fInCleanupDelete = true;
	// clean up the vphysics delete list as well
	PhysOnCleanupDeleteList();

	g_bDisableEhandleAccess = true;
	for ( int i = 0; i < g_DeleteList.Count(); i++ )
	{
		g_DeleteList[i]->Release();
	}
	g_bDisableEhandleAccess = false;
	g_DeleteList.RemoveAll();

	g_fInCleanupDelete = false;
}

int CGlobalEntityList::ResetDeleteList( void )
{
	int result = g_DeleteList.Count();
	g_DeleteList.RemoveAll();
	return result;
}


	// add a class that gets notified of entity events
void CGlobalEntityList::AddListenerEntity( IEntityListener *pListener )
{
	if ( m_entityListeners.Find( pListener ) >= 0 )
	{
		AssertMsg( 0, "Can't add listeners multiple times\n" );
		return;
	}
	m_entityListeners.AddToTail( pListener );
}

void CGlobalEntityList::RemoveListenerEntity( IEntityListener *pListener )
{
	m_entityListeners.FindAndRemove( pListener );
}

void CGlobalEntityList::Clear( void )
{
	m_bClearingEntities = true;

	// Add all remaining entities in the game to the delete list and call appropriate UpdateOnRemove
	CBaseHandle hCur = FirstHandle();
	while ( hCur != InvalidHandle() )
	{
		IServerNetworkable *ent = GetServerNetworkable( hCur );
		if ( ent )
		{
			MDLCACHE_CRITICAL_SECTION();
			// Force UpdateOnRemove to be called
			UTIL_Remove( ent );
		}
		hCur = NextHandle( hCur );
	}
		
	CleanupDeleteList();
	// free the memory
	g_DeleteList.Purge();

	CBaseEntity::m_nDebugPlayer = -1;
	CBaseEntity::m_bInDebugSelect = false; 
	m_iHighestEnt = 0;
	m_iNumEnts = 0;

	m_bClearingEntities = false;
}


int CGlobalEntityList::NumberOfEntities( void )
{
	return m_iNumEnts;
}

int CGlobalEntityList::NumberOfEdicts( void )
{
	return m_iNumEdicts;
}

CBaseEntity *CGlobalEntityList::NextEnt( CBaseEntity *pCurrentEnt ) 
{ 
	if ( !pCurrentEnt )
	{
		const CEntInfo *pInfo = FirstEntInfo();
		if ( !pInfo )
			return NULL;

		return (CBaseEntity *)pInfo->m_pEntity;
	}

	// Run through the list until we get a CBaseEntity.
	const CEntInfo *pList = GetEntInfoPtr( pCurrentEnt->GetRefEHandle() );
	if ( pList )
		pList = NextEntInfo(pList);

	while ( pList )
	{
#if 0
		if ( pList->m_pEntity )
		{
			IServerUnknown *pUnk = static_cast<IServerUnknown*>(const_cast<IHandleEntity*>(pList->m_pEntity));
			CBaseEntity *pRet = pUnk->GetBaseEntity();
			if ( pRet )
				return pRet;
		}
#else
		return (CBaseEntity *)pList->m_pEntity;
#endif
		pList = pList->m_pNext;
	}
	
	return NULL; 

}


void CGlobalEntityList::ReportEntityFlagsChanged( CBaseEntity *pEntity, unsigned int flagsOld, unsigned int flagsNow )
{
	if ( pEntity->IsMarkedForDeletion() )
		return;
	// UNDONE: Move this into IEntityListener instead?
	unsigned int flagsChanged = flagsOld ^ flagsNow;
	if ( flagsChanged & FL_AIMTARGET )
	{
		unsigned int flagsAdded = flagsNow & flagsChanged;
		unsigned int flagsRemoved = flagsOld & flagsChanged;

		if ( flagsAdded & FL_AIMTARGET )
		{
			g_AimManager.AddEntity( pEntity );
		}
		if ( flagsRemoved & FL_AIMTARGET )
		{
			g_AimManager.RemoveEntity( pEntity );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used to confirm a pointer is a pointer to an entity, useful for
//			asserts.
//-----------------------------------------------------------------------------
bool CGlobalEntityList::IsEntityPtr( void *pTest )
{
	if ( pTest )
	{
		const CEntInfo *pInfo = FirstEntInfo();
		for ( ;pInfo; pInfo = pInfo->m_pNext )
		{
			if ( pTest == (void *)pInfo->m_pEntity )
				return true;
		}
	}

	return false; 
}

//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given classname.
// Input  : pStartEntity - Last entity found, NULL to start a new iteration.
//			szName - Classname to search for.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByClassname( CBaseEntity *pStartEntity, const char *szName, IEntityFindFilter *pFilter )
{
	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		CBaseEntity *pEntity = (CBaseEntity *)pInfo->m_pEntity;
		if ( !pEntity )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		if ( pEntity->ClassMatches(szName) )
		{
			if ( pFilter && !pFilter->ShouldFindEntity( pEntity ) )
				continue;

			return pEntity;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds an entity given a procedural name.
// Input  : szName - The procedural name to search for, should start with '!'.
//			pSearchingEntity - 
//			pActivator - The activator entity if this was called from an input
//				or Use handler.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityProcedural( const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	//
	// Check for the name escape character.
	//
	if ( szName[0] == '!' )
	{
		const char *pName = szName + 1;

		//
		// It is a procedural name, look for the ones we understand.
		//
		if ( FStrEq( pName, "player" ) )
		{
			return (CBaseEntity *)UTIL_PlayerByIndex( 1 );
		}
		else if ( FStrEq( pName, "pvsplayer" ) )
		{
			if ( pSearchingEntity )
			{
				return CBaseEntity::Instance( UTIL_FindClientInPVS( pSearchingEntity->edict() ) );
			}
			else if ( pActivator )
			{
				// FIXME: error condition?
				return CBaseEntity::Instance( UTIL_FindClientInPVS( pActivator->edict() ) );
			}
			else
			{
				// FIXME: error condition?
				return (CBaseEntity *)UTIL_PlayerByIndex( 1 );
			}

		}
		else if ( FStrEq( pName, "activator" ) )
		{
			return pActivator;
		}
		else if ( FStrEq( pName, "caller" ) )
		{
			return pCaller;
		}
		else if ( FStrEq( pName, "picker" ) )
		{
			return FindPickerEntity( UTIL_PlayerByIndex(1) );
		}
		else if ( FStrEq( pName, "self" ) )
		{
			return pSearchingEntity;
		}
		else 
		{
			Warning( "Invalid entity search name %s\n", szName );
			Assert(0);
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given name.
// Input  : pStartEntity - Last entity found, NULL to start a new iteration.
//			szName - Name to search for.
//			pActivator - Activator entity if this was called from an input
//				handler or Use handler.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByName( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller, IEntityFindFilter *pFilter )
{
	if ( !szName || szName[0] == 0 )
		return NULL;

	if ( szName[0] == '!' )
	{
		//
		// Avoid an infinite loop, only find one match per procedural search!
		//
		if (pStartEntity == NULL)
			return FindEntityProcedural( szName, pSearchingEntity, pActivator, pCaller );

		return NULL;
	}
	
	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		if ( !ent->m_iName )
			continue;

		if ( ent->NameMatches( szName ) )
		{
			if ( pFilter && !pFilter->ShouldFindEntity(ent) )
				continue;

			return ent;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pStartEntity - 
//			szModelName - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByModel( CBaseEntity *pStartEntity, const char *szModelName, IEntityFindFilter *pFilter )
{
	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		if ( !ent->edict() || !ent->GetModelName() )
			continue;

		if ( FStrEq( STRING(ent->GetModelName()), szModelName ) )
		{
			if ( pFilter && !pFilter->ShouldFindEntity( ent ) )
				continue;

			return ent;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given target.
// Input  : pStartEntity - 
//			szName - 
//-----------------------------------------------------------------------------
// FIXME: obsolete, remove
CBaseEntity	*CGlobalEntityList::FindEntityByTarget( CBaseEntity *pStartEntity, const char *szName, IEntityFindFilter *pFilter )
{
	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		if ( !ent->m_target )
			continue;

		if ( FStrEq( STRING(ent->m_target), szName ) )
		{
			if ( pFilter && !pFilter->ShouldFindEntity( ent ) )
				continue;

			return ent;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Used to iterate all the entities within a sphere.
// Input  : pStartEntity - 
//			vecCenter - 
//			flRadius - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius, IEntityFindFilter *pFilter )
{
	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		if ( !ent->edict() )
			continue;

		Vector vecRelativeCenter;
		ent->CollisionProp()->WorldToCollisionSpace( vecCenter, &vecRelativeCenter );
		if ( !IsBoxIntersectingSphere( ent->CollisionProp()->OBBMins(),	ent->CollisionProp()->OBBMaxs(), vecRelativeCenter, flRadius ) )
			continue;

		if ( pFilter && !pFilter->ShouldFindEntity( ent ) )
			continue;

		return ent;
	}

	// nothing found
	return NULL; 
}


//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by name within a radius
// Input  : szName - Entity name to search for.
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByNameNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller, IEntityFindFilter *pFilter )
{
	CBaseEntity *pEntity = NULL;

	//
	// Check for matching class names within the search radius.
	//
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		flMaxDist2 = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
	}

	CBaseEntity *pSearch = NULL;
	while ((pSearch = gEntList.FindEntityByName( pSearch, szName, pSearchingEntity, pActivator, pCaller )) != NULL)
	{
		if ( !pSearch->edict() )
			continue;

		float flDist2 = (pSearch->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			if ( pFilter && !pFilter->ShouldFindEntity( pSearch ) )
				continue;

			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}

	return pEntity;
}



//-----------------------------------------------------------------------------
// Purpose: Finds the first entity by name within a radius
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for.
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByNameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller, IEntityFindFilter *pFilter )
{
	//
	// Check for matching class names within the search radius.
	//
	CBaseEntity *pEntity = pStartEntity;
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		return gEntList.FindEntityByName( pEntity, szName, pSearchingEntity, pActivator, pCaller );
	}

	while ((pEntity = gEntList.FindEntityByName( pEntity, szName, pSearchingEntity, pActivator, pCaller )) != NULL)
	{
		if ( !pEntity->edict() )
			continue;

		float flDist2 = (pEntity->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			if ( pFilter && !pFilter->ShouldFindEntity( pEntity ) )
				continue;

			return pEntity;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by class name withing given search radius.
// Input  : szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius, IEntityFindFilter *pFilter )
{
	CBaseEntity *pEntity = NULL;

	//
	// Check for matching class names within the search radius.
	//
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		flMaxDist2 = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
	}

	CBaseEntity *pSearch = NULL;
	while ((pSearch = gEntList.FindEntityByClassname( pSearch, szName )) != NULL)
	{
		if ( !pSearch->edict() )
			continue;

		float flDist2 = (pSearch->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			if ( pFilter && !pFilter->ShouldFindEntity( pSearch ) )
				continue;

			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}

	return pEntity;
}



//-----------------------------------------------------------------------------
// Purpose: Finds the first entity within radius distance by class name.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, IEntityFindFilter *pFilter )
{
	//
	// Check for matching class names within the search radius.
	//
	CBaseEntity *pEntity = pStartEntity;
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		return gEntList.FindEntityByClassname( pEntity, szName );
	}

	while ((pEntity = gEntList.FindEntityByClassname( pEntity, szName )) != NULL)
	{
		if ( !pEntity->edict() )
			continue;

		float flDist2 = (pEntity->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			if ( pFilter && !pFilter->ShouldFindEntity( pEntity ) )
				continue;

			return pEntity;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds the first entity within an extent by class name.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity class name, ie "info_target".
//			vecMins - Search mins.
//			vecMaxs - Search maxs.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecMins, const Vector &vecMaxs, IEntityFindFilter *pFilter )
{
	//
	// Check for matching class names within the search radius.
	//
	CBaseEntity *pEntity = pStartEntity;

	while ((pEntity = gEntList.FindEntityByClassname( pEntity, szName )) != NULL)
	{
		if ( !pEntity->edict() && !pEntity->IsEFlagSet( EFL_SERVER_ONLY ) )
			continue;

		// check if the aabb intersects the search aabb.
		Vector entMins, entMaxs;
		pEntity->CollisionProp()->WorldSpaceAABB( &entMins, &entMaxs );
		if ( IsBoxIntersectingBox( vecMins, vecMaxs, entMins, entMaxs ) )
		{
			if ( pFilter && !pFilter->ShouldFindEntity( pEntity ) )
				continue;

			return pEntity;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds an entity by target name or class name.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityGeneric( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	CBaseEntity *pEntity = NULL;

	pEntity = gEntList.FindEntityByName( pStartEntity, szName, pSearchingEntity, pActivator, pCaller );
	if (!pEntity)
	{
		pEntity = gEntList.FindEntityByClassname( pStartEntity, szName );
	}

	return pEntity;
} 


//-----------------------------------------------------------------------------
// Purpose: Finds the first entity by target name or class name within a radius
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityGenericWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	CBaseEntity *pEntity = NULL;

	pEntity = gEntList.FindEntityByNameWithin( pStartEntity, szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller );
	if (!pEntity)
	{
		pEntity = gEntList.FindEntityByClassnameWithin( pStartEntity, szName, vecSrc, flRadius );
	}

	return pEntity;
} 

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by target name or class name within a radius.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	CBaseEntity *pEntity = NULL;

	pEntity = gEntList.FindEntityByNameNearest( szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller );
	if (!pEntity)
	{
		pEntity = gEntList.FindEntityByClassnameNearest( szName, vecSrc, flRadius );
	}

	return pEntity;
} 


//-----------------------------------------------------------------------------
// Purpose: Find the nearest entity along the facing direction from the given origin
//			within the angular threshold (ignores worldspawn) with the
//			given classname.
// Input  : origin - 
//			facing - 
//			threshold - 
//			classname - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, char *classname)
{
	float bestDot = threshold;
	CBaseEntity *best_ent = NULL;

	const CEntInfo *pInfo = FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		// FIXME: why is this skipping pointsize entities?
		if (ent->IsPointSized() )
			continue;

		// Make vector to entity
		Vector	to_ent = (ent->GetAbsOrigin() - origin);

		VectorNormalize( to_ent );
		float dot = DotProduct (facing , to_ent );
		if (dot > bestDot) 
		{
			if (FClassnameIs(ent,classname))
			{
				// Ignore if worldspawn
				if (!FClassnameIs( ent, "worldspawn" )  && !FClassnameIs( ent, "soundent")) 
				{
					bestDot	= dot;
					best_ent = ent;
				}
			}
		}
	}
	return best_ent;
}


//-----------------------------------------------------------------------------
// Purpose: Find the nearest entity along the facing direction from the given origin
//			within the angular threshold (ignores worldspawn)
// Input  : origin - 
//			facing - 
//			threshold - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityNearestFacing( const Vector &origin, const Vector &facing, float threshold)
{
	float bestDot = threshold;
	CBaseEntity *best_ent = NULL;

	const CEntInfo *pInfo = FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		// Ignore logical entities
		if (!ent->edict())
			continue;

		// Make vector to entity
		Vector	to_ent = ent->WorldSpaceCenter() - origin;
		VectorNormalize(to_ent);

		float dot = DotProduct( facing, to_ent );
		if (dot <= bestDot) 
			continue;

		// Ignore if worldspawn
		if (!FStrEq( STRING(ent->m_iClassname), "worldspawn")  && !FStrEq( STRING(ent->m_iClassname), "soundent")) 
		{
			bestDot	= dot;
			best_ent = ent;
		}
	}
	return best_ent;
}


void CGlobalEntityList::OnAddEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
	int i = handle.GetEntryIndex();

	// record current list details
	m_iNumEnts++;
	if ( i > m_iHighestEnt )
		m_iHighestEnt = i;

	// If it's a CBaseEntity, notify the listeners.
	CBaseEntity *pBaseEnt = static_cast<IServerUnknown*>(pEnt)->GetBaseEntity();
	if ( pBaseEnt->edict() )
		m_iNumEdicts++;
	
	// NOTE: Must be a CBaseEntity on server
	Assert( pBaseEnt );
	//DevMsg(2,"Created %s\n", pBaseEnt->GetClassname() );
	for ( i = m_entityListeners.Count()-1; i >= 0; i-- )
	{
		m_entityListeners[i]->OnEntityCreated( pBaseEnt );
	}
}


void CGlobalEntityList::OnRemoveEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
#ifdef DBGFLAG_ASSERT
	if ( !g_fInCleanupDelete )
	{
		int i;
		for ( i = 0; i < g_DeleteList.Count(); i++ )
		{
			if ( g_DeleteList[i]->GetEntityHandle() == pEnt )
			{
				g_DeleteList.FastRemove( i );
				Msg( "ERROR: Entity being destroyed but previously threaded on g_DeleteList\n" );
				break;
			}
		}
	}
#endif

	CBaseEntity *pBaseEnt = static_cast<IServerUnknown*>(pEnt)->GetBaseEntity();
	if ( pBaseEnt->edict() )
		m_iNumEdicts--;

	m_iNumEnts--;
}

void CGlobalEntityList::NotifyCreateEntity( CBaseEntity *pEnt )
{
	if ( !pEnt )
		return;

	//DevMsg(2,"Deleted %s\n", pBaseEnt->GetClassname() );
	for ( int i = m_entityListeners.Count()-1; i >= 0; i-- )
	{
		m_entityListeners[i]->OnEntityCreated( pEnt );
	}
}

void CGlobalEntityList::NotifySpawn( CBaseEntity *pEnt )
{
	if ( !pEnt )
		return;

	//DevMsg(2,"Deleted %s\n", pBaseEnt->GetClassname() );
	for ( int i = m_entityListeners.Count()-1; i >= 0; i-- )
	{
		m_entityListeners[i]->OnEntitySpawned( pEnt );
	}
}

// NOTE: This doesn't happen in OnRemoveEntity() specifically because 
// listeners may want to reference the object as it's being deleted
// OnRemoveEntity isn't called until the destructor and all data is invalid.
void CGlobalEntityList::NotifyRemoveEntity( CBaseHandle hEnt )
{
	CBaseEntity *pBaseEnt = GetBaseEntity( hEnt );
	if ( !pBaseEnt )
		return;

	//DevMsg(2,"Deleted %s\n", pBaseEnt->GetClassname() );
	for ( int i = m_entityListeners.Count()-1; i >= 0; i-- )
	{
		m_entityListeners[i]->OnEntityDeleted( pBaseEnt );
	}
}


//-----------------------------------------------------------------------------
// NOTIFY LIST
// 
// Allows entities to get events fired when another entity changes
//-----------------------------------------------------------------------------
struct entitynotify_t
{
	CBaseEntity	*pNotify;
	CBaseEntity	*pWatched;
};
class CNotifyList : public INotify, public IEntityListener
{
public:
	// INotify
	void AddEntity( CBaseEntity *pNotify, CBaseEntity *pWatched );
	void RemoveEntity( CBaseEntity *pNotify, CBaseEntity *pWatched );
	void ReportNamedEvent( CBaseEntity *pEntity, const char *pEventName );
	void ClearEntity( CBaseEntity *pNotify );
	void ReportSystemEvent( CBaseEntity *pEntity, notify_system_event_t eventType, const notify_system_event_params_t &params );

	// IEntityListener
	virtual void OnEntityCreated( CBaseEntity *pEntity );
	virtual void OnEntityDeleted( CBaseEntity *pEntity );

	// Called from CEntityListSystem
	void LevelInitPreEntity();
	void LevelShutdownPreEntity();

private:
	CUtlVector<entitynotify_t>	m_notifyList;
};

void CNotifyList::AddEntity( CBaseEntity *pNotify, CBaseEntity *pWatched )
{
	// OPTIMIZE: Also flag pNotify for faster "RemoveAllNotify" ?
	pWatched->AddEFlags( EFL_NOTIFY );
	int index = m_notifyList.AddToTail();
	entitynotify_t &notify = m_notifyList[index];
	notify.pNotify = pNotify;
	notify.pWatched = pWatched;
}

// Remove noitfication for an entity
void CNotifyList::RemoveEntity( CBaseEntity *pNotify, CBaseEntity *pWatched )
{
	for ( int i = m_notifyList.Count(); --i >= 0; )
	{
		if ( m_notifyList[i].pNotify == pNotify && m_notifyList[i].pWatched == pWatched)
		{
			m_notifyList.FastRemove(i);
		}
	}
}


void CNotifyList::ReportNamedEvent( CBaseEntity *pEntity, const char *pInputName )
{
	variant_t emptyVariant;

	if ( !pEntity->IsEFlagSet(EFL_NOTIFY) )
		return;

	for ( int i = 0; i < m_notifyList.Count(); i++ )
	{
		if ( m_notifyList[i].pWatched == pEntity )
		{
			m_notifyList[i].pNotify->AcceptInput( pInputName, pEntity, pEntity, emptyVariant, 0 );
		}
	}
}

void CNotifyList::LevelInitPreEntity()
{
	gEntList.AddListenerEntity( this );
}

void CNotifyList::LevelShutdownPreEntity( void )
{
	gEntList.RemoveListenerEntity( this );
	m_notifyList.Purge();
}

void CNotifyList::OnEntityCreated( CBaseEntity *pEntity )
{
}

void CNotifyList::OnEntityDeleted( CBaseEntity *pEntity )
{
	ReportDestroyEvent( pEntity );
	ClearEntity( pEntity );
}


// UNDONE: Slow linear search?
void CNotifyList::ClearEntity( CBaseEntity *pNotify )
{
	for ( int i = m_notifyList.Count(); --i >= 0; )
	{
		if ( m_notifyList[i].pNotify == pNotify || m_notifyList[i].pWatched == pNotify)
		{
			m_notifyList.FastRemove(i);
		}
	}
}

void CNotifyList::ReportSystemEvent( CBaseEntity *pEntity, notify_system_event_t eventType, const notify_system_event_params_t &params )
{
	if ( !pEntity->IsEFlagSet(EFL_NOTIFY) )
		return;

	for ( int i = 0; i < m_notifyList.Count(); i++ )
	{
		if ( m_notifyList[i].pWatched == pEntity )
		{
			m_notifyList[i].pNotify->NotifySystemEvent( pEntity, eventType, params );
		}
	}
}

static CNotifyList g_NotifyList;
INotify *g_pNotify = &g_NotifyList;

class CEntityTouchManager : public IEntityListener
{
public:
	// called by CEntityListSystem
	void LevelInitPreEntity() 
	{ 
		gEntList.AddListenerEntity( this );
		Clear(); 
	}
	void LevelShutdownPostEntity() 
	{ 
		gEntList.RemoveListenerEntity( this );
		Clear(); 
	}
	void FrameUpdatePostEntityThink();

	void Clear()
	{
		m_updateList.Purge();
	}
	
	// IEntityListener
	virtual void OnEntityCreated( CBaseEntity *pEntity ) {}
	virtual void OnEntityDeleted( CBaseEntity *pEntity )
	{
		if ( !pEntity->GetCheckUntouch() )
			return;
		int index = m_updateList.Find( pEntity );
		if ( m_updateList.IsValidIndex(index) )
		{
			m_updateList.FastRemove( index );
		}
	}
	void AddEntity( CBaseEntity *pEntity )
	{
		if ( pEntity->IsMarkedForDeletion() )
			return;
		m_updateList.AddToTail( pEntity );
	}

private:
	CUtlVector<CBaseEntity *>	m_updateList;
};

static CEntityTouchManager g_TouchManager;

void EntityTouch_Add( CBaseEntity *pEntity )
{
	g_TouchManager.AddEntity( pEntity );
}


void CEntityTouchManager::FrameUpdatePostEntityThink()
{
	VPROF( "CEntityTouchManager::FrameUpdatePostEntityThink" );
	// Loop through all entities again, checking their untouch if flagged to do so
	
	int count = m_updateList.Count();
	if ( count )
	{
		// copy off the list
		CBaseEntity **ents = (CBaseEntity **)stackalloc( sizeof(CBaseEntity *) * count );
		memcpy( ents, m_updateList.Base(), sizeof(CBaseEntity *) * count );
		// clear it
		m_updateList.RemoveAll();
		
		// now update those ents
		for ( int i = 0; i < count; i++ )
		{
			//Assert( ents[i]->GetCheckUntouch() );
			if ( ents[i]->GetCheckUntouch() )
			{
				ents[i]->PhysicsCheckForEntityUntouch();
			}
		}
		stackfree( ents );
	}
}

class CRespawnEntitiesFilter : public IMapEntityFilter
{
public:
	virtual bool ShouldCreateEntity( const char *pClassname )
	{
		// Create everything but the world
		return Q_stricmp( pClassname, "worldspawn" ) != 0;
	}

	virtual CBaseEntity* CreateNextEntity( const char *pClassname )
	{
		return CreateEntityByName( pClassname );
	}
};

// One hook to rule them all...
// Since most of the little list managers in here only need one or two of the game
// system callbacks, this hook is a game system that passes them the appropriate callbacks
class CEntityListSystem : public CAutoGameSystemPerFrame
{
public:
	CEntityListSystem( char const *name ) : CAutoGameSystemPerFrame( name )
	{
		m_bRespawnAllEntities = false;
	}
	void LevelInitPreEntity()
	{
		g_NotifyList.LevelInitPreEntity();
		g_TouchManager.LevelInitPreEntity();
		g_AimManager.LevelInitPreEntity();
		g_SimThinkManager.LevelInitPreEntity();
#ifdef HL2_DLL
		OverrideMoveCache_LevelInitPreEntity();
#endif	// HL2_DLL
	}
	void LevelShutdownPreEntity()
	{
		g_NotifyList.LevelShutdownPreEntity();
	}
	void LevelShutdownPostEntity()
	{
		g_TouchManager.LevelShutdownPostEntity();
		g_AimManager.LevelShutdownPostEntity();
		g_SimThinkManager.LevelShutdownPostEntity();
#ifdef HL2_DLL
		OverrideMoveCache_LevelShutdownPostEntity();
#endif // HL2_DLL
		CBaseEntityClassList *pClassList = s_pClassLists;
		while ( pClassList )
		{
			pClassList->LevelShutdownPostEntity();
			pClassList = pClassList->m_pNextClassList;
		}
	}

	void FrameUpdatePostEntityThink()
	{
		g_TouchManager.FrameUpdatePostEntityThink();

		if ( m_bRespawnAllEntities )
		{
			m_bRespawnAllEntities = false;

			// Don't change globalstate owing to deletion here
			GlobalEntity_EnableStateUpdates( false );

			// Remove all entities
			int nPlayerIndex = -1;
			CBaseEntity *pEnt = gEntList.FirstEnt();
			while ( pEnt )
			{
				CBaseEntity *pNextEnt = gEntList.NextEnt( pEnt );
				if ( pEnt->IsPlayer() )
				{
					nPlayerIndex = pEnt->entindex();
				}
				if ( !pEnt->IsEFlagSet( EFL_KEEP_ON_RECREATE_ENTITIES ) )
				{
					UTIL_Remove( pEnt );
				}
				pEnt = pNextEnt;
			}
			
			gEntList.CleanupDeleteList();

			GlobalEntity_EnableStateUpdates( true );

			// Allows us to immediately re-use the edict indices we just freed to avoid edict overflow
			engine->AllowImmediateEdictReuse();

			// Reset node counter used during load
			CNodeEnt::m_nNodeCount = 0;

			CRespawnEntitiesFilter filter;
			MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );

			// Allocate a CBasePlayer for pev, and call spawn
			if ( nPlayerIndex >= 0 )
			{
				edict_t *pEdict = engine->PEntityOfEntIndex( nPlayerIndex );
				ClientPutInServer( pEdict, "unnamed" );
				ClientActive( pEdict, false );

				CBasePlayer *pPlayer = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
				SceneManager_ClientActive( pPlayer );
			}
		}
	}

	bool m_bRespawnAllEntities;
};

static CEntityListSystem g_EntityListSystem( "CEntityListSystem" );

//-----------------------------------------------------------------------------
// Respawns all entities in the level
//-----------------------------------------------------------------------------
void RespawnEntities()
{
	g_EntityListSystem.m_bRespawnAllEntities = true;
}

static ConCommand restart_entities( "respawn_entities", RespawnEntities, "Respawn all the entities in the map.", FCVAR_CHEAT | FCVAR_SPONLY );

class CSortedEntityList
{
public:
	CSortedEntityList() : m_sortedList(), m_emptyCount(0) {}

	typedef CBaseEntity *ENTITYPTR;
	class CEntityReportLess
	{
	public:
		bool Less( const ENTITYPTR &src1, const ENTITYPTR &src2, void *pCtx )
		{
			if ( stricmp( src1->GetClassname(), src2->GetClassname() ) < 0 )
				return true;
	
			return false;
		}
	};

	void AddEntityToList( CBaseEntity *pEntity )
	{
		if ( !pEntity )
		{
			m_emptyCount++;
		}
		else
		{
			m_sortedList.Insert( pEntity );
		}
	}
	void ReportEntityList()
	{
		const char *pLastClass = "";
		int count = 0;
		int edicts = 0;
		for ( int i = 0; i < m_sortedList.Count(); i++ )
		{
			CBaseEntity *pEntity = m_sortedList[i];
			if ( !pEntity )
				continue;

			if ( pEntity->edict() )
				edicts++;

			const char *pClassname = pEntity->GetClassname();
			if ( !FStrEq( pClassname, pLastClass ) )
			{
				if ( count )
				{
					Msg("Class: %s (%d)\n", pLastClass, count );
				}

				pLastClass = pClassname;
				count = 1;
			}
			else
				count++;
		}
		if ( pLastClass[0] != 0 && count )
		{
			Msg("Class: %s (%d)\n", pLastClass, count );
		}
		if ( m_sortedList.Count() )
		{
			Msg("Total %d entities (%d empty, %d edicts)\n", m_sortedList.Count(), m_emptyCount, edicts );
		}
	}
private:
	CUtlSortVector< CBaseEntity *, CEntityReportLess > m_sortedList;
	int		m_emptyCount;
};



CON_COMMAND(report_entities, "Lists all entities")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CSortedEntityList list;
	CBaseEntity *pEntity = gEntList.FirstEnt();
	while ( pEntity )
	{
		list.AddEntityToList( pEntity );
		pEntity = gEntList.NextEnt( pEntity );
	}
	list.ReportEntityList();
}


CON_COMMAND(report_touchlinks, "Lists all touchlinks")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CSortedEntityList list;
	CBaseEntity *pEntity = gEntList.FirstEnt();
	const char *pClassname = NULL;
	if ( args.ArgC() > 1 )
	{
		pClassname = args.Arg(1);
	}
	while ( pEntity )
	{
		if ( !pClassname || FClassnameIs(pEntity, pClassname) )
		{
			touchlink_t *root = ( touchlink_t * )pEntity->GetDataObject( TOUCHLINK );
			if ( root )
			{
				touchlink_t *link = root->nextLink;
				while ( link != root )
				{
					list.AddEntityToList( link->entityTouched );
					link = link->nextLink;
				}
			}
		}
		pEntity = gEntList.NextEnt( pEntity );
	}
	list.ReportEntityList();
}

CON_COMMAND(report_simthinklist, "Lists all simulating/thinking entities")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBaseEntity *pTmp[NUM_ENT_ENTRIES];
	int count = SimThink_ListCopy( pTmp, ARRAYSIZE(pTmp) );

	CSortedEntityList list;
	for ( int i = 0; i < count; i++ )
	{
		if ( !pTmp[i] )
			continue;

		list.AddEntityToList( pTmp[i] );
	}
	list.ReportEntityList();
}

