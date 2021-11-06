//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: The CResponseSystem class. Don't include this header; include the response_types
// into which it is transcluded.
//
// $NoKeywords: $
//=============================================================================//

#ifndef RESPONSE_SYSTEM_H
#define RESPONSE_SYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "utldict.h"

namespace ResponseRules
{
	typedef ResponseParams	AI_ResponseParams ;
	#define AI_CriteriaSet ResponseRules::CriteriaSet 

	//-----------------------------------------------------------------------------
	// Purpose: The database of all available responses.
	// The Rules are partitioned based on a variety of factors (presently,
	// speaker and concept) for faster lookup, basically a seperate-chained hash.
	//-----------------------------------------------------------------------------
	class CResponseSystem : public IResponseSystem 
	{
	public:
		CResponseSystem();
		~CResponseSystem();

		typedef void (CResponseSystem::*pfnResponseDispatch)( void );
		typedef void (CResponseSystem::*pfnParseRuleDispatch)( Rule & );
		typedef void (CResponseSystem::*pfnParseResponseDispatch)( ParserResponse &, ResponseGroup&, AI_ResponseParams * );
		typedef void (CResponseSystem::*pfnParseResponseGroupDispatch) ( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );

		typedef CUtlMap< unsigned,pfnResponseDispatch > DispatchMap_t;  
		typedef CUtlMap< unsigned,pfnParseRuleDispatch > ParseRuleDispatchMap_t;  
		typedef CUtlMap< unsigned,pfnParseResponseDispatch > ParseResponseDispatchMap_t;  
		typedef CUtlMap< unsigned,pfnParseResponseGroupDispatch > ParseResponseGroupDispatchMap_t;  

#pragma region IResponseSystem
		// IResponseSystem
		virtual bool FindBestResponse( const CriteriaSet& set, CRR_Response& response, IResponseFilter *pFilter = NULL );
		virtual void GetAllResponses( CUtlVector<CRR_Response> *pResponses );

#ifdef MAPBASE
		virtual void SetProspective( bool bToggle ) { m_bInProspective = bToggle; }

		virtual void MarkResponseAsUsed( short iGroup, short iWithinGroup );
#endif
#pragma endregion Implement interface from IResponseSystem

		virtual void Release() = 0;

		virtual void DumpRules();

		bool		IsCustomManagable()	{ return m_bCustomManagable; }

#ifdef MAPBASE
		virtual
#endif
		void		Clear();

		void		DumpDictionary( const char *pszName );

	protected:

		void		BuildDispatchTables();
		bool		Dispatch( char const *pToken, unsigned int uiHash, DispatchMap_t &rMap );
		bool		DispatchParseRule( char const *pToken, unsigned int uiHash, ParseRuleDispatchMap_t &rMap, Rule &newRule );
		bool		DispatchParseResponse( char const *pToken, unsigned int uiHash, ParseResponseDispatchMap_t &rMap, ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		bool		DispatchParseResponseGroup( char const *pToken, unsigned int uiHash, ParseResponseGroupDispatchMap_t &rMap, char const *responseGroupName, ResponseGroup& newGroup, AI_ResponseParams &groupResponseParams );

		virtual const char *GetScriptFile( void ) = 0;
		void		LoadRuleSet( const char *setname );

		void		ResetResponseGroups();

		float		LookForCriteria( const CriteriaSet &criteriaSet, int iCriteria );
		float		RecursiveLookForCriteria( const CriteriaSet &criteriaSet, Criteria *pParent );

	public:

		void		CopyRuleFrom( Rule *pSrcRule, ResponseRulePartition::tIndex iRule, CResponseSystem *pCustomSystem );
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
			ParserResponse		*action;
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

			m_ScriptStack[ 0 ].currenttoken = IEngineEmulator::Get()->ParseFile( m_ScriptStack[ 0 ].currenttoken, token, sizeof( token ) );
			m_ScriptStack[ 0 ].tokencount++;
			return m_ScriptStack[ 0 ].currenttoken != NULL ? true : false;
		}

#ifdef MAPBASE
		inline bool ParseTokenIntact( void )
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

