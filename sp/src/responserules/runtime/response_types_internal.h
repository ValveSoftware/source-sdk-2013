//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Core types for the response rules -- criteria, responses, rules, and matchers.
//
// $NoKeywords: $
//=============================================================================//

#ifndef RESPONSE_TYPES_INTERNAL_H
#define RESPONSE_TYPES_INTERNAL_H
#ifdef _WIN32
#pragma once
#endif

#include "responserules/response_types.h"
#include "utldict.h"


namespace ResponseRules
{

	inline unsigned FASTCALL HashStringConventional( const char *pszKey )
	{
		unsigned hash = 0xAAAAAAAA; // Alternating 1's and 0's to maximize the effect of the later multiply and add

		for( ; *pszKey ; pszKey++ )
		{
			hash = ( ( hash << 5 ) + hash ) + (uint8)(*pszKey);
		}

		return hash;
	}

	// Note: HashString causes collisions!!!
#define RR_HASH HashStringConventional

#pragma pack(push,1)

	class Matcher
	{
	public:
		Matcher();

		void Describe( void );

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
		bool	isbit : 1;      //8
#endif

		void	SetToken( char const *s );

		char const *GetToken();

		void	SetRaw( char const *raw );

		char const *GetRaw();

	private:
		CUtlSymbol	token;
		CUtlSymbol	rawtoken;
	};
#pragma pack(pop)

	struct Criteria
	{
		Criteria();
		Criteria& operator =(const Criteria& src );

		Criteria(const Criteria& src );
		~Criteria();

		// Does this criterion recursively contain more criteria?
		inline bool IsSubCriteriaType() const
		{
			return ( subcriteria.Count() > 0 ) ? true : false;
		}

	    // const char					*name;
	    CUtlSymbol					nameSym;
		const char					*value;
		float16						weight;
		bool						required;

		Matcher						matcher;

		// Indices into sub criteria
		CUtlVectorConservative< unsigned short >	subcriteria;
	};

#pragma pack(push,1)
	/// This is a response block as read from the file, 
	/// different from CRR_Response which is what is handed
	/// back to queries.
	struct ParserResponse
	{
		DECLARE_SIMPLE_DATADESC_INSIDE_NAMESPACE();

		ParserResponse();
		ParserResponse( const ParserResponse& src );
		ParserResponse& operator =( const ParserResponse& src );
		~ParserResponse();

		ResponseType_t GetType() { return (ResponseType_t)type; }

		ResponseParams				params;

		const char					*value;  // fixed up value spot		// 4
		float16						weight;								// 6

		byte						depletioncount;						// 7
		byte						type : 6;							// 8
		byte						first : 1;							// 
		byte						last : 1;							// 

		ALIGN32 AI_ResponseFollowup	m_followup; // info on whether I should force the other guy to say something
	};
#pragma pack(pop)

#pragma pack(push,1)
	struct ResponseGroup
	{
		DECLARE_SIMPLE_DATADESC_INSIDE_NAMESPACE();

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
				ParserResponse *r = &group[ i ];

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
				ParserResponse *r = &group[ i ];

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

		CUtlVector< ParserResponse >	group;

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
#pragma pack(pop)

#pragma pack(push,1)
	struct Rule
	{
		Rule();
		Rule( const Rule& src );
		~Rule();
		Rule& operator =( const Rule& src );

		void SetContext( const char *context );

		const char *GetContext( void ) const { return m_szContext; }

		inline bool	IsEnabled() const { return m_bEnabled; }
		inline void	Disable() { m_bEnabled = false; }
		inline bool	IsMatchOnce() const { return m_bMatchOnce; }
#ifdef MAPBASE
		inline int	GetContextFlags() const { return m_iContextFlags; }
		inline bool	IsApplyContextToWorld() const { return (m_iContextFlags & APPLYCONTEXT_WORLD) != 0; }
#else
		inline bool	IsApplyContextToWorld() const { return m_bApplyContextToWorld; }
#endif

