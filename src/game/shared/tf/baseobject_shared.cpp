//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "baseobject_shared.h"
#include <KeyValues.h>
#include "tf_shareddefs.h"
#include "engine/ivmodelinfo.h"

#ifdef GAME_DLL
	#include "func_no_build.h"
	#include "tf_player.h"
	#include "tf_team.h"
	#include "func_no_build.h"
	#include "func_respawnroom.h"
#else
	#include "c_tf_player.h"
	#include "c_tf_team.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_obj_build_rotation_speed( "tf_obj_build_rotation_speed", "250", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Degrees per second to rotate building when player alt-fires during placement." );

//-----------------------------------------------------------------------------
// Purpose: Parse our model and create the buildpoints in it
//-----------------------------------------------------------------------------
void CBaseObject::CreateBuildPoints( void )
{
	// Clear out any existing build points
	m_BuildPoints.RemoveAll();

	KeyValues * modelKeyValues = new KeyValues("");
	if ( !modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		return;
	}

	// Do we have a build point section?
	KeyValues *pkvAllBuildPoints = modelKeyValues->FindKey("build_points");
	if ( pkvAllBuildPoints )
	{
		KeyValues *pkvBuildPoint = pkvAllBuildPoints->GetFirstSubKey();
		while ( pkvBuildPoint )
		{
			// Find the attachment first
			const char *sAttachment = pkvBuildPoint->GetName();
			int iAttachmentNumber = LookupAttachment( sAttachment );
			if ( iAttachmentNumber > 0 )
			{
				AddAndParseBuildPoint( iAttachmentNumber, pkvBuildPoint );
			}
			else
			{
				Msg( "ERROR: Model %s specifies buildpoint %s, but has no attachment named %s.\n", STRING(GetModelName()), pkvBuildPoint->GetString(), pkvBuildPoint->GetString() );
			}

			pkvBuildPoint = pkvBuildPoint->GetNextKey();
		}
	}

	// Any virtual build points (build points that aren't on an attachment)?
	pkvAllBuildPoints = modelKeyValues->FindKey("virtual_build_points");
	if ( pkvAllBuildPoints )
	{
		KeyValues *pkvBuildPoint = pkvAllBuildPoints->GetFirstSubKey();
		while ( pkvBuildPoint )
		{
			AddAndParseBuildPoint( -1, pkvBuildPoint );
			pkvBuildPoint = pkvBuildPoint->GetNextKey();
		}
	}

	modelKeyValues->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::AddAndParseBuildPoint( int iAttachmentNumber, KeyValues *pkvBuildPoint )
{
	int iPoint = AddBuildPoint( iAttachmentNumber );

	
	m_BuildPoints[iPoint].m_bPutInAttachmentSpace = (pkvBuildPoint->GetInt( "PutInAttachmentSpace", 0 ) != 0);

	// Now see if we've got a set of valid objects specified
	KeyValues *pkvValidObjects = pkvBuildPoint->FindKey( "valid_objects" );
	if ( pkvValidObjects )
	{
		KeyValues *pkvObject = pkvValidObjects->GetFirstSubKey();
		while ( pkvObject )
		{
			const char *pSpecifiedObject = pkvObject->GetName();
			int iLenObjName = Q_strlen( pSpecifiedObject );

			// Find the object index for the name
			for ( int i = 0; i < OBJ_LAST; i++ )
			{
				if ( !Q_strncasecmp( GetObjectInfo( i )->m_pClassName, pSpecifiedObject, iLenObjName) )
				{
					AddValidObjectToBuildPoint( iPoint, i );
					break;
				}
			}

			pkvObject = pkvObject->GetNextKey();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add a new buildpoint to my list of buildpoints
//-----------------------------------------------------------------------------
int CBaseObject::AddBuildPoint( int iAttachmentNum )
{
	// Make a new buildpoint
	BuildPoint_t sNewPoint;
	sNewPoint.m_hObject = NULL;
	sNewPoint.m_iAttachmentNum = iAttachmentNum;
	sNewPoint.m_bPutInAttachmentSpace = false;
	Q_memset( sNewPoint.m_bValidObjects, 0, sizeof( sNewPoint.m_bValidObjects ) );

	// Insert it into our list
	return m_BuildPoints.AddToTail( sNewPoint );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::AddValidObjectToBuildPoint( int iPoint, int iObjectType )
{
	Assert( iPoint <= GetNumBuildPoints() );
	m_BuildPoints[iPoint].m_bValidObjects[ iObjectType ] = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseObject::GetNumBuildPoints( void ) const
{
	return m_BuildPoints.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject* CBaseObject::GetBuildPointObject( int iPoint )
{
	Assert( iPoint >= 0 && iPoint <= GetNumBuildPoints() );

	return m_BuildPoints[iPoint].m_hObject;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified object type can be built on this point
//-----------------------------------------------------------------------------
bool CBaseObject::CanBuildObjectOnBuildPoint( int iPoint, int iObjectType )
{
	Assert( iPoint >= 0 && iPoint <= GetNumBuildPoints() );

	// Allowed to build here?
	if ( !m_BuildPoints[iPoint].m_bValidObjects[ iObjectType ] )
		return false;

	// Buildpoint empty?
	return ( m_BuildPoints[iPoint].m_hObject == NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseObject::GetBuildPoint( int iPoint, Vector &vecOrigin, QAngle &vecAngles )
{
	Assert( iPoint >= 0 && iPoint <= GetNumBuildPoints() );

	int iAttachmentNum = m_BuildPoints[iPoint].m_iAttachmentNum;
	if ( iAttachmentNum == -1 )
	{
		vecOrigin = GetAbsOrigin();
		vecAngles = GetAbsAngles();
		return true;
	}
	else
	{
		return GetAttachment( m_BuildPoints[iPoint].m_iAttachmentNum, vecOrigin, vecAngles );
	}
}


int CBaseObject::GetBuildPointAttachmentIndex( int iPoint ) const
{
	Assert( iPoint >= 0 && iPoint <= GetNumBuildPoints() );

	if ( m_BuildPoints[iPoint].m_bPutInAttachmentSpace )
	{
		return m_BuildPoints[iPoint].m_iAttachmentNum;
	}
	else
	{
		return 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseObject::SetObjectOnBuildPoint( int iPoint, CBaseObject *pObject )
{
	Assert( iPoint >= 0 && iPoint <= GetNumBuildPoints() );
	m_BuildPoints[iPoint].m_hObject = pObject;
}

ConVar tf_obj_max_attach_dist( "tf_obj_max_attach_dist", "160", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseObject::GetMaxSnapDistance( int iPoint )
{
	return tf_obj_max_attach_dist.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Return the number of objects on my build points
//-----------------------------------------------------------------------------
int CBaseObject::GetNumObjectsOnMe( void )
{
	int iObjects = 0;
	for ( int i = 0; i < GetNumBuildPoints(); i++ )
	{
		if ( m_BuildPoints[i].m_hObject )
		{
			iObjects++;
		}
	}

	return iObjects;
}


//-----------------------------------------------------------------------------
// I've finished building the specified object on the specified build point
//-----------------------------------------------------------------------------
int CBaseObject::FindObjectOnBuildPoint( CBaseObject *pObject )
{
	for (int i = m_BuildPoints.Count(); --i >= 0; )
	{
		if (m_BuildPoints[i].m_hObject == pObject)
			return i;
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject *CBaseObject::GetObjectOfTypeOnMe( int iObjectType )
{
	for ( int iObject = 0; iObject < GetNumObjectsOnMe(); ++iObject )
	{
		CBaseObject *pObject = dynamic_cast<CBaseObject*>( m_BuildPoints[iObject].m_hObject.Get() );
		if ( pObject )
		{
			if ( pObject->GetType() == iObjectType )
				return pObject;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::RemoveAllObjects( void )
{
#ifndef CLIENT_DLL
	for ( int i = 0; i < GetNumBuildPoints(); i++ )
	{
		if ( m_BuildPoints[i].m_hObject )
		{

			UTIL_Remove( m_BuildPoints[i].m_hObject );
		}
	}
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject	*CBaseObject::GetParentObject( void )
{
	if ( GetMoveParent() )
		return dynamic_cast<CBaseObject*>(GetMoveParent());

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity	*CBaseObject::GetParentEntity( void )
{
	if ( GetMoveParent() )
		return GetMoveParent();

	return NULL;
}

static ConVar sv_ignore_hitboxes( "sv_ignore_hitboxes", "0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Disable hitboxes" );

bool CBaseObject::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	bool bReturn = BaseClass::TestHitboxes( ray, fContentsMask, tr );

	if( !sv_ignore_hitboxes.GetBool() )
		return bReturn;


	if( !bReturn )
	{
		return false;
	}

	if( tr.fraction == 1.f  && !tr.allsolid && !tr.startsolid )
	{
		return false;
	}

	return bReturn;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this object should be active
//-----------------------------------------------------------------------------
bool CBaseObject::ShouldBeActive( void )
{
	if ( IsDisabled() )
		return false;

	// Placing and/or constructing objects shouldn't be active
	if ( IsPlacing() || IsBuilding() || IsCarried() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Set the object's type
//-----------------------------------------------------------------------------
void CBaseObject::SetType( int iObjectType )
{
	m_iObjectType = iObjectType;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : act - 
//-----------------------------------------------------------------------------
void CBaseObject::SetActivity( Activity act ) 
{ 
	// Hrm, it's not actually a studio model...
	if ( !GetModelPtr() )
		return;

	int sequence = SelectWeightedSequence( act ); 
	if ( sequence != ACTIVITY_NOT_AVAILABLE )
	{
		m_Activity = act; 
		SetObjectSequence( sequence );
	}
	else
	{
		m_Activity = ACT_INVALID;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CBaseObject::GetActivity( ) const
{ 
	return m_Activity;
}

//-----------------------------------------------------------------------------
// Purpose: Thin wrapper over CBaseAnimating::SetSequence to do bookkeeping.
// Input  : sequence - 
//-----------------------------------------------------------------------------
void CBaseObject::SetObjectSequence( int sequence )
{
	ResetSequence( sequence );

	SetCycle( GetReversesBuildingConstructionSpeed() != 0.0f ? 1.0f : 0.0f );

#if !defined( CLIENT_DLL )
	if ( IsUsingClientSideAnimation() )
	{
		ResetClientsideFrame();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::OnGoActive( void )
{
#ifndef CLIENT_DLL
	while ( m_nDefaultUpgradeLevel + 1 > m_iUpgradeLevel )
	{
		StartUpgrading();
	}

	// Play startup animation
	PlayStartupAnimation();

	// Switch to the on state
	if ( GetModelPtr() )
	{
		int index = FindBodygroupByName( "powertoggle" );
		if ( index >= 0 )
		{
			SetBodygroup( index, 1 );
		}
	}

	UpdateDisabledState();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::OnGoInactive( void )
{
#ifndef CLIENT_DLL
	if ( GetModelPtr() )
	{
		// Switch to the off state
		int index = FindBodygroupByName( "powertoggle" );
		if ( index >= 0 )
		{
			SetBodygroup( index, 0 );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseObject::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		if ( GetCollisionGroup() == TFCOLLISION_GROUP_OBJECT_SOLIDTOPLAYERMOVEMENT )
		{
			return true;
		}

		switch( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;

		case TF_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;
		}
	}

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

//-----------------------------------------------------------------------------
// Purpose: Should objects repel players on the same team
//-----------------------------------------------------------------------------
bool CBaseObject::ShouldPlayersAvoid( void )
{
	return ( GetCollisionGroup() == TFCOLLISION_GROUP_OBJECT );
}

//-----------------------------------------------------------------------------
// Do we have to be built on an attachment point
//-----------------------------------------------------------------------------
bool CBaseObject::MustBeBuiltOnAttachmentPoint( void ) const
{
	return (m_fObjectFlags & OF_MUST_BE_BUILT_ON_ATTACHMENT) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Find a place in the world where we should try to build this object
//-----------------------------------------------------------------------------
bool CBaseObject::CalculatePlacementPos( void )
{
	CTFPlayer *pPlayer = GetOwner();

	if ( !pPlayer )
		return false;

	// Calculate build angles
	QAngle vecAngles = vec3_angle;
	vecAngles.y = pPlayer->EyeAngles().y;

	QAngle objAngles = vecAngles;

	SetAbsAngles( objAngles );

	UpdateDesiredBuildRotation( gpGlobals->frametime );

	objAngles.y = objAngles.y + m_flCurrentBuildRotation;

	SetLocalAngles( objAngles );
	AngleVectors( vecAngles, &m_vecBuildForward );

	// Adjust build distance based upon object size
	Vector2D vecObjectRadius;
	vecObjectRadius.x = MAX( fabs( m_vecBuildMins.m_Value.x ), fabs( m_vecBuildMaxs.m_Value.x ) );
	vecObjectRadius.y = MAX( fabs( m_vecBuildMins.m_Value.y ), fabs( m_vecBuildMaxs.m_Value.y ) );

	Vector2D vecPlayerRadius;
	Vector vecPlayerMins = pPlayer->WorldAlignMins();
	Vector vecPlayerMaxs = pPlayer->WorldAlignMaxs();
	vecPlayerRadius.x = MAX( fabs( vecPlayerMins.x ), fabs( vecPlayerMaxs.x ) );
	vecPlayerRadius.y = MAX( fabs( vecPlayerMins.y ), fabs( vecPlayerMaxs.y ) );

	m_flBuildDistance = vecObjectRadius.Length() + vecPlayerRadius.Length() + 4; // small safety buffer
	Vector vecBuildOrigin = pPlayer->WorldSpaceCenter() + m_vecBuildForward * m_flBuildDistance;

	m_vecBuildOrigin = vecBuildOrigin;
	Vector vErrorOrigin = vecBuildOrigin - (m_vecBuildMaxs - m_vecBuildMins) * 0.5f - m_vecBuildMins;

	Vector vBuildDims = m_vecBuildMaxs - m_vecBuildMins;
	Vector vHalfBuildDims = vBuildDims * 0.5;
	Vector vHalfBuildDimsXY( vHalfBuildDims.x, vHalfBuildDims.y, 0 );

	// Here, we start at the highest Z we'll allow for the top of the object. Then
	// we sweep an XY cross section downwards until it hits the ground.
	//
	// The rule is that the top of to box can't go lower than the player's feet, and the bottom of the
	// box can't go higher than the player's head.
	//
	// To simplify things in here, we treat the box as though it's symmetrical about all axes
	// (so mins = -maxs), then reoffset the box at the very end.
	Vector vHalfPlayerDims = (pPlayer->WorldAlignMaxs() - pPlayer->WorldAlignMins()) * 0.5f;
	float flBoxTopZ = pPlayer->WorldSpaceCenter().z + vHalfPlayerDims.z + vBuildDims.z;
	float flBoxBottomZ = pPlayer->WorldSpaceCenter().z - vHalfPlayerDims.z - vBuildDims.z;

	// First, find the ground (ie: where the bottom of the box goes).
	trace_t tr;
	float bottomZ = 0;
	int nIterations = 8;
	float topZ = flBoxTopZ;
	float topZInc = (flBoxBottomZ - flBoxTopZ) / (nIterations-1);
	int iIteration;
	for ( iIteration = 0; iIteration < nIterations; iIteration++ )
	{
		UTIL_TraceHull( 
			Vector( m_vecBuildOrigin.x, m_vecBuildOrigin.y, topZ ), 
			Vector( m_vecBuildOrigin.x, m_vecBuildOrigin.y, flBoxBottomZ ), 
			-vHalfBuildDimsXY, vHalfBuildDimsXY, MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );
		bottomZ = tr.endpos.z;

		// If there is no ground, then we can't place here.
		if ( tr.fraction == 1 )
		{
			m_vecBuildOrigin = vErrorOrigin;
			return false;
		}

		// if we found enough space to fit our object, place here
		if ( topZ - bottomZ > vBuildDims.z && !tr.startsolid )
		{
			break;
		}

		topZ += topZInc;
	}

	if ( iIteration == nIterations )
	{
		m_vecBuildOrigin = vErrorOrigin;
		return false;
	}

	// Now see if the range we've got leaves us room for our box.
	if ( topZ - bottomZ < vBuildDims.z )
	{
		m_vecBuildOrigin = vErrorOrigin;
		return false;
	}

	// Don't allow buildables on the train just yet.
	if ( tr.m_pEnt && tr.m_pEnt->IsBSPModel() )
	{
		if ( FClassnameIs( tr.m_pEnt, "func_tracktrain" ) )
			return false;
	}

	// Verify that it's not on too much of a slope by seeing how far the corners are from the ground.
	Vector vBottomCenter( m_vecBuildOrigin.x, m_vecBuildOrigin.y, bottomZ );
	if ( !VerifyCorner( vBottomCenter, -vHalfBuildDims.x, -vHalfBuildDims.y ) ||
		!VerifyCorner( vBottomCenter, +vHalfBuildDims.x, +vHalfBuildDims.y ) ||
		!VerifyCorner( vBottomCenter, +vHalfBuildDims.x, -vHalfBuildDims.y ) ||
		!VerifyCorner( vBottomCenter, -vHalfBuildDims.x, +vHalfBuildDims.y ) )
	{
		m_vecBuildOrigin = vErrorOrigin;
		return false;
	}

	// Ok, now we know the Z range where this box can fit.
	Vector vBottomLeft = m_vecBuildOrigin - vHalfBuildDims;
	vBottomLeft.z = bottomZ;
	m_vecBuildOrigin = vBottomLeft - m_vecBuildMins;

	m_vecBuildCenterOfMass = m_vecBuildOrigin + Vector( 0, 0, vHalfBuildDims.z );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks a position to make sure a corner of a building can live there
//-----------------------------------------------------------------------------
bool CBaseObject::VerifyCorner( const Vector &vBottomCenter, float xOffset, float yOffset )
{
	// NOTE: I am changing the 0.1 on the bottom start to 2.0 to deal with the epsilon differnece
	//       between the trace hull and trace line version of collision against a rotated bsp object.
	//       I will probably want to change the code if we find more bugs around this, but for now as 
	//       a test changing it hear should be fine.
	// Start slightly above the surface
	Vector vStart( vBottomCenter.x + xOffset, vBottomCenter.y + yOffset, vBottomCenter.z + 2.0 );

	trace_t tr;
	UTIL_TraceLine( 
		vStart, 
		vStart - Vector( 0, 0, TF_OBJ_GROUND_CLEARANCE ), 
		MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

	// Cannot build on very steep slopes ( > 45 degrees )
	if ( tr.fraction < 1.0f )
	{
		Vector vecUp(0,0,1);
		tr.plane.normal.NormalizeInPlace();
		float flDot = DotProduct( tr.plane.normal, vecUp );

		if ( flDot < 0.65 )
		{
			// Too steep
			return false;
		}
	}

	return !tr.startsolid && tr.fraction < 1;
}

//-----------------------------------------------------------------------------
// Purpose: Check that the selected position is buildable
//-----------------------------------------------------------------------------
bool CBaseObject::IsPlacementPosValid( void )
{
	bool bValid = CalculatePlacementPos();

	if ( !bValid )
	{
		return false;
	}

	CTFPlayer *pPlayer = GetOwner();

	if ( !pPlayer )
	{
		return false;
	}

#ifndef CLIENT_DLL
	if ( !EstimateValidBuildPos() )
		return false;
#endif

	// Verify that the entire object can fit here
	// Important! here we want to collide with players and other buildings, but not dropped weapons
	trace_t tr;
	UTIL_TraceEntity( this, m_vecBuildOrigin, m_vecBuildOrigin, MASK_SOLID, NULL, COLLISION_GROUP_PLAYER, &tr );

	if ( tr.fraction < 1.0f )
		return false;

	// Make sure we can see the final position (using a small hull to catch being able to build through seams in the map)
	UTIL_TraceHull( pPlayer->EyePosition(), m_vecBuildOrigin + Vector( 0, 0, m_vecBuildMaxs[2] * 0.5 ), Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), MASK_PLAYERSOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction < 1.0 )
	{
		return false;
	}

	// Make sure we're not building on top of another building (only an issue on stairs, inclines)
	// Note: Didn't use a hulltrace as it always returned the world, and not whatever objects were there (but maybe I was doing something wrong).
	const int nMaxEnts = 64;
	const float flBoxSize = 24.f;	// TODO(driller): Ask each object for Mins/Maxs, but this will do for now
	const float flBoxDepth = 32.f;
	CBaseEntity *pList[nMaxEnts];
	int nCount = UTIL_EntitiesInBox( pList, nMaxEnts, m_vecBuildOrigin + Vector( -flBoxSize, -flBoxSize, -flBoxDepth ), m_vecBuildOrigin + Vector( flBoxSize, flBoxSize, flBoxSize ), FL_OBJECT );
	// 	NDebugOverlay::Box( vecTestPos, Vector( -flBoxSize, -flBoxSize, -flBoxDepth ), TELEPORTER_MAXS, 255, 0, 0, 25, 0.5f );
	// 	NDebugOverlay::Cross3D( vecTestPos, 64, 0, 255, 25, false, 0.5f );
	for ( int i = 0; i < nCount; ++i )
	{
		if ( !pList[i] )
			continue;

		if ( pList[i] == this )
			continue;

		if ( pList[i]->IsBaseObject() )
		{
			CBaseObject *pObject = static_cast< CBaseObject* >( pList[i] );
			if ( pObject->IsPlacing() )
				continue;

			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Shared, update the build rotation
//-----------------------------------------------------------------------------

void CBaseObject::UpdateDesiredBuildRotation( float flFrameTime )
{
	// approach desired build rotation
	float flBuildRotation = 90.0f * m_iDesiredBuildRotations;

	m_flCurrentBuildRotation = ApproachAngle( flBuildRotation, m_flCurrentBuildRotation, tf_obj_build_rotation_speed.GetFloat() * flFrameTime );
}