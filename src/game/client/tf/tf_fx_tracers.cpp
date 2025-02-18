//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================
#include "cbase.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "tier0/vprof.h"
#include "clientsideeffects.h"
#include "clienteffectprecachesystem.h"
#include "view.h"
#include "collisionutils.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheTFTracers )
	CLIENTEFFECT_MATERIAL( "effects/spark" )
CLIENTEFFECT_REGISTER_END()

#define LISTENER_HEIGHT 24


#define TRACER_TYPE_FAINT	4

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FX_TFTracerSound( const Vector &start, const Vector &end, int iTracerType )
{
	// don't play on very short hits
	if ( ( start - end ).Length() < 200 )
		return;
	
	const char *pszSoundName = "Bullets.DefaultNearmiss";
	float flWhizDist = 64;
	Vector vecListenOrigin = MainViewOrigin();

	switch( iTracerType )
	{
	case TRACER_TYPE_DEFAULT:
		flWhizDist = 96;
		// fall through !

	default:
		{
			Ray_t bullet, listener;
			bullet.Init( start, end );

			Vector vecLower = vecListenOrigin;
			vecLower.z -= LISTENER_HEIGHT;
			listener.Init( vecListenOrigin,	vecLower );

			float s, t;
			IntersectRayWithRay( bullet, listener, s, t );
			t = clamp( t, 0.f, 1.f );
			vecListenOrigin.z -= t * LISTENER_HEIGHT;
		}
		break;
	}

	static float flNextWhizTime = 0;

	// Is it time yet?
	float dt = flNextWhizTime - gpGlobals->curtime;
	if ( dt > 0 )
		return;

	// Did the thing pass close enough to our head?
	float vDist = CalcDistanceSqrToLineSegment( vecListenOrigin, start, end );
	if ( vDist >= (flWhizDist * flWhizDist) )
		return;

	CSoundParameters params;
	if( C_BaseEntity::GetParametersForSound( pszSoundName, params, NULL ) )
	{
		// Get shot direction
		Vector shotDir;
		VectorSubtract( end, start, shotDir );
		VectorNormalize( shotDir );

		CLocalPlayerFilter filter;
		enginesound->EmitSound(	filter, SOUND_FROM_WORLD, CHAN_STATIC, params.soundname, 
			params.volume, SNDLVL_TO_ATTN(params.soundlevel), 0, params.pitch, 0, &start, &shotDir, NULL);
	}

	// Don't play another bullet whiz for this client until this time has run out
	flNextWhizTime = gpGlobals->curtime + 0.1f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FX_BrightTracer( Vector& start, Vector& end )
{
	//Don't make small tracers
	float dist;
	Vector dir;
	int velocity = 5000;

	VectorSubtract( end, start, dir );
	dist = VectorNormalize( dir );

	// Don't make short tracers.
	float length = random->RandomFloat( 64.0f, 128.0f );
	float life = ( dist + length ) / velocity;	//NOTENOTE: We want the tail to finish its run as well
		
	//Add it
	FX_AddDiscreetLine( start, dir, velocity, length, dist, random->RandomFloat( 1, 3 ), life, "effects/spark" );

	FX_TFTracerSound( start, end, TRACER_TYPE_DEFAULT );	
}

//-----------------------------------------------------------------------------
// Purpose: Bright Tracer for Machine Guns
//-----------------------------------------------------------------------------
void BrightTracerCallback( const CEffectData &data )
{
	FX_BrightTracer( (Vector&)data.m_vStart, (Vector&)data.m_vOrigin );
}

DECLARE_CLIENT_EFFECT( "BrightTracer", BrightTracerCallback );