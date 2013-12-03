//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "clienteffectprecachesystem.h"
#include "fx_sparks.h"
#include "iefx.h"
#include "c_te_effect_dispatch.h"
#include "particles_ez.h"
#include "decals.h"
#include "engine/IEngineSound.h"
#include "fx_quad.h"
#include "tier0/vprof.h"
#include "fx.h"
#include "fx_water.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectSplash )
CLIENTEFFECT_MATERIAL( "effects/splash1" )
CLIENTEFFECT_MATERIAL( "effects/splash2" )
CLIENTEFFECT_MATERIAL( "effects/splash4" )
CLIENTEFFECT_MATERIAL( "effects/slime1" )
CLIENTEFFECT_REGISTER_END()


#define	SPLASH_MIN_SPEED	50.0f
#define	SPLASH_MAX_SPEED	100.0f

ConVar	cl_show_splashes( "cl_show_splashes", "1" );

static Vector s_vecSlimeColor( 46.0f/255.0f, 90.0f/255.0f, 36.0f/255.0f );

// Each channel does not contribute to the luminosity equally, as represented here
#define	RED_CHANNEL_CONTRIBUTION	0.30f
#define GREEN_CHANNEL_CONTRIBUTION	0.59f
#define	BLUE_CHANNEL_CONTRIBUTION	0.11f

