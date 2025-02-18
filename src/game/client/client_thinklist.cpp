//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CClientThinkList g_ClientThinkList;


CClientThinkList::CClientThinkList()
{
}


CClientThinkList::~CClientThinkList()
{
}


//-----------------------------------------------------------------------------
// Methods of IGameSystem
//-----------------------------------------------------------------------------
bool CClientThinkList::Init()
{
	m_nIterEnum = 0;
	m_bInThinkLoop = false;
	return true;
}


void CClientThinkList::Shutdown()
{
}


void CClientThinkList::LevelInitPreEntity()
{
	m_nIterEnum = 0;
}

void CClientThinkList::LevelShutdownPreEntity()
{
}


void CClientThinkList::LevelShutdownPostEntity()
{
}


void CClientThinkList::PreRender()
{
}


void CClientThinkList::Update( float frametime )
{
}


//-----------------------------------------------------------------------------
// Sets the client think
//-----------------------------------------------------------------------------
void CClientThinkList::SetNextClientThink( ClientThinkHandle_t hThink, float flNextTime )
{
	if ( hThink == INVALID_THINK_HANDLE )
		return;

	if ( m_bInThinkLoop )
	{
		// Queue up all changes
		int i = m_aChangeList.AddToTail();
		m_aChangeList[i].m_hEnt = INVALID_CLIENTENTITY_HANDLE;
		m_aChangeList[i].m_hThink = hThink;
		m_aChangeList[i].m_flNextTime = flNextTime;
		return;
	}

	if ( flNextTime == CLIENT_THINK_NEVER )
	{
		RemoveThinkable( hThink );
	}
	else
	{
		GetThinkEntry( hThink )->m_flNextClientThink = flNextTime;
	}
}

void CClientThinkList::SetNextClientThink( ClientEntityHandle_t hEnt, float flNextTime )
{
	if ( flNextTime == CLIENT_THINK_NEVER )
	{
		RemoveThinkable( hEnt );
		return;
	}

	IClientThinkable *pThink = ClientEntityList().GetClientThinkableFromHandle( hEnt );
	if ( !pThink )
		return;

	ClientThinkHandle_t hThink = pThink->GetThinkHandle();

	if ( m_bInThinkLoop )
	{
		// Queue up all changes
		int i = m_aChangeList.AddToTail();
		m_aChangeList[i].m_hEnt = hEnt;
		m_aChangeList[i].m_hThink = hThink;
		m_aChangeList[i].m_flNextTime = flNextTime;
		return;
	}

	// Add it to the list if it's not already in there.
	if ( hThink == INVALID_THINK_HANDLE )
	{
		hThink = (ClientThinkHandle_t)(uintp)m_ThinkEntries.AddToTail();
		pThink->SetThinkHandle( hThink );

		ThinkEntry_t *pEntry = GetThinkEntry( hThink );
		pEntry->m_hEnt = hEnt;
		pEntry->m_nIterEnum = -1;
		pEntry->m_flLastClientThink = 0.0f;
	}

	Assert( GetThinkEntry( hThink )->m_hEnt == hEnt );
	GetThinkEntry( hThink )->m_flNextClientThink = flNextTime;
}


//-----------------------------------------------------------------------------
// Removes the thinkable from the list
//-----------------------------------------------------------------------------
void CClientThinkList::RemoveThinkable( ClientThinkHandle_t hThink )
{
	if ( hThink == INVALID_THINK_HANDLE )
		return;

	if ( m_bInThinkLoop )
	{
		// Queue up all changes
		int i = m_aChangeList.AddToTail();
		m_aChangeList[i].m_hEnt = INVALID_CLIENTENTITY_HANDLE;
		m_aChangeList[i].m_hThink = hThink;
		m_aChangeList[i].m_flNextTime = CLIENT_THINK_NEVER;
		return;
	}

	ThinkEntry_t *pEntry = GetThinkEntry( hThink );
	IClientThinkable *pThink = ClientEntityList().GetClientThinkableFromHandle( pEntry->m_hEnt );
	if ( pThink )
	{
		pThink->SetThinkHandle( INVALID_THINK_HANDLE );
	}
	m_ThinkEntries.Remove( (uintp)hThink );
}


