//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "ai_movesolver.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

inline float V_round( float f )
{
	return (float)( (int)( f + 0.5 ) );
}

//-----------------------------------------------------------------------------
// CAI_MoveSolver
//-----------------------------------------------------------------------------

// The epsilon used by the solver
const float AIMS_EPS = 0.01;


//-----------------------------------------------------------------------------
// Visualization
//-----------------------------------------------------------------------------
void CAI_MoveSolver::VisualizeRegulations( const Vector& origin )
{
	if ( m_Regulations.Count() )
	{
		CAI_MoveSuggestions regulations;
		regulations.AddVectorToTail( m_Regulations );
		NormalizeSuggestions( &regulations[0], (&regulations[0]) + regulations.Count() );

		Vector side1, mid, side2;
		for (int i = regulations.Count(); --i >= 0; )
		{
			// Compute the positions of the angles...
			float flMinAngle = regulations[i].arc.center - regulations[i].arc.span * 0.5f;
			float flMaxAngle = regulations[i].arc.center + regulations[i].arc.span * 0.5f;

			side1 = UTIL_YawToVector( flMinAngle );
			side2 = UTIL_YawToVector( flMaxAngle );
			mid = UTIL_YawToVector( regulations[i].arc.center );

			// Stronger weighted ones are bigger
			if ( regulations[i].weight < 0 )
			{
				float flLength = 10 + 40 * ( regulations[i].weight * -1.0);
				side1 *= flLength;
				side2 *= flLength;
				mid *= flLength;

				side1 += origin;
				side2 += origin;
				mid += origin;

				NDebugOverlay::Triangle(origin,	mid, side1, 255, 0, 0, 48, true, 0.1f );
				NDebugOverlay::Triangle(origin,	side2, mid, 255, 0, 0, 48, true, 0.1f );
			}
		}
	}
}


//-------------------------------------
// Purpose: The actual solver function. Reweights according to type and sums
//			all the suggestions, identifying the best.
//-------------------------------------
bool CAI_MoveSolver::Solve( const AI_MoveSuggestion_t *pSuggestions, int nSuggestions, AI_MoveSolution_t *pResult)
{
	//---------------------------------
	//
	// Quick out
	//
	if ( !nSuggestions )
		return false;

	if ( nSuggestions == 1 && m_Regulations.Count() == 0 && pSuggestions->type == AIMST_MOVE )
	{
		pResult->dir = pSuggestions->arc.center;
		return true;
	}

	//---------------------------------
	//
	// Setup
	//
	CAI_MoveSuggestions suggestions;

	suggestions.EnsureCapacity( m_Regulations.Count() + nSuggestions );

	suggestions.CopyArray( pSuggestions, nSuggestions);
	suggestions.AddVectorToTail( m_Regulations );

	// Initialize the solver
	const int NUM_SOLUTIONS	= 120;
	const int SOLUTION_ANG	= 360 / NUM_SOLUTIONS;

	COMPILE_TIME_ASSERT( ( 360 % NUM_SOLUTIONS ) == 0 );

	struct Solution_t
	{
		// The sum bias
		float					   bias;
		float					   highBias;
		AI_MoveSuggestion_t *pHighSuggestion;
	};

	Solution_t 	solutions[NUM_SOLUTIONS]	= { 0 };

	//---------------------------------

	// The first thing we do is reweight and normalize the weights into a range of [-1..1], where
	// a negative weight is a repulsion. This becomes a bias for the solver.
	// @TODO (toml 06-18-02): this can be made sligtly more optimal by precalculating regulation adjusted weights
	Assert( suggestions.Count() >= 1 );
	NormalizeSuggestions( &suggestions[0], (&suggestions[0]) + suggestions.Count() );

	//
	// Add the biased suggestions to the solutions
	//
	for ( int iSuggestion = 0; iSuggestion < suggestions.Count(); ++iSuggestion )
	{
		AI_MoveSuggestion_t &current = suggestions[iSuggestion];

		// Convert arc values to solution indices relative to right post. Right is angle down, left is angle up.
		float halfSpan	= current.arc.span * 0.5;
		int   center 	= V_round( ( halfSpan * NUM_SOLUTIONS ) / 360 );
		int   left		= ( current.arc.span * NUM_SOLUTIONS ) / 360;

		float angRight   = current.arc.center - halfSpan;

		if (angRight < 0.0)
			angRight += 360;

		int base = ( angRight * NUM_SOLUTIONS ) / 360;

		// Sweep from left to right, summing the bias. For positive suggestions,
		// the bias is further weighted to favor the center of the arc.
		const float positiveDegradePer180 = 0.05; // i.e., lose 5% of weight by the time hit 180 degrees off center
		const float positiveDegrade       = ( positiveDegradePer180 / ( NUM_SOLUTIONS * 0.5 ) ); 

		for ( int i = 0; i < left + 1; ++i )
		{
			float bias = 0.0;

			if ( current.weight > 0)
			{
				int	iOffset = center - i;
				float degrade = abs( iOffset ) * positiveDegrade;

				if ( ( (current.flags & AIMS_FAVOR_LEFT ) && i > center ) || 
					 ( (current.flags & AIMS_FAVOR_RIGHT) && i < center ) )
				{
					degrade *= 0.9;
				}

				bias = current.weight - ( current.weight * degrade );
			}
			else
				bias = current.weight;

			int iCurSolution = (base + i) % NUM_SOLUTIONS;

			solutions[iCurSolution].bias += bias;
			if ( bias > solutions[iCurSolution].highBias )
			{
				solutions[iCurSolution].highBias        = bias;
				solutions[iCurSolution].pHighSuggestion = &current;
			}
		}
	}

	//
	// Find the best solution
	//
	int   best     = -1;
	float biasBest = 0;

	for ( int i = 0; i < NUM_SOLUTIONS; ++i )
	{
		if ( solutions[i].bias > biasBest )
		{
			best     = i;
			biasBest = solutions[i].bias;
		}
	}

	if ( best == -1 )
		return false; // no solution

	//
	// Construct the results
	//
	float result = best * SOLUTION_ANG;

	// If the matching suggestion is within the solution, use that as the result,
	// as it is valid and more precise.
	const float suggestionCenter = solutions[best].pHighSuggestion->arc.center;

	if ( suggestionCenter > result && suggestionCenter <= result + SOLUTION_ANG )
		result = suggestionCenter;

	pResult->dir = result;

	return true;
}

