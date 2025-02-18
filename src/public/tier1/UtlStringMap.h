//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef UTLSTRINGMAP_H
#define UTLSTRINGMAP_H
#ifdef _WIN32
#pragma once
#endif

#include "utlsymbol.h"

template <class T>
class CUtlStringMap
{
public:
	CUtlStringMap( bool caseInsensitive = true ) : m_SymbolTable( 0, 32, caseInsensitive )
	{
	}

	// Get data by the string itself:
	T& operator[]( const char *pString )
	{
		CUtlSymbol symbol = m_SymbolTable.AddString( pString );
		int index = ( int )( UtlSymId_t )symbol;
		if( m_Vector.Count() <= index )
		{
			m_Vector.EnsureCount( index + 1 );
		}
		return m_Vector[index];
	}

	// Get data by the string's symbol table ID - only used to retrieve a pre-existing symbol, not create a new one!
	T& operator[]( UtlSymId_t n )
	{
		Assert( n >=0 && n <= m_Vector.Count() );
		return m_Vector[n];
	}

	const T& operator[]( UtlSymId_t n ) const
	{
		Assert( n >=0 && n <= m_Vector.Count() );
		return m_Vector[n];
	}

	bool Defined( const char *pString ) const
	{
		return m_SymbolTable.Find( pString ) != UTL_INVAL_SYMBOL;
	}

	UtlSymId_t Find( const char *pString ) const
	{
		return m_SymbolTable.Find( pString );
	}

	static UtlSymId_t InvalidIndex()
	{
		return UTL_INVAL_SYMBOL;
	}

	int GetNumStrings( void ) const
	{
		return m_SymbolTable.GetNumStrings();
	}

	const char *String( int n )	const
	{
		return m_SymbolTable.String( n );
	}

	// Clear all of the data from the map
	void Clear()
	{
		m_Vector.RemoveAll();
		m_SymbolTable.RemoveAll();
	}

	void Purge()
	{
		m_Vector.Purge();
		m_SymbolTable.RemoveAll();
	}

	void PurgeAndDeleteElements()
	{
		m_Vector.PurgeAndDeleteElements();
		m_SymbolTable.RemoveAll();
	}



private:
	CUtlVector<T> m_Vector;
	CUtlSymbolTable m_SymbolTable;
};

#endif // UTLSTRINGMAP_H
