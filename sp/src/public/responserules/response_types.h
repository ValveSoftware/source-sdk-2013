//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Core types for the response rules -- criteria, responses, rules, and matchers.
//
// $NoKeywords: $
//=============================================================================//

#ifndef RESPONSE_TYPES_H
#define RESPONSE_TYPES_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlrbtree.h"
#include "tier1/utlsymbol.h"
#include "tier1/interval.h"
#include "mathlib/compressed_vector.h"
#include "datamap.h"
#include "soundflags.h"
#include "tier1/utlsymbol.h"

namespace ResponseRules 
{
	/// Custom symbol table for the response rules.
	extern CUtlSymbolTable g_RS;
};

#ifdef _MANAGED
// forward declare some editor types just so we can friend them.
namespace ResponseRulesCLI
{
	ref class ResponseQueryResult;
}
#endif

namespace ResponseRules 
{
	using ::DataMapAccess;
	// using ::DataMapInit;
	class CResponseSystem;

#pragma pack(push,1)
	template<typename T>
	struct response_interval_t
	{
		T start;
		T range;

		interval_t &ToInterval( interval_t &dest ) const	{ dest.start = start; dest.range = range; return dest; }
		void FromInterval( const interval_t &from )			{ start = from.start; range = from.range; }
		float Random() const								{ interval_t temp = { start, range }; return RandomInterval( temp ); }
	};

	typedef response_interval_t<float16_with_assign> responseparams_interval_t;
#pragma pack(pop)

#pragma pack(push,1)
	struct AI_ResponseFollowup 
	{


		// TODO: make less wasteful of memory, by using a symbol table.
		const char					*followup_concept;							// 12  -- next response
		const char					*followup_contexts;							// 16
		float						followup_delay;								// 20
		const char 					*followup_target;							// 24 -- to whom is this despatched?
		// AIConceptHandle_t			hConcept;
		const char					*followup_entityiotarget;	//< if this rule involves firing entity io
		const char					*followup_entityioinput;	//< if this rule involves firing entity io
		float						followup_entityiodelay;
		bool						bFired;

		inline bool IsValid( void ) const { return (followup_concept && followup_contexts); }
		inline void Invalidate() { followup_concept = NULL; followup_contexts = NULL; }
		inline void SetFired( bool fired ) { bFired = fired; }
		inline bool HasBeenFired() { return bFired; }

		AI_ResponseFollowup( void ) : followup_concept(NULL), followup_contexts(NULL), followup_delay(0), followup_target(NULL), followup_entityiotarget(NULL), followup_entityioinput(NULL), followup_entityiodelay(0), bFired(false)
		{};
		AI_ResponseFollowup( char *_followup_concept, char *_followup_contexts, float _followup_delay, char *_followup_target,
			char *_followup_entityiotarget, char *_followup_entityioinput, float _followup_entityiodelay ) :
		followup_concept(_followup_concept), followup_contexts(_followup_contexts), followup_delay(_followup_delay), followup_target(_followup_target),
			followup_entityiotarget(_followup_entityiotarget), followup_entityioinput(_followup_entityioinput), followup_entityiodelay(_followup_entityiodelay),		
			bFired(false)
		{};
	};
#pragma pack(pop)


	enum ResponseType_t
	{
		RESPONSE_NONE = 0,
		RESPONSE_SPEAK,
		RESPONSE_SENTENCE,
		RESPONSE_SCENE,
		RESPONSE_RESPONSE, // A reference to another response by name
		RESPONSE_PRINT,
		RESPONSE_ENTITYIO, // poke an input on an entity
#ifdef MAPBASE
		RESPONSE_VSCRIPT, // Run VScript code
		RESPONSE_VSCRIPT_FILE, // Run a VScript file (bypasses ugliness and character limits when just using IncludeScript() with RESPONSE_VSCRIPT)
#endif

