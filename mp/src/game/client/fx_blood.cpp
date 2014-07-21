//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A blood spray effect, like a big exit wound, used when people are
//			violently impaled, skewered, eviscerated, etc.
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
#include "engine/ivdebugoverlay.h"
#include "shareddefs.h"
#include "fx.h"
#include "fx_blood.h"
#include "effect_color_tables.h"
#include "particle_simple3d.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectBloodSpray )
CLIENTEFFECT_MATERIAL( "effects/blood_core" )
CLIENTEFFECT_MATERIAL( "effects/blood_gore" )
CLIENTEFFECT_MATERIAL( "effects/blood_drop" )
CLIENTEFFECT_MATERIAL( "effects/blood_puff" )
CLIENTEFFECT_REGISTER_END()

// Cached material handles
PMaterialHandle g_Blood_Core = NULL;
PMaterialHandle g_Blood_Gore = NULL;
PMaterialHandle g_Blood_Drops = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bloodtype - 
//			r - 
//			g - 
//			b - 
//-----------------------------------------------------------------------------
void GetBloodColor( int bloodtype, colorentry_t &color )
{
	int i;

	for( i = 0 ; i < COLOR_TABLE_SIZE( bloodcolors ) ; i++ )
	{
		if( bloodcolors[i].index == bloodtype )
		{
			color = bloodcolors[ i ];
			return;
		}
	}

	// build a ridiculous default color
	color.r = 255;
	color.g = 0;
	color.b = 255;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&normal - 
//			scale - 
//			r - 
//			g - 
//			b - 
//			flags - 
//-----------------------------------------------------------------------------
void FX_BloodSpray( const Vector &origin, const Vector &normal, float scale, unsigned char r, unsigned char g, unsigned char b, int flags )
{
	if ( UTIL_IsLowViolence() )
		return;

	//debugoverlay->AddLineOverlay( origin, origin + normal * 72, 255, 255, 255, true, 10 ); 

	Vector offset;
	float spread	= 0.2f;
	
	//Find area ambient light color and use it to tint smoke
	Vector worldLight = WorldGetLightForPoint( origin, true );
	Vector color = Vector( (float)(worldLight[0] * r) / 255.0f, (float)(worldLight[1] * g) / 255.0f, (float)(worldLight[2] * b) / 255.0f );
	float colorRamp;

	int i;

	Vector	offDir;

	Vector right;
	Vector up;

	if (normal != Vector(0, 0, 1) )
	{
		right = normal.Cross( Vector(0, 0, 1) );
		up = right.Cross( normal );
	}
	else
	{
		right = Vector(0, 0, 1);
		up = right.Cross( normal );
	}

	//
	// Dump out drops
	//
	if (flags & FX_BLOODSPRAY_DROPS)
	{
		TrailParticle *tParticle;

		CSmartPtr<CTrailParticles> pTrailEmitter = CTrailParticles::Create( "blooddrops" );
		if ( !pTrailEmitter )
			return;

		pTrailEmitter->SetSortOrigin( origin );

		// Partial gravity on blood drops.
		pTrailEmitter->SetGravity( 600.0 ); 

		pTrailEmitter->GetBinding().SetBBox( origin - Vector( 32, 32, 32 ), origin + Vector( 32, 32, 32 ) );
		pTrailEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
		pTrailEmitter->SetVelocityDampen( 0.2f );

		PMaterialHandle	hMaterial = ParticleMgr()->GetPMaterial( "effects/blood_drop" );

		//
		// Long stringy drops of blood.
		//
		for ( i = 0; i < 14; i++ )
		{
			// Originate from within a circle 'scale' inches in diameter.
			offset = origin;
			offset += right * random->RandomFloat( -0.5f, 0.5f ) * scale;
			offset += up * random->RandomFloat( -0.5f, 0.5f ) * scale;

			tParticle = (TrailParticle *) pTrailEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

			if ( tParticle == NULL )
				break;

			tParticle->m_flLifetime	= 0.0f;

			offDir = normal + RandomVector( -0.3f, 0.3f );

			tParticle->m_vecVelocity = offDir * random->RandomFloat( 4.0f * scale, 40.0f * scale );
			tParticle->m_vecVelocity[2] += random->RandomFloat( 4.0f, 16.0f ) * scale;

			tParticle->m_flWidth		= random->RandomFloat( 0.125f, 0.275f ) * scale;
			tParticle->m_flLength		= random->RandomFloat( 0.02f, 0.03f ) * scale;
			tParticle->m_flDieTime		= random->RandomFloat( 0.5f, 1.0f );

			FloatToColor32( tParticle->m_color, color[0], color[1], color[2], 1.0f );
		}

		//
		// Shorter droplets.
		//
		for ( i = 0; i < 24; i++ )
		{
			// Originate from within a circle 'scale' inches in diameter.
			offset = origin;
			offset += right * random->RandomFloat( -0.5f, 0.5f ) * scale;
			offset += up * random->RandomFloat( -0.5f, 0.5f ) * scale;

			tParticle = (TrailParticle *) pTrailEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

			if ( tParticle == NULL )
				break;

			tParticle->m_flLifetime	= 0.0f;

			offDir = normal + RandomVector( -1.0f, 1.0f );
			offDir[2] += random->RandomFloat(0, 1.0f);

			tParticle->m_vecVelocity = offDir * random->RandomFloat( 2.0f * scale, 25.0f * scale );
			tParticle->m_vecVelocity[2] += random->RandomFloat( 4.0f, 16.0f ) * scale;

			tParticle->m_flWidth		= random->RandomFloat( 0.25f, 0.375f ) * scale;
			tParticle->m_flLength		= random->RandomFloat( 0.0025f, 0.005f ) * scale;
			tParticle->m_flDieTime		= random->RandomFloat( 0.5f, 1.0f );

			FloatToColor32( tParticle->m_color, color[0], color[1], color[2], 1.0f );
		}
	}

	if ((flags & FX_BLOODSPRAY_GORE) || (flags & FX_BLOODSPRAY_CLOUD))
	{
		CSmartPtr<CBloodSprayEmitter> pSimple = CBloodSprayEmitter::Create( "bloodgore" );
		if ( !pSimple )
			return;

		pSimple->SetSortOrigin( origin );
		pSimple->SetGravity( 0 );

		PMaterialHandle	hMaterial;

		//
		// Tight blossom of blood at the center.
		//
		if (flags & FX_BLOODSPRAY_GORE)
		{
			hMaterial = ParticleMgr()->GetPMaterial( "effects/blood_gore" );

			SimpleParticle *pParticle;

			for ( i = 0; i < 6; i++ )
			{
				// Originate from within a circle 'scale' inches in diameter.
				offset = origin + ( 0.5 * scale * normal );
				offset += right * random->RandomFloat( -0.5f, 0.5f ) * scale;
				offset += up * random->RandomFloat( -0.5f, 0.5f ) * scale;

				pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, offset );

				if ( pParticle != NULL )
				{
					pParticle->m_flLifetime = 0.0f;
					pParticle->m_flDieTime	= 0.3f;

					spread = 0.2f;
					pParticle->m_vecVelocity.Random( -spread, spread );
					pParticle->m_vecVelocity += normal * random->RandomInt( 10, 100 );
					//VectorNormalize( pParticle->m_vecVelocity );

					colorRamp = random->RandomFloat( 0.75f, 1.25f );

					pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
					pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
					pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;
					
					pParticle->m_uchStartSize	= random->RandomFloat( scale * 0.25, scale );
					pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 2;
					
					pParticle->m_uchStartAlpha	= random->RandomInt( 200, 255 );
					pParticle->m_uchEndAlpha	= 0;
					
					pParticle->m_flRoll			= random->RandomInt( 0, 360 );
					pParticle->m_flRollDelta	= 0.0f;
				}
			}
		}

		//
		// Diffuse cloud just in front of the exit wound.
		//
		if (flags & FX_BLOODSPRAY_CLOUD)
		{
			hMaterial = ParticleMgr()->GetPMaterial( "effects/blood_puff" );

			SimpleParticle *pParticle;

			for ( i = 0; i < 6; i++ )
			{
				// Originate from within a circle '2 * scale' inches in diameter.
				offset = origin + ( scale * normal );
				offset += right * random->RandomFloat( -1, 1 ) * scale;
				offset += up * random->RandomFloat( -1, 1 ) * scale;

				pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, offset );

				if ( pParticle != NULL )
				{
					pParticle->m_flLifetime = 0.0f;
					pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 0.8f);

					spread = 0.5f;
					pParticle->m_vecVelocity.Random( -spread, spread );
					pParticle->m_vecVelocity += normal * random->RandomInt( 100, 200 );

					colorRamp = random->RandomFloat( 0.75f, 1.25f );

					pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
					pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
					pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;
					
					pParticle->m_uchStartSize	= random->RandomFloat( scale * 1.5f, scale * 2.0f );
					pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4;
					
					pParticle->m_uchStartAlpha	= random->RandomInt( 80, 128 );
					pParticle->m_uchEndAlpha	= 0;
					
					pParticle->m_flRoll			= random->RandomInt( 0, 360 );
					pParticle->m_flRollDelta	= 0.0f;
				}
			}
		}
	}

	// TODO: Play a sound?
	//CLocalPlayerFilter filter;
	//C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, CHAN_VOICE, "Physics.WaterSplash", 1.0, ATTN_NORM, 0, 100, &origin );
}

//-----------------------------------------------------------------------------
// Purpose: Used for bullets hitting bleeding surfaces
// Input  : origin - 
//			normal - 
//			scale - This parameter is not currently used
//-----------------------------------------------------------------------------
void FX_BloodBulletImpact( const Vector &origin, const Vector &normal, float scale /*NOTE: Unused!*/, unsigned char r, unsigned char g, unsigned char b )
{
	if ( UTIL_IsLowViolence() )
		return;

	Vector offset;
	
	//Find area ambient light color and use it to tint smoke
	Vector worldLight = WorldGetLightForPoint( origin, true );
	
	if ( gpGlobals->maxClients > 1 )
	{
		worldLight = Vector( 1.0, 1.0, 1.0 );
		r = 96;
		g = 0;
		b = 10;
	}

	Vector color = Vector( (float)(worldLight[0] * r) / 255.0f, (float)(worldLight[1] * g) / 255.0f, (float)(worldLight[2] * b) / 255.0f );
	float colorRamp;

	Vector	offDir;

	CSmartPtr<CBloodSprayEmitter> pSimple = CBloodSprayEmitter::Create( "bloodgore" );
	if ( !pSimple )
		return;

	pSimple->SetSortOrigin( origin );
	pSimple->SetGravity( 200 );
	
	// Setup a bounding box to contain the particles without (stops auto-updating)
	pSimple->GetBinding().SetBBox( origin - Vector( 16, 16, 16 ), origin + Vector( 16, 16, 16 ) );

	// Cache the material if we haven't already
	if ( g_Blood_Core == NULL )
	{
		g_Blood_Core = ParticleMgr()->GetPMaterial( "effects/blood_core" );
	}

	SimpleParticle *pParticle;

	Vector	dir = normal * RandomVector( -0.5f, 0.5f );

	offset = origin + ( 2.0f * normal );

	pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Blood_Core, offset );

	if ( pParticle != NULL )
	{
		pParticle->m_flLifetime = 0.0f;
		pParticle->m_flDieTime	= random->RandomFloat( 0.25f, 0.5f);

		pParticle->m_vecVelocity	= dir * random->RandomFloat( 16.0f, 32.0f );
		pParticle->m_vecVelocity[2] -= random->RandomFloat( 8.0f, 16.0f );

		colorRamp = random->RandomFloat( 0.75f, 2.0f );

		pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;
		
		pParticle->m_uchStartSize	= random->RandomInt( 2, 4 );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 8;
	
		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 0;
		
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	// Cache the material if we haven't already
	if ( g_Blood_Gore == NULL )
	{
		g_Blood_Gore = ParticleMgr()->GetPMaterial( "effects/blood_gore" );
	}

	for ( int i = 0; i < 4; i++ )
	{
		offset = origin + ( 2.0f * normal );

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Blood_Gore, offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 0.75f);

			pParticle->m_vecVelocity	= dir * random->RandomFloat( 16.0f, 32.0f )*(i+1);
			pParticle->m_vecVelocity[2] -= random->RandomFloat( 32.0f, 64.0f )*(i+1);

			colorRamp = random->RandomFloat( 0.75f, 2.0f );

			pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;
			
			pParticle->m_uchStartSize	= random->RandomInt( 2, 4 );
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4;
		
			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= 0.0f;
		}
	}

	//
	// Dump out drops
	//
	TrailParticle *tParticle;

	CSmartPtr<CTrailParticles> pTrailEmitter = CTrailParticles::Create( "blooddrops" );
	if ( !pTrailEmitter )
		return;

	pTrailEmitter->SetSortOrigin( origin );

	// Partial gravity on blood drops
	pTrailEmitter->SetGravity( 400.0 ); 
	
	// Enable simple collisions with nearby surfaces
	pTrailEmitter->Setup(origin, &normal, 1, 10, 100, 400, 0.2, 0 );

	if ( g_Blood_Drops == NULL )
	{
		g_Blood_Drops = ParticleMgr()->GetPMaterial( "effects/blood_drop" );
	}

	//
	// Shorter droplets
	//
	for ( int i = 0; i < 8; i++ )
	{
		// Originate from within a circle 'scale' inches in diameter
		offset = origin;

		tParticle = (TrailParticle *) pTrailEmitter->AddParticle( sizeof(TrailParticle), g_Blood_Drops, offset );

		if ( tParticle == NULL )
			break;

		tParticle->m_flLifetime	= 0.0f;

		offDir = RandomVector( -1.0f, 1.0f );

		tParticle->m_vecVelocity = offDir * random->RandomFloat( 64.0f, 128.0f );

		tParticle->m_flWidth		= random->RandomFloat( 0.5f, 2.0f );
		tParticle->m_flLength		= random->RandomFloat( 0.05f, 0.15f );
		tParticle->m_flDieTime		= random->RandomFloat( 0.25f, 0.5f );

		FloatToColor32( tParticle->m_color, color[0], color[1], color[2], 1.0f );
	}

	// TODO: Play a sound?
	//CLocalPlayerFilter filter;
	//C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, CHAN_VOICE, "Physics.WaterSplash", 1.0, ATTN_NORM, 0, 100, &origin );
}

// FIXME: This will be simplified when the initializer can take color parameters as an input
//	      For now, we use different systems

struct ParticleForBlood_t
{
	int nColor;
	const char *lpszParticleSystemName;
};

ParticleForBlood_t	bloodCallbacks[] =
{
	{ BLOOD_COLOR_RED,		"blood_impact_red_01" },
	{ BLOOD_COLOR_GREEN,	"blood_impact_green_01" },
	{ BLOOD_COLOR_YELLOW,	"blood_impact_yellow_01" },
#if defined( HL2_EPISODIC )
	{ BLOOD_COLOR_ANTLION,			"blood_impact_antlion_01" },		// FIXME: Move to Base HL2
	{ BLOOD_COLOR_ZOMBIE,			"blood_impact_zombie_01" },			// FIXME: Move to Base HL2
	{ BLOOD_COLOR_ANTLION_WORKER,	"blood_impact_antlion_worker_01" },
#endif // HL2_EPISODIC
};

//-----------------------------------------------------------------------------
// Purpose: Intercepts the blood spray message.
//-----------------------------------------------------------------------------
void BloodSprayCallback( const CEffectData &data )
{
	colorentry_t color;

	GetBloodColor( data.m_nColor, color );
	FX_BloodSpray( data.m_vOrigin, data.m_vNormal, data.m_flScale, color.r, color.g, color.b, data.m_fFlags );
}