//-------------------------------------
// Purpose: Adjusts the suggestion weights according to the type of the suggestion,
//			apply the appropriate sign, ensure values are in expected ranges
//-------------------------------------

struct AI_MoveSuggWeights
{
	float min;
	float max;
};

static AI_MoveSuggWeights g_AI_MoveSuggWeights[] = // @TODO (toml 06-18-02): these numbers need tuning
{
	{  0.20,  1.00 },	// AIMST_MOVE
	{ -0.00, -0.25 },	// AIMST_AVOID_DANGER
	{ -0.00, -0.25 },	// AIMST_AVOID_OBJECT
	{ -0.00, -0.25 },	// AIMST_AVOID_NPC
	{ -0.00, -0.25 },	// AIMST_AVOID_WORLD
	{ -1.00, -1.00 },	// AIMST_NO_KNOWLEDGE
	{ -0.60, -0.60 },	// AIMST_OSCILLATION_DETERRANCE
	{  0.00,  0.00 },	// AIMST_INVALID
};

void CAI_MoveSolver::NormalizeSuggestions( AI_MoveSuggestion_t *pBegin, AI_MoveSuggestion_t *pEnd )
{
	while ( pBegin != pEnd )
	{
		const float min = g_AI_MoveSuggWeights[pBegin->type].min;
		const float max = g_AI_MoveSuggWeights[pBegin->type].max;

		Assert( pBegin->weight >= -AIMS_EPS && pBegin->weight <= 1.0 + AIMS_EPS );

		if ( pBegin->weight < AIMS_EPS ) // zero normalizes to zero
			pBegin->weight = 0.0;
		else
			pBegin->weight = ( ( max - min ) * pBegin->weight ) + min;

		while (pBegin->arc.center < 0)
			pBegin->arc.center += 360;

		while (pBegin->arc.center >= 360)
			pBegin->arc.center -= 360;

		++pBegin;
	}
}