		NUM_RESPONSES,
	};

#ifdef MAPBASE
	// The "apply to world" context option has been replaced with a flag-based integer which can apply contexts to more things.
	// 
	// New ones should be implemented in: 
	// CResponseSystem::BuildDispatchTables() - AI_ResponseSystem.cpp (with their own funcs for m_RuleDispatch)
	// CRR_Response::Describe() - rr_response.cpp
	// CAI_Expresser::SpeakDispatchResponse() - ai_speech.cpp
	// 
	// Also mind that this is 8-bit
	enum : uint8
	{
		APPLYCONTEXT_SELF = (1 << 0), // Included for contexts that apply to both self and something else
		APPLYCONTEXT_WORLD = (1 << 1), // Apply to world

		APPLYCONTEXT_SQUAD = (1 << 2), // Apply to squad
		APPLYCONTEXT_ENEMY = (1 << 3), // Apply to enemy
	};
#endif


#pragma pack(push,1)
	struct ResponseParams
	{
		DECLARE_SIMPLE_DATADESC_INSIDE_NAMESPACE();

		enum
		{
			RG_DELAYAFTERSPEAK =	(1<<0),
			RG_SPEAKONCE =			(1<<1),
			RG_ODDS =				(1<<2),
			RG_RESPEAKDELAY =		(1<<3),
			RG_SOUNDLEVEL =			(1<<4),
			RG_DONT_USE_SCENE =		(1<<5),
			RG_STOP_ON_NONIDLE =	(1<<6),
			RG_WEAPONDELAY =		(1<<7),
			RG_DELAYBEFORESPEAK =	(1<<8),
		};

		ResponseParams()
		{
			flags = 0;
			odds = 100;
			delay.start = 0;
			delay.range = 0;
			respeakdelay.start = 0;
			respeakdelay.range = 0;
			weapondelay.start = 0;
			weapondelay.range = 0;
			soundlevel = 0;
			predelay.start = 0;
			predelay.range = 0;
		}
		responseparams_interval_t				delay;			//4
		responseparams_interval_t				respeakdelay;	//8
		responseparams_interval_t				weapondelay;	//12

		short					odds;							//14

		short					flags;							//16
		byte 					soundlevel;						//17

		responseparams_interval_t				predelay;		//21

		ALIGN32 AI_ResponseFollowup *m_pFollowup;

	};
#pragma pack(pop)

	class CriteriaSet
	{
	public:
		typedef CUtlSymbol CritSymbol_t; ///< just to make it clear that some symbols come out of our special static table
	public:
		CriteriaSet();
		CriteriaSet( const CriteriaSet& src );
		CriteriaSet( const char *criteria, const char *value ) ;  // construct initialized with a key/value pair (convenience)
		~CriteriaSet();

		static CritSymbol_t	ComputeCriteriaSymbol( const char *criteria );
		void		AppendCriteria( CritSymbol_t criteria, const char *value = "", float weight = 1.0f );
		void		AppendCriteria( const char *criteria, const char *value = "", float weight = 1.0f );
		void		AppendCriteria( const char *criteria, float value, float weight = 1.0f );
		void		RemoveCriteria( const char *criteria );

		void		Describe() const;

		int			GetCount() const;
		int			FindCriterionIndex( CritSymbol_t criteria ) const;
		int			FindCriterionIndex( const char *name ) const;
		inline bool	IsValidIndex( int index ) const; 

		CritSymbol_t	GetNameSymbol( int nIndex ) const;
		inline static const char *SymbolToStr( const CritSymbol_t &symbol );
		const char *GetName( int index ) const;
		const char *GetValue( int index ) const;
		float		GetWeight( int index ) const;

		/// Merge another CriteriaSet into this one.
		void		Merge( const CriteriaSet *otherCriteria );
		void		Merge( const char *modifiers ); // add criteria parsed from a text string

		/// add all of the contexts herein onto an entity. all durations are infinite.
		void		WriteToEntity( CBaseEntity *pEntity );

		// Accessors to things that need only be done under unusual circumstances.
		inline void EnsureCapacity( int num );
		void Reset(); // clear out this criteria (should not be necessary)

		/// When this is true, calls to AppendCriteria on a criteria that already exists
		/// will override the existing value. (This is the default behavior). Can be temporarily
		/// set false to prevent such overrides.
		inline void	OverrideOnAppend( bool bOverride ) { m_bOverrideOnAppend = bOverride; }

		// For iteration from beginning to end (also should not be necessary except in
		// save/load)
		inline int  Head() const;
		inline int  Next( int i ) const; 	// use with IsValidIndex above

