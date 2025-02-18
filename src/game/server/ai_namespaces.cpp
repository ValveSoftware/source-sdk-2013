//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "stringregistry.h"
#include "ai_namespaces.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CAI_GlobalNamespace
//

CAI_GlobalNamespace::CAI_GlobalNamespace()
 :	m_pSymbols( new CStringRegistry ),
 	m_NextGlobalBase( GLOBAL_IDS_BASE )
{
}

//-------------------------------------

CAI_GlobalNamespace::~CAI_GlobalNamespace()
{
	delete m_pSymbols;
}

//-------------------------------------

void CAI_GlobalNamespace::Clear()
{
	m_pSymbols->ClearStrings();
	m_NextGlobalBase = GLOBAL_IDS_BASE;
}

//-------------------------------------

void CAI_GlobalNamespace::AddSymbol( const char *pszSymbol, int symbolID )
{
	MEM_ALLOC_CREDIT();
	AssertMsg( symbolID != -1, "Invalid symbol id passed to CAI_GlobalNamespace::AddSymbol()" );
	if (symbolID == -1 )
		return;

	AssertMsg( AI_IdIsGlobal( symbolID ), ("Local symbol ID passed to CAI_GlobalNamespace::AddSymbol()") );
	AssertMsg( !IdToSymbol( symbolID ) , ("Duplicate symbol ID passed to CAI_GlobalNamespace::AddSymbol()") );
	AssertMsg( SymbolToId( pszSymbol ) == -1, ("Duplicate symbol passed to CAI_GlobalNamespace::AddSymbol()") );

	m_pSymbols->AddString( pszSymbol, symbolID );

	if ( m_NextGlobalBase < symbolID + 1 )
		m_NextGlobalBase = symbolID + 1;
}

//-------------------------------------

const char *CAI_GlobalNamespace::IdToSymbol( int symbolID ) const
{
	AssertMsg( AI_IdIsGlobal( symbolID ), ("Local symbol ID passed to CAI_GlobalNamespace::IdToSymbol()") );
	if ( symbolID == -1 )
		return "<<null>>";
	return (const_cast<CStringRegistry *>(m_pSymbols))->GetStringText( symbolID );
}

//-------------------------------------

int CAI_GlobalNamespace::SymbolToId( const char *pszSymbol ) const
{
	return (const_cast<CStringRegistry *>(m_pSymbols))->GetStringID( pszSymbol );
}

//-------------------------------------

int CAI_GlobalNamespace::NextGlobalBase() const
{
	return m_NextGlobalBase;
}

//-----------------------------------------------------------------------------
// CAI_LocalIdSpace
//
// Purpose: Maps per class IDs to global IDs, so that various classes can use
//			the same integer in local space to represent different globally 
//			unique integers. Used for schedules, tasks, conditons and squads
//

CAI_LocalIdSpace::CAI_LocalIdSpace( bool fIsRoot )
 : 	m_pGlobalNamespace( NULL ),
 	m_pParentIDSpace( NULL ),
 	m_globalBase( (fIsRoot) ? 0 : -1 ),
 	m_localBase( (fIsRoot) ? 0 : MAX_STRING_INDEX ),
 	m_localTop( -1 ),
 	m_globalTop( -1 )
{
};

//-------------------------------------

bool CAI_LocalIdSpace::Init( CAI_GlobalNamespace *pGlobalNamespace, CAI_LocalIdSpace *pParentIDSpace )
{
	if ( m_globalTop != -1 )
	{
 		m_localBase = (!pParentIDSpace) ? 0 : MAX_STRING_INDEX;
 		m_localTop = -1;
 		m_globalTop = -1;
	}

	m_pParentIDSpace 	= pParentIDSpace;
	m_pGlobalNamespace 	= pGlobalNamespace;
	m_globalBase 	 	= m_pGlobalNamespace->NextGlobalBase();

	return true;
}

//-------------------------------------