//-----------------------------------------------------------------------------
// Removes the thinkable from the list
//-----------------------------------------------------------------------------
void CClientThinkList::RemoveThinkable( ClientEntityHandle_t hEnt )
{
	IClientThinkable *pThink = ClientEntityList().GetClientThinkableFromHandle( hEnt );
	if ( pThink )
	{
		ClientThinkHandle_t hThink = pThink->GetThinkHandle();
		if ( hThink != INVALID_THINK_HANDLE )
		{
			Assert( GetThinkEntry( hThink )->m_hEnt == hEnt );
			RemoveThinkable( hThink );
		}
	}
}


//-----------------------------------------------------------------------------
// Performs the think function
//-----------------------------------------------------------------------------
void CClientThinkList::PerformThinkFunction( ThinkEntry_t *pEntry, float flCurtime )
{
	IClientThinkable *pThink = ClientEntityList().GetClientThinkableFromHandle( pEntry->m_hEnt );
	if ( !pThink )
	{
		RemoveThinkable( pEntry->m_hEnt );
		return;
	}

	if ( pEntry->m_flNextClientThink == CLIENT_THINK_ALWAYS )
	{
		// NOTE: The Think function here could call SetNextClientThink
		// which would cause it to be removed + readded into the list
		pThink->ClientThink();
	}
	else if ( pEntry->m_flNextClientThink == FLT_MAX )
	{
		// This is an entity that doesn't need to think again; remove it
		RemoveThinkable( pEntry->m_hEnt );
	}
	else
	{
		Assert( pEntry->m_flNextClientThink <= flCurtime );

		// Indicate we're not going to think again
		pEntry->m_flNextClientThink = FLT_MAX;

		// NOTE: The Think function here could call SetNextClientThink
		// which would cause it to be readded into the list
		pThink->ClientThink();
	}

	// Set this after the Think calls in case they look at LastClientThink
	pEntry->m_flLastClientThink = flCurtime;
}


//-----------------------------------------------------------------------------
// Add entity to frame think list
//-----------------------------------------------------------------------------
void CClientThinkList::AddEntityToFrameThinkList( ThinkEntry_t *pEntry, bool bAlwaysChain, int &nCount, ThinkEntry_t **ppFrameThinkList )
{
	// We may already have processed this owing to hierarchy rules
	if ( pEntry->m_nIterEnum == m_nIterEnum )
		return;

	// If we're not thinking this frame, we don't have to worry about thinking after our parents
	bool bThinkThisInterval = ( pEntry->m_flNextClientThink == CLIENT_THINK_ALWAYS ) ||
								( pEntry->m_flNextClientThink <= gpGlobals->curtime );

	// This logic makes it so that if a child thinks,
	// *all* hierarchical parents + grandparents will think first, even if some
	// of the parents don't need to think this frame
	if ( !bThinkThisInterval && !bAlwaysChain )
		return;

	// Respect hierarchy
	C_BaseEntity *pEntity = ClientEntityList().GetBaseEntityFromHandle( pEntry->m_hEnt );
	if ( pEntity )
	{
		C_BaseEntity *pParent = pEntity->GetMoveParent();
		if ( pParent && (pParent->GetThinkHandle() != INVALID_THINK_HANDLE) )
		{
			ThinkEntry_t *pParentEntry = GetThinkEntry( pParent->GetThinkHandle() );
			AddEntityToFrameThinkList( pParentEntry, true, nCount, ppFrameThinkList );
		}
	}

	if ( !bThinkThisInterval )
		return;

	// Add the entry into the list
	pEntry->m_nIterEnum = m_nIterEnum;
	ppFrameThinkList[nCount++] = pEntry;
}


