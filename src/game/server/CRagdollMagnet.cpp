//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "game.h"
#include "CRagdollMagnet.h"
#include "cplane.h"

ConVar ai_debug_ragdoll_magnets( "ai_debug_ragdoll_magnets", "0");

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( phys_ragdollmagnet, CRagdollMagnet );
BEGIN_DATADESC( CRagdollMagnet )
	DEFINE_KEYFIELD( m_radius,		FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( m_force,		FIELD_FLOAT, "force" ),
	DEFINE_KEYFIELD( m_axis,		FIELD_VECTOR, "axis" ),
	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CRagdollMagnet::InputEnable( inputdata_t &inputdata )
{
	Enable( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CRagdollMagnet::InputDisable( inputdata_t &inputdata )
{
	Enable( false );
}

//-----------------------------------------------------------------------------
// Purpose: Find the ragdoll magnet entity that should pull this entity's ragdoll
// Input  : *pNPC - the npc that's dying
// Output : CRagdollMagnet - the magnet that's best to use.
//
// NOTES:
//
// The nearest ragdoll magnet pulls me in IF:
//	- Present
//	- I'm within the magnet's RADIUS
//	- LATER: There is clear line of sight from my center to the magnet
//  - LATER: I'm not flagged to ignore ragdoll magnets
//	- LATER: The magnet is not turned OFF
//-----------------------------------------------------------------------------
CRagdollMagnet *CRagdollMagnet::FindBestMagnet( CBaseEntity *pNPC )
{
	CRagdollMagnet	*pMagnet = NULL;
	CRagdollMagnet	*pBestMagnet;

	float			flClosestDist;

	// Assume we won't find one.
	pBestMagnet = NULL;
	flClosestDist = FLT_MAX;
	
	do
	{
		pMagnet = (CRagdollMagnet *)gEntList.FindEntityByClassname( pMagnet, "phys_ragdollmagnet" );

		if( pMagnet && pMagnet->IsEnabled() )
		{
			if( pMagnet->m_target != NULL_STRING )
			{
				// if this magnet has a target, only affect that target!
				if( pNPC->GetEntityName() == pMagnet->m_target )
				{
					return pMagnet;
				}
				else
				{
					continue;
				}
			}

			float flDist;
			flDist = pMagnet->DistToPoint( pNPC->WorldSpaceCenter() );

			if( flDist < flClosestDist && flDist <= pMagnet->GetRadius() )
			{
				// This is the closest magnet that can pull this npc.
				flClosestDist = flDist;
				pBestMagnet = pMagnet;
			}
		}

	} while( pMagnet );

	return pBestMagnet;
}

//-----------------------------------------------------------------------------
// Purpose: Get the force that we should add to this NPC's ragdoll.
// Input  : *pNPC - 
// Output : Vector
//
// NOTE: This function assumes pNPC is within this magnet's radius.
//-----------------------------------------------------------------------------
Vector CRagdollMagnet::GetForceVector( CBaseEntity *pNPC )
{
	Vector vecForceToApply;

	if( IsBarMagnet() )
	{
		CPlane axis;
		Vector vecForceDir;
		Vector vecClosest;

		CalcClosestPointOnLineSegment( pNPC->WorldSpaceCenter(), GetAbsOrigin(), m_axis, vecClosest, NULL );

		vecForceDir = (vecClosest - pNPC->WorldSpaceCenter() );
		VectorNormalize( vecForceDir );

		vecForceToApply = vecForceDir * m_force;
	}
	else
	{
		Vector vecForce;

		vecForce = GetAbsOrigin() - pNPC->WorldSpaceCenter();
		VectorNormalize( vecForce );

		vecForceToApply = vecForce * m_force;
	}

	if( ai_debug_ragdoll_magnets.GetBool() )
	{
		IPhysicsObject *pPhysObject;

		pPhysObject = pNPC->VPhysicsGetObject();

		if( pPhysObject )
		{
			Msg("Ragdoll magnet adding %f inches/sec to %s\n", m_force/pPhysObject->GetMass(), pNPC->GetClassname() );
		}
	}

	return vecForceToApply;
}

//-----------------------------------------------------------------------------
// Purpose: How far away is this point? This is different for point and bar magnets
// Input  : &vecPoint - the point
// Output : float - the dist
//-----------------------------------------------------------------------------
float CRagdollMagnet::DistToPoint( const Vector &vecPoint )
{
	if( IsBarMagnet() )
	{
		// I'm a bar magnet, so the point's distance is really the plane constant.
		// A bar magnet is a cylinder who's length is AbsOrigin() to m_axis, and whose
		// diameter is m_radius.

		// first we build two planes. The TOP and BOTTOM planes.
		// the idea is that vecPoint must be on the right side of both
		// planes to be affected by this particular magnet.
		// TOP and BOTTOM planes can be visualized as the 'caps' of the cylinder
		// that describes the bar magnet, and they point towards each other.
		// We're making sure vecPoint is between the caps.
		Vector vecAxis;

		vecAxis = GetAxisVector();
		VectorNormalize( vecAxis );

		CPlane top, bottom;

		bottom.InitializePlane( -vecAxis, m_axis );
		top.InitializePlane( vecAxis, GetAbsOrigin() );

		if( top.PointInFront( vecPoint ) && bottom.PointInFront( vecPoint ) )
		{
			// This point is between the two caps, so calculate the distance
			// of vecPoint from the axis of the bar magnet
			CPlane axis;
			Vector vecUp;
			Vector vecRight;

			// Horizontal and Vertical distances.
			float hDist, vDist;

			// Need to get a vector that's right-hand to m_axis
			VectorVectors( vecAxis, vecRight, vecUp );
			
			//CrossProduct( vecAxis, vecUp, vecRight );
			//VectorNormalize( vecRight );
			//VectorNormalize( vecUp );

			// Set up the plane to measure horizontal dist.
			axis.InitializePlane( vecRight, GetAbsOrigin() );
			hDist = fabs( axis.PointDist( vecPoint ) );

			axis.InitializePlane( vecUp, GetAbsOrigin() );
			vDist = fabs( axis.PointDist( vecPoint ) );

			return MAX( hDist, vDist );
		}
		else
		{
			return FLT_MAX;
		}
	}
	else
	{
		// I'm a point magnet. Just return dist
		return ( GetAbsOrigin() - vecPoint ).Length();
	}
}
