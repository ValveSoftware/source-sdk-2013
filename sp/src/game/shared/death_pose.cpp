//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "death_pose.h"

#ifdef CLIENT_DLL

void GetRagdollCurSequenceWithDeathPose( C_BaseAnimating *entity, matrix3x4_t *curBones, float flTime, int activity, int frame )
{
	// blow the cached prev bones
	entity->InvalidateBoneCache();

	Vector vPrevOrigin = entity->GetAbsOrigin();

	entity->Interpolate( flTime );
	
	if ( activity != ACT_INVALID )
	{
		Vector vNewOrigin = entity->GetAbsOrigin();
		Vector vDirection = vNewOrigin - vPrevOrigin;

		float flVelocity = VectorNormalize( vDirection );

		Vector vAdjustedOrigin = vNewOrigin + vDirection * ( ( flVelocity * flVelocity ) * gpGlobals->frametime );

		int iTempSequence = entity->GetSequence();
		float flTempCycle = entity->GetCycle();

		entity->SetSequence( activity );

		entity->SetCycle( (float)frame / MAX_DEATHPOSE_FRAMES );

		entity->SetAbsOrigin( vAdjustedOrigin );

		// Now do the current bone setup
		entity->SetupBones( curBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, flTime );

		entity->SetAbsOrigin( vNewOrigin );

		// blow the cached prev bones
		entity->InvalidateBoneCache();

		entity->SetSequence( iTempSequence );
		entity->SetCycle( flTempCycle );

		entity->Interpolate( gpGlobals->curtime );

		entity->SetupBones( NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime );
	}
	else
	{
		entity->SetupBones( curBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, flTime );

		// blow the cached prev bones
		entity->InvalidateBoneCache();

		entity->SetupBones( NULL, -1, BONE_USED_BY_ANYTHING, flTime );
	}
}

#else // !CLIENT_DLL

Activity GetDeathPoseActivity( CBaseAnimating *entity, const CTakeDamageInfo &info )
{
	if ( !entity )
	{
		return ACT_INVALID;
	}

	Activity aActivity;

	Vector vForward, vRight;
	entity->GetVectors( &vForward, &vRight, NULL );

	Vector vDir = -info.GetDamageForce();
	VectorNormalize( vDir );

	float flDotForward	= DotProduct( vForward, vDir );
	float flDotRight	= DotProduct( vRight, vDir );

	bool bNegativeForward = false;
	bool bNegativeRight = false;

	if ( flDotForward < 0.0f )
	{
		bNegativeForward = true;
		flDotForward = flDotForward * -1;
	}

	if ( flDotRight < 0.0f )
	{
		bNegativeRight = true;
		flDotRight = flDotRight * -1;
	}

	if ( flDotRight > flDotForward )
	{
		if ( bNegativeRight == true )
			aActivity = ACT_DIE_LEFTSIDE;
		else 
			aActivity = ACT_DIE_RIGHTSIDE;
	}
	else
	{
		if ( bNegativeForward == true )
			aActivity = ACT_DIE_BACKSIDE;
		else 
			aActivity = ACT_DIE_FRONTSIDE;
	}

	return aActivity;
}

void SelectDeathPoseActivityAndFrame( CBaseAnimating *entity, const CTakeDamageInfo &info, int hitgroup, Activity& activity, int& frame )
{
	activity = ACT_INVALID;
	frame = 0;

	if ( !entity->GetModelPtr() )
		return;

	activity = GetDeathPoseActivity( entity, info );
	frame = DEATH_FRAME_HEAD;

	switch( hitgroup )
	{
		//Do normal ragdoll stuff if no specific hitgroup was hit.
		case HITGROUP_GENERIC:
		default:
		{
			return;
		}
			
		case HITGROUP_HEAD:
		{
			frame = DEATH_FRAME_HEAD;
			break;
		}
		case HITGROUP_LEFTARM:
			frame = DEATH_FRAME_LEFTARM;
			break;

		case HITGROUP_RIGHTARM:
			frame = DEATH_FRAME_RIGHTARM;
			break;

		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
			frame = DEATH_FRAME_STOMACH;
			break;
	
		case HITGROUP_LEFTLEG:
			frame = DEATH_FRAME_LEFTLEG;
			break;

		case HITGROUP_RIGHTLEG:
			frame = DEATH_FRAME_RIGHTLEG;
			break;
	}
}

#endif // !CLIENT_DLL
