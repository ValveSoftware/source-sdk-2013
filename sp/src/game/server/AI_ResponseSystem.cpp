//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//


#include "cbase.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "AI_ResponseSystem.h"
#include "igamesystem.h"
#include "AI_Criteria.h"
#include <KeyValues.h>
#include "filesystem.h"
#include "utldict.h"
#include "ai_speech.h"
#include "tier0/icommandline.h"
#include <ctype.h>
#include "sceneentity.h"
#include "isaverestore.h"
#include "utlbuffer.h"
#include "stringpool.h"
#include "fmtstr.h"
#include "multiplay_gamerules.h"
#ifdef MAPBASE
#include "mapbase/matchers.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar rr_debugresponses( "rr_debugresponses", "0", FCVAR_NONE, "Show verbose matching output (1 for simple, 2 for rule scoring). If set to 3, it will only show response success/failure for npc_selected NPCs." );
ConVar rr_debugrule( "rr_debugrule", "", FCVAR_NONE, "If set to the name of the rule, that rule's score will be shown whenever a concept is passed into the response rules system.");
ConVar rr_dumpresponses( "rr_dumpresponses", "0", FCVAR_NONE, "Dump all response_rules.txt and rules (requires restart)" );

#ifdef MAPBASE
ConVar rr_enhanced_saverestore( "rr_enhanced_saverestore", "0", FCVAR_NONE, "Enables enhanced save/restore capabilities for the Response System." );
#endif

static CUtlSymbolTable g_RS;

inline static char *CopyString( const char *in )
{
	if ( !in )
		return NULL;

	int len = Q_strlen( in );
	char *out = new char[ len + 1 ];
	Q_memcpy( out, in, len );
	out[ len ] = 0;
	return out;
}

#pragma pack(1)
class Matcher
{
public:
	Matcher()
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

	void Describe( void )
	{
		if ( !valid )
		{
			DevMsg( "    invalid!\n" );
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
			DevMsg( "    matcher:  %s\n", sz );
			return;
		}

#ifdef MAPBASE
		if (isbit)
		{
			DevMsg( "    matcher:  &%s%s\n", notequal ? "~" : "", GetToken() );
			return;
		}
#endif
		
		if ( notequal )
		{
			DevMsg( "    matcher:  !=%s\n", GetToken() );
			return;
		}

		DevMsg( "    matcher:  ==%s\n", GetToken() );
	}

	float	maxval;
	float	minval;

	bool	valid : 1;      //1
	bool	isnumeric : 1;  //2
	bool	notequal : 1;   //3
	bool	usemin : 1;     //4
	bool	minequals : 1;  //5
	bool	usemax : 1;     //6
	bool	maxequals : 1;  //7
#ifdef MAPBASE
	bool	isbit : 1; //8
#endif

	void	SetToken( char const *s )
	{
		token = g_RS.AddString( s );
	}

	char const *GetToken()
	{
		if ( token.IsValid() )
		{
			return g_RS.String( token );
		}
		return "";
	}
	void	SetRaw( char const *raw )
	{
		rawtoken = g_RS.AddString( raw );
	}
	char const *GetRaw()
	{
		if ( rawtoken.IsValid() )
		{
			return g_RS.String( rawtoken );
		}
		return "";
	}

private:
	CUtlSymbol	token;
	CUtlSymbol	rawtoken;
};

struct Response
{
	DECLARE_SIMPLE_DATADESC();

	Response()
	{
		type = RESPONSE_NONE;
		value = NULL;
		weight.SetFloat( 1.0f );
		depletioncount = 0;
		first = false;
		last = false;
	}

	Response( const Response& src )
	{
		weight = src.weight;
		type = src.type;
		value = CopyString( src.value );
		depletioncount = src.depletioncount;
		first = src.first;
		last = src.last;
	}

	Response& operator =( const Response& src )
	{
		if ( this == &src )
			return *this;
		weight = src.weight;
		type = src.type;
		value = CopyString( src.value );
		depletioncount = src.depletioncount;
		first = src.first;
		last = src.last;
		return *this;
	}

	~Response()
	{
		delete[] value;
	}

	ResponseType_t GetType() { return (ResponseType_t)type; }

	char						*value;  // fixed up value spot		// 4
	float16						weight;								// 6

	byte						depletioncount;						// 7
	byte						type : 6;							// 8
	byte						first : 1;							// 
	byte						last : 1;							// 
};

struct ResponseGroup
{
	DECLARE_SIMPLE_DATADESC();

	ResponseGroup()
	{
		// By default visit all nodes before repeating
		m_bSequential = false;
		m_bNoRepeat = false;
		m_bEnabled = true;
		m_nCurrentIndex = 0;
		m_bDepleteBeforeRepeat = true;
		m_nDepletionCount = 1;
		m_bHasFirst = false;
		m_bHasLast = false;
	}

	ResponseGroup( const ResponseGroup& src )
	{
		int c = src.group.Count();
		for ( int i = 0; i < c; i++ )
		{
			group.AddToTail( src.group[ i ] );
		}

		rp = src.rp;
		m_bDepleteBeforeRepeat = src.m_bDepleteBeforeRepeat;
		m_nDepletionCount = src.m_nDepletionCount;
		m_bHasFirst = src.m_bHasFirst;
		m_bHasLast = src.m_bHasLast;
		m_bSequential = src.m_bSequential;
		m_bNoRepeat = src.m_bNoRepeat;
		m_bEnabled = src.m_bEnabled;
		m_nCurrentIndex = src.m_nCurrentIndex;
	}

	ResponseGroup& operator=( const ResponseGroup& src )
	{
		if ( this == &src )
			return *this;
		int c = src.group.Count();
		for ( int i = 0; i < c; i++ )
		{
			group.AddToTail( src.group[ i ] );
		}

		rp = src.rp;
		m_bDepleteBeforeRepeat = src.m_bDepleteBeforeRepeat;
		m_nDepletionCount = src.m_nDepletionCount;
		m_bHasFirst = src.m_bHasFirst;
		m_bHasLast = src.m_bHasLast;
		m_bSequential = src.m_bSequential;
		m_bNoRepeat = src.m_bNoRepeat;
		m_bEnabled = src.m_bEnabled;
		m_nCurrentIndex = src.m_nCurrentIndex;
		return *this;
	}

	bool	HasUndepletedChoices() const
	{
		if ( !m_bDepleteBeforeRepeat )
			return true;

		int c = group.Count();
		for ( int i = 0; i < c; i++ )
		{
			if ( group[ i ].depletioncount != m_nDepletionCount )
				return true;
		}

		return false;
	}

	void	MarkResponseUsed( int idx )
	{
		if ( !m_bDepleteBeforeRepeat )
			return;

		if ( idx < 0 || idx >= group.Count() )
		{
			Assert( 0 );
			return;
		}

		group[ idx ].depletioncount = m_nDepletionCount;
	}

	void	ResetDepletionCount()
	{
		if ( !m_bDepleteBeforeRepeat )
			return;
		++m_nDepletionCount;
	}

	void	Reset()
	{
		ResetDepletionCount();
		SetEnabled( true );
		SetCurrentIndex( 0 );
		m_nDepletionCount = 1;

		for ( int i = 0; i < group.Count(); ++i )
		{
			group[ i ].depletioncount = 0;
		}
	}

	bool HasUndepletedFirst( int& index )
	{
		index = -1;

		if ( !m_bDepleteBeforeRepeat )
			return false;

		int c = group.Count();
		for ( int i = 0; i < c; i++ )
		{
			Response *r = &group[ i ];

			if ( ( r->depletioncount != m_nDepletionCount ) && r->first )
			{
				index = i;
				return true;
			}
		}

		return false;
	}
	
	bool HasUndepletedLast( int& index )
	{
		index = -1;

		if ( !m_bDepleteBeforeRepeat )
			return false;

		int c = group.Count();
		for ( int i = 0; i < c; i++ )
		{
			Response *r = &group[ i ];

			if ( ( r->depletioncount != m_nDepletionCount ) && r->last )
			{
				index = i;
				return true;
			}
		}

		return false;
	}

	bool	ShouldCheckRepeats() const { return m_bDepleteBeforeRepeat; }
	int		GetDepletionCount() const { return m_nDepletionCount; }

	bool	IsSequential() const { return m_bSequential; }
	void	SetSequential( bool seq ) { m_bSequential = seq; }

	bool	IsNoRepeat() const { return m_bNoRepeat; }
	void	SetNoRepeat( bool norepeat ) { m_bNoRepeat = norepeat; }

	bool	IsEnabled() const { return m_bEnabled; }
	void	SetEnabled( bool enabled ) { m_bEnabled = enabled; }

	int		GetCurrentIndex() const { return m_nCurrentIndex; }
	void	SetCurrentIndex( byte idx ) { m_nCurrentIndex = idx; }

	CUtlVector< Response >	group;

	AI_ResponseParams		rp;

	bool					m_bEnabled;

	byte					m_nCurrentIndex;
	// Invalidation counter
	byte					m_nDepletionCount;

	// Use all slots before repeating any
	bool					m_bDepleteBeforeRepeat : 1;
	bool					m_bHasFirst : 1;
	bool					m_bHasLast : 1;
	bool					m_bSequential : 1;
	bool					m_bNoRepeat : 1;
	
};

