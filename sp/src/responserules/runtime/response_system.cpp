//========= Copyright © 1996-2010, Valve Corporation, All rights reserved. ============//
//
// Purpose: Core types for the response rules -- criteria, responses, rules, and matchers.
//
// $NoKeywords: $
//=============================================================================//

#include "rrbase.h"
#include "vstdlib/random.h"
#include "utlbuffer.h"
#include "tier1/interval.h"
#include "convar.h"
#include "fmtstr.h"
#include "generichash.h"
#include "tier1/mapbase_con_groups.h"
#ifdef MAPBASE
#include "tier1/mapbase_matchers_base.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// #pragma optimize( "", off )

using namespace ResponseRules;
static void CC_RR_Debug_ResponseConcept_Exclude( const CCommand &args );
static ConCommand rr_debug_responseconcept_exclude( "rr_debugresponseconcept_exclude", CC_RR_Debug_ResponseConcept_Exclude, "Set a list of concepts to exclude from rr_debugresponseconcept. Separate multiple concepts with spaces. Call with no arguments to see current list. Call 'rr_debug_responseconcept_exclude !' to reset.");

namespace ResponseRules
{
	// ick
	// Wrap string lookup with a hash on the string so that all of the repetitive playerxxx type strings get bucketed out better
	#define STRING_BUCKETS_COUNT 64		// Must be power of two
	#define STRING_BUCKETS_MASK ( STRING_BUCKETS_COUNT - 1 )

	CUtlRBTree<const char *> g_ResponseStrings[ STRING_BUCKETS_COUNT ];
	class CResponseStringBuckets
	{
	public:
		CResponseStringBuckets()
		{
			for ( int i = 0; i < ARRAYSIZE( g_ResponseStrings ); ++i )
			{
				g_ResponseStrings[ i ].SetLessFunc( &StringLessThan );
			}
		}
	} g_ReponseStringBucketInitializer;

	const char *ResponseCopyString( const char *in )
	{
		if ( !in )
			return NULL;
		if ( !*in )
			return "";

		int	bucket = ( RR_HASH( in ) & STRING_BUCKETS_MASK );

		CUtlRBTree<const char *> &list = g_ResponseStrings[ bucket ];

		int i = list.Find( in );
		if ( i != list.InvalidIndex() )
{
			return list[i];
		}

		int len = Q_strlen( in );
		char *out = new char[ len + 1 ];
		Q_memcpy( out, in, len );
		out[ len ] = 0;
		list.Insert( out );
		return out;
	}
}

IEngineEmulator *IEngineEmulator::Get()
{
	AssertMsg( IEngineEmulator::s_pSingleton, "Response rules fail: no IEngineEmulator" );
	return IEngineEmulator::s_pSingleton;
}


//-----------------------------------------------------------------------------
// Convars 
//-----------------------------------------------------------------------------


ConVar rr_debugresponses( "rr_debugresponses", "0", FCVAR_NONE, "Show verbose matching output (1 for simple, 2 for rule scoring, 3 for noisy). If set to 4, it will only show response success/failure for npc_selected NPCs." );
ConVar rr_debugrule( "rr_debugrule", "", FCVAR_NONE, "If set to the name of the rule, that rule's score will be shown whenever a concept is passed into the response rules system.");
ConVar rr_dumpresponses( "rr_dumpresponses", "0", FCVAR_NONE, "Dump all response_rules.txt and rules (requires restart)" );
ConVar rr_debugresponseconcept( "rr_debugresponseconcept", "", FCVAR_NONE, "If set, rr_debugresponses will print only responses testing for the specified concept" );
#define RR_DEBUGRESPONSES_SPECIALCASE 4

#ifdef MAPBASE
ConVar rr_disableemptyrules( "rr_disableemptyrules", "0", FCVAR_NONE, "Disables rules with no remaining responses, e.g. rules which use norepeat responses." );
#endif



//-----------------------------------------------------------------------------
// Copied from SoundParametersInternal.cpp 
//-----------------------------------------------------------------------------

#define SNDLVL_PREFIX "SNDLVL_"

struct SoundLevelLookup
{
	soundlevel_t	level;
	char const		*name;
};

// NOTE:  Needs to reflect the soundlevel_t enum defined in soundflags.h
static SoundLevelLookup g_pSoundLevels[] =
{
	{ SNDLVL_NONE, "SNDLVL_NONE" },
	{ SNDLVL_20dB, "SNDLVL_20dB" },
	{ SNDLVL_25dB, "SNDLVL_25dB" },
	{ SNDLVL_30dB, "SNDLVL_30dB" },
	{ SNDLVL_35dB, "SNDLVL_35dB" },
	{ SNDLVL_40dB, "SNDLVL_40dB" },
	{ SNDLVL_45dB, "SNDLVL_45dB" },
	{ SNDLVL_50dB, "SNDLVL_50dB" },
	{ SNDLVL_55dB, "SNDLVL_55dB" },
	{ SNDLVL_IDLE, "SNDLVL_IDLE" },
	{ SNDLVL_TALKING, "SNDLVL_TALKING" },
	{ SNDLVL_60dB, "SNDLVL_60dB" },
	{ SNDLVL_65dB, "SNDLVL_65dB" },
	{ SNDLVL_STATIC, "SNDLVL_STATIC" },
	{ SNDLVL_70dB, "SNDLVL_70dB" },
	{ SNDLVL_NORM, "SNDLVL_NORM" },
	{ SNDLVL_75dB, "SNDLVL_75dB" },
	{ SNDLVL_80dB, "SNDLVL_80dB" },
	{ SNDLVL_85dB, "SNDLVL_85dB" },
	{ SNDLVL_90dB, "SNDLVL_90dB" },
	{ SNDLVL_95dB, "SNDLVL_95dB" },
	{ SNDLVL_100dB, "SNDLVL_100dB" },
	{ SNDLVL_105dB, "SNDLVL_105dB" },
	{ SNDLVL_110dB, "SNDLVL_110dB" },
	{ SNDLVL_120dB, "SNDLVL_120dB" },
	{ SNDLVL_130dB, "SNDLVL_130dB" },
	{ SNDLVL_GUNFIRE, "SNDLVL_GUNFIRE" },
	{ SNDLVL_140dB, "SNDLVL_140dB" },
	{ SNDLVL_150dB, "SNDLVL_150dB" },
	{ SNDLVL_180dB, "SNDLVL_180dB" },
};

static soundlevel_t TextToSoundLevel( const char *key )
{
	if ( !key )
	{
		Assert( 0 );
		return SNDLVL_NORM;
	}

	int c = ARRAYSIZE( g_pSoundLevels );

	int i;

		for ( i = 0; i < c; i++ )
	{
		SoundLevelLookup *entry = &g_pSoundLevels[ i ];
		if ( !Q_strcasecmp( key, entry->name ) )
			return entry->level;
	}

	if ( !Q_strnicmp( key, SNDLVL_PREFIX, Q_strlen( SNDLVL_PREFIX ) ) )
	{
		char const *val = key + Q_strlen( SNDLVL_PREFIX );
		int sndlvl = atoi( val );
		if ( sndlvl > 0 && sndlvl <= 180 )
		{
			return ( soundlevel_t )sndlvl;
		}
	}

	DevMsg( "CSoundEmitterSystem:  Unknown sound level %s\n", key );

	return SNDLVL_NORM;
}

