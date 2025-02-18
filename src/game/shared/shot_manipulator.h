//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SHOT_MANIPULATOR_H
#define SHOT_MANIPULATOR_H
#ifdef _WIN32
#pragma once
#endif


#include "mathlib/vector.h"


extern ConVar ai_shot_bias_min;
extern ConVar ai_shot_bias_max;

class IUniformRandomStream;

//---------------------------------------------------------
// Caches off a shot direction and allows you to perform
// various operations on it without having to recalculate
// vecRight and vecUp each time. 
//---------------------------------------------------------
class CShotManipulator
{
public:
	CShotManipulator( const Vector &vecForward )
	{
		SetShootDir( vecForward );
	};

	void SetShootDir( const Vector &vecForward )
	{
		m_vecShotDirection = vecForward;
		VectorVectors( m_vecShotDirection, m_vecRight, m_vecUp );
	}

	const Vector &ApplySpread( const Vector &vecSpread, float bias = 1.0, IUniformRandomStream* pCustomRandom = NULL );

	const Vector &GetShotDirection()	{ return m_vecShotDirection; }
	const Vector &GetResult()			{ return m_vecResult; }
	const Vector &GetRightVector()		{ return m_vecRight; }
	const Vector &GetUpVector()			{ return m_vecUp;}

private:
	Vector m_vecShotDirection;
	Vector m_vecRight;
	Vector m_vecUp;
	Vector m_vecResult;
};

//---------------------------------------------------------
// Take a vector (direction) and another vector (spread) 
// and modify the direction to point somewhere within the 
// spread. This used to live inside FireBullets.
//---------------------------------------------------------
inline const Vector &CShotManipulator::ApplySpread( const Vector &vecSpread, float bias /*= 1.0*/, IUniformRandomStream* pCustomRandom /*= NULL*/ )
{
	// get circular gaussian spread
	float x, y, z;

	if ( bias > 1.0 )
		bias = 1.0;
	else if ( bias < 0.0 )
		bias = 0.0;

	float shotBiasMin = ai_shot_bias_min.GetFloat();
	float shotBiasMax = ai_shot_bias_max.GetFloat();

	// 1.0 gaussian, 0.0 is flat, -1.0 is inverse gaussian
	float shotBias = ( ( shotBiasMax - shotBiasMin ) * bias ) + shotBiasMin;

	float flatness = ( fabsf(shotBias) * 0.5 );

	do
	{
		IUniformRandomStream* pRandom = pCustomRandom ? pCustomRandom : random;
		x = pRandom->RandomFloat(-1,1) * flatness + pRandom->RandomFloat(-1,1) * (1 - flatness);
		y = pRandom->RandomFloat(-1,1) * flatness + pRandom->RandomFloat(-1,1) * (1 - flatness);
		if ( shotBias < 0 )
		{
			x = ( x >= 0 ) ? 1.0 - x : -1.0 - x;
			y = ( y >= 0 ) ? 1.0 - y : -1.0 - y;
		}
		z = x*x+y*y;
	} while (z > 1);

	m_vecResult = m_vecShotDirection + x * vecSpread.x * m_vecRight + y * vecSpread.y * m_vecUp;

	return m_vecResult;
}


#endif // SHOT_MANIPULATOR_H