struct Criteria
{
	Criteria()
	{
		name = NULL;
		value = NULL;
		weight.SetFloat( 1.0f );
		required = false;
	}
	Criteria& operator =(const Criteria& src )
	{
		if ( this == &src )
			return *this;

		name = CopyString( src.name );
		value = CopyString( src.value );
		weight = src.weight;
		required = src.required;

		matcher = src.matcher;

		int c = src.subcriteria.Count();
		for ( int i = 0; i < c; i++ )
		{
			subcriteria.AddToTail( src.subcriteria[ i ] );
		}

		return *this;
	}
	Criteria(const Criteria& src )
	{
		name = CopyString( src.name );
		value = CopyString( src.value );
		weight = src.weight;
		required = src.required;

		matcher = src.matcher;

		int c = src.subcriteria.Count();
		for ( int i = 0; i < c; i++ )
		{
			subcriteria.AddToTail( src.subcriteria[ i ] );
		}
	}
	~Criteria()
	{
		delete[] name;
		delete[] value;
	}

	bool IsSubCriteriaType() const
	{
		return ( subcriteria.Count() > 0 ) ? true : false;
	}

	char						*name;
	char						*value;
	float16						weight;
	bool						required;

	Matcher						matcher;

	// Indices into sub criteria
	CUtlVector< unsigned short >	subcriteria;
};

struct Rule
{
	Rule()
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

	Rule& operator =( const Rule& src )
	{
		if ( this == &src )
			return *this;

		int i;
		int c;
		
		c = src.m_Criteria.Count(); 
		for ( i = 0; i < c; i++ )
		{
			m_Criteria.AddToTail( src.m_Criteria[ i ] );
		}

		c = src.m_Responses.Count(); 
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
		return *this;
	}

	Rule( const Rule& src )
	{
		int i;
		int c;
		
		c = src.m_Criteria.Count(); 
		for ( i = 0; i < c; i++ )
		{
			m_Criteria.AddToTail( src.m_Criteria[ i ] );
		}

		c = src.m_Responses.Count(); 
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
	}

	~Rule()
	{
		delete[] m_szContext;
	}

	void SetContext( const char *context )
	{
		delete[] m_szContext;
		m_szContext = CopyString( context );
	}

	const char *GetContext( void ) const { return m_szContext; }

	bool	IsEnabled() const { return m_bEnabled; }
	void	Disable() { m_bEnabled = false; }
	bool	IsMatchOnce() const { return m_bMatchOnce; }
#ifdef MAPBASE
	bool	IsApplyContextToWorld() const { return (m_iContextFlags & APPLYCONTEXT_WORLD) != 0; }
#else
	bool	IsApplyContextToWorld() const { return m_bApplyContextToWorld; }
#endif

	// Indices into underlying criteria and response dictionaries
	CUtlVector< unsigned short >	m_Criteria;
	CUtlVector< unsigned short>		m_Responses;

	char				*m_szContext;
#ifdef MAPBASE
	int					m_iContextFlags;
#else
	bool				m_bApplyContextToWorld : 1;
#endif

	bool				m_bMatchOnce : 1;
	bool				m_bEnabled : 1;
};
#pragma pack()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
abstract_class CResponseSystem : public IResponseSystem 
{
public:
	CResponseSystem();
	~CResponseSystem();

	// IResponseSystem
	virtual bool FindBestResponse( const AI_CriteriaSet& set, AI_Response& response, IResponseFilter *pFilter = NULL );
	virtual void GetAllResponses( CUtlVector<AI_Response *> *pResponses );

	virtual void Release() = 0;

	virtual void DumpRules();

	virtual void Precache();

	virtual void PrecacheResponses( bool bEnable )
	{
		m_bPrecache = bEnable;
	}

	bool		ShouldPrecache()	{ return m_bPrecache; }
	bool		IsCustomManagable()	{ return m_bCustomManagable; }

	void		Clear();

	void		DumpDictionary( const char *pszName );

protected:

	virtual const char *GetScriptFile( void ) = 0;
	void		LoadRuleSet( const char *setname );

	void		ResetResponseGroups();

	float		LookForCriteria( const AI_CriteriaSet &criteriaSet, int iCriteria );
	float		RecursiveLookForCriteria( const AI_CriteriaSet &criteriaSet, Criteria *pParent );

public:

	void		CopyRuleFrom( Rule *pSrcRule, int iRule, CResponseSystem *pCustomSystem );
	void		CopyCriteriaFrom( Rule *pSrcRule, Rule *pDstRule, CResponseSystem *pCustomSystem );
	void		CopyResponsesFrom( Rule *pSrcRule, Rule *pDstRule, CResponseSystem *pCustomSystem );
	void		CopyEnumerationsFrom( CResponseSystem *pCustomSystem );

//private:

	struct Enumeration
	{
		float		value;
	};

	struct ResponseSearchResult
	{
		ResponseSearchResult()
		{
			group = NULL;
			action = NULL;
		}

		ResponseGroup	*group;
		Response		*action;
	};

	inline bool ParseToken( void )
	{
		if ( m_bUnget )
		{
			m_bUnget = false;
			return true;
		}
		if ( m_ScriptStack.Count() <= 0 )
		{
			Assert( 0 );
			return false;
		}

		m_ScriptStack[ 0 ].currenttoken = engine->ParseFile( m_ScriptStack[ 0 ].currenttoken, token, sizeof( token ) );
		m_ScriptStack[ 0 ].tokencount++;
		return m_ScriptStack[ 0 ].currenttoken != NULL ? true : false;
	}

	inline void Unget()
	{
		m_bUnget = true;
	}

	inline bool TokenWaiting( void )
	{
		if ( m_ScriptStack.Count() <= 0 )
		{
			Assert( 0 );
			return false;
		}

		const char *p = m_ScriptStack[ 0 ].currenttoken;

		if ( !p )
		{
			Error( "AI_ResponseSystem:  Unxpected TokenWaiting() with NULL buffer in %p", m_ScriptStack[ 0 ].name );
			return false;
		}


		while ( *p && *p!='\n')
		{
			// Special handler for // comment blocks
			if ( *p == '/' && *(p+1) == '/' )
				return false;

			if ( !isspace( *p ) || isalnum( *p ) )
				return true;

			p++;
		}

		return false;
	}
	
	void		ParseOneResponse( const char *responseGroupName, ResponseGroup& group );

#ifdef MAPBASE
	void		ParseInclude( CStringPool &includedFiles, const char *scriptfile = NULL );
#else
	void		ParseInclude( CStringPool &includedFiles );
#endif
	void		ParseResponse( void );
	void		ParseCriterion( void );
	void		ParseRule( void );
	void		ParseEnumeration( void );

	int			ParseOneCriterion( const char *criterionName );
	
	bool		Compare( const char *setValue, Criteria *c, bool verbose = false );
	bool		CompareUsingMatcher( const char *setValue, Matcher& m, bool verbose = false );
	void		ComputeMatcher( Criteria *c, Matcher& matcher );
	void		ResolveToken( Matcher& matcher, char *token, size_t bufsize, char const *rawtoken );
	float		LookupEnumeration( const char *name, bool& found );

	int			FindBestMatchingRule( const AI_CriteriaSet& set, bool verbose );

	float		ScoreCriteriaAgainstRule( const AI_CriteriaSet& set, int irule, bool verbose = false );
	float		RecursiveScoreSubcriteriaAgainstRule( const AI_CriteriaSet& set, Criteria *parent, bool& exclude, bool verbose /*=false*/ );
	float		ScoreCriteriaAgainstRuleCriteria( const AI_CriteriaSet& set, int icriterion, bool& exclude, bool verbose = false );
	bool		GetBestResponse( ResponseSearchResult& result, Rule *rule, bool verbose = false, IResponseFilter *pFilter = NULL );
	bool		ResolveResponse( ResponseSearchResult& result, int depth, const char *name, bool verbose = false, IResponseFilter *pFilter = NULL );
	int			SelectWeightedResponseFromResponseGroup( ResponseGroup *g, IResponseFilter *pFilter );
	void		DescribeResponseGroup( ResponseGroup *group, int selected, int depth );
	void		DebugPrint( int depth, const char *fmt, ... );

	void		LoadFromBuffer( const char *scriptfile, const char *buffer, CStringPool &includedFiles );

	void		GetCurrentScript( char *buf, size_t buflen );
	int			GetCurrentToken() const;
	void		SetCurrentScript( const char *script );
	bool		IsRootCommand();

	void		PushScript( const char *scriptfile, unsigned char *buffer );
	void		PopScript(void);

	void		ResponseWarning( const char *fmt, ... );

	CUtlDict< ResponseGroup, short >	m_Responses;
	CUtlDict< Criteria, short >	m_Criteria;
	CUtlDict< Rule, short >	m_Rules;
	CUtlDict< Enumeration, short > m_Enumerations;

	char		token[ 1204 ];

	bool		m_bUnget;
	bool		m_bPrecache;	

	bool		m_bCustomManagable;

	struct ScriptEntry
	{
		unsigned char	*buffer;
		FileNameHandle_t name;
		const char		*currenttoken;
		int				tokencount;
	};

	CUtlVector< ScriptEntry >		m_ScriptStack;

	friend class CDefaultResponseSystemSaveRestoreBlockHandler;
	friend class CResponseSystemSaveRestoreOps;
};