CResponseSystem::ExcludeList_t CResponseSystem::m_DebugExcludeList( 4, 0 );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CResponseSystem::CResponseSystem() : 
	m_RootCommandHashes( 0, 0, DefLessFunc( unsigned int ) ),
	m_FileDispatch( 0, 0, DefLessFunc( unsigned int ) ),
	m_RuleDispatch( 0, 0, DefLessFunc( unsigned int ) ),
	m_ResponseDispatch( 0, 0, DefLessFunc( unsigned int ) ),
	m_ResponseGroupDispatch( 0, 0, DefLessFunc( unsigned int ) )
{
	token[0] = 0;
	m_bUnget = false;
	m_bCustomManagable = false;
#ifdef MAPBASE
	m_bInProspective = false;
#endif

	BuildDispatchTables();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CResponseSystem::~CResponseSystem()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
void CResponseSystem::GetCurrentScript( char *buf, size_t buflen )
{
	Assert( buf );
	buf[ 0 ] = 0;
	if ( m_ScriptStack.Count() <= 0 )
		return;

	if ( IEngineEmulator::Get()->GetFilesystem()->String( m_ScriptStack[ 0 ].name, buf, buflen ) )
	{
		return;
	}
	buf[ 0 ] = 0;
}

void CResponseSystem::PushScript( const char *scriptfile, unsigned char *buffer )
{
	ScriptEntry e;
	e.name = IEngineEmulator::Get()->GetFilesystem()->FindOrAddFileName( scriptfile );
	e.buffer = buffer;
	e.currenttoken = (char *)e.buffer;
	e.tokencount = 0;
	m_ScriptStack.AddToHead( e );
}

void CResponseSystem::PopScript(void)
{
	Assert( m_ScriptStack.Count() >= 1 );
	if ( m_ScriptStack.Count() <= 0 )
		return;

	m_ScriptStack.Remove( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CResponseSystem::Clear()
{
	m_Responses.RemoveAll();
	m_Criteria.RemoveAll();
#ifdef MAPBASE
	// Must purge to avoid issues with reloading the system
	m_RulePartitions.PurgeAndDeleteElements();
#else
	m_RulePartitions.RemoveAll();
#endif
	m_Enumerations.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			found - 
// Output : float
//-----------------------------------------------------------------------------
float CResponseSystem::LookupEnumeration( const char *name, bool& found )
{
	int idx = m_Enumerations.Find( name );
	if ( idx == m_Enumerations.InvalidIndex() )
	{
		found = false;
		return 0.0f;
	}


	found = true;
	return m_Enumerations[ idx ].value;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : matcher - 
//-----------------------------------------------------------------------------
void CResponseSystem::ResolveToken( Matcher& matcher, char *token, size_t bufsize, char const *rawtoken )
{
	if ( rawtoken[0] != '[' )
	{
		Q_strncpy( token, rawtoken, bufsize );
		return;
	}

	// Now lookup enumeration
	bool found = false;
	float f = LookupEnumeration( rawtoken, found );
	if ( !found )
	{
		Q_strncpy( token, rawtoken, bufsize );
		ResponseWarning( "No such enumeration '%s'\n", token );
		return;
	}

	Q_snprintf( token, bufsize, "%f", f );
}


#ifndef MAPBASE // Already in mapbase_matchers_base
static bool AppearsToBeANumber( char const *token )
{
	if ( atof( token ) != 0.0f )
		return true;

	char const *p = token;
	while ( *p )
	{
		if ( *p != '0' )
			return false;

		p++;
	}

	return true;
}
#endif

void CResponseSystem::ComputeMatcher( Criteria *c, Matcher& matcher )
{
	const char *s = c->value;
	if ( !s )
	{
		matcher.valid = false;
		return;
	}

	const char *in = s;

	char token[ 128 ];
	char rawtoken[ 128 ];

	token[ 0 ] = 0;
	rawtoken[ 0 ] = 0;

	int n = 0;

	bool gt = false;
	bool lt = false;
	bool eq = false;
	bool nt = false;
#ifdef MAPBASE
	bool bit = false;
#endif

	bool done = false;
	while ( !done )
	{
		switch( *in )
		{
		case '>':
			{
				gt = true;
				Assert( !lt ); // Can't be both
			}
			break;
		case '<':
			{
				lt = true;
				Assert( !gt );  // Can't be both
			}
			break;
		case '=':
			{
				eq = true;
			}
			break;
		case ',':
		case '\0':
			{
				rawtoken[ n ] = 0;
				n = 0;

				// Convert raw token to real token in case token is an enumerated type specifier
				ResolveToken( matcher, token, sizeof( token ), rawtoken );

#ifdef MAPBASE
				// Bits are an entirely different and independent story
				if (bit)
				{
					matcher.isbit = true;
					matcher.notequal = nt;

					matcher.isnumeric = true;
				}
				else
#endif
				// Fill in first data set
				if ( gt )
				{
					matcher.usemin = true;
					matcher.minequals = eq;
					matcher.minval = (float)atof( token );

					matcher.isnumeric = true;
				}
				else if ( lt )
				{
					matcher.usemax = true;
					matcher.maxequals = eq;
					matcher.maxval = (float)atof( token );

					matcher.isnumeric = true;
				}
				else
				{
					if ( *in == ',' )
					{
						// If there's a comma, this better have been a less than or a gt key
						Assert( 0 );
					}

					matcher.notequal = nt;

					matcher.isnumeric = AppearsToBeANumber( token );
				}

				gt = lt = eq = nt = false;

				if ( !(*in) )
				{
					done = true;
				}
			}
			break;
		case '!':
			nt = true;
			break;
#ifdef MAPBASE
		case '~':
			nt = true;
		case '&':
			bit = true;
			break;
#endif
		default:
			rawtoken[ n++ ] = *in;
			break;
		}

		in++;
	}

	matcher.SetToken( token );
	matcher.SetRaw( rawtoken );
	matcher.valid = true;
}

bool CResponseSystem::CompareUsingMatcher( const char *setValue, Matcher& m, bool verbose /*=false*/ )
{
	if ( !m.valid )
		return false;

	float v = (float)atof( setValue );
	if ( setValue[0] == '[' )
	{
		bool found = false;
		v = LookupEnumeration( setValue, found );
	}

#ifdef MAPBASE
	// Bits are always a different story
	if (m.isbit)
	{
		int v1 = v;
		int v2 = atoi( m.GetToken() );
		if (m.notequal)
			return (v1 & v2) == 0;
		else
			return (v1 & v2) != 0;
	}
#endif

	int minmaxcount = 0;

	if ( m.usemin )
	{
		if ( m.minequals )
		{
			if ( v < m.minval )
				return false;
		}
		else
		{
			if ( v <= m.minval )
				return false;
		}

		++minmaxcount;
	}

	if ( m.usemax )
	{
		if ( m.maxequals )
		{
			if ( v > m.maxval )
				return false;
		}
		else
		{
			if ( v >= m.maxval )
				return false;
		}

		++minmaxcount;
	}

	// Had one or both criteria and met them
	if ( minmaxcount >= 1 )
	{
		return true;
	}

	if ( m.notequal )
	{
		if ( m.isnumeric )
		{
			if ( v == (float)atof( m.GetToken() ) )
				return false;
		}
		else
		{
#ifdef MAPBASE
			if ( Matcher_NamesMatch( m.GetToken(), setValue ) )
#else
			if ( !Q_stricmp( setValue, m.GetToken() ) )
#endif
				return false;
		}

		return true;
	}

	if ( m.isnumeric )
	{
		// If the setValue is "", the NPC doesn't have the key at all,
		// in which case we shouldn't match "0".
		if ( !setValue || !setValue[0] )
			return false;

		return v == (float)atof( m.GetToken() );
	}

#ifdef MAPBASE
	return Matcher_NamesMatch( m.GetToken(), setValue );
#else
	return !Q_stricmp( setValue, m.GetToken() ) ? true : false;
#endif
}

bool CResponseSystem::Compare( const char *setValue, Criteria *c, bool verbose /*= false*/ )
{
	Assert( c );
	Assert( setValue );

	bool bret = CompareUsingMatcher( setValue, c->matcher, verbose );

	if ( verbose )
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "'%20s' vs. '%20s' = ", setValue, c->value );

		{
			//DevMsg( "\n" );
			//m.Describe();
		}
	}
	return bret;
}

float CResponseSystem::RecursiveScoreSubcriteriaAgainstRule( const CriteriaSet& set, Criteria *parent, bool& exclude, bool verbose /*=false*/ )
{
	float score = 0.0f;
	int subcount = parent->subcriteria.Count();
	for ( int i = 0; i < subcount; i++ )
	{
		int icriterion = parent->subcriteria[ i ];

		bool excludesubrule = false;
		if (verbose)
		{
			DevMsg( "\n" );
		}
		score += ScoreCriteriaAgainstRuleCriteria( set, icriterion, excludesubrule, verbose );
	}

	exclude = ( parent->required && score == 0.0f ) ? true : false;

	return score * parent->weight.GetFloat();
}

float CResponseSystem::RecursiveLookForCriteria( const CriteriaSet &criteriaSet, Criteria *pParent )
{
	float flScore = 0.0f;
	int nSubCount = pParent->subcriteria.Count();
	for ( int iSub = 0; iSub < nSubCount; ++iSub )
	{
		int iCriteria = pParent->subcriteria[iSub];
		flScore += LookForCriteria( criteriaSet, iCriteria );
	}

	return flScore;
}

float CResponseSystem::LookForCriteria( const CriteriaSet &criteriaSet, int iCriteria )
{
	Criteria *pCriteria = &m_Criteria[iCriteria];
	if ( pCriteria->IsSubCriteriaType() )
	{
		return RecursiveLookForCriteria( criteriaSet, pCriteria );
	}

	int iIndex = criteriaSet.FindCriterionIndex( pCriteria->nameSym );
	if ( iIndex == -1 )
		return 0.0f;

	Assert( criteriaSet.GetValue( iIndex ) );
	if ( Q_stricmp( criteriaSet.GetValue( iIndex ), pCriteria->value ) )
		return 0.0f;

	return 1.0f;
}

float CResponseSystem::ScoreCriteriaAgainstRuleCriteria( const CriteriaSet& set, int icriterion, bool& exclude, bool verbose /*=false*/ )
{
	Criteria *c = &m_Criteria[ icriterion ];

	if ( c->IsSubCriteriaType() )
	{
		return RecursiveScoreSubcriteriaAgainstRule( set, c, exclude, verbose );
	}

	if ( verbose )
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "  criterion '%25s':'%15s' ", m_Criteria.GetElementName( icriterion ), CriteriaSet::SymbolToStr(c->nameSym) );
	}

	exclude = false;

	float score = 0.0f;

	const char *actualValue = "";

	/*
	const char * RESTRICT critname = c->name;
	CUtlSymbol sym(critname);
	const char * nameDoubleCheck = sym.String();
	*/
	int found = set.FindCriterionIndex( c->nameSym );
	if ( found != -1 )
	{
		actualValue = set.GetValue( found );
		if ( !actualValue )
		{
			Assert( 0 );
			return score;
		}
	}

	Assert( actualValue );

	if ( Compare( actualValue, c, verbose ) )
	{
		float w = set.GetWeight( found );
		score = w * c->weight.GetFloat();

		if ( verbose )
		{
			CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "matched, weight %4.2f (s %4.2f x c %4.2f)",
				score, w, c->weight.GetFloat() );
		}
	}
	else
	{
		if ( c->required )
		{
			exclude = true;
			if ( verbose )
			{
				CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "failed (+exclude rule)" );
			}
		}
		else
		{
			if ( verbose )
			{
				CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "failed" );
			}
		}
	}

	return score;
}

float CResponseSystem::ScoreCriteriaAgainstRule( const CriteriaSet& set, ResponseRulePartition::tRuleDict &dict, int irule, bool verbose /*=false*/ )
{
	Rule * RESTRICT rule = dict[ irule ];
	float score = 0.0f;

	bool bBeingWatched = false;

	// See if we're trying to debug this rule
	const char *pszText = rr_debugrule.GetString();
	if ( pszText && pszText[0] && !Q_stricmp( pszText, dict.GetElementName( irule ) ) )
	{
		bBeingWatched = true;
	}

	if ( !rule->IsEnabled() )
	{
		if ( bBeingWatched )
		{
			CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "Rule is disabled.\n" );
		}
		return 0.0f;
	}

	if ( bBeingWatched )
	{
		verbose = true;
	}

	if ( verbose )
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "Scoring rule '%s' (%i)\n{\n", dict.GetElementName( irule ), irule+1 );
	}

	// Iterate set criteria
	int count = rule->m_Criteria.Count();
	int i;
	for ( i = 0; i < count; i++ )
	{
		int icriterion = rule->m_Criteria[ i ];

		bool exclude = false;
		score += ScoreCriteriaAgainstRuleCriteria( set, icriterion, exclude, verbose );

		if ( verbose )
		{
			CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, ", score %4.2f\n", score );
		}

		if ( exclude ) 
		{
			score = 0.0f;
			break;
		}
	}

	if ( verbose )
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "}\n" );
	}

	if ( rule->m_nForceWeight > 0 )
	{	// this means override the cumulative weight of criteria and just force the rule's total score,
		// assuming it matched at all.
		return fsel( score - FLT_MIN, rule->m_nForceWeight, 0 );
	}
	else
	{
	return score;
}
}