		const static char kAPPLYTOWORLDPREFIX = '$';

		/// A last minute l4d2 change: deferred contexts prefixed with a '$'
		/// character are actually applied to the world. This matches the 
		/// related hack in CBaseEntity::AppplyContext.
		/// This function works IN-PLACE on the "from" parameter.
		/// any $-prefixed criteria in pFrom become prefixed by "world",
		/// and are also written into pSetOnWorld.
		/// *IF* a response matches using the modified criteria, then and only
		/// then should you write back the criteria in pSetOnWorld to the world
		/// entity, subsequent to the match but BEFORE the dispatch.
		/// Returns the number of contexts modified. If it returns 0, then 
		/// pSetOnWorld is empty.
		static int InterceptWorldSetContexts( CriteriaSet * RESTRICT pFrom, 
			CriteriaSet * RESTRICT pSetOnWorld );

	private:
		void		RemoveCriteria( int idx, bool bTestForPrefix );

		struct CritEntry_t
		{
			CritEntry_t() :
				criterianame( UTL_INVAL_SYMBOL ),
				weight( 0.0f )
			{
				value[ 0 ] = 0;
			}

			CritEntry_t( const CritEntry_t& src )
			{
				criterianame = src.criterianame;
				value[ 0 ] = 0;
				weight = src.weight;
				SetValue( src.value );
			}

			CritEntry_t& operator=( const CritEntry_t& src )
			{
				if ( this == &src )
					return *this;

				criterianame = src.criterianame;
				weight = src.weight;
				SetValue( src.value );

				return *this;
			}

			static bool LessFunc( const CritEntry_t& lhs, const CritEntry_t& rhs )
			{
				return lhs.criterianame < rhs.criterianame;
			}

			void SetValue( char const *str )
			{
				if ( !str )
				{
					value[ 0 ] = 0;
				}
				else
				{
					Q_strncpy( value, str, sizeof( value ) );
				}
			}

			CritSymbol_t criterianame;
			char		value[ 64 ];
			float		weight;
		};

		static CUtlSymbolTable sm_CriteriaSymbols;
		typedef CUtlRBTree< CritEntry_t, short > Dict_t;
		Dict_t m_Lookup;
		int m_nNumPrefixedContexts; // number of contexts prefixed with kAPPLYTOWORLDPREFIX
		bool m_bOverrideOnAppend;
	};

	inline void CriteriaSet::EnsureCapacity( int num )
	{
		m_Lookup.EnsureCapacity(num);
	}

	//-----------------------------------------------------------------------------
	// Purpose: Generic container for a response to a match to a criteria set
	//  This is what searching for a response returns
	//-----------------------------------------------------------------------------

	class CRR_Response
	{
	public:
		DECLARE_SIMPLE_DATADESC_INSIDE_NAMESPACE();

		CRR_Response();
		CRR_Response( const CRR_Response &from );
		CRR_Response &operator=( const CRR_Response &from );
		~CRR_Response();
	private:
		void operator delete(void* p); // please do not new or delete CRR_Responses.
	public:

		// void	Release(); // we no longer encourage new and delete on these things

		void			GetName( char *buf, size_t buflen ) const;
		void			GetResponse( char *buf, size_t buflen ) const;
#ifdef MAPBASE
		void			GetRule( char *buf, size_t buflen ) const;
#endif
		const char* GetNamePtr() const;
		const char* GetResponsePtr() const;
		const ResponseParams *GetParams() const { return &m_Params; }
		ResponseType_t	GetType() const { return (ResponseType_t)m_Type; }
		soundlevel_t	GetSoundLevel() const;
		float			GetRespeakDelay() const;
		float			GetWeaponDelay() const;
		bool			GetSpeakOnce() const;
		bool			ShouldntUseScene( ) const;
		bool			ShouldBreakOnNonIdle( void ) const;
		int				GetOdds() const;
		float			GetDelay() const;
		float			GetPreDelay() const;

		inline bool		IsEmpty() const; // true iff my response name is empty 
		void			Invalidate() ; // wipe out my contents, mark me invalid

