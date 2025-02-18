//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//===========================================================================//

//-----------------------------------------------------------------------------
// Purpose: a global list of all the entities in the game.  All iteration through
//			entities is done through this object.
//-----------------------------------------------------------------------------
#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

// Create interface
static CClientEntityList s_EntityList;
CBaseEntityList *g_pEntityList = &s_EntityList;

// Expose list to engine
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientEntityList, IClientEntityList, VCLIENTENTITYLIST_INTERFACE_VERSION, s_EntityList );

// Store local pointer to interface for rest of client .dll only 
//  (CClientEntityList instead of IClientEntityList )
CClientEntityList *cl_entitylist = &s_EntityList; 


bool PVSNotifierMap_LessFunc( IClientUnknown* const &a, IClientUnknown* const &b )
{
	return a < b;
}

								 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CClientEntityList::CClientEntityList( void ) : 
	m_PVSNotifierMap( 0, 0, PVSNotifierMap_LessFunc )
{
	m_iMaxUsedServerIndex = -1;
	m_iMaxServerEnts = 0;
	Release();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CClientEntityList::~CClientEntityList( void )
{
	Release();
}

//-----------------------------------------------------------------------------
// Purpose: Clears all entity lists and releases entities
//-----------------------------------------------------------------------------
void CClientEntityList::Release( void )
{
	// Free all the entities.
	ClientEntityHandle_t iter = FirstHandle();
	while( iter != InvalidHandle() )
	{
		// Try to call release on anything we can.
		IClientNetworkable *pNet = GetClientNetworkableFromHandle( iter );
		if ( pNet )
		{
			pNet->Release();
		}
		else
		{
			// Try to call release on anything we can.
			IClientThinkable *pThinkable = GetClientThinkableFromHandle( iter );
			if ( pThinkable )
			{
				pThinkable->Release();
			}
		}
		RemoveEntity( iter );

		iter = FirstHandle();
	}

	m_iNumServerEnts = 0;
	m_iMaxServerEnts = 0;
	m_iNumClientNonNetworkable = 0;
	m_iMaxUsedServerIndex = -1;
}

IClientNetworkable* CClientEntityList::GetClientNetworkable( int entnum )
{
	Assert( entnum >= 0 );
	Assert( entnum < MAX_EDICTS );
	if ( entnum < 0 || entnum >= MAX_EDICTS )
	{
		return NULL;
	}
	return m_EntityCacheInfo[entnum].m_pNetworkable;
}


IClientEntity* CClientEntityList::GetClientEntity( int entnum )
{
	IClientUnknown *pEnt = GetListedEntity( entnum );
	return pEnt ? pEnt->GetIClientEntity() : 0;
}


int CClientEntityList::NumberOfEntities( bool bIncludeNonNetworkable )
{
	if ( bIncludeNonNetworkable == true )
		 return m_iNumServerEnts + m_iNumClientNonNetworkable;

	return m_iNumServerEnts;
}


void CClientEntityList::SetMaxEntities( int maxents )
{
	m_iMaxServerEnts = maxents;
}


int CClientEntityList::GetMaxEntities( void )
{
	return m_iMaxServerEnts;
}


//-----------------------------------------------------------------------------
// Convenience methods to convert between entindex + ClientEntityHandle_t
//-----------------------------------------------------------------------------
int CClientEntityList::HandleToEntIndex( ClientEntityHandle_t handle )
{
	if ( handle == INVALID_EHANDLE )
		return -1;
	C_BaseEntity *pEnt = GetBaseEntityFromHandle( handle );
	return pEnt ? pEnt->entindex() : -1; 
}


//-----------------------------------------------------------------------------
// Purpose: Because m_iNumServerEnts != last index
// Output : int
//-----------------------------------------------------------------------------
int CClientEntityList::GetHighestEntityIndex( void )
{
	return m_iMaxUsedServerIndex;
}

void CClientEntityList::RecomputeHighestEntityUsed( void )
{
	m_iMaxUsedServerIndex = -1;

	// Walk backward looking for first valid index
	int i;
	for ( i = MAX_EDICTS - 1; i >= 0; i-- )
	{
		if ( GetListedEntity( i ) != NULL )
		{
			m_iMaxUsedServerIndex = i;
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Add a raw C_BaseEntity to the entity list.
// Input  : index - 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------

C_BaseEntity* CClientEntityList::GetBaseEntity( int entnum )
{
	IClientUnknown *pEnt = GetListedEntity( entnum );
	return pEnt ? pEnt->GetBaseEntity() : 0;
}


ICollideable* CClientEntityList::GetCollideable( int entnum )
{
	IClientUnknown *pEnt = GetListedEntity( entnum );
	return pEnt ? pEnt->GetCollideable() : 0;
}


IClientNetworkable* CClientEntityList::GetClientNetworkableFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetClientNetworkable() : 0;
}


IClientEntity* CClientEntityList::GetClientEntityFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetIClientEntity() : 0;
}


IClientRenderable* CClientEntityList::GetClientRenderableFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetClientRenderable() : 0;
}


C_BaseEntity* CClientEntityList::GetBaseEntityFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetBaseEntity() : 0;
}