void CResponseSystem::DebugPrint( int depth, const char *fmt, ... )
{
	int indentchars = 3 * depth;
	char *indent = (char *) stackalloc( indentchars + 1);
	indent[ indentchars ] = 0;
	while ( --indentchars >= 0 )
	{
		indent[ indentchars ] = ' ';
	}

	// Dump text to debugging console.
	va_list argptr;
	char szText[1024];

	va_start (argptr, fmt);
	Q_vsnprintf (szText, sizeof( szText ), fmt, argptr);
	va_end (argptr);

	CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "%s%s", indent, szText );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CResponseSystem::ResetResponseGroups()
{
	int i;
	int c = m_Responses.Count();
	for ( i = 0; i < c; i++ )
	{
		m_Responses[ i ].Reset();
	}

#ifdef MAPBASE
	for ( ResponseRulePartition::tIndex idx = m_RulePartitions.First() ;
		m_RulePartitions.IsValid(idx) ;
		idx = m_RulePartitions.Next(idx) )
	{
		m_RulePartitions[ idx ].m_bEnabled = true;
	}
#endif
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CResponseSystem::DisableEmptyRules()
{
	if (rr_disableemptyrules.GetBool() == false)
		return;

	for ( ResponseRulePartition::tIndex idx = m_RulePartitions.First() ;
		m_RulePartitions.IsValid(idx) ;
		idx = m_RulePartitions.Next(idx) )
	{
		Rule &rule = m_RulePartitions[ idx ];

		// Set it as disabled in advance
		rule.m_bEnabled = false;

		int c2 = rule.m_Responses.Count();
		for (int s = 0; s < c2; s++)
		{
			if (m_Responses[rule.m_Responses[s]].IsEnabled())
			{
				// Re-enable it if there's any valid responses
				rule.m_bEnabled = true;
				break;
			}
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Make certain responses unavailable by marking them as depleted 
//-----------------------------------------------------------------------------
void CResponseSystem::FakeDepletes( ResponseGroup *g, IResponseFilter *pFilter )
{
	m_FakedDepletes.RemoveAll();

	// Fake depletion of unavailable choices
	int c = g->group.Count();
	if ( pFilter && g->ShouldCheckRepeats() )
	{
		for ( int i = 0; i < c; i++ )
		{
			ParserResponse *r = &g->group[ i ];
			if ( r->depletioncount != g->GetDepletionCount() && !pFilter->IsValidResponse( r->GetType(), r->value ) )
			{
				m_FakedDepletes.AddToTail( i );
				g->MarkResponseUsed( i );
			}
		}
	}

	// Fake depletion of choices that fail the odds check
	for ( int i = 0; i < c; i++ )
	{
		ParserResponse *r = &g->group[ i ];
		if ( RandomInt( 1, 100 ) > r->params.odds )
		{
			m_FakedDepletes.AddToTail( i );
			g->MarkResponseUsed( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Restore responses that were faked as being depleted 
//-----------------------------------------------------------------------------
void CResponseSystem::RevertFakedDepletes( ResponseGroup *g )
{
	for ( int i = 0; i < m_FakedDepletes.Count(); i++ )
	{
		g->group[ m_FakedDepletes[ i ] ].depletioncount = 0;
	}
	m_FakedDepletes.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *g - 
// Output : int
//-----------------------------------------------------------------------------
int CResponseSystem::SelectWeightedResponseFromResponseGroup( ResponseGroup *g, IResponseFilter *pFilter )
{
	int c = g->group.Count();
	if ( !c )
	{
		Assert( !"Expecting response group with >= 1 elements" );
		return -1;
	}

	FakeDepletes( g, pFilter );

	if ( !g->HasUndepletedChoices() )
	{
		g->ResetDepletionCount();

		FakeDepletes( g, pFilter );

		if ( !g->HasUndepletedChoices() )
			return -1;

		// Disable the group if we looped through all the way
		if ( g->IsNoRepeat() )
		{
			g->SetEnabled( false );
#ifdef MAPBASE
			DisableEmptyRules();
#endif
			return -1;
		}
	}

	bool checkrepeats = g->ShouldCheckRepeats();
	int	depletioncount = g->GetDepletionCount();

	float totalweight = 0.0f;
	int slot = -1;

	if ( checkrepeats )
	{
		int check= -1;
		// Snag the first slot right away
		if ( g->HasUndepletedFirst( check ) && check != -1 )
		{
			slot = check;
		}

		if ( slot == -1 && g->HasUndepletedLast( check ) && check != -1 )
		{
			// If this is the only undepleted one, use it now
			int i;
			for ( i = 0; i < c; i++ )
			{
				ParserResponse *r = &g->group[ i ];
				if ( checkrepeats && 
					( r->depletioncount == depletioncount ) )
				{
					continue;
				}		

				if ( r->last )
				{
					Assert( i == check );
					continue;
				}

				// There's still another undepleted entry
				break;
			}

			// No more undepleted so use the r->last slot 
			if ( i >= c )
			{
				slot = check;
			}
		}
	}

	if ( slot == -1 )
	{
		for ( int i = 0; i < c; i++ )
		{
			ParserResponse *r = &g->group[ i ];
			if ( checkrepeats && 
				( r->depletioncount == depletioncount ) )
			{
				continue;
			}

			// Always skip last entry here since we will deal with it above
			if ( checkrepeats && r->last )
				continue;

			int prevSlot = slot;

			if ( !totalweight )
			{
				slot = i;
			}

			// Always assume very first slot will match
			totalweight += r->weight.GetFloat();
			if ( !totalweight || IEngineEmulator::Get()->GetRandomStream()->RandomFloat(0,totalweight) < r->weight.GetFloat() )
			{
				slot = i;
			}

			if ( !checkrepeats && slot != prevSlot && pFilter && !pFilter->IsValidResponse( r->GetType(), r->value ) )
			{
				slot = prevSlot;
				totalweight -= r->weight.GetFloat();
			}
		}
	}

	if ( slot != -1 )
	{
#ifdef MAPBASE
		// Don't mark responses as used in prospective mode
		if (m_bInProspective == false)
#endif
			g->MarkResponseUsed( slot );
	}

	// Revert fake depletion of unavailable choices
	RevertFakedDepletes( g );

	return slot;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : searchResult - 
//			depth - 
//			*name - 
//			verbose - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CResponseSystem::ResolveResponse( ResponseSearchResult& searchResult, int depth, const char *name, bool verbose /*= false*/, IResponseFilter *pFilter )
{
	int responseIndex = m_Responses.Find( name );
	if ( responseIndex == m_Responses.InvalidIndex() )
		return false;

	ResponseGroup *g = &m_Responses[ responseIndex ];
	// Group has been disabled
	if ( !g->IsEnabled() )
		return false;

	int c = g->group.Count();
	if ( !c )
		return false;

	int idx = 0;

	if ( g->IsSequential() )
	{
		// See if next index is valid
		int initialIndex = g->GetCurrentIndex();
		bool bFoundValid = false;

		do 
		{
			idx = g->GetCurrentIndex();
			g->SetCurrentIndex( idx + 1 );
			if ( idx >= c )
			{
				if ( g->IsNoRepeat() )
				{
					g->SetEnabled( false );
#ifdef MAPBASE
					DisableEmptyRules();
#endif
					return false;
				}
				idx = 0;
				g->SetCurrentIndex( 0 );
			}

			if ( !pFilter || pFilter->IsValidResponse( g->group[idx].GetType(), g->group[idx].value ) )
			{
				bFoundValid = true;
				break;
			}

		} while ( g->GetCurrentIndex() != initialIndex );

		if ( !bFoundValid )
			return false;
	}
	else
	{
		idx = SelectWeightedResponseFromResponseGroup( g, pFilter );
		if ( idx < 0 )
			return false;
	}

	if ( verbose )
	{
		DebugPrint( depth, "%s\n", m_Responses.GetElementName( responseIndex ) );
		DebugPrint( depth, "{\n" );
		DescribeResponseGroup( g, idx, depth );
	}

	bool bret = true;

	ParserResponse *result = &g->group[ idx ];
	if ( result->type == RESPONSE_RESPONSE )
	{
		// Recurse
		bret = ResolveResponse( searchResult, depth + 1, result->value, verbose, pFilter );
	}
	else
	{
		searchResult.action = result;
		searchResult.group	= g;
	}

	if( verbose )
	{
		DebugPrint( depth, "}\n" );
	}

	return bret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *group - 
//			selected - 
//			depth - 
//-----------------------------------------------------------------------------
void CResponseSystem::DescribeResponseGroup( ResponseGroup *group, int selected, int depth )
{
	int c = group->group.Count();

	for ( int i = 0; i < c ; i++ )
	{
		ParserResponse *r = &group->group[ i ];
		DebugPrint( depth + 1, "%s%20s : %40s %5.3f\n",
			i == selected ? "-> " : "   ",
			CRR_Response::DescribeResponse( r->GetType() ),
			r->value,
			r->weight.GetFloat() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *rule - 
// Output : CResponseSystem::Response
//-----------------------------------------------------------------------------
bool CResponseSystem::GetBestResponse( ResponseSearchResult& searchResult, Rule *rule, bool verbose /*=false*/, IResponseFilter *pFilter )
{
	int c = rule->m_Responses.Count();
	if ( !c )
		return false;

	int index = IEngineEmulator::Get()->GetRandomStream()->RandomInt( 0, c - 1 );
	int groupIndex = rule->m_Responses[ index ];

	ResponseGroup *g = &m_Responses[ groupIndex ];

	// Group has been disabled
	if ( !g->IsEnabled() )
		return false;

	int count = g->group.Count();
	if ( !count )
		return false;

	int responseIndex = 0;

	if ( g->IsSequential() )
	{
		// See if next index is valid
		int initialIndex = g->GetCurrentIndex();
		bool bFoundValid = false;

		do 
		{
			responseIndex = g->GetCurrentIndex();
			g->SetCurrentIndex( responseIndex + 1 );
			if ( responseIndex >= count )
			{
				if ( g->IsNoRepeat() )
				{
					g->SetEnabled( false );
#ifdef MAPBASE
					DisableEmptyRules();
#endif
					return false;
				}
				responseIndex = 0;
				g->SetCurrentIndex( 0 );
			}

			if ( !pFilter || pFilter->IsValidResponse( g->group[responseIndex].GetType(), g->group[responseIndex].value ) )
			{
				bFoundValid = true;
				break;
			}

		} while ( g->GetCurrentIndex() != initialIndex );

		if ( !bFoundValid )
			return false;
	}
	else
	{
		responseIndex = SelectWeightedResponseFromResponseGroup( g, pFilter );
		if ( responseIndex < 0 )
			return false;
	}


	ParserResponse *r = &g->group[ responseIndex ];

	int depth = 0;

	if ( verbose )
	{
		DebugPrint( depth, "%s\n", m_Responses.GetElementName( groupIndex ) );
		DebugPrint( depth, "{\n" );

		DescribeResponseGroup( g, responseIndex, depth );
	}

	bool bret = true;

	if ( r->type == RESPONSE_RESPONSE )
	{
		bret = ResolveResponse( searchResult, depth + 1, r->value, verbose, pFilter );
	}
	else
	{
		searchResult.action = r;
		searchResult.group	= g;
	}

	if ( verbose )
	{
		DebugPrint( depth, "}\n" );
	}

	return bret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
//			verbose - 
// Output : int
// Warning: If you change this, be sure to also change 
//          ResponseSystemImplementationCLI::FindAllRulesMatchingCriteria().
//-----------------------------------------------------------------------------
ResponseRulePartition::tIndex CResponseSystem::FindBestMatchingRule( const CriteriaSet& set, bool verbose, float &scoreOfBestMatchingRule )
{
	CUtlVector< ResponseRulePartition::tIndex >	bestrules(16,4);
	float bestscore = 0.001f;
	scoreOfBestMatchingRule = 0;

	CUtlVectorFixed< ResponseRulePartition::tRuleDict *, 2 > buckets( 0, 2 );
	m_RulePartitions.GetDictsForCriteria( &buckets, set );
	for ( int b = 0 ; b < buckets.Count() ; ++b )
	{
		ResponseRulePartition::tRuleDict *prules = buckets[b];
		int c = prules->Count();
	int i;
	for ( i = 0; i < c; i++ )
	{
			float score = ScoreCriteriaAgainstRule( set, *prules, i, verbose );
		// Check equals so that we keep track of all matching rules
		if ( score >= bestscore )
		{
			// Reset bucket
			if( score != bestscore )
			{
				bestscore = score;
				bestrules.RemoveAll();
			}

			// Add to bucket
				bestrules.AddToTail( m_RulePartitions.IndexFromDictElem( prules, i ) );
			}
		}
	}

	int bestCount = bestrules.Count();
	if ( bestCount <= 0 )
		return m_RulePartitions.InvalidIdx();

	scoreOfBestMatchingRule = bestscore ;
	if ( bestCount == 1 )
	{
		return bestrules[ 0 ] ;
	}
	else
	{
		// Randomly pick one of the tied matching rules
		int idx = IEngineEmulator::Get()->GetRandomStream()->RandomInt( 0, bestCount - 1 );
		if ( verbose )
		{
			CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "Found %i matching rules, selecting slot %i\n", bestCount, idx );
		}
		return bestrules[ idx ] ;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
// Output : CRR_Response
//-----------------------------------------------------------------------------
bool CResponseSystem::FindBestResponse( const CriteriaSet& set, CRR_Response& response, IResponseFilter *pFilter )
{
	bool valid = false;

	int iDbgResponse = rr_debugresponses.GetInt();
	bool showRules = ( iDbgResponse >= 2 && iDbgResponse < RR_DEBUGRESPONSES_SPECIALCASE );
	bool showResult = ( iDbgResponse >= 1 && iDbgResponse < RR_DEBUGRESPONSES_SPECIALCASE );

	// Look for match. verbose mode used to be at level 2, but disabled because the writers don't actually care for that info.
	float scoreOfBestRule;
	ResponseRulePartition::tIndex bestRule = FindBestMatchingRule( set, 
		( iDbgResponse >= 3 && iDbgResponse < RR_DEBUGRESPONSES_SPECIALCASE ), 
		scoreOfBestRule ); 

	ResponseType_t responseType = RESPONSE_NONE;
	ResponseParams rp;

	char ruleName[ 128 ];
	char responseName[ 128 ];
	const char *context;
#ifdef MAPBASE
	int contextflags;
#else
	bool bcontexttoworld;
#endif
	ruleName[ 0 ] = 0;
	responseName[ 0 ] = 0;
	context = NULL;
#ifdef MAPBASE
	contextflags = 0;
#else
	bcontexttoworld = false;
#endif
	if ( m_RulePartitions.IsValid( bestRule ) )
	{
		Rule * RESTRICT r = &m_RulePartitions[ bestRule ];

		ResponseSearchResult result;
		if ( GetBestResponse( result, r, showResult, pFilter ) )
		{
			Q_strncpy( responseName, result.action->value, sizeof( responseName ) );
			responseType = result.action->GetType();
			rp = result.action->params;
			rp.m_pFollowup = &result.action->m_followup;
		}

		Q_strncpy( ruleName, m_RulePartitions.GetElementName( bestRule ), sizeof( ruleName ) );

		// Disable the rule if it only allows for matching one time
		if ( r->IsMatchOnce() )
		{
			r->Disable();
		}
		context = r->GetContext();
#ifdef MAPBASE
		contextflags = r->GetContextFlags();

		// Sets the internal indices for the response to call back to later for prospective responses
		// (NOTE: Performance not tested; Be wary of turning off the m_bInProspective check!)
		if (m_bInProspective)
		{
			for ( int i = 0; i < (int)m_Responses.Count(); i++ )
			{
				if (&m_Responses[i] == result.group)
				{
					ResponseGroup &group = m_Responses[i];
					for ( int j = 0; j < group.group.Count(); j++)
					{
						if (&group.group[j] == result.action)
						{
							response.SetInternalIndices( i, j );
						}
					}
				}
			}
		}
#else
		bcontexttoworld = r->IsApplyContextToWorld();
#endif

		response.SetMatchScore(scoreOfBestRule);
		valid = true;
	}

#ifdef MAPBASE
	response.Init( responseType, responseName, rp, ruleName, context, contextflags );
#else
	response.Init( responseType, responseName, rp, ruleName, context, bcontexttoworld );
#endif

	if ( showResult )
	{
		/*
		// clipped -- chet doesn't really want this info
		if ( valid )
		{
		// Rescore the winner and dump to console
		ScoreCriteriaAgainstRule( set, bestRule, true );
		}
		*/


		if ( valid || showRules )
		{
			const char *pConceptFilter = rr_debugresponseconcept.GetString();
			// Describe the response, too
			if ( V_strlen(pConceptFilter) > 0 && !rr_debugresponseconcept.GetBool() )
			{	// filter for only one concept
				if ( V_stricmp(pConceptFilter, set.GetValue(set.FindCriterionIndex("concept")) ) == 0 )
				{
					response.Describe(&set);
				} // else don't print
			}
			else
			{
				// maybe we need to filter *out* some concepts
				if ( m_DebugExcludeList.IsValidIndex( m_DebugExcludeList.Head() ) )
				{
					// we are excluding at least one concept
					CRR_Concept test( set.GetValue(set.FindCriterionIndex("concept")) );
					if ( ! m_DebugExcludeList.IsValidIndex( m_DebugExcludeList.Find( test ) ) )
					{	// if not found in exclude list, then print
						response.Describe(&set);
					}
				}
				else
				{
					// describe everything
				response.Describe(&set);
			}
		}
	}
	}

	return valid;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CResponseSystem::GetAllResponses( CUtlVector<CRR_Response> *pResponses )
{
	for ( int i = 0; i < (int)m_Responses.Count(); i++ )
	{
		ResponseGroup &group = m_Responses[i];

		for ( int j = 0; j < group.group.Count(); j++)
		{
			ParserResponse &response = group.group[j];
			if ( response.type != RESPONSE_RESPONSE )
			{
				/*
				CRR_Response *pResponse = new CRR_Response;
				pResponse->Init( response.GetType(), response.value, CriteriaSet(), response.params, NULL, NULL, false );
				pResponses->AddToTail(pResponse);
				*/
				pResponses->Element(pResponses->AddToTail()).Init( response.GetType(), response.value, response.params, NULL, NULL, false );
			}
		}
	}
}

#ifdef MAPBASE
void CResponseSystem::MarkResponseAsUsed( short iGroup, short iWithinGroup )
{
	if (m_Responses.Count() > (unsigned int)iGroup)
	{
		ResponseGroup &group = m_Responses[iGroup];
		if (group.group.Count() > (int)iWithinGroup)
		{
			group.MarkResponseUsed( iWithinGroup );

			CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "Marked response %s (%i) used\n", group.group[iWithinGroup].value, iWithinGroup );
		}
	}
}
#endif

void CResponseSystem::ParseInclude()
{
	char includefile[ 256 ];
	ParseToken();

#ifdef MAPBASE
	char scriptfile[256];
	GetCurrentScript( scriptfile, sizeof( scriptfile ) );

	// Gets first path
	// (for example, an #include from a file in resource/script/resp will return resource)
	size_t len = strlen(scriptfile)-1;
	for (size_t i = 0; i < len; i++)
	{
		if (scriptfile[i] == CORRECT_PATH_SEPARATOR || scriptfile[i] == INCORRECT_PATH_SEPARATOR)
		{
			len = i;
		}
	}
	Q_strncpy(includefile, scriptfile, len+1);

	if (len+1 != strlen(scriptfile))
	{
		Q_strncat( includefile, "/", sizeof( includefile ) );
		Q_strncat( includefile, token, sizeof( includefile ) );
	}
	else
		includefile[0] = '\0';

	if (!includefile[0])
		Q_snprintf( includefile, sizeof( includefile ), "scripts/%s", token );
#else
	Q_snprintf( includefile, sizeof( includefile ), "scripts/%s", token );
#endif

	// check if the file is already included
	if ( m_IncludedFiles.Find( includefile ) != NULL )
	{
		return;
	}

	MEM_ALLOC_CREDIT();

	// Try and load it
	CUtlBuffer buf;
	if ( !IEngineEmulator::Get()->GetFilesystem()->ReadFile( includefile, "GAME", buf ) )
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "Unable to load #included script %s\n", includefile );
		return;
	}

	LoadFromBuffer( includefile, (const char *)buf.PeekGet() );
}

void CResponseSystem::LoadFromBuffer( const char *scriptfile, const char *buffer )
{
	COM_TimestampedLog( "CResponseSystem::LoadFromBuffer [%s] - Start", scriptfile );
	m_IncludedFiles.Allocate( scriptfile );
	PushScript( scriptfile, (unsigned char * )buffer );

	if( rr_dumpresponses.GetBool() )
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM,"Reading: %s\n", scriptfile );
	}

	while ( 1 )
	{
		ParseToken();
		if ( !token[0] )
		{
			break;
		}

		unsigned int hash = RR_HASH( token );
		bool bSuccess = Dispatch( token, hash, m_FileDispatch );
		if ( !bSuccess )
		{
			int byteoffset = m_ScriptStack[ 0 ].currenttoken - (const char *)m_ScriptStack[ 0 ].buffer;

			Error( "CResponseSystem::LoadFromBuffer:  Unknown entry type '%s', expecting 'response', 'criterion', 'enumeration' or 'rules' in file %s(offset:%i)\n", 
				token, scriptfile, byteoffset );
			break;
		}
	}

	if ( m_ScriptStack.Count() == 1 )
	{
		char cur[ 256 ];
		GetCurrentScript( cur, sizeof( cur ) );
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "CResponseSystem:  %s (%i rules, %i criteria, and %i responses)\n",
			cur, m_RulePartitions.Count(), m_Criteria.Count(), m_Responses.Count() );

		if( rr_dumpresponses.GetBool() )
		{
			DumpRules();
		}
	}

	PopScript();
	COM_TimestampedLog( "CResponseSystem::LoadFromBuffer [%s] - Finish", scriptfile );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CResponseSystem::LoadRuleSet( const char *basescript )
{
	float flStart = Plat_FloatTime();
	int length = 0;
	unsigned char *buffer = (unsigned char *)IEngineEmulator::Get()->LoadFileForMe( basescript, &length );
	if ( length <= 0 || !buffer )
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "CResponseSystem:  failed to load %s\n", basescript );
		return;
	}

	m_IncludedFiles.FreeAll();
	LoadFromBuffer( basescript, (const char *)buffer );

	IEngineEmulator::Get()->FreeFile( buffer );

	Assert( m_ScriptStack.Count() == 0 );
	float flEnd = Plat_FloatTime();
	COM_TimestampedLog( "CResponseSystem::LoadRuleSet took %f msec", 1000.0f * ( flEnd - flStart ) );
}

inline ResponseType_t ComputeResponseType( const char *s )
{
	switch ( s[ 0 ] )
	{
	default:
		break;
	case 's':
		switch ( s[ 1 ] )
		{
		default:
			break;
		case 'c':
		return RESPONSE_SCENE;
		case 'e':
		return RESPONSE_SENTENCE;
		case 'p':
		return RESPONSE_SPEAK;
	}
		break;
	case 'r':
		return RESPONSE_RESPONSE;
	case 'p':
		return RESPONSE_PRINT;
	case 'e':
		return RESPONSE_ENTITYIO;
#ifdef MAPBASE
	case 'v':
		if (*(s + 7) == '_')
			return RESPONSE_VSCRIPT_FILE;
		else
			return RESPONSE_VSCRIPT;
#endif
	}

	return RESPONSE_NONE;
}

void CResponseSystem::ParseResponse_Weight( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	ParseToken();
	newResponse.weight.SetFloat( (float)atof( token ) );
}

void CResponseSystem::ParseResponse_PreDelay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	ParseToken();
	rp->flags |= AI_ResponseParams::RG_DELAYBEFORESPEAK;
	rp->predelay.FromInterval( ReadInterval( token ) );
}

void CResponseSystem::ParseResponse_NoDelay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	ParseToken();
	rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
	rp->delay.start = 0;
	rp->delay.range = 0;
}

void CResponseSystem::ParseResponse_DefaultDelay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
	rp->delay.start = AIS_DEF_MIN_DELAY;
	rp->delay.range = ( AIS_DEF_MAX_DELAY - AIS_DEF_MIN_DELAY );
}

void CResponseSystem::ParseResponse_Delay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	ParseToken();
	rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
	rp->delay.FromInterval( ReadInterval( token ) );
}

void CResponseSystem::ParseResponse_SpeakOnce( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	rp->flags |= AI_ResponseParams::RG_SPEAKONCE;
}

void CResponseSystem::ParseResponse_NoScene( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	rp->flags |= AI_ResponseParams::RG_DONT_USE_SCENE;
}

void CResponseSystem::ParseResponse_StopOnNonIdle( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	rp->flags |= AI_ResponseParams::RG_STOP_ON_NONIDLE;
}

void CResponseSystem::ParseResponse_Odds( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	ParseToken();
	rp->flags |= AI_ResponseParams::RG_ODDS;
	rp->odds = clamp( atoi( token ), 0, 100 );
}

void CResponseSystem::ParseResponse_RespeakDelay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	ParseToken();
	rp->flags |= AI_ResponseParams::RG_RESPEAKDELAY;
	rp->respeakdelay.FromInterval( ReadInterval( token ) );
}

void CResponseSystem::ParseResponse_WeaponDelay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	ParseToken();
	rp->flags |= AI_ResponseParams::RG_WEAPONDELAY;
	rp->weapondelay.FromInterval( ReadInterval( token ) );
}

void CResponseSystem::ParseResponse_Soundlevel( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	ParseToken();
	rp->flags |= AI_ResponseParams::RG_SOUNDLEVEL;
	rp->soundlevel = (soundlevel_t)TextToSoundLevel( token );
}

void CResponseSystem::ParseResponse_DisplayFirst( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	newResponse.first = true;
	group.m_bHasFirst = true;
}

void CResponseSystem::ParseResponse_DisplayLast( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	newResponse.last = true;
	group.m_bHasLast= true;
}

void CResponseSystem::ParseResponse_Fire( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	// get target name
	bool bSuc = ParseToken();
	if (!bSuc)
	{
		ResponseWarning( "FIRE token in response needs exactly three parameters." );
		return;
	}
	newResponse.m_followup.followup_entityiotarget = ResponseCopyString(token);

	bSuc = ParseToken();
	if (!bSuc)
	{
		ResponseWarning( "FIRE token in response needs exactly three parameters." );
		return;
	}
	newResponse.m_followup.followup_entityioinput = ResponseCopyString(token);

	bSuc = ParseToken();
	if (!bSuc)
	{
		ResponseWarning( "FIRE token in response needs exactly three parameters." );
		return;
	}
	newResponse.m_followup.followup_entityiodelay = atof( token );
	/*
	m_followup.followup_entityioinput = ResponseCopyString(src.m_followup.followup_entityioinput);
	m_followup.followup_entityiotarget = ResponseCopyString(src.m_followup.followup_entityiotarget);
	*/
}

void CResponseSystem::ParseResponse_Then( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	// eg, "subject TALK_ANSWER saidunplant:1 3"
	bool bSuc = ParseToken();
	if (!bSuc)
	{
		AssertMsg(false, "THEN token in response lacked any further info.\n");
		ResponseWarning( "THEN token in response lacked any further info.\n" );
		return;
	}

	newResponse.m_followup.followup_target = ResponseCopyString(token);

	bSuc = ParseToken(); // get another token
	if (!bSuc)
	{
		AssertMsg1(false, "THEN token in response had a target '%s', but lacked any further info.\n", newResponse.m_followup.followup_target );
		ResponseWarning( "THEN token in response had a target '%s', but lacked any further info.\n", newResponse.m_followup.followup_target );
		return;
	}

	newResponse.m_followup.followup_concept = ResponseCopyString( token );


	// Okay, this is totally asinine.
	// Because the ParseToken() function will split foo:bar into three tokens
	// (which is reasonable), but we have no safe way to parse the file otherwise
	// because it's all behind an engine interface, it's necessary to parse all
	// the tokens to the end of the line and catenate them, except for the last one
	// which is the delay. That's crap.
	bSuc = ParseToken();
	if (!bSuc)
	{
		AssertMsg(false, "THEN token in response lacked contexts.\n");
		ResponseWarning( "THEN token in response lacked contexts.\n" );
		return;
	}

	// okay, as long as there is at least one more token, catenate the ones we
	// see onto a temporary buffer. When we're down to the last token, that is
	// the delay.
	char buf[4096];
	buf[0] = '\0';
	while ( TokenWaiting() )
	{
		Q_strncat( buf, token, 4096 );
		bSuc = ParseToken();
		AssertMsg(bSuc, "Token parsing mysteriously failed.");
	}

	// down here, token is the last token, and buf is everything up to there.
	newResponse.m_followup.followup_contexts = ResponseCopyString( buf );

	newResponse.m_followup.followup_delay = atof( token );
}

void CResponseSystem::ParseOneResponse( const char *responseGroupName, ResponseGroup& group, ResponseParams *defaultParams )
{
	ParserResponse &newResponse = group.group[ group.group.AddToTail() ];
	newResponse.weight.SetFloat( 1.0f );
	// inherit from group if appropriate
	if (defaultParams)
	{
		newResponse.params = *defaultParams;
	}

	ResponseParams *rp = &newResponse.params;

	newResponse.type = ComputeResponseType( token );
	if ( RESPONSE_NONE == newResponse.type )
{
		ResponseWarning( "response entry '%s' with unknown response type '%s'\n", responseGroupName, token );
		return;
}

#ifdef MAPBASE
	// HACKHACK: Some response system usage in the pre-Alien Swarm system require response names to preserve casing or even have escaped quotes.
	ParseTokenIntact();
#else
	ParseToken();
#endif
	newResponse.value = ResponseCopyString( token );

	while ( TokenWaiting() )
	{
		ParseToken();

		unsigned int hash = RR_HASH( token );
		if ( DispatchParseResponse( token, hash, m_ResponseDispatch, newResponse, group, rp ) )
		{
			continue;
		}

		ResponseWarning( "response entry '%s' with unknown command '%s'\n", responseGroupName, token );
	}

}

void CResponseSystem::ParseResponseGroup_Start( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
			while ( 1 )
			{
				ParseToken();
				if ( !Q_stricmp( token, "}" ) )
					break;

				if ( !Q_stricmp( token, "permitrepeats" ) )
				{
					newGroup.m_bDepleteBeforeRepeat = false;
					continue;
				}
				else if ( !Q_stricmp( token, "sequential" ) )
				{
					newGroup.SetSequential( true );
					continue;
				}
				else if ( !Q_stricmp( token, "norepeat" ) )
				{
					newGroup.SetNoRepeat( true );
					continue;
				}

				ParseOneResponse( responseGroupName, newGroup, &groupResponseParams );
			}
		}

void CResponseSystem::ParseResponseGroup_PreDelay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
			ParseToken();
	groupResponseParams.flags |= AI_ResponseParams::RG_DELAYBEFORESPEAK;
			groupResponseParams.predelay.FromInterval( ReadInterval( token ) );
		}

void CResponseSystem::ParseResponseGroup_NoDelay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
			ParseToken();
	groupResponseParams.flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			groupResponseParams.delay.start = 0;
			groupResponseParams.delay.range = 0;
		}

void CResponseSystem::ParseResponseGroup_DefaultDelay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
	groupResponseParams.flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			groupResponseParams.delay.start = AIS_DEF_MIN_DELAY;
			groupResponseParams.delay.range = ( AIS_DEF_MAX_DELAY - AIS_DEF_MIN_DELAY );
		}

void CResponseSystem::ParseResponseGroup_Delay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
			ParseToken();
	groupResponseParams.flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			groupResponseParams.delay.FromInterval( ReadInterval( token ) );
		}

