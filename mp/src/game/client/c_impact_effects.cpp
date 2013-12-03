//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "fx.h"
#include "fx_sparks.h"
#include "clienteffectprecachesystem.h"
#include "particle_simple3d.h"
#include "decals.h"
#include "engine/IEngineSound.h"
#include "c_te_particlesystem.h"
#include "engine/ivmodelinfo.h"
#include "particles_ez.h"
#include "c_impact_effects.h"
#include "engine/IStaticPropMgr.h"
#include "tier0/vprof.h"
#include "c_te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Precahce the effects
CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectImpacts )
CLIENTEFFECT_MATERIAL( "effects/fleck_cement1" )
CLIENTEFFECT_MATERIAL( "effects/fleck_cement2" )
CLIENTEFFECT_MATERIAL( "effects/fleck_antlion1" )
CLIENTEFFECT_MATERIAL( "effects/fleck_antlion2" )
CLIENTEFFECT_MATERIAL( "effects/fleck_wood1" )
CLIENTEFFECT_MATERIAL( "effects/fleck_wood2" )
CLIENTEFFECT_MATERIAL( "effects/blood" )
CLIENTEFFECT_MATERIAL( "effects/blood2" )
CLIENTEFFECT_MATERIAL( "sprites/bloodspray" )
CLIENTEFFECT_MATERIAL( "particle/particle_noisesphere" )
CLIENTEFFECT_REGISTER_END()

// Cached handles to commonly used materials
PMaterialHandle g_Mat_Fleck_Wood[2] = { NULL, NULL };
PMaterialHandle g_Mat_Fleck_Cement[2] = { NULL, NULL };
PMaterialHandle g_Mat_Fleck_Antlion[2] = { NULL, NULL };
PMaterialHandle g_Mat_Fleck_Glass[2] = { NULL, NULL };
PMaterialHandle g_Mat_Fleck_Tile[2] = { NULL, NULL };
PMaterialHandle g_Mat_DustPuff[2] = { NULL, NULL };
PMaterialHandle g_Mat_BloodPuff[2] = { NULL, NULL };
PMaterialHandle g_Mat_SMG_Muzzleflash[4] = { NULL, NULL, NULL, NULL };
PMaterialHandle g_Mat_Combine_Muzzleflash[3] = { NULL, NULL, NULL };

static ConVar fx_drawimpactdebris( "fx_drawimpactdebris", "1", FCVAR_DEVELOPMENTONLY, "Draw impact debris effects." );
static ConVar fx_drawimpactdust( "fx_drawimpactdust", "1", FCVAR_DEVELOPMENTONLY, "Draw impact dust effects." );

void FX_CacheMaterialHandles( void )
{
	g_Mat_Fleck_Wood[0] = ParticleMgr()->GetPMaterial( "effects/fleck_wood1" );
	g_Mat_Fleck_Wood[1] = ParticleMgr()->GetPMaterial( "effects/fleck_wood2" );

	g_Mat_Fleck_Cement[0] = ParticleMgr()->GetPMaterial( "effects/fleck_cement1");
	g_Mat_Fleck_Cement[1] = ParticleMgr()->GetPMaterial( "effects/fleck_cement2" );

	g_Mat_Fleck_Antlion[0] = ParticleMgr()->GetPMaterial( "effects/fleck_antlion1" );
	g_Mat_Fleck_Antlion[1] = ParticleMgr()->GetPMaterial( "effects/fleck_antlion2" );

	g_Mat_Fleck_Glass[0] = ParticleMgr()->GetPMaterial( "effects/fleck_glass1" );
	g_Mat_Fleck_Glass[1] = ParticleMgr()->GetPMaterial( "effects/fleck_glass2" );

	g_Mat_Fleck_Tile[0] = ParticleMgr()->GetPMaterial( "effects/fleck_tile1" );
	g_Mat_Fleck_Tile[1] = ParticleMgr()->GetPMaterial( "effects/fleck_tile2" );

	g_Mat_DustPuff[0] = ParticleMgr()->GetPMaterial( "particle/particle_smokegrenade" );
	g_Mat_DustPuff[1] = ParticleMgr()->GetPMaterial( "particle/particle_noisesphere" );

	g_Mat_BloodPuff[0] = ParticleMgr()->GetPMaterial( "effects/blood" );
	g_Mat_BloodPuff[1] = ParticleMgr()->GetPMaterial( "effects/blood2" );
	
#ifndef TF_CLIENT_DLL
	g_Mat_SMG_Muzzleflash[0] = ParticleMgr()->GetPMaterial( "effects/muzzleflash1" );
	g_Mat_SMG_Muzzleflash[1] = ParticleMgr()->GetPMaterial( "effects/muzzleflash2" );
	g_Mat_SMG_Muzzleflash[2] = ParticleMgr()->GetPMaterial( "effects/muzzleflash3" );
	g_Mat_SMG_Muzzleflash[3] = ParticleMgr()->GetPMaterial( "effects/muzzleflash4" );
#ifndef CSTRIKE_DLL
	g_Mat_Combine_Muzzleflash[0] = ParticleMgr()->GetPMaterial( "effects/combinemuzzle1" );
	g_Mat_Combine_Muzzleflash[1] = ParticleMgr()->GetPMaterial( "effects/combinemuzzle2" );
	g_Mat_Combine_Muzzleflash[2] = ParticleMgr()->GetPMaterial( "effects/strider_muzzle" );
#endif
#endif
}

extern PMaterialHandle g_Material_Spark;

//-----------------------------------------------------------------------------
// Purpose: Returns the color given trace information
// Input  : *trace - trace to get results for
//			*color - return color, gamma corrected (0.0f to 1.0f)
//-----------------------------------------------------------------------------
void GetColorForSurface( trace_t *trace, Vector *color )
{
	Vector	baseColor, diffuseColor;
	Vector	end = trace->startpos + ( ( Vector )trace->endpos - ( Vector )trace->startpos ) * 1.1f;
	
	if ( trace->DidHitWorld() )
	{
		if ( trace->hitbox == 0 )
		{
			// If we hit the world, then ask the world for the fleck color
			engine->TraceLineMaterialAndLighting( trace->startpos, end, diffuseColor, baseColor );
		}
		else
		{
			// In this case we hit a static prop.
			staticpropmgr->GetStaticPropMaterialColorAndLighting( trace, trace->hitbox - 1, diffuseColor, baseColor );
		}
	}
	else
	{
		// In this case, we hit an entity. Find out the model associated with it
		C_BaseEntity *pEnt = trace->m_pEnt;
		if ( !pEnt )
		{
			Msg("Couldn't find surface in GetColorForSurface()\n");
			color->x = 255;
			color->y = 255;
			color->z = 255;
			return;
		}

		ICollideable *pCollide = pEnt->GetCollideable();
		int modelIndex = pCollide->GetCollisionModelIndex();
		model_t* pModel = const_cast<model_t*>(modelinfo->GetModel( modelIndex ));

		// Ask the model info about what we need to know
		modelinfo->GetModelMaterialColorAndLighting( pModel, pCollide->GetCollisionOrigin(),
			pCollide->GetCollisionAngles(), trace, diffuseColor, baseColor );
	}

	//Get final light value
	color->x = pow( diffuseColor[0], 1.0f/2.2f ) * baseColor[0];
	color->y = pow( diffuseColor[1], 1.0f/2.2f ) * baseColor[1];
	color->z = pow( diffuseColor[2], 1.0f/2.2f ) * baseColor[2];
}