ICollideable* CClientEntityList::GetCollideableFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetCollideable() : 0;
}


IClientThinkable* CClientEntityList::GetClientThinkableFromHandle( ClientEntityHandle_t hEnt )
{
	IClientUnknown *pEnt = GetClientUnknownFromHandle( hEnt );
	return pEnt ? pEnt->GetClientThinkable() : 0;
}


void CClientEntityList::AddPVSNotifier( IClientUnknown *pUnknown )
{
	IClientRenderable *pRen = pUnknown->GetClientRenderable();
	if ( pRen )
	{
		IPVSNotify *pNotify = pRen->GetPVSNotifyInterface();
		if ( pNotify )
		{
			unsigned short index = m_PVSNotifyInfos.AddToTail();
			CPVSNotifyInfo *pInfo = &m_PVSNotifyInfos[index];
			pInfo->m_pNotify = pNotify;
			pInfo->m_pRenderable = pRen;
			pInfo->m_InPVSStatus = 0;
			pInfo->m_PVSNotifiersLink = index;

			m_PVSNotifierMap.Insert( pUnknown, index );
		}
	}
}


void CClientEntityList::RemovePVSNotifier( IClientUnknown *pUnknown )
{
	IClientRenderable *pRenderable = pUnknown->GetClientRenderable();
	if ( pRenderable )
	{
		IPVSNotify *pNotify = pRenderable->GetPVSNotifyInterface();
		if ( pNotify )
		{
			unsigned short index = m_PVSNotifierMap.Find( pUnknown );
			if ( !m_PVSNotifierMap.IsValidIndex( index ) )
			{
				Warning( "PVS notifier not in m_PVSNotifierMap\n" );
				Assert( false );
				return;
			}

			unsigned short indexIntoPVSNotifyInfos = m_PVSNotifierMap[index];
			
			Assert( m_PVSNotifyInfos[indexIntoPVSNotifyInfos].m_pNotify == pNotify );
			Assert( m_PVSNotifyInfos[indexIntoPVSNotifyInfos].m_pRenderable == pRenderable );
			
			m_PVSNotifyInfos.Remove( indexIntoPVSNotifyInfos );
			m_PVSNotifierMap.RemoveAt( index );
			return;
		}
	}

	// If it didn't report itself as a notifier, let's hope it's not in the notifier list now
	// (which would mean that it reported itself as a notifier earlier, but not now).
#ifdef _DEBUG
	unsigned short index = m_PVSNotifierMap.Find( pUnknown );
	Assert( !m_PVSNotifierMap.IsValidIndex( index ) );
#endif
}

void CClientEntityList::AddListenerEntity( IClientEntityListener *pListener )
{
	if ( m_entityListeners.Find( pListener ) >= 0 )
	{
		AssertMsg( 0, "Can't add listeners multiple times\n" );
		return;
	}
	m_entityListeners.AddToTail( pListener );
}

void CClientEntityList::RemoveListenerEntity( IClientEntityListener *pListener )
{
	m_entityListeners.FindAndRemove( pListener );
}

void CClientEntityList::OnAddEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
	int entnum = handle.GetEntryIndex();
	EntityCacheInfo_t *pCache = &m_EntityCacheInfo[entnum];

	if ( entnum >= 0 && entnum < MAX_EDICTS )
	{
		// Update our counters.
		m_iNumServerEnts++;
		if ( entnum > m_iMaxUsedServerIndex )
		{
			m_iMaxUsedServerIndex = entnum;
		}


		// Cache its networkable pointer.
		Assert( dynamic_cast< IClientUnknown* >( pEnt ) );
		Assert( ((IClientUnknown*)pEnt)->GetClientNetworkable() ); // Server entities should all be networkable.
		pCache->m_pNetworkable = ((IClientUnknown*)pEnt)->GetClientNetworkable();
	}

	IClientUnknown *pUnknown = (IClientUnknown*)pEnt;

	// If this thing wants PVS notifications, hook it up.
	AddPVSNotifier( pUnknown );

	// Store it in a special list for fast iteration if it's a C_BaseEntity.
	C_BaseEntity *pBaseEntity = pUnknown->GetBaseEntity();
	if ( pBaseEntity )
	{
		pCache->m_BaseEntitiesIndex = m_BaseEntities.AddToTail( pBaseEntity );

		if ( pBaseEntity->ObjectCaps() & FCAP_SAVE_NON_NETWORKABLE  )
		{
			 m_iNumClientNonNetworkable++;
		}

		//DevMsg(2,"Created %s\n", pBaseEnt->GetClassname() );
		for ( int i = m_entityListeners.Count()-1; i >= 0; i-- )
		{
			m_entityListeners[i]->OnEntityCreated( pBaseEntity );
		}
	}
	else
	{
		pCache->m_BaseEntitiesIndex = m_BaseEntities.InvalidIndex();
	}


}


