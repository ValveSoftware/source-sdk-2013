//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef BASEHANDLE_H
#define BASEHANDLE_H
#ifdef _WIN32
#pragma once
#endif


#include "const.h"
#include "tier0/platform.h"
#include "tier0/dbg.h"


class IHandleEntity;

// -------------------------------------------------------------------------------------------------- //
// CBaseHandle.
// -------------------------------------------------------------------------------------------------- //

enum INVALID_EHANDLE_tag
{
	INVALID_EHANDLE
};

class CBaseHandle
{
friend class CBaseEntityList;

public:

	CBaseHandle();
	CBaseHandle( INVALID_EHANDLE_tag );
	CBaseHandle( const CBaseHandle &other );
	explicit CBaseHandle( IHandleEntity* pHandleObj );
	CBaseHandle( int iEntry, int iSerialNumber );

	// NOTE: The following constructor is not type-safe, and can allow creating an
	//       arbitrary CBaseHandle that doesn't necessarily point to an actual object.
	//
	// It is your responsibility to ensure that the target of the handle actually points
	// to the object you think it does.  Generally, the argument to this function should
	// have been obtained from CBaseHandle::ToInt() on a valid handle.
	static CBaseHandle UnsafeFromIndex( int index );

	void Init( int iEntry, int iSerialNumber );
	void Term();

	// Even if this returns true, Get() still can return return a non-null value.
	// This just tells if the handle has been initted with any values.
	bool IsValid() const;

	int GetEntryIndex() const;
	int GetSerialNumber() const;

	int ToInt() const;
	bool operator !=( const CBaseHandle &other ) const;
	bool operator ==( const CBaseHandle &other ) const;
	bool operator ==( const IHandleEntity* pEnt ) const;
	bool operator !=( const IHandleEntity* pEnt ) const;
	bool operator <( const CBaseHandle &other ) const;
	bool operator <( const IHandleEntity* pEnt ) const;

	// Assign a value to the handle.
	const CBaseHandle& operator=( const IHandleEntity *pEntity );
	const CBaseHandle& Set( const IHandleEntity *pEntity );

	// Use this to dereference the handle.
	// Note: this is implemented in game code (ehandle.h)
	IHandleEntity* Get() const;


protected:
	// The low NUM_SERIAL_BITS hold the index. If this value is less than MAX_EDICTS, then the entity is networkable.
	// The high NUM_SERIAL_NUM_BITS bits are the serial number.
	uint32	m_Index;
};


#include "ihandleentity.h"


inline CBaseHandle::CBaseHandle()
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline CBaseHandle::CBaseHandle( INVALID_EHANDLE_tag )
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline CBaseHandle::CBaseHandle( const CBaseHandle &other )
{
	m_Index = other.m_Index;
}

inline CBaseHandle::CBaseHandle( IHandleEntity* pEntity )
{
	Set( pEntity );
}

inline CBaseHandle::CBaseHandle( int iEntry, int iSerialNumber )
{
	Init( iEntry, iSerialNumber );
}

inline CBaseHandle CBaseHandle::UnsafeFromIndex( int index )
{
	CBaseHandle ret;
	ret.m_Index = index;
	return ret;
}

inline void CBaseHandle::Init( int iEntry, int iSerialNumber )
{
	Assert( iEntry >= 0 && (iEntry & ENT_ENTRY_MASK) == iEntry);
	Assert( iSerialNumber >= 0 && iSerialNumber < (1 << NUM_SERIAL_NUM_BITS) );

	m_Index = iEntry | (iSerialNumber << NUM_SERIAL_NUM_SHIFT_BITS);
}

inline void CBaseHandle::Term()
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline bool CBaseHandle::IsValid() const
{
	return m_Index != INVALID_EHANDLE_INDEX;
}

inline int CBaseHandle::GetEntryIndex() const
{
	// There is a hack here: due to a bug in the original implementation of the 
	// entity handle system, an attempt to look up an invalid entity index in 
	// certain cirumstances might fall through to the the mask operation below.
	// This would mask an invalid index to be in fact a lookup of entity number
	// NUM_ENT_ENTRIES, so invalid ent indexes end up actually looking up the
	// last slot in the entities array. Since this slot is always empty, the 
	// lookup returns NULL and the expected behavior occurs through this unexpected
	// route.
	// A lot of code actually depends on this behavior, and the bug was only exposed
	// after a change to NUM_SERIAL_NUM_BITS increased the number of allowable
	// static props in the world. So the if-stanza below detects this case and 
	// retains the prior (bug-submarining) behavior.
	if ( !IsValid() )
		return NUM_ENT_ENTRIES-1;
	return m_Index & ENT_ENTRY_MASK;
}

inline int CBaseHandle::GetSerialNumber() const
{
	return m_Index >> NUM_SERIAL_NUM_SHIFT_BITS;
}

inline int CBaseHandle::ToInt() const
{
	return (int)m_Index;
}

inline bool CBaseHandle::operator !=( const CBaseHandle &other ) const
{
	return m_Index != other.m_Index;
}

inline bool CBaseHandle::operator ==( const CBaseHandle &other ) const
{
	return m_Index == other.m_Index;
}

inline bool CBaseHandle::operator ==( const IHandleEntity* pEnt ) const
{
	return Get() == pEnt;
}

inline bool CBaseHandle::operator !=( const IHandleEntity* pEnt ) const
{
	return Get() != pEnt;
}

inline bool CBaseHandle::operator <( const CBaseHandle &other ) const
{
	return m_Index < other.m_Index;
}

inline bool CBaseHandle::operator <( const IHandleEntity *pEntity ) const
{
	uint32 otherIndex = (pEntity) ? pEntity->GetRefEHandle().m_Index : INVALID_EHANDLE_INDEX;
	return m_Index < otherIndex;
}

inline const CBaseHandle& CBaseHandle::operator=( const IHandleEntity *pEntity )
{
	return Set( pEntity );
}

inline const CBaseHandle& CBaseHandle::Set( const IHandleEntity *pEntity ) 
{ 
	if ( pEntity )
	{
		*this = pEntity->GetRefEHandle();
	}
	else
	{
		m_Index = INVALID_EHANDLE_INDEX;
	}
	
	return *this;
}


#endif // BASEHANDLE_H