//-----------------------------------------------------------------------------
// This does the actual debris flecks
//-----------------------------------------------------------------------------
#define	FLECK_MIN_SPEED		64.0f
#define	FLECK_MAX_SPEED		128.0f
#define	FLECK_GRAVITY		800.0f
#define	FLECK_DAMPEN		0.3f
#define	FLECK_ANGULAR_SPRAY	0.6f

#ifndef _XBOX

//
// PC ONLY!
//

static void CreateFleckParticles( const Vector& origin, const Vector &color, trace_t *trace, char materialType, int iScale )
{
	Vector	spawnOffset	= trace->endpos + ( trace->plane.normal * 1.0f );

	CSmartPtr<CFleckParticles> fleckEmitter = CFleckParticles::Create( "FX_DebrisFlecks", spawnOffset, Vector(5,5,5) );

	if ( !fleckEmitter )
		return;

	// Handle increased scale
	float flMaxSpeed = FLECK_MAX_SPEED * iScale;
	float flAngularSpray = MAX( 0.2, FLECK_ANGULAR_SPRAY - ( (float)iScale * 0.2f) ); // More power makes the spray more controlled
	// Setup our collision information
	fleckEmitter->m_ParticleCollision.Setup( spawnOffset, &trace->plane.normal, flAngularSpray, FLECK_MIN_SPEED, flMaxSpeed, FLECK_GRAVITY, FLECK_DAMPEN );

	PMaterialHandle	*hMaterial;
	switch ( materialType )
	{
	case CHAR_TEX_WOOD:
		hMaterial = g_Mat_Fleck_Wood;
		break;

	case CHAR_TEX_CONCRETE:
	case CHAR_TEX_TILE:
	default:
		hMaterial = g_Mat_Fleck_Cement;
		break;
	}

	Vector	dir, end;

	float	colorRamp;

	float fScale = g_pParticleSystemMgr->ParticleThrottleScaling() * (float)iScale;
	int	numFlecks = (int)( 0.5f + fScale * (float)( random->RandomInt( 4, 16 ) ) );

	FleckParticle	*pFleckParticle;

	//Dump out flecks
	int i;
	for ( i = 0; i < numFlecks; i++ )
	{
		pFleckParticle = (FleckParticle *) fleckEmitter->AddParticle( sizeof(FleckParticle), hMaterial[random->RandomInt(0,1)], spawnOffset );

		if ( pFleckParticle == NULL )
			break;

		pFleckParticle->m_flLifetime	= 0.0f;
		pFleckParticle->m_flDieTime		= 3.0f;

		dir[0] = trace->plane.normal[0] + random->RandomFloat( -flAngularSpray, flAngularSpray );
		dir[1] = trace->plane.normal[1] + random->RandomFloat( -flAngularSpray, flAngularSpray );
		dir[2] = trace->plane.normal[2] + random->RandomFloat( -flAngularSpray, flAngularSpray );

		pFleckParticle->m_uchSize		= random->RandomInt( 1, 2 );

		pFleckParticle->m_vecVelocity	= dir * ( random->RandomFloat( FLECK_MIN_SPEED, flMaxSpeed) * ( 3 - pFleckParticle->m_uchSize ) );

		pFleckParticle->m_flRoll		= random->RandomFloat( 0, 360 );
		pFleckParticle->m_flRollDelta	= random->RandomFloat( 0, 360 );

		colorRamp = random->RandomFloat( 0.75f, 1.25f );

		pFleckParticle->m_uchColor[0] = MIN( 1.0f, color[0]*colorRamp )*255.0f;
		pFleckParticle->m_uchColor[1] = MIN( 1.0f, color[1]*colorRamp )*255.0f;
		pFleckParticle->m_uchColor[2] = MIN( 1.0f, color[2]*colorRamp )*255.0f;
	}
}

#endif // _XBOX

//-----------------------------------------------------------------------------
// Purpose: Debris flecks caused by impacts
// Input  : origin - start
//			*trace - trace information
//			*materialName - material hit
//			materialType - type of material hit
//-----------------------------------------------------------------------------
void FX_DebrisFlecks( const Vector& origin, trace_t *tr, char materialType, int iScale, bool bNoFlecks )
{
	VPROF_BUDGET( "FX_DebrisFlecks", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	if ( !fx_drawimpactdebris.GetBool() )
		return;

#ifdef _XBOX

	//
	// XBox version
	//

	Vector	offset;
	float	spread = 0.2f;

	CSmartPtr<CDustParticle> pSimple = CDustParticle::Create( "dust" );
	pSimple->SetSortOrigin( origin );
	
	// Lock the bbox
	pSimple->GetBinding().SetBBox( origin - ( Vector( 16, 16, 16 ) * iScale ), origin + ( Vector( 16, 16, 16 ) * iScale ) );

	// Get the color of the surface we've impacted
	Vector	color;
	float	colorRamp;
	GetColorForSurface( tr, &color );

	int i;
	SimpleParticle	*pParticle;
	for ( i = 0; i < 4; i++ )
	{
		if ( i == 3 )
		{
			pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_BloodPuff[0], origin );
		}
		else
		{
			pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_DustPuff[0], origin );
		}

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );

			pParticle->m_vecVelocity.Random( -spread, spread );
			pParticle->m_vecVelocity += ( tr->plane.normal * random->RandomFloat( 1.0f, 6.0f ) );

			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 250, 500 ) * i * 0.5f;

			// scaled
			pParticle->m_vecVelocity *= fForce * iScale;

			// Ramp the color
			colorRamp = random->RandomFloat( 0.5f, 1.25f );
			pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;

			// scaled
			pParticle->m_uchStartSize	= (iScale*0.5f) * random->RandomInt( 3, 4 ) * (i+1);

			// scaled
			pParticle->m_uchEndSize		= (iScale*0.5f) * pParticle->m_uchStartSize * 4;

			pParticle->m_uchStartAlpha	= random->RandomInt( 200, 255 );
			pParticle->m_uchEndAlpha	= 0;

			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -1.0f, 1.0f );
		}
	}			

	// Covers the impact spot with flecks
	pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_DustPuff2, origin );

	if ( pParticle != NULL )
	{
		offset = origin;
		offset[0] += random->RandomFloat( -8.0f, 8.0f );
		offset[1] += random->RandomFloat( -8.0f, 8.0f );

		pParticle->m_flLifetime = 0.0f;
		pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );

		spread = 1.0f;

		pParticle->m_vecVelocity.Init();

		colorRamp = random->RandomFloat( 0.5f, 1.25f );

		pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;

		pParticle->m_uchStartSize	= random->RandomInt( 4, 8 );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4;

		pParticle->m_uchStartAlpha	= random->RandomInt( 64, 128 );
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -0.1f, 0.1f );
	}

