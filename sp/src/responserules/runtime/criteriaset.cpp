//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include "rrbase.h"

#include "utlmap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace ResponseRules;

//-----------------------------------------------------------------------------
// Case-insensitive criteria symbol table
//-----------------------------------------------------------------------------
CUtlSymbolTable CriteriaSet::sm_CriteriaSymbols( 1024, 1024, true );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *raw - 
//			*key - 
//			keylen - 
//			*value - 
//			valuelen - 
//			*duration -
// Output : static bool
//-----------------------------------------------------------------------------
const char *SplitContext( const char *raw, char *key, int keylen, char *value, int valuelen, float *duration, const char *entireContext )
{
	char *colon1 = Q_strstr( raw, ":" );
	if ( !colon1 )
	{
		DevMsg( "SplitContext:  warning, ignoring context '%s', missing colon separator!\n", raw );
		*key = *value = 0;
		return NULL;
	}

	int len = colon1 - raw;
	Q_strncpy( key, raw, MIN( len + 1, keylen ) );
	key[ MIN( len, keylen - 1 ) ] = 0;

	bool last = false;
	char *end = Q_strstr( colon1 + 1, "," );
	if ( !end )
	{
		int remaining = Q_strlen( colon1 + 1 );
		end = colon1 + 1 + remaining;
		last = true;
	}

	char *colon2 = Q_strstr( colon1 + 1, ":" );
	if ( colon2 && ( colon2 < end ) )
	{
		if ( duration )
			*duration = atof( colon2 + 1 );

		char durationStartChar = *(colon2 + 1);
		if ( durationStartChar < '0' || durationStartChar > '9' )
		{
			DevMsg( "SplitContext:  warning, ignoring context '%s', missing comma separator!  Entire context was '%s'.\n", raw, entireContext );
			*key = *value = 0;
			return NULL;
		}

		len = MIN( colon2 - ( colon1 + 1 ), valuelen - 1 );
		Q_strncpy( value, colon1 + 1, len + 1 );
		value[ len ] = 0;
	}
	else
	{
		if ( duration )
			*duration = 0.0;

		len = MIN( end - ( colon1 + 1 ), valuelen - 1 );
		Q_strncpy( value, colon1 + 1, len + 1 );
		value[ len ] = 0;
	}

	return last ? NULL : end + 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CriteriaSet::CriteriaSet() : m_Lookup( 0, 0, CritEntry_t::LessFunc ), m_bOverrideOnAppend(true), 
	m_nNumPrefixedContexts(0)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CriteriaSet::CriteriaSet( const CriteriaSet& src ) : m_Lookup( 0, 0, CritEntry_t::LessFunc ), m_nNumPrefixedContexts(src.m_nNumPrefixedContexts)
{
	m_Lookup.EnsureCapacity( src.m_Lookup.Count() );
	for ( short i = src.m_Lookup.FirstInorder(); 
		i != src.m_Lookup.InvalidIndex(); 
		i = src.m_Lookup.NextInorder( i ) )
	{
		m_Lookup.Insert( src.m_Lookup[ i ] );
	}
}

CriteriaSet::CriteriaSet( const char *criteria, const char *value ) : m_Lookup( 0, 0, CritEntry_t::LessFunc ), m_bOverrideOnAppend(true)
{
	AppendCriteria(criteria,value);
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CriteriaSet::~CriteriaSet()
{
}

//-----------------------------------------------------------------------------
// Computes a symbol for the criteria
//-----------------------------------------------------------------------------
CriteriaSet::CritSymbol_t CriteriaSet::ComputeCriteriaSymbol( const char *criteria )
{
	return sm_CriteriaSymbols.AddString( criteria );
}


//-----------------------------------------------------------------------------
// Computes a symbol for the criteria
//-----------------------------------------------------------------------------
void CriteriaSet::AppendCriteria( CriteriaSet::CritSymbol_t criteria, const char *value, float weight )
{
	int idx = FindCriterionIndex( criteria );
	if ( idx == -1 )
	{
		CritEntry_t entry;
		entry.criterianame = criteria;
		MEM_ALLOC_CREDIT();
		idx = m_Lookup.Insert( entry );
		if ( sm_CriteriaSymbols.String(criteria)[0] == kAPPLYTOWORLDPREFIX )
		{
			m_nNumPrefixedContexts += 1;
		}
	}
	else // criteria already existed
	{
		// bail out if override existing criteria is not allowed
		if ( !m_bOverrideOnAppend )
			return; 
	}

	CritEntry_t *entry = &m_Lookup[ idx ];
	entry->SetValue( value );
	entry->weight = weight;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *criteria - 
//			"" - 
//			1.0f - 
//-----------------------------------------------------------------------------
void CriteriaSet::AppendCriteria( const char *pCriteriaName, const char *value /*= ""*/, float weight /*= 1.0f*/ )
{
	CUtlSymbol criteria = ComputeCriteriaSymbol( pCriteriaName );
	AppendCriteria( criteria, value, weight );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *criteria - 
//			"" - 
//			1.0f - 
//-----------------------------------------------------------------------------
void	CriteriaSet::AppendCriteria( const char *criteria, float value, float weight /*= 1.0f*/ )
{
	char buf[32];
	V_snprintf( buf, 32, "%f", value );
	AppendCriteria( criteria, buf, weight );
}


//-----------------------------------------------------------------------------
// Removes criteria in a set
//-----------------------------------------------------------------------------
void CriteriaSet::RemoveCriteria( const char *criteria )
{
	const int idx = FindCriterionIndex( criteria );
	if ( idx == -1 )
		return;

	if ( criteria[0] == kAPPLYTOWORLDPREFIX )
	{
		Assert( m_nNumPrefixedContexts > 0 );
		m_nNumPrefixedContexts = isel( m_nNumPrefixedContexts - 1, m_nNumPrefixedContexts - 1, 0 );
	}
	RemoveCriteria( idx, false );
}

// bTestForIndex tells us whether the calling function has already checked for a 
// $ prefix and decremented m_nNumPrefixedContexts appropriately (false), 
// or if this function should do that (true).
void CriteriaSet::RemoveCriteria( int idx, bool bTestForPrefix )
{
	Assert( m_Lookup.IsValidIndex(idx) );
	if ( bTestForPrefix )
	{
		if ( sm_CriteriaSymbols.String( m_Lookup[idx].criterianame )[0] == kAPPLYTOWORLDPREFIX )
		{
			Assert( m_nNumPrefixedContexts > 0 );
			m_nNumPrefixedContexts = isel( m_nNumPrefixedContexts - 1, m_nNumPrefixedContexts - 1, 0 );
		}
	}
	m_Lookup.RemoveAt( idx );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CriteriaSet::GetCount() const
{
	return m_Lookup.Count();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CriteriaSet::FindCriterionIndex( CritSymbol_t criteria ) const
{
	CritEntry_t search;
	search.criterianame = criteria;
	int idx = m_Lookup.Find( search );
	return ( idx == m_Lookup.InvalidIndex() ) ? -1 : idx;
}

int CriteriaSet::FindCriterionIndex( const char *name ) const
{
	CUtlSymbol criteria = ComputeCriteriaSymbol( name );
	return FindCriterionIndex( criteria );
}


//-----------------------------------------------------------------------------
// Returns the name symbol
//-----------------------------------------------------------------------------
CriteriaSet::CritSymbol_t CriteriaSet::GetNameSymbol( int nIndex ) const
{
	if ( nIndex < 0 || nIndex >= (int)m_Lookup.Count() )
		return UTL_INVAL_SYMBOL;

	const CritEntry_t *entry = &m_Lookup[ nIndex ];
	return entry->criterianame;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CriteriaSet::GetName( int index ) const
{
	if ( index < 0 || index >= (int)m_Lookup.Count() )
		return "";
	else
	{
		const char *pCriteriaName = sm_CriteriaSymbols.String( m_Lookup[ index ].criterianame );
		return pCriteriaName ? pCriteriaName : "";
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CriteriaSet::GetValue( int index ) const
{
	if ( index < 0 || index >= (int)m_Lookup.Count() )
		return "";

	const CritEntry_t *entry = &m_Lookup[ index ];
	return entry->value ? entry->value : "";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : float
//-----------------------------------------------------------------------------
float CriteriaSet::GetWeight( int index ) const
{
	if ( index < 0 || index >= (int)m_Lookup.Count() )
		return 1.0f;

	const CritEntry_t *entry = &m_Lookup[ index ];
	return entry->weight;
}


//-----------------------------------------------------------------------------
// Purpose: Merge another criteria set into this one.
//-----------------------------------------------------------------------------
void CriteriaSet::Merge( const CriteriaSet * RESTRICT otherCriteria )
{
	Assert(otherCriteria);
	if (!otherCriteria)
		return;

	// for now, just duplicate everything.
	int count = otherCriteria->GetCount();
	EnsureCapacity( count + GetCount() );
	for ( int i = 0 ; i < count ; ++i )
	{
		AppendCriteria( otherCriteria->GetNameSymbol(i), otherCriteria->GetValue(i), otherCriteria->GetWeight(i) );
	}
}

void CriteriaSet::Merge( const char *modifiers ) // add criteria parsed from a text string
{
	// Always include any optional modifiers
	if ( modifiers == NULL )
		return;

	char copy_modifiers[ 255 ];
	const char *pCopy;
	char key[ 128 ] = { 0 };
	char value[ 128 ] = { 0 };

	Q_strncpy( copy_modifiers, modifiers, sizeof( copy_modifiers ) );
	pCopy = copy_modifiers;

	while( pCopy )
	{
		pCopy = SplitContext( pCopy, key, sizeof( key ), value, sizeof( value ), NULL, modifiers );

		if( *key && *value )
		{
			AppendCriteria( key, value, 1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CriteriaSet::Describe() const
{
	// build an alphabetized representation of the set for printing
	typedef CUtlMap<const char *, const CritEntry_t *> tMap;
	tMap m_TempMap( 0, m_Lookup.Count(), CaselessStringLessThan );

	for ( short i = m_Lookup.FirstInorder(); i != m_Lookup.InvalidIndex(); i = m_Lookup.NextInorder( i ) )
	{
		const CritEntry_t *entry = &m_Lookup[ i ];

		m_TempMap.Insert( sm_CriteriaSymbols.String( entry->criterianame ), entry );
	}

	for ( tMap::IndexType_t i = m_TempMap.FirstInorder(); i != m_TempMap.InvalidIndex(); i = m_TempMap.NextInorder( i ) )
	{
		// const CritEntry_t *entry = &m_TempMap[ i ];
		// const char *pCriteriaName = sm_CriteriaSymbols.String( entry->criterianame );

		const char *name = m_TempMap.Key( i );
		const CritEntry_t *entry  = m_TempMap.Element( i );
		if ( entry->weight != 1.0f )
		{
			DevMsg( "  %20s = '%s' (weight %f)\n", name, entry->value ? entry->value : "", entry->weight );
		}
		else
		{
			DevMsg( "  %20s = '%s'\n", name, entry->value ? entry->value : "" );
		}
	}

	/*
	for ( short i = m_Lookup.FirstInorder(); i != m_Lookup.InvalidIndex(); i = m_Lookup.NextInorder( i ) )
	{
		const CritEntry_t *entry = &m_Lookup[ i ];

		const char *pCriteriaName = sm_CriteriaSymbols.String( entry->criterianame );
		if ( entry->weight != 1.0f )
		{
			DevMsg( "  %20s = '%s' (weight %f)\n", pCriteriaName, entry->value ? entry->value : "", entry->weight );
		}
		else
		{
			DevMsg( "  %20s = '%s'\n", pCriteriaName, entry->value ? entry->value : "" );
		}
	}
	*/
}


void CriteriaSet::Reset()
{
	m_Lookup.Purge();
}

void CriteriaSet::WriteToEntity( CBaseEntity *pEntity )
{
#if 0
	if ( GetCount() < 1 )
		return;

	for ( int i = Head() ; IsValidIndex(i); i = Next(i) )
	{
		pEntity->AddContext( GetName(i), GetValue(i), 0 );
	}
#else
	AssertMsg( false, "CriteriaSet::WriteToEntity has not been ported from l4d2.\n" );
#endif
}

int CriteriaSet::InterceptWorldSetContexts( CriteriaSet * RESTRICT pFrom, CriteriaSet * RESTRICT pSetOnWorld )
{
	// Assert( pFrom ); Assert( pTo ); Assert( pSetOnWorld );
	Assert( pSetOnWorld != pFrom );
	Assert( pSetOnWorld->GetCount() == 0 );

	if ( pFrom->m_nNumPrefixedContexts == 0 )
	{
		// nothing needs to be done to it.
		return 0;
	}

#ifdef DEBUG
	// save this off for later error checking.
	const int nPrefixedContexts = pFrom->m_nNumPrefixedContexts;
#endif

	// make enough space for the expected output quantity.
	pSetOnWorld->EnsureCapacity( pFrom->m_nNumPrefixedContexts ); 

	// initialize a buffer with the "world" prefix (so we can use strncpy instead of snprintf and be much faster)
	char buf[80] = { 'w', 'o', 'r', 'l', 'd', '\0' };
	const unsigned int PREFIXLEN = 5; // strlen("world")

	// create a second tree that has the appropriately renamed criteria,
	// then swap it into pFrom
	CriteriaSet rewrite;
	rewrite.EnsureCapacity( pFrom->GetCount() + 1 );

	for ( int i = pFrom->Head(); pFrom->IsValidIndex(i); i = pFrom->Next(i) )
	{
		const char *pszName = pFrom->GetName( i );
		if ( pszName[0] == CriteriaSet::kAPPLYTOWORLDPREFIX )
		{	// redirect to the world contexts
			V_strncpy( buf+PREFIXLEN, pszName+1, sizeof(buf) - PREFIXLEN );
			rewrite.AppendCriteria( buf, pFrom->GetValue(i), pFrom->GetWeight(i) );
			pSetOnWorld->AppendCriteria( pszName+1, pFrom->GetValue(i), pFrom->GetWeight(i) );
			buf[PREFIXLEN] = 0;
		}
		else
		{	// does not need to be fiddled; do not write back to world
			rewrite.AppendCriteria( pFrom->GetNameSymbol(i), pFrom->GetValue(i), pFrom->GetWeight(i) );
		}
	}
	AssertMsg2( pSetOnWorld->GetCount() == nPrefixedContexts, "Count of $ persistent RR contexts is inconsistent (%d vs %d)! Call Elan.",
		pSetOnWorld->GetCount(), nPrefixedContexts	);

	pFrom->m_nNumPrefixedContexts = 0;
	pFrom->m_Lookup.Swap(rewrite.m_Lookup);
	return pSetOnWorld->GetCount();
}
