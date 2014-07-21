//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DEATH_POSE_H
#define DEATH_POSE_H
#ifdef _WIN32
#pragma once
#endif

enum
{
	DEATH_FRAME_HEAD = 1,
	DEATH_FRAME_STOMACH,
	DEATH_FRAME_LEFTARM,
	DEATH_FRAME_RIGHTARM,
	DEATH_FRAME_LEFTLEG,
	DEATH_FRAME_RIGHTLEG,
	MAX_DEATHPOSE_FRAMES  = DEATH_FRAME_RIGHTLEG
};

#ifdef CLIENT_DLL

void GetRagdollCurSequenceWithDeathPose( C_BaseAnimating *entity, matrix3x4_t *curBones, float flTime, int activity, int frame );

#else // !CLIENT_DLL

/// Calculates death pose activity and frame
void SelectDeathPoseActivityAndFrame( CBaseAnimating *entity, const CTakeDamageInfo &info, int hitgroup, Activity& activity, int& frame );

#endif // !CLIENT_DLL

#endif // DEATH_POSE_H
