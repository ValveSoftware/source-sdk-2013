//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENTITYLIST_BASE_H
#define ENTITYLIST_BASE_H
#ifdef _WIN32
#pragma once
#endif


#include "const.h"
#include "basehandle.h"
#include "utllinkedlist.h"
#include "ihandleentity.h"


class CEntInfo
{
public:
	IHandleEntity	*m_pEntity;
	int				m_SerialNumber;
	CEntInfo		*m_pPrev;
	CEntInfo		*m_pNext;

	void			ClearLinks();
};


class CBaseEntityList
{
public:
	CBaseEntityList();
	~CBaseEntityList();
	
	// Add and remove entities. iForcedSerialNum should only be used on the client. The server
	// gets to dictate what the networkable serial numbers are on the client so it can send
	// ehandles over and they work.
	CBaseHandle AddNetworkableEntity( IHandleEntity *pEnt, int index, int iForcedSerialNum = -1 );
	CBaseHandle AddNonNetworkableEntity( IHandleEntity *pEnt );
	void RemoveEntity( CBaseHandle handle );

	// Get an ehandle from a networkable entity's index (note: if there is no entity in that slot,
	// then the ehandle will be invalid and produce NULL).
	CBaseHandle GetNetworkableHandle( int iEntity ) const;

	// ehandles use this in their Get() function to produce a pointer to the entity.
	IHandleEntity* LookupEntity( const CBaseHandle &handle ) const;
	IHandleEntity* LookupEntityByNetworkIndex( int edictIndex ) const;

	// Use these to iterate over all the entities.
	CBaseHandle FirstHandle() const;
	CBaseHandle NextHandle( CBaseHandle hEnt ) const;
	static CBaseHandle InvalidHandle();

	const CEntInfo *FirstEntInfo() const;
	const CEntInfo *NextEntInfo( const CEntInfo *pInfo ) const;
	const CEntInfo *GetEntInfoPtr( const CBaseHandle &hEnt ) const;
	const CEntInfo *GetEntInfoPtrByIndex( int index ) const;

// Overridables.
protected:

	// These are notifications to the derived class. It can cache info here if it wants.
	virtual void OnAddEntity( IHandleEntity *pEnt, CBaseHandle handle );
	
	// It is safe to delete the entity here. We won't be accessing the pointer after
	// calling OnRemoveEntity.
	virtual void OnRemoveEntity( IHandleEntity *pEnt, CBaseHandle handle );


private:

	CBaseHandle AddEntityAtSlot( IHandleEntity *pEnt, int iSlot, int iForcedSerialNum );
	void RemoveEntityAtSlot( int iSlot );

	
private:
	
	class CEntInfoList
	{
	public:
		CEntInfoList();

		const CEntInfo	*Head() const { return m_pHead; }
		const CEntInfo	*Tail() const { return m_pTail; }
		CEntInfo		*Head() { return m_pHead; }
		CEntInfo		*Tail() { return m_pTail; }
		void			AddToHead( CEntInfo *pElement ) { LinkAfter( NULL, pElement ); }
		void			AddToTail( CEntInfo *pElement ) { LinkBefore( NULL, pElement ); }

		void LinkBefore( CEntInfo *pBefore, CEntInfo *pElement );
		void LinkAfter( CEntInfo *pBefore, CEntInfo *pElement );
		void Unlink( CEntInfo *pElement );
		bool IsInList( CEntInfo *pElement );
	
	private:
		CEntInfo		*m_pHead;
		CEntInfo		*m_pTail;
	};

	int GetEntInfoIndex( const CEntInfo *pEntInfo ) const;


	// The first MAX_EDICTS entities are networkable. The rest are client-only or server-only.
	CEntInfo m_EntPtrArray[NUM_ENT_ENTRIES];
	CEntInfoList	m_activeList;
	CEntInfoList	m_freeNonNetworkableList;
};


// ------------------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------------------ //

inline int CBaseEntityList::GetEntInfoIndex( const CEntInfo *pEntInfo ) const
{
	Assert( pEntInfo );
	int index = (int)(pEntInfo - m_EntPtrArray);
	Assert( index >= 0 && index < NUM_ENT_ENTRIES );
	return index;
}

inline CBaseHandle CBaseEntityList::GetNetworkableHandle( int iEntity ) const
{
	Assert( iEntity >= 0 && iEntity < MAX_EDICTS );
	if ( m_EntPtrArray[iEntity].m_pEntity )
		return CBaseHandle( iEntity, m_EntPtrArray[iEntity].m_SerialNumber );
	else
		return CBaseHandle();
}


inline IHandleEntity* CBaseEntityList::LookupEntity( const CBaseHandle &handle ) const
{
	if ( handle.m_Index == INVALID_EHANDLE )
		return NULL;

	const CEntInfo *pInfo = &m_EntPtrArray[ handle.GetEntryIndex() ];
	if ( pInfo && pInfo->m_SerialNumber == handle.GetSerialNumber() )
		return pInfo->m_pEntity;
	else
		return NULL;
}


inline IHandleEntity* CBaseEntityList::LookupEntityByNetworkIndex( int edictIndex ) const
{
	// (Legacy support).
	if ( edictIndex < 0 )
		return NULL;

	Assert( edictIndex < NUM_ENT_ENTRIES );

	if ( edictIndex >= NUM_ENT_ENTRIES )
		return NULL;

	return m_EntPtrArray[edictIndex].m_pEntity;
}


inline CBaseHandle CBaseEntityList::FirstHandle() const
{
	if ( !m_activeList.Head() )
		return INVALID_EHANDLE;

	int index = GetEntInfoIndex( m_activeList.Head() );
	return CBaseHandle( index, m_EntPtrArray[index].m_SerialNumber );
}

inline CBaseHandle CBaseEntityList::NextHandle( CBaseHandle hEnt ) const
{
	int iSlot = hEnt.GetEntryIndex();
	CEntInfo *pNext = m_EntPtrArray[iSlot].m_pNext;
	if ( !pNext )
		return INVALID_EHANDLE;

	int index = GetEntInfoIndex( pNext );

	return CBaseHandle( index, m_EntPtrArray[index].m_SerialNumber );
}
	
inline CBaseHandle CBaseEntityList::InvalidHandle()
{
	return INVALID_EHANDLE;
}

inline const CEntInfo *CBaseEntityList::FirstEntInfo() const
{
	return m_activeList.Head();
}

inline const CEntInfo *CBaseEntityList::NextEntInfo( const CEntInfo *pInfo ) const
{
	return pInfo->m_pNext;
}

inline const CEntInfo *CBaseEntityList::GetEntInfoPtr( const CBaseHandle &hEnt ) const
{
	int iSlot = hEnt.GetEntryIndex();
	return &m_EntPtrArray[iSlot];
}

inline const CEntInfo *CBaseEntityList::GetEntInfoPtrByIndex( int index ) const
{
	return &m_EntPtrArray[index];
}

extern CBaseEntityList *g_pEntityList;

#endif // ENTITYLIST_BASE_H
