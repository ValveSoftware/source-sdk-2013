//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "entitylist_base.h"
#include "ihandleentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// the max value of a serial number, rolls back to 0 when it hits this limit
// we use 1 less than the number of serial bits, since highest bit is reserved for static props
static const uint32 SERIAL_MASK = ( ( 1 << ( NUM_SERIAL_NUM_BITS - 1 ) ) - 1 ); 

void CEntInfo::ClearLinks()
{
	m_pPrev = m_pNext = this;
}

CBaseEntityList::CEntInfoList::CEntInfoList()
{
	m_pHead = NULL;
	m_pTail = NULL;
}

// NOTE: Cut from UtlFixedLinkedList<>, UNDONE: Find a way to share this code
void CBaseEntityList::CEntInfoList::LinkBefore( CEntInfo *pBefore, CEntInfo *pElement )
{
	Assert( pElement );
	
	// Unlink it if it's in the list at the moment
	Unlink(pElement);
	
	// The element *after* our newly linked one is the one we linked before.
	pElement->m_pNext = pBefore;
	
	if (pBefore == NULL)
	{
		// In this case, we're linking to the end of the list, so reset the tail
		pElement->m_pPrev = m_pTail;
		m_pTail = pElement;
	}
	else
	{
		// Here, we're not linking to the end. Set the prev pointer to point to
		// the element we're linking.
		Assert( IsInList(pBefore) );
		pElement->m_pPrev = pBefore->m_pPrev;
		pBefore->m_pPrev = pElement;
	}
	
	// Reset the head if we linked to the head of the list
	if (pElement->m_pPrev == NULL)
	{
		m_pHead = pElement;
	}
	else
	{
		pElement->m_pPrev->m_pNext = pElement;
	}
}

void CBaseEntityList::CEntInfoList::LinkAfter( CEntInfo *pAfter, CEntInfo *pElement )
{
	Assert( pElement );
	
	// Unlink it if it's in the list at the moment
	if ( IsInList(pElement) )
		Unlink(pElement);
	
	// The element *before* our newly linked one is the one we linked after
	pElement->m_pPrev = pAfter;
	if (pAfter == NULL)
	{
		// In this case, we're linking to the head of the list, reset the head
		pElement->m_pNext = m_pHead;
		m_pHead = pElement;
	}
	else
	{
		// Here, we're not linking to the end. Set the next pointer to point to
		// the element we're linking.
		Assert( IsInList(pAfter) );
		pElement->m_pNext = pAfter->m_pNext;
		pAfter->m_pNext = pElement;
	}
	
	// Reset the tail if we linked to the tail of the list
	if (pElement->m_pNext == NULL )
	{
		m_pTail = pElement;
	}
	else
	{
		pElement->m_pNext->m_pPrev = pElement;
	}
}

void CBaseEntityList::CEntInfoList::Unlink( CEntInfo *pElement )
{
	if (IsInList(pElement))
	{
		// If we're the first guy, reset the head
		// otherwise, make our previous node's next pointer = our next
		if ( pElement->m_pPrev )
		{
			pElement->m_pPrev->m_pNext = pElement->m_pNext;
		}
		else
		{
			m_pHead = pElement->m_pNext;
		}
		
		// If we're the last guy, reset the tail
		// otherwise, make our next node's prev pointer = our prev
		if ( pElement->m_pNext )
		{
			pElement->m_pNext->m_pPrev = pElement->m_pPrev;
		}
		else
		{
			m_pTail = pElement->m_pPrev;
		}
		
		// This marks this node as not in the list, 
		// but not in the free list either
		pElement->ClearLinks();
	}
}

bool CBaseEntityList::CEntInfoList::IsInList( CEntInfo *pElement )
{
	return pElement->m_pPrev != pElement;
}

