//========= Copyright © 1996-2010, Valve Corporation, All rights reserved. ============//
//
// Purpose: Core types for the response rules -- criteria, responses, rules, and matchers.
//
// $NoKeywords: $
//=============================================================================//

#include "rrbase.h"

#include "tier1/mapbase_con_groups.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace ResponseRules;


// bizarre function handed down from the misty days of yore
// and the original response system. a lot of stuff uses it
// and I can't be arsed to replace everything with the c stdlib 
// stuff
namespace ResponseRules 
{
	extern const char *ResponseCopyString( const char *in );
};


//-------------------- MATCHER ----------------------------------------------

Matcher::Matcher()
{
	valid = false;
	isnumeric = false;
	notequal = false;
	usemin = false;
	minequals = false;
	usemax = false;
	maxequals = false;
#ifdef MAPBASE
	isbit = false;
#endif
	maxval = 0.0f;
	minval = 0.0f;

	token = UTL_INVAL_SYMBOL;
	rawtoken = UTL_INVAL_SYMBOL;
}

void Matcher::Describe( void )
{
	if ( !valid )
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "    invalid!\n" );
		return;
	}
	char sz[ 128 ];

	sz[ 0] = 0;
	int minmaxcount = 0;
	if ( usemin )
	{
		Q_snprintf( sz, sizeof( sz ), ">%s%.3f", minequals ? "=" : "", minval );
		minmaxcount++;
	}
	if ( usemax )
	{
		char sz2[ 128 ];
		Q_snprintf( sz2, sizeof( sz2 ), "<%s%.3f", maxequals ? "=" : "", maxval );

		if ( minmaxcount > 0 )
		{
			Q_strncat( sz, " and ", sizeof( sz ), COPY_ALL_CHARACTERS );
		}
		Q_strncat( sz, sz2, sizeof( sz ), COPY_ALL_CHARACTERS );
		minmaxcount++;
	}

	if ( minmaxcount >= 1 )
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "    matcher:  %s\n", sz );
		return;
	}

#ifdef MAPBASE
	if ( isbit )
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "    matcher:  &%s%s\n", notequal ? "~" : "", GetToken() );
		return;
	}
#endif

	if ( notequal )
	{
		CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "    matcher:  !=%s\n", GetToken() );
		return;
	}

	CGMsg( 1, CON_GROUP_RESPONSE_SYSTEM, "    matcher:  ==%s\n", GetToken() );
}

void Matcher::SetToken( char const *s )
{
	token = g_RS.AddString( s );
}

void Matcher::SetRaw( char const *raw )
{
	rawtoken = g_RS.AddString( raw );
}

char const *Matcher::GetToken()
{
	if ( token.IsValid() )
	{
		return g_RS.String( token );
	}
	return "";
}

char const *Matcher::GetRaw()
{
	if ( rawtoken.IsValid() )
	{
		return g_RS.String( rawtoken );
	}
	return "";
}

//-------------------- CRITERIA ----------------------------------------------

Criteria::Criteria()
{
	value = NULL;
	weight.SetFloat( 1.0f );
	required = false;
}
Criteria::Criteria(const Criteria& src )
{
	operator=( src );
}

Criteria::~Criteria()
{
	// do nothing because we don't own name and value anymore
}

Criteria& Criteria::operator =(const Criteria& src )
{
	if ( this == &src )
		return *this;

	nameSym = src.nameSym;
	value = ResponseCopyString( src.value );
	weight = src.weight;
	required = src.required;

	matcher = src.matcher;

	int c = src.subcriteria.Count();
	subcriteria.EnsureCapacity( c );
	for ( int i = 0; i < c; i++ )
	{
		subcriteria.AddToTail( src.subcriteria[ i ] );
	}

	return *this;
}


//-------------------- RESPONSE ----------------------------------------------



ParserResponse::ParserResponse() : m_followup()
{
	type = RESPONSE_NONE;
	value = NULL;
	weight.SetFloat( 1.0f );
	depletioncount = 0;
	first = false;
	last = false;
}

ParserResponse& ParserResponse::operator =( const ParserResponse& src )
{
	if ( this == &src )
		return *this;
	weight = src.weight;
	type = src.type;
	value = ResponseCopyString( src.value );
	depletioncount = src.depletioncount;
	first = src.first;
	last = src.last;
	params = src.params;

	m_followup.followup_concept = ResponseCopyString(src.m_followup.followup_concept);
	m_followup.followup_contexts = ResponseCopyString(src.m_followup.followup_contexts);
	m_followup.followup_target = ResponseCopyString(src.m_followup.followup_target);
	m_followup.followup_entityioinput = ResponseCopyString(src.m_followup.followup_entityioinput);
	m_followup.followup_entityiotarget = ResponseCopyString(src.m_followup.followup_entityiotarget);
	m_followup.followup_delay = src.m_followup.followup_delay;
	m_followup.followup_entityiodelay = src.m_followup.followup_entityiodelay;

	return *this;
}

ParserResponse::ParserResponse( const ParserResponse& src )
{
	operator=( src );
}

ParserResponse::~ParserResponse()
{
	// nothing to do, since we don't own
	// the strings anymore
}

// ------------ RULE ---------------

Rule::Rule() : m_nForceWeight(0)
{
	m_bMatchOnce = false;
	m_bEnabled = true;
	m_szContext = NULL;
#ifdef MAPBASE
	m_iContextFlags = 0;
#else
	m_bApplyContextToWorld = false;
#endif
}

Rule& Rule::operator =( const Rule& src )
{
	if ( this == &src )
		return *this;

	int i;
	int c;

	c = src.m_Criteria.Count(); 
	m_Criteria.EnsureCapacity( c );
	for ( i = 0; i < c; i++ )
	{
		m_Criteria.AddToTail( src.m_Criteria[ i ] );
	}

	c = src.m_Responses.Count(); 
	m_Responses.EnsureCapacity( c );
	for ( i = 0; i < c; i++ )
	{
		m_Responses.AddToTail( src.m_Responses[ i ] );
	}

	SetContext( src.m_szContext );
	m_bMatchOnce = src.m_bMatchOnce;
	m_bEnabled = src.m_bEnabled;
#ifdef MAPBASE
	m_iContextFlags = src.m_iContextFlags;
#else
	m_bApplyContextToWorld = src.m_bApplyContextToWorld;
#endif
	m_nForceWeight = src.m_nForceWeight;
	return *this;
}

Rule::Rule( const Rule& src )
{
	operator=(src);
}

Rule::~Rule()
{
}

void Rule::SetContext( const char *context )
{
	// we don't own the data we point to, so just update pointer
	m_szContext = ResponseCopyString( context );
}