BEGIN_SIMPLE_DATADESC( Response )
	// DEFINE_FIELD( type, FIELD_INTEGER ),
	// DEFINE_ARRAY( value, FIELD_CHARACTER ),
	// DEFINE_FIELD( weight, FIELD_FLOAT ),
	DEFINE_FIELD( depletioncount, FIELD_CHARACTER ),
	// DEFINE_FIELD( first, FIELD_BOOLEAN ),
	// DEFINE_FIELD( last, FIELD_BOOLEAN ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( ResponseGroup )
	// DEFINE_FIELD( group, FIELD_UTLVECTOR ),
	// DEFINE_FIELD( rp, FIELD_EMBEDDED ),
	// DEFINE_FIELD( m_bDepleteBeforeRepeat, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nDepletionCount, FIELD_CHARACTER ),
	// DEFINE_FIELD( m_bHasFirst, FIELD_BOOLEAN ),
	// DEFINE_FIELD( m_bHasLast, FIELD_BOOLEAN ),
	// DEFINE_FIELD( m_bSequential, FIELD_BOOLEAN ),
	// DEFINE_FIELD( m_bNoRepeat, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nCurrentIndex, FIELD_CHARACTER ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CResponseSystem::CResponseSystem()
{
	token[0] = 0;
	m_bUnget = false;
	m_bPrecache = true;
	m_bCustomManagable = false;
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
	
	if ( filesystem->String( m_ScriptStack[ 0 ].name, buf, buflen ) )
	{
		return;
	}
	buf[ 0 ] = 0;
}

void CResponseSystem::PushScript( const char *scriptfile, unsigned char *buffer )
{
	ScriptEntry e;
	e.name = filesystem->FindOrAddFileName( scriptfile );
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
	m_Rules.RemoveAll();
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


#ifndef MAPBASE // Moved to matchers.h
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
		int v2 = atoi(m.GetToken());
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
			if ( Matcher_NamesMatch(m.GetToken(), setValue) )
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
	// Wildcards.
	return Matcher_NamesMatch(m.GetToken(), setValue) ? true : false;
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
		DevMsg( "'%20s' vs. '%20s' = ", setValue, c->value );

		{
			//DevMsg( "\n" );
			//m.Describe();
		}
	}
	return bret;
}

float CResponseSystem::RecursiveScoreSubcriteriaAgainstRule( const AI_CriteriaSet& set, Criteria *parent, bool& exclude, bool verbose /*=false*/ )
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

float CResponseSystem::RecursiveLookForCriteria( const AI_CriteriaSet &criteriaSet, Criteria *pParent )
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

float CResponseSystem::LookForCriteria( const AI_CriteriaSet &criteriaSet, int iCriteria )
{
	Criteria *pCriteria = &m_Criteria[iCriteria];
	if ( pCriteria->IsSubCriteriaType() )
	{
		return RecursiveLookForCriteria( criteriaSet, pCriteria );
	}

	int iIndex = criteriaSet.FindCriterionIndex( pCriteria->name );
	if ( iIndex == -1 )
		return 0.0f;

	Assert( criteriaSet.GetValue( iIndex ) );
	if ( Q_stricmp( criteriaSet.GetValue( iIndex ), pCriteria->value ) )
		return 0.0f;

	return 1.0f;
}

float CResponseSystem::ScoreCriteriaAgainstRuleCriteria( const AI_CriteriaSet& set, int icriterion, bool& exclude, bool verbose /*=false*/ )
{
	Criteria *c = &m_Criteria[ icriterion ];

	if ( c->IsSubCriteriaType() )
	{
		return RecursiveScoreSubcriteriaAgainstRule( set, c, exclude, verbose );
	}

	if ( verbose )
	{
		DevMsg( "  criterion '%25s':'%15s' ", m_Criteria.GetElementName( icriterion ), c->name );
	}

	exclude = false;

	float score = 0.0f;

	const char *actualValue = "";

	int found = set.FindCriterionIndex( c->name );
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
			DevMsg( "matched, weight %4.2f (s %4.2f x c %4.2f)",
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
				DevMsg( "failed (+exclude rule)" );
			}
		}
		else
		{
			if ( verbose )
			{
				DevMsg( "failed" );
			}
		}
	}

	return score;
}

float CResponseSystem::ScoreCriteriaAgainstRule( const AI_CriteriaSet& set, int irule, bool verbose /*=false*/ )
{
	Rule *rule = &m_Rules[ irule ];
	float score = 0.0f;

	bool bBeingWatched = false;

	// See if we're trying to debug this rule
	const char *pszText = rr_debugrule.GetString();
	if ( pszText && pszText[0] && !Q_stricmp( pszText, m_Rules.GetElementName( irule ) ) )
	{
		bBeingWatched = true;
	}

	if ( !rule->IsEnabled() )
	{
		if ( bBeingWatched )
		{
			DevMsg("Rule '%s' is disabled.\n", m_Rules.GetElementName( irule ) );
		}
		return 0.0f;
	}

	if ( bBeingWatched )
	{
		verbose = true;
	}

	if ( verbose )
	{
		DevMsg( "Scoring rule '%s' (%i)\n{\n", m_Rules.GetElementName( irule ), irule+1 );
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
			DevMsg( ", score %4.2f\n", score );
		}

		if ( exclude ) 
		{
			score = 0.0f;
			break;
		}
	}

	if ( verbose )
	{
		DevMsg( "}\n" );
	}
	
	return score;
}

void CResponseSystem::DebugPrint( int depth, const char *fmt, ... )
{
	int indentchars = 3 * depth;
	char *indent = (char *)_alloca( indentchars + 1);
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

	DevMsg( "%s%s", indent, szText );
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

	int i;

	// Fake depletion of unavailable choices
	CUtlVector<int> fakedDepletes;
	if ( pFilter && g->ShouldCheckRepeats() )
	{
		for ( i = 0; i < c; i++ )
		{
			Response *r = &g->group[ i ];
			if ( r->depletioncount != g->GetDepletionCount() && !pFilter->IsValidResponse( r->GetType(), r->value ) )
			{
				fakedDepletes.AddToTail( i );
				g->MarkResponseUsed( i );
			}
		}
	}

	if ( !g->HasUndepletedChoices() )
	{
		g->ResetDepletionCount();

		if ( pFilter && g->ShouldCheckRepeats() )
		{
			fakedDepletes.RemoveAll();
			for ( i = 0; i < c; i++ )
			{
				Response *r = &g->group[ i ];
				if ( !pFilter->IsValidResponse( r->GetType(), r->value ) )
				{
					fakedDepletes.AddToTail( i );
					g->MarkResponseUsed( i );
				}
			}
		}

		if ( !g->HasUndepletedChoices() )
			return -1;

		// Disable the group if we looped through all the way
		if ( g->IsNoRepeat() )
		{
			g->SetEnabled( false );
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
			for ( i = 0; i < c; i++ )
			{
				Response *r = &g->group[ i ];
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
		for ( i = 0; i < c; i++ )
		{
			Response *r = &g->group[ i ];
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
			if ( !totalweight || random->RandomFloat(0,totalweight) < r->weight.GetFloat() )
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
		g->MarkResponseUsed( slot );

	// Revert fake depletion of unavailable choices
	if ( pFilter && g->ShouldCheckRepeats() )
	{
		for ( i = 0; i < fakedDepletes.Count(); i++ )
		{
			g->group[ fakedDepletes[ i ] ].depletioncount = 0;;
		}
	}

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

	Response *result = &g->group[ idx ];
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
		Response *r = &group->group[ i ];
		DebugPrint( depth + 1, "%s%20s : %40s %5.3f\n",
			i == selected ? "-> " : "   ",
			AI_Response::DescribeResponse( r->GetType() ),
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

	int index = random->RandomInt( 0, c - 1 );
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


	Response *r = &g->group[ responseIndex ];

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
//-----------------------------------------------------------------------------
int CResponseSystem::FindBestMatchingRule( const AI_CriteriaSet& set, bool verbose )
{
	CUtlVector< int >	bestrules;
	float bestscore = 0.001f;

	int c = m_Rules.Count();
	int i;
	for ( i = 0; i < c; i++ )
	{
		float score = ScoreCriteriaAgainstRule( set, i, verbose );
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
			bestrules.AddToTail( i );
		}
	}

	int bestCount = bestrules.Count();
	if ( bestCount <= 0 )
		return -1;

	if ( bestCount == 1 )
		return bestrules[ 0 ];

	// Randomly pick one of the tied matching rules
	int idx = random->RandomInt( 0, bestCount - 1 );
	if ( verbose )
	{
		DevMsg( "Found %i matching rules, selecting slot %i\n", bestCount, idx );
	}
	return bestrules[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
// Output : AI_Response
//-----------------------------------------------------------------------------
bool CResponseSystem::FindBestResponse( const AI_CriteriaSet& set, AI_Response& response, IResponseFilter *pFilter )
{
	bool valid = false;

	int iDbgResponse = rr_debugresponses.GetInt();
	bool showRules = ( iDbgResponse == 2 );
	bool showResult = ( iDbgResponse == 1 || iDbgResponse == 2 );

	// Look for match. verbose mode used to be at level 2, but disabled because the writers don't actually care for that info.
	int bestRule = FindBestMatchingRule( set, iDbgResponse == 3 ); 

	ResponseType_t responseType = RESPONSE_NONE;
	AI_ResponseParams rp;

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
	if ( bestRule != -1 )
	{
		Rule *r = &m_Rules[ bestRule ];

		ResponseSearchResult result;
		if ( GetBestResponse( result, r, showResult, pFilter ) )
		{
			Q_strncpy( responseName, result.action->value, sizeof( responseName ) );
			responseType = result.action->GetType();
			rp = result.group->rp;
		}

		Q_strncpy( ruleName, m_Rules.GetElementName( bestRule ), sizeof( ruleName ) );

		// Disable the rule if it only allows for matching one time
		if ( r->IsMatchOnce() )
		{
			r->Disable();
		}
		context = r->GetContext();
#ifdef MAPBASE
		contextflags = r->m_iContextFlags;
#else
		bcontexttoworld = r->IsApplyContextToWorld();
#endif

		valid = true;
	}

#ifdef MAPBASE
	response.Init( responseType, responseName, set, rp, ruleName, context, contextflags );
#else
	response.Init( responseType, responseName, set, rp, ruleName, context, bcontexttoworld );
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
			// Describe the response, too
			response.Describe();
		}
	}

	return valid;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CResponseSystem::GetAllResponses( CUtlVector<AI_Response *> *pResponses )
{
	for ( int i = 0; i < (int)m_Responses.Count(); i++ )
	{
		ResponseGroup &group = m_Responses[i];

		for ( int j = 0; j < group.group.Count(); j++)
		{
			Response &response = group.group[j];
			if ( response.type != RESPONSE_RESPONSE )
			{
				AI_Response *pResponse = new AI_Response;
				pResponse->Init( response.GetType(), response.value, AI_CriteriaSet(), group.rp, NULL, NULL, false );
				pResponses->AddToTail(pResponse);
			}
		}
	}
}

static void TouchFile( char const *pchFileName )
{
	filesystem->Size( pchFileName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CResponseSystem::Precache()
{
	bool bTouchFiles = CommandLine()->FindParm( "-makereslists" ) != 0;

	// enumerate and mark all the scripts so we know they're referenced
	for ( int i = 0; i < (int)m_Responses.Count(); i++ )
	{
		ResponseGroup &group = m_Responses[i];

		for ( int j = 0; j < group.group.Count(); j++)
		{
			Response &response = group.group[j];

			switch ( response.type )
			{
			default:
				break;
			case RESPONSE_SCENE:
				{
					// fixup $gender references
					char file[_MAX_PATH];
					Q_strncpy( file, response.value, sizeof(file) );
					char *gender = strstr( file, "$gender" );
					if ( gender )
					{
						// replace with male & female
						const char *postGender = gender + strlen("$gender");
						*gender = 0;
						char genderFile[_MAX_PATH];
						// male
						Q_snprintf( genderFile, sizeof(genderFile), "%smale%s", file, postGender);

						PrecacheInstancedScene( genderFile );
						if ( bTouchFiles )
						{
							TouchFile( genderFile );
						}

						Q_snprintf( genderFile, sizeof(genderFile), "%sfemale%s", file, postGender);

						PrecacheInstancedScene( genderFile );
						if ( bTouchFiles )
						{
							TouchFile( genderFile );
						}
					}
					else
					{
						PrecacheInstancedScene( file );
						if ( bTouchFiles )
						{
							TouchFile( file );
						}
					}
				}
				break;
			case RESPONSE_SPEAK:
				{
					CBaseEntity::PrecacheScriptSound( response.value );
				}
				break;
			}
		}
	}
}

#ifdef MAPBASE
void CResponseSystem::ParseInclude( CStringPool &includedFiles, const char *scriptfile )
#else
void CResponseSystem::ParseInclude( CStringPool &includedFiles )
#endif
{
	char includefile[ 256 ];
	ParseToken();
#ifdef MAPBASE
	if (scriptfile)
	{
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
	}

	if (!includefile[0])
		Q_snprintf( includefile, sizeof( includefile ), "scripts/%s", token );
#else
	Q_snprintf( includefile, sizeof( includefile ), "scripts/%s", token );
#endif

	// check if the file is already included
	if ( includedFiles.Find( includefile ) != NULL )
	{
		return;
	}

	MEM_ALLOC_CREDIT();

	// Try and load it
	CUtlBuffer buf;
	if ( !filesystem->ReadFile( includefile, "GAME", buf ) )
	{
		DevMsg( "Unable to load #included script %s\n", includefile );
		return;
	}

	LoadFromBuffer( includefile, (const char *)buf.PeekGet(), includedFiles );
}

void CResponseSystem::LoadFromBuffer( const char *scriptfile, const char *buffer, CStringPool &includedFiles )
{
	includedFiles.Allocate( scriptfile );
	PushScript( scriptfile, (unsigned char * )buffer );

	if( rr_dumpresponses.GetBool() )
	{
		DevMsg("Reading: %s\n", scriptfile );
	}

	while ( 1 )
	{
		ParseToken();
		if ( !token[0] )
		{
			break;
		}

		if ( !Q_stricmp( token, "#include" ) )
		{
#ifdef MAPBASE
			ParseInclude( includedFiles, scriptfile );
#else
			ParseInclude( includedFiles );
#endif
		}
#ifdef MAPBASE
		else if ( !Q_stricmp( token, "#base" ) )
		{
			// Actual #base in the future?
			ParseInclude( includedFiles, scriptfile );
		}
#endif
		else if ( !Q_stricmp( token, "response" ) )
		{
			ParseResponse();
		}
		else if ( !Q_stricmp( token, "criterion" ) || 
			!Q_stricmp( token, "criteria" ) )
		{
			ParseCriterion();
		}
		else if ( !Q_stricmp( token, "rule" ) )
		{
			ParseRule();
		}
		else if ( !Q_stricmp( token, "enumeration" ) )
		{
			ParseEnumeration();
		}
		else
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
		DevMsg( 1, "CResponseSystem:  %s (%i rules, %i criteria, and %i responses)\n",
			cur, m_Rules.Count(), m_Criteria.Count(), m_Responses.Count() );

		if( rr_dumpresponses.GetBool() )
		{
			DumpRules();
		}
	}

	PopScript();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CResponseSystem::LoadRuleSet( const char *basescript )
{
	int length = 0;
	unsigned char *buffer = (unsigned char *)UTIL_LoadFileForMe( basescript, &length );
	if ( length <= 0 || !buffer )
	{
		DevMsg( 1, "CResponseSystem:  failed to load %s\n", basescript );
		return;
	}

	CStringPool includedFiles;

	LoadFromBuffer( basescript, (const char *)buffer, includedFiles );

	UTIL_FreeFile( buffer );

	Assert( m_ScriptStack.Count() == 0 );
}

static ResponseType_t ComputeResponseType( const char *s )
{
	if ( !Q_stricmp( s, "scene" ) )
	{
		return RESPONSE_SCENE;
	}
	else if ( !Q_stricmp( s, "sentence" ) )
	{
		return RESPONSE_SENTENCE;
	}
	else if ( !Q_stricmp( s, "speak" ) )
	{
		return RESPONSE_SPEAK;
	}
	else if ( !Q_stricmp( s, "response" ) )
	{
		return RESPONSE_RESPONSE;
	}
	else if ( !Q_stricmp( s, "print" ) )
	{
		return RESPONSE_PRINT;
	}

	return RESPONSE_NONE;
}

void CResponseSystem::ParseOneResponse( const char *responseGroupName, ResponseGroup& group )
{
	Response newResponse;
	newResponse.weight.SetFloat( 1.0f );
	AI_ResponseParams *rp = &group.rp;

	newResponse.type = ComputeResponseType( token );
	if ( RESPONSE_NONE == newResponse.type )
	{
		ResponseWarning( "response entry '%s' with unknown response type '%s'\n", responseGroupName, token );
		return;
	}

	ParseToken();
	newResponse.value = CopyString( token );

	while ( TokenWaiting() )
	{
		ParseToken();
		if ( !Q_stricmp( token, "weight" ) )
		{
			ParseToken();
			newResponse.weight.SetFloat( (float)atof( token ) );
			continue;
		}

		if ( !Q_stricmp( token, "predelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_DELAYBEFORESPEAK;
			rp->predelay.FromInterval( ReadInterval( token ) );
			continue;
		}

		if ( !Q_stricmp( token, "nodelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay.start = 0;
			rp->delay.range = 0;
			continue;
		}

		if ( !Q_stricmp( token, "defaultdelay" ) )
		{
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay.start = AIS_DEF_MIN_DELAY;
			rp->delay.range = ( AIS_DEF_MAX_DELAY - AIS_DEF_MIN_DELAY );
			continue;
		}
	
		if ( !Q_stricmp( token, "delay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay.FromInterval( ReadInterval( token ) );
			continue;
		}
		
		if ( !Q_stricmp( token, "speakonce" ) )
		{
			rp->flags |= AI_ResponseParams::RG_SPEAKONCE;
			continue;
		}
		
		if ( !Q_stricmp( token, "noscene" ) )
		{
			rp->flags |= AI_ResponseParams::RG_DONT_USE_SCENE;
			continue;
		}

		if ( !Q_stricmp( token, "stop_on_nonidle" ) )
		{
			rp->flags |= AI_ResponseParams::RG_STOP_ON_NONIDLE;
			continue;
		}
		
		if ( !Q_stricmp( token, "odds" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_ODDS;
			rp->odds = clamp( atoi( token ), 0, 100 );
			continue;
		}
		
		if ( !Q_stricmp( token, "respeakdelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_RESPEAKDELAY;
			rp->respeakdelay.FromInterval( ReadInterval( token ) );
			continue;
		}
		
		if ( !Q_stricmp( token, "weapondelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_WEAPONDELAY;
			rp->weapondelay.FromInterval( ReadInterval( token ) );
			continue;
		}

		if ( !Q_stricmp( token, "soundlevel" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_SOUNDLEVEL;
			rp->soundlevel = (soundlevel_t)TextToSoundLevel( token );
			continue;
		}

		if ( !Q_stricmp( token, "displayfirst" ) )
		{
			newResponse.first = true;
			group.m_bHasFirst = true;
			continue;
		}

		if ( !Q_stricmp( token, "displaylast" ) )
		{
			newResponse.last = true;
			group.m_bHasLast= true;
			continue;
		}

		ResponseWarning( "response entry '%s' with unknown command '%s'\n", responseGroupName, token );
	}

	group.group.AddToTail( newResponse );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CResponseSystem::IsRootCommand()
{
	if ( !Q_stricmp( token, "#include" ) )
		return true;
	if ( !Q_stricmp( token, "response" ) )
		return true;
	if ( !Q_stricmp( token, "enumeration" ) )
		return true;
	if ( !Q_stricmp( token, "criteria" ) )
		return true;
	if ( !Q_stricmp( token, "criterion" ) )
		return true;
	if ( !Q_stricmp( token, "rule" ) )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *kv - 
//-----------------------------------------------------------------------------
void CResponseSystem::ParseResponse( void )
{
	// Should have groupname at start
	char responseGroupName[ 128 ];

	ResponseGroup newGroup;
	AI_ResponseParams *rp = &newGroup.rp;

	// Response Group Name
	ParseToken();
	Q_strncpy( responseGroupName, token, sizeof( responseGroupName ) );

	while ( 1 )
	{
		ParseToken();

		// Oops, part of next definition
		if( IsRootCommand() )
		{
			Unget();
			break;
		}

		if ( !Q_stricmp( token, "{" ) )
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

				ParseOneResponse( responseGroupName, newGroup );
			}
			break;
		}

		if ( !Q_stricmp( token, "predelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_DELAYBEFORESPEAK;
			rp->predelay.FromInterval( ReadInterval( token ) );
			continue;
		}

		if ( !Q_stricmp( token, "nodelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay.start = 0;
			rp->delay.range = 0;
			continue;
		}

		if ( !Q_stricmp( token, "defaultdelay" ) )
		{
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay.start = AIS_DEF_MIN_DELAY;
			rp->delay.range = ( AIS_DEF_MAX_DELAY - AIS_DEF_MIN_DELAY );
			continue;
		}
	
		if ( !Q_stricmp( token, "delay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay.FromInterval( ReadInterval( token ) );
			continue;
		}
		
		if ( !Q_stricmp( token, "speakonce" ) )
		{
			rp->flags |= AI_ResponseParams::RG_SPEAKONCE;
			continue;
		}
		
		if ( !Q_stricmp( token, "noscene" ) )
		{
			rp->flags |= AI_ResponseParams::RG_DONT_USE_SCENE;
			continue;
		}
		
		if ( !Q_stricmp( token, "stop_on_nonidle" ) )
		{
			rp->flags |= AI_ResponseParams::RG_STOP_ON_NONIDLE;
			continue;
		}

		if ( !Q_stricmp( token, "odds" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_ODDS;
			rp->odds = clamp( atoi( token ), 0, 100 );
			continue;
		}
		
		if ( !Q_stricmp( token, "respeakdelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_RESPEAKDELAY;
			rp->respeakdelay.FromInterval( ReadInterval( token ) );
			continue;
		}

		if ( !Q_stricmp( token, "weapondelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_WEAPONDELAY;
			rp->weapondelay.FromInterval( ReadInterval( token ) );
			continue;
		}

		if ( !Q_stricmp( token, "soundlevel" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_SOUNDLEVEL;
			rp->soundlevel = (soundlevel_t)TextToSoundLevel( token );
			continue;
		}

		ParseOneResponse( responseGroupName, newGroup );
	}

#ifdef MAPBASE
	short existing = m_Responses.Find( responseGroupName );
	if ( existing != m_Responses.InvalidIndex() )
	{
		//ResponseWarning( "Additional definition for response '%s', overwriting\n", responseGroupName );
		m_Responses[existing] = newGroup;
		m_Responses.SetElementName(existing, responseGroupName);
		return;
	}
#endif
	m_Responses.Insert( responseGroupName, newGroup );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *criterion - 
//-----------------------------------------------------------------------------
int CResponseSystem::ParseOneCriterion( const char *criterionName )
{
	char key[ 128 ];
	char value[ 128 ];

	Criteria newCriterion;

	bool gotbody = false;

	while ( TokenWaiting() || !gotbody )
	{
		ParseToken();

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
					newCriterion.subcriteria.AddToTail( idx );
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
			newCriterion.required = true;
		}
		else if ( !Q_stricmp( token, "weight" ) )
		{
			ParseToken();
			newCriterion.weight.SetFloat( (float)atof( token ) );
		}
		else
		{
			Assert( newCriterion.subcriteria.Count() == 0 );

			// Assume it's the math info for a non-subcriteria resposne
			Q_strncpy( key, token, sizeof( key ) );
			ParseToken();
			Q_strncpy( value, token, sizeof( value ) );

			newCriterion.name = CopyString( key );
			newCriterion.value = CopyString( value );

			gotbody = true;
		}
	}

	if ( !newCriterion.IsSubCriteriaType() )
	{
		ComputeMatcher( &newCriterion, newCriterion.matcher );
	}

#ifdef MAPBASE
	short existing = m_Criteria.Find( criterionName );
	if ( existing != m_Criteria.InvalidIndex() )
	{
		//ResponseWarning( "Additional definition for criteria '%s', overwriting\n", criterionName );
		m_Criteria[existing] = newCriterion;
		m_Criteria.SetElementName(existing, criterionName);
		return existing;
	}
#else
	if ( m_Criteria.Find( criterionName ) != m_Criteria.InvalidIndex() )
	{
		ResponseWarning( "Multiple definitions for criteria '%s'\n", criterionName );
		return m_Criteria.InvalidIndex();
	}
#endif

	int idx = m_Criteria.Insert( criterionName, newCriterion );
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
#ifdef MAPBASE
		else
		{
			short existing = m_Enumerations.Find( sz );
			//ResponseWarning( "Additional definition for enumeration '%s', overwriting\n", sz );
			m_Enumerations[existing] = newEnum;
			m_Enumerations.SetElementName(existing, sz);
		}
#else
		/*
		else
		{
			ResponseWarning( "Ignoring duplication enumeration '%s'\n", sz );
		}
		*/
#endif
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
	Rule newRule;

	char sz[ 128 ];

	bool validRule = true;
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

		if ( !Q_stricmp( token, "matchonce" ) )
		{
			newRule.m_bMatchOnce = true;
			continue;
		}

		if ( !Q_stricmp( token, "applyContextToWorld" ) )
		{
#ifdef MAPBASE
			newRule.m_iContextFlags |= APPLYCONTEXT_WORLD;
#else
			newRule.m_bApplyContextToWorld = true;
#endif
			continue;
		}

#ifdef MAPBASE
		if ( !Q_stricmp( token, "applyContextToSquad" ) )
		{
			newRule.m_iContextFlags |= APPLYCONTEXT_SQUAD;
			continue;
		}

		if ( !Q_stricmp( token, "applyContextToEnemy" ) )
		{
			newRule.m_iContextFlags |= APPLYCONTEXT_ENEMY;
			continue;
		}
#endif

		if ( !Q_stricmp( token, "applyContext" ) )
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
			continue;
		}
		
		if ( !Q_stricmp( token, "response" ) )
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
					validRule = false;
					ResponseWarning( "No such response '%s' for rule '%s'\n", token, ruleName );
				}
			}
			continue;
		}

		if ( !Q_stricmp( token, "criteria" ) ||
			 !Q_stricmp( token, "criterion" ) )
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
					validRule = false;
					ResponseWarning( "No such criterion '%s' for rule '%s'\n", token, ruleName );
				}
			}
			continue;
		}

		// It's an inline criteria, generate a name and parse it in
		Q_snprintf( sz, sizeof( sz ), "[%s%03i]", ruleName, ++instancedCriteria );
		Unget();
		int idx = ParseOneCriterion( sz );
		if ( idx != m_Criteria.InvalidIndex() )
		{
			newRule.m_Criteria.AddToTail( idx );
		}
	}

	if ( validRule )
	{
#ifdef MAPBASE
		short existing = m_Rules.Find( ruleName );
		if ( existing != m_Rules.InvalidIndex() )
		{
			//ResponseWarning( "Additional definition for rule '%s', overwriting\n", ruleName );
			m_Rules[existing] = newRule;
			m_Rules.SetElementName(existing, ruleName);
			return;
		}
#endif
		m_Rules.Insert( ruleName, newRule );
	}
	else
	{
		DevMsg( "Discarded rule %s\n", ruleName );
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
#ifndef _XBOX
	static char	string[1024];
#else
	char		string[1024];
#endif	

	va_start (argptr, fmt);
	Q_vsnprintf(string, sizeof(string), fmt,argptr);
	va_end (argptr);

	char cur[ 256 ];
	GetCurrentScript( cur, sizeof( cur ) );
	DevMsg( 1, "%s(token %i) : %s", cur, GetCurrentToken(), string );
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

			dstCriteria.name = CopyString( pSrcCriteria->name );
			dstCriteria.value = CopyString( pSrcCriteria->value );
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

					dstSubCriteria.name = CopyString( pSrcSubCriteria->name );
					dstSubCriteria.value = CopyString( pSrcSubCriteria->value );
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

			dstResponseGroup.rp = pSrcResponseGroup->rp;
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
				Response *pSrcResponse = &pSrcResponseGroup->group[iResponse];
				if ( pSrcResponse )
				{
					// Add Response
					Response dstResponse;

					dstResponse.weight = pSrcResponse->weight;
					dstResponse.type = pSrcResponse->type;
					dstResponse.value = CopyString( pSrcResponse->value );
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
void CResponseSystem::CopyRuleFrom( Rule *pSrcRule, int iRule, CResponseSystem *pCustomSystem )
{
	// Verify data.
	Assert( pSrcRule );
	Assert( pCustomSystem );
	if ( !pSrcRule || !pCustomSystem )
		return;

	// New rule
	Rule dstRule;

	dstRule.SetContext( pSrcRule->GetContext() );
	dstRule.m_bMatchOnce = pSrcRule->m_bMatchOnce;
	dstRule.m_bEnabled = pSrcRule->m_bEnabled;
#ifdef MAPBASE
	dstRule.m_iContextFlags = pSrcRule->m_iContextFlags;
#else
	dstRule.m_bApplyContextToWorld = pSrcRule->m_bApplyContextToWorld;
#endif

	// Copy off criteria.
	CopyCriteriaFrom( pSrcRule, &dstRule, pCustomSystem );

	// Copy off responses.
	CopyResponsesFrom( pSrcRule, &dstRule, pCustomSystem );

	// Copy off enumerations - Don't think we use these.
//	CopyEnumerationsFrom( pCustomSystem );

	// Add rule.
	pCustomSystem->m_Rules.Insert( m_Rules.GetElementName( iRule ), dstRule );
}

#ifdef MAPBASE
ConVar mapbase_rs_clear("mapbase_rs_clear", "1");
#endif

//-----------------------------------------------------------------------------
// Purpose: A special purpose response system associated with a custom entity
//-----------------------------------------------------------------------------
class CInstancedResponseSystem : public CResponseSystem
{
	typedef CResponseSystem BaseClass;

public:
	CInstancedResponseSystem( const char *scriptfile ) :
	  m_pszScriptFile( 0 )
	{
		Assert( scriptfile );

		int len = Q_strlen( scriptfile ) + 1;
		m_pszScriptFile = new char[ len ];
		Assert( m_pszScriptFile );
		Q_strncpy( m_pszScriptFile, scriptfile, len );
	}

	~CInstancedResponseSystem()
	{
		delete[] m_pszScriptFile;
	}
	virtual const char *GetScriptFile( void ) 
	{
		Assert( m_pszScriptFile );
		return m_pszScriptFile;
	}

	// CAutoGameSystem
	virtual bool Init()
	{
		const char *basescript = GetScriptFile();
		LoadRuleSet( basescript );
		return true;
	}

	virtual void LevelInitPostEntity()
	{
		ResetResponseGroups();
	}

	virtual void Release()
	{
		Clear();
		delete this;
	}

#ifdef MAPBASE
	// From an old version of Mapbase's map-specific response system stuff.
	/*
	#define CopyRSDict(dict) for (unsigned int i = 0; i < dict.Count(); i++) \
		{ \
			rs->dict.Insert(dict.GetElementName(i), dict[i]); \
		} \

	bool MergeWithMain(bool bRemoveThis = true)
	{
		extern IResponseSystem *g_pResponseSystem;
		CResponseSystem *rs = static_cast<CResponseSystem*>(g_pResponseSystem);

		CopyRSDict(m_Responses);
		CopyRSDict(m_Criteria);
		CopyRSDict(m_Rules);
		CopyRSDict(m_Enumerations);

		if (mapbase_rs_clear.GetBool())
			Release();

		return true;
	}
	*/
#endif
private:

	char *m_pszScriptFile;
};

//-----------------------------------------------------------------------------
// Purpose: The default response system for expressive AIs
//-----------------------------------------------------------------------------
class CDefaultResponseSystem : public CResponseSystem, public CAutoGameSystem
{
	typedef CAutoGameSystem BaseClass;

public:
	CDefaultResponseSystem() : CAutoGameSystem( "CDefaultResponseSystem" )
	{
	}

	virtual const char *GetScriptFile( void ) 
	{
		return "scripts/talker/response_rules.txt";
	}

	// CAutoServerSystem
	virtual bool Init();
	virtual void Shutdown();

	virtual void LevelInitPostEntity()
	{
	}

	virtual void Release()
	{
		Assert( 0 );
	}

	void AddInstancedResponseSystem( const char *scriptfile, CInstancedResponseSystem *sys )
	{
		m_InstancedSystems.Insert( scriptfile, sys );
	}

	CInstancedResponseSystem *FindResponseSystem( const char *scriptfile )
	{
		int idx = m_InstancedSystems.Find( scriptfile );
		if ( idx == m_InstancedSystems.InvalidIndex() )
			return NULL;
		return m_InstancedSystems[ idx ];
	}

	IResponseSystem *PrecacheCustomResponseSystem( const char *scriptfile )
	{
		CInstancedResponseSystem *sys = ( CInstancedResponseSystem * )FindResponseSystem( scriptfile );
		if ( !sys )
		{
			sys = new CInstancedResponseSystem( scriptfile );
			if ( !sys )
			{
				Error( "Failed to load response system data from %s", scriptfile );
			}

			if ( !sys->Init() )
			{
				Error( "CInstancedResponseSystem:  Failed to init response system from %s!", scriptfile );
			}

			AddInstancedResponseSystem( scriptfile, sys );
		}

		sys->Precache();

		return ( IResponseSystem * )sys;
	}

	IResponseSystem *BuildCustomResponseSystemGivenCriteria( const char *pszBaseFile, const char *pszCustomName, AI_CriteriaSet &criteriaSet, float flCriteriaScore );
	void DestroyCustomResponseSystems();

	virtual void LevelInitPreEntity()
	{
		// This will precache the default system
		// All user installed systems are init'd by PrecacheCustomResponseSystem which will call sys->Precache() on the ones being used

		// FIXME:  This is SLOW the first time you run the engine (can take 3 - 10 seconds!!!)
		if ( ShouldPrecache() )
		{
			Precache();
		}

#ifdef MAPBASE
		if (!rr_enhanced_saverestore.GetBool() || gpGlobals->eLoadType != MapLoad_Transition)
#endif
		ResetResponseGroups();
	}

	void ReloadAllResponseSystems()
	{
		Clear();
		Init();

		int c = m_InstancedSystems.Count();
		for ( int i = c - 1 ; i >= 0; i-- )
		{
			CInstancedResponseSystem *sys = m_InstancedSystems[ i ];
			if ( !IsCustomManagable() )
			{
				sys->Clear();
				sys->Init();
			}
			else
			{
				// Custom reponse rules will manage/reload themselves - remove them.
				m_InstancedSystems.RemoveAt( i );
			}
		}

	}

private:

	void ClearInstanced()
	{
		int c = m_InstancedSystems.Count();
		for ( int i = c - 1 ; i >= 0; i-- )
		{
			CInstancedResponseSystem *sys = m_InstancedSystems[ i ];
			sys->Release();
		}
		m_InstancedSystems.RemoveAll();
	}

	CUtlDict< CInstancedResponseSystem *, int > m_InstancedSystems;
};

IResponseSystem *CDefaultResponseSystem::BuildCustomResponseSystemGivenCriteria( const char *pszBaseFile, const char *pszCustomName, AI_CriteriaSet &criteriaSet, float flCriteriaScore )
{
	// Create a instanced response system. 
	CInstancedResponseSystem *pCustomSystem = new CInstancedResponseSystem( pszCustomName );
	if ( !pCustomSystem )
	{
		Error( "BuildCustomResponseSystemGivenCriterea: Failed to create custom response system %s!", pszCustomName );
	}

	pCustomSystem->Clear();

	// Copy the relevant rules and data.
	int nRuleCount = m_Rules.Count();
	for ( int iRule = 0; iRule < nRuleCount; ++iRule )
	{
		Rule *pRule = &m_Rules[iRule];
		if ( pRule )
		{
			float flScore = 0.0f;

			int nCriteriaCount = pRule->m_Criteria.Count();
			for ( int iCriteria = 0; iCriteria < nCriteriaCount; ++iCriteria )
			{
				int iRuleCriteria = pRule->m_Criteria[iCriteria];

				flScore += LookForCriteria( criteriaSet, iRuleCriteria );
				if ( flScore >= flCriteriaScore )
				{
					CopyRuleFrom( pRule, iRule, pCustomSystem );
					break;
				}
			}
		}
	}

	// Set as a custom response system.
	m_bCustomManagable = true;
	AddInstancedResponseSystem( pszCustomName, pCustomSystem );

//	pCustomSystem->DumpDictionary( pszCustomName );

	return pCustomSystem;
}

void CDefaultResponseSystem::DestroyCustomResponseSystems()
{
	ClearInstanced();
}


static CDefaultResponseSystem defaultresponsesytem;
IResponseSystem *g_pResponseSystem = &defaultresponsesytem;

CON_COMMAND( rr_reloadresponsesystems, "Reload all response system scripts." )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	defaultresponsesytem.ReloadAllResponseSystems();

#if defined( TF_DLL )
	// This is kind of hacky, but I need to get it in for now!
	if( g_pGameRules->IsMultiplayer() )
	{
		CMultiplayRules *pMultiplayRules = static_cast<CMultiplayRules*>( g_pGameRules );
		pMultiplayRules->InitCustomResponseRulesDicts();
	}
#endif
}

#ifdef MAPBASE
// Designed for extern magic, this gives the <, >, etc. of response system criteria to the outside world.
// Mostly just used for Matcher_Match in matchers.h.
bool ResponseSystemCompare(const char *criterion, const char *value)
{
	Criteria criteria;
	criteria.value = CopyString( criterion );
	defaultresponsesytem.ComputeMatcher(&criteria, criteria.matcher);
	return defaultresponsesytem.CompareUsingMatcher(value, criteria.matcher, true);

	return false;
}

// Another version that returns the criterion without the operators.
// I ended up not using this, but feel free to uncomment it.
// Just keep in mind it was scrapped before I could test it...
/*
const char *ResponseSystemCompare(const char *criterion, const char *value, bool bReturnToken)
{
	CResponseSystem *responsesys = dynamic_cast<CResponseSystem*>(g_pResponseSystem);
	if (responsesys)
	{
		Criteria criteria;
		criteria.value = CopyString( criterion );
		responsesys->ComputeMatcher(&criteria, criteria.matcher);
		return responsesys->CompareUsingMatcher(value, criteria.matcher, true) ? criteria.matcher.GetToken() : NULL;
	}
	return NULL;
}
*/

//-----------------------------------------------------------------------------
// CResponseFilePrecacher
//
// Purpose: Precaches a single talker file. That's it.
// 
// It copies from a bunch of the original Response System class and therefore it's really messy.
// Despite the horrors a normal programmer might find in here, I think it performs better than anything else I could've come up with.
//-----------------------------------------------------------------------------
class CResponseFilePrecacher
{
public:

	// Stuff copied from the Response System.
	// Direct copy-pastes are very compact, to say the least.
	inline bool ParseToken( void )
	{
		if ( m_bUnget )
		{ m_bUnget = false; return true; }
		if ( m_ScriptStack.Count() <= 0 )
		{ return false; }

		m_ScriptStack[ 0 ].currenttoken = engine->ParseFile( m_ScriptStack[ 0 ].currenttoken, token, sizeof( token ) );
		m_ScriptStack[ 0 ].tokencount++;
		return m_ScriptStack[ 0 ].currenttoken != NULL ? true : false;
	}

	CUtlVector< CResponseSystem::ScriptEntry >		m_ScriptStack;
	bool m_bUnget;
	char		token[ 1204 ];


	void PrecacheResponse( const char *response, byte type )
	{
		switch ( type )
		{
		default:
			break;
		case RESPONSE_SCENE:
			{
				DevMsg("Precaching scene %s...\n", response);

				// fixup $gender references
				char file[_MAX_PATH];
				Q_strncpy( file, response, sizeof(file) );
				char *gender = strstr( file, "$gender" );
				if ( gender )
				{
					// replace with male & female
					const char *postGender = gender + strlen("$gender");
					*gender = 0;
					char genderFile[_MAX_PATH];

					Q_snprintf( genderFile, sizeof(genderFile), "%smale%s", file, postGender);
					PrecacheInstancedScene( genderFile );

					Q_snprintf( genderFile, sizeof(genderFile), "%sfemale%s", file, postGender);
					PrecacheInstancedScene( genderFile );
				}
				else
				{
					PrecacheInstancedScene( file );
				}
			}
			break;
		case RESPONSE_SPEAK:
			{
				DevMsg("Precaching sound %s...\n", response);
				CBaseEntity::PrecacheScriptSound( response );
			}
			break;
		}
	}
	
	bool IsRootCommand()
	{
		if (!Q_stricmp( token, "#include" ) || !Q_stricmp( token, "response" )
			|| !Q_stricmp( token, "enumeration" ) || !Q_stricmp( token, "criteria" )
			|| !Q_stricmp( token, "criterion" ) || !Q_stricmp( token, "rule" ))
			return true;
		return false;
	}

	void ParseResponse( void )
	{
		// Must go to response group name
		ParseToken();

		while ( 1 )
		{
			ParseToken();

			if ( !Q_stricmp( token, "{" ) )
			{
				while ( 1 )
				{
					ParseToken();
					if ( !Q_stricmp( token, "}" ) )
						break;

					byte type = ComputeResponseType( token );
					if (type == RESPONSE_NONE)
						continue;

					ParseToken();
					char *value = CopyString( token );

					PrecacheResponse(value, type);
				}
				break;
			}

			byte type = ComputeResponseType( token );
			if (type == RESPONSE_NONE)
				break;

			ParseToken();
			char *value = CopyString( token );

			PrecacheResponse(value, type);

			break;
		}
	}

	bool LoadFromBuffer(const char *scriptfile, unsigned char *buffer, CStringPool &includedFiles)
	{
		includedFiles.Allocate( scriptfile );

		CResponseSystem::ScriptEntry e;
		e.name = filesystem->FindOrAddFileName( scriptfile );
		e.buffer = buffer;
		e.currenttoken = (char *)e.buffer;
		e.tokencount = 0;
		m_ScriptStack.AddToHead( e );

		while ( 1 )
		{
			ParseToken();
			if ( !token[0] )
			{
				break;
			}

			if ( !Q_stricmp( token, "response" ) )
			{
				ParseResponse();
			}
			else if ( !Q_stricmp( token, "#include" ) || !Q_stricmp( token, "#base" ) )
			{
				// Compacted version of ParseInclude(), including new changes.
				// Look at that if you want to read.
				char includefile[ 256 ];
				ParseToken();
				if (scriptfile) { size_t len = strlen(scriptfile)-1;
					for (size_t i = 0; i < len; i++)
					{ if (scriptfile[i] == CORRECT_PATH_SEPARATOR || scriptfile[i] == INCORRECT_PATH_SEPARATOR)
						{ len = i; }
					} Q_strncpy(includefile, scriptfile, len+1);
					if (len+1 != strlen(scriptfile))
					{ Q_snprintf(includefile, sizeof(includefile), "%s/%s", includefile, token); }
					else includefile[0] = '\0';
				} if (!includefile[0]) Q_snprintf( includefile, sizeof( includefile ), "scripts/%s", token );

				if ( includedFiles.Find( includefile ) == NULL )
				{
					MEM_ALLOC_CREDIT();

					// Try and load it
					CUtlBuffer buf;
					if ( filesystem->ReadFile( includefile, "GAME", buf ) )
					{
						LoadFromBuffer( includefile, (unsigned char *)buf.PeekGet(), includedFiles );
					}
				}
			}
		}

		if ( m_ScriptStack.Count() > 0 )
			m_ScriptStack.Remove( 0 );

		return true;
	}
};

// Loads a file directly to the main response system
bool LoadResponseSystemFile(const char *scriptfile)
{
	CUtlBuffer buf;
	if ( !filesystem->ReadFile( scriptfile, "GAME", buf ) )
	{
		return false;
	}

	// This is a really messy and specialized system that precaches the responses and only the responses of a talker file.
	CStringPool includedFiles;
	CResponseFilePrecacher *rs = new CResponseFilePrecacher();
	if (!rs || !rs->LoadFromBuffer(scriptfile, (unsigned char *)buf.PeekGet(), includedFiles))
	{
		Warning( "Failed to load response system data from %s", scriptfile );
		delete rs;
		return false;
	}
	delete rs;

	CStringPool includedFiles2;
	defaultresponsesytem.LoadFromBuffer(scriptfile, (const char *)buf.PeekGet(), includedFiles2);

	return true;
}

// Called from Mapbase manifests to flush
void ReloadResponseSystem()
{
	defaultresponsesytem.ReloadAllResponseSystems();
}
#endif

static short RESPONSESYSTEM_SAVE_RESTORE_VERSION = 1;

// note:  this won't save/restore settings from instanced response systems.  Could add that with a CDefSaveRestoreOps implementation if needed
// 
class CDefaultResponseSystemSaveRestoreBlockHandler : public CDefSaveRestoreBlockHandler
{
public:
	const char *GetBlockName()
	{
		return "ResponseSystem";
	}

	void WriteSaveHeaders( ISave *pSave )
	{
		pSave->WriteShort( &RESPONSESYSTEM_SAVE_RESTORE_VERSION );
	}
	
	void ReadRestoreHeaders( IRestore *pRestore )
	{
		// No reason why any future version shouldn't try to retain backward compatability. The default here is to not do so.
		short version;
		pRestore->ReadShort( &version );
		m_fDoLoad = ( version == RESPONSESYSTEM_SAVE_RESTORE_VERSION );
	}

	void Save( ISave *pSave )
	{
		CDefaultResponseSystem& rs = defaultresponsesytem;

		int count = rs.m_Responses.Count();
		pSave->WriteInt( &count );
		for ( int i = 0; i < count; ++i )
		{
			pSave->StartBlock( "ResponseGroup" );

			pSave->WriteString( rs.m_Responses.GetElementName( i ) );
			const ResponseGroup *group = &rs.m_Responses[ i ];
			pSave->WriteAll( group );

			short groupCount = group->group.Count();
			pSave->WriteShort( &groupCount );
			for ( int j = 0; j < groupCount; ++j )
			{
				const Response *response = &group->group[ j ];
				pSave->StartBlock( "Response" );
				pSave->WriteString( response->value );
				pSave->WriteAll( response );
				pSave->EndBlock();
			}

			pSave->EndBlock();
		}

#ifdef MAPBASE
		// Enhanced Response System save/restore
		int count2 = 0;
		if (rr_enhanced_saverestore.GetBool())
		{
			// Rule state save/load
			count2 = rs.m_Rules.Count();
			pSave->WriteInt( &count2 );
			for ( int i = 0; i < count2; ++i )
			{
				pSave->StartBlock( "Rule" );

				pSave->WriteString( rs.m_Rules.GetElementName( i ) );
				const Rule *rule = &rs.m_Rules[ i ];

				bool bEnabled = rule->m_bEnabled;
				pSave->WriteBool( &bEnabled );

				pSave->EndBlock();
			}
		}
		else
		{
			// Indicate this isn't using enhanced save/restore
			pSave->WriteInt( &count2 );
		}
#endif
	}

	void Restore( IRestore *pRestore, bool createPlayers )
	{
		if ( !m_fDoLoad )
			return;

		CDefaultResponseSystem& rs = defaultresponsesytem;

		int count = pRestore->ReadInt();
		for ( int i = 0; i < count; ++i )
		{
			char szResponseGroupBlockName[SIZE_BLOCK_NAME_BUF];
			pRestore->StartBlock( szResponseGroupBlockName );
			if ( !Q_stricmp( szResponseGroupBlockName, "ResponseGroup" ) )
			{

				char groupname[ 256 ];
				pRestore->ReadString( groupname, sizeof( groupname ), 0 );

				// Try and find it
				int idx = rs.m_Responses.Find( groupname );
				if ( idx != rs.m_Responses.InvalidIndex() )
				{
					ResponseGroup *group = &rs.m_Responses[ idx ];
					pRestore->ReadAll( group );

					short groupCount = pRestore->ReadShort();
					for ( int j = 0; j < groupCount; ++j )
					{
						char szResponseBlockName[SIZE_BLOCK_NAME_BUF];

						char responsename[ 256 ];
						pRestore->StartBlock( szResponseBlockName );
						if ( !Q_stricmp( szResponseBlockName, "Response" ) )
						{
							pRestore->ReadString( responsename, sizeof( responsename ), 0 );

							// Find it by name
							int ri;
							for ( ri = 0; ri < group->group.Count(); ++ri )
							{
								Response *response = &group->group[ ri ];
								if ( !Q_stricmp( response->value, responsename ) )
								{
									break;
								}
							}

							if ( ri < group->group.Count() )
							{
								Response *response = &group->group[ ri ];
								pRestore->ReadAll( response );
							}
						}

						pRestore->EndBlock();
					}
				}
			}

			pRestore->EndBlock();
		}

#ifdef MAPBASE
		// Enhanced Response System save/restore
		count = pRestore->ReadInt();
		for ( int i = 0; i < count; ++i )
		{
			char szRuleBlockName[SIZE_BLOCK_NAME_BUF];
			pRestore->StartBlock( szRuleBlockName );
			if ( !Q_stricmp( szRuleBlockName, "Rule" ) )
			{
				char groupname[ 256 ];
				pRestore->ReadString( groupname, sizeof( groupname ), 0 );

				// Try and find it
				int idx = rs.m_Rules.Find( groupname );
				if ( idx != rs.m_Rules.InvalidIndex() )
				{
					Rule *rule = &rs.m_Rules[ idx ];

					bool bEnabled;
					pRestore->ReadBool( &bEnabled );
					rule->m_bEnabled = bEnabled;
				}
			}

			pRestore->EndBlock();
		}
#endif
	}
private:

	bool		m_fDoLoad;

} g_DefaultResponseSystemSaveRestoreBlockHandler;
	
ISaveRestoreBlockHandler *GetDefaultResponseSystemSaveRestoreBlockHandler()
{
	return &g_DefaultResponseSystemSaveRestoreBlockHandler;
}

//-----------------------------------------------------------------------------
// CResponseSystemSaveRestoreOps
//
// Purpose: Handles save and load for instanced response systems...
//
// BUGBUG:  This will save the same response system to file multiple times for "shared" response systems and 
//  therefore it'll restore the same data onto the same pointer N times on reload (probably benign for now, but we could
//  write code to save/restore the instanced ones by filename in the block handler above maybe?
//-----------------------------------------------------------------------------

class CResponseSystemSaveRestoreOps : public CDefSaveRestoreOps
{
public:

	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{
		CResponseSystem *pRS = *(CResponseSystem **)fieldInfo.pField;
		if ( !pRS || pRS == &defaultresponsesytem )
			return;
		
		int count = pRS->m_Responses.Count();
		pSave->WriteInt( &count );
		for ( int i = 0; i < count; ++i )
		{
			pSave->StartBlock( "ResponseGroup" );

			pSave->WriteString( pRS->m_Responses.GetElementName( i ) );
			const ResponseGroup *group = &pRS->m_Responses[ i ];
			pSave->WriteAll( group );

			short groupCount = group->group.Count();
			pSave->WriteShort( &groupCount );
			for ( int j = 0; j < groupCount; ++j )
			{
				const Response *response = &group->group[ j ];
				pSave->StartBlock( "Response" );
				pSave->WriteString( response->value );
				pSave->WriteAll( response );
				pSave->EndBlock();
			}

			pSave->EndBlock();
		}
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		CResponseSystem *pRS = *(CResponseSystem **)fieldInfo.pField;
		if ( !pRS || pRS == &defaultresponsesytem )
			return;

		int count = pRestore->ReadInt();
		for ( int i = 0; i < count; ++i )
		{
			char szResponseGroupBlockName[SIZE_BLOCK_NAME_BUF];
			pRestore->StartBlock( szResponseGroupBlockName );
			if ( !Q_stricmp( szResponseGroupBlockName, "ResponseGroup" ) )
			{

				char groupname[ 256 ];
				pRestore->ReadString( groupname, sizeof( groupname ), 0 );

				// Try and find it
				int idx = pRS->m_Responses.Find( groupname );
				if ( idx != pRS->m_Responses.InvalidIndex() )
				{
					ResponseGroup *group = &pRS->m_Responses[ idx ];
					pRestore->ReadAll( group );

					short groupCount = pRestore->ReadShort();
					for ( int j = 0; j < groupCount; ++j )
					{
						char szResponseBlockName[SIZE_BLOCK_NAME_BUF];

						char responsename[ 256 ];
						pRestore->StartBlock( szResponseBlockName );
						if ( !Q_stricmp( szResponseBlockName, "Response" ) )
						{
							pRestore->ReadString( responsename, sizeof( responsename ), 0 );

							// Find it by name
							int ri;
							for ( ri = 0; ri < group->group.Count(); ++ri )
							{
								Response *response = &group->group[ ri ];
								if ( !Q_stricmp( response->value, responsename ) )
								{
									break;
								}
							}

							if ( ri < group->group.Count() )
							{
								Response *response = &group->group[ ri ];
								pRestore->ReadAll( response );
							}
						}

						pRestore->EndBlock();
					}
				}
			}

			pRestore->EndBlock();
		}
	}
	
} g_ResponseSystemSaveRestoreOps;

ISaveRestoreOps *responseSystemSaveRestoreOps = &g_ResponseSystemSaveRestoreOps;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDefaultResponseSystem::Init()
{
/*
	Warning( "sizeof( Response ) == %d\n", sizeof( Response ) );
	Warning( "sizeof( ResponseGroup ) == %d\n", sizeof( ResponseGroup ) );
	Warning( "sizeof( Criteria ) == %d\n", sizeof( Criteria ) );
	Warning( "sizeof( AI_ResponseParams ) == %d\n", sizeof( AI_ResponseParams ) );
*/
	const char *basescript = GetScriptFile();

	LoadRuleSet( basescript );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDefaultResponseSystem::Shutdown()
{
	// Wipe instanced versions
	ClearInstanced();

	// Clear outselves
	Clear();
	// IServerSystem chain
	BaseClass::Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Instance a custom response system
// Input  : *scriptfile - 
// Output : IResponseSystem
//-----------------------------------------------------------------------------
IResponseSystem *PrecacheCustomResponseSystem( const char *scriptfile )
{
	return defaultresponsesytem.PrecacheCustomResponseSystem( scriptfile );
}

//-----------------------------------------------------------------------------
// Purpose: Instance a custom response system
// Input  : *scriptfile -
//			set - 
// Output : IResponseSystem
//-----------------------------------------------------------------------------
IResponseSystem *BuildCustomResponseSystemGivenCriteria( const char *pszBaseFile, const char *pszCustomName, AI_CriteriaSet &criteriaSet, float flCriteriaScore )
{
	return defaultresponsesytem.BuildCustomResponseSystemGivenCriteria( pszBaseFile, pszCustomName, criteriaSet, flCriteriaScore );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void DestroyCustomResponseSystems()
{
	defaultresponsesytem.DestroyCustomResponseSystems();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CResponseSystem::DumpRules()
{
	int c = m_Rules.Count();
	int i;

	for ( i = 0; i < c; i++ )
	{
		Msg("%s\n", m_Rules.GetElementName( i ) );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CResponseSystem::DumpDictionary( const char *pszName )
{
	Msg( "\nDictionary: %s\n", pszName );

	int nRuleCount = m_Rules.Count();
	for ( int iRule = 0; iRule < nRuleCount; ++iRule )
	{
		Msg("	Rule %d: %s\n", iRule, m_Rules.GetElementName( iRule ) );

		Rule *pRule = &m_Rules[iRule];

		int nCriteriaCount = pRule->m_Criteria.Count();
		for( int iCriteria = 0; iCriteria < nCriteriaCount; ++iCriteria )
		{
			int iRuleCriteria = pRule->m_Criteria[iCriteria];
			Criteria *pCriteria = &m_Criteria[iRuleCriteria];
			Msg( "		Criteria %d: %s %s\n", iCriteria, pCriteria->name, pCriteria->value );
		}

		int nResponseGroupCount = pRule->m_Responses.Count();
		for ( int iResponseGroup = 0; iResponseGroup < nResponseGroupCount; ++iResponseGroup )
		{
			int iRuleResponse = pRule->m_Responses[iResponseGroup];
			ResponseGroup *pResponseGroup = &m_Responses[iRuleResponse];

			Msg( "		ResponseGroup %d: %s\n", iResponseGroup, m_Responses.GetElementName( iRuleResponse ) );

			int nResponseCount = pResponseGroup->group.Count();
			for ( int iResponse = 0; iResponse < nResponseCount; ++iResponse )
			{
				Response *pResponse = &pResponseGroup->group[iResponse];
				Msg( "			Response %d: %s\n", iResponse, pResponse->value );
			}
		}
	}
}
