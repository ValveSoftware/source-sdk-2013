//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "tier0/vprof.h"
#include "fx_line.h"
#include "fx_quad.h"
#include "view.h"
#include "particles_localspace.h"
#include "dlight.h"
#include "iefx.h"
#include "clienteffectprecachesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern Vector GetTracerOrigin( const CEffectData &data );
extern void FX_TracerSound( const Vector &start, const Vector &end, int iTracerType );

extern ConVar muzzleflash_light;


CLIENTEFFECT_REGISTER_BEGIN( PrecacheTracers )
CLIENTEFFECT_MATERIAL( "effects/gunshiptracer" )
CLIENTEFFECT_MATERIAL( "effects/combinemuzzle1" )
CLIENTEFFECT_MATERIAL( "effects/combinemuzzle2_nocull" )
CLIENTEFFECT_REGISTER_END()

//-----------------------------------------------------------------------------
// Purpose: Gunship's Tracer
//-----------------------------------------------------------------------------
void GunshipTracerCallback( const CEffectData &data )
{
	float flVelocity = data.m_flScale;
	bool bWhiz = (data.m_fFlags & TRACER_FLAG_WHIZ);
	FX_GunshipTracer( (Vector&)data.m_vStart, (Vector&)data.m_vOrigin, flVelocity, bWhiz );
}

DECLARE_CLIENT_EFFECT( "GunshipTracer", GunshipTracerCallback );


//-----------------------------------------------------------------------------
// Purpose: Strider's Tracer
//-----------------------------------------------------------------------------
void StriderTracerCallback( const CEffectData &data )
{
	float flVelocity = data.m_flScale;
	bool bWhiz = (data.m_fFlags & TRACER_FLAG_WHIZ);
	FX_StriderTracer( (Vector&)data.m_vStart, (Vector&)data.m_vOrigin, flVelocity, bWhiz );
}

DECLARE_CLIENT_EFFECT( "StriderTracer", StriderTracerCallback );


//-----------------------------------------------------------------------------
// Purpose: Hunter's Tracer
//-----------------------------------------------------------------------------
void HunterTracerCallback( const CEffectData &data )
{
	float flVelocity = data.m_flScale;
	bool bWhiz = (data.m_fFlags & TRACER_FLAG_WHIZ);
	FX_HunterTracer( (Vector&)data.m_vStart, (Vector&)data.m_vOrigin, flVelocity, bWhiz );
}

DECLARE_CLIENT_EFFECT( "HunterTracer", HunterTracerCallback );


//-----------------------------------------------------------------------------
// Purpose: Gauss Gun's Tracer
//-----------------------------------------------------------------------------
void GaussTracerCallback( const CEffectData &data )
{
	float flVelocity = data.m_flScale;
	bool bWhiz = (data.m_fFlags & TRACER_FLAG_WHIZ);
	FX_GaussTracer( (Vector&)data.m_vStart, (Vector&)data.m_vOrigin, flVelocity, bWhiz );
}

DECLARE_CLIENT_EFFECT( "GaussTracer", GaussTracerCallback );


//-----------------------------------------------------------------------------
// Purpose: Airboat gun tracers 
//-----------------------------------------------------------------------------
void AirboatGunHeavyTracerCallback( const CEffectData &data )
{
	// Grab the data
	Vector vecStart = GetTracerOrigin( data );
	float flVelocity = data.m_flScale;

	// Use default velocity if none specified
	if ( !flVelocity )
	{
		flVelocity = 8000;
	}

	//Get out shot direction and length
	Vector vecShotDir;
	VectorSubtract( data.m_vOrigin, vecStart, vecShotDir );
	float flTotalDist = VectorNormalize( vecShotDir );

	// Don't make small tracers
	if ( flTotalDist <= 64 )
		return;

	float flLength = random->RandomFloat( 300.0f, 400.0f );
	float flLife = ( flTotalDist + flLength ) / flVelocity;	//NOTENOTE: We want the tail to finish its run as well
	
	// Add it
	FX_AddDiscreetLine( vecStart, vecShotDir, flVelocity, flLength, flTotalDist, 5.0f, flLife, "effects/gunshiptracer" );
}

DECLARE_CLIENT_EFFECT( "AirboatGunHeavyTracer", AirboatGunHeavyTracerCallback );

//-----------------------------------------------------------------------------
// Purpose: Airboat gun tracers 
//-----------------------------------------------------------------------------
void AirboatGunTracerCallback( const CEffectData &data )
{
	// Grab the data
	Vector vecStart = GetTracerOrigin( data );
	float flVelocity = data.m_flScale;

	// Use default velocity if none specified
	if ( !flVelocity )
	{
		flVelocity = 10000;
	}

	//Get out shot direction and length
	Vector vecShotDir;
	VectorSubtract( data.m_vOrigin, vecStart, vecShotDir );
	float flTotalDist = VectorNormalize( vecShotDir );

	// Don't make small tracers
	if ( flTotalDist <= 64 )
		return;

	float flLength = random->RandomFloat( 256.0f, 384.0f );
	float flLife = ( flTotalDist + flLength ) / flVelocity;	//NOTENOTE: We want the tail to finish its run as well
	
	// Add it
	FX_AddDiscreetLine( vecStart, vecShotDir, flVelocity, flLength, flTotalDist, 2.0f, flLife, "effects/gunshiptracer" );
}

DECLARE_CLIENT_EFFECT( "AirboatGunTracer", AirboatGunTracerCallback );


//-----------------------------------------------------------------------------
// Purpose: Airboat gun tracers 
//-----------------------------------------------------------------------------
void HelicopterTracerCallback( const CEffectData &data )
{
	// Grab the data
	Vector vecStart = GetTracerOrigin( data );
	float flVelocity = data.m_flScale;

	// Use default velocity if none specified
	if ( !flVelocity )
	{
		flVelocity = 8000;
	}

	//Get out shot direction and length
	Vector vecShotDir;
	VectorSubtract( data.m_vOrigin, vecStart, vecShotDir );
	float flTotalDist = VectorNormalize( vecShotDir );

	// Don't make small tracers
	if ( flTotalDist <= 256 )
		return;

	float flLength = random->RandomFloat( 256.0f, 384.0f );
	float flLife = ( flTotalDist + flLength ) / flVelocity;	//NOTENOTE: We want the tail to finish its run as well
	
	// Add it
	FX_AddDiscreetLine( vecStart, vecShotDir, flVelocity, flLength, flTotalDist, 5.0f, flLife, "effects/gunshiptracer" );

	if (data.m_fFlags & TRACER_FLAG_WHIZ)
	{
		FX_TracerSound( vecStart, data.m_vOrigin, TRACER_TYPE_GUNSHIP );
	}
}