#else

	//
	// PC version
	//

	Vector	color;
	GetColorForSurface( tr, &color );

	if ( !bNoFlecks )
	{
		CreateFleckParticles( origin, color, tr, materialType, iScale );
	}

	//
	// Dust trail
	//
	Vector	offset = tr->endpos + ( tr->plane.normal * 2.0f );

	SimpleParticle newParticle;

	int i;
	for ( i = 0; i < 2; i++ )
	{
		newParticle.m_Pos = offset;

		newParticle.m_flLifetime	= 0.0f;
		newParticle.m_flDieTime	= 1.0f;

		Vector dir;
		dir[0] = tr->plane.normal[0] + random->RandomFloat( -0.8f, 0.8f );
		dir[1] = tr->plane.normal[1] + random->RandomFloat( -0.8f, 0.8f );
		dir[2] = tr->plane.normal[2] + random->RandomFloat( -0.8f, 0.8f );

		newParticle.m_uchStartSize	= random->RandomInt( 2, 4 ) * iScale;
		newParticle.m_uchEndSize	= newParticle.m_uchStartSize * 8 * iScale;

		newParticle.m_vecVelocity = dir * random->RandomFloat( 2.0f, 24.0f )*(i+1);
		newParticle.m_vecVelocity[2] -= random->RandomFloat( 8.0f, 32.0f )*(i+1);

		newParticle.m_uchStartAlpha	= random->RandomInt( 100, 200 );
		newParticle.m_uchEndAlpha	= 0;

		newParticle.m_flRoll			= random->RandomFloat( 0, 360 );
		newParticle.m_flRollDelta	= random->RandomFloat( -1, 1 );

		float colorRamp = random->RandomFloat( 0.5f, 1.25f );

		newParticle.m_uchColor[0] = MIN( 1.0f, color[0]*colorRamp )*255.0f;
		newParticle.m_uchColor[1] = MIN( 1.0f, color[1]*colorRamp )*255.0f;
		newParticle.m_uchColor[2] = MIN( 1.0f, color[2]*colorRamp )*255.0f;

		AddSimpleParticle( &newParticle, g_Mat_DustPuff[0] );
	}


	for ( i = 0; i < 4; i++ )
	{
		newParticle.m_Pos = offset;

		newParticle.m_flLifetime	= 0.0f;
		newParticle.m_flDieTime	= random->RandomFloat( 0.25f, 0.5f );

		Vector dir;
		dir[0] = tr->plane.normal[0] + random->RandomFloat( -0.8f, 0.8f );
		dir[1] = tr->plane.normal[1] + random->RandomFloat( -0.8f, 0.8f );
		dir[2] = tr->plane.normal[2] + random->RandomFloat( -0.8f, 0.8f );

		newParticle.m_uchStartSize	= random->RandomInt( 1, 4 );
		newParticle.m_uchEndSize	= newParticle.m_uchStartSize * 4;

		newParticle.m_vecVelocity = dir * random->RandomFloat( 8.0f, 32.0f );
		newParticle.m_vecVelocity[2] -= random->RandomFloat( 8.0f, 64.0f );

		newParticle.m_uchStartAlpha	= 255;
		newParticle.m_uchEndAlpha	= 0;

		newParticle.m_flRoll			= random->RandomFloat( 0, 360 );
		newParticle.m_flRollDelta	= random->RandomFloat( -2.0f, 2.0f );

		float colorRamp = random->RandomFloat( 0.5f, 1.25f );

		newParticle.m_uchColor[0] = MIN( 1.0f, color[0]*colorRamp )*255.0f;
		newParticle.m_uchColor[1] = MIN( 1.0f, color[1]*colorRamp )*255.0f;
		newParticle.m_uchColor[2] = MIN( 1.0f, color[2]*colorRamp )*255.0f;

		AddSimpleParticle( &newParticle, g_Mat_BloodPuff[0] );
	}

	//
	// Bullet hole capper
	//
	newParticle.m_Pos = offset;

	newParticle.m_flLifetime		= 0.0f;
	newParticle.m_flDieTime		= random->RandomFloat( 1.0f, 1.5f );

	Vector dir;
	dir[0] = tr->plane.normal[0] + random->RandomFloat( -0.8f, 0.8f );
	dir[1] = tr->plane.normal[1] + random->RandomFloat( -0.8f, 0.8f );
	dir[2] = tr->plane.normal[2] + random->RandomFloat( -0.8f, 0.8f );

	newParticle.m_uchStartSize	= random->RandomInt( 4, 8 );
	newParticle.m_uchEndSize		= newParticle.m_uchStartSize * 4.0f;

	newParticle.m_vecVelocity = dir * random->RandomFloat( 2.0f, 24.0f );
	newParticle.m_vecVelocity[2] = random->RandomFloat( -2.0f, 2.0f );

	newParticle.m_uchStartAlpha	= random->RandomInt( 100, 200 );
	newParticle.m_uchEndAlpha	= 0;

	newParticle.m_flRoll			= random->RandomFloat( 0, 360 );
	newParticle.m_flRollDelta	= random->RandomFloat( -2, 2 );

	float colorRamp = random->RandomFloat( 0.5f, 1.25f );

	newParticle.m_uchColor[0] = MIN( 1.0f, color[0]*colorRamp )*255.0f;
	newParticle.m_uchColor[1] = MIN( 1.0f, color[1]*colorRamp )*255.0f;
	newParticle.m_uchColor[2] = MIN( 1.0f, color[2]*colorRamp )*255.0f;

	AddSimpleParticle( &newParticle, g_Mat_DustPuff[0] );

#endif
}

