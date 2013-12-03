// SharedFunctorUtils.h
// Useful functors for client and server
//========= Copyright Valve Corporation, All rights reserved. ============//

//--------------------------------------------------------------------------------------------------------
/**
* NOTE: The functors in this file should ideally be game-independant, 
* and work for any Source based game
*/
//--------------------------------------------------------------------------------------------------------
#ifndef _SHARED_FUNCTOR_UTILS_H_
#define _SHARED_FUNCTOR_UTILS_H_

#include "debugoverlay_shared.h"
#include "vprof.h"

//--------------------------------------------------------------------------------------------------------
/**
 * Finds visible player on given team that we are pointing at.
 * "team" can be TEAM_ANY 
 * Use with ForEachPlayer()
 */
template < class PlayerType >
class TargetScan
{
public:
	TargetScan( PlayerType *me, int team, float aimTolerance = 0.01f, float maxRange = 2000.0f, float closestPointTestDistance = 50.0f, bool debug = false )
	{
		m_me = me;
		AngleVectors( m_me->EyeAngles(), &m_viewForward );
		m_team = team;
		m_closeDot = 1.0f - aimTolerance;
		m_bestDot = m_closeDot;
		m_maxRange = maxRange;
		m_target = NULL;
		m_closestPointTestDistance = closestPointTestDistance;
		m_debug = debug;
	}


	virtual bool operator() ( PlayerType *them )
	{
		VPROF( "TargetScan()" );
		if ( them != m_me && 
			 them->IsAlive() && 
			 (m_team == TEAM_ANY || them->GetTeamNumber() == m_team) &&
			 IsPotentialTarget( them ) )
		{
			// move the start point out for determining closestPos, to help with close-in checks (healing, etc)
			Vector closestPos;
			Vector start = m_me->EyePosition();
			Vector end = start + m_viewForward * m_closestPointTestDistance;
			Vector testPos;
			CalcClosestPointOnLineSegment( them->WorldSpaceCenter(), start, end, testPos );

			start = them->GetAbsOrigin();
			end = start;
			end.z += them->CollisionProp()->OBBMaxs().z;
			CalcClosestPointOnLineSegment( testPos, start, end, closestPos );
			if ( m_debug )
			{
				NDebugOverlay::Cross3D( closestPos, 1, 255, 255, 255, true, -1.0f );
				NDebugOverlay::Line( end, start, 255, 0, 0, true, -1.0f );
			}

			Vector to = closestPos - m_me->EyePosition();
			to.NormalizeInPlace();

			Vector meRangePoint, themRangePoint;
			m_me->CollisionProp()->CalcNearestPoint( closestPos, &meRangePoint );
			them->CollisionProp()->CalcNearestPoint( meRangePoint, &themRangePoint );
			float range = meRangePoint.DistTo( themRangePoint );

			if ( range > m_maxRange )
			{
				// too far away
				return true;
			}

			float dot = ViewDot( to );
			if ( dot > m_closeDot )
			{
				// target is within angle cone, check visibility
				if ( IsTargetVisible( them ) )
				{
					if ( dot >= m_bestDot )
					{
						m_target = them;
						m_bestDot = dot;
					}

					m_allTargets.AddToTail( them );
				}
			}
		}

		return true;
	}

	PlayerType *GetTarget( void ) const
	{
		return m_target;
	}

	const CUtlVector< PlayerType * > &GetAllTargets( void ) const
	{
		return m_allTargets;
	}

	float GetTargetDot( void ) const
	{
		return m_bestDot;
	}

protected:
	/**
	 * Is the point in our FOV?
	 */
	virtual float ViewDot( const Vector &dir ) const
	{
		return DotProduct( m_viewForward, dir );
	}

	/**
	 * Is the given actor a visible target?
	 */
	virtual bool IsTargetVisible( PlayerType *them ) const
	{
		// The default check is a straight-up IsAbleToSee
		return m_me->IsAbleToSee( them, CBaseCombatCharacter::DISREGARD_FOV ); // already have a dot product checking FOV
	}

	/**
	 * Is the given player a possible target at all?
	 */
	virtual bool IsPotentialTarget( PlayerType *them ) const
	{
		return true;
	}

	PlayerType *m_me;	
	Vector m_viewForward;
	int m_team;

	float m_closeDot;
	float m_bestDot;
	float m_maxRange;
	float m_closestPointTestDistance;
	bool m_debug;

	PlayerType *m_target;
	CUtlVector< PlayerType * > m_allTargets;
};


#endif
