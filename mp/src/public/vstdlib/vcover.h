//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A simple tool for coverage tests
//
//=============================================================================

#ifndef VCOVER_H
#define VCOVER_H

#include "tier1/utlrbtree.h"
#include "vstdlib.h"

#if defined( _WIN32 )
#pragma once
#endif

class CVCoverage
{
public:
	CVCoverage() :
	  m_bActive( false ),
	  m_depth( 0 ),
	  m_token( 1 )
	{
	}

	bool IsActive() const
	{
		return m_bActive;
	}

	void SetActive( bool bActive )
	{
		Assert( bActive != m_bActive );
		m_bActive = bActive;
		if ( bActive )
			++m_token;
	}

	void Begin()
	{
		++m_depth;
	}

	void End()
	{
		--m_depth;
	}

	void Reset()
	{
		m_locations.RemoveAll();
	}

	bool ShouldCover( unsigned token ) const
	{
		return ( m_bActive && m_depth > 0 && token != m_token );
	}

	unsigned Cover( const char *pszFile, int line )
	{
		Location_t location = { pszFile, line };

		m_locations.Insert( location );

		return m_token;
	}

	void Report()
	{
		for ( int i = m_locations.FirstInorder(); i != m_locations.InvalidIndex(); i = m_locations.NextInorder( i ) )
		{
			Msg( "%s(%d) :\n", m_locations[i].pszFile, m_locations[i].line );
		}
	}

private:
	struct Location_t
	{
		const char *pszFile;
		int line;

	};

	class CLocationLess
	{
	public:
		CLocationLess( int ignored ) {}
		bool operator!() { return false; }

		bool operator()(  const Location_t &lhs, const Location_t &rhs ) const
		{
			if ( lhs.line < rhs.line )
			{
				return true;
			}

			return CaselessStringLessThan( lhs.pszFile, rhs.pszFile );
		}
	};

	bool m_bActive;
	int m_depth;
	unsigned m_token;

	CUtlRBTree< Location_t, unsigned short, CLocationLess > m_locations;
};

VSTDLIB_INTERFACE CVCoverage g_VCoverage;

#ifdef VCOVER_ENABLED
#define VCOVER() \
	do \
	{ \
		static token; \
		if ( g_VCoverage.ShouldCover( token ) ) \
		{ \
			token = g_VCoverage.Cover(  __FILE__, __LINE__ ); \
		} \
	} while( 0 )
#else
#define VCOVER() ((void)0)
#endif

#endif // VCOVER_H
