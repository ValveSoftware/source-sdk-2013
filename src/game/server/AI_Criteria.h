//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_CRITERIA_H
#define AI_CRITERIA_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlrbtree.h"
#include "tier1/utlsymbol.h"
#include "interval.h"
#include "mathlib/compressed_vector.h"

extern const char *SplitContext( const char *raw, char *key, int keylen, char *value, int valuelen, float *duration );


class AI_CriteriaSet
{
public:
	AI_CriteriaSet();
	AI_CriteriaSet( const AI_CriteriaSet& src );
	~AI_CriteriaSet();

	void AppendCriteria( const char *criteria, const char *value = "", float weight = 1.0f );
	void RemoveCriteria( const char *criteria );
	
	void Describe();

	int GetCount() const;
	int			FindCriterionIndex( const char *name ) const;

	const char *GetName( int index ) const;
	const char *GetValue( int index ) const;
	float		GetWeight( int index ) const;

private:

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
			return Q_stricmp( lhs.criterianame.String(), rhs.criterianame.String() ) < 0 ? true : false;
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

		// We use CUtlRBTree CopyFrom() in ctor, so CritEntry_t must be POD. If you add
		// CUtlString or something then you must change AI_CriteriaSet copy ctor.
		CUtlSymbol	criterianame;
		char		value[ 64 ];
		float		weight;
	};

	CUtlRBTree< CritEntry_t, short > m_Lookup;
};

#pragma pack(1)
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

struct AI_ResponseParams
{
	DECLARE_SIMPLE_DATADESC();

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

	AI_ResponseParams()
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
};
#pragma pack()

//-----------------------------------------------------------------------------
// Purpose: Generic container for a response to a match to a criteria set
//  This is what searching for a response returns
//-----------------------------------------------------------------------------
enum ResponseType_t
{
	RESPONSE_NONE = 0,
	RESPONSE_SPEAK,
	RESPONSE_SENTENCE,
	RESPONSE_SCENE,
	RESPONSE_RESPONSE, // A reference to another response by name
	RESPONSE_PRINT,

	NUM_RESPONSES,
};

class AI_Response
{
public:
	DECLARE_SIMPLE_DATADESC();

	AI_Response();
	AI_Response( const AI_Response &from );
	~AI_Response();
	AI_Response &operator=( const AI_Response &from );

	void			Release();

	const char *	GetNamePtr() const;
	const char *	GetResponsePtr() const;
	const AI_ResponseParams *GetParams() const { return &m_Params; }
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

	void			SetContext( const char *context );
	const char *	GetContext( void ) const { return m_szContext.Length() ? m_szContext.Get() : NULL; }

	bool			IsApplyContextToWorld( void ) { return m_bApplyContextToWorld; }

	void Describe();

	const AI_CriteriaSet* GetCriteria();

	void	Init( ResponseType_t type, 
				const char *responseName, 
				const AI_CriteriaSet& criteria, 
				const AI_ResponseParams& responseparams,
				const char *matchingRule,
				const char *applyContext,
				bool bApplyContextToWorld );

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

	// The initial criteria to which we are responsive
	AI_CriteriaSet	*m_pCriteria;

	AI_ResponseParams m_Params;

	CUtlString		m_szContext;
	bool			m_bApplyContextToWorld;
};

#endif // AI_CRITERIA_H