void CClientEntityList::OnRemoveEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
	int entnum = handle.GetEntryIndex();
	EntityCacheInfo_t *pCache = &m_EntityCacheInfo[entnum];

	if ( entnum >= 0 && entnum < MAX_EDICTS )
	{
		// This is a networkable ent. Clear out our cache info for it.
		pCache->m_pNetworkable = NULL;
		m_iNumServerEnts--;

		if ( entnum >= m_iMaxUsedServerIndex )
		{
			RecomputeHighestEntityUsed();
		}
	}


	IClientUnknown *pUnknown = (IClientUnknown*)pEnt;

	// If this is a PVS notifier, remove it.
	RemovePVSNotifier( pUnknown );

	C_BaseEntity *pBaseEntity = pUnknown->GetBaseEntity();


	if ( pBaseEntity )
	{
		if ( pBaseEntity->ObjectCaps() & FCAP_SAVE_NON_NETWORKABLE )
		{
			 m_iNumClientNonNetworkable--;
		}

		//DevMsg(2,"Deleted %s\n", pBaseEnt->GetClassname() );
		for ( int i = m_entityListeners.Count()-1; i >= 0; i-- )
		{
			m_entityListeners[i]->OnEntityDeleted( pBaseEntity );
		}
	}

	if ( pCache->m_BaseEntitiesIndex != m_BaseEntities.InvalidIndex() )
		m_BaseEntities.Remove( pCache->m_BaseEntitiesIndex );

	pCache->m_BaseEntitiesIndex = m_BaseEntities.InvalidIndex();
}


// Use this to iterate over all the C_BaseEntities.
C_BaseEntity* CClientEntityList::FirstBaseEntity() const
{
	const CEntInfo *pList = FirstEntInfo();
	while ( pList )
	{
		if ( pList->m_pEntity )
		{
			IClientUnknown *pUnk = static_cast<IClientUnknown*>( pList->m_pEntity );
			C_BaseEntity *pRet = pUnk->GetBaseEntity();
			if ( pRet )
				return pRet;
		}
		pList = pList->m_pNext;
	}

	return NULL;

}

C_BaseEntity* CClientEntityList::NextBaseEntity( C_BaseEntity *pEnt ) const
{
	if ( pEnt == NULL )
		return FirstBaseEntity();

	// Run through the list until we get a C_BaseEntity.
	const CEntInfo *pList = GetEntInfoPtr( pEnt->GetRefEHandle() );
	if ( pList )
	{
		pList = NextEntInfo(pList);
	}

	while ( pList )
	{
		if ( pList->m_pEntity )
		{
			IClientUnknown *pUnk = static_cast<IClientUnknown*>( pList->m_pEntity );
			C_BaseEntity *pRet = pUnk->GetBaseEntity();
			if ( pRet )
				return pRet;
		}
		pList = pList->m_pNext;
	}
	
	return NULL; 
}



// -------------------------------------------------------------------------------------------------- //
// C_AllBaseEntityIterator
// -------------------------------------------------------------------------------------------------- //
C_AllBaseEntityIterator::C_AllBaseEntityIterator()
{
	Restart();
}


void C_AllBaseEntityIterator::Restart()
{
	m_CurBaseEntity = ClientEntityList().m_BaseEntities.Head();
}

	
C_BaseEntity* C_AllBaseEntityIterator::Next()
{
	if ( m_CurBaseEntity == ClientEntityList().m_BaseEntities.InvalidIndex() )
		return NULL;

	C_BaseEntity *pRet = ClientEntityList().m_BaseEntities[m_CurBaseEntity];
	m_CurBaseEntity = ClientEntityList().m_BaseEntities.Next( m_CurBaseEntity );
	return pRet;
}


// -------------------------------------------------------------------------------------------------- //
// C_BaseEntityIterator
// -------------------------------------------------------------------------------------------------- //
C_BaseEntityIterator::C_BaseEntityIterator()
{
	Restart();
}

void C_BaseEntityIterator::Restart()
{
	m_CurBaseEntity = ClientEntityList().m_BaseEntities.Head();
}

C_BaseEntity* C_BaseEntityIterator::Next()
{
	// Skip dormant entities
	while ( m_CurBaseEntity != ClientEntityList().m_BaseEntities.InvalidIndex() )
	{
		C_BaseEntity *pRet = ClientEntityList().m_BaseEntities[m_CurBaseEntity];
		m_CurBaseEntity = ClientEntityList().m_BaseEntities.Next( m_CurBaseEntity );

		if (!pRet->IsDormant())
			return pRet;
	}

	return NULL;
}
