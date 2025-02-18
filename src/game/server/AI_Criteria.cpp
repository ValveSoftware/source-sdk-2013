//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "AI_Criteria.h"
#include "ai_speech.h"
#include <KeyValues.h>
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
AI_CriteriaSet::AI_CriteriaSet() : m_Lookup( 0, 0, CritEntry_t::LessFunc )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
//-----------------------------------------------------------------------------
AI_CriteriaSet::AI_CriteriaSet( const AI_CriteriaSet& src ) : m_Lookup( 0, 0, CritEntry_t::LessFunc )
{
	// Use fast Copy CUtlRBTree CopyFrom. WARNING: It only handles POD.
	m_Lookup.CopyFrom( src.m_Lookup );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
AI_CriteriaSet::~AI_CriteriaSet()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *criteria - 
//			"" - 
//			1.0f - 
//-----------------------------------------------------------------------------
void AI_CriteriaSet::AppendCriteria( const char *criteria, const char *value /*= ""*/, float weight /*= 1.0f*/ )
{
	// Note: value pointer may come from an entry inside m_Lookup!
	// that value string must be copied out before any modification
	// to the m_Lookup struct which could make the pointer invalid
	int idx = FindCriterionIndex( criteria );
	if ( idx == -1 )
	{
		CritEntry_t entry;
		entry.criterianame = criteria;
		MEM_ALLOC_CREDIT();
		entry.SetValue(value);
		entry.weight = weight;
		m_Lookup.Insert( entry );
	}
	else
	{
		CritEntry_t *entry = &m_Lookup[ idx ];
		entry->SetValue( value );
		entry->weight = weight;
	}
}


//-----------------------------------------------------------------------------
// Removes criteria in a set
//-----------------------------------------------------------------------------
void AI_CriteriaSet::RemoveCriteria( const char *criteria )
{
	int idx = FindCriterionIndex( criteria );
	if ( idx == -1 )
		return;

	m_Lookup.RemoveAt( idx );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int AI_CriteriaSet::GetCount() const
{
	return m_Lookup.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int AI_CriteriaSet::FindCriterionIndex( const char *name ) const
{
	CritEntry_t search;
	search.criterianame = name;
	int idx = m_Lookup.Find( search );
	if ( idx == m_Lookup.InvalidIndex() )
		return -1;

	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *AI_CriteriaSet::GetName( int index ) const
{
	static char namebuf[ 128 ];
	if ( index < 0 || index >= (int)m_Lookup.Count() )
		return "";

	const CritEntry_t *entry = &m_Lookup[ index ];
	Q_strncpy( namebuf, entry->criterianame.String(), sizeof( namebuf ) );
	return namebuf;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *AI_CriteriaSet::GetValue( int index ) const
{
	if ( index < 0 || index >= (int)m_Lookup.Count() )
		return "";

	const CritEntry_t *entry = &m_Lookup[ index ];
	return entry->value;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : float
//-----------------------------------------------------------------------------
float AI_CriteriaSet::GetWeight( int index ) const
{
	if ( index < 0 || index >= (int)m_Lookup.Count() )
		return 1.0f;

	const CritEntry_t *entry = &m_Lookup[ index ];
	return entry->weight;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AI_CriteriaSet::Describe()
{
	for ( short i = m_Lookup.FirstInorder(); i != m_Lookup.InvalidIndex(); i = m_Lookup.NextInorder( i ) )
	{
		CritEntry_t *entry = &m_Lookup[ i ];

		if ( entry->weight != 1.0f )
		{
			DevMsg( "  %20s = '%s' (weight %f)\n", entry->criterianame.String(), entry->value, entry->weight );
		}
		else
		{
			DevMsg( "  %20s = '%s'\n", entry->criterianame.String(), entry->value );
		}
	}
}

BEGIN_SIMPLE_DATADESC( AI_ResponseParams )
	DEFINE_FIELD( flags,	FIELD_SHORT ),
	DEFINE_FIELD( odds,	FIELD_SHORT ),	
	DEFINE_FIELD( soundlevel,	FIELD_CHARACTER ),	
	DEFINE_FIELD( delay,	FIELD_INTEGER ),		// These are compressed down to two float16s, so treat as an INT for saverestore
	DEFINE_FIELD( respeakdelay,	FIELD_INTEGER ),	//  
END_DATADESC()

BEGIN_SIMPLE_DATADESC( AI_Response )
	DEFINE_FIELD( m_Type,	FIELD_CHARACTER ),
	DEFINE_ARRAY( m_szResponseName, FIELD_CHARACTER, AI_Response::MAX_RESPONSE_NAME ),	
	DEFINE_ARRAY( m_szMatchingRule, FIELD_CHARACTER, AI_Response::MAX_RULE_NAME ),	
	// DEFINE_FIELD( m_pCriteria, FIELD_??? ), // Don't need to save this probably
	DEFINE_EMBEDDED( m_Params ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
AI_Response::AI_Response()
{
	m_Type = RESPONSE_NONE;
	m_szResponseName[0] = 0;
	m_szMatchingRule[0] = 0;

	m_pCriteria = NULL;
	m_bApplyContextToWorld = false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
AI_Response::AI_Response( const AI_Response &from )
{
	m_pCriteria = NULL;
	*this = from;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
AI_Response::~AI_Response()
{
	delete m_pCriteria;
	m_pCriteria = NULL;
}

//-----------------------------------------------------------------------------
AI_Response &AI_Response::operator=( const AI_Response &from )
{
	Assert( (void*)(&m_Type) == (void*)this );

	if (this == &from)
		return *this;

	m_Type = from.m_Type;

	V_strcpy_safe( m_szResponseName, from.m_szResponseName );
	V_strcpy_safe( m_szMatchingRule, from.m_szMatchingRule );

	delete m_pCriteria;
	m_pCriteria = NULL;

	// Copy criteria.
	if (from.m_pCriteria)
		m_pCriteria = new AI_CriteriaSet(*from.m_pCriteria);

	m_Params = from.m_Params;

	m_szContext = from.m_szContext;
	m_bApplyContextToWorld = from.m_bApplyContextToWorld;

	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *response - 
//			*criteria - 
//-----------------------------------------------------------------------------
void AI_Response::Init( ResponseType_t type, const char *responseName, const AI_CriteriaSet& criteria,
					const AI_ResponseParams& responseparams, const char *ruleName, const char *applyContext,
					bool bApplyContextToWorld )
{
	m_Type = type;

	V_strcpy_safe( m_szResponseName, responseName );
	V_strcpy_safe( m_szMatchingRule, ruleName ? ruleName : "NULL" );

	// Copy underlying criteria
	Assert( !m_pCriteria );
	m_pCriteria = new AI_CriteriaSet( criteria );

	m_Params = responseparams;

	m_szContext = applyContext;
	m_bApplyContextToWorld = bApplyContextToWorld;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AI_Response::Describe()
{
	if ( m_pCriteria )
	{
		DevMsg( "Search criteria:\n" );
		m_pCriteria->Describe();
	}
	if ( m_szMatchingRule[ 0 ] )
		DevMsg( "Matched rule '%s', ", m_szMatchingRule );
	if ( m_szContext.Length() )
		DevMsg( "Contexts to set '%s' on %s, ", m_szContext.Get(), m_bApplyContextToWorld ? "world" : "speaker" );

	DevMsg( "response %s = '%s'\n", DescribeResponse( (ResponseType_t)m_Type ), m_szResponseName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char * AI_Response::GetNamePtr() const
{
    return m_szResponseName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char * AI_Response::GetResponsePtr() const
{
    return m_szResponseName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : char const
//-----------------------------------------------------------------------------
const char *AI_Response::DescribeResponse( ResponseType_t type )
{
	if ( (int)type < 0 || (int)type >= NUM_RESPONSES )
	{
		Assert( 0 );
		return "???AI_Response bogus index";
	}

	switch( type )
	{
	case RESPONSE_NONE:     return "RESPONSE_NONE";
	case RESPONSE_SPEAK:    return "RESPONSE_SPEAK";
	case RESPONSE_SENTENCE: return "RESPONSE_SENTENCE";
	case RESPONSE_SCENE:    return "RESPONSE_SCENE";
	case RESPONSE_RESPONSE: return "RESPONSE_RESPONSE";
	case RESPONSE_PRINT:    return "RESPONSE_PRINT";
	}

	Assert( 0 );
	return "RESPONSE_NONE";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const AI_CriteriaSet
//-----------------------------------------------------------------------------
const AI_CriteriaSet *AI_Response::GetCriteria()
{
	return m_pCriteria;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AI_Response::Release()
{
	delete this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : soundlevel_t
//-----------------------------------------------------------------------------
soundlevel_t AI_Response::GetSoundLevel() const
{
	if ( m_Params.flags & AI_ResponseParams::RG_SOUNDLEVEL )
	{
		return (soundlevel_t)m_Params.soundlevel;
	}

	return SNDLVL_TALKING;
}

float AI_Response::GetRespeakDelay( void ) const
{
	if ( m_Params.flags & AI_ResponseParams::RG_RESPEAKDELAY )
	{
		interval_t temp;
		m_Params.respeakdelay.ToInterval( temp );
		return RandomInterval( temp );
	}

	return 0.0f;
}

float AI_Response::GetWeaponDelay( void ) const
{
	if ( m_Params.flags & AI_ResponseParams::RG_WEAPONDELAY )
	{
		interval_t temp;
		m_Params.weapondelay.ToInterval( temp );
		return RandomInterval( temp );
	}

	return 0.0f;
}

bool AI_Response::GetSpeakOnce( void ) const
{
	if ( m_Params.flags & AI_ResponseParams::RG_SPEAKONCE )
	{
		return true;
	}

	return false;
}

bool AI_Response::ShouldntUseScene( void ) const
{
	return ( m_Params.flags & AI_ResponseParams::RG_DONT_USE_SCENE ) != 0;
}

bool AI_Response::ShouldBreakOnNonIdle( void ) const
{
	return ( m_Params.flags & AI_ResponseParams::RG_STOP_ON_NONIDLE ) != 0;
}

int AI_Response::GetOdds( void ) const
{
	if ( m_Params.flags & AI_ResponseParams::RG_ODDS )
	{
		return m_Params.odds;
	}
	return 100;
}

float AI_Response::GetDelay() const
{
	if ( m_Params.flags & AI_ResponseParams::RG_DELAYAFTERSPEAK )
	{
		interval_t temp;
		m_Params.delay.ToInterval( temp );
		return RandomInterval( temp );
	}
	return 0.0f;
}

float AI_Response::GetPreDelay() const
{
	if ( m_Params.flags & AI_ResponseParams::RG_DELAYBEFORESPEAK )
	{
		interval_t temp;
		m_Params.predelay.ToInterval( temp );
		return RandomInterval( temp );
	}
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Sets context string
// Output : void
//-----------------------------------------------------------------------------
void AI_Response::SetContext( const char *context )
{
	m_szContext = context;
}

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
const char *SplitContext( const char *raw, char *key, int keylen, char *value, int valuelen, float *duration )
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