	    const char *GetValueForRuleCriterionByName( CResponseSystem *pSystem, const CUtlSymbol &pCritNameSym );
	    const Criteria *GetPointerForRuleCriterionByName( CResponseSystem *pSystem, const CUtlSymbol &pCritNameSym );

		// Indices into underlying criteria and response dictionaries
		CUtlVectorConservative< unsigned short >	m_Criteria;
		CUtlVectorConservative< unsigned short>		m_Responses;

		const char			*m_szContext;
		uint8				m_nForceWeight;

#ifdef MAPBASE
		// TODO: Could this cause any issues with the code optimization?
		uint8				m_iContextFlags;
#else
		bool				m_bApplyContextToWorld : 1;
#endif

		bool				m_bMatchOnce : 1;
		bool				m_bEnabled : 1;

	private:
		// what is this, lisp?
	    const char *RecursiveGetValueForRuleCriterionByName( CResponseSystem *pSystem, const Criteria *pCrit, const CUtlSymbol &pCritNameSym );
	    const Criteria *RecursiveGetPointerForRuleCriterionByName( CResponseSystem *pSystem, const Criteria *pCrit, const CUtlSymbol &pCritNameSym );
	};
#pragma pack(pop)

	template <typename T, typename I = unsigned short>
	class CResponseDict : public CUtlMap<unsigned int, T, I>
	{
	public:
		CResponseDict() : CUtlMap<unsigned int, T, I>( DefLessFunc( unsigned int ) ), m_ReverseMap( DefLessFunc( unsigned int ) )
		{
		}

		I Insert( const char *pName, const T &element )
		{
			extern const char *ResponseCopyString( const char *in );
			char const *pString = ResponseCopyString( pName );
			unsigned int hash = RR_HASH( pString );
			m_ReverseMap.Insert( hash, pString );
			return CUtlMap<unsigned int, T, I>::Insert( hash, element );
		}

		I Insert( const char *pName )
		{
			extern const char *ResponseCopyString( const char *in );
			char const *pString = ResponseCopyString( pName );
			unsigned int hash = RR_HASH( pString );
			m_ReverseMap.Insert( hash, pString );
			return CUtlMap<unsigned int, T, I>::Insert( hash );
		}

		I Find( char const *pName ) const
		{
			unsigned int hash = RR_HASH( pName );
			return CUtlMap<unsigned int, T, I>::Find( hash );
		}

		const char *GetElementName( I i )
		{
			int k = this->Key( i );
			int slot = m_ReverseMap.Find( k );
			if ( slot == m_ReverseMap.InvalidIndex() )
				return "";
			return m_ReverseMap[ slot ];
		}

		const char *GetElementName( I i ) const
		{
			int k = this->Key( i );
			int slot = m_ReverseMap.Find( k );
			if ( slot == m_ReverseMap.InvalidIndex() )
				return "";
			return m_ReverseMap[ slot ];
		}

	private:
		CUtlMap< unsigned int, const char * > m_ReverseMap;

	};

    // define this to 1 to enable printing some occupancy
    // information on the response system via concommmand
    // rr_dumphashinfo
    #define RR_DUMPHASHINFO_ENABLED 0
	// The Rules are partitioned based on a variety of factors (presently,
	// speaker and concept) for faster lookup, basically a seperate-chained hash.
	struct ResponseRulePartition
	{
		ResponseRulePartition( void );
		~ResponseRulePartition();

		typedef CResponseDict< Rule * > tRuleDict;
		typedef uint32 tIndex; // an integer that can be used to find any rule in the dict

		/// get the appropriate m_rules dict for the provided rule
		tRuleDict &GetDictForRule( CResponseSystem *pSystem, Rule *pRule );

	    /// get all bucket full of rules that might possibly match the given criteria.
		/// (right now they are bucketed such that all rules that can possibly match a 
	    ///  criteria are in one of two dictionaries)
	    void GetDictsForCriteria( CUtlVectorFixed< ResponseRulePartition::tRuleDict *, 2 > *pResult, const CriteriaSet &criteria );