DECLARE_CLIENT_EFFECT( "bloodspray", BloodSprayCallback );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void BloodImpactCallback( const CEffectData & data )
{
	bool bFoundBlood = false;

	// Find which sort of blood we are
	for ( int i = 0; i < ARRAYSIZE( bloodCallbacks ); i++ )
	{
		if ( bloodCallbacks[i].nColor == data.m_nColor )
		{
			QAngle	vecAngles;
			VectorAngles( -data.m_vNormal, vecAngles );
			DispatchParticleEffect( bloodCallbacks[i].lpszParticleSystemName, data.m_vOrigin, vecAngles );
			bFoundBlood = true;
			break;
		}
	}

	if ( bFoundBlood == false )
	{
		Vector vecPosition;
		vecPosition = data.m_vOrigin;
		
		// Fetch the blood color.
		colorentry_t color;
		GetBloodColor( data.m_nColor, color );

		FX_BloodBulletImpact( vecPosition, data.m_vNormal, data.m_flScale, color.r, color.g, color.b );
	}
}

DECLARE_CLIENT_EFFECT( "BloodImpact", BloodImpactCallback );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void HunterDamageCallback( const CEffectData &data )
{
	CSmartPtr<CSimple3DEmitter> pGlassEmitter = CSimple3DEmitter::Create( "HunterDamage" );
	if ( pGlassEmitter == NULL )
		return;

	pGlassEmitter->SetSortOrigin( data.m_vOrigin );

	// Handle increased scale
	const float flMaxSpeed = 400.0f;
	const float flMinSpeed = 50.0f;
	float flAngularSpray = 1.0f;

	// Setup our collision information
	pGlassEmitter->m_ParticleCollision.Setup( data.m_vOrigin, &data.m_vNormal, flAngularSpray, flMinSpeed, flMaxSpeed, 600.0f, 0.2f );

	Vector	dir, end;

	int	numFlecks = 32;

	Particle3D *pFleckParticle;
	Vector spawnOffset;

	//Dump out flecks
	for ( int i = 0; i < numFlecks; i++ )
	{
		spawnOffset = data.m_vOrigin + RandomVector( -32.0f, 32.0f );
		pFleckParticle = (Particle3D *) pGlassEmitter->AddParticle( sizeof(Particle3D), g_Mat_Fleck_Antlion[random->RandomInt(0,1)], spawnOffset );

		if ( pFleckParticle == NULL )
			break;

		pFleckParticle->m_flLifeRemaining	= random->RandomFloat( 2.0f, 3.0f );

		dir[0] = data.m_vNormal[0] + random->RandomFloat( -flAngularSpray, flAngularSpray );
		dir[1] = data.m_vNormal[1] + random->RandomFloat( -flAngularSpray, flAngularSpray );
		dir[2] = data.m_vNormal[2] + random->RandomFloat( -flAngularSpray, flAngularSpray );

		pFleckParticle->m_uchSize		= random->RandomInt( 3, 8 );

		pFleckParticle->m_vecVelocity	= dir * random->RandomFloat( flMinSpeed, flMaxSpeed);

		pFleckParticle->m_vAngles		= RandomAngle( 0, 360 );
		pFleckParticle->m_flAngSpeed	= random->RandomFloat( -800, 800 );

		unsigned char color = 255;
		pFleckParticle->m_uchFrontColor[0]	= color;
		pFleckParticle->m_uchFrontColor[1]	= color;
		pFleckParticle->m_uchFrontColor[2]	= color;
		pFleckParticle->m_uchBackColor[0]	= color * 0.25f;
		pFleckParticle->m_uchBackColor[1]	= color * 0.25f;
		pFleckParticle->m_uchBackColor[2]	= color * 0.25f;
	}
}

DECLARE_CLIENT_EFFECT( "HunterDamage", HunterDamageCallback );