#define	GLASS_SHARD_MIN_LIFE	2.5f
#define	GLASS_SHARD_MAX_LIFE	5.0f
#define	GLASS_SHARD_NOISE		0.8
#define	GLASS_SHARD_GRAVITY		800
#define	GLASS_SHARD_DAMPING		0.3
#define	GLASS_SHARD_MIN_SPEED	1
#define	GLASS_SHARD_MAX_SPEED	300

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FX_GlassImpact( const Vector &pos, const Vector &normal )
{
	VPROF_BUDGET( "FX_GlassImpact", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	CSmartPtr<CSimple3DEmitter> pGlassEmitter = CSimple3DEmitter::Create( "FX_GlassImpact" );
	pGlassEmitter->SetSortOrigin( pos );

	Vector vecColor;
	engine->ComputeLighting( pos, NULL, true, vecColor );

	// HACK: Blend a little toward white to match the materials...
	VectorLerp( vecColor, Vector( 1, 1, 1 ), 0.3, vecColor );

	float flShardSize	= random->RandomFloat( 2.0f, 6.0f );

	unsigned char color[3] = { 200, 200, 210 };

	// ---------------------
	// Create glass shards
	// ----------------------

	int numShards = random->RandomInt( 2, 4 );

	for ( int i = 0; i < numShards; i++ )
	{
		Particle3D *pParticle;
		
		pParticle = (Particle3D *) pGlassEmitter->AddParticle( sizeof(Particle3D), g_Mat_Fleck_Glass[random->RandomInt(0,1)], pos );

		if ( pParticle )
		{
			pParticle->m_flLifeRemaining	= random->RandomFloat(GLASS_SHARD_MIN_LIFE,GLASS_SHARD_MAX_LIFE);

			pParticle->m_vecVelocity[0]		= ( normal[0] + random->RandomFloat( -0.8f, 0.8f ) ) * random->RandomFloat( GLASS_SHARD_MIN_SPEED, GLASS_SHARD_MAX_SPEED );
			pParticle->m_vecVelocity[1]		= ( normal[1] + random->RandomFloat( -0.8f, 0.8f ) ) * random->RandomFloat( GLASS_SHARD_MIN_SPEED, GLASS_SHARD_MAX_SPEED );
			pParticle->m_vecVelocity[2]		= ( normal[2] + random->RandomFloat( -0.8f, 0.8f ) ) * random->RandomFloat( GLASS_SHARD_MIN_SPEED, GLASS_SHARD_MAX_SPEED );

			pParticle->m_uchSize			= flShardSize + random->RandomFloat(-0.5*flShardSize,0.5*flShardSize);
			pParticle->m_vAngles			= RandomAngle( 0, 360 );
			pParticle->m_flAngSpeed			= random->RandomFloat(-800,800);

			pParticle->m_uchFrontColor[0]	= (byte)(color[0] * vecColor.x);
			pParticle->m_uchFrontColor[1]	= (byte)(color[1] * vecColor.y);
			pParticle->m_uchFrontColor[2]	= (byte)(color[2] * vecColor.z);
			pParticle->m_uchBackColor[0]	= (byte)(color[0] * vecColor.x);
			pParticle->m_uchBackColor[1]	= (byte)(color[1] * vecColor.y);
			pParticle->m_uchBackColor[2]	= (byte)(color[2] * vecColor.z);
		}
	}

	pGlassEmitter->m_ParticleCollision.Setup( pos, &normal, GLASS_SHARD_NOISE, GLASS_SHARD_MIN_SPEED, GLASS_SHARD_MAX_SPEED, GLASS_SHARD_GRAVITY, GLASS_SHARD_DAMPING );

	color[0] = 64;
	color[1] = 64;
	color[2] = 92;

	// ---------------------------
	// Dust
	// ---------------------------

	Vector	dir;
	Vector	offset = pos + ( normal * 2.0f );
	float	colorRamp;

	SimpleParticle newParticle;

	for ( int i = 0; i < 4; i++ )
	{
		newParticle.m_Pos = offset;

		newParticle.m_flLifetime= 0.0f;
		newParticle.m_flDieTime	= random->RandomFloat( 0.1f, 0.25f );
		
		dir[0] = normal[0] + random->RandomFloat( -0.8f, 0.8f );
		dir[1] = normal[1] + random->RandomFloat( -0.8f, 0.8f );
		dir[2] = normal[2] + random->RandomFloat( -0.8f, 0.8f );

		newParticle.m_uchStartSize	= random->RandomInt( 1, 4 );
		newParticle.m_uchEndSize	= newParticle.m_uchStartSize * 8;

		newParticle.m_vecVelocity	= dir * random->RandomFloat( 8.0f, 16.0f )*(i+1);
		newParticle.m_vecVelocity[2] -= random->RandomFloat( 16.0f, 32.0f )*(i+1);

		newParticle.m_uchStartAlpha	= random->RandomInt( 128, 255 );
		newParticle.m_uchEndAlpha	= 0;
		
		newParticle.m_flRoll		= random->RandomFloat( 0, 360 );
		newParticle.m_flRollDelta	= random->RandomFloat( -1, 1 );

		colorRamp = random->RandomFloat( 0.5f, 1.25f );

		newParticle.m_uchColor[0] = MIN( 1.0f, color[0]*colorRamp )*255.0f;
		newParticle.m_uchColor[1] = MIN( 1.0f, color[1]*colorRamp )*255.0f;
		newParticle.m_uchColor[2] = MIN( 1.0f, color[2]*colorRamp )*255.0f;

		AddSimpleParticle( &newParticle, g_Mat_BloodPuff[0] );
	}

	//
	// Bullet hole capper
	//
	newParticle.m_Pos = offset;

	newParticle.m_flLifetime		= 0.0f;
	newParticle.m_flDieTime		= random->RandomFloat( 1.0f, 1.5f );

	dir[0] = normal[0] + random->RandomFloat( -0.8f, 0.8f );
	dir[1] = normal[1] + random->RandomFloat( -0.8f, 0.8f );
	dir[2] = normal[2] + random->RandomFloat( -0.8f, 0.8f );

	newParticle.m_uchStartSize	= random->RandomInt( 4, 8 );
	newParticle.m_uchEndSize		= newParticle.m_uchStartSize * 4.0f;

	newParticle.m_vecVelocity = dir * random->RandomFloat( 2.0f, 8.0f );
	newParticle.m_vecVelocity[2] = random->RandomFloat( -2.0f, 2.0f );

	newParticle.m_uchStartAlpha	= random->RandomInt( 32, 64 );
	newParticle.m_uchEndAlpha	= 0;
	
	newParticle.m_flRoll			= random->RandomFloat( 0, 360 );
	newParticle.m_flRollDelta	= random->RandomFloat( -2, 2 );

	colorRamp = random->RandomFloat( 0.5f, 1.25f );

	newParticle.m_uchColor[0] = MIN( 1.0f, color[0]*colorRamp )*255.0f;
	newParticle.m_uchColor[1] = MIN( 1.0f, color[1]*colorRamp )*255.0f;
	newParticle.m_uchColor[2] = MIN( 1.0f, color[2]*colorRamp )*255.0f;

	AddSimpleParticle( &newParticle, g_Mat_DustPuff[0] );
}

void GlassImpactCallback( const CEffectData &data )
{
	FX_GlassImpact( data.m_vOrigin, data.m_vNormal );
}

DECLARE_CLIENT_EFFECT( "GlassImpact", GlassImpactCallback );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &pos - 
//			*tr - 
//-----------------------------------------------------------------------------
void FX_AntlionImpact( const Vector &pos, trace_t *trace )
{
#if defined( _X360 )
	return;
#endif // _X360

	VPROF_BUDGET( "FX_AntlionImpact", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	CSmartPtr<CSimple3DEmitter> fleckEmitter = CSimple3DEmitter::Create( "FX_DebrisFlecks" );
	if ( fleckEmitter == NULL )
		return;

	Vector	shotDir = ( trace->startpos - trace->endpos );
	VectorNormalize( shotDir );

	Vector	spawnOffset	= trace->endpos + ( shotDir * 2.0f );

	Vector vWorldMins, vWorldMaxs;
	if ( trace->m_pEnt )
	{
		float scale = trace->m_pEnt->CollisionProp()->BoundingRadius();
		vWorldMins[0] = spawnOffset[0] - scale;
		vWorldMins[1] = spawnOffset[1] - scale;
		vWorldMins[2] = spawnOffset[2] - scale;
		vWorldMaxs[0] = spawnOffset[0] + scale;
		vWorldMaxs[1] = spawnOffset[1] + scale;
		vWorldMaxs[2] = spawnOffset[2] + scale;
	}
	else
	{
		return;
	}

	fleckEmitter->SetSortOrigin( spawnOffset );
	fleckEmitter->GetBinding().SetBBox( spawnOffset-Vector(32,32,32), spawnOffset+Vector(32,32,32), true );

	// Handle increased scale
	float flMaxSpeed = 256.0f;
	float flAngularSpray = 1.0f;

	// Setup our collision information
	fleckEmitter->m_ParticleCollision.Setup( spawnOffset, &shotDir, flAngularSpray, 8.0f, flMaxSpeed, FLECK_GRAVITY, FLECK_DAMPEN );

	Vector	dir, end;
	Vector	color = Vector( 1, 0.9, 0.75 );
	float	colorRamp;

	int	numFlecks = random->RandomInt( 8, 16 );

	Particle3D *pFleckParticle;

	// Dump out flecks
	int i;
	for ( i = 0; i < numFlecks; i++ )
	{
		pFleckParticle = (Particle3D *) fleckEmitter->AddParticle( sizeof(Particle3D), g_Mat_Fleck_Antlion[random->RandomInt(0,1)], spawnOffset );
		if ( pFleckParticle == NULL )
			break;

		pFleckParticle->m_flLifeRemaining = 3.0f;

		dir[0] = shotDir[0] + random->RandomFloat( -flAngularSpray, flAngularSpray );
		dir[1] = shotDir[1] + random->RandomFloat( -flAngularSpray, flAngularSpray );
		dir[2] = shotDir[2] + random->RandomFloat( -flAngularSpray, flAngularSpray );

		pFleckParticle->m_uchSize		= random->RandomInt( 1, 6 );

		pFleckParticle->m_vecVelocity	= dir * random->RandomFloat( FLECK_MIN_SPEED, flMaxSpeed);
		pFleckParticle->m_vAngles.Random( 0, 360 );
		pFleckParticle->m_flAngSpeed	= random->RandomFloat(-800,800);

		pFleckParticle->m_uchFrontColor[0] = 255;
		pFleckParticle->m_uchFrontColor[1] = 255;
		pFleckParticle->m_uchFrontColor[2] = 255;

		pFleckParticle->m_uchBackColor[0] = 128;
		pFleckParticle->m_uchBackColor[1] = 128;
		pFleckParticle->m_uchBackColor[2] = 128;
	}

	//
	// Dust trail
	//

	SimpleParticle	*pParticle;

	CSmartPtr<CSimpleEmitter> dustEmitter = CSimpleEmitter::Create( "FX_DebrisFlecks" );					
	if ( !dustEmitter )
		return;

	Vector	offset = trace->endpos + ( shotDir * 4.0f );

	dustEmitter->SetSortOrigin( offset );
	dustEmitter->GetBinding().SetBBox( spawnOffset-Vector(32,32,32), spawnOffset+Vector(32,32,32), true );

	for ( i = 0; i < 4; i++ )
	{
		pParticle = (SimpleParticle *) dustEmitter->AddParticle( sizeof(SimpleParticle), g_Mat_DustPuff[0], offset );

		if ( pParticle == NULL )
			break;

		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= 1.0f;
		
		dir[0] = shotDir[0] + random->RandomFloat( -0.8f, 0.8f );
		dir[1] = shotDir[1] + random->RandomFloat( -0.8f, 0.8f );
		dir[2] = shotDir[2] + random->RandomFloat( -0.8f, 0.8f );

		pParticle->m_uchStartSize	= random->RandomInt( 8, 16 );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4.0f;

		pParticle->m_vecVelocity = dir * random->RandomFloat( 4.0f, 64.0f );

		pParticle->m_uchStartAlpha	= random->RandomInt( 32, 64);
		pParticle->m_uchEndAlpha	= 0;
		
		pParticle->m_flRoll			= random->RandomFloat( 0, 2.0f*M_PI );
		pParticle->m_flRollDelta	= random->RandomFloat( -0.5f, 0.5f );

		colorRamp = random->RandomFloat( 0.5f, 1.0f );

		pParticle->m_uchColor[0] = MIN( 1.0f, color[0]*colorRamp )*255.0f;
		pParticle->m_uchColor[1] = MIN( 1.0f, color[1]*colorRamp )*255.0f;
		pParticle->m_uchColor[2] = MIN( 1.0f, color[2]*colorRamp )*255.0f;
	}


	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, 0, "FX_AntlionImpact.ShellImpact", &trace->endpos );
}

//-----------------------------------------------------------------------------
// Purpose: Spurt out bug blood
// Input  : &pos - 
//			&dir - 
//-----------------------------------------------------------------------------
#if defined( _XBOX )
#define NUM_BUG_BLOOD	16
#define NUM_BUG_BLOOD2	8
#define NUM_BUG_SPLATS	8
#else
#define NUM_BUG_BLOOD	32
#define NUM_BUG_BLOOD2	16
#define NUM_BUG_SPLATS	16
#endif
void FX_BugBlood( Vector &pos, Vector &dir, Vector &vWorldMins, Vector &vWorldMaxs )
{
	VPROF_BUDGET( "FX_BugBlood", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "FX_BugBlood" );
	if ( !pSimple )
		return;

	pSimple->SetSortOrigin( pos );
	pSimple->GetBinding().SetBBox( vWorldMins, vWorldMaxs, true );
	pSimple->GetBinding().SetBBox( pos-Vector(32,32,32), pos+Vector(32,32,32), true );

	Vector	vDir;
	vDir[0] = dir[0] + random->RandomFloat( -2.0f, 2.0f );
	vDir[1] = dir[1] + random->RandomFloat( -2.0f, 2.0f );
	vDir[2] = dir[2] + random->RandomFloat( -2.0f, 2.0f );

	VectorNormalize( vDir );

	int i;
	for ( i = 0; i < NUM_BUG_BLOOD; i++ )
	{
		SimpleParticle *sParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_BloodPuff[0], pos );
			
		if ( sParticle == NULL )
			return;

		sParticle->m_flLifetime		= 0.0f;
		sParticle->m_flDieTime		= 0.25f;
			
		float	speed = random->RandomFloat( 32.0f, 150.0f );

		sParticle->m_vecVelocity	= vDir * -speed;
		sParticle->m_vecVelocity[2] -= 32.0f;

		sParticle->m_uchColor[0]	= 255;
		sParticle->m_uchColor[1]	= 200;
		sParticle->m_uchColor[2]	= 32;
		sParticle->m_uchStartAlpha	= 255;
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchStartSize	= random->RandomInt( 1, 2 );
		sParticle->m_uchEndSize		= sParticle->m_uchStartSize*random->RandomInt( 1, 4 );
		sParticle->m_flRoll			= random->RandomInt( 0, 360 );
		sParticle->m_flRollDelta	= random->RandomFloat( -2.0f, 2.0f );
	}

	for ( i = 0; i < NUM_BUG_BLOOD2; i++ )
	{
		SimpleParticle *sParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_BloodPuff[1], pos );
			
		if ( sParticle == NULL )
		{
			return;
		}

		sParticle->m_flLifetime		= 0.0f;
		sParticle->m_flDieTime		= random->RandomFloat( 0.25f, 0.5f );
			
		float	speed = random->RandomFloat( 8.0f, 255.0f );

		sParticle->m_vecVelocity	= vDir * -speed;
		sParticle->m_vecVelocity[2] -= 16.0f;

		sParticle->m_uchColor[0]	= 255;
		sParticle->m_uchColor[1]	= 200;
		sParticle->m_uchColor[2]	= 32;
		sParticle->m_uchStartAlpha	= random->RandomInt( 16, 32 );
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchStartSize	= random->RandomInt( 1, 3 );
		sParticle->m_uchEndSize		= sParticle->m_uchStartSize*random->RandomInt( 1, 4 );
		sParticle->m_flRoll			= random->RandomInt( 0, 360 );
		sParticle->m_flRollDelta	= random->RandomFloat( -2.0f, 2.0f );
	}

	Vector	offset;

	for ( i = 0; i < NUM_BUG_SPLATS; i++ )
	{
		offset.Random( -2, 2 );
		offset += pos;

		SimpleParticle *sParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_BloodPuff[1], offset );
			
		if ( sParticle == NULL )
		{
			return;
		}
		
		sParticle->m_flLifetime		= 0.0f;
		sParticle->m_flDieTime		= random->RandomFloat( 0.25f, 0.5f );
			
		float speed = 75.0f * ((i/(float)NUM_BUG_SPLATS)+1);

		sParticle->m_vecVelocity.Random( -16.0f, 16.0f );

		sParticle->m_vecVelocity	+= vDir * -speed;
		sParticle->m_vecVelocity[2] -= ( 64.0f * ((i/(float)NUM_BUG_SPLATS)+1) );

		sParticle->m_uchColor[0]	= 255;
		sParticle->m_uchColor[1]	= 200;
		sParticle->m_uchColor[2]	= 32;
		sParticle->m_uchStartAlpha	= 255;
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchStartSize	= random->RandomInt( 1, 2 );
		sParticle->m_uchEndSize		= sParticle->m_uchStartSize*4;
		sParticle->m_flRoll			= random->RandomInt( 0, 360 );
		sParticle->m_flRollDelta	= random->RandomFloat( -2.0f, 2.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Blood puff
//-----------------------------------------------------------------------------
void FX_Blood( Vector &pos, Vector &dir, float r, float g, float b, float a )
{
	VPROF_BUDGET( "FX_Blood", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	// Cloud
	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "FX_Blood" );
	if ( !pSimple )
		return;
	pSimple->SetSortOrigin( pos );

	Vector	vDir;

	vDir[0] = dir[0] + random->RandomFloat( -1.0f, 1.0f );
	vDir[1] = dir[1] + random->RandomFloat( -1.0f, 1.0f );
	vDir[2] = dir[2] + random->RandomFloat( -1.0f, 1.0f );

	VectorNormalize( vDir );

	int i;
	for ( i = 0; i < 2; i++ )
	{
		SimpleParticle *sParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_BloodPuff[0], pos );
			
		if ( sParticle == NULL )
		{
			return;
		}

		sParticle->m_flLifetime		= 0.0f;
		sParticle->m_flDieTime		= random->RandomFloat( 0.25f, 0.5f );
			
		float	speed = random->RandomFloat( 2.0f, 8.0f );

		sParticle->m_vecVelocity	= vDir * (speed*i);
		sParticle->m_vecVelocity[2] += random->RandomFloat( -32.0f, -16.0f );

		sParticle->m_uchColor[0]	= r;
		sParticle->m_uchColor[1]	= g;
		sParticle->m_uchColor[2]	= b;
		sParticle->m_uchStartAlpha	= a;
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchStartSize	= 2;
		sParticle->m_uchEndSize		= sParticle->m_uchStartSize*4;
		sParticle->m_flRoll			= random->RandomInt( 0, 360 );
		sParticle->m_flRollDelta	= random->RandomFloat( -2.0f, 2.0f );
	}

	for ( i = 0; i < 2; i++ )
	{
		SimpleParticle *sParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_BloodPuff[1], pos );
			
		if ( sParticle == NULL )
		{
			return;
		}

		sParticle->m_flLifetime		= 0.0f;
		sParticle->m_flDieTime		= 0.5f;
			
		float	speed = random->RandomFloat( 4.0f, 16.0f );

		sParticle->m_vecVelocity	= vDir * (speed*i);

		sParticle->m_uchColor[0]	= r;
		sParticle->m_uchColor[1]	= g;
		sParticle->m_uchColor[2]	= b;
		sParticle->m_uchStartAlpha	= 128;
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchStartSize	= 2;
		sParticle->m_uchEndSize		= sParticle->m_uchStartSize*4;
		sParticle->m_flRoll			= random->RandomInt( 0, 360 );
		sParticle->m_flRollDelta	= random->RandomFloat( -4.0f, 4.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Dust impact
// Input  : &origin - position
//			&tr - trace information
//-----------------------------------------------------------------------------
void FX_DustImpact( const Vector &origin, trace_t *tr, int iScale )
{
	if ( !fx_drawimpactdust.GetBool() )
		return;

#ifdef _XBOX

	//
	// XBox version
	//

	VPROF_BUDGET( "FX_DustImpact", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	offset;
	float	spread = 0.2f;

	CSmartPtr<CDustParticle> pSimple = CDustParticle::Create( "dust" );
	pSimple->SetSortOrigin( origin );
	pSimple->GetBinding().SetBBox( origin - ( Vector( 32, 32, 32 ) * iScale ), origin + ( Vector( 32, 32, 32 ) * iScale ) );

	Vector	color;
	float	colorRamp;
	GetColorForSurface( tr, &color );

	int i;
	SimpleParticle *pParticle;
	for ( i = 0; i < 4; i++ )
	{
		// Last puff is gritty (hides end)
		if ( i == 3 )
		{
			pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_BloodPuff[0], origin );
		}
		else
		{
			pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_DustPuff[0], origin );
		}

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;

			pParticle->m_vecVelocity.Random( -spread, spread );
			pParticle->m_vecVelocity += ( tr->plane.normal * random->RandomFloat( 1.0f, 6.0f ) );

			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 250, 500 ) * i;

			// scaled
			pParticle->m_vecVelocity *= fForce * iScale;

			colorRamp = random->RandomFloat( 0.75f, 1.25f );

			pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;

			// scaled
			pParticle->m_uchStartSize	= iScale * random->RandomInt( 3, 4 ) * (i+1);

			// scaled
			pParticle->m_uchEndSize		= iScale * pParticle->m_uchStartSize * 4;

			pParticle->m_uchStartAlpha	= random->RandomInt( 32, 255 );
			pParticle->m_uchEndAlpha	= 0;

			pParticle->m_flRoll			= random->RandomInt( 0, 360 );

			if ( i == 3 )
			{
				pParticle->m_flRollDelta = random->RandomFloat( -0.1f, 0.1f );
				pParticle->m_flDieTime	= 0.5f;
			}
			else
			{
				pParticle->m_flRollDelta = random->RandomFloat( -8.0f, 8.0f );
				pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );
			}
		}
	}			

	//Impact hit
	pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_DustPuff, origin );

	if ( pParticle != NULL )
	{
		offset = origin;
		offset[0] += random->RandomFloat( -8.0f, 8.0f );
		offset[1] += random->RandomFloat( -8.0f, 8.0f );

		pParticle->m_flLifetime = 0.0f;
		pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );

		pParticle->m_vecVelocity.Init();

		colorRamp = random->RandomFloat( 0.75f, 1.25f );
		pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;

		pParticle->m_uchStartSize	= random->RandomInt( 4, 8 );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4;

		pParticle->m_uchStartAlpha	= random->RandomInt( 32, 64 );
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -1.0f, 1.0f );
	}

#else
	FX_DustImpact( origin, tr, (float)iScale );
#endif // _XBOX
}

void FX_DustImpact( const Vector &origin, trace_t *tr, float flScale )
{
	//
	// PC version
	//

	VPROF_BUDGET( "FX_DustImpact", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	offset;
	float	spread = 0.2f;
	
	CSmartPtr<CDustParticle> pSimple = CDustParticle::Create( "dust" );
	pSimple->SetSortOrigin( origin );

	// Three types of particle, ideally we want 4 of each.
	float fNumParticles = 4.0f * g_pParticleSystemMgr->ParticleThrottleScaling();
	int nParticles1 = (int)( 0.50f + fNumParticles );
	int nParticles2 = (int)( 0.83f + fNumParticles );		// <-- most visible particle type.
	int nParticles3 = (int)( 0.17f + fNumParticles );

	SimpleParticle	*pParticle;

	Vector	color;
	float	colorRamp;

	GetColorForSurface( tr, &color );

	// To get a decent spread even when scaling down the number of particles...
	const static int nParticleIdArray[4] = {3,1,2,0};

	int i;
	for ( i = 0; i < nParticles1; i++ )
	{
		int nId = nParticleIdArray[i];

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_DustPuff[0], origin );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );

			pParticle->m_vecVelocity.Random( -spread, spread );
			pParticle->m_vecVelocity += ( tr->plane.normal * random->RandomFloat( 1.0f, 6.0f ) );
			
			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 250, 500 ) * nId;

			// scaled
			pParticle->m_vecVelocity *= fForce * flScale;
			
			colorRamp = random->RandomFloat( 0.75f, 1.25f );

			pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;
			
			// scaled
			pParticle->m_uchStartSize	= ( unsigned char )( flScale * random->RandomInt( 3, 4 ) * (nId+1) );

			// scaled
			pParticle->m_uchEndSize		= ( unsigned char )( flScale * pParticle->m_uchStartSize * 4 );
			
			pParticle->m_uchStartAlpha	= random->RandomInt( 32, 255 );
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
		}
	}			

	//Dust specs
	for ( i = 0; i < nParticles2; i++ )
	{
		int nId = nParticleIdArray[i];

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_BloodPuff[0], origin );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 0.25f, 0.75f );

			pParticle->m_vecVelocity.Random( -spread, spread );
			pParticle->m_vecVelocity += ( tr->plane.normal * random->RandomFloat( 1.0f, 6.0f ) );
			
			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 250, 500 ) * nId;

			pParticle->m_vecVelocity *= fForce;
			
			colorRamp = random->RandomFloat( 0.75f, 1.25f );

			pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;
			
			pParticle->m_uchStartSize	= random->RandomInt( 2, 4 ) * (nId+1);
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 2;
			
			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -2.0f, 2.0f );
		}
	}

	//Impact hit
	for ( i = 0; i < nParticles3; i++ )
	{
		//int nId = nParticleIdArray[i];

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_DustPuff[0], origin );

		if ( pParticle != NULL )
		{
			offset = origin;
			offset[0] += random->RandomFloat( -8.0f, 8.0f );
			offset[1] += random->RandomFloat( -8.0f, 8.0f );

			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );

			spread = 1.0f;

			pParticle->m_vecVelocity.Random( -spread, spread );
			pParticle->m_vecVelocity += tr->plane.normal;
			
			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 0, 50 );

			pParticle->m_vecVelocity *= fForce;
			
			colorRamp = random->RandomFloat( 0.75f, 1.25f );

			pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;
			
			pParticle->m_uchStartSize	= random->RandomInt( 1, 4 );
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4;
			
			pParticle->m_uchStartAlpha	= random->RandomInt( 32, 64 );
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -16.0f, 16.0f );
		}
	}			
}

#ifdef _XBOX
extern PMaterialHandle g_Material_Spark;
#endif // _XBOX

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &pos - 
//			&dir - 
//			type - 
//-----------------------------------------------------------------------------
void FX_GaussExplosion( const Vector &pos, const Vector &dir, int type )
{
	Vector	vDir;

	vDir[0] = dir[0] + random->RandomFloat( -1.0f, 1.0f );
	vDir[1] = dir[1] + random->RandomFloat( -1.0f, 1.0f );
	vDir[2] = dir[2] + random->RandomFloat( -1.0f, 1.0f );

	VectorNormalize( vDir );

	int i;

#if defined(_XBOX) || defined(_X360)

	//
	// XBox version
	//
	CSmartPtr<CTrailParticles> pSparkEmitter = CTrailParticles::Create( "FX_GaussExplosion" );
	if ( pSparkEmitter == NULL )
	{
		Assert(0);
		return;
	}

	if ( g_Material_Spark == NULL )
	{
		g_Material_Spark = pSparkEmitter->GetPMaterial( "effects/spark" );
	}

	pSparkEmitter->SetSortOrigin( pos );
	pSparkEmitter->m_ParticleCollision.SetGravity( 800.0f );
	pSparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
	pSparkEmitter->GetBinding().SetBBox( pos - Vector( 32, 32, 32 ), pos + Vector( 32, 32, 32 ) );

	int numSparks = random->RandomInt( 8, 16 );
	TrailParticle	*pParticle;
	
	// Dump out sparks
	for ( i = 0; i < numSparks; i++ )
	{
		pParticle = (TrailParticle *) pSparkEmitter->AddParticle( sizeof(TrailParticle), g_Material_Spark, pos );

		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime	= 0.0f;

		vDir.Random( -0.6f, 0.6f );
		vDir += dir;
		VectorNormalize( vDir );
		
		pParticle->m_flWidth		= random->RandomFloat( 1.0f, 4.0f );
		pParticle->m_flLength		= random->RandomFloat( 0.01f, 0.1f );
		pParticle->m_flDieTime		= random->RandomFloat( 0.25f, 0.5f );
		
		pParticle->m_vecVelocity	= vDir * random->RandomFloat( 128, 512 );

		Color32Init( pParticle->m_color, 255, 255, 255, 255 );
	}

	// End cap
	SimpleParticle particle;

	particle.m_Pos = pos;
	particle.m_flLifetime = 0.0f;
	particle.m_flDieTime = 0.1f;
	particle.m_vecVelocity.Init();
	particle.m_flRoll = random->RandomInt( 0, 360 );
	particle.m_flRollDelta = 0.0f;
	particle.m_uchColor[0] = 255;
	particle.m_uchColor[1] = 255;
	particle.m_uchColor[2] = 255;
	particle.m_uchStartAlpha = 255;
	particle.m_uchEndAlpha = 255;
	particle.m_uchStartSize = random->RandomInt( 24, 32 );
	particle.m_uchEndSize = 0;

	AddSimpleParticle( &particle, ParticleMgr()->GetPMaterial( "effects/yellowflare" ) );

#else
	
	//
	// PC version
	//
	CSmartPtr<CTrailParticles> pSparkEmitter = CTrailParticles::Create( "FX_ElectricSpark" );

	if ( !pSparkEmitter )
	{
		Assert(0);
		return;
	}

	PMaterialHandle	hMaterial	= pSparkEmitter->GetPMaterial( "effects/spark" );

	pSparkEmitter->SetSortOrigin( pos );

	pSparkEmitter->m_ParticleCollision.SetGravity( 800.0f );
	pSparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN|bitsPARTICLE_TRAIL_COLLIDE );

	//Setup our collision information
	pSparkEmitter->m_ParticleCollision.Setup( pos, &vDir, 0.8f, 128, 512, 800, 0.3f );

	int numSparks = random->RandomInt( 16, 32 );
	TrailParticle	*pParticle;

	// Dump out sparks
	for ( i = 0; i < numSparks; i++ )
	{
		pParticle = (TrailParticle *) pSparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, pos );

		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime	= 0.0f;

		vDir.Random( -0.6f, 0.6f );
		vDir += dir;
		VectorNormalize( vDir );

		pParticle->m_flWidth		= random->RandomFloat( 1.0f, 4.0f );
		pParticle->m_flLength		= random->RandomFloat( 0.01f, 0.1f );
		pParticle->m_flDieTime		= random->RandomFloat( 0.25f, 1.0f );

		pParticle->m_vecVelocity	= vDir * random->RandomFloat( 128, 512 );

		Color32Init( pParticle->m_color, 255, 255, 255, 255 );
	}


	FX_ElectricSpark( pos, 1, 1, &vDir );
#endif
}

class C_TEGaussExplosion : public C_TEParticleSystem
{
public:
	DECLARE_CLASS( C_TEGaussExplosion, C_TEParticleSystem );
	DECLARE_CLIENTCLASS();

				C_TEGaussExplosion();
	virtual		~C_TEGaussExplosion();

public:
	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	virtual	bool	ShouldDraw() { return true; }

public:

	int			m_nType;
	Vector		m_vecDirection;
};

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TEGaussExplosion, DT_TEGaussExplosion, CTEGaussExplosion )
	RecvPropInt(RECVINFO(m_nType)),
	RecvPropVector(RECVINFO(m_vecDirection)),
END_RECV_TABLE()

//==================================================
// C_TEGaussExplosion
//==================================================

C_TEGaussExplosion::C_TEGaussExplosion()
{
}

C_TEGaussExplosion::~C_TEGaussExplosion()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bNewEntity - whether or not to start a new entity
//-----------------------------------------------------------------------------
void C_TEGaussExplosion::PostDataUpdate( DataUpdateType_t updateType )
{
	FX_GaussExplosion( m_vecOrigin, m_vecDirection, m_nType );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : filter - 
//			delay - 
//			&pos - 
//			&dir - 
//			type - 
//-----------------------------------------------------------------------------
void TE_GaussExplosion( IRecipientFilter& filter, float delay, const Vector &pos, const Vector &dir, int type )
{
	FX_GaussExplosion( pos, dir, type );
}

