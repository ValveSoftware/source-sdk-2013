//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-server neutral effects interface
//
// $NoKeywords: $
//=============================================================================//

#ifndef IEFFECTS_H
#define IEFFECTS_H

#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "mathlib/vector.h"
#include "interface.h"
#include "ipredictionsystem.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
enum ShakeCommand_t;
class Vector;
class CGameTrace;
typedef CGameTrace trace_t;


//-----------------------------------------------------------------------------
// Client-server neutral effects interface
//-----------------------------------------------------------------------------
#define IEFFECTS_INTERFACE_VERSION	"IEffects001"
abstract_class IEffects : public IPredictionSystem
{
public:
	//
	// Particle effects
	//
	virtual void Beam( const Vector &Start, const Vector &End, int nModelIndex, 
		int nHaloIndex, unsigned char frameStart, unsigned char frameRate,
		float flLife, unsigned char width, unsigned char endWidth, unsigned char fadeLength, 
		unsigned char noise, unsigned char red, unsigned char green,
		unsigned char blue, unsigned char brightness, unsigned char speed) = 0;

	//-----------------------------------------------------------------------------
	// Purpose: Emits smoke sprites.
	// Input  : origin - Where to emit the sprites.
	//			scale - Sprite scale * 10.
	//			framerate - Framerate at which to animate the smoke sprites.
	//-----------------------------------------------------------------------------
	virtual void Smoke( const Vector &origin, int modelIndex, float scale, float framerate ) = 0;

	virtual void Sparks( const Vector &position, int nMagnitude = 1, int nTrailLength = 1, const Vector *pvecDir = NULL ) = 0;

	virtual void Dust( const Vector &pos, const Vector &dir, float size, float speed ) = 0;

	virtual void MuzzleFlash( const Vector &vecOrigin, const QAngle &vecAngles, float flScale, int iType ) = 0;

	// like ricochet, but no sound
	virtual void MetalSparks( const Vector &position, const Vector &direction ) = 0; 

	virtual void EnergySplash( const Vector &position, const Vector &direction, bool bExplosive = false ) = 0;

	virtual void Ricochet( const Vector &position, const Vector &direction ) = 0;

	// FIXME: Should these methods remain in this interface? Or go in some 
	// other client-server neutral interface?
	virtual float Time() = 0;
	virtual bool IsServer() = 0;

	// Used by the playback system to suppress sounds
	virtual void SuppressEffectsSounds( bool bSuppress ) = 0;
};


//-----------------------------------------------------------------------------
// Client-server neutral effects interface accessor
//-----------------------------------------------------------------------------
extern IEffects *g_pEffects;
			   

#endif // IEFFECTS_H