bool CAI_LocalIdSpace::AddSymbol( const char *pszSymbol, int localId, const char *pszDebugSymbolType, const char *pszDebugOwner )
{
	AssertMsg( AI_IdIsLocal( localId ), ("Global symbol ID passed to CAI_LocalIdSpace::AddSymbol()") );
	
	if ( !m_pGlobalNamespace )
	{
		DevMsg( "ERROR: Adding symbol to uninitialized table %s\n", pszDebugOwner );
		return false;
	}
	
	if ( !IsLocalBaseSet() && !SetLocalBase( localId ) )
	{
		DevMsg( "ERROR: Bad %s LOCALID for %s\n", pszDebugSymbolType, pszDebugOwner );
		return false;
	}
	else if (localId < GetLocalBase() )
	{
		DevMsg("ERROR: %s First added %s must be first LOCALID!\n", pszDebugSymbolType, pszDebugOwner);
		return false;
	}

	AssertMsg3( LocalToGlobal( localId ) == -1 || !m_pGlobalNamespace->IdToSymbol( LocalToGlobal( localId ) ) , "Duplicate symbol ID passed to CAI_LocalIdSpace::AddSymbol(): %s (%d), had %s", pszSymbol, localId, m_pGlobalNamespace->IdToSymbol( LocalToGlobal( localId ) ) );
	AssertMsg( m_pGlobalNamespace->SymbolToId( pszSymbol ) == -1, "Duplicate symbol passed to CAI_LocalIdSpace::AddSymbol()" );

	if ( m_localTop != -1 )
	{
		if ( localId > m_localTop )
		{
			m_localTop = localId;
			m_globalTop = ( m_localTop - m_localBase ) + m_globalBase;
		}
	}
	else
	{
		m_localTop = m_localBase;
		m_globalTop = m_globalBase;
	}
	
	m_pGlobalNamespace->AddSymbol( pszSymbol, LocalToGlobal( localId ) );
	return true;
}

//-------------------------------------

bool CAI_LocalIdSpace::SetLocalBase( int newBase )
{
	AssertMsg( AI_IdIsLocal( newBase ), ("Global symbol ID passed to CAI_LocalIdSpace::SetLocalBase()") );

	if (m_localBase == MAX_STRING_INDEX)
	{
		// UNDONE: Make sure this is the largest Index in the list so far??
		m_localBase = newBase;
		if ( !m_pParentIDSpace || !m_pParentIDSpace->IsLocalBaseSet() || m_localBase > m_pParentIDSpace->m_localBase )// @Note (toml 08-20-02): this doesn't actually catch namespace collisions, only woefully bad situations where a derived class' offset is less than the base
			return true;
	}
	return false;
}

//-------------------------------------

int CAI_LocalIdSpace::GlobalToLocal( int globalID ) const
{
	if ( globalID == -1 )
		return -1;

	AssertMsg( AI_IdIsGlobal( globalID ), ("Local symbol ID passed to CAI_LocalIdSpace::GlobalToLocal()") );

	const CAI_LocalIdSpace *pCurrentMap = this;
	
	do 
	{
		if ( pCurrentMap->IsLocalBaseSet() && globalID >= pCurrentMap->GetGlobalBase() && globalID <= pCurrentMap->GetGlobalTop()  )
			return ( globalID - pCurrentMap->GetGlobalBase() + pCurrentMap->GetLocalBase() );
		pCurrentMap = pCurrentMap->m_pParentIDSpace;
	} while ( pCurrentMap != NULL );
	
	// AssertMsg( 0, ("Invalid ID passed to CAI_LocalIdSpace::GlobalToLocal()") );
	return -1;
}

//-------------------------------------

int CAI_LocalIdSpace::LocalToGlobal( int localID ) const
{
	if ( localID == -1 )
		return -1;

	AssertMsg( AI_IdIsLocal( localID ), ("Global symbol ID passed to CAI_LocalIdSpace::LocalToGlobal()") );

	const CAI_LocalIdSpace *pCurrentMap = this;
	
	do 
	{
		if ( pCurrentMap->IsLocalBaseSet() && localID >= pCurrentMap->GetLocalBase() && localID <= pCurrentMap->GetLocalTop() )
			return ( localID + pCurrentMap->GetGlobalBase() - pCurrentMap->GetLocalBase() );
		pCurrentMap = pCurrentMap->m_pParentIDSpace;
	} while ( pCurrentMap != NULL );
	
	// AssertMsg( 0, ("Invalid ID passed to CAI_LocalIdSpace::LocalToGlobal()") );
	return -1;
}

//-----------------------------------------------------------------------------