DECLARE_CLIENT_EFFECT( "HelicopterTracer", HelicopterTracerCallback );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			end - 
//-----------------------------------------------------------------------------
void FX_PlayerAR2Tracer( const Vector &start, const Vector &end )
{
	VPROF_BUDGET( "FX_PlayerAR2Tracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	
	Vector	shotDir, dStart, dEnd;
	float	length;

	//Find the direction of the tracer
	VectorSubtract( end, start, shotDir );
	length = VectorNormalize( shotDir );

	//We don't want to draw them if they're too close to us
	if ( length < 128 )
		return;

	//Randomly place the tracer along this line, with a random length
	VectorMA( start, random->RandomFloat( 0.0f, 8.0f ), shotDir, dStart );
	VectorMA( dStart, MIN( length, random->RandomFloat( 256.0f, 1024.0f ) ), shotDir, dEnd );

	//Create the line
	CFXStaticLine *tracerLine = new CFXStaticLine( "Tracer", dStart, dEnd, random->RandomFloat( 6.0f, 12.0f ), 0.01f, "effects/gunshiptracer", 0 );
	assert( tracerLine );

	//Throw it into the list
	clienteffects->AddEffect( tracerLine );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			end - 
//			velocity - 
//			makeWhiz - 
//-----------------------------------------------------------------------------
void FX_AR2Tracer( Vector& start, Vector& end, int velocity, bool makeWhiz )
{
	VPROF_BUDGET( "FX_AR2Tracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	
	//Don't make small tracers
	float dist;
	Vector dir;

	VectorSubtract( end, start, dir );
	dist = VectorNormalize( dir );

	// Don't make short tracers.
	if ( dist < 128 )
		return;

	float length = random->RandomFloat( 128.0f, 256.0f );
	float life = ( dist + length ) / velocity;	//NOTENOTE: We want the tail to finish its run as well
	
	//Add it
	FX_AddDiscreetLine( start, dir, velocity, length, dist, random->RandomFloat( 0.5f, 1.5f ), life, "effects/gunshiptracer" );

	if( makeWhiz )
	{
		FX_TracerSound( start, end, TRACER_TYPE_GUNSHIP );	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AR2TracerCallback( const CEffectData &data )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	
	if ( player == NULL )
		return;

	// Grab the data
	Vector vecStart = GetTracerOrigin( data );
	float flVelocity = data.m_flScale;
	bool bWhiz = (data.m_fFlags & TRACER_FLAG_WHIZ);
	int iEntIndex = data.entindex();

	if ( iEntIndex && iEntIndex == player->index )
	{
		Vector	foo = data.m_vStart;
		QAngle	vangles;
		Vector	vforward, vright, vup;

		engine->GetViewAngles( vangles );
		AngleVectors( vangles, &vforward, &vright, &vup );

		VectorMA( data.m_vStart, 4, vright, foo );
		foo[2] -= 0.5f;

		FX_PlayerAR2Tracer( foo, (Vector&)data.m_vOrigin );
		return;
	}
	
	// Use default velocity if none specified
	if ( !flVelocity )
	{
		flVelocity = 8000;
	}

	// Do tracer effect
	FX_AR2Tracer( (Vector&)vecStart, (Vector&)data.m_vOrigin, flVelocity, bWhiz );
}

DECLARE_CLIENT_EFFECT( "AR2Tracer", AR2TracerCallback );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void AR2ExplosionCallback( const CEffectData &data )
{
	float lifetime = random->RandomFloat( 0.4f, 0.75f );

	// Ground splash
	FX_AddQuad( data.m_vOrigin, 
				data.m_vNormal, 
				data.m_flRadius, 
				data.m_flRadius * 4.0f,
				0.85f, 
				1.0f,
				0.0f,
				0.25f,
				random->RandomInt( 0, 360 ), 
				random->RandomFloat( -4, 4 ), 
				Vector( 1.0f, 1.0f, 1.0f ), 
				lifetime,
				"effects/combinemuzzle1",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );

	Vector	vRight, vUp;
	VectorVectors( data.m_vNormal, vRight, vUp );

	Vector	start, end;
	
	float radius = data.m_flRadius * 0.15f;

	// Base vertical shaft
	FXLineData_t lineData;

	start = data.m_vOrigin;
	end = start + ( data.m_vNormal * random->RandomFloat( radius*2.0f, radius*4.0f ) );

	lineData.m_flDieTime = lifetime;
	
	lineData.m_flStartAlpha= 1.0f;
	lineData.m_flEndAlpha = 0.0f;
	
	lineData.m_flStartScale = radius*4;
	lineData.m_flEndScale = radius*5; 

	lineData.m_pMaterial = materials->FindMaterial( "effects/ar2ground2", 0, 0 );

	lineData.m_vecStart = start;
	lineData.m_vecStartVelocity = vec3_origin;

	lineData.m_vecEnd = end;
	lineData.m_vecEndVelocity = data.m_vNormal * random->RandomFloat( 200, 350 );

	FX_AddLine( lineData );

	// Inner filler shaft
	start = data.m_vOrigin;
	end = start + ( data.m_vNormal * random->RandomFloat( 16, radius*0.25f ) );

	lineData.m_flDieTime = lifetime - 0.1f;
	
	lineData.m_flStartAlpha= 1.0f;
	lineData.m_flEndAlpha = 0.0f;
	
	lineData.m_flStartScale = radius*2;
	lineData.m_flEndScale = radius*4; 

	lineData.m_pMaterial = materials->FindMaterial( "effects/ar2ground2", 0, 0 );

	lineData.m_vecStart = start;
	lineData.m_vecStartVelocity = vec3_origin;

	lineData.m_vecEnd = end;
	lineData.m_vecEndVelocity = data.m_vNormal * random->RandomFloat( 64, 128 );

	FX_AddLine( lineData );
}

DECLARE_CLIENT_EFFECT( "AR2Explosion", AR2ExplosionCallback );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void AR2ImpactCallback( const CEffectData &data )
{
	FX_AddQuad( data.m_vOrigin, 
				data.m_vNormal, 
				random->RandomFloat( 24, 32 ),
				0,
				0.75f, 
				1.0f,
				0.0f,
				0.4f,
				random->RandomInt( 0, 360 ), 
				0,
				Vector( 1.0f, 1.0f, 1.0f ), 
				0.25f, 
				"effects/combinemuzzle2_nocull",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );
}

DECLARE_CLIENT_EFFECT( "AR2Impact", AR2ImpactCallback );

//-----------------------------------------------------------------------------
// Creates a muzzleflash elight
//-----------------------------------------------------------------------------
void CreateMuzzleflashELight( const Vector &origin, int exponent, int nMinRadius, int nMaxRadius, ClientEntityHandle_t hEntity )
{
	if ( muzzleflash_light.GetInt() )
	{
		int entityIndex = ClientEntityList().HandleToEntIndex( hEntity );
		if ( entityIndex >= 0 )
		{
			dlight_t *el = effects->CL_AllocElight( LIGHT_INDEX_MUZZLEFLASH + entityIndex );

			el->origin	= origin;

			el->color.r = 255;
			el->color.g = 192;
			el->color.b = 64;
			el->color.exponent = exponent;

			el->radius	= random->RandomInt( nMinRadius, nMaxRadius );
			el->decay	= el->radius / 0.05f;
			el->die		= gpGlobals->curtime + 0.1f;
		}
	}
}


//-----------------------------------------------------------------------------
// Airboat muzzle flashes
//-----------------------------------------------------------------------------
void MuzzleFlash_Airboat( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	VPROF_BUDGET( "MuzzleFlash_Airboat", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	CSmartPtr<CLocalSpaceEmitter> pSimple = CLocalSpaceEmitter::Create( "MuzzleFlash", hEntity, attachmentIndex );

	SimpleParticle *pParticle;
	Vector			forward(1,0,0), offset; //NOTENOTE: All coords are in local space

	float flScale = random->RandomFloat( 0.75f, IsXbox() ? 2.0f : 2.5f );

	PMaterialHandle pMuzzle[2];
	pMuzzle[0] = pSimple->GetPMaterial( "effects/combinemuzzle1" );
	pMuzzle[1] = pSimple->GetPMaterial( "effects/combinemuzzle2" );

	// Flash
	for ( int i = 1; i < 7; i++ )
	{
		offset = (forward * (i*6.0f*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pMuzzle[random->RandomInt(0,1)], offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= IsXbox() ? 0.0001f : 0.01f;

		pParticle->m_vecVelocity.Init();

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 128;

		pParticle->m_uchStartSize	= ( (random->RandomFloat( 6.0f, 8.0f ) * (9-(i))/7) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	// Tack on the smoke
	pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "sprites/ar2_muzzle1" ), vec3_origin );
		
	if ( pParticle == NULL )
		return;

	pParticle->m_flLifetime		= 0.0f;
	pParticle->m_flDieTime		= 0.05f;

	pParticle->m_vecVelocity.Init();

	pParticle->m_uchColor[0]	= 255;
	pParticle->m_uchColor[1]	= 255;
	pParticle->m_uchColor[2]	= 255;

	pParticle->m_uchStartAlpha	= 255;
	pParticle->m_uchEndAlpha	= 128;

	pParticle->m_uchStartSize	= random->RandomFloat( 16.0f, 24.0f );
	pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
	
	float spokePos = random->RandomInt( 0, 5 );

	pParticle->m_flRoll			= (360.0/6.0f)*spokePos;
	pParticle->m_flRollDelta	= 0.0f;
	
#ifndef _XBOX
	// Grab the origin out of the transform for the attachment
	if ( muzzleflash_light.GetInt() )
	{
		// If the client hasn't seen this entity yet, bail.
		matrix3x4_t	matAttachment;
		if ( FX_GetAttachmentTransform( hEntity, attachmentIndex, matAttachment ) )
		{
			Vector		origin;
			MatrixGetColumn( matAttachment, 3, &origin );
			CreateMuzzleflashELight( origin, 5, 64, 128, hEntity );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AirboatMuzzleFlashCallback( const CEffectData &data )
{
	MuzzleFlash_Airboat( data.m_hEntity, data.m_nAttachmentIndex );
}

DECLARE_CLIENT_EFFECT( "AirboatMuzzleFlash", AirboatMuzzleFlashCallback );


//-----------------------------------------------------------------------------
// Chopper muzzle flashes
//-----------------------------------------------------------------------------
void MuzzleFlash_Chopper( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	VPROF_BUDGET( "MuzzleFlash_Chopper", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	matrix3x4_t	matAttachment;
	// If the client hasn't seen this entity yet, bail.
	if ( !FX_GetAttachmentTransform( hEntity, attachmentIndex, matAttachment ) )
		return;
	
	CSmartPtr<CLocalSpaceEmitter> pSimple = CLocalSpaceEmitter::Create( "MuzzleFlash", hEntity, attachmentIndex );

	SimpleParticle *pParticle;
	Vector			forward(1,0,0), offset; //NOTENOTE: All coords are in local space

	float flScale = random->RandomFloat( 2.5f, 4.5f );

	// Flash
	for ( int i = 1; i < 7; i++ )
	{
		offset = (forward * (i*2.0f*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/combinemuzzle%d", random->RandomInt(1,2) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= random->RandomFloat( 0.05f, 0.1f );

		pParticle->m_vecVelocity.Init();

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 128;

		pParticle->m_uchStartSize	= ( (random->RandomFloat( 6.0f, 8.0f ) * (10-(i))/7) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}
	
	// Grab the origin out of the transform for the attachment
	Vector		origin;
	MatrixGetColumn( matAttachment, 3, &origin );	
	CreateMuzzleflashELight( origin, 6, 128, 256, hEntity );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ChopperMuzzleFlashCallback( const CEffectData &data )
{
	MuzzleFlash_Chopper( data.m_hEntity, data.m_nAttachmentIndex );
}

DECLARE_CLIENT_EFFECT( "ChopperMuzzleFlash", ChopperMuzzleFlashCallback );


//-----------------------------------------------------------------------------
// Gunship muzzle flashes
//-----------------------------------------------------------------------------
void MuzzleFlash_Gunship( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	VPROF_BUDGET( "MuzzleFlash_Gunship", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	// If the client hasn't seen this entity yet, bail.
	matrix3x4_t	matAttachment;
	if ( !FX_GetAttachmentTransform( hEntity, attachmentIndex, matAttachment ) )
		return;

	CSmartPtr<CLocalSpaceEmitter> pSimple = CLocalSpaceEmitter::Create( "MuzzleFlash", hEntity, attachmentIndex );

	SimpleParticle *pParticle;
	Vector			forward(1,0,0), offset; //NOTENOTE: All coords are in local space

	float flScale = random->RandomFloat( 2.5f, 4.5f );

	// Flash
	offset = (forward * (2.0f*flScale));

	pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "effects/gunshipmuzzle" ), offset );
	if ( pParticle == NULL )
		return;

	pParticle->m_flLifetime		= 0.0f;
	pParticle->m_flDieTime		= random->RandomFloat( 0.05f, 0.1f );

	pParticle->m_vecVelocity.Init();

	pParticle->m_uchColor[0]	= 255;
	pParticle->m_uchColor[1]	= 255;
	pParticle->m_uchColor[2]	= 255;

	pParticle->m_uchStartAlpha	= 255;
	pParticle->m_uchEndAlpha	= 128;

	pParticle->m_uchStartSize	= ( (random->RandomFloat( 6.0f, 8.0f ) * 10.0/7.0) * flScale );
	pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
	pParticle->m_flRoll			= random->RandomInt( 0, 360 );
	pParticle->m_flRollDelta	= 0.0f;
	
	// Grab the origin out of the transform for the attachment
	Vector		origin;
	MatrixGetColumn( matAttachment, 3, &origin );	
	CreateMuzzleflashELight( origin, 6, 128, 256, hEntity );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GunshipMuzzleFlashCallback( const CEffectData &data )
{
	MuzzleFlash_Gunship( data.m_hEntity, data.m_nAttachmentIndex );
}

DECLARE_CLIENT_EFFECT( "GunshipMuzzleFlash", GunshipMuzzleFlashCallback );


//-----------------------------------------------------------------------------
// Hunter muzzle flashes
//-----------------------------------------------------------------------------
void MuzzleFlash_Hunter( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	VPROF_BUDGET( "MuzzleFlash_Hunter", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	// If the client hasn't seen this entity yet, bail.
	matrix3x4_t	matAttachment;
	if ( !FX_GetAttachmentTransform( hEntity, attachmentIndex, matAttachment ) )
		return;

	// Grab the origin out of the transform for the attachment
	Vector		origin;
	MatrixGetColumn( matAttachment, 3, &origin );	
	
	dlight_t *el = effects->CL_AllocElight( LIGHT_INDEX_MUZZLEFLASH );
	el->origin = origin;// + Vector( 12.0f, 0, 0 );

	el->color.r = 50;
	el->color.g = 222;
	el->color.b = 213;
	el->color.exponent = 5;

	el->radius = random->RandomInt( 120, 200 );
	el->decay = el->radius / 0.05f;
	el->die = gpGlobals->curtime + 0.05f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HunterMuzzleFlashCallback( const CEffectData &data )
{
	MuzzleFlash_Hunter( data.m_hEntity, data.m_nAttachmentIndex );
}

DECLARE_CLIENT_EFFECT( "HunterMuzzleFlash", HunterMuzzleFlashCallback );
