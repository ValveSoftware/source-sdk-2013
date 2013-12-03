//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "view.h"
#include "viewrender.h"
#include "c_tracer.h"
#include "dlight.h"
#include "clienteffectprecachesystem.h"
#include "fx_sparks.h"
#include "iefx.h"
#include "c_te_effect_dispatch.h"
#include "tier0/vprof.h"
#include "fx_quad.h"
#include "fx.h"
#include "c_pixel_visibility.h"
#include "particles_ez.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Precahce the effects
CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectSparks )
CLIENTEFFECT_MATERIAL( "effects/spark" )
CLIENTEFFECT_MATERIAL( "effects/energysplash" )
CLIENTEFFECT_MATERIAL( "effects/energyball" )
CLIENTEFFECT_MATERIAL( "sprites/rico1" )
CLIENTEFFECT_MATERIAL( "sprites/rico1_noz" )
CLIENTEFFECT_MATERIAL( "sprites/blueflare1" )
CLIENTEFFECT_MATERIAL( "effects/yellowflare" )
CLIENTEFFECT_MATERIAL( "effects/combinemuzzle1_nocull" )
CLIENTEFFECT_MATERIAL( "effects/combinemuzzle2_nocull" )
CLIENTEFFECT_MATERIAL( "effects/yellowflare_noz" )
CLIENTEFFECT_REGISTER_END()

PMaterialHandle g_Material_Spark = NULL;

static ConVar fx_drawmetalspark( "fx_drawmetalspark", "1", FCVAR_DEVELOPMENTONLY, "Draw metal spark effects." );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &pos - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool EffectOccluded( const Vector &pos, pixelvis_handle_t *queryHandle )
{
	if ( !queryHandle )
	{
		// NOTE: This is called by networking code before the current view is set up.
		// so use the main view instead
		trace_t	tr;
		UTIL_TraceLine( pos, MainViewOrigin(), MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );
		
		return ( tr.fraction < 1.0f ) ? true : false;
	}
	pixelvis_queryparams_t params;
	params.Init(pos);
	
	return PixelVisibility_FractionVisible( params, queryHandle ) > 0.0f ? false : true;
}


CSimpleGlowEmitter::CSimpleGlowEmitter( const char *pDebugName, const Vector &sortOrigin, float flDeathTime )
	: CSimpleEmitter( pDebugName )
{
	SetSortOrigin( sortOrigin );
	m_queryHandle = 0;
	m_wasTested = 0;
	m_isVisible = 0;
	m_startTime = gpGlobals->curtime;
	m_flDeathTime = flDeathTime;
}
	
CSimpleGlowEmitter *CSimpleGlowEmitter::Create( const char *pDebugName, const Vector &sortOrigin, float flDeathTime )
{
	return new CSimpleGlowEmitter( pDebugName, sortOrigin, flDeathTime );
}


void CSimpleGlowEmitter::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	if ( gpGlobals->curtime > m_flDeathTime )
	{
		pIterator->RemoveAllParticles();
		return;
	}

	if ( !WasTestedInView(1<<0) )
		return;

	BaseClass::SimulateParticles( pIterator );
}

bool CSimpleGlowEmitter::WasTestedInView( unsigned char viewMask )
{
	return (m_wasTested & viewMask) ? true : false;
}

bool CSimpleGlowEmitter::IsVisibleInView( unsigned char viewMask )
{
	return (m_isVisible & viewMask) ? true : false;
}

void CSimpleGlowEmitter::SetTestedInView( unsigned char viewMask, bool bTested )
{
	m_wasTested &= ~viewMask;
	if ( bTested )
	{
		m_wasTested |= viewMask;
	}
}

void CSimpleGlowEmitter::SetVisibleInView( unsigned char viewMask, bool bVisible )
{
	m_isVisible &= ~viewMask;
	if ( bVisible )
	{
		m_isVisible |= viewMask;
	}
}

unsigned char CSimpleGlowEmitter::CurrentViewMask() const
{
	int viewId = (int)CurrentViewID();
	viewId = clamp(viewId, 0, 7);
	return 1<<viewId;
}

