//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: "Force-field" obstacle avoidance & steering
//
// @Note (toml 06-18-02): Currently only controls direction. Ultimately could
// also incorporate body facing (yaw), speed, and translational/rotational
// acceleration.
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_MOVESOLVER_H
#define AI_MOVESOLVER_H

#if defined( _WIN32 )
#pragma once
#endif

#include "utlvector.h"
#include "ai_obstacle_type.h"


//-----------------------------------------------------------------------------

inline float NormalizeAngle( float angle )
{
	if ( angle < 0.0 )
		angle += 360.0;
	else if ( angle >= 360.0 )
		angle -= 360.0;
	return angle;
}

//-----------------------------------------------------------------------------
// ENUMERATIONS
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// STRUCTURES
//-----------------------------------------------------------------------------

//-------------------------------------
// AI_Arc_t
//
// Purpose: Represents an arc.
//
//-------------------------------------
struct AI_Arc_t
{
	AI_Arc_t()
	 : 	center( 0 ),
	 	span( 0 )
	{
	}

	// Set by center and span
	void Set( float newCenter, float newSpan );

	// Set by the right and left extremes (coordinates run counter clockwise)
	void SetByLimits( float yawRight, float yawLeft );

	// Center of the arc (as "yaw")
	float center;

	// Span of the arc (in degrees)
	float span;
};

//-------------------------------------
// AI_MoveSuggestion_t
//
// Purpose: Suggests a possible move/avoidance, with a range of acceptable alternatives
//
// @Note (toml 06-20-02): this probably will eventually want to incorporate facing and
// destination of the motivating goal.
//
//-------------------------------------

enum AI_MoveSuggestionFlags_t
{
	AIMS_FAVOR_LEFT		= 0x01,
	AIMS_FAVOR_RIGHT	= 0x02
};

//-----------------

struct AI_MoveSuggestion_t
{
	AI_MoveSuggestion_t();
	AI_MoveSuggestion_t( AI_MoveSuggType_t newType, float newWeight, float newDir, float newSpan, CBaseEntity *pEntity = NULL );
	AI_MoveSuggestion_t( AI_MoveSuggType_t newType, float newWeight, const AI_Arc_t &arc, CBaseEntity *pEntity = NULL );

	void Set( AI_MoveSuggType_t newType, float newWeight, float newDir, float newSpan, CBaseEntity *pEntity = NULL );
	void Set( AI_MoveSuggType_t newType, float newWeight, const AI_Arc_t &arc, CBaseEntity *pEntity = NULL );

	//---------------------------------

	// The kind of suggestion
	AI_MoveSuggType_t	type;

	// The unadjusted weight of the suggestion [0..1], although [-1..1] within the solver
	float 				weight;

	// The desired direction to move/avoid
	AI_Arc_t			arc;

	// The causing entity, if any
	EHANDLE				hObstacleEntity;
	
	// Flags
	unsigned			flags;

};

//-----------------

typedef CUtlVector<AI_MoveSuggestion_t> CAI_MoveSuggestions;

//-------------------------------------
// AI_MoveSolution_t
//
// Purpose: The result of resolving suggestions
//
// @Note (toml 06-18-02): Currently, this is a very dopey little structure.
// However, it will probably eventually incorporate much of the info
// passed or calculated piecemeal between Move...() and Move...Execute()
// functions. Once suggestions incorprate more information, the solution
// may want to include a copy of the winning suggestion, so that the
// caller need retain less state. If this is not the case, reduce it to just
// a yaw.
//
//-------------------------------------
struct AI_MoveSolution_t
{
	AI_MoveSolution_t()
	 :	dir(0)
	{
	}

	// The direction to move
	float dir;
};

//-----------------------------------------------------------------------------
// class CAI_MoveSolver
//
// Purpose: Given a set of precalculated "regulations" (typically negative),
//			and a set of instantaneous suggestions (usually positive)
//-----------------------------------------------------------------------------

class CAI_MoveSolver
{
public:
	CAI_MoveSolver();

	//---------------------------------
	// Purpose: A regulation is a suggestion that is kept around as a rule until
	//			cleared. They are generally negative suggestions.
	//---------------------------------
	void AddRegulation( const AI_MoveSuggestion_t &suggestion );
	void AddRegulations( const AI_MoveSuggestion_t *pSuggestion, int nSuggestions );

	bool HaveRegulations() const;
	void ClearRegulations();

