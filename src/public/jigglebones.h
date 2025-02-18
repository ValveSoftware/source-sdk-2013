//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#ifndef C_JIGGLEBONES_H
#define C_JIGGLEBONES_H

#ifdef _WIN32
#pragma once
#endif

#include "studio.h"
#include "utlvector.h"
#include "utllinkedlist.h"

//-----------------------------------------------------------------------------
/**
 * JiggleData is the instance-specific data for a jiggle bone
 */
struct JiggleData
{
	void Init( int initBone, float currenttime, const Vector &initBasePos, const Vector &initTipPos )
	{
		bone = initBone;

		lastUpdate = currenttime;

		basePos = initBasePos;
		baseLastPos = basePos;
		baseVel.Init();
		baseAccel.Init();

		tipPos = initTipPos;
		tipVel.Init();
		tipAccel.Init();

		lastLeft = Vector( 0, 0, 0 );

		lastBoingPos = initBasePos;
		boingDir = Vector( 0.0f, 0.0f, 1.0f );
		boingVelDir.Init();
		boingSpeed = 0.0f;
		boingTime = 0.0f;
	}

	int bone;

	float lastUpdate;	// based on gpGlobals->realtime

	Vector basePos;		// position of the base of the jiggle bone
	Vector baseLastPos;
	Vector baseVel;
	Vector baseAccel;

	Vector tipPos;		// position of the tip of the jiggle bone
	Vector tipVel;
	Vector tipAccel;
	Vector lastLeft;		// previous up vector

	Vector lastBoingPos;	// position of base of jiggle bone last update for tracking velocity
	Vector boingDir;		// current direction along which the boing effect is occurring
	Vector boingVelDir;		// current estimation of jiggle bone unit velocity vector for boing effect
	float boingSpeed;		// current estimation of jiggle bone speed for boing effect
	float boingTime;
	
	int useGoalMatrixCount;	// Count of times we need to fast draw using goal matrix.
	int useJiggleBoneCount; // Count of times we need to draw using real jiggly bones.
};

class CJiggleBones
{
public:
	JiggleData * GetJiggleData( int bone, float currenttime, const Vector &initBasePos, const Vector &initTipPos );
	void BuildJiggleTransformations( int boneIndex, float currentime, const mstudiojigglebone_t *jiggleParams, const matrix3x4_t &goalMX, matrix3x4_t &boneMX );

	CUtlLinkedList< JiggleData >	m_jiggleBoneState;
};


extern void DevMsgRT( PRINTF_FORMAT_STRING char const* pMsg, ... );

#endif // C_BASEANIMATING_H