void CSimpleGlowEmitter::RenderParticles( CParticleRenderIterator *pIterator )
{
	unsigned char viewMask = CurrentViewMask();
	if ( !WasTestedInView(CurrentViewMask()) )
	{
		pixelvis_queryparams_t params;
		params.Init(GetSortOrigin());
		
		float visible = PixelVisibility_FractionVisible( params, &m_queryHandle );
		if ( visible == 0.0f )
		{
			if ( (gpGlobals->curtime - m_startTime) <= 0.1f )
				return;
			SetVisibleInView(viewMask, false);
		}
		else
		{
			SetVisibleInView(viewMask, true);
		}

		SetTestedInView(viewMask, true);
	}
	if ( !IsVisibleInView(viewMask) )
		return;

	BaseClass::RenderParticles( pIterator );
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CTrailParticles::CTrailParticles( const char *pDebugName ) : CSimpleEmitter( pDebugName )
{
	m_fFlags			= 0;
	m_flVelocityDampen	= 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Test for surrounding collision surfaces for quick collision testing for the particle system
// Input  : &origin - starting position
//			*dir - direction of movement (if NULL, will do a point emission test in four directions)
//			angularSpread - looseness of the spread
//			minSpeed - minimum speed
//			maxSpeed - maximum speed
//			gravity - particle gravity for the sytem
//			dampen - dampening amount on collisions
//			flags - extra information
//-----------------------------------------------------------------------------
void CTrailParticles::Setup( const Vector &origin, const Vector *direction, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen, int flags, bool bNotCollideable )
{
	//Take the flags
	if ( !bNotCollideable )
	{
		SetFlag( (flags|bitsPARTICLE_TRAIL_COLLIDE) ); //Force this if they've called this function
	}
	else
	{
		SetFlag( flags );
	}

	//See if we've specified a direction
	m_ParticleCollision.Setup( origin, direction, angularSpread, minSpeed, maxSpeed, gravity, dampen );
}


void CTrailParticles::RenderParticles( CParticleRenderIterator *pIterator )
{
	const TrailParticle *pParticle = (const TrailParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		//Get our remaining time
		float lifePerc = 1.0f - ( pParticle->m_flLifetime / pParticle->m_flDieTime  );
		float scale = (pParticle->m_flLength*lifePerc);

		if ( scale < 0.01f )
			scale = 0.01f;

		Vector	start, delta;

		//NOTE: We need to do everything in screen space
		TransformParticle( ParticleMgr()->GetModelView(), pParticle->m_Pos, start );
		float sortKey = start.z;

		Vector3DMultiply( ParticleMgr()->GetModelView(), pParticle->m_vecVelocity, delta );
		
		float	color[4];
		float	ramp = 1.0;

		// Fade in for the first few frames
		if ( pParticle->m_flLifetime <= 0.3 && m_fFlags & bitsPARTICLE_TRAIL_FADE_IN )
		{
			ramp = pParticle->m_flLifetime;
		}
		else if ( m_fFlags & bitsPARTICLE_TRAIL_FADE )
		{
			ramp = ( 1.0f - ( pParticle->m_flLifetime / pParticle->m_flDieTime  ) );
		}

		color[0] = pParticle->m_color.r * ramp * (1.0f / 255.0f);
		color[1] = pParticle->m_color.g * ramp * (1.0f / 255.0f);
		color[2] = pParticle->m_color.b * ramp * (1.0f / 255.0f);
		color[3] = pParticle->m_color.a * ramp * (1.0f / 255.0f);

		float	flLength = (pParticle->m_vecVelocity * scale).Length();//( delta - pos ).Length();
		float	flWidth	 = ( flLength < pParticle->m_flWidth ) ? flLength : pParticle->m_flWidth;

		//See if we should fade
		Vector vecScaledDelta = (delta*scale);
		Tracer_Draw( pIterator->GetParticleDraw(), start, vecScaledDelta, flWidth, color );
		
		pParticle = (const TrailParticle*)pIterator->GetNext( sortKey );
	}
}


void CTrailParticles::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	//Turn off collision if we're not told to do it
	if (( m_fFlags & bitsPARTICLE_TRAIL_COLLIDE )==false)
	{
		m_ParticleCollision.ClearActivePlanes();
	}

	TrailParticle *pParticle = (TrailParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		const float	timeDelta = pIterator->GetTimeDelta();

		//Simulate the movement with collision
		trace_t trace;
		m_ParticleCollision.MoveParticle( pParticle->m_Pos, pParticle->m_vecVelocity, NULL, timeDelta, &trace );

		//Laterally dampen if asked to do so
		if ( m_fFlags & bitsPARTICLE_TRAIL_VELOCITY_DAMPEN )
		{
			float attenuation = 1.0f - (timeDelta * m_flVelocityDampen);

			if ( attenuation < 0.0f )
				attenuation = 0.0f;

			//Laterally dampen
			pParticle->m_vecVelocity[0] *= attenuation;
			pParticle->m_vecVelocity[1] *= attenuation;
			pParticle->m_vecVelocity[2] *= attenuation;
		}

		//Should this particle die?
		pParticle->m_flLifetime += timeDelta;

		if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
			pIterator->RemoveParticle( pParticle );

		pParticle = (TrailParticle*)pIterator->GetNext();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Electric spark
// Input  : &pos - origin point of effect
//-----------------------------------------------------------------------------
#define	SPARK_ELECTRIC_SPREAD	0.0f
#define	SPARK_ELECTRIC_MINSPEED	64.0f
#define	SPARK_ELECTRIC_MAXSPEED	300.0f
#define	SPARK_ELECTRIC_GRAVITY	800.0f
#define	SPARK_ELECTRIC_DAMPEN	0.3f

void FX_ElectricSpark( const Vector &pos, int nMagnitude, int nTrailLength, const Vector *vecDir )
{
	VPROF_BUDGET( "FX_ElectricSpark", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	CSmartPtr<CTrailParticles> pSparkEmitter	= CTrailParticles::Create( "FX_ElectricSpark 1" );

	if ( !pSparkEmitter )
	{
		Assert(0);
		return;
	}

	if ( g_Material_Spark == NULL )
	{
		g_Material_Spark = pSparkEmitter->GetPMaterial( "effects/spark" );
	}

	//Setup our collision information
	pSparkEmitter->Setup( (Vector &) pos, 
							NULL, 
							SPARK_ELECTRIC_SPREAD, 
							SPARK_ELECTRIC_MINSPEED, 
							SPARK_ELECTRIC_MAXSPEED, 
							SPARK_ELECTRIC_GRAVITY, 
							SPARK_ELECTRIC_DAMPEN, 
							bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );

	pSparkEmitter->SetSortOrigin( pos );

	//
	// Big sparks.
	//
	Vector	dir;
	int		numSparks = nMagnitude * nMagnitude * random->RandomFloat( 2, 4 );

	int i;
	TrailParticle	*pParticle;
	for ( i = 0; i < numSparks; i++ )
	{
		pParticle = (TrailParticle *) pSparkEmitter->AddParticle( sizeof(TrailParticle), g_Material_Spark, pos );

		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= nMagnitude * random->RandomFloat( 1.0f, 2.0f );

		dir.Random( -1.0f, 1.0f );
		dir[2] = random->RandomFloat( 0.5f, 1.0f );
	
		if ( vecDir )
		{
			dir += 2 * (*vecDir);
			VectorNormalize( dir );
		}
			
		pParticle->m_flWidth		= random->RandomFloat( 2.0f, 5.0f );
		pParticle->m_flLength		= nTrailLength * random->RandomFloat( 0.02, 0.05f );
		
		pParticle->m_vecVelocity	= dir * random->RandomFloat( SPARK_ELECTRIC_MINSPEED, SPARK_ELECTRIC_MAXSPEED );

		Color32Init( pParticle->m_color, 255, 255, 255, 255 );
	}

#ifdef _XBOX

	//
	// Cap
	//

	SimpleParticle sParticle;

	sParticle.m_Pos = pos;
	sParticle.m_flLifetime		= 0.0f;
	sParticle.m_flDieTime		= 0.2f;

	sParticle.m_vecVelocity.Init();

	sParticle.m_uchColor[0]	= 255;
	sParticle.m_uchColor[1]	= 255;
	sParticle.m_uchColor[2]	= 255;
	sParticle.m_uchStartAlpha	= 255;
	sParticle.m_uchEndAlpha	= 255;
	sParticle.m_uchStartSize	= nMagnitude * random->RandomInt( 4, 8 );
	sParticle.m_uchEndSize		= 0;
	sParticle.m_flRoll			= random->RandomInt( 0, 360 );
	sParticle.m_flRollDelta	= 0.0f;

	AddSimpleParticle( &sParticle, ParticleMgr()->GetPMaterial( "effects/yellowflare" ) );

#else

	//
	// Little sparks
	//
	
	CSmartPtr<CTrailParticles> pSparkEmitter2	= CTrailParticles::Create( "FX_ElectricSpark 2" );

	if ( !pSparkEmitter2 )
	{
		Assert(0);
		return;
	}

	pSparkEmitter2->SetSortOrigin( pos );
	
	pSparkEmitter2->m_ParticleCollision.SetGravity( 400.0f );
	pSparkEmitter2->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );

	numSparks = nMagnitude * random->RandomInt( 16, 32 );

	// Dump out sparks
	for ( i = 0; i < numSparks; i++ )
	{
		pParticle = (TrailParticle *) pSparkEmitter2->AddParticle( sizeof(TrailParticle), g_Material_Spark, pos );

		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime	= 0.0f;

		dir.Random( -1.0f, 1.0f );
		if ( vecDir )
		{
			dir += *vecDir;
			VectorNormalize( dir );
		}
		
		pParticle->m_flWidth		= random->RandomFloat( 2.0f, 4.0f );
		pParticle->m_flLength		= nTrailLength * random->RandomFloat( 0.02f, 0.03f );
		pParticle->m_flDieTime		= nMagnitude * random->RandomFloat( 0.1f, 0.2f );
		
		pParticle->m_vecVelocity	= dir * random->RandomFloat( 128, 256 );

		Color32Init( pParticle->m_color, 255, 255, 255, 255 );
	}

	//
	// Caps
	//
	CSmartPtr<CSimpleGlowEmitter> pSimple = CSimpleGlowEmitter::Create( "FX_ElectricSpark 3", pos, gpGlobals->curtime + 0.2 );

	// NOTE: None of these will render unless the effect is visible!
	//
	// Inner glow
	//
	SimpleParticle *sParticle;

	sParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "effects/yellowflare_noz" ), pos );
		
	if ( sParticle == NULL )
		return;

	sParticle->m_flLifetime		= 0.0f;
	sParticle->m_flDieTime		= 0.2f;
	
	sParticle->m_vecVelocity.Init();

	sParticle->m_uchColor[0]	= 255;
	sParticle->m_uchColor[1]	= 255;
	sParticle->m_uchColor[2]	= 255;
	sParticle->m_uchStartAlpha	= 255;
	sParticle->m_uchEndAlpha	= 255;
	sParticle->m_uchStartSize	= nMagnitude * random->RandomInt( 4, 8 );
	sParticle->m_uchEndSize		= 0;
	sParticle->m_flRoll			= random->RandomInt( 0, 360 );
	sParticle->m_flRollDelta	= 0.0f;
	
	sParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "effects/yellowflare_noz" ), pos );
		
	if ( sParticle == NULL )
		return;

	sParticle->m_flLifetime		= 0.0f;
	sParticle->m_flDieTime		= 0.2f;
	
	sParticle->m_vecVelocity.Init();

	float	fColor = random->RandomInt( 32, 64 );
	sParticle->m_uchColor[0]	= fColor;
	sParticle->m_uchColor[1]	= fColor;
	sParticle->m_uchColor[2]	= fColor;
	sParticle->m_uchStartAlpha	= fColor;
	sParticle->m_uchEndAlpha	= 0;
	sParticle->m_uchStartSize	= nMagnitude * random->RandomInt( 32, 64 );
	sParticle->m_uchEndSize		= 0;
	sParticle->m_flRoll			= random->RandomInt( 0, 360 );
	sParticle->m_flRollDelta	= random->RandomFloat( -1.0f, 1.0f );

	//
	// Smoke
	//
	Vector	sOffs;

	sOffs[0] = pos[0] + random->RandomFloat( -4.0f, 4.0f );
	sOffs[1] = pos[1] + random->RandomFloat( -4.0f, 4.0f );
	sOffs[2] = pos[2];

	sParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_DustPuff[1], sOffs );
		
	if ( sParticle == NULL )
		return;

	sParticle->m_flLifetime		= 0.0f;
	sParticle->m_flDieTime		= 1.0f;
	
	sParticle->m_vecVelocity.Init();
	
	sParticle->m_vecVelocity[2] = 16.0f;
	
	sParticle->m_vecVelocity[0] = random->RandomFloat( -16.0f, 16.0f );
	sParticle->m_vecVelocity[1] = random->RandomFloat( -16.0f, 16.0f );

	sParticle->m_uchColor[0]	= 255;
	sParticle->m_uchColor[1]	= 255;
	sParticle->m_uchColor[2]	= 200;
	sParticle->m_uchStartAlpha	= random->RandomInt( 16, 32 );
	sParticle->m_uchEndAlpha	= 0;
	sParticle->m_uchStartSize	= random->RandomInt( 4, 8 );
	sParticle->m_uchEndSize		= sParticle->m_uchStartSize*4.0f;
	sParticle->m_flRoll			= random->RandomInt( 0, 360 );
	sParticle->m_flRollDelta	= random->RandomFloat( -2.0f, 2.0f );

	//
	// Dlight
	//

	/*
	dlight_t *dl= effects->CL_AllocDlight ( 0 );

	dl->origin	= pos;
	dl->color.r = dl->color.g = dl->color.b = 250;
	dl->radius	= random->RandomFloat(16,32);
	dl->die		= gpGlobals->curtime + 0.001;
	*/