//-----------------------------------------------------------------------------
// Think for all entities that need it
//-----------------------------------------------------------------------------
void CClientThinkList::PerformThinkFunctions()
{
	VPROF_("Client Thinks", 1, VPROF_BUDGETGROUP_CLIENT_SIM, false, BUDGETFLAG_CLIENT);

	int nMaxList = m_ThinkEntries.Count();
	if ( nMaxList == 0 )
		return;

	++m_nIterEnum;

	// Build a list of entities to think this frame, in order of hierarchy.
	// Do this because the list may be modified during the thinking and also to
	// prevent bad situations where an entity can think more than once in a frame.
	ThinkEntry_t **ppThinkEntryList = (ThinkEntry_t**)stackalloc( nMaxList * sizeof(ThinkEntry_t*) );
	int nThinkCount = 0;
	for ( unsigned short iCur=m_ThinkEntries.Head(); iCur != m_ThinkEntries.InvalidIndex(); iCur = m_ThinkEntries.Next( iCur ) )
	{
		AddEntityToFrameThinkList( &m_ThinkEntries[iCur], false, nThinkCount, ppThinkEntryList );
		Assert( nThinkCount <= nMaxList );
	}

	// While we're in the loop, no changes to the think list are allowed
	m_bInThinkLoop = true;

	// Perform thinks on all entities that need it
	int i;
	for ( i = 0; i < nThinkCount; ++i )
	{
		PerformThinkFunction( ppThinkEntryList[i], gpGlobals->curtime );		
	}

	m_bInThinkLoop = false;

	// Apply changes to the think list
	int nCount = m_aChangeList.Count();
	for ( i = 0; i < nCount; ++i )
	{
		ClientThinkHandle_t hThink = m_aChangeList[i].m_hThink;
		if ( hThink != INVALID_THINK_HANDLE )
		{
			// This can happen if the same think handle was removed twice
			if ( !m_ThinkEntries.IsInList( (uintp)hThink ) )
				continue;

			// NOTE: This is necessary for the case where the client entity handle
			// is slammed to NULL during a think interval; the hThink will get stuck
			// in the list and can never leave.
			SetNextClientThink( hThink, m_aChangeList[i].m_flNextTime );
		}
		else
		{
			SetNextClientThink( m_aChangeList[i].m_hEnt, m_aChangeList[i].m_flNextTime );
		}
	}
	m_aChangeList.RemoveAll();

	// Clear out the client-side entity deletion list.
	CleanUpDeleteList();
}


//-----------------------------------------------------------------------------
// Queued-up entity deletion
//-----------------------------------------------------------------------------
void CClientThinkList::AddToDeleteList( ClientEntityHandle_t hEnt )
{
	// Sanity check!
	Assert( hEnt != ClientEntityList().InvalidHandle() );
	if ( hEnt == ClientEntityList().InvalidHandle() )
		return;

	// Check to see if entity is networkable -- don't let it release!
	C_BaseEntity *pEntity = ClientEntityList().GetBaseEntityFromHandle( hEnt );
	if ( pEntity )
	{
		// Check to see if the entity is already being removed!
		if ( pEntity->IsMarkedForDeletion() )
			return;

		// Don't add networkable entities to delete list -- the server should
		// take care of this.  The delete list is for client-side only entities.
		if ( !pEntity->GetClientNetworkable() )
		{
			m_aDeleteList.AddToTail( hEnt );
			pEntity->SetRemovalFlag( true );
		}
	}
}

void CClientThinkList::RemoveFromDeleteList( ClientEntityHandle_t hEnt )
{
	// Sanity check!
	Assert( hEnt != ClientEntityList().InvalidHandle() );
	if ( hEnt == ClientEntityList().InvalidHandle() )
		return;

	int nSize = m_aDeleteList.Count();
	for ( int iHandle = 0; iHandle < nSize; ++iHandle )
	{
		if ( m_aDeleteList[iHandle] == hEnt )
		{
			m_aDeleteList[iHandle] = ClientEntityList().InvalidHandle();

			C_BaseEntity *pEntity = ClientEntityList().GetBaseEntityFromHandle( hEnt );
			if ( pEntity )
			{
				pEntity->SetRemovalFlag( false );
			}
		}
	}
}

void CClientThinkList::CleanUpDeleteList()
{
	int nThinkCount = m_aDeleteList.Count();
	for ( int iThink = 0; iThink < nThinkCount; ++iThink )
	{
		ClientEntityHandle_t handle = m_aDeleteList[iThink];
		if ( handle != ClientEntityList().InvalidHandle() )
		{
			C_BaseEntity *pEntity = ClientEntityList().GetBaseEntityFromHandle( handle );
			if ( pEntity )
			{
				pEntity->SetRemovalFlag( false );
			}

			IClientThinkable *pThink = ClientEntityList().GetClientThinkableFromHandle( handle );
			if ( pThink )
			{
				pThink->Release();
			}
		}
	}

	m_aDeleteList.RemoveAll();
}

