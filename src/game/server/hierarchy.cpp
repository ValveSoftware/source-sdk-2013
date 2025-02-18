//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains the set of functions for manipulating entity hierarchies.
//
// $NoKeywords: $
//=============================================================================//

// UNDONE: Reconcile this with SetParent()

#include "cbase.h"
#include "hierarchy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Does the linked list work of removing a child object from the hierarchy.
// Input  : pParent - 
//			pChild - 
//-----------------------------------------------------------------------------
void UnlinkChild( CBaseEntity *pParent, CBaseEntity *pChild )
{
	CBaseEntity *pList;
	EHANDLE *pPrev;

	pList = pParent->m_hMoveChild;
	pPrev = &pParent->m_hMoveChild;
	while ( pList )
	{
		CBaseEntity *pNext = pList->m_hMovePeer;
		if ( pList == pChild )
		{
			// patch up the list
			pPrev->Set( pNext );

			// Clear hierarchy bits for this guy
			pList->m_hMoveParent.Set( NULL );
			pList->m_hMovePeer.Set( NULL );
			pList->NetworkProp()->SetNetworkParent( INVALID_EHANDLE );
			pList->DispatchUpdateTransmitState();	
			pList->OnEntityEvent( ENTITY_EVENT_PARENT_CHANGED, NULL );
			
			pParent->RecalcHasPlayerChildBit();
			return;
		}
		else
		{
			pPrev = &pList->m_hMovePeer;
			pList = pNext;
		}
	}

	// This only happens if the child wasn't found in the parent's child list
	Assert(0);
}

void LinkChild( CBaseEntity *pParent, CBaseEntity *pChild )
{
	EHANDLE hParent;
	hParent.Set( pParent );
	pChild->m_hMovePeer.Set( pParent->FirstMoveChild() );
	pParent->m_hMoveChild.Set( pChild );
	pChild->m_hMoveParent = hParent;
	pChild->NetworkProp()->SetNetworkParent( hParent );
	pChild->DispatchUpdateTransmitState();
	pChild->OnEntityEvent( ENTITY_EVENT_PARENT_CHANGED, NULL );
	pParent->RecalcHasPlayerChildBit();
}

void TransferChildren( CBaseEntity *pOldParent, CBaseEntity *pNewParent )
{
	CBaseEntity *pChild = pOldParent->FirstMoveChild();
	while ( pChild )
	{
		// NOTE: Have to do this before the unlink to ensure local coords are valid
		Vector vecAbsOrigin = pChild->GetAbsOrigin();
		QAngle angAbsRotation = pChild->GetAbsAngles();
		Vector vecAbsVelocity = pChild->GetAbsVelocity();
//		QAngle vecAbsAngVelocity = pChild->GetAbsAngularVelocity();

		UnlinkChild( pOldParent, pChild );
		LinkChild( pNewParent, pChild );

		// FIXME: This is a hack to guarantee update of the local origin, angles, etc.
		pChild->m_vecAbsOrigin.Init( FLT_MAX, FLT_MAX, FLT_MAX );
		pChild->m_angAbsRotation.Init( FLT_MAX, FLT_MAX, FLT_MAX );
		pChild->m_vecAbsVelocity.Init( FLT_MAX, FLT_MAX, FLT_MAX );

		pChild->SetAbsOrigin(vecAbsOrigin);
		pChild->SetAbsAngles(angAbsRotation);
		pChild->SetAbsVelocity(vecAbsVelocity);
//		pChild->SetAbsAngularVelocity(vecAbsAngVelocity);

		pChild  = pOldParent->FirstMoveChild();
	}
}

void UnlinkFromParent( CBaseEntity *pRemove )
{
	if ( pRemove->GetMoveParent() )
	{
		// NOTE: Have to do this before the unlink to ensure local coords are valid
		Vector vecAbsOrigin = pRemove->GetAbsOrigin();
		QAngle angAbsRotation = pRemove->GetAbsAngles();
		Vector vecAbsVelocity = pRemove->GetAbsVelocity();
//		QAngle vecAbsAngVelocity = pRemove->GetAbsAngularVelocity();

		UnlinkChild( pRemove->GetMoveParent(), pRemove );

		pRemove->SetLocalOrigin(vecAbsOrigin);
		pRemove->SetLocalAngles(angAbsRotation);
		pRemove->SetLocalVelocity(vecAbsVelocity);
//		pRemove->SetLocalAngularVelocity(vecAbsAngVelocity);
		pRemove->UpdateWaterState();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Clears the parent of all the children of the given object.
//-----------------------------------------------------------------------------
void UnlinkAllChildren( CBaseEntity *pParent )
{
	CBaseEntity *pChild = pParent->FirstMoveChild();
	while ( pChild )
	{
		CBaseEntity *pNext = pChild->NextMovePeer();
		UnlinkFromParent( pChild );
		pChild  = pNext;
	}
}

bool EntityIsParentOf( CBaseEntity *pParent, CBaseEntity *pEntity )
{
	while ( pEntity->GetMoveParent() )
	{
		pEntity = pEntity->GetMoveParent();
		if ( pParent == pEntity )
			return true;
	}

	return false;
}

static void GetAllChildren_r( CBaseEntity *pEntity, CUtlVector<CBaseEntity *> &list )
{
	for ( ; pEntity != NULL; pEntity = pEntity->NextMovePeer() )
	{
		list.AddToTail( pEntity );
		GetAllChildren_r( pEntity->FirstMoveChild(), list );
	}
}

int GetAllChildren( CBaseEntity *pParent, CUtlVector<CBaseEntity *> &list )
{
	if ( !pParent )
		return 0;

	GetAllChildren_r( pParent->FirstMoveChild(), list );
	return list.Count();
}

int	GetAllInHierarchy( CBaseEntity *pParent, CUtlVector<CBaseEntity *> &list )
{
	if (!pParent)
		return 0;
	list.AddToTail( pParent );
	return GetAllChildren( pParent, list ) + 1;
}