#endif	// !_XBOX
}

//-----------------------------------------------------------------------------
// Purpose: Sparks created by scraping metal
// Input  : &position - start
//			&normal - direction of spark travel
//-----------------------------------------------------------------------------

#define	METAL_SCRAPE_MINSPEED	128.0f
#define METAL_SCRAPE_MAXSPEED	512.0f
#define METAL_SCRAPE_SPREAD		0.3f
#define METAL_SCRAPE_GRAVITY	800.0f
#define METAL_SCRAPE_DAMPEN		0.4f

void FX_MetalScrape( Vector &position, Vector &normal )
{
	VPROF_BUDGET( "FX_MetalScrape", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	offset = position + ( normal * 1.0f );

	CSmartPtr<CTrailParticles> sparkEmitter = CTrailParticles::Create( "FX_MetalScrape 1" );

	if ( !sparkEmitter )
		return;

	sparkEmitter->SetSortOrigin( offset );

	//Setup our collision information
	sparkEmitter->Setup( offset, 
						&normal, 
						METAL_SCRAPE_SPREAD, 
						METAL_SCRAPE_MINSPEED, 
						METAL_SCRAPE_MAXSPEED, 
						METAL_SCRAPE_GRAVITY, 
						METAL_SCRAPE_DAMPEN, 
						bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );

	int	numSparks = random->RandomInt( 4, 8 );
	
	if ( g_Material_Spark == NULL )
	{
		g_Material_Spark = sparkEmitter->GetPMaterial( "effects/spark" );
	}

	Vector	dir;
	TrailParticle *pParticle;
	float	length	= 0.06f;

	//Dump out sparks
	for ( int i = 0; i < numSparks; i++ )
	{
		pParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), g_Material_Spark, offset );

		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime	= 0.0f;

		float	spreadOfs = random->RandomFloat( 0.0f, 2.0f );

		dir[0] = normal[0] + random->RandomFloat( -(METAL_SCRAPE_SPREAD*spreadOfs), (METAL_SCRAPE_SPREAD*spreadOfs) );
		dir[1] = normal[1] + random->RandomFloat( -(METAL_SCRAPE_SPREAD*spreadOfs), (METAL_SCRAPE_SPREAD*spreadOfs) );
		dir[2] = normal[2] + random->RandomFloat( -(METAL_SCRAPE_SPREAD*spreadOfs), (METAL_SCRAPE_SPREAD*spreadOfs) );
	
		pParticle->m_flWidth		= random->RandomFloat( 2.0f, 5.0f );
		pParticle->m_flLength		= random->RandomFloat( length*0.25f, length );
		pParticle->m_flDieTime		= random->RandomFloat( 2.0f, 2.0f );
		
		pParticle->m_vecVelocity	= dir * random->RandomFloat( (METAL_SCRAPE_MINSPEED*(2.0f-spreadOfs)), (METAL_SCRAPE_MAXSPEED*(2.0f-spreadOfs)) );
		
		Color32Init( pParticle->m_color, 255, 255, 255, 255 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Ricochet spark on metal
// Input  : &position - origin of effect
//			&normal - normal of the surface struck
//-----------------------------------------------------------------------------
#define	METAL_SPARK_SPREAD		0.5f
#define	METAL_SPARK_MINSPEED	128.0f
#define	METAL_SPARK_MAXSPEED	512.0f
#define	METAL_SPARK_GRAVITY		400.0f
#define	METAL_SPARK_DAMPEN		0.25f

void FX_MetalSpark( const Vector &position, const Vector &direction, const Vector &surfaceNormal, int iScale )
{
	VPROF_BUDGET( "FX_MetalSpark", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	if ( !fx_drawmetalspark.GetBool() )
		return;

	//
	// Emitted particles
	//

	Vector offset = position + ( surfaceNormal * 1.0f );

	CSmartPtr<CTrailParticles> sparkEmitter = CTrailParticles::Create( "FX_MetalSpark 1" );

	if ( sparkEmitter == NULL )
		return;

	//Setup our information
	sparkEmitter->SetSortOrigin( offset );
	sparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
	sparkEmitter->SetVelocityDampen( 8.0f );
	sparkEmitter->SetGravity( METAL_SPARK_GRAVITY );
	sparkEmitter->SetCollisionDamped( METAL_SPARK_DAMPEN );
	sparkEmitter->GetBinding().SetBBox( offset - Vector( 32, 32, 32 ), offset + Vector( 32, 32, 32 ) );

	int	numSparks = random->RandomInt( 4, 8 ) * ( iScale * 2 );
	numSparks = (int)( 0.5f + (float)numSparks * g_pParticleSystemMgr->ParticleThrottleScaling() );
	
	if ( g_Material_Spark == NULL )
	{
		g_Material_Spark = sparkEmitter->GetPMaterial( "effects/spark" );
	}

	TrailParticle	*pParticle;
	Vector	dir;
	float	length	= 0.1f;

	//Dump out sparks
	for ( int i = 0; i < numSparks; i++ )
	{
		pParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), g_Material_Spark, offset );

		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime	= 0.0f;
		
		if( iScale > 1 && i%3 == 0 )
		{
			// Every third spark goes flying far if we're having a big batch of sparks.
			pParticle->m_flDieTime	= random->RandomFloat( 0.15f, 0.25f );
		}
		else
		{
			pParticle->m_flDieTime	= random->RandomFloat( 0.05f, 0.1f );
		}

		float	spreadOfs = random->RandomFloat( 0.0f, 2.0f );

		dir[0] = direction[0] + random->RandomFloat( -(METAL_SPARK_SPREAD*spreadOfs), (METAL_SPARK_SPREAD*spreadOfs) );
		dir[1] = direction[1] + random->RandomFloat( -(METAL_SPARK_SPREAD*spreadOfs), (METAL_SPARK_SPREAD*spreadOfs) );
		dir[2] = direction[2] + random->RandomFloat( -(METAL_SPARK_SPREAD*spreadOfs), (METAL_SPARK_SPREAD*spreadOfs) );
	
		VectorNormalize( dir );

		pParticle->m_flWidth		= random->RandomFloat( 1.0f, 4.0f );
		pParticle->m_flLength		= random->RandomFloat( length*0.25f, length );
		
		pParticle->m_vecVelocity	= dir * random->RandomFloat( (METAL_SPARK_MINSPEED*(2.0f-spreadOfs)), (METAL_SPARK_MAXSPEED*(2.0f-spreadOfs)) );
		
		Color32Init( pParticle->m_color, 255, 255, 255, 255 );
	}

	//
	// Impact point glow
	//

	FXQuadData_t data;

	data.SetMaterial( "effects/yellowflare" );
	data.SetColor( 1.0f, 1.0f, 1.0f );
	data.SetOrigin( offset );
	data.SetNormal( surfaceNormal );
	data.SetAlpha( 1.0f, 0.0f );
	data.SetLifeTime( 0.1f );
	data.SetYaw( random->RandomInt( 0, 360 ) );
	
	int scale = random->RandomInt( 24, 28 );
	data.SetScale( scale, 0 );

	FX_AddQuad( data );
}

//-----------------------------------------------------------------------------
// Purpose: Spark effect. Nothing but sparks.
// Input  : &pos - origin point of effect
//-----------------------------------------------------------------------------
#define	SPARK_SPREAD	3.0f
#define	SPARK_GRAVITY	800.0f
#define	SPARK_DAMPEN	0.3f

void FX_Sparks( const Vector &pos, int nMagnitude, int nTrailLength, const Vector &vecDir, float flWidth, float flMinSpeed, float flMaxSpeed, char *pSparkMaterial )
{
	VPROF_BUDGET( "FX_Sparks", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	CSmartPtr<CTrailParticles> pSparkEmitter	= CTrailParticles::Create( "FX_Sparks 1" );

	if ( !pSparkEmitter )
	{
		Assert(0);
		return;
	}

	PMaterialHandle hMaterial;
	if ( pSparkMaterial )
	{
		hMaterial = pSparkEmitter->GetPMaterial( pSparkMaterial );
	}
	else
	{
		if ( g_Material_Spark == NULL )
		{
			g_Material_Spark = pSparkEmitter->GetPMaterial( "effects/spark" );
		}

		hMaterial = g_Material_Spark;
	}

	//Setup our collision information
	pSparkEmitter->Setup( (Vector &) pos, 
							NULL, 
							SPARK_SPREAD, 
							flMinSpeed, 
							flMaxSpeed, 
							SPARK_GRAVITY, 
							SPARK_DAMPEN, 
							bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );

	pSparkEmitter->SetSortOrigin( pos );

	//
	// Big sparks.
	//
	Vector	dir;
	int		numSparks = nMagnitude * nMagnitude * random->RandomFloat( 2, 4 );

	int i;
	TrailParticle	*pParticle;
	for ( i = 0; i < numSparks; i++ )
	{
		pParticle = (TrailParticle *) pSparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, pos );

		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= nMagnitude * random->RandomFloat( 1.0f, 2.0f );

		float	spreadOfs = random->RandomFloat( 0.0f, 2.0f );
		dir[0] = vecDir[0] + random->RandomFloat( -(SPARK_SPREAD*spreadOfs), (SPARK_SPREAD*spreadOfs) );
		dir[1] = vecDir[1] + random->RandomFloat( -(SPARK_SPREAD*spreadOfs), (SPARK_SPREAD*spreadOfs) );
		dir[2] = vecDir[2] + random->RandomFloat( -(SPARK_SPREAD*spreadOfs), (SPARK_SPREAD*spreadOfs) );
		pParticle->m_vecVelocity	= dir * random->RandomFloat( (flMinSpeed*(2.0f-spreadOfs)), (flMaxSpeed*(2.0f-spreadOfs)) );
			
		pParticle->m_flWidth		= flWidth + random->RandomFloat( 0.0f, 0.5f );
		pParticle->m_flLength		= nTrailLength * random->RandomFloat( 0.02, 0.05f );
		Color32Init( pParticle->m_color, 255, 255, 255, 255 );
	}

	//
	// Little sparks
	//
	CSmartPtr<CTrailParticles> pSparkEmitter2	= CTrailParticles::Create( "FX_ElectricSpark 2" );

	if ( !pSparkEmitter2 )
	{
		Assert(0);
		return;
	}

	if ( pSparkMaterial )
	{
		hMaterial = pSparkEmitter->GetPMaterial( pSparkMaterial );
	}
	else
	{
		if ( g_Material_Spark == NULL )
		{
			g_Material_Spark = pSparkEmitter2->GetPMaterial( "effects/spark" );
		}
		
		hMaterial = g_Material_Spark;
	}

	pSparkEmitter2->SetSortOrigin( pos );
	
	pSparkEmitter2->m_ParticleCollision.SetGravity( 400.0f );
	pSparkEmitter2->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );

	numSparks = nMagnitude * random->RandomInt( 4, 8 );

	// Dump out sparks
	for ( i = 0; i < numSparks; i++ )
	{
		pParticle = (TrailParticle *) pSparkEmitter2->AddParticle( sizeof(TrailParticle), hMaterial, pos );

		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime	= 0.0f;

		dir.Random( -1.0f, 1.0f );
		dir += vecDir;
		VectorNormalize( dir );
		
		pParticle->m_flWidth		= (flWidth * 0.25) + random->RandomFloat( 0.0f, 0.5f );
		pParticle->m_flLength		= nTrailLength * random->RandomFloat( 0.02f, 0.03f );
		pParticle->m_flDieTime		= nMagnitude * random->RandomFloat( 0.3f, 0.5f );
		
		pParticle->m_vecVelocity	= dir * random->RandomFloat( flMinSpeed, flMaxSpeed );
		
		Color32Init( pParticle->m_color, 255, 255, 255, 255 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Energy splash for plasma/beam weapon impacts
// Input  : &pos - origin point of effect
//-----------------------------------------------------------------------------
#define	ENERGY_SPLASH_SPREAD	0.7f
#define	ENERGY_SPLASH_MINSPEED	128.0f
#define	ENERGY_SPLASH_MAXSPEED	160.0f
#define	ENERGY_SPLASH_GRAVITY	800.0f
#define	ENERGY_SPLASH_DAMPEN	0.3f

void FX_EnergySplash( const Vector &pos, const Vector &normal, int nFlags )
{
	VPROF_BUDGET( "FX_EnergySplash", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	offset = pos + ( normal * 2.0f );

	// Quick flash
	FX_AddQuad( pos,
				normal,
				64.0f,
				0,
				0.75f, 
				1.0f,
				0.0f,
				0.4f,
				random->RandomInt( 0, 360 ), 
				0,
				Vector( 1.0f, 1.0f, 1.0f ), 
				0.25f, 
				"effects/combinemuzzle1_nocull",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );

	// Lingering burn
	FX_AddQuad( pos,
				normal, 
				16,
				32,
				0.75f, 
				1.0f,
				0.0f,
				0.4f,
				random->RandomInt( 0, 360 ), 
				0,
				Vector( 1.0f, 1.0f, 1.0f ), 
				0.5f, 
				"effects/combinemuzzle2_nocull",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );

	SimpleParticle *sParticle;

	CSmartPtr<CSimpleEmitter> pEmitter;

	pEmitter = CSimpleEmitter::Create( "C_EntityDissolve" );
	pEmitter->SetSortOrigin( pos );

	if ( g_Material_Spark == NULL )
	{
		g_Material_Spark = pEmitter->GetPMaterial( "effects/spark" );
	}

	// Anime ground effects
	for ( int j = 0; j < 8; j++ )
	{
		offset.x = random->RandomFloat( -8.0f, 8.0f );
		offset.y = random->RandomFloat( -8.0f, 8.0f );
		offset.z = random->RandomFloat( 0.0f, 4.0f );

		offset += pos;

		sParticle = (SimpleParticle *) pEmitter->AddParticle( sizeof(SimpleParticle), g_Material_Spark, offset );
		
		if ( sParticle == NULL )
			return;

		sParticle->m_vecVelocity = Vector( Helper_RandomFloat( -4.0f, 4.0f ), Helper_RandomFloat( -4.0f, 4.0f ), Helper_RandomFloat( 16.0f, 64.0f ) );
		
		sParticle->m_uchStartSize	= random->RandomFloat( 2, 4 );

		sParticle->m_flDieTime = random->RandomFloat( 0.4f, 0.6f );
		
		sParticle->m_flLifetime		= 0.0f;

		sParticle->m_flRoll			= Helper_RandomInt( 0, 360 );

		float alpha = 255;

		sParticle->m_flRollDelta	= Helper_RandomFloat( -4.0f, 4.0f );
		sParticle->m_uchColor[0]	= alpha;
		sParticle->m_uchColor[1]	= alpha;
		sParticle->m_uchColor[2]	= alpha;
		sParticle->m_uchStartAlpha	= alpha;
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchEndSize		= 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Micro-Explosion effect
// Input  : &position - origin of effect
//			&normal - normal of the surface struck
//-----------------------------------------------------------------------------

#define	MICRO_EXPLOSION_MINSPEED	100.0f
#define MICRO_EXPLOSION_MAXSPEED	150.0f
#define MICRO_EXPLOSION_SPREAD		1.0f
#define MICRO_EXPLOSION_GRAVITY		0.0f
#define MICRO_EXPLOSION_DAMPEN		0.4f

void FX_MicroExplosion( Vector &position, Vector &normal )
{
	VPROF_BUDGET( "FX_MicroExplosion", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	offset = position + ( normal * 2.0f );

	CSmartPtr<CTrailParticles> sparkEmitter = CTrailParticles::Create( "FX_MicroExplosion 1" );

	if ( !sparkEmitter )
		return;

	sparkEmitter->SetSortOrigin( offset );

	//Setup our collision information
	sparkEmitter->Setup( offset, 
						&normal, 
						MICRO_EXPLOSION_SPREAD, 
						MICRO_EXPLOSION_MINSPEED, 
						MICRO_EXPLOSION_MAXSPEED, 
						MICRO_EXPLOSION_GRAVITY, 
						MICRO_EXPLOSION_DAMPEN, 
						bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );

	int	numSparks = random->RandomInt( 8, 16 );
	
	if ( g_Material_Spark == NULL )
	{
		g_Material_Spark = sparkEmitter->GetPMaterial( "effects/spark" );
	}

	TrailParticle	*pParticle;
	Vector	dir, vOfs;
	float	length = 0.2f;

	//Fast lines
	for ( int i = 0; i < numSparks; i++ )
	{
		pParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), g_Material_Spark, offset );

		if ( pParticle )
		{
			pParticle->m_flLifetime	= 0.0f;

			float	ramp = ( (float) i / (float)numSparks );

			dir[0] = normal[0] + random->RandomFloat( -MICRO_EXPLOSION_SPREAD*ramp, MICRO_EXPLOSION_SPREAD*ramp );
			dir[1] = normal[1] + random->RandomFloat( -MICRO_EXPLOSION_SPREAD*ramp, MICRO_EXPLOSION_SPREAD*ramp );
			dir[2] = normal[2] + random->RandomFloat( -MICRO_EXPLOSION_SPREAD*ramp, MICRO_EXPLOSION_SPREAD*ramp );
		
			pParticle->m_flWidth		= random->RandomFloat( 5.0f, 10.0f );
			pParticle->m_flLength		= (length*((1.0f-ramp)*(1.0f-ramp)*0.5f));
			pParticle->m_flDieTime		= 0.2f;
			pParticle->m_vecVelocity	= dir * random->RandomFloat( MICRO_EXPLOSION_MINSPEED*(1.5f-ramp), MICRO_EXPLOSION_MAXSPEED*(1.5f-ramp) );

			Color32Init( pParticle->m_color, 255, 255, 255, 255 );
		}
	}

	//
	// Filler
	//

	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "FX_MicroExplosion 2" );
	pSimple->SetSortOrigin( offset );
	
	SimpleParticle *sParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "sprites/rico1" ), offset );
		
	if ( sParticle )
	{
		sParticle->m_flLifetime		= 0.0f;
		sParticle->m_flDieTime		= 0.3f;
		
		sParticle->m_vecVelocity.Init();

		sParticle->m_uchColor[0]	= 255;
		sParticle->m_uchColor[1]	= 255;
		sParticle->m_uchColor[2]	= 255;
		sParticle->m_uchStartAlpha	= random->RandomInt( 128, 255 );
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchStartSize	= random->RandomInt( 12, 16 );
		sParticle->m_uchEndSize		= sParticle->m_uchStartSize;
		sParticle->m_flRoll			= random->RandomInt( 0, 360 );
		sParticle->m_flRollDelta	= 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Ugly prototype explosion effect
//-----------------------------------------------------------------------------
#define	EXPLOSION_MINSPEED		300.0f
#define EXPLOSION_MAXSPEED		300.0f
#define EXPLOSION_SPREAD		0.8f
#define EXPLOSION_GRAVITY		800.0f
#define EXPLOSION_DAMPEN		0.4f

#define	EXPLOSION_FLECK_MIN_SPEED		150.0f
#define	EXPLOSION_FLECK_MAX_SPEED		350.0f
#define	EXPLOSION_FLECK_GRAVITY			800.0f
#define	EXPLOSION_FLECK_DAMPEN			0.3f
#define	EXPLOSION_FLECK_ANGULAR_SPRAY	0.8f

void FX_Explosion( Vector& origin, Vector& normal, char materialType )
{
	VPROF_BUDGET( "FX_Explosion", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	offset = origin + ( normal * 2.0f );

	CSmartPtr<CTrailParticles> pSparkEmitter = CTrailParticles::Create( "FX_Explosion 1" );
	if ( !pSparkEmitter )
		return;

	// Get color data from our hit point
	IMaterial	*pTraceMaterial;
	Vector		diffuseColor, baseColor;
	pTraceMaterial = engine->TraceLineMaterialAndLighting( origin, normal * -16.0f, diffuseColor, baseColor );
	// Get final light value
	float r = pow( diffuseColor[0], 1.0f/2.2f ) * baseColor[0];
	float g = pow( diffuseColor[1], 1.0f/2.2f ) * baseColor[1];
	float b = pow( diffuseColor[2], 1.0f/2.2f ) * baseColor[2];

	if ( g_Material_Spark == NULL )
	{
		g_Material_Spark = pSparkEmitter->GetPMaterial( "effects/spark" );
	}

	// Setup our collision information
	pSparkEmitter->Setup(	offset, 
							&normal, 
							EXPLOSION_SPREAD, 
							EXPLOSION_MINSPEED, 
							EXPLOSION_MAXSPEED, 
							EXPLOSION_GRAVITY, 
							EXPLOSION_DAMPEN, 
							bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );

	pSparkEmitter->SetSortOrigin( offset );

	Vector	dir;
	int		numSparks = random->RandomInt( 8,16 );

	// Dump out sparks
	int i;
	for ( i = 0; i < numSparks; i++ )
	{
		TrailParticle *pParticle = (TrailParticle *) pSparkEmitter->AddParticle( sizeof(TrailParticle), g_Material_Spark, offset );

		if ( pParticle == NULL )
			break;

		pParticle->m_flLifetime	= 0.0f;

		pParticle->m_flWidth		= random->RandomFloat( 5.0f, 10.0f );
		pParticle->m_flLength		= random->RandomFloat( 0.05, 0.1f );
		pParticle->m_flDieTime		= random->RandomFloat( 1.0f, 2.0f );
		
		dir[0] = normal[0] + random->RandomFloat( -EXPLOSION_SPREAD, EXPLOSION_SPREAD );
		dir[1] = normal[1] + random->RandomFloat( -EXPLOSION_SPREAD, EXPLOSION_SPREAD );
		dir[2] = normal[2] + random->RandomFloat( -EXPLOSION_SPREAD, EXPLOSION_SPREAD );
		pParticle->m_vecVelocity	= dir * random->RandomFloat( EXPLOSION_MINSPEED, EXPLOSION_MAXSPEED );

		Color32Init( pParticle->m_color, 255, 255, 255, 255 );
	}


	// Chunks o'dirt
	// Only create dirt chunks on concrete/world
	if ( materialType == 'C' || materialType == 'W' )
	{
		CSmartPtr<CFleckParticles> fleckEmitter = CFleckParticles::Create( "FX_Explosion 10", offset, Vector(5,5,5) );
		if ( !fleckEmitter )
			return;

		// Setup our collision information
		fleckEmitter->m_ParticleCollision.Setup( offset, &normal, EXPLOSION_FLECK_ANGULAR_SPRAY, EXPLOSION_FLECK_MIN_SPEED, EXPLOSION_FLECK_MAX_SPEED, EXPLOSION_FLECK_GRAVITY, EXPLOSION_FLECK_DAMPEN );

		PMaterialHandle	*hMaterialArray;
		
		switch ( materialType )
		{
		case 'C':
		case 'c':
		default:
			hMaterialArray = g_Mat_Fleck_Cement;
			break;
		}

		int	numFlecks = random->RandomInt( 48, 64 );
		// Dump out flecks
		for ( i = 0; i < numFlecks; i++ )
		{
			FleckParticle *pParticle = (FleckParticle *) fleckEmitter->AddParticle( sizeof(FleckParticle), hMaterialArray[random->RandomInt(0,1)], offset );

			if ( pParticle == NULL )
				break;

			pParticle->m_flLifetime	= 0.0f;
			pParticle->m_flDieTime		= 3.0f;
			dir[0] = normal[0] + random->RandomFloat( -EXPLOSION_FLECK_ANGULAR_SPRAY, EXPLOSION_FLECK_ANGULAR_SPRAY );
			dir[1] = normal[1] + random->RandomFloat( -EXPLOSION_FLECK_ANGULAR_SPRAY, EXPLOSION_FLECK_ANGULAR_SPRAY );
			dir[2] = normal[2] + random->RandomFloat( -EXPLOSION_FLECK_ANGULAR_SPRAY, EXPLOSION_FLECK_ANGULAR_SPRAY );
			pParticle->m_uchSize		= random->RandomInt( 2, 6 );
			pParticle->m_vecVelocity	= dir * ( random->RandomFloat( EXPLOSION_FLECK_MIN_SPEED, EXPLOSION_FLECK_MAX_SPEED ) * ( 7 - pParticle->m_uchSize ) );
			pParticle->m_flRoll		= random->RandomFloat( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( 0, 360 );

			float colorRamp = random->RandomFloat( 0.75f, 1.5f );
			pParticle->m_uchColor[0] = MIN( 1.0f, r*colorRamp )*255.0f;
			pParticle->m_uchColor[1] = MIN( 1.0f, g*colorRamp )*255.0f;
			pParticle->m_uchColor[2] = MIN( 1.0f, b*colorRamp )*255.0f;
		}
	}

	// Large sphere bursts
	CSmartPtr<CSimpleEmitter> pSimpleEmitter = CSimpleEmitter::Create( "FX_Explosion 1" );
	PMaterialHandle	hSphereMaterial = g_Mat_DustPuff[1];
	Vector vecBurstOrigin = offset + normal * 8.0;
	pSimpleEmitter->SetSortOrigin( vecBurstOrigin );
	SimpleParticle *pSphereParticle = (SimpleParticle *) pSimpleEmitter->AddParticle( sizeof(SimpleParticle), hSphereMaterial, vecBurstOrigin );
	if ( pSphereParticle )
	{
		pSphereParticle->m_flLifetime		= 0.0f;
		pSphereParticle->m_flDieTime		= 0.3f;
		pSphereParticle->m_uchStartAlpha	= 150.0;
		pSphereParticle->m_uchEndAlpha		= 64.0;
		pSphereParticle->m_uchStartSize		= 0.0;
		pSphereParticle->m_uchEndSize		= 255.0;
		pSphereParticle->m_vecVelocity		= Vector(0,0,0);

		float colorRamp = random->RandomFloat( 0.75f, 1.5f );
		pSphereParticle->m_uchColor[0] = MIN( 1.0f, r*colorRamp )*255.0f;
		pSphereParticle->m_uchColor[1] = MIN( 1.0f, g*colorRamp )*255.0f;
		pSphereParticle->m_uchColor[2] = MIN( 1.0f, b*colorRamp )*255.0f;
	}

	// Throw some smoke balls out around the normal
	int numBalls = 12;
	Vector vecRight, vecForward, vecUp;
	QAngle vecAngles;
	VectorAngles( normal, vecAngles );
	AngleVectors( vecAngles, NULL, &vecRight, &vecUp );
	for ( i = 0; i < numBalls; i++ )
	{
		SimpleParticle *pParticle = (SimpleParticle *) pSimpleEmitter->AddParticle( sizeof(SimpleParticle), hSphereMaterial, vecBurstOrigin );
		if ( pParticle )
		{
			pParticle->m_flLifetime		= 0.0f;
			pParticle->m_flDieTime		= 0.25f;
			pParticle->m_uchStartAlpha	= 128.0;
			pParticle->m_uchEndAlpha	= 64.0;
			pParticle->m_uchStartSize	= 16.0;
			pParticle->m_uchEndSize		= 64.0;

			float flAngle = ((float)i * M_PI * 2) / numBalls;
			float x = cos( flAngle );
			float y = sin( flAngle );
			pParticle->m_vecVelocity = (vecRight*x + vecUp*y) * 1024.0;

			float colorRamp = random->RandomFloat( 0.75f, 1.5f );
			pParticle->m_uchColor[0] = MIN( 1.0f, r*colorRamp )*255.0f;
			pParticle->m_uchColor[1] = MIN( 1.0f, g*colorRamp )*255.0f;
			pParticle->m_uchColor[2] = MIN( 1.0f, b*colorRamp )*255.0f;
		}
	}

	// Create a couple of big, floating smoke clouds
	CSmartPtr<CSimpleEmitter> pSmokeEmitter = CSimpleEmitter::Create( "FX_Explosion 2" );
	pSmokeEmitter->SetSortOrigin( offset );
	for ( i = 0; i < 2; i++ )
	{
		SimpleParticle *pParticle = (SimpleParticle *) pSmokeEmitter->AddParticle( sizeof(SimpleParticle), g_Mat_DustPuff[1], offset );
		if ( pParticle == NULL )
			break;

		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= random->RandomFloat( 2.0f, 3.0f );
		pParticle->m_uchStartSize	= 64;
		pParticle->m_uchEndSize		= 255;
		dir[0] = normal[0] + random->RandomFloat( -0.8f, 0.8f );
		dir[1] = normal[1] + random->RandomFloat( -0.8f, 0.8f );
		dir[2] = normal[2] + random->RandomFloat( -0.8f, 0.8f );
		pParticle->m_vecVelocity = dir * random->RandomFloat( 2.0f, 24.0f )*(i+1);
		pParticle->m_uchStartAlpha	= 160;
		pParticle->m_uchEndAlpha	= 0;
		pParticle->m_flRoll			= random->RandomFloat( 180, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -1, 1 );

		float colorRamp = random->RandomFloat( 0.5f, 1.25f );
		pParticle->m_uchColor[0] = MIN( 1.0f, r*colorRamp )*255.0f;
		pParticle->m_uchColor[1] = MIN( 1.0f, g*colorRamp )*255.0f;
		pParticle->m_uchColor[2] = MIN( 1.0f, b*colorRamp )*255.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			normal - 
//-----------------------------------------------------------------------------
void FX_ConcussiveExplosion( Vector &origin, Vector &normal )
{
	VPROF_BUDGET( "FX_ConcussiveExplosion", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	offset = origin + ( normal * 2.0f );
	Vector	dir;
	int		i;

	// 
	// Smoke
	//

	CSmartPtr<CSimpleEmitter> pSmokeEmitter = CSimpleEmitter::Create( "FX_ConcussiveExplosion 1" );
	pSmokeEmitter->SetSortOrigin( offset );

	//Quick moving sprites
	 for ( i = 0; i < 16; i++ )
	{
		SimpleParticle *pParticle = (SimpleParticle *) pSmokeEmitter->AddParticle( sizeof(SimpleParticle), g_Mat_DustPuff[1], offset );

		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= random->RandomFloat( 0.2f, 0.4f );
		pParticle->m_uchStartSize	= random->RandomInt( 4, 8 );
		pParticle->m_uchEndSize		= random->RandomInt( 32, 64 );
		
		dir[0] = random->RandomFloat( -1.0f, 1.0f );
		dir[1] = random->RandomFloat( -1.0f, 1.0f );
		dir[2] = random->RandomFloat( -1.0f, 1.0f );
		
		pParticle->m_vecVelocity	= dir * random->RandomFloat( 64.0f, 128.0f );
		pParticle->m_uchStartAlpha	= random->RandomInt( 64, 128 );
		pParticle->m_uchEndAlpha	= 0;
		pParticle->m_flRoll			= random->RandomFloat( 180, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -4, 4 );

		int colorRamp = random->RandomFloat( 235, 255 );
		pParticle->m_uchColor[0] = colorRamp;
		pParticle->m_uchColor[1] = colorRamp;
		pParticle->m_uchColor[2] = colorRamp;
	}

	//Slow lingering sprites
	for ( i = 0; i < 2; i++ )
	{
		SimpleParticle *pParticle = (SimpleParticle *) pSmokeEmitter->AddParticle( sizeof(SimpleParticle), g_Mat_DustPuff[1], offset );
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= random->RandomFloat( 1.0f, 2.0f );
		pParticle->m_uchStartSize	= random->RandomInt( 32, 64 );
		pParticle->m_uchEndSize		= random->RandomInt( 100, 128 );

		dir[0] = normal[0] + random->RandomFloat( -0.8f, 0.8f );
		dir[1] = normal[1] + random->RandomFloat( -0.8f, 0.8f );
		dir[2] = normal[2] + random->RandomFloat( -0.8f, 0.8f );

		pParticle->m_vecVelocity = dir * random->RandomFloat( 16.0f, 32.0f );

		pParticle->m_uchStartAlpha	= random->RandomInt( 32, 64 );
		pParticle->m_uchEndAlpha	= 0;
		pParticle->m_flRoll			= random->RandomFloat( 180, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -1, 1 );

		int colorRamp = random->RandomFloat( 235, 255 );
		pParticle->m_uchColor[0] = colorRamp;
		pParticle->m_uchColor[1] = colorRamp;
		pParticle->m_uchColor[2] = colorRamp;
	}
	

	//	
	// Quick sphere
	//

	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "FX_ConcussiveExplosion 2" );

	pSimple->SetSortOrigin( offset );
	
	SimpleParticle *pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof(SimpleParticle), pSimple->GetPMaterial( "effects/blueflare1" ), offset );
	
	if ( pParticle )
	{
		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= 0.1f;
		pParticle->m_vecVelocity.Init();
		pParticle->m_flRoll			= random->RandomFloat( 180, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -1, 1 );

		pParticle->m_uchColor[0] = pParticle->m_uchColor[1] = pParticle->m_uchColor[2] = 128;
		
		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_uchStartSize	= 16;
		pParticle->m_uchEndSize		= 64;
	}
	
	pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof(SimpleParticle), pSimple->GetPMaterial( "effects/blueflare1" ), offset );
	
	if ( pParticle )
	{
		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= 0.2f;
		pParticle->m_vecVelocity.Init();
		pParticle->m_flRoll		= random->RandomFloat( 180, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -1, 1 );
		pParticle->m_uchColor[0] = pParticle->m_uchColor[1] = pParticle->m_uchColor[2] = 32;
		
		pParticle->m_uchStartAlpha	= 64;
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_uchStartSize	= 64;
		pParticle->m_uchEndSize		= 128;
	}


	//
	// Dlight
	//

	dlight_t *dl= effects->CL_AllocDlight ( 0 );

	dl->origin	= offset;
	dl->color.r = dl->color.g = dl->color.b = 64;
	dl->radius	= random->RandomFloat(128,256);
	dl->die		= gpGlobals->curtime + 0.1;


	//
	// Moving lines
	//

	TrailParticle	*pTrailParticle;
	CSmartPtr<CTrailParticles> pSparkEmitter	= CTrailParticles::Create( "FX_ConcussiveExplosion 3" );
	PMaterialHandle hMaterial;
	int				numSparks;

	if ( pSparkEmitter.IsValid() )
	{
		hMaterial= pSparkEmitter->GetPMaterial( "effects/blueflare1" );

		pSparkEmitter->SetSortOrigin( offset );
		pSparkEmitter->m_ParticleCollision.SetGravity( 0.0f );

		numSparks = random->RandomInt( 16, 32 );

		//Dump out sparks
		for ( i = 0; i < numSparks; i++ )
		{
			pTrailParticle = (TrailParticle *) pSparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

			if ( pTrailParticle == NULL )
				return;

			pTrailParticle->m_flLifetime	= 0.0f;

			dir.Random( -1.0f, 1.0f );

			pTrailParticle->m_flWidth		= random->RandomFloat( 1.0f, 2.0f );
			pTrailParticle->m_flLength		= random->RandomFloat( 0.01f, 0.1f );
			pTrailParticle->m_flDieTime		= random->RandomFloat( 0.1f, 0.2f );
			
			pTrailParticle->m_vecVelocity	= dir * random->RandomFloat( 800, 1000 );

			float colorRamp = random->RandomFloat( 0.75f, 1.0f );
			FloatToColor32( pTrailParticle->m_color, colorRamp, colorRamp, 1.0f, 1.0f );
		}
	}

	//Moving particles
	CSmartPtr<CTrailParticles> pCollisionEmitter	= CTrailParticles::Create( "FX_ConcussiveExplosion 4" );

	if ( pCollisionEmitter.IsValid() )
	{
		//Setup our collision information
		pCollisionEmitter->Setup( (Vector &) offset,
								NULL, 
								SPARK_ELECTRIC_SPREAD, 
								SPARK_ELECTRIC_MINSPEED*6, 
								SPARK_ELECTRIC_MAXSPEED*6, 
								-400, 
								SPARK_ELECTRIC_DAMPEN, 
								bitsPARTICLE_TRAIL_FADE );

		pCollisionEmitter->SetSortOrigin( offset );

		numSparks = random->RandomInt( 8, 16 );
		hMaterial = pCollisionEmitter->GetPMaterial( "effects/blueflare1" );

		//Dump out sparks
		for ( i = 0; i < numSparks; i++ )
		{
			pTrailParticle = (TrailParticle *) pCollisionEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

			if ( pTrailParticle == NULL )
				return;

			pTrailParticle->m_flLifetime	= 0.0f;

			dir.Random( -1.0f, 1.0f );
			dir[2] = random->RandomFloat( 0.0f, 0.75f );

			pTrailParticle->m_flWidth		= random->RandomFloat( 1.0f, 2.0f );
			pTrailParticle->m_flLength		= random->RandomFloat( 0.01f, 0.1f );
			pTrailParticle->m_flDieTime		= random->RandomFloat( 0.2f, 1.0f );
			
			pTrailParticle->m_vecVelocity	= dir * random->RandomFloat( 128, 512 );

			float colorRamp = random->RandomFloat( 0.75f, 1.0f );
			FloatToColor32( pTrailParticle->m_color, colorRamp, colorRamp, 1.0f, 1.0f );
		}
	}
}


void FX_SparkFan( Vector &position, Vector &normal )
{
	Vector	offset = position + ( normal * 1.0f );

	CSmartPtr<CTrailParticles> sparkEmitter = CTrailParticles::Create( "FX_MetalScrape 1" );

	if ( !sparkEmitter )
		return;

	sparkEmitter->SetSortOrigin( offset );

	//Setup our collision information
	sparkEmitter->Setup( offset, 
						&normal, 
						METAL_SCRAPE_SPREAD, 
						METAL_SCRAPE_MINSPEED, 
						METAL_SCRAPE_MAXSPEED, 
						METAL_SCRAPE_GRAVITY, 
						METAL_SCRAPE_DAMPEN, 
						bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );

	if ( g_Material_Spark == NULL )
	{
		g_Material_Spark = sparkEmitter->GetPMaterial( "effects/spark" );
	}

	TrailParticle	*pParticle;
	Vector			dir;

	float	length	= 0.06f;

	//Dump out sparks
	for ( int i = 0; i < 35; i++ )
	{
		pParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), g_Material_Spark, offset );
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime	= 0.0f;

		float	spreadOfs = random->RandomFloat( 0.0f, 2.0f );

		dir[0] = normal[0] + random->RandomFloat( -(METAL_SCRAPE_SPREAD*spreadOfs), (METAL_SCRAPE_SPREAD*spreadOfs) );
		dir[1] = normal[1] + random->RandomFloat( -(METAL_SCRAPE_SPREAD*spreadOfs), (METAL_SCRAPE_SPREAD*spreadOfs) );
		dir[2] = normal[2] + random->RandomFloat( -(METAL_SCRAPE_SPREAD*spreadOfs), (METAL_SCRAPE_SPREAD*spreadOfs) );
	
		pParticle->m_flWidth		= random->RandomFloat( 2.0f, 5.0f );
		pParticle->m_flLength		= random->RandomFloat( length*0.25f, length );
		pParticle->m_flDieTime		= random->RandomFloat( 2.0f, 2.0f );
		
		pParticle->m_vecVelocity	= dir * random->RandomFloat( (METAL_SCRAPE_MINSPEED*(2.0f-spreadOfs)), (METAL_SCRAPE_MAXSPEED*(2.0f-spreadOfs)) );
		
		Color32Init( pParticle->m_color, 255, 255, 255, 255 );
	}
}


void ManhackSparkCallback( const CEffectData & data )
{
	Vector vecNormal;
	Vector vecPosition;
	QAngle angles;

	vecPosition = data.m_vOrigin;
	vecNormal = data.m_vNormal;
	angles = data.m_vAngles;

	FX_SparkFan( vecPosition, vecNormal );
}

DECLARE_CLIENT_EFFECT( "ManhackSparks", ManhackSparkCallback );