		// Get/set the contexts we apply to character and world after execution
		void			SetContext( const char *context );
		const char *	GetContext( void ) const { return m_szContext; }

		// Get/set the score I matched with (under certain circumstances)
		inline float	GetMatchScore( void ) { return m_fMatchScore; }
		inline void		SetMatchScore( float f ) { m_fMatchScore = f; }

#ifdef MAPBASE
		int				GetContextFlags() { return m_iContextFlags; }
		bool			IsApplyContextToWorld( void ) { return (m_iContextFlags & APPLYCONTEXT_WORLD) != 0; }

		inline short	*GetInternalIndices() { return m_InternalIndices; }
		inline void		SetInternalIndices( short iGroup, short iWithinGroup ) { m_InternalIndices[0] = iGroup; m_InternalIndices[1] = iWithinGroup; }
#else
		bool			IsApplyContextToWorld( void ) { return m_bApplyContextToWorld; }
#endif

		void Describe( const CriteriaSet *pDebugCriteria = NULL ); 

		void	Init( ResponseType_t type, 
			const char *responseName, 
			const ResponseParams& responseparams,
			const char *matchingRule,
			const char *applyContext,
			bool bApplyContextToWorld );

#ifdef MAPBASE
		void	Init( ResponseType_t type,
			const char *responseName,
			const ResponseParams& responseparams,
			const char *matchingRule,
			const char *applyContext,
			int iContextFlags );
#endif

		static const char *DescribeResponse( ResponseType_t type );

		enum
		{
			MAX_RESPONSE_NAME = 64,
			MAX_RULE_NAME = 64
		};


	private:
		byte			m_Type;
		char			m_szResponseName[ MAX_RESPONSE_NAME ];
		char			m_szMatchingRule[ MAX_RULE_NAME ];

		ResponseParams m_Params;
		float	m_fMatchScore; // when instantiated dynamically in SpeakFindResponse, the score of the rule that matched it.

		char *			m_szContext; // context data we apply to character after running
#ifdef MAPBASE
		int				m_iContextFlags;

		// The response's original indices in the system. [0] is the group's index, [1] is the index within the group.
		// For now, this is only set in prospecctive mode. It's used to call back to the ParserResponse and mark a prospectively chosen response as used.
		short			m_InternalIndices[2];
#else
		bool			m_bApplyContextToWorld;
#endif

#ifdef _MANAGED
		friend ref class ResponseRulesCLI::ResponseQueryResult;
#endif
	};



	abstract_class IResponseFilter
	{
	public:
		virtual bool IsValidResponse( ResponseType_t type, const char *pszValue ) = 0;
	};

	abstract_class IResponseSystem
	{
	public:
		virtual ~IResponseSystem() {}

		virtual bool FindBestResponse( const CriteriaSet& set, CRR_Response& response, IResponseFilter *pFilter = NULL ) = 0;
		virtual void GetAllResponses( CUtlVector<CRR_Response> *pResponses ) = 0;
		virtual void PrecacheResponses( bool bEnable ) = 0;

#ifdef MAPBASE
		// (Optional) Call this before and after using FindBestResponse() for a prospective lookup, e.g. a response that might not actually be used
		// and should not trigger displayfirst, etc.
		virtual void SetProspective( bool bToggle ) {};

		// (Optional) Marks a prospective response as used
		virtual void MarkResponseAsUsed( short iGroup, short iWithinGroup ) {};
#endif
	};



	// INLINE FUNCTIONS

	// Used as a failsafe in finding responses.
	bool CRR_Response::IsEmpty() const
	{
		return m_szResponseName[0] == 0;
	}

	inline bool CriteriaSet::IsValidIndex( int index ) const
	{
		return ( index >= 0 && index < ((int)(m_Lookup.Count())) );
	}

	inline int CriteriaSet::Head() const
	{
		return m_Lookup.FirstInorder();
	}

	inline int CriteriaSet::Next( int i ) const
	{
		return m_Lookup.NextInorder(i);
	}

	inline const char *CriteriaSet::SymbolToStr( const CritSymbol_t &symbol )
	{
		return sm_CriteriaSymbols.String(symbol);
	}

}

#include "rr_speechconcept.h"
#include "response_host_interface.h"

#endif
