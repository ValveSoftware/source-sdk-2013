//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple class to make it easier to deal with flags
//
//=============================================================================

#ifndef UTLFLAGS_H
#define UTLFLAGS_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"


//-----------------------------------------------------------------------------
// Simple class to make it easier to deal with flags
//-----------------------------------------------------------------------------
template< class T >
class CUtlFlags
{
public:
	CUtlFlags( int nInitialFlags = 0 );

	// Flag setting
	void SetFlag( int nFlagMask );
	void SetFlag( int nFlagMask, bool bEnable );

	// Flag clearing
	void ClearFlag( int nFlagMask );
	void ClearAllFlags();
	bool IsFlagSet( int nFlagMask ) const;

	// Is any flag set?
	bool IsAnyFlagSet() const;

private:
	T m_nFlags;
};


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
template< class T >
CUtlFlags<T>::CUtlFlags( int nInitialFlags )
{
	// Makes sure we didn't truncate
	Assert( nInitialFlags == (T)nInitialFlags );

	m_nFlags = (T)nInitialFlags;
}


//-----------------------------------------------------------------------------
// Set flags
//-----------------------------------------------------------------------------
template< class T >
void CUtlFlags<T>::SetFlag( int nFlagMask )
{
	// Makes sure we didn't truncate
	Assert( nFlagMask == (T)nFlagMask );

	m_nFlags |= (T)nFlagMask;
}

template< class T >
void CUtlFlags<T>::SetFlag( int nFlagMask, bool bEnable )
{
	// Makes sure we didn't truncate
	Assert( nFlagMask == (T)nFlagMask );

	if ( bEnable )
	{
		m_nFlags |= (T)nFlagMask;
	}
	else
	{
		m_nFlags &= ~((T)nFlagMask);
	}
}


//-----------------------------------------------------------------------------
// Clear flags
//-----------------------------------------------------------------------------
template< class T >
void CUtlFlags<T>::ClearFlag( int nFlagMask )
{
	// Makes sure we didn't truncate
	Assert( nFlagMask == (T)nFlagMask );
	m_nFlags &= ~((T)nFlagMask);
}

template< class T >
void CUtlFlags<T>::ClearAllFlags()
{
	m_nFlags = 0;
}


//-----------------------------------------------------------------------------
// Is a flag set?
//-----------------------------------------------------------------------------
template< class T >
bool CUtlFlags<T>::IsFlagSet( int nFlagMask ) const
{
	// Makes sure we didn't truncate
	Assert( nFlagMask == (T)nFlagMask );
	return ( m_nFlags & nFlagMask ) != 0;
}


//-----------------------------------------------------------------------------
// Is any flag set?
//-----------------------------------------------------------------------------
template< class T >
bool CUtlFlags<T>::IsAnyFlagSet() const
{
	return m_nFlags != 0;
}


#endif // UTLFLAGS_H