//-------------------------------------

bool CAI_MoveSolver::HaveRegulationForObstacle( CBaseEntity *pEntity)
{
   for ( int i = 0; i < m_Regulations.Count(); ++i )
   {
      if ( m_Regulations[i].hObstacleEntity != NULL &&
           pEntity == m_Regulations[i].hObstacleEntity.Get() )
      {
         return true;
      }
   }
   return false;
}

//-----------------------------------------------------------------------------
//
// Commands and tests
//

#ifdef DEBUG
CON_COMMAND(ai_test_move_solver, "Tests the AI move solver system")
{
#ifdef DEBUG
	const float EPS = 0.001;
#endif
	DevMsg( "Beginning move solver tests...\n" );

	CAI_MoveSolver 		solver;
	AI_MoveSolution_t 	solution;
	int					i;

	//
	// Value in, no regulations, should yield value out
	//
	{
	DevMsg( "Simple... " );

	for (i = 0; i < 360; ++i)
	{
		Assert( solver.Solve( AI_MoveSuggestion_t( AIMST_MOVE, 1, i, 180 ), &solution ) );
		Assert( solution.dir == (float)i );

	}

	DevMsg( "pass.\n" );
	solver.ClearRegulations();
	}

	//
	// Two values in, should yield the first
	//
	{
	DevMsg( "Two positive... " );

	AI_MoveSuggestion_t suggestions[2];

	suggestions[0].Set( AIMST_MOVE, 1.0, 180, 100 );
	suggestions[1].Set( AIMST_MOVE, 0.5, 0, 100 );

	Assert( solver.Solve( suggestions, 2, &solution ) );
	Assert( solution.dir == (float)suggestions[0].arc.center );


	DevMsg( "pass.\n" );
	solver.ClearRegulations();
	}

	//
	// Two values in, first regulated, should yield the second
	//
	{
	DevMsg( "Avoid one of two... " );

	AI_MoveSuggestion_t suggestions[2];

	solver.AddRegulation(AI_MoveSuggestion_t( AIMST_AVOID_OBJECT, 1, 260, 60 ) );

	suggestions[0].Set( AIMST_MOVE, 1.0, 270, 45 );
	suggestions[1].Set( AIMST_MOVE, 1.0, 0, 45 );

	Assert( solver.Solve( suggestions, 2, &solution ) );
	Assert( solution.dir == (float)suggestions[1].arc.center );


	DevMsg( "pass.\n" );
	solver.ClearRegulations();
	}

	//
	// No solution
	//
	{
	DevMsg( "No solution... " );

	AI_MoveSuggestion_t suggestions[2];

	suggestions[0].Set( AIMST_MOVE, 1.0, 270, 90 );
	suggestions[1].Set( AIMST_AVOID_OBJECT, 1.0, 260, 180 );

	Assert( !solver.Solve( suggestions, 2, &solution ) );

	DevMsg( "pass.\n" );
	solver.ClearRegulations();
	}

	//
	// Nearest solution, in tolerance
	//
	{
	DevMsg( "Nearest solution, in tolerance... " );

	AI_MoveSuggestion_t suggestions[2];

	suggestions[0].Set( AIMST_MOVE, 1.0, 278, 90 );
	suggestions[1].Set( AIMST_AVOID_OBJECT, 1.0, 260, 24 );

	Assert( solver.Solve( suggestions, 2, &solution ) );
	Assert( solution.dir == (float)suggestions[0].arc.center );

	DevMsg( "pass.\n" );
	solver.ClearRegulations();
	}

	//
	// Nearest solution
	//
	{
	DevMsg( "Nearest solution... " );

	AI_MoveSuggestion_t suggestions[2];

	suggestions[0].Set( AIMST_MOVE, 1.0, 270, 90 );
	suggestions[1].Set( AIMST_AVOID_OBJECT, 1.0, 260, 40 );

	Assert( solver.Solve( suggestions, 2, &solution ) );
	Assert( solution.dir - 282 < EPS ); // given 60 solutions

	DevMsg( "pass.\n" );
	solver.ClearRegulations();
	}

}
#endif

//=============================================================================