//-----------------------------------------------------------------------------
// Purpose: Returns a normalized tint and luminosity for a specified color
// Input  : &color - normalized input color to extract information from
//			*tint - normalized tint of that color
//			*luminosity - normalized luminosity of that color
//-----------------------------------------------------------------------------
void UTIL_GetNormalizedColorTintAndLuminosity( const Vector &color, Vector *tint, float *luminosity )
{
	// Give luminosity if requested
	if ( luminosity != NULL )
	{
		// Each channel contributes differently than the others
		*luminosity =	( color.x * RED_CHANNEL_CONTRIBUTION ) +
						( color.y * GREEN_CHANNEL_CONTRIBUTION ) +
						( color.z * BLUE_CHANNEL_CONTRIBUTION );
	}

	// Give tint if requested
	if ( tint != NULL )
	{
		if ( color == vec3_origin )
		{
			*tint = vec3_origin;
		}
		else
		{
			float maxComponent = MAX( color.x, MAX( color.y, color.z ) );
			*tint = color / maxComponent;
		}
	}

}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&normal - 
//			scale - 
//-----------------------------------------------------------------------------
void FX_WaterRipple( const Vector &origin, float scale, Vector *pColor, float flLifetime, float flAlpha )
{
	VPROF_BUDGET( "FX_WaterRipple", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	trace_t	tr;

	Vector	color = pColor ? *pColor : Vector( 0.8f, 0.8f, 0.75f );

	Vector startPos = origin + Vector(0,0,8);
	Vector endPos = origin + Vector(0,0,-64);

	UTIL_TraceLine( startPos, endPos, MASK_WATER, NULL, COLLISION_GROUP_NONE, &tr );
	
	if ( tr.fraction < 1.0f )
	{
		//Add a ripple quad to the surface
		FX_AddQuad( tr.endpos + ( tr.plane.normal * 0.5f ), 
					tr.plane.normal, 
					16.0f*scale, 
					128.0f*scale, 
					0.7f,
					flAlpha,	// start alpha
					0.0f,		// end alpha
					0.25f,
					random->RandomFloat( 0, 360 ),
					random->RandomFloat( -16.0f, 16.0f ),
					color, 
					flLifetime, 
					"effects/splashwake1", 
					(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&normal - 
//-----------------------------------------------------------------------------
void FX_GunshotSplash( const Vector &origin, const Vector &normal, float scale )
{
	VPROF_BUDGET( "FX_GunshotSplash", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	
	if ( cl_show_splashes.GetBool() == false )
		return;

	Vector	color;
	float	luminosity;
	
	// Get our lighting information
	FX_GetSplashLighting( origin + ( normal * scale ), &color, &luminosity );

	float flScale = scale / 8.0f;

	if ( flScale > 4.0f )
	{
		flScale = 4.0f;
	}

	// Setup our trail emitter
	CSmartPtr<CTrailParticles> sparkEmitter = CTrailParticles::Create( "splash" );

	if ( !sparkEmitter )
		return;

	sparkEmitter->SetSortOrigin( origin );
	sparkEmitter->m_ParticleCollision.SetGravity( 800.0f );
	sparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
	sparkEmitter->SetVelocityDampen( 2.0f );
	sparkEmitter->GetBinding().SetBBox( origin - Vector( 32, 32, 32 ), origin + Vector( 32, 32, 32 ) );

	PMaterialHandle	hMaterial = ParticleMgr()->GetPMaterial( "effects/splash2" );

	TrailParticle	*tParticle;

	Vector	offDir;
	Vector	offset;
	float	colorRamp;

	//Dump out drops
	for ( int i = 0; i < 16; i++ )
	{
		offset = origin;
		offset[0] += random->RandomFloat( -8.0f, 8.0f ) * flScale;
		offset[1] += random->RandomFloat( -8.0f, 8.0f ) * flScale;

		tParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

		if ( tParticle == NULL )
			break;

		tParticle->m_flLifetime	= 0.0f;
		tParticle->m_flDieTime	= random->RandomFloat( 0.25f, 0.5f );

		offDir = normal + RandomVector( -0.8f, 0.8f );

		tParticle->m_vecVelocity = offDir * random->RandomFloat( SPLASH_MIN_SPEED * flScale * 3.0f, SPLASH_MAX_SPEED * flScale * 3.0f );
		tParticle->m_vecVelocity[2] += random->RandomFloat( 32.0f, 64.0f ) * flScale;

		tParticle->m_flWidth		= random->RandomFloat( 1.0f, 3.0f );
		tParticle->m_flLength		= random->RandomFloat( 0.025f, 0.05f );

		colorRamp = random->RandomFloat( 0.75f, 1.25f );

		tParticle->m_color.r = MIN( 1.0f, color[0] * colorRamp ) * 255;
		tParticle->m_color.g = MIN( 1.0f, color[1] * colorRamp ) * 255;
		tParticle->m_color.b = MIN( 1.0f, color[2] * colorRamp ) * 255;
		tParticle->m_color.a = luminosity * 255;
	}

	// Setup the particle emitter
	CSmartPtr<CSplashParticle> pSimple = CSplashParticle::Create( "splish" );
	pSimple->SetSortOrigin( origin );
	pSimple->SetClipHeight( origin.z );
	pSimple->SetParticleCullRadius( scale * 2.0f );
	pSimple->GetBinding().SetBBox( origin - Vector( 32, 32, 32 ), origin + Vector( 32, 32, 32 ) );

	SimpleParticle	*pParticle;

	//Main gout
	for ( int i = 0; i < 8; i++ )
	{
		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, origin );

		if ( pParticle == NULL )
			break;

		pParticle->m_flLifetime = 0.0f;
		pParticle->m_flDieTime	= 2.0f;	//NOTENOTE: We use a clip plane to realistically control our lifespan

		pParticle->m_vecVelocity.Random( -0.2f, 0.2f );
		pParticle->m_vecVelocity += ( normal * random->RandomFloat( 4.0f, 6.0f ) );
		
		VectorNormalize( pParticle->m_vecVelocity );

		pParticle->m_vecVelocity *= 50 * flScale * (8-i);
		
		colorRamp = random->RandomFloat( 0.75f, 1.25f );

		pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;
		
		pParticle->m_uchStartSize	= 24 * flScale * RemapValClamped( i, 7, 0, 1, 0.5f );
		pParticle->m_uchEndSize		= MIN( 255, pParticle->m_uchStartSize * 2 );
		
		pParticle->m_uchStartAlpha	= RemapValClamped( i, 7, 0, 255, 32 ) * luminosity;
		pParticle->m_uchEndAlpha	= 0;
		
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -4.0f, 4.0f );
	}

	// Do a ripple
	FX_WaterRipple( origin, flScale, &color, 1.5f, luminosity );

	//Play a sound
	CLocalPlayerFilter filter;

	EmitSound_t ep;
	ep.m_nChannel = CHAN_VOICE;
	ep.m_pSoundName =  "Physics.WaterSplash";
	ep.m_flVolume = 1.0f;
	ep.m_SoundLevel = SNDLVL_NORM;
	ep.m_pOrigin = &origin;


	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, ep );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&normal - 
//			scale - 
//			*pColor - 
//-----------------------------------------------------------------------------
void FX_GunshotSlimeSplash( const Vector &origin, const Vector &normal, float scale )
{
	if ( cl_show_splashes.GetBool() == false )
		return;

	VPROF_BUDGET( "FX_GunshotSlimeSplash", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	
#if 0

	float	colorRamp;
	float	flScale = MIN( 1.0f, scale / 8.0f );

	PMaterialHandle	hMaterial = ParticleMgr()->GetPMaterial( "effects/slime1" );
	PMaterialHandle	hMaterial2 = ParticleMgr()->GetPMaterial( "effects/splash4" );

	Vector	color;
	float	luminosity;
	
	// Get our lighting information
	FX_GetSplashLighting( origin + ( normal * scale ), &color, &luminosity );

	Vector	offDir;
	Vector	offset;

	TrailParticle	*tParticle;

	CSmartPtr<CTrailParticles> sparkEmitter = CTrailParticles::Create( "splash" );

	if ( !sparkEmitter )
		return;

	sparkEmitter->SetSortOrigin( origin );
	sparkEmitter->m_ParticleCollision.SetGravity( 800.0f );
	sparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
	sparkEmitter->SetVelocityDampen( 2.0f );
	if ( IsXbox() )
	{
		sparkEmitter->GetBinding().SetBBox( origin - Vector( 32, 32, 64 ), origin + Vector( 32, 32, 64 ) );
	}

	//Dump out drops
	for ( int i = 0; i < 24; i++ )
	{
		offset = origin;
		offset[0] += random->RandomFloat( -16.0f, 16.0f ) * flScale;
		offset[1] += random->RandomFloat( -16.0f, 16.0f ) * flScale;

		tParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

		if ( tParticle == NULL )
			break;

		tParticle->m_flLifetime	= 0.0f;
		tParticle->m_flDieTime	= random->RandomFloat( 0.25f, 0.5f );

		offDir = normal + RandomVector( -0.6f, 0.6f );

		tParticle->m_vecVelocity = offDir * random->RandomFloat( SPLASH_MIN_SPEED * flScale * 3.0f, SPLASH_MAX_SPEED * flScale * 3.0f );
		tParticle->m_vecVelocity[2] += random->RandomFloat( 32.0f, 64.0f ) * flScale;
   
		tParticle->m_flWidth		= random->RandomFloat( 3.0f, 6.0f ) * flScale;
		tParticle->m_flLength		= random->RandomFloat( 0.025f, 0.05f ) * flScale;

		colorRamp = random->RandomFloat( 0.75f, 1.25f );

		tParticle->m_color.r = MIN( 1.0f, color.x * colorRamp ) * 255;
		tParticle->m_color.g = MIN( 1.0f, color.y * colorRamp ) * 255;
		tParticle->m_color.b = MIN( 1.0f, color.z * colorRamp ) * 255;
		tParticle->m_color.a = 255 * luminosity;
	}

	// Setup splash emitter
	CSmartPtr<CSplashParticle> pSimple = CSplashParticle::Create( "splish" );
	pSimple->SetSortOrigin( origin );
	pSimple->SetClipHeight( origin.z );
	pSimple->SetParticleCullRadius( scale * 2.0f );

	if ( IsXbox() )
	{
		pSimple->GetBinding().SetBBox( origin - Vector( 32, 32, 64 ), origin + Vector( 32, 32, 64 ) );
	}

	SimpleParticle	*pParticle;

	// Tint
	colorRamp = random->RandomFloat( 0.75f, 1.0f );
	color = Vector( 1.0f, 0.8f, 0.0f ) * color * colorRamp;

	//Main gout
	for ( int i = 0; i < 8; i++ )
	{
		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial2, origin );

		if ( pParticle == NULL )
			break;

		pParticle->m_flLifetime = 0.0f;
		pParticle->m_flDieTime	= 2.0f;	//NOTENOTE: We use a clip plane to realistically control our lifespan

		pParticle->m_vecVelocity.Random( -0.2f, 0.2f );
		pParticle->m_vecVelocity += ( normal * random->RandomFloat( 4.0f, 6.0f ) );
		
		VectorNormalize( pParticle->m_vecVelocity );

		pParticle->m_vecVelocity *= 50 * flScale * (8-i);
		
		colorRamp = random->RandomFloat( 0.75f, 1.25f );

		pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;
		
		pParticle->m_uchStartSize	= 24 * flScale * RemapValClamped( i, 7, 0, 1, 0.5f );
		pParticle->m_uchEndSize		= MIN( 255, pParticle->m_uchStartSize * 2 );
		
		pParticle->m_uchStartAlpha	= RemapValClamped( i, 7, 0, 255, 32 ) * luminosity;
		pParticle->m_uchEndAlpha	= 0;
		
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -4.0f, 4.0f );
	}
	
#else
	
	QAngle vecAngles;
	VectorAngles( normal, vecAngles );
	if ( scale < 2.0f )
	{
		DispatchParticleEffect( "slime_splash_01", origin, vecAngles );
	}
	else if ( scale < 4.0f )
	{
		DispatchParticleEffect( "slime_splash_02", origin, vecAngles );
	}
	else
	{
		DispatchParticleEffect( "slime_splash_03", origin, vecAngles );
	}

#endif

	//Play a sound
	CLocalPlayerFilter filter;

	EmitSound_t ep;
	ep.m_nChannel = CHAN_VOICE;
	ep.m_pSoundName =  "Physics.WaterSplash";
	ep.m_flVolume = 1.0f;
	ep.m_SoundLevel = SNDLVL_NORM;
	ep.m_pOrigin = &origin;

	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, ep );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SplashCallback( const CEffectData &data )
{
	Vector	normal;

	AngleVectors( data.m_vAngles, &normal );

	if ( data.m_fFlags & FX_WATER_IN_SLIME )
	{
		FX_GunshotSlimeSplash( data.m_vOrigin, Vector(0,0,1), data.m_flScale );
	}
	else
	{
		FX_GunshotSplash( data.m_vOrigin, Vector(0,0,1), data.m_flScale );
	}
}

DECLARE_CLIENT_EFFECT( "watersplash", SplashCallback );


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void GunshotSplashCallback( const CEffectData &data )
{
	if ( data.m_fFlags & FX_WATER_IN_SLIME )
	{
		FX_GunshotSlimeSplash( data.m_vOrigin, Vector(0,0,1), data.m_flScale );
	}
	else
	{
		FX_GunshotSplash( data.m_vOrigin, Vector(0,0,1), data.m_flScale );
	}
}

DECLARE_CLIENT_EFFECT( "gunshotsplash", GunshotSplashCallback );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void RippleCallback( const CEffectData &data )
{
	float	flScale = data.m_flScale / 8.0f;

	Vector	color;
	float	luminosity;
	
	// Get our lighting information
	FX_GetSplashLighting( data.m_vOrigin + ( Vector(0,0,1) * 4.0f ), &color, &luminosity );

	FX_WaterRipple( data.m_vOrigin, flScale, &color, 1.5f, luminosity );
}

DECLARE_CLIENT_EFFECT( "waterripple", RippleCallback );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pDebugName - 
// Output : WaterDebrisEffect*
//-----------------------------------------------------------------------------
WaterDebrisEffect* WaterDebrisEffect::Create( const char *pDebugName )
{
	return new WaterDebrisEffect( pDebugName );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float WaterDebrisEffect::UpdateAlpha( const SimpleParticle *pParticle )
{
	return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float CSplashParticle::UpdateRoll( SimpleParticle *pParticle, float timeDelta )
{
	pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
	
	pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -4.0f );

	//Cap the minimum roll
	if ( fabs( pParticle->m_flRollDelta ) < 0.5f )
	{
		pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.5f : -0.5f;
	}

	return pParticle->m_flRoll;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
void CSplashParticle::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
	//Decellerate
	static float dtime;
	static float decay;

	if ( dtime != timeDelta )
	{
		dtime = timeDelta;
		float expected = 3.0f;
		decay = exp( log( 0.0001f ) * dtime / expected );
	}

	pParticle->m_vecVelocity *= decay;
	pParticle->m_vecVelocity[2] -= ( 800.0f * timeDelta );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
// Output : float
//-----------------------------------------------------------------------------
float CSplashParticle::UpdateAlpha( const SimpleParticle *pParticle )
{
	if ( m_bUseClipHeight )
	{
		float flAlpha = pParticle->m_uchStartAlpha / 255.0f;

		return  flAlpha * RemapValClamped(pParticle->m_Pos.z,
								m_flClipHeight,
								m_flClipHeight - ( UpdateScale( pParticle ) * 0.5f ),
								1.0f,
								0.0f );
	}

	return (pParticle->m_uchStartAlpha/255.0f) + ( (float)(pParticle->m_uchEndAlpha/255.0f) - (float)(pParticle->m_uchStartAlpha/255.0f) ) * (pParticle->m_flLifetime / pParticle->m_flDieTime);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &clipPlane - 
//-----------------------------------------------------------------------------
void CSplashParticle::SetClipHeight( float flClipHeight )
{
	m_bUseClipHeight = true;
	m_flClipHeight = flClipHeight;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pIterator - 
//-----------------------------------------------------------------------------
void CSplashParticle::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	float timeDelta = pIterator->GetTimeDelta();

	SimpleParticle *pParticle = (SimpleParticle*)pIterator->GetFirst();
	
	while ( pParticle )
	{
		//Update velocity
		UpdateVelocity( pParticle, timeDelta );
		pParticle->m_Pos += pParticle->m_vecVelocity * timeDelta;

		// Clip by height if requested
		if ( m_bUseClipHeight )
		{
			// See if we're below, and therefore need to clip
			if ( pParticle->m_Pos.z + UpdateScale( pParticle ) < m_flClipHeight )
			{
				pIterator->RemoveParticle( pParticle );
				pParticle = (SimpleParticle*)pIterator->GetNext();
				continue;
			}
		}

		//Should this particle die?
		pParticle->m_flLifetime += timeDelta;
		UpdateRoll( pParticle, timeDelta );

		if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
			pIterator->RemoveParticle( pParticle );

		pParticle = (SimpleParticle*)pIterator->GetNext();
	}
}