CBaseEntityList::CBaseEntityList()
{
	// These are not in any list (yet)
	int i;
	for ( i = 0; i < NUM_ENT_ENTRIES; i++ )
	{
		m_EntPtrArray[i].ClearLinks();
		m_EntPtrArray[i].m_SerialNumber = (rand()& SERIAL_MASK); // generate random starting serial number
		m_EntPtrArray[i].m_pEntity = NULL;
	}

	// make a free list of the non-networkable entities
	// Initially, all the slots are free.
	for ( i=MAX_EDICTS+1; i < NUM_ENT_ENTRIES; i++ )
	{
		CEntInfo *pList = &m_EntPtrArray[i];
		m_freeNonNetworkableList.AddToTail( pList );
	}
}


CBaseEntityList::~CBaseEntityList()
{
	CEntInfo *pList = m_activeList.Head();

	while ( pList )
	{
		CEntInfo *pNext = pList->m_pNext;
		RemoveEntityAtSlot( GetEntInfoIndex( pList ) );
		pList = pNext;
	}
}


CBaseHandle CBaseEntityList::AddNetworkableEntity( IHandleEntity *pEnt, int index, int iForcedSerialNum )
{
	Assert( index >= 0 && index < MAX_EDICTS );
	return AddEntityAtSlot( pEnt, index, iForcedSerialNum );
}


CBaseHandle CBaseEntityList::AddNonNetworkableEntity( IHandleEntity *pEnt )
{
	// Find a slot for it.
	CEntInfo *pSlot = m_freeNonNetworkableList.Head();
	if ( !pSlot )
	{
		Warning( "CBaseEntityList::AddNonNetworkableEntity: no free slots!\n" );
		AssertMsg( 0, ( "CBaseEntityList::AddNonNetworkableEntity: no free slots!\n" ) );
		return CBaseHandle();
	}

	// Move from the free list into the allocated list.
	m_freeNonNetworkableList.Unlink( pSlot );
	int iSlot = GetEntInfoIndex( pSlot );
	
	return AddEntityAtSlot( pEnt, iSlot, -1 );
}


void CBaseEntityList::RemoveEntity( CBaseHandle handle )
{
	RemoveEntityAtSlot( handle.GetEntryIndex() );
}


CBaseHandle CBaseEntityList::AddEntityAtSlot( IHandleEntity *pEnt, int iSlot, int iForcedSerialNum )
{
	// Init the CSerialEntity.
	CEntInfo *pSlot = &m_EntPtrArray[iSlot];
	Assert( pSlot->m_pEntity == NULL );
	pSlot->m_pEntity = pEnt;

	// Force the serial number (client-only)?
	if ( iForcedSerialNum != -1 )
	{
		pSlot->m_SerialNumber = iForcedSerialNum;
		
		#if !defined( CLIENT_DLL )
			// Only the client should force the serial numbers.
			Assert( false );
		#endif
	}
	
	// Update our list of active entities.
	m_activeList.AddToTail( pSlot );
	CBaseHandle retVal( iSlot, pSlot->m_SerialNumber );

	// Tell the entity to store its handle.
	pEnt->SetRefEHandle( retVal );

	// Notify any derived class.
	OnAddEntity( pEnt, retVal );
	return retVal;
}


void CBaseEntityList::RemoveEntityAtSlot( int iSlot )
{
	Assert( iSlot >= 0 && iSlot < NUM_ENT_ENTRIES );

	CEntInfo *pInfo = &m_EntPtrArray[iSlot];

	if ( pInfo->m_pEntity )
	{
		pInfo->m_pEntity->SetRefEHandle( INVALID_EHANDLE );

		// Notify the derived class that we're about to remove this entity.
		OnRemoveEntity( pInfo->m_pEntity, CBaseHandle( iSlot, pInfo->m_SerialNumber ) );

		// Increment the serial # so ehandles go invalid.
		pInfo->m_pEntity = NULL;
		pInfo->m_SerialNumber = ( pInfo->m_SerialNumber+1)& SERIAL_MASK;

		m_activeList.Unlink( pInfo );

		// Add the slot back to the free list if it's a non-networkable entity.
		if ( iSlot >= MAX_EDICTS )
		{
			m_freeNonNetworkableList.AddToTail( pInfo );
		}
	}
}


void CBaseEntityList::OnAddEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
}



void CBaseEntityList::OnRemoveEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
}