			m_ScriptStack[ 0 ].currenttoken = IEngineEmulator::Get()->ParseFilePreserve( m_ScriptStack[ 0 ].currenttoken, token, sizeof( token ) );
			m_ScriptStack[ 0 ].tokencount++;
			return m_ScriptStack[ 0 ].currenttoken != NULL ? true : false;
		}
#endif

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
				Error( "AI_ResponseSystem:  Unxpected TokenWaiting() with NULL buffer in %s", (char * ) m_ScriptStack[ 0 ].name );
				return false;
			}


			while ( *p && *p!='\n')
			{
				// Special handler for // comment blocks
				if ( *p == '/' && *(p+1) == '/' )
					return false;

				if ( !V_isspace( *p ) || isalnum( *p ) )
					return true;

				p++;
			}

			return false;
		}

		void		ParseOneResponse( const char *responseGroupName, ResponseGroup& group, ResponseParams *defaultParams = NULL );

		void		ParseInclude( void );
		void		ParseResponse( void );
		void		ParseCriterion( void );
		void		ParseRule( void );
		void		ParseEnumeration( void );

	private:
		void		ParseRule_MatchOnce( Rule &newRule );
		void		ParseRule_ApplyContextToWorld( Rule &newRule );
#ifdef MAPBASE
		void		ParseRule_ApplyContextToSquad( Rule &newRule );
		void		ParseRule_ApplyContextToEnemy( Rule &newRule );