	//---------------------------------
	// Purpose: Solve the move, picking the best direction from a set of suggestions,
	//			after applying the regulations
	//---------------------------------
	bool Solve( const AI_MoveSuggestion_t *pSuggestions, int nSuggestions, AI_MoveSolution_t *pResult );
	bool Solve( const AI_MoveSuggestion_t &suggestion, AI_MoveSolution_t *pResult );

	//---------------------------------
	bool HaveRegulationForObstacle( CBaseEntity *pEntity);

	//---------------------------------
	// Visualization
	void VisualizeRegulations( const Vector& origin );

private:
	enum
	{
		REGS_RESERVE = 8,
	};

	//---------------------------------
	void NormalizeSuggestions( AI_MoveSuggestion_t *pBegin, AI_MoveSuggestion_t *pEnd );

	//---------------------------------
	CAI_MoveSuggestions m_Regulations;
};

//-----------------------------------------------------------------------------
// AI_Arc_t inline methods
//-----------------------------------------------------------------------------

inline void AI_Arc_t::Set( float newCenter, float newSpan )
{
	center = NormalizeAngle( newCenter );
	span   = NormalizeAngle( newSpan );
}

//-------------------------------------

inline void AI_Arc_t::SetByLimits( float yawRight, float yawLeft )
{
	// Yaw runs counter-clockwise
	span = yawLeft - yawRight;

	if ( span < 0 )
		span += 360;

	center = yawRight + span * 0.5;

	if ( center >= 360 )
		center -= 360;
}

//-----------------------------------------------------------------------------
// AI_MoveSuggestion_t inline methods
//-----------------------------------------------------------------------------

inline void AI_MoveSuggestion_t::Set( AI_MoveSuggType_t newType, float newWeight, float newDir, float newSpan, CBaseEntity *pEntity )
{
	type		    = newType;
	weight          = newWeight;
	hObstacleEntity = pEntity;
	flags           = 0;

	arc.Set( newDir, newSpan );
}

//-------------------------------------

inline AI_MoveSuggestion_t::AI_MoveSuggestion_t()
 :	type( AIMS_INVALID ),
  	weight( 0 ),
  	flags( 0 )
{
}

//-------------------------------------

inline AI_MoveSuggestion_t::AI_MoveSuggestion_t( AI_MoveSuggType_t newType, float newWeight, float newDir, float newSpan, CBaseEntity *pEntity )
{
	Set( newType, newWeight, newDir, newSpan, pEntity );
}

//-------------------------------------

inline AI_MoveSuggestion_t::AI_MoveSuggestion_t( AI_MoveSuggType_t newType, float newWeight, const AI_Arc_t &arc, CBaseEntity *pEntity  )
{
	Set( newType, newWeight, arc.center, arc.span, pEntity );
}

//-------------------------------------

inline void AI_MoveSuggestion_t::Set( AI_MoveSuggType_t newType, float newWeight, const AI_Arc_t &arc, CBaseEntity *pEntity )
{
	Set( newType, newWeight, arc.center, arc.span, pEntity );
}

//-----------------------------------------------------------------------------
// CAI_MoveSolver inline methods
//-----------------------------------------------------------------------------

inline CAI_MoveSolver::CAI_MoveSolver()
{
	m_Regulations.EnsureCapacity( REGS_RESERVE );
}

//-------------------------------------

inline void CAI_MoveSolver::AddRegulation( const AI_MoveSuggestion_t &suggestion )
{
	m_Regulations.AddToTail( suggestion );
}

//-------------------------------------

inline void CAI_MoveSolver::AddRegulations( const AI_MoveSuggestion_t *pSuggestions, int nSuggestions )
{
	for (int i = 0; i < nSuggestions; ++i)
	{
		m_Regulations.AddToTail( pSuggestions[i] );
	}
}

//-------------------------------------

inline bool CAI_MoveSolver::HaveRegulations() const
{
	return (m_Regulations.Count() > 0);
}

//-------------------------------------

inline void CAI_MoveSolver::ClearRegulations()
{
	m_Regulations.RemoveAll();
}

//-------------------------------------

inline bool CAI_MoveSolver::Solve( const AI_MoveSuggestion_t &suggestion, AI_MoveSolution_t *pResult)
{
	return Solve( &suggestion, 1, pResult);
}

//=============================================================================

#endif // AI_MOVESOLVER_H
