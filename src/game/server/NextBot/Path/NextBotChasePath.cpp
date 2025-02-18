//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#include "NextBotChasePath.h"
#include "tier1/fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//----------------------------------------------------------------------------------------------
/**
 * Try to cutoff our chase subject
 */
Vector ChasePath::PredictSubjectPosition( INextBot *bot, CBaseEntity *subject ) const
{
	ILocomotion *mover = bot->GetLocomotionInterface();

	const Vector &subjectPos = subject->GetAbsOrigin();

	Vector to = subjectPos - bot->GetPosition();
	to.z = 0.0f;
	float flRangeSq = to.LengthSqr();

	// don't lead if subject is very far away
	float flLeadRadiusSq = GetLeadRadius();
	flLeadRadiusSq *= flLeadRadiusSq;
	if ( flRangeSq > flLeadRadiusSq )
		return subjectPos;

	// Normalize in place
	float range = sqrt( flRangeSq );
	to /= ( range + 0.0001f );	// avoid divide by zero

	// estimate time to reach subject, assuming maximum speed
	float leadTime = 0.5f + ( range / ( mover->GetRunSpeed() + 0.0001f ) );
	
	// estimate amount to lead the subject	
	Vector lead = leadTime * subject->GetAbsVelocity();
	lead.z = 0.0f;

	if ( DotProduct( to, lead ) < 0.0f )
	{
		// the subject is moving towards us - only pay attention 
		// to his perpendicular velocity for leading
		Vector2D to2D = to.AsVector2D();
		to2D.NormalizeInPlace();

		Vector2D perp( -to2D.y, to2D.x );

		float enemyGroundSpeed = lead.x * perp.x + lead.y * perp.y;

		lead.x = enemyGroundSpeed * perp.x;
		lead.y = enemyGroundSpeed * perp.y;
	}

	// compute our desired destination
	Vector pathTarget = subjectPos + lead;

	// validate this destination

	// don't lead through walls
	if ( lead.LengthSqr() > 36.0f )
	{
		float fraction;
		if ( !mover->IsPotentiallyTraversable( subjectPos, pathTarget, ILocomotion::IMMEDIATELY, &fraction ) )
		{
			// tried to lead through an unwalkable area - clip to walkable space
			pathTarget = subjectPos + fraction * ( pathTarget - subjectPos );
		}
	}

	// don't lead over cliffs
	CNavArea *leadArea = NULL;

#ifdef NEED_GPGLOBALS_SERVERCOUNT_TO_DO_THIS
	CBaseCombatCharacter *pBCC = subject->MyCombatCharacterPointer();
	if ( pBCC && CloseEnough( pathTarget, subjectPos, 3.0 ) )
	{
		pathTarget = subjectPos;
		leadArea = pBCC->GetLastKnownArea(); // can return null?
	}
	else
	{
		struct CacheEntry_t
		{
			CacheEntry_t() : pArea(NULL) {}
			Vector target;
			CNavArea *pArea;
		};

		static int iServer;
		static CacheEntry_t cache[4];
		static int iNext;
		int i;

		bool bFound = false;
		if ( iServer != gpGlobals->serverCount )
		{
			for ( i = 0; i < ARRAYSIZE(cache); i++ )
			{
				cache[i].pArea = NULL;
			}
			iServer = gpGlobals->serverCount;
		}
		else
		{
			for ( i = 0; i < ARRAYSIZE(cache); i++ )
			{
				if ( cache[i].pArea && CloseEnough( cache[i].target, pathTarget, 2.0 ) )
				{
					pathTarget = cache[i].target;
					leadArea = cache[i].pArea;
					bFound = true;
					break;
				}
			}
		}

		if ( !bFound )
		{
			leadArea = TheNavMesh->GetNearestNavArea( pathTarget );
			if ( leadArea )
			{
				cache[iNext].target = pathTarget;
				cache[iNext].pArea = leadArea;
				iNext = ( iNext + 1 ) % ARRAYSIZE( cache );
			}
		}
	}
#else
	leadArea = TheNavMesh->GetNearestNavArea( pathTarget );
#endif


	if ( !leadArea || leadArea->GetZ( pathTarget.x, pathTarget.y ) < pathTarget.z - mover->GetMaxJumpHeight() )
	{
		// would fall off a cliff
		return subjectPos;		
	}
	
	/** This needs more thought - it is preventing bots from using dropdowns
	if ( mover->HasPotentialGap( subjectPos, pathTarget, &fraction ) )
	{
		// tried to lead over a cliff - clip to safe region
		pathTarget = subjectPos + fraction * ( pathTarget - subjectPos );
	}
	*/
	
	return pathTarget;
}

// if the victim is a player, poke them so they know they're being chased
void DirectChasePath::NotifyVictim( INextBot *me, CBaseEntity *victim )
{
	CBaseCombatCharacter *pBCCVictim = ToBaseCombatCharacter( victim );
	if ( !pBCCVictim )
		return;
	
	pBCCVictim->OnPursuedBy( me );
}