		// dump everything.
		void RemoveAll();
#ifdef MAPBASE
		void PurgeAndDeleteElements();
#endif

		inline Rule &operator[]( tIndex idx );
		int Count( void ); // number of elements inside, but you can't iterate from 0 to this
		char const *GetElementName( const tIndex &i ) const;
		Rule *FindByName( char const *name ) const;

		/// given a dictionary and an element number inside that dict,
		/// return a tIndex
		tIndex IndexFromDictElem( tRuleDict* pDict, int elem );

		// for iteration:
		inline tIndex First( void );
		inline tIndex Next( const tIndex &idx );
		inline bool IsValid( const tIndex &idx ) const;
		inline static tIndex InvalidIdx( void ) 
		{
			return ((tIndex) -1);
		}

		// used only for debug prints, do not rely on them otherwise
		inline unsigned int BucketFromIdx( const tIndex &idx ) const ;
		inline unsigned int PartFromIdx( const tIndex &idx ) const ;

		enum { 
                  N_RESPONSE_PARTITIONS = 256,
                  kIDX_ELEM_MASK = 0xFFF, ///< this is used to mask the element number part of a ResponseRulePartition::tIndex 
                };

#if RR_DUMPHASHINFO_ENABLED
	    void PrintBucketInfo( CResponseSystem *pSys );
#endif

	private:
		tRuleDict m_RuleParts[N_RESPONSE_PARTITIONS];
	    unsigned int GetBucketForSpeakerAndConcept( const char *pszSpeaker, const char *pszConcept, const char *pszSubject );
	};

	// // // // // inline functions

	inline ResponseRulePartition::tIndex ResponseRulePartition::First( void )
	{
		// find the first bucket that has anything
		for ( int bucket = 0 ; bucket < N_RESPONSE_PARTITIONS; bucket++ )
		{
			if ( m_RuleParts[bucket].Count() > 0 )
				return bucket << 16;
		}
		return InvalidIdx();
	}

	inline ResponseRulePartition::tIndex ResponseRulePartition::Next( const tIndex &idx )
	{
		int bucket = BucketFromIdx( idx );
		unsigned int elem = PartFromIdx( idx );
		Assert( IsValid(idx) );
		AssertMsg( elem < kIDX_ELEM_MASK, "Too many response rules! Overflow! Doom!" );
		if ( elem + 1 < m_RuleParts[bucket].Count() )
		{
			return idx+1;
		}
		else
		{
			// walk through the other buckets, skipping empty ones, until we find one with responses and give up.
			while ( ++bucket < N_RESPONSE_PARTITIONS )
			{
				if ( m_RuleParts[bucket].Count() > 0 )
				{
					// 0th element in nth bucket
					return bucket << 16;
				}
			}

			// out of buckets
			return InvalidIdx();
			
		}
	}

	inline Rule &ResponseRulePartition::operator[]( tIndex idx )
	{
		Assert( IsValid(idx) );
		return *m_RuleParts[ BucketFromIdx(idx) ][ PartFromIdx(idx) ] ;
	}

	inline unsigned int ResponseRulePartition::BucketFromIdx( const tIndex &idx ) const
	{
		return idx >> 16;
	}

	inline unsigned int ResponseRulePartition::PartFromIdx( const tIndex &idx ) const
	{
		return idx & kIDX_ELEM_MASK;
	}

	inline bool ResponseRulePartition::IsValid( const tIndex & idx ) const
	{
		// make sure that the idx type for the dicts is still short
		COMPILE_TIME_ASSERT( sizeof(m_RuleParts[0].FirstInorder()) == 2 );

		if ( idx == -1 ) 
			return false;

		int bucket = idx >> 16;
		unsigned int elem = idx & kIDX_ELEM_MASK;

		return ( bucket < N_RESPONSE_PARTITIONS && 
			elem < m_RuleParts[bucket].Count() );
	}

	//-----------------------------------------------------------------------------
	// PARSER TYPES -- these are internal to the response system, and represent
	// the objects as loaded from disk. 
	//-----------------------------------------------------------------------------


}

#include "response_system.h"

#endif