#endif
		void		ParseRule_ApplyContext( Rule &newRule );
		void		ParseRule_Response( Rule &newRule );
		//void		ParseRule_ForceWeight( Rule &newRule );
		void		ParseRule_Criteria( Rule &newRule );
		char const	*m_pParseRuleName;
		bool		m_bParseRuleValid;

		void		ParseResponse_Weight( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_PreDelay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_NoDelay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_DefaultDelay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_Delay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_SpeakOnce( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_NoScene( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_StopOnNonIdle( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_Odds( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_RespeakDelay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_WeaponDelay( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_Soundlevel( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_DisplayFirst( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_DisplayLast( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_Fire( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );
		void		ParseResponse_Then( ParserResponse &newResponse, ResponseGroup& group, AI_ResponseParams *rp );

		void		ParseResponseGroup_Start( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );
		void		ParseResponseGroup_PreDelay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );
		void		ParseResponseGroup_NoDelay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );
		void		ParseResponseGroup_DefaultDelay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );
		void		ParseResponseGroup_Delay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );
		void		ParseResponseGroup_SpeakOnce( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );
		void		ParseResponseGroup_NoScene( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );
		void		ParseResponseGroup_StopOnNonIdle( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );
		void		ParseResponseGroup_Odds( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );
		void		ParseResponseGroup_RespeakDelay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );
		void		ParseResponseGroup_WeaponDelay( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );
		void		ParseResponseGroup_Soundlevel( char const *responseGroupName, ResponseGroup &newGroup, AI_ResponseParams &groupResponseParams );

public:
		int			ParseOneCriterion( const char *criterionName );

		bool		Compare( const char *setValue, Criteria *c, bool verbose = false );
		bool		CompareUsingMatcher( const char *setValue, Matcher& m, bool verbose = false );
		void		ComputeMatcher( Criteria *c, Matcher& matcher );
		void		ResolveToken( Matcher& matcher, char *token, size_t bufsize, char const *rawtoken );
		float		LookupEnumeration( const char *name, bool& found );

		ResponseRulePartition::tIndex FindBestMatchingRule( const CriteriaSet& set, bool verbose, float &scoreOfBestMatchingRule );

#ifdef MAPBASE
		void		DisableEmptyRules();
#endif
		
		float		ScoreCriteriaAgainstRule( const CriteriaSet& set, ResponseRulePartition::tRuleDict &dict, int irule, bool verbose = false );
		float		RecursiveScoreSubcriteriaAgainstRule( const CriteriaSet& set, Criteria *parent, bool& exclude, bool verbose /*=false*/ );
		float		ScoreCriteriaAgainstRuleCriteria( const CriteriaSet& set, int icriterion, bool& exclude, bool verbose = false );
		void		FakeDepletes( ResponseGroup *g, IResponseFilter *pFilter );
		void		RevertFakedDepletes( ResponseGroup *g );
		bool		GetBestResponse( ResponseSearchResult& result, Rule *rule, bool verbose = false, IResponseFilter *pFilter = NULL );
		bool		ResolveResponse( ResponseSearchResult& result, int depth, const char *name, bool verbose = false, IResponseFilter *pFilter = NULL );
		int			SelectWeightedResponseFromResponseGroup( ResponseGroup *g, IResponseFilter *pFilter );
		void		DescribeResponseGroup( ResponseGroup *group, int selected, int depth );
		void		DebugPrint( int depth, const char *fmt, ... );

		void		LoadFromBuffer( const char *scriptfile, const char *buffer );

		void		GetCurrentScript( char *buf, size_t buflen );
		int			GetCurrentToken() const;
		void		SetCurrentScript( const char *script );
		
		inline bool IsRootCommand( unsigned int hash ) const
		{
			int slot = m_RootCommandHashes.Find( hash );
			return slot != m_RootCommandHashes.InvalidIndex();
		}

		inline bool IsRootCommand() const
		{
			return IsRootCommand( RR_HASH( token ) );
		}

		void		PushScript( const char *scriptfile, unsigned char *buffer );
		void		PopScript(void);

		void		ResponseWarning( const char *fmt, ... );

		CUtlDict< ResponseGroup, short >	m_Responses;
		CUtlDict< Criteria, short >	m_Criteria;
		// CUtlDict< Rule, short >	m_Rules;
		ResponseRulePartition m_RulePartitions;
		CUtlDict< Enumeration, short > m_Enumerations;

		CUtlVector<int> m_FakedDepletes;

		char		token[ 1204 ];

		bool		m_bUnget;

		bool		m_bCustomManagable;

#ifdef MAPBASE
		// This is a hack specifically designed to fix displayfirst, speakonce, etc. in "prospective" response searches,
		// especially the prospective lookups in followup responses.
		// It works by preventing responses from being marked as "used".
		bool		m_bInProspective;
#endif

		struct ScriptEntry
		{
			unsigned char	*buffer;
			FileNameHandle_t name;
			const char		*currenttoken;
			int				tokencount;
		};

		CUtlVector< ScriptEntry >		m_ScriptStack;
		CStringPool						m_IncludedFiles;

		DispatchMap_t					m_FileDispatch;
		ParseRuleDispatchMap_t			m_RuleDispatch;
		ParseResponseDispatchMap_t		m_ResponseDispatch;
		ParseResponseGroupDispatchMap_t m_ResponseGroupDispatch;
		CUtlRBTree< unsigned int >		m_RootCommandHashes;

        // for debugging purposes only: concepts to be emitted from rr_debugresponses 2
        typedef CUtlLinkedList< CRR_Concept, unsigned short, false, unsigned int > ExcludeList_t;
        static ExcludeList_t m_DebugExcludeList;

		friend class CDefaultResponseSystemSaveRestoreBlockHandler;
		friend class CResponseSystemSaveRestoreOps;
	};

	// Some globals inherited from AI_Speech.h:
	const float AIS_DEF_MIN_DELAY 	= 2.8; // Minimum amount of time an NPCs will wait after someone has spoken before considering speaking again
	const float AIS_DEF_MAX_DELAY 	= 3.2; // Maximum amount of time an NPCs will wait after someone has spoken before considering speaking again
}

#endif // RESPONSE_SYSTEM_H