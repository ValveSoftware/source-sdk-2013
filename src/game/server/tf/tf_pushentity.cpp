//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "pushentity.h"
#include "tf_player.h"
#include "collisionutils.h"
#include "tf_gamerules.h"
#include "func_respawnroom.h"
//#include "mathlib/mathlib.h"

class CTFPhysicsPushEntities : public CPhysicsPushedEntities
{
public:

	DECLARE_CLASS( CTFPhysicsPushEntities, CPhysicsPushedEntities );

	// Constructor/Destructor.
	CTFPhysicsPushEntities();
	~CTFPhysicsPushEntities();

protected:

	// Speculatively checks to see if all entities in this list can be pushed
	virtual bool SpeculativelyCheckRotPush( const RotatingPushMove_t &rotPushMove, CBaseEntity *pRoot );
	virtual bool SpeculativelyCheckLinearPush( const Vector &vecAbsPush );
	virtual void FinishRotPushedEntity( CBaseEntity *pPushedEntity, const RotatingPushMove_t &rotPushMove );
	
private:

	bool RotationPushTFPlayer( PhysicsPushedInfo_t &info, const Vector &vecAbsPush, const RotatingPushMove_t &rotPushMove, bool bRotationalPush );
	bool RotationCheckPush( PhysicsPushedInfo_t &info );
	bool LinearPushTFPlayer( PhysicsPushedInfo_t &info, const Vector &vecAbsPush, bool bRotationalPush );
	bool LinearCheckPush( PhysicsPushedInfo_t &info );

	bool IsPlayerAABBIntersetingPusherOBB( CBaseEntity *pEntity, CBaseEntity *pRootEntity );

	void MovePlayer( CBaseEntity *pBlocker, PhysicsPushedInfo_t &info, float flMoveScale, bool bPusherIsTrain );
	void FindNewPushDirection( Vector &vecCurrent, Vector &vecNormal, Vector &vecOutput );

	float m_flPushDist;
	Vector	m_vecPushVector;
};

CTFPhysicsPushEntities s_TFPushedEntities;
CPhysicsPushedEntities *g_pPushedEntities = &s_TFPushedEntities;

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFPhysicsPushEntities::CTFPhysicsPushEntities()
{

}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFPhysicsPushEntities::~CTFPhysicsPushEntities()
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPhysicsPushEntities::SpeculativelyCheckRotPush( const RotatingPushMove_t &rotPushMove, CBaseEntity *pRoot )
{
	// Only do this for "payload" or "escort" maps.
	if ( !TFGameRules()->GameModeUsesEscortPushLogic() )
		return BaseClass::SpeculativelyCheckRotPush( rotPushMove, pRoot );

	Vector vecAbsPush( 0.0f, 0.0f, 0.0f );
	m_nBlocker = -1;
	int nMovedCount = m_rgMoved.Count();
	for ( int i = ( nMovedCount - 1 ); i >= 0; --i )
	{
		// Is the entity and TF Player?
		CTFPlayer *pTFPlayer = NULL;
		if ( m_rgMoved[i].m_pEntity && m_rgMoved[i].m_pEntity->IsPlayer() )
		{
			pTFPlayer = ToTFPlayer( m_rgMoved[i].m_pEntity );
		}

		// Special code to move the player away from the func_train.
		if ( pTFPlayer )
		{
			// Rotationally push the player!
			RotationPushTFPlayer( m_rgMoved[i], vecAbsPush, rotPushMove, true );
		}
		else
		{
			ComputeRotationalPushDirection( m_rgMoved[i].m_pEntity, rotPushMove, &vecAbsPush, pRoot );
			if (!SpeculativelyCheckPush( m_rgMoved[i], vecAbsPush, true ))
			{
				m_nBlocker = i;
				return false;
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Speculatively checks to see if all entities in this list can be pushed
//-----------------------------------------------------------------------------
bool CTFPhysicsPushEntities::SpeculativelyCheckLinearPush( const Vector &vecAbsPush )
{
	// Only do this for "payload" or "escort" maps.
	if ( !TFGameRules()->GameModeUsesEscortPushLogic() )
		return BaseClass::SpeculativelyCheckLinearPush( vecAbsPush );

	m_nBlocker = -1;
	int nMovedCount = m_rgMoved.Count();
	for ( int i = ( nMovedCount - 1 ); i >= 0; --i )
	{
		// Is the entity and TF Player?
		CTFPlayer *pTFPlayer = NULL;
		if ( m_rgMoved[i].m_pEntity && m_rgMoved[i].m_pEntity->IsPlayer() )
		{
			pTFPlayer = ToTFPlayer( m_rgMoved[i].m_pEntity );
		}

		// Special code to move the player away from the func_train.
		if ( pTFPlayer )
		{
			// Linearly push the player!
			LinearPushTFPlayer( m_rgMoved[i], vecAbsPush, false );
		}
		else
		{
			if (!SpeculativelyCheckPush( m_rgMoved[i], vecAbsPush, false ))
			{
				m_nBlocker = i;
				return false;
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPhysicsPushEntities::RotationPushTFPlayer( PhysicsPushedInfo_t &info, const Vector &vecAbsPush, const RotatingPushMove_t &rotPushMove, bool bRotationalPush )
{
	// Clear out the collision entity so that if we early out we don't send bogus collision data to the physics system.
	info.m_Trace.m_pEnt = NULL;

	// Look into doing a full engine->CM_Clear( trace)

	// Get the player.
	CTFPlayer *pPlayer = ToTFPlayer( info.m_pEntity );
	if ( !pPlayer )
		return false;

	info.m_vecStartAbsOrigin = pPlayer->GetAbsOrigin();

	// Get the player collision data.
	CCollisionProperty *pCollisionPlayer = info.m_pEntity->CollisionProp();
	if ( !pCollisionPlayer )
		return false;

	// Find the root object if in hierarchy.
	CBaseEntity *pRootEntity = m_rgPusher[0].m_pEntity->GetRootMoveParent();
	if ( !pRootEntity )
		return false;

	// Get the pusher collision data.
	CCollisionProperty *pCollisionPusher = pRootEntity->CollisionProp();
	if ( !pCollisionPusher )
		return false;

	// Do we have a collision.
	if ( !IsOBBIntersectingOBB( pCollisionPlayer->GetCollisionOrigin(), pCollisionPlayer->GetCollisionAngles(), pCollisionPlayer->OBBMins(), pCollisionPlayer->OBBMaxs(), 
		pCollisionPusher->GetCollisionOrigin(), pCollisionPusher->GetCollisionAngles(), pCollisionPusher->OBBMins(), pCollisionPusher->OBBMaxs(), 
		0.0f ) )
		return false;

	// For speed use spheres to approximate push distance.	
	Vector vecPlayerOrigin = pCollisionPlayer->GetCollisionOrigin();
	float flPlayerRadius = pCollisionPlayer->BoundingRadius();

	Vector vecPusherOrigin = pCollisionPusher->GetCollisionOrigin();
	float flPusherRadius = pCollisionPusher->BoundingRadius();

	Vector vecDeltaOrigin;
	VectorSubtract( vecPlayerOrigin, vecPusherOrigin, vecDeltaOrigin );

	float flRadiusTotal = flPlayerRadius + flPusherRadius;
	float flLength = vecDeltaOrigin.Length();
	float flDistanceDelta = fabs( flRadiusTotal - flLength );

	// Put special code in if we are riding the pusher - only push upward.
	if ( pPlayer->GetGroundEntity() == pRootEntity )
	{
		// Set the push direction and distance.
		m_vecPushVector.Init( 0.0f, 0.0f, 1.0f );
		if ( rotPushMove.amove[0] != 0.0f )
		{
			m_flPushDist = fabs( tan( DEG2RAD( rotPushMove.amove[0] ) ) * flPusherRadius );
			float flPushAdd = m_flPushDist * 0.1f;
			m_flPushDist += flPushAdd;
		}
		else
		{
			m_flPushDist = 0.0f;
		}
	}
	else
	{
		// Set the push direction and distance.
		m_vecPushVector = vecDeltaOrigin;
		m_vecPushVector.NormalizeInPlace();
		m_flPushDist = flDistanceDelta;
		float flPushAdd = m_flPushDist * 0.1f;
		m_flPushDist += flPushAdd;
	}

	return RotationCheckPush( info );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPhysicsPushEntities::RotationCheckPush( PhysicsPushedInfo_t &info )
{
	// Get the blocking and pushing entities.
	CBaseEntity *pBlocker = info.m_pEntity;
	CBaseEntity *pRootEntity = m_rgPusher[0].m_pEntity->GetRootMoveParent();
	if ( !pBlocker || !pRootEntity )
		return true;

	int *pPusherHandles = ( int* )stackalloc( m_rgPusher.Count() * sizeof( int ) );
	UnlinkPusherList( pPusherHandles );
	for ( int iPushTry = 0; iPushTry < 3; ++iPushTry )
	{
		MovePlayer( pBlocker, info, 0.35f, pRootEntity->IsBaseTrain() );
		if ( !IsPlayerAABBIntersetingPusherOBB( pBlocker, pRootEntity ) )
			break;
	}
	RelinkPusherList( pPusherHandles );

	// Is the blocked ground the push entity?
	info.m_bPusherIsGround = false;
	if ( pBlocker->GetGroundEntity() && pBlocker->GetGroundEntity()->GetRootMoveParent() == m_rgPusher[0].m_pEntity )
	{
		info.m_bPusherIsGround = true;
	}

	// Check to see if the player is in a good spot and attempt a move again if not - but only if it isn't being ridden on.
	if ( IsPlayerAABBIntersetingPusherOBB( pBlocker, pRootEntity ) )
	{
		// Try again is the player is still blocked.
//		DevMsg( 1, "Pushing rotation hard!\n" );
		UnlinkPusherList( pPusherHandles );
		MovePlayer( pBlocker, info, 1.0f, pRootEntity->IsBaseTrain() );
		RelinkPusherList( pPusherHandles );
	}

	// The player will never stop a train from moving in TF.
	info.m_bBlocked = false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPhysicsPushEntities::LinearPushTFPlayer( PhysicsPushedInfo_t &info, const Vector &vecAbsPush, bool bRotationalPush )
{
	// Clear out the collision entity so that if we early out we don't send bogus collision data to the physics system.
	info.m_Trace.m_pEnt = NULL;

	// Get the player.
	CTFPlayer *pPlayer = ToTFPlayer( info.m_pEntity );
	if ( !pPlayer )
		return false;

	info.m_vecStartAbsOrigin = pPlayer->GetAbsOrigin();

	// Get the player collision data.
	CCollisionProperty *pCollisionPlayer = info.m_pEntity->CollisionProp();
	if ( !pCollisionPlayer )
		return false;

	// Find the root object if in hierarchy.
	CBaseEntity *pRootEntity = m_rgPusher[0].m_pEntity->GetRootMoveParent();
	if ( !pRootEntity )
		return false;

	// Get the pusher collision data.
	CCollisionProperty *pCollisionPusher = pRootEntity->CollisionProp();
	if ( !pCollisionPusher )
		return false;

	// Do we have a collision.
	if ( !IsOBBIntersectingOBB( pCollisionPlayer->GetCollisionOrigin(), pCollisionPlayer->GetCollisionAngles(), pCollisionPlayer->OBBMins(), pCollisionPlayer->OBBMaxs(), 
		pCollisionPusher->GetCollisionOrigin(), pCollisionPusher->GetCollisionAngles(), pCollisionPusher->OBBMins(), pCollisionPusher->OBBMaxs(), 
		0.0f ) )
		return false;

	if ( pPlayer->GetGroundEntity() == pRootEntity )
	{
		m_vecPushVector = vecAbsPush;
		m_flPushDist = VectorNormalize( m_vecPushVector );
	}
	else
	{
		m_vecPushVector = vecAbsPush;
		m_flPushDist = VectorNormalize( m_vecPushVector );
		m_vecPushVector.z = 0.0f;
		VectorNormalize( m_vecPushVector );
	}

	return LinearCheckPush( info );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPhysicsPushEntities::LinearCheckPush( PhysicsPushedInfo_t &info )
{
	// Get the blocking and pushing entities.
	CBaseEntity *pBlocker = info.m_pEntity;
	CBaseEntity *pRootEntity = m_rgPusher[0].m_pEntity->GetRootMoveParent();
	if ( !pBlocker || !pRootEntity )
		return true;

	// Unlink the pusher from the spatial partition and attempt a player move.
	int *pPusherHandles = ( int* )stackalloc( m_rgPusher.Count() * sizeof( int ) );
	UnlinkPusherList( pPusherHandles );
	MovePlayer( pBlocker, info, 1.0f, pRootEntity->IsBaseTrain() );
	RelinkPusherList( pPusherHandles );

	// Is the pusher the ground entity the blocker is standing on?
	info.m_bPusherIsGround = false;
	if ( pBlocker->GetGroundEntity() && pBlocker->GetGroundEntity()->GetRootMoveParent() == m_rgPusher[0].m_pEntity )
	{
		info.m_bPusherIsGround = true;
	}

	// Check to see if the player is in a good spot and attempt a move again if not - but only if it isn't being ridden on.
	if ( !info.m_bPusherIsGround && !IsPushedPositionValid( pBlocker ) )
	{
		// Try again is the player is still blocked.
//		DevMsg( 1, "Pushing linear hard!\n" );
		UnlinkPusherList( pPusherHandles );
		MovePlayer( pBlocker, info, 1.0f, pRootEntity->IsBaseTrain() );
		RelinkPusherList( pPusherHandles );
	}

	// The player will never stop a train from moving in TF.
	info.m_bBlocked = false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPhysicsPushEntities::IsPlayerAABBIntersetingPusherOBB( CBaseEntity *pEntity, CBaseEntity *pRootEntity )
{
	// Get the player.
	CTFPlayer *pPlayer = ToTFPlayer( pEntity );
	if ( !pPlayer )
		return false;

	// Get the player collision data.
	CCollisionProperty *pCollisionPlayer = pEntity->CollisionProp();
	if ( !pCollisionPlayer )
		return false;

	// Get the pusher collision data.
	CCollisionProperty *pCollisionPusher = pRootEntity->CollisionProp();
	if ( !pCollisionPusher )
		return false;

	// Do we have a collision.
	 return IsOBBIntersectingOBB( pCollisionPlayer->GetCollisionOrigin(), pCollisionPlayer->GetCollisionAngles(), pCollisionPlayer->OBBMins(), pCollisionPlayer->OBBMaxs(), 
		pCollisionPusher->GetCollisionOrigin(), pCollisionPusher->GetCollisionAngles(), pCollisionPusher->OBBMins(), pCollisionPusher->OBBMaxs(), 
		0.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPhysicsPushEntities::FindNewPushDirection( Vector &vecCurrent, Vector &vecNormal, Vector &vecOutput )
{
	// Determine how far along plane to slide based on incoming direction.
	float flBackOff = DotProduct( vecCurrent, vecNormal );

	for ( int iAxis = 0; iAxis < 3; ++iAxis )
	{
		float flDelta = vecNormal[iAxis] * flBackOff;
		vecOutput[iAxis] = vecCurrent[iAxis] - flDelta; 
	}

	// iterate once to make sure we aren't still moving through the plane
	float flAdjust = DotProduct( vecOutput, vecNormal );
	if( flAdjust < 0.0f )
	{
		vecOutput -= ( vecNormal * flAdjust );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPhysicsPushEntities::MovePlayer( CBaseEntity *pBlocker, PhysicsPushedInfo_t &info, float flMoveScale, bool bPusherIsTrain )
{
	// Find out how far we still need to move.
	float flFractionLeft = 1.0f;
	float flNewDist = m_flPushDist *flMoveScale;
	Vector vecPush = m_vecPushVector;

	// Find a new push vector.
	Vector vecStart = pBlocker->GetAbsOrigin();
	vecStart.z += 4.0f;
	for ( int iTest = 0; iTest < 4; ++iTest )
	{
		// Clear the trace entity.
		Vector vecEnd = pBlocker->GetAbsOrigin() + ( flNewDist * vecPush );
		UTIL_TraceEntity( pBlocker, vecStart, vecEnd, MASK_PLAYERSOLID, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &info.m_Trace );

		// we don't want trains pushing enemy players through a respawn room visualizer
		if ( bPusherIsTrain && pBlocker->IsPlayer() )
		{
			if ( PointsCrossRespawnRoomVisualizer( vecStart, info.m_Trace.endpos, pBlocker->GetTeamNumber() ) )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( pBlocker );
				if ( pTFPlayer )
				{
					pTFPlayer->CommitSuicide( true, true );
					return;
				}
			}
		}

		if ( info.m_Trace.fraction > 0.0f )
		{
			pBlocker->SetAbsOrigin( info.m_Trace.endpos );
		}

		if ( info.m_Trace.fraction == 1.0f || !info.m_Trace.m_pEnt )
			break;

		// New test distance and position.
		flFractionLeft = 1.0f - info.m_Trace.fraction;
		flNewDist = flFractionLeft * flNewDist;
		flNewDist = flNewDist * ( 1.0f + ( 1.0f - fabs( info.m_Trace.plane.normal.Dot( vecPush ) ) ) );

		// Find the new push direction.
		Vector vecTmp;
		FindNewPushDirection( vecPush, info.m_Trace.plane.normal, vecTmp );
		VectorCopy( vecTmp, vecPush );
	}
}

//-----------------------------------------------------------------------------
// Causes all entities in the list to touch triggers from their prev position
//-----------------------------------------------------------------------------
void CTFPhysicsPushEntities::FinishRotPushedEntity( CBaseEntity *pPushedEntity, const RotatingPushMove_t &rotPushMove )
{
	// Only do this for "payload" or "escort" maps.
	if ( !TFGameRules()->GameModeUsesEscortPushLogic() )
		return BaseClass::FinishRotPushedEntity( pPushedEntity, rotPushMove );

	if ( !pPushedEntity->IsPlayer() )
	{
		QAngle angles = pPushedEntity->GetAbsAngles();

		// only rotate YAW with pushing.  Freely rotateable entities should either use VPHYSICS
		// or be set up as children
		angles.y += rotPushMove.amove.y;
		pPushedEntity->SetAbsAngles( angles );
	}
}