void CResponseSystem::ParseResponseGroup_SpeakOnce( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
	groupResponseParams.flags |= AI_ResponseParams::RG_SPEAKONCE;
		}

void CResponseSystem::ParseResponseGroup_NoScene( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
	groupResponseParams.flags |= AI_ResponseParams::RG_DONT_USE_SCENE;
		}

void CResponseSystem::ParseResponseGroup_StopOnNonIdle( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
	groupResponseParams.flags |= AI_ResponseParams::RG_STOP_ON_NONIDLE;
		}

void CResponseSystem::ParseResponseGroup_Odds( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
			ParseToken();
	groupResponseParams.flags |= AI_ResponseParams::RG_ODDS;
			groupResponseParams.odds = clamp( atoi( token ), 0, 100 );
		}

void CResponseSystem::ParseResponseGroup_RespeakDelay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
			ParseToken();
	groupResponseParams.flags |= AI_ResponseParams::RG_RESPEAKDELAY;
			groupResponseParams.respeakdelay.FromInterval( ReadInterval( token ) );
		}

void CResponseSystem::ParseResponseGroup_WeaponDelay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
			ParseToken();
	groupResponseParams.flags |= AI_ResponseParams::RG_WEAPONDELAY;
			groupResponseParams.weapondelay.FromInterval( ReadInterval( token ) );
		}

void CResponseSystem::ParseResponseGroup_Soundlevel( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams )
		{
			ParseToken();
	groupResponseParams.flags |= AI_ResponseParams::RG_SOUNDLEVEL;
			groupResponseParams.soundlevel = (soundlevel_t)TextToSoundLevel( token );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CResponseSystem::ParseResponse( void )
{
	AI_ResponseParams groupResponseParams; // default response parameters inherited from single line format for group

	// Should have groupname at start
	ParseToken();
	char responseGroupName[ 128 ];
	Q_strncpy( responseGroupName, token, sizeof( responseGroupName ) );

	int slot = m_Responses.Insert( responseGroupName );
	ResponseGroup &newGroup = m_Responses[ slot ];

	while ( 1 )
	{
#ifdef MAPBASE
		if ( !ParseToken() || !Q_stricmp( token, "}" ) )
		{
			break;
		}
#else
		ParseToken();
#endif

		unsigned int hash = RR_HASH( token );

		// Oops, part of next definition
		if( IsRootCommand( hash ) )
		{
			Unget();
			break;
		}

		if ( DispatchParseResponseGroup( token, hash, m_ResponseGroupDispatch, responseGroupName, newGroup, groupResponseParams ) )
		{
			continue;
		}

		ParseOneResponse( responseGroupName, newGroup, &groupResponseParams );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *criterion - 
//-----------------------------------------------------------------------------
int CResponseSystem::ParseOneCriterion( const char *criterionName )
{
	char key[ 128 ];
	char value[ 128 ];

	Criteria *pNewCriterion = NULL;

	int idx;
#ifdef MAPBASE
	short existing = m_Criteria.Find( criterionName );
	if ( existing != m_Criteria.InvalidIndex() )
	{
		//ResponseWarning( "Additional definition for criteria '%s', overwriting\n", criterionName );
		m_Criteria[existing] = Criteria();
		m_Criteria.SetElementName(existing, criterionName);
		idx = existing;
		pNewCriterion = &m_Criteria[ idx ];
	}
#else
	if ( m_Criteria.Find( criterionName ) != m_Criteria.InvalidIndex() )
	{
		static Criteria dummy;
		pNewCriterion = &dummy;

		ResponseWarning( "Multiple definitions for criteria '%s' [%d]\n", criterionName, RR_HASH( criterionName ) );
		idx = m_Criteria.InvalidIndex();
	}
#endif
	else
	{
		idx = m_Criteria.Insert( criterionName );
		pNewCriterion = &m_Criteria[ idx ];
	}

	bool gotbody = false;

	while ( TokenWaiting() || !gotbody )
	{
#ifdef MAPBASE
		if ( !ParseToken() )
		{
			break;
		}
#else
		ParseToken();
#endif

		// Oops, part of next definition
		if( IsRootCommand() )
		{
			Unget();
			break;
		}

		if ( !Q_stricmp( token, "{" ) )
		{
			gotbody = true;

			while ( 1 )
			{
				ParseToken();
				if ( !Q_stricmp( token, "}" ) )
					break;

				// Look up subcriteria index
				int idx = m_Criteria.Find( token );
				if ( idx != m_Criteria.InvalidIndex() )
				{
					pNewCriterion->subcriteria.AddToTail( idx );
				}
				else
				{
					ResponseWarning( "Skipping unrecongized subcriterion '%s' in '%s'\n", token, criterionName );
				}
			}
			continue;
		}
		else if ( !Q_stricmp( token, "required" ) )
		{
			pNewCriterion->required = true;
		}
		else if ( !Q_stricmp( token, "weight" ) )
		{
			ParseToken();
			pNewCriterion->weight.SetFloat( (float)atof( token ) );
		}
		else
		{
			Assert( pNewCriterion->subcriteria.Count() == 0 );

			// Assume it's the math info for a non-subcriteria resposne
			Q_strncpy( key, token, sizeof( key ) );
			ParseToken();
			Q_strncpy( value, token, sizeof( value ) );

			V_strlower( key );
			pNewCriterion->nameSym = CriteriaSet::ComputeCriteriaSymbol( key );
			pNewCriterion->value = ResponseCopyString( value );

			gotbody = true;
		}
	}

	if ( !pNewCriterion->IsSubCriteriaType() )
	{
		ComputeMatcher( pNewCriterion, pNewCriterion->matcher );
	}

	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *kv - 
//-----------------------------------------------------------------------------
void CResponseSystem::ParseCriterion( void )
{
	// Should have groupname at start
	char criterionName[ 128 ];
	ParseToken();
	Q_strncpy( criterionName, token, sizeof( criterionName ) );

	ParseOneCriterion( criterionName );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *kv - 
//-----------------------------------------------------------------------------
void CResponseSystem::ParseEnumeration( void )
{
	char enumerationName[ 128 ];
	ParseToken();
	Q_strncpy( enumerationName, token, sizeof( enumerationName ) );

	ParseToken();
	if ( Q_stricmp( token, "{" ) )
	{
		ResponseWarning( "Expecting '{' in enumeration '%s', got '%s'\n", enumerationName, token );
		return;
	}

	while ( 1 )
	{
		ParseToken();
		if ( !Q_stricmp( token, "}" ) )
			break;

		if ( Q_strlen( token ) <= 0 )
		{
			ResponseWarning( "Expecting more tokens in enumeration '%s'\n", enumerationName );
			break;
		}

		char key[ 128 ];

		Q_strncpy( key, token, sizeof( key ) );
		ParseToken();
		float value = (float)atof( token );

		char sz[ 128 ];
		Q_snprintf( sz, sizeof( sz ), "[%s::%s]", enumerationName, key );
		Q_strlower( sz );

		Enumeration newEnum;
		newEnum.value = value;

		if ( m_Enumerations.Find( sz ) == m_Enumerations.InvalidIndex() )
		{
			m_Enumerations.Insert( sz, newEnum );
		}
		/*
		else
		{
		ResponseWarning( "Ignoring duplication enumeration '%s'\n", sz );
		}
		*/
	}
}

void CResponseSystem::ParseRule_MatchOnce( Rule &newRule )
		{
			newRule.m_bMatchOnce = true;
		}

#ifdef MAPBASE
void CResponseSystem::ParseRule_ApplyContextToWorld( Rule &newRule )
		{
			newRule.m_iContextFlags |= APPLYCONTEXT_WORLD;
		}

void CResponseSystem::ParseRule_ApplyContextToSquad( Rule &newRule )
		{
			newRule.m_iContextFlags |= APPLYCONTEXT_SQUAD;
		}

void CResponseSystem::ParseRule_ApplyContextToEnemy( Rule &newRule )
		{
			newRule.m_iContextFlags |= APPLYCONTEXT_ENEMY;
		}
#else
void CResponseSystem::ParseRule_ApplyContextToWorld( Rule &newRule )
		{
			newRule.m_bApplyContextToWorld = true;
		}
#endif

void CResponseSystem::ParseRule_ApplyContext( Rule &newRule )
		{
			ParseToken();
			if ( newRule.GetContext() == NULL )
			{
				newRule.SetContext( token );
			}
			else
			{
				CFmtStrN<1024> newContext( "%s,%s", newRule.GetContext(), token );
				newRule.SetContext( newContext );
			}
		}

void CResponseSystem::ParseRule_Response( Rule &newRule )
		{
			// Read them until we run out.
			while ( TokenWaiting() )
			{
				ParseToken();
				int idx = m_Responses.Find( token );
				if ( idx != m_Responses.InvalidIndex() )
				{
					MEM_ALLOC_CREDIT();
					newRule.m_Responses.AddToTail( idx );
				}
				else
				{
			m_bParseRuleValid = false;
			ResponseWarning( "No such response '%s' for rule '%s'\n", token, m_pParseRuleName );
		}
	}
}

/*
void CResponseSystem::ParseRule_ForceWeight( Rule &newRule )
{
	ParseToken();
	if ( token[0] == 0 )
	{
		// no token followed forceweight?
		ResponseWarning( "Forceweight token in rule '%s' did not specify a numerical weight! Ignoring.\n", m_pParseRuleName );
	}
	else
	{
		newRule.m_nForceWeight = atoi(token);
		if ( newRule.m_nForceWeight == 0 )
		{
			ResponseWarning( "Rule '%s' had forceweight '%s', which doesn't work out to a nonzero number. Ignoring.\n", 
				m_pParseRuleName, token );
				}
			}
		}
*/

void CResponseSystem::ParseRule_Criteria( Rule &newRule )
		{
			// Read them until we run out.
			while ( TokenWaiting() )
			{
				ParseToken();

				int idx = m_Criteria.Find( token );
				if ( idx != m_Criteria.InvalidIndex() )
				{
					MEM_ALLOC_CREDIT();
					newRule.m_Criteria.AddToTail( idx );
				}
				else
				{
			m_bParseRuleValid = false;
			ResponseWarning( "No such criterion '%s' for rule '%s'\n", token, m_pParseRuleName );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *kv - 
//-----------------------------------------------------------------------------
void CResponseSystem::ParseRule( void )
{
	static int instancedCriteria = 0;

	char ruleName[ 128 ];
	ParseToken();
	Q_strncpy( ruleName, token, sizeof( ruleName ) );

	ParseToken();
	if ( Q_stricmp( token, "{" ) )
	{
		ResponseWarning( "Expecting '{' in rule '%s', got '%s'\n", ruleName, token );
		return;
				}

	// entries are "criteria", "response" or an in-line criteria to instance
	Rule *newRule = new Rule;

	char sz[ 128 ];

	m_bParseRuleValid = true;
	m_pParseRuleName = ruleName;
	while ( 1 )
	{
		ParseToken();
		if ( !Q_stricmp( token, "}" ) )
		{
			break;
		}

		if ( Q_strlen( token ) <= 0 )
		{
			ResponseWarning( "Expecting more tokens in rule '%s'\n", ruleName );
			break;
			}

		unsigned int hash = RR_HASH( token );
		if ( DispatchParseRule( token, hash, m_RuleDispatch, *newRule ) )
			continue;

		// It's an inline criteria, generate a name and parse it in
		Q_snprintf( sz, sizeof( sz ), "[%s%03i]", ruleName, ++instancedCriteria );
		Unget();
		int idx = ParseOneCriterion( sz );
		if ( idx != m_Criteria.InvalidIndex() )
		{
			newRule->m_Criteria.AddToTail( idx );
		}
	}

	if ( m_bParseRuleValid )
	{
		m_RulePartitions.GetDictForRule( this, newRule ).Insert( ruleName, newRule );
	}
	else
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "Discarded rule %s\n", ruleName );
		delete newRule;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CResponseSystem::GetCurrentToken() const
{
	if ( m_ScriptStack.Count() <= 0 )
		return -1;

	return m_ScriptStack[ 0 ].tokencount;
}


void CResponseSystem::ResponseWarning( const char *fmt, ... )
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr, fmt);
	Q_vsnprintf(string, sizeof(string), fmt,argptr);
	va_end (argptr);

	char cur[ 256 ];
	GetCurrentScript( cur, sizeof( cur ) );
	CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "%s(token %i) : %s", cur, GetCurrentToken(), string );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CResponseSystem::CopyCriteriaFrom( Rule *pSrcRule, Rule *pDstRule, CResponseSystem *pCustomSystem )
{
	// Add criteria from this rule to global list in custom response system.
	int nCriteriaCount = pSrcRule->m_Criteria.Count();
	for ( int iCriteria = 0; iCriteria < nCriteriaCount; ++iCriteria )
	{
		int iSrcIndex = pSrcRule->m_Criteria[iCriteria];
		Criteria *pSrcCriteria = &m_Criteria[iSrcIndex];
		if ( pSrcCriteria )
		{
			int iIndex = pCustomSystem->m_Criteria.Find( m_Criteria.GetElementName( iSrcIndex ) );
			if ( iIndex != pCustomSystem->m_Criteria.InvalidIndex() )
			{
				pDstRule->m_Criteria.AddToTail( iIndex );
				continue;
			}

			// Add the criteria.
			Criteria dstCriteria;

			dstCriteria.nameSym = pSrcCriteria->nameSym ;
			dstCriteria.value = ResponseCopyString( pSrcCriteria->value );
			dstCriteria.weight = pSrcCriteria->weight;
			dstCriteria.required = pSrcCriteria->required;
			dstCriteria.matcher = pSrcCriteria->matcher;

			int nSubCriteriaCount = pSrcCriteria->subcriteria.Count();
			for ( int iSubCriteria = 0; iSubCriteria < nSubCriteriaCount; ++iSubCriteria )
			{
				int iSrcSubIndex = pSrcCriteria->subcriteria[iSubCriteria];
				Criteria *pSrcSubCriteria = &m_Criteria[iSrcSubIndex];
				if ( pSrcCriteria )
				{
					int iSubIndex = pCustomSystem->m_Criteria.Find( pSrcSubCriteria->value );
					if ( iSubIndex != pCustomSystem->m_Criteria.InvalidIndex() )
						continue;

					// Add the criteria.
					Criteria dstSubCriteria;

					dstSubCriteria.nameSym = pSrcSubCriteria->nameSym ;
					dstSubCriteria.value = ResponseCopyString( pSrcSubCriteria->value );
					dstSubCriteria.weight = pSrcSubCriteria->weight;
					dstSubCriteria.required = pSrcSubCriteria->required;
					dstSubCriteria.matcher = pSrcSubCriteria->matcher;

					int iSubInsertIndex = pCustomSystem->m_Criteria.Insert( pSrcSubCriteria->value, dstSubCriteria );
					dstCriteria.subcriteria.AddToTail( iSubInsertIndex );
				}
			}

			int iInsertIndex = pCustomSystem->m_Criteria.Insert( m_Criteria.GetElementName( iSrcIndex ), dstCriteria );
			pDstRule->m_Criteria.AddToTail( iInsertIndex );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CResponseSystem::CopyResponsesFrom( Rule *pSrcRule, Rule *pDstRule, CResponseSystem *pCustomSystem )
{
	// Add responses from this rule to global list in custom response system.
	int nResponseGroupCount = pSrcRule->m_Responses.Count();
	for ( int iResponseGroup = 0; iResponseGroup < nResponseGroupCount; ++iResponseGroup )
	{
		int iSrcResponseGroup = pSrcRule->m_Responses[iResponseGroup];
		ResponseGroup *pSrcResponseGroup = &m_Responses[iSrcResponseGroup];
		if ( pSrcResponseGroup )
		{
			// Add response group.			
			ResponseGroup dstResponseGroup;

			dstResponseGroup.m_bDepleteBeforeRepeat = pSrcResponseGroup->m_bDepleteBeforeRepeat;
			dstResponseGroup.m_nDepletionCount = pSrcResponseGroup->m_nDepletionCount;
			dstResponseGroup.m_bHasFirst = pSrcResponseGroup->m_bHasFirst;
			dstResponseGroup.m_bHasLast = pSrcResponseGroup->m_bHasLast;
			dstResponseGroup.m_bSequential = pSrcResponseGroup->m_bSequential;
			dstResponseGroup.m_bNoRepeat = pSrcResponseGroup->m_bNoRepeat;
			dstResponseGroup.m_bEnabled = pSrcResponseGroup->m_bEnabled;
			dstResponseGroup.m_nCurrentIndex = pSrcResponseGroup->m_nCurrentIndex;

			int nSrcResponseCount = pSrcResponseGroup->group.Count();
			for ( int iResponse = 0; iResponse < nSrcResponseCount; ++iResponse )
			{
				ParserResponse *pSrcResponse = &pSrcResponseGroup->group[iResponse];
				if ( pSrcResponse )
				{
					// Add Response
					ParserResponse dstResponse;

					dstResponse.weight = pSrcResponse->weight;
					dstResponse.type = pSrcResponse->type;
					dstResponse.value = ResponseCopyString( pSrcResponse->value );
					dstResponse.depletioncount = pSrcResponse->depletioncount;
					dstResponse.first = pSrcResponse->first;
					dstResponse.last = pSrcResponse->last;

					dstResponseGroup.group.AddToTail( dstResponse );
				}
			}

			int iInsertIndex = pCustomSystem->m_Responses.Insert( m_Responses.GetElementName( iSrcResponseGroup ), dstResponseGroup );
			pDstRule->m_Responses.AddToTail( iInsertIndex );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CResponseSystem::CopyEnumerationsFrom( CResponseSystem *pCustomSystem )
{
	int nEnumerationCount = m_Enumerations.Count();
	for ( int iEnumeration = 0; iEnumeration < nEnumerationCount; ++iEnumeration )
	{
		Enumeration *pSrcEnumeration = &m_Enumerations[iEnumeration];
		if ( pSrcEnumeration )
		{
			Enumeration dstEnumeration;
			dstEnumeration.value = pSrcEnumeration->value;
			pCustomSystem->m_Enumerations.Insert( m_Enumerations.GetElementName( iEnumeration ), dstEnumeration );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CResponseSystem::CopyRuleFrom( Rule *pSrcRule, ResponseRulePartition::tIndex iRule, CResponseSystem *pCustomSystem )
{
	// Verify data.
	Assert( pSrcRule );
	Assert( pCustomSystem );
	if ( !pSrcRule || !pCustomSystem )
		return;

	// New rule
	Rule *dstRule = new Rule;

	dstRule->SetContext( pSrcRule->GetContext() );
	dstRule->m_bMatchOnce = pSrcRule->m_bMatchOnce;
	dstRule->m_bEnabled = pSrcRule->m_bEnabled;
#ifdef MAPBASE
	dstRule->m_iContextFlags = pSrcRule->m_iContextFlags;
#else
	dstRule->m_bApplyContextToWorld = pSrcRule->m_bApplyContextToWorld;
#endif

	// Copy off criteria.
	CopyCriteriaFrom( pSrcRule, dstRule, pCustomSystem );

	// Copy off responses.
	CopyResponsesFrom( pSrcRule, dstRule, pCustomSystem );

	// Copy off enumerations - Don't think we use these.
	//	CopyEnumerationsFrom( pCustomSystem );

	// Add rule.
	pCustomSystem->m_RulePartitions.GetDictForRule( this, dstRule ).Insert( m_RulePartitions.GetElementName( iRule ), dstRule );
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CResponseSystem::DumpRules()
{
	for ( ResponseRulePartition::tIndex idx = m_RulePartitions.First() ;
		m_RulePartitions.IsValid(idx) ;
		idx = m_RulePartitions.Next(idx) )
	{
		CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "%s\n", m_RulePartitions.GetElementName( idx ) );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CResponseSystem::DumpDictionary( const char *pszName )
{
	CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "\nDictionary: %s\n", pszName );

	// int nRuleCount = m_Rules.Count();
	// for ( int iRule = 0; iRule < nRuleCount; ++iRule )
	for ( ResponseRulePartition::tIndex idx = m_RulePartitions.First() ;
		m_RulePartitions.IsValid(idx) ;
		idx = m_RulePartitions.Next(idx) )
	{
		CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "	Rule %d/%d: %s\n", m_RulePartitions.BucketFromIdx( idx ), m_RulePartitions.PartFromIdx( idx ), m_RulePartitions.GetElementName( idx ) );

		Rule *pRule = &m_RulePartitions[idx];

		int nCriteriaCount = pRule->m_Criteria.Count();
		for( int iCriteria = 0; iCriteria < nCriteriaCount; ++iCriteria )
		{
			int iRuleCriteria = pRule->m_Criteria[iCriteria];
			Criteria *pCriteria = &m_Criteria[iRuleCriteria];
			CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "		Criteria %d: %s %s\n", iCriteria, CriteriaSet::SymbolToStr( pCriteria->nameSym ), pCriteria->value );
		}

		int nResponseGroupCount = pRule->m_Responses.Count();
		for ( int iResponseGroup = 0; iResponseGroup < nResponseGroupCount; ++iResponseGroup )
		{
			int iRuleResponse = pRule->m_Responses[iResponseGroup];
			ResponseGroup *pResponseGroup = &m_Responses[iRuleResponse];

			CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "		ResponseGroup %d: %s\n", iResponseGroup, m_Responses.GetElementName( iRuleResponse ) );

			int nResponseCount = pResponseGroup->group.Count();
			for ( int iResponse = 0; iResponse < nResponseCount; ++iResponse )
			{
				ParserResponse *pResponse = &pResponseGroup->group[iResponse];
				CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "			Response %d: %s\n", iResponse, pResponse->value );
			}
		}
	}
}

void CResponseSystem::BuildDispatchTables()
{
	m_RootCommandHashes.Insert( RR_HASH( "#include" ) );
	m_RootCommandHashes.Insert( RR_HASH( "response" ) );
	m_RootCommandHashes.Insert( RR_HASH( "enumeration" ) );
	m_RootCommandHashes.Insert( RR_HASH( "criterion" ) );
	m_RootCommandHashes.Insert( RR_HASH( "criteria" ) );
	m_RootCommandHashes.Insert( RR_HASH( "rule" ) );

	m_FileDispatch.Insert( RR_HASH( "#include" ), &CResponseSystem::ParseInclude );
	m_FileDispatch.Insert( RR_HASH( "response" ), &CResponseSystem::ParseResponse );
	m_FileDispatch.Insert( RR_HASH( "criterion" ), &CResponseSystem::ParseCriterion );
	m_FileDispatch.Insert( RR_HASH( "criteria" ), &CResponseSystem::ParseCriterion );
	m_FileDispatch.Insert( RR_HASH( "rule" ), &CResponseSystem::ParseRule );
	m_FileDispatch.Insert( RR_HASH( "enumeration" ), &CResponseSystem::ParseEnumeration );

	m_RuleDispatch.Insert( RR_HASH( "matchonce" ), &CResponseSystem::ParseRule_MatchOnce );
	m_RuleDispatch.Insert( RR_HASH( "applycontexttoworld" ), &CResponseSystem::ParseRule_ApplyContextToWorld );
#ifdef MAPBASE
	m_RuleDispatch.Insert( RR_HASH( "applycontexttosquad" ), &CResponseSystem::ParseRule_ApplyContextToSquad );
	m_RuleDispatch.Insert( RR_HASH( "applycontexttoenemy" ), &CResponseSystem::ParseRule_ApplyContextToEnemy );
#endif
	m_RuleDispatch.Insert( RR_HASH( "applycontext" ), &CResponseSystem::ParseRule_ApplyContext );
	m_RuleDispatch.Insert( RR_HASH( "response" ), &CResponseSystem::ParseRule_Response );
//	m_RuleDispatch.Insert( RR_HASH( "forceweight" ), &CResponseSystem::ParseRule_ForceWeight );
	m_RuleDispatch.Insert( RR_HASH( "criteria" ), &CResponseSystem::ParseRule_Criteria );
	m_RuleDispatch.Insert( RR_HASH( "criterion" ), &CResponseSystem::ParseRule_Criteria );


	m_ResponseDispatch.Insert( RR_HASH( "weight" ), &CResponseSystem::ParseResponse_Weight );
	m_ResponseDispatch.Insert( RR_HASH( "predelay" ), &CResponseSystem::ParseResponse_PreDelay );
	m_ResponseDispatch.Insert( RR_HASH( "nodelay" ), &CResponseSystem::ParseResponse_NoDelay );
	m_ResponseDispatch.Insert( RR_HASH( "defaultdelay" ), &CResponseSystem::ParseResponse_DefaultDelay );
	m_ResponseDispatch.Insert( RR_HASH( "delay" ), &CResponseSystem::ParseResponse_Delay );
	m_ResponseDispatch.Insert( RR_HASH( "speakonce" ), &CResponseSystem::ParseResponse_SpeakOnce );
	m_ResponseDispatch.Insert( RR_HASH( "noscene" ), &CResponseSystem::ParseResponse_NoScene );
	m_ResponseDispatch.Insert( RR_HASH( "stop_on_nonidle" ), &CResponseSystem::ParseResponse_StopOnNonIdle );
	m_ResponseDispatch.Insert( RR_HASH( "odds" ), &CResponseSystem::ParseResponse_Odds );
	m_ResponseDispatch.Insert( RR_HASH( "respeakdelay" ), &CResponseSystem::ParseResponse_RespeakDelay );
	m_ResponseDispatch.Insert( RR_HASH( "weapondelay" ), &CResponseSystem::ParseResponse_WeaponDelay );
	m_ResponseDispatch.Insert( RR_HASH( "soundlevel" ), &CResponseSystem::ParseResponse_Soundlevel );
	m_ResponseDispatch.Insert( RR_HASH( "displayfirst" ), &CResponseSystem::ParseResponse_DisplayFirst );
	m_ResponseDispatch.Insert( RR_HASH( "displaylast" ), &CResponseSystem::ParseResponse_DisplayLast );
	m_ResponseDispatch.Insert( RR_HASH( "fire" ), &CResponseSystem::ParseResponse_Fire );
	m_ResponseDispatch.Insert( RR_HASH( "then" ), &CResponseSystem::ParseResponse_Then );

	m_ResponseGroupDispatch.Insert( RR_HASH( "{" ), &CResponseSystem::ParseResponseGroup_Start );
	m_ResponseGroupDispatch.Insert( RR_HASH( "predelay" ), &CResponseSystem::ParseResponseGroup_PreDelay );
	m_ResponseGroupDispatch.Insert( RR_HASH( "nodelay" ), &CResponseSystem::ParseResponseGroup_NoDelay );
	m_ResponseGroupDispatch.Insert( RR_HASH( "defaultdelay" ), &CResponseSystem::ParseResponseGroup_DefaultDelay );
	m_ResponseGroupDispatch.Insert( RR_HASH( "delay" ), &CResponseSystem::ParseResponseGroup_Delay );
	m_ResponseGroupDispatch.Insert( RR_HASH( "speakonce" ), &CResponseSystem::ParseResponseGroup_SpeakOnce );
	m_ResponseGroupDispatch.Insert( RR_HASH( "noscene" ), &CResponseSystem::ParseResponseGroup_NoScene );
	m_ResponseGroupDispatch.Insert( RR_HASH( "stop_on_nonidle" ), &CResponseSystem::ParseResponseGroup_StopOnNonIdle );
	m_ResponseGroupDispatch.Insert( RR_HASH( "odds" ), &CResponseSystem::ParseResponseGroup_Odds );
	m_ResponseGroupDispatch.Insert( RR_HASH( "respeakdelay" ), &CResponseSystem::ParseResponseGroup_RespeakDelay );
	m_ResponseGroupDispatch.Insert( RR_HASH( "weapondelay" ), &CResponseSystem::ParseResponseGroup_WeaponDelay );
	m_ResponseGroupDispatch.Insert( RR_HASH( "soundlevel" ), &CResponseSystem::ParseResponseGroup_Soundlevel );
}

bool CResponseSystem::Dispatch( char const *pToken, unsigned int uiHash, CResponseSystem::DispatchMap_t &rMap )
{
	int slot = rMap.Find( uiHash );
	if ( slot != rMap.InvalidIndex() )
	{
		CResponseSystem::pfnResponseDispatch dispatch = rMap[ slot ];
		(this->*dispatch)();
		return true;
	}

	return false;
}

bool CResponseSystem::DispatchParseRule( char const *pToken, unsigned int uiHash, ParseRuleDispatchMap_t &rMap, Rule &newRule )
{
	int slot = rMap.Find( uiHash );
	if ( slot != rMap.InvalidIndex() )
	{
		CResponseSystem::pfnParseRuleDispatch dispatch = rMap[ slot ];
		(this->*dispatch)( newRule );
		return true;
	}

	return false;
}

bool CResponseSystem::DispatchParseResponse( char const *pToken, unsigned int uiHash, ParseResponseDispatchMap_t &rMap, ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp )
{
	int slot = rMap.Find( uiHash );
	if ( slot != rMap.InvalidIndex() )
	{
		CResponseSystem::pfnParseResponseDispatch dispatch = rMap[ slot ];
		(this->*dispatch)( newResponse, group, rp );
		return true;
	}

	return false;
}

bool CResponseSystem::DispatchParseResponseGroup( char const *pToken, unsigned int uiHash, ParseResponseGroupDispatchMap_t &rMap, char const *responseGroupName, ResponseGroup& newGroup, AI_ResponseParams &groupResponseParams )
{
	int slot = rMap.Find( uiHash );
	if ( slot != rMap.InvalidIndex() )
	{
		CResponseSystem::pfnParseResponseGroupDispatch dispatch = rMap[ slot ];
		(this->*dispatch)( responseGroupName, newGroup, groupResponseParams );
		return true;
	}

	return false;
}

unsigned int ResponseRulePartition::GetBucketForSpeakerAndConcept( const char *pszSpeaker, const char *pszConcept, const char *pszSubject )
{
	// make sure is a power of two
	COMPILE_TIME_ASSERT( ( N_RESPONSE_PARTITIONS & ( N_RESPONSE_PARTITIONS - 1 ) ) == 0 );

	// hash together the speaker and concept strings, and mask off by the bucket mask
	unsigned hashSpeaker = 0; // pszSpeaker ? HashStringCaseless( pszSpeaker ) : 0;
	unsigned hashConcept = pszConcept ? HashStringCaseless( pszConcept ) : 0;
	unsigned hashSubject = pszSubject ? HashStringCaseless( pszSubject ) : 0;
	unsigned hashBrowns = ( ( hashSubject >> 3 ) ^ (hashSpeaker >> 1) ^ hashConcept ) & ( N_RESPONSE_PARTITIONS - 1 );
	return hashBrowns;
}

const char *Rule::GetValueForRuleCriterionByName( CResponseSystem * RESTRICT pSystem, const CUtlSymbol &pCritNameSym )
{
	const char * retval = NULL;
	// for each rule criterion...
	for ( int i = 0 ; i < m_Criteria.Count() ; ++i )
	{
		retval = RecursiveGetValueForRuleCriterionByName( pSystem, &pSystem->m_Criteria[m_Criteria[i]], pCritNameSym );
		if ( retval != NULL )
		{
			// we found a result, early out
			break;
		}
	}

	return retval;
}

const Criteria *Rule::GetPointerForRuleCriterionByName( CResponseSystem *pSystem, const CUtlSymbol &pCritNameSym )
{
	const Criteria * retval = NULL;
	// for each rule criterion...
	for ( int i = 0 ; i < m_Criteria.Count() ; ++i )
	{
		retval = RecursiveGetPointerForRuleCriterionByName( pSystem, &pSystem->m_Criteria[m_Criteria[i]], pCritNameSym );
		if ( retval != NULL )
		{
			// we found a result, early out
			break;
		}
	}

	return retval;
}

const char *Rule::RecursiveGetValueForRuleCriterionByName( CResponseSystem * RESTRICT pSystem, 
														    const Criteria * RESTRICT pCrit, const CUtlSymbol &pCritNameSym )
{
	Assert( pCrit );
	if ( !pCrit ) return NULL;
	if ( pCrit->IsSubCriteriaType() )
	{
		// test each of the children (depth first)
		const char *pRet = NULL;
		for ( int i = 0 ; i < pCrit->subcriteria.Count() ; ++i )
		{
			pRet = RecursiveGetValueForRuleCriterionByName( pSystem, &pSystem->m_Criteria[pCrit->subcriteria[i]], pCritNameSym );
			if ( pRet ) // if found something, early out
				return pRet;
		}
	}
	else // leaf criterion
	{
		if ( pCrit->nameSym == pCritNameSym )
		{
			return pCrit->value;
		}
		else
		{
			return NULL;
		}
	}

	return NULL;
}


const Criteria *Rule::RecursiveGetPointerForRuleCriterionByName( CResponseSystem *pSystem, const Criteria *pCrit, const CUtlSymbol &pCritNameSym )
{
	Assert( pCrit );
	if ( !pCrit ) return NULL;
	if ( pCrit->IsSubCriteriaType() )
	{
		// test each of the children (depth first)
		const Criteria *pRet = NULL;
		for ( int i = 0 ; i < pCrit->subcriteria.Count() ; ++i )
		{
			pRet = RecursiveGetPointerForRuleCriterionByName( pSystem, &pSystem->m_Criteria[pCrit->subcriteria[i]], pCritNameSym );
			if ( pRet ) // if found something, early out
				return pRet;
		}
	}
	else // leaf criterion
	{
		if ( pCrit->nameSym == pCritNameSym )
		{
			return pCrit;
		}
		else
		{
			return NULL;
		}
	}

	return NULL;
}


static void CC_RR_Debug_ResponseConcept_Exclude( const CCommand &args )
{
	// shouldn't use this extern elsewhere -- it's meant to be a hidden
	// implementation detail
	extern CRR_ConceptSymbolTable *g_pRRConceptTable;
	Assert( g_pRRConceptTable );
	if ( !g_pRRConceptTable ) return;


	// different things for different argument lengths
	switch ( args.ArgC() )
	{
	case 0:
	{
		AssertMsg( args.ArgC() > 0, "WTF error in ccommand parsing: zero arguments!\n" );
		return;
	}
	case 1:
	{
		// print usage info
		CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "Usage:  rr_debugresponseconcept_exclude  Concept1 Concept2 Concept3...\n");
		CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "\tseparate multiple concepts with spaces.\n");
		CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "\tcall with no arguments to see this message and a list of current excludes.\n");
		CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "\tto reset the exclude list, type \"rr_debugresponseconcept_exclude !\"\n");
		
		// print current excludes
		CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "\nCurrent exclude list:\n" );
		if ( !CResponseSystem::m_DebugExcludeList.IsValidIndex( CResponseSystem::m_DebugExcludeList.Head() ) )
		{
			CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "\t<none>\n" );
		}
		else
		{
			CResponseSystem::ExcludeList_t::IndexLocalType_t i;
			for ( i = CResponseSystem::m_DebugExcludeList.Head()		;
				  CResponseSystem::m_DebugExcludeList.IsValidIndex(i)	;
				  i = CResponseSystem::m_DebugExcludeList.Next(i)		)
			{
				CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "\t%s\n", CResponseSystem::m_DebugExcludeList[i].GetStringConcept() );
			}
		}
		return;
	}
	case 2:
		// deal with the erase operator
		if ( args[1][0] == '!' )
		{
			CResponseSystem::m_DebugExcludeList.Purge();
			CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "Exclude list emptied.\n" );
			return;
		}
		// else, FALL THROUGH:
	default:
		// add each arg to the exclude list
		for ( int i = 1 ; i < args.ArgC() ; ++i )
		{
			if ( !g_pRRConceptTable->Find(args[i]).IsValid() )
			{
				CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "\t'%s' is not a known concept (adding it anyway)\n", args[i] );
			}
			CRR_Concept concept( args[i] );
			CResponseSystem::m_DebugExcludeList.AddToTail( concept );
		}
	}
}
#if RR_DUMPHASHINFO_ENABLED
void ResponseRulePartition::PrintBucketInfo( CResponseSystem *pSys )
{
	struct bucktuple_t
	{
		int nBucket;
		int nCount;
		bucktuple_t() : nBucket(-1), nCount(-1) {};
		bucktuple_t( int bucket, int count )  : nBucket(bucket), nCount(count) {};

		static int __cdecl SortCompare( const bucktuple_t * a, const bucktuple_t * b )
		{
			return a->nCount - b->nCount;
		}
	};

	CUtlVector<bucktuple_t> infos( N_RESPONSE_PARTITIONS, N_RESPONSE_PARTITIONS );

	float nAverage = 0;
	for ( int i = 0 ; i < N_RESPONSE_PARTITIONS ; ++i )
	{
		int count = m_RuleParts[i].Count();
		infos.AddToTail( bucktuple_t( i, count ) );
		nAverage += count;
	}
	nAverage /= N_RESPONSE_PARTITIONS;
	infos.Sort( bucktuple_t::SortCompare );
	CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "%d buckets, %d total, %.2f average size\n", N_RESPONSE_PARTITIONS, Count(), nAverage );
	CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "8 shortest buckets:\n" );
	for ( int i = 0 ; i < 8 ; ++i )
	{
		CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "\t%d: %d\n", infos[i].nBucket, infos[i].nCount );
	}
	CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "8 longest buckets:\n" );
	for ( int i = infos.Count() - 1 ; i >= infos.Count() - 9 ; --i )
	{
		CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "\t%d: %d\n", infos[i].nBucket, infos[i].nCount );
	}
	int nempty = 0;
	for ( nempty = 0 ; nempty < infos.Count() ; ++nempty )
	{
		if ( infos[nempty].nCount != 0 )
			break;
	}
	CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "%d empty buckets\n", nempty );

	/*
	CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, " Contents of longest bucket\nwho\tconcept\n" );
	tRuleDict &bucket = m_RuleParts[infos[infos.Count()-1].nBucket];
	for ( tRuleDict::IndexType_t i = bucket.FirstInorder(); bucket.IsValidIndex(i); i = bucket.NextInorder(i) )
	{
		Rule &rule = bucket.Element(i) ;
		CGMsg( 0, CON_GROUP_RESPONSE_SYSTEM, "%s\t%s\n", rule.GetValueForRuleCriterionByName( pSys, "who" ), rule.GetValueForRuleCriterionByName( pSys, CriteriaSet::ComputeCriteriaSymbol("concept") ) );
	}
	*/
}
#endif