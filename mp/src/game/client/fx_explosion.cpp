//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base explosion effect
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "fx_explosion.h"
#include "clienteffectprecachesystem.h"
#include "fx_sparks.h"
#include "dlight.h"
#include "tempentity.h"
#include "iefx.h"
#include "engine/IEngineSound.h"
#include "engine/ivdebugoverlay.h"
#include "c_te_effect_dispatch.h"
#include "fx.h"
#include "fx_quad.h"
#include "fx_line.h"
#include "fx_water.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	__EXPLOSION_DEBUG	0

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectExplosion )
CLIENTEFFECT_MATERIAL( "effects/fire_cloud1" )
CLIENTEFFECT_MATERIAL( "effects/fire_cloud2" )
CLIENTEFFECT_MATERIAL( "effects/fire_embers1" )
CLIENTEFFECT_MATERIAL( "effects/fire_embers2" )
CLIENTEFFECT_MATERIAL( "effects/fire_embers3" )
CLIENTEFFECT_MATERIAL( "particle/particle_smokegrenade" )
CLIENTEFFECT_MATERIAL( "particle/particle_smokegrenade1" )
CLIENTEFFECT_MATERIAL( "effects/splash3" )
CLIENTEFFECT_MATERIAL( "effects/splashwake1" )
CLIENTEFFECT_REGISTER_END()

//
// CExplosionParticle
//

class CExplosionParticle : public CSimpleEmitter
{
public:
	
	CExplosionParticle( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	//Create
	static CExplosionParticle *Create( const char *pDebugName )
	{
		return new CExplosionParticle( pDebugName );
	}

	//Roll
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -8.0f );

		//Cap the minimum roll
		if ( fabs( pParticle->m_flRollDelta ) < 0.5f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.5f : -0.5f;
		}

		return pParticle->m_flRoll;
	}

	//Velocity
	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		Vector	saveVelocity = pParticle->m_vecVelocity;

		//Decellerate
		//pParticle->m_vecVelocity += pParticle->m_vecVelocity * ( timeDelta * -20.0f );
		static float dtime;
		static float decay;

		if ( dtime != timeDelta )
		{
			dtime = timeDelta;
			float expected = 0.5;
			decay = exp( log( 0.0001f ) * dtime / expected );
		}

		pParticle->m_vecVelocity = pParticle->m_vecVelocity * decay;


		//Cap the minimum speed
		if ( pParticle->m_vecVelocity.LengthSqr() < (32.0f*32.0f) )
		{
			VectorNormalize( saveVelocity );
			pParticle->m_vecVelocity = saveVelocity * 32.0f;
		}
	}

	//Alpha
	virtual float UpdateAlpha( const SimpleParticle *pParticle )
	{
		float	tLifetime = pParticle->m_flLifetime / pParticle->m_flDieTime;
		float	ramp = 1.0f - tLifetime;

		return Bias( ramp, 0.25f );
	}

	//Color
	virtual Vector UpdateColor( const SimpleParticle *pParticle )
	{
		Vector	color;

		float	tLifetime = pParticle->m_flLifetime / pParticle->m_flDieTime;
		float	ramp = Bias( 1.0f - tLifetime, 0.25f );

		color[0] = ( (float) pParticle->m_uchColor[0] * ramp ) / 255.0f;
		color[1] = ( (float) pParticle->m_uchColor[1] * ramp ) / 255.0f;
		color[2] = ( (float) pParticle->m_uchColor[2] * ramp ) / 255.0f;

		return color;
	}

private:
	CExplosionParticle( const CExplosionParticle & );
};

//Singleton static member definition
C_BaseExplosionEffect	C_BaseExplosionEffect::m_instance;

C_BaseExplosionEffect::C_BaseExplosionEffect( void ) : m_Material_Smoke( NULL ), m_Material_FireCloud( NULL )
{
	m_Material_Embers[0] = NULL;
	m_Material_Embers[1] = NULL;
}

//Singleton accessor
C_BaseExplosionEffect &BaseExplosionEffect( void )
{ 
	return C_BaseExplosionEffect::Instance(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &deviant - 
//			&source - 
// Output : float
//-----------------------------------------------------------------------------
float C_BaseExplosionEffect::ScaleForceByDeviation( Vector &deviant, Vector &source, float spread, float *force )
{
	if ( ( deviant == vec3_origin ) || ( source == vec3_origin ) )
		return 1.0f;

	float	dot = source.Dot( deviant );
	
	dot = spread * fabs( dot );	

	if ( force != NULL )
	{
		(*force) *= dot;
	}

	return dot;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : position - 
//			force - 
// Output : virtual void
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::Create( const Vector &position, float force, float scale, int flags )
{
	m_vecOrigin = position;
	m_fFlags	= flags;

	//Find the force of the explosion
	GetForceDirection( m_vecOrigin, force, &m_vecDirection, &m_flForce );

#if __EXPLOSION_DEBUG
	debugoverlay->AddBoxOverlay( m_vecOrigin, -Vector(32,32,32), Vector(32,32,32), vec3_angle, 255, 0, 0, 64, 5.0f );
	debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin+(m_vecDirection*force*m_flForce), 0, 0, 255, false, 3 );
#endif

	PlaySound();

	if ( scale != 0 )
	{
		// UNDONE: Make core size parametric to scale or remove scale?
		CreateCore();
	}

	CreateDebris();
	//FIXME: CreateDynamicLight();
	CreateMisc();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::CreateCore( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOFIREBALL )
		return;

	Vector	offset;
	int		i;

	//Spread constricts as force rises
	float force = m_flForce;

	//Cap our force
	if ( force < EXPLOSION_FORCE_MIN )
		force = EXPLOSION_FORCE_MIN;
	
	if ( force > EXPLOSION_FORCE_MAX )
		force = EXPLOSION_FORCE_MAX;

	float spread = 1.0f - (0.15f*force);

	SimpleParticle	*pParticle;

	CSmartPtr<CExplosionParticle> pSimple = CExplosionParticle::Create( "exp_smoke" );
	pSimple->SetSortOrigin( m_vecOrigin );
	pSimple->SetNearClip( 64, 128 );

	pSimple->GetBinding().SetBBox( m_vecOrigin - Vector( 128, 128, 128 ), m_vecOrigin + Vector( 128, 128, 128 ) );
	
	if ( m_Material_Smoke == NULL )
	{
		m_Material_Smoke = g_Mat_DustPuff[1];
	}

	//FIXME: Better sampling area
	offset = m_vecOrigin + ( m_vecDirection * 32.0f );

	//Find area ambient light color and use it to tint smoke
	Vector worldLight = WorldGetLightForPoint( offset, true );
	
	Vector	tint;
	float	luminosity;
	if ( worldLight == vec3_origin )
	{
		tint = vec3_origin;
		luminosity = 0.0f;
	}
	else
	{
		UTIL_GetNormalizedColorTintAndLuminosity( worldLight, &tint, &luminosity );
	}

	// We only take a portion of the tint
	tint = (tint * 0.25f)+(Vector(0.75f,0.75f,0.75f));
	
	// Rescale to a character range
	luminosity *= 255;

	if ( (m_fFlags & TE_EXPLFLAG_NOFIREBALLSMOKE) == 0 )
	{
		//
		// Smoke - basic internal filler
		//

		for ( i = 0; i < 4; i++ )
		{
			pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), m_Material_Smoke, m_vecOrigin );

			if ( pParticle != NULL )
			{
				pParticle->m_flLifetime = 0.0f;

	#ifdef INVASION_CLIENT_DLL
				pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );
	#endif
	#ifdef _XBOX
				pParticle->m_flDieTime	= 1.0f;
	#else
				pParticle->m_flDieTime	= random->RandomFloat( 2.0f, 3.0f );
	#endif

				pParticle->m_vecVelocity.Random( -spread, spread );
				pParticle->m_vecVelocity += ( m_vecDirection * random->RandomFloat( 1.0f, 6.0f ) );
				
				VectorNormalize( pParticle->m_vecVelocity );

				float	fForce = random->RandomFloat( 1, 750 ) * force;

				//Scale the force down as we fall away from our main direction
				ScaleForceByDeviation( pParticle->m_vecVelocity, m_vecDirection, spread, &fForce );

				pParticle->m_vecVelocity *= fForce;
				
				#if __EXPLOSION_DEBUG
				debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
				#endif

				int nColor = random->RandomInt( luminosity*0.5f, luminosity );
				pParticle->m_uchColor[0] = ( worldLight[0] * nColor );
				pParticle->m_uchColor[1] = ( worldLight[1] * nColor );
				pParticle->m_uchColor[2] = ( worldLight[2] * nColor );
				
				pParticle->m_uchStartSize	= 72;
				pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 2;
				
				pParticle->m_uchStartAlpha	= 255;
				pParticle->m_uchEndAlpha	= 0;
				
				pParticle->m_flRoll			= random->RandomInt( 0, 360 );
				pParticle->m_flRollDelta	= random->RandomFloat( -2.0f, 2.0f );
			}
		}


		//
		// Inner core
		//

#ifndef _XBOX

		for ( i = 0; i < 8; i++ )
		{
			offset.Random( -16.0f, 16.0f );
			offset += m_vecOrigin;

			pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), m_Material_Smoke, offset );

			if ( pParticle != NULL )
			{
				pParticle->m_flLifetime = 0.0f;

	#ifdef INVASION_CLIENT_DLL
				pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );
	#else
				pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );
	#endif

				pParticle->m_vecVelocity.Random( -spread, spread );
				pParticle->m_vecVelocity += ( m_vecDirection * random->RandomFloat( 1.0f, 6.0f ) );
				
				VectorNormalize( pParticle->m_vecVelocity );

				float	fForce = random->RandomFloat( 1, 2000 ) * force;

				//Scale the force down as we fall away from our main direction
				ScaleForceByDeviation( pParticle->m_vecVelocity, m_vecDirection, spread, &fForce );

				pParticle->m_vecVelocity *= fForce;
				
				#if __EXPLOSION_DEBUG
				debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
				#endif

				int nColor = random->RandomInt( luminosity*0.5f, luminosity );
				pParticle->m_uchColor[0] = ( worldLight[0] * nColor );
				pParticle->m_uchColor[1] = ( worldLight[1] * nColor );
				pParticle->m_uchColor[2] = ( worldLight[2] * nColor );
						
				pParticle->m_uchStartSize	= random->RandomInt( 32, 64 );
				pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 2;

				pParticle->m_uchStartAlpha	= random->RandomFloat( 128, 255 );
				pParticle->m_uchEndAlpha	= 0;
				
				pParticle->m_flRoll			= random->RandomInt( 0, 360 );
				pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
			}
		}
#endif // !_XBOX

		//
		// Ground ring
		//

		Vector	vRight, vUp;
		VectorVectors( m_vecDirection, vRight, vUp );

		Vector	forward;

#ifndef INVASION_CLIENT_DLL

#ifndef _XBOX 
		int	numRingSprites = 32;
#else
		int	numRingSprites = 8;
#endif

		float flIncr = (2*M_PI) / (float) numRingSprites; // Radians
		float flYaw = 0.0f;

		for ( i = 0; i < numRingSprites; i++ )
		{
			flYaw += flIncr;
			SinCos( flYaw, &forward.y, &forward.x );
			forward.z = 0.0f;

			offset = ( RandomVector( -4.0f, 4.0f ) + m_vecOrigin ) + ( forward * random->RandomFloat( 8.0f, 16.0f ) );

			pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), m_Material_Smoke, offset );

			if ( pParticle != NULL )
			{
				pParticle->m_flLifetime = 0.0f;
				pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.5f );

				pParticle->m_vecVelocity = forward;
			
				float	fForce = random->RandomFloat( 500, 2000 ) * force;

				//Scale the force down as we fall away from our main direction
				ScaleForceByDeviation( pParticle->m_vecVelocity, pParticle->m_vecVelocity, spread, &fForce );

				pParticle->m_vecVelocity *= fForce;
				
				#if __EXPLOSION_DEBUG
				debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
				#endif

				int nColor = random->RandomInt( luminosity*0.5f, luminosity );
				pParticle->m_uchColor[0] = ( worldLight[0] * nColor );
				pParticle->m_uchColor[1] = ( worldLight[1] * nColor );
				pParticle->m_uchColor[2] = ( worldLight[2] * nColor );

				pParticle->m_uchStartSize	= random->RandomInt( 16, 32 );
				pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4;

				pParticle->m_uchStartAlpha	= random->RandomFloat( 16, 32 );
				pParticle->m_uchEndAlpha	= 0;
				
				pParticle->m_flRoll			= random->RandomInt( 0, 360 );
				pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
			}
		}
#endif
	}

#ifndef _XBOX

	//
	// Embers
	//

	if ( m_Material_Embers[0] == NULL )
	{
		m_Material_Embers[0] = pSimple->GetPMaterial( "effects/fire_embers1" );
	}

	if ( m_Material_Embers[1] == NULL )
	{
		m_Material_Embers[1] = pSimple->GetPMaterial( "effects/fire_embers2" );
	}

	for ( i = 0; i < 16; i++ )
	{
		offset.Random( -32.0f, 32.0f );
		offset += m_vecOrigin;

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), m_Material_Embers[random->RandomInt(0,1)], offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 2.0f, 3.0f );

			pParticle->m_vecVelocity.Random( -spread*2, spread*2 );
			pParticle->m_vecVelocity += m_vecDirection;
			
			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 1.0f, 400.0f );

			//Scale the force down as we fall away from our main direction
			float	vDev = ScaleForceByDeviation( pParticle->m_vecVelocity, m_vecDirection, spread );

			pParticle->m_vecVelocity *= fForce * ( 16.0f * (vDev*vDev*0.5f) );
			
			#if __EXPLOSION_DEBUG
			debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
			#endif

			int nColor = random->RandomInt( 192, 255 );
			pParticle->m_uchColor[0]	= pParticle->m_uchColor[1] = pParticle->m_uchColor[2] = nColor;
			
			pParticle->m_uchStartSize	= random->RandomInt( 8, 16 ) * vDev;

			pParticle->m_uchStartSize	= clamp( pParticle->m_uchStartSize, (uint8) 4, (uint8) 32 );

			pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
			
			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
		}
	}
#endif // !_XBOX

	//
	// Fireballs
	//

	if ( m_Material_FireCloud == NULL )
	{
		m_Material_FireCloud = pSimple->GetPMaterial( "effects/fire_cloud2" );
	}

#ifndef _XBOX
	int numFireballs = 32;
#else
	int numFireballs = 16;
#endif

	for ( i = 0; i < numFireballs; i++ )
	{
		offset.Random( -48.0f, 48.0f );
		offset += m_vecOrigin;

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), m_Material_FireCloud, offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 0.2f, 0.4f );

			pParticle->m_vecVelocity.Random( -spread*0.75f, spread*0.75f );
			pParticle->m_vecVelocity += m_vecDirection;
			
			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 400.0f, 800.0f );

			//Scale the force down as we fall away from our main direction
			float	vDev = ScaleForceByDeviation( pParticle->m_vecVelocity, m_vecDirection, spread );

			pParticle->m_vecVelocity *= fForce * ( 16.0f * (vDev*vDev*0.5f) );

			#if __EXPLOSION_DEBUG
			debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
			#endif

			int nColor = random->RandomInt( 128, 255 );
			pParticle->m_uchColor[0]	= pParticle->m_uchColor[1] = pParticle->m_uchColor[2] = nColor;
			
			pParticle->m_uchStartSize	= random->RandomInt( 32, 85 ) * vDev;

			pParticle->m_uchStartSize	= clamp( pParticle->m_uchStartSize, (uint8) 32, (uint8) 85 );

			pParticle->m_uchEndSize		= (int)((float)pParticle->m_uchStartSize * 1.5f);
			
			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -16.0f, 16.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::CreateDebris( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOPARTICLES )
		return;

	//
	// Sparks
	//

	CSmartPtr<CTrailParticles> pSparkEmitter	= CTrailParticles::Create( "CreateDebris 1" );
	if ( pSparkEmitter == NULL )
	{
		assert(0);
		return;
	}

	if ( m_Material_FireCloud == NULL )
	{
		m_Material_FireCloud = pSparkEmitter->GetPMaterial( "effects/fire_cloud2" );
	}

	pSparkEmitter->SetSortOrigin( m_vecOrigin );
	
	pSparkEmitter->m_ParticleCollision.SetGravity( 200.0f );
	pSparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
	pSparkEmitter->SetVelocityDampen( 8.0f );
	
	// Set our bbox, don't auto-calculate it!
	pSparkEmitter->GetBinding().SetBBox( m_vecOrigin - Vector( 128, 128, 128 ), m_vecOrigin + Vector( 128, 128, 128 ) );

#ifndef _XBOX
	int		numSparks = random->RandomInt( 8, 16 );
#else
	int		numSparks = random->RandomInt( 2, 4 );
#endif

	Vector	dir;
	float	spread = 1.0f;
	TrailParticle	*tParticle;

	// Dump out sparks
	int i;
	for ( i = 0; i < numSparks; i++ )
	{
		tParticle = (TrailParticle *) pSparkEmitter->AddParticle( sizeof(TrailParticle), m_Material_FireCloud, m_vecOrigin );

		if ( tParticle == NULL )
			break;

		tParticle->m_flLifetime	= 0.0f;
		tParticle->m_flDieTime	= random->RandomFloat( 0.1f, 0.15f );

		dir.Random( -spread, spread );
		dir += m_vecDirection;
		VectorNormalize( dir );
		
		tParticle->m_flWidth		= random->RandomFloat( 2.0f, 16.0f );
		tParticle->m_flLength		= random->RandomFloat( 0.05f, 0.1f );
		
		tParticle->m_vecVelocity	= dir * random->RandomFloat( 1500, 2500 );

		Color32Init( tParticle->m_color, 255, 255, 255, 255 );
	}

#ifndef _XBOX
	//
	// Chunks
	//

	Vector	offset;
	CSmartPtr<CFleckParticles> fleckEmitter = CFleckParticles::Create( "CreateDebris 2", m_vecOrigin, Vector(128,128,128) );
	if ( !fleckEmitter )
		return;

	// Setup our collision information
	fleckEmitter->m_ParticleCollision.Setup( m_vecOrigin, &m_vecDirection, 0.9f, 512, 1024, 800, 0.5f );
	

#ifdef _XBOX
	int	numFlecks = random->RandomInt( 8, 16 );
#else	
	int	numFlecks = random->RandomInt( 16, 32 );
#endif // _XBOX


	// Dump out flecks
	for ( i = 0; i < numFlecks; i++ )
	{
		offset = m_vecOrigin + ( m_vecDirection * 16.0f );

		offset[0] += random->RandomFloat( -8.0f, 8.0f );
		offset[1] += random->RandomFloat( -8.0f, 8.0f );
		offset[2] += random->RandomFloat( -8.0f, 8.0f );

		FleckParticle *pParticle = (FleckParticle *) fleckEmitter->AddParticle( sizeof(FleckParticle), g_Mat_Fleck_Cement[random->RandomInt(0,1)], offset );

		if ( pParticle == NULL )
			break;

		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= 3.0f;
		
		dir[0] = m_vecDirection[0] + random->RandomFloat( -1.0f, 1.0f );
		dir[1] = m_vecDirection[1] + random->RandomFloat( -1.0f, 1.0f );
		dir[2] = m_vecDirection[2] + random->RandomFloat( -1.0f, 1.0f );

		pParticle->m_uchSize		= random->RandomInt( 1, 3 );

		VectorNormalize( dir );

		float	fForce = ( random->RandomFloat( 64, 256 ) * ( 4 - pParticle->m_uchSize ) );

		float	fDev = ScaleForceByDeviation( dir, m_vecDirection, 0.8f );

		pParticle->m_vecVelocity = dir * ( fForce * ( 16.0f * (fDev*fDev*0.5f) ) );

		pParticle->m_flRoll			= random->RandomFloat( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( 0, 360 );

		float colorRamp = random->RandomFloat( 0.5f, 1.5f );
		pParticle->m_uchColor[0] = MIN( 1.0f, 0.25f*colorRamp )*255.0f;
		pParticle->m_uchColor[1] = MIN( 1.0f, 0.25f*colorRamp )*255.0f;
		pParticle->m_uchColor[2] = MIN( 1.0f, 0.25f*colorRamp )*255.0f;
	}
#endif // !_XBOX
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::CreateMisc( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::CreateDynamicLight( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NODLIGHTS )
		return;

	dlight_t *dl = effects->CL_AllocDlight( 0 );
	
	VectorCopy (m_vecOrigin, dl->origin);
	
	dl->decay	= 200;
	dl->radius	= 255;
	dl->color.r = 255;
	dl->color.g = 220;
	dl->color.b = 128;
	dl->die		= gpGlobals->curtime + 0.1f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::PlaySound( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOSOUND )
		return;

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "BaseExplosionEffect.Sound", &m_vecOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			&m_vecDirection - 
//			strength - 
// Output : float
//-----------------------------------------------------------------------------
float C_BaseExplosionEffect::Probe( const Vector &origin, Vector *vecDirection, float strength )
{
	//Press out
	Vector endpos = origin + ( (*vecDirection) * strength );

	//Trace into the world
	trace_t	tr;
	UTIL_TraceLine( origin, endpos, CONTENTS_SOLID, NULL, COLLISION_GROUP_NONE, &tr );

	//Push back a proportional amount to the probe
	(*vecDirection) = -(*vecDirection) * (1.0f-tr.fraction);

#if __EXPLOSION_DEBUG
	debugoverlay->AddLineOverlay( m_vecOrigin, endpos, (255*(1.0f-tr.fraction)), (255*tr.fraction), 0, false, 3 );
#endif

	assert(( 1.0f - tr.fraction ) >= 0.0f );

	//Return the impacted proportion of the probe
	return (1.0f-tr.fraction);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			&m_vecDirection - 
//			&m_flForce - 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::GetForceDirection( const Vector &origin, float magnitude, Vector *resultDirection, float *resultForce )
{
	Vector	d[6];

	//All cardinal directions
	d[0] = Vector(  1,  0,  0 );
	d[1] = Vector( -1,  0,  0 );
	d[2] = Vector(  0,  1,  0 );
	d[3] = Vector(  0, -1,  0 );
	d[4] = Vector(  0,  0,  1 );
	d[5] = Vector(  0,  0, -1 );

	//Init the results
	(*resultDirection).Init();
	(*resultForce) = 1.0f;
	
	//Get the aggregate force vector
	for ( int i = 0; i < 6; i++ )
	{
		(*resultForce) += Probe( origin, &d[i], magnitude );
		(*resultDirection) += d[i];
	}

	//If we've hit nothing, then point up
	if ( (*resultDirection) == vec3_origin )
	{
		(*resultDirection) = Vector( 0, 0, 1 );
		(*resultForce) = EXPLOSION_FORCE_MIN;
	}

	//Just return the direction
	VectorNormalize( (*resultDirection) );
}

//-----------------------------------------------------------------------------
// Purpose: Intercepts the water explosion dispatch effect
//-----------------------------------------------------------------------------
void ExplosionCallback( const CEffectData &data )
{
	BaseExplosionEffect().Create( data.m_vOrigin, data.m_flMagnitude, data.m_flScale, data.m_fFlags );
}

DECLARE_CLIENT_EFFECT( "Explosion", ExplosionCallback );


//===============================================================================================================
// Water Explosion
//===============================================================================================================
//
// CExplosionParticle
//

class CWaterExplosionParticle : public CSimpleEmitter
{
public:
	
	CWaterExplosionParticle( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	//Create
	static CWaterExplosionParticle *Create( const char *pDebugName )
	{
		return new CWaterExplosionParticle( pDebugName );
	}

	//Roll
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -8.0f );

		//Cap the minimum roll
		if ( fabs( pParticle->m_flRollDelta ) < 0.25f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.25f : -0.25f;
		}

		return pParticle->m_flRoll;
	}

	//Velocity
	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		Vector	saveVelocity = pParticle->m_vecVelocity;

		//Decellerate
		//pParticle->m_vecVelocity += pParticle->m_vecVelocity * ( timeDelta * -20.0f );
		static float dtime;
		static float decay;

		if ( dtime != timeDelta )
		{
			dtime = timeDelta;
			float expected = 0.5;
			decay = exp( log( 0.0001f ) * dtime / expected );
		}

		pParticle->m_vecVelocity = pParticle->m_vecVelocity * decay;


		//Cap the minimum speed
		if ( pParticle->m_vecVelocity.LengthSqr() < (8.0f*8.0f) )
		{
			VectorNormalize( saveVelocity );
			pParticle->m_vecVelocity = saveVelocity * 8.0f;
		}
	}

	//Alpha
	virtual float UpdateAlpha( const SimpleParticle *pParticle )
	{
		float	tLifetime = pParticle->m_flLifetime / pParticle->m_flDieTime;
		float	ramp = 1.0f - tLifetime;

		//Non-linear fade
		if ( ramp < 0.75f )
			ramp *= ramp;

		return ramp;
	}

private:
	CWaterExplosionParticle( const CWaterExplosionParticle & );
};

//Singleton static member definition
C_WaterExplosionEffect	C_WaterExplosionEffect::m_waterinstance;

//Singleton accessor
C_WaterExplosionEffect &WaterExplosionEffect( void )
{ 
	return C_WaterExplosionEffect::Instance(); 
}

#define	MAX_WATER_SURFACE_DISTANCE	512

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WaterExplosionEffect::Create( const Vector &position, float force, float scale, int flags )
{
	m_vecOrigin = position;

	// Find our water surface by tracing up till we're out of the water
	trace_t tr;
	Vector vecTrace( 0, 0, MAX_WATER_SURFACE_DISTANCE );
	UTIL_TraceLine( m_vecOrigin, m_vecOrigin + vecTrace, MASK_WATER, NULL, COLLISION_GROUP_NONE, &tr );
	
	// If we didn't start in water, we're above it
	if ( tr.startsolid == false )
	{
		// Look downward to find the surface
		vecTrace.Init( 0, 0, -MAX_WATER_SURFACE_DISTANCE );
		UTIL_TraceLine( m_vecOrigin, m_vecOrigin + vecTrace, MASK_WATER, NULL, COLLISION_GROUP_NONE, &tr );

		// If we hit it, setup the explosion
		if ( tr.fraction < 1.0f )
		{
			m_vecWaterSurface = tr.endpos;
			m_flDepth = 0.0f;
		}
		else
		{
			//NOTENOTE: We somehow got into a water explosion without being near water?
			Assert( 0 );
			m_vecWaterSurface = m_vecOrigin;
			m_flDepth = 0.0f;
		}
	}
	else if ( tr.fractionleftsolid )
	{
		// Otherwise we came out of the water at this point
		m_vecWaterSurface = m_vecOrigin + (vecTrace * tr.fractionleftsolid);
		m_flDepth = MAX_WATER_SURFACE_DISTANCE * tr.fractionleftsolid;
	}
	else
	{
		// Use default values, we're really deep
		m_vecWaterSurface = m_vecOrigin;
		m_flDepth = MAX_WATER_SURFACE_DISTANCE;
	}

	// Get our lighting information
	FX_GetSplashLighting( m_vecOrigin + Vector( 0, 0, 32 ), &m_vecColor, &m_flLuminosity );

	BaseClass::Create( position, force, scale, flags );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WaterExplosionEffect::CreateCore( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOFIREBALL )
		return;

	// Get our lighting information for the water surface
	Vector	color;
	float	luminosity;
	FX_GetSplashLighting( m_vecWaterSurface + Vector( 0, 0, 8 ), &color, &luminosity );

	float lifetime = random->RandomFloat( 0.8f, 1.0f );

	// Ground splash
	FX_AddQuad( m_vecWaterSurface + Vector(0,0,2), 
				Vector(0,0,1), 
				64, 
				64 * 4.0f,
				0.85f, 
				luminosity,
				0.0f,
				0.25f,
				random->RandomInt( 0, 360 ), 
				random->RandomFloat( -4, 4 ), 
				color,
				2.0f,
				"effects/splashwake1",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );

	Vector	vRight, vUp;
	VectorVectors( Vector(0,0,1) , vRight, vUp );

	Vector	start, end;
	
	float radius = 50.0f;

	unsigned int flags = 0;

	// Base vertical shaft
	FXLineData_t lineData;

	start = m_vecWaterSurface;
	end = start + ( Vector( 0, 0, 1 ) * random->RandomFloat( radius, radius*1.5f ) );

	if ( random->RandomInt( 0, 1 ) )
	{
		flags |= FXSTATICLINE_FLIP_HORIZONTAL;
	}
	else
	{
		flags = 0;
	}

	lineData.m_flDieTime = lifetime * 0.5f;
	
	lineData.m_flStartAlpha= luminosity;
	lineData.m_flEndAlpha = 0.0f;
	
	lineData.m_flStartScale = radius*0.5f;
	lineData.m_flEndScale = radius*2; 

	lineData.m_pMaterial = materials->FindMaterial( "effects/splash3", 0, 0 );

	lineData.m_vecStart = start;
	lineData.m_vecStartVelocity = vec3_origin;

	lineData.m_vecEnd = end;
	lineData.m_vecEndVelocity = Vector(0,0,random->RandomFloat( 650, 750 ));

	FX_AddLine( lineData );

	// Inner filler shaft
	start = m_vecWaterSurface;
	end = start + ( Vector(0,0,1) * random->RandomFloat( 32, 64 ) );

	if ( random->RandomInt( 0, 1 ) )
	{
		flags |= FXSTATICLINE_FLIP_HORIZONTAL;
	}
	else
	{
		flags = 0;
	}

	lineData.m_flDieTime = lifetime * 0.5f;
	
	lineData.m_flStartAlpha= luminosity;
	lineData.m_flEndAlpha = 0.0f;
	
	lineData.m_flStartScale = radius;
	lineData.m_flEndScale = radius*2; 

	lineData.m_pMaterial = materials->FindMaterial( "effects/splash3", 0, 0 );

	lineData.m_vecStart = start;
	lineData.m_vecStartVelocity = vec3_origin;

	lineData.m_vecEnd = end;
	lineData.m_vecEndVelocity = Vector(0,0,1) * random->RandomFloat( 64, 128 );

	FX_AddLine( lineData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WaterExplosionEffect::CreateDebris( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOPARTICLES )
		return;

	// Must be in deep enough water
	if ( m_flDepth <= 128 )
		return;

	Vector	offset;
	int		i;

	//Spread constricts as force rises
	float force = m_flForce;

	//Cap our force
	if ( force < EXPLOSION_FORCE_MIN )
		force = EXPLOSION_FORCE_MIN;
	
	if ( force > EXPLOSION_FORCE_MAX )
		force = EXPLOSION_FORCE_MAX;

	float spread = 1.0f - (0.15f*force);

	SimpleParticle	*pParticle;

	CSmartPtr<CWaterExplosionParticle> pSimple = CWaterExplosionParticle::Create( "waterexp_bubbles" );
	pSimple->SetSortOrigin( m_vecOrigin );
	pSimple->SetNearClip( 64, 128 );

	//FIXME: Better sampling area
	offset = m_vecOrigin + ( m_vecDirection * 64.0f );

	//Find area ambient light color and use it to tint bubbles
	Vector	worldLight;
	FX_GetSplashLighting( offset, &worldLight, NULL );

	//
	// Smoke
	//

	CParticleSubTexture *pMaterial[2];

	pMaterial[0] = pSimple->GetPMaterial( "effects/splash1" );
	pMaterial[1] = pSimple->GetPMaterial( "effects/splash2" );

	for ( i = 0; i < 16; i++ )
	{
		offset.Random( -32.0f, 32.0f );
		offset += m_vecOrigin;

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pMaterial[random->RandomInt(0,1)], offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;

#ifdef INVASION_CLIENT_DLL
			pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );
#else
			pParticle->m_flDieTime	= random->RandomFloat( 2.0f, 3.0f );
#endif

			pParticle->m_vecVelocity.Random( -spread, spread );
			pParticle->m_vecVelocity += ( m_vecDirection * random->RandomFloat( 1.0f, 6.0f ) );
			
			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = 1500 * force;

			//Scale the force down as we fall away from our main direction
			ScaleForceByDeviation( pParticle->m_vecVelocity, m_vecDirection, spread, &fForce );

			pParticle->m_vecVelocity *= fForce;
			
			#if __EXPLOSION_DEBUG
			debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
			#endif

			pParticle->m_uchColor[0] = m_vecColor.x * 255;
			pParticle->m_uchColor[1] = m_vecColor.y * 255;
			pParticle->m_uchColor[2] = m_vecColor.z * 255;
			
			pParticle->m_uchStartSize	= random->RandomInt( 32, 64 );
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 2;
			
			pParticle->m_uchStartAlpha	= m_flLuminosity;
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WaterExplosionEffect::CreateMisc( void )
{
	Vector	offset;
	float	colorRamp;
	
	int i;
	float	flScale = 2.0f;

	PMaterialHandle	hMaterial = ParticleMgr()->GetPMaterial( "effects/splash2" );

#ifndef _XBOX

	int		numDrops = 32;
	float	length = 0.1f;
	Vector	vForward, vRight, vUp;
	Vector	offDir;

	TrailParticle	*tParticle;

	CSmartPtr<CTrailParticles> sparkEmitter = CTrailParticles::Create( "splash" );

	if ( !sparkEmitter )
		return;

	sparkEmitter->SetSortOrigin( m_vecWaterSurface );
	sparkEmitter->m_ParticleCollision.SetGravity( 800.0f );
	sparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
	sparkEmitter->SetVelocityDampen( 2.0f );

	//Dump out drops
	for ( i = 0; i < numDrops; i++ )
	{
		offset = m_vecWaterSurface;
		offset[0] += random->RandomFloat( -16.0f, 16.0f ) * flScale;
		offset[1] += random->RandomFloat( -16.0f, 16.0f ) * flScale;

		tParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

		if ( tParticle == NULL )
			break;

		tParticle->m_flLifetime	= 0.0f;
		tParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );

		offDir = Vector(0,0,1) + RandomVector( -1.0f, 1.0f );

		tParticle->m_vecVelocity = offDir * random->RandomFloat( 50.0f * flScale * 2.0f, 100.0f * flScale * 2.0f );
		tParticle->m_vecVelocity[2] += random->RandomFloat( 32.0f, 128.0f ) * flScale;

		tParticle->m_flWidth		= clamp( random->RandomFloat( 1.0f, 3.0f ) * flScale, 0.1f, 4.0f );
		tParticle->m_flLength		= random->RandomFloat( length*0.25f, length )/* * flScale*/;

		colorRamp = random->RandomFloat( 1.5f, 2.0f );

		FloatToColor32( tParticle->m_color, MIN( 1.0f, m_vecColor[0] * colorRamp ), MIN( 1.0f, m_vecColor[1] * colorRamp ), MIN( 1.0f, m_vecColor[2] * colorRamp ), m_flLuminosity );
	}

	//Dump out drops
	for ( i = 0; i < 4; i++ )
	{
		offset = m_vecWaterSurface;
		offset[0] += random->RandomFloat( -16.0f, 16.0f ) * flScale;
		offset[1] += random->RandomFloat( -16.0f, 16.0f ) * flScale;

		tParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

		if ( tParticle == NULL )
			break;

		tParticle->m_flLifetime	= 0.0f;
		tParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );

		offDir = Vector(0,0,1) + RandomVector( -0.2f, 0.2f );

		tParticle->m_vecVelocity = offDir * random->RandomFloat( 50 * flScale * 3.0f, 100 * flScale * 3.0f );
		tParticle->m_vecVelocity[2] += random->RandomFloat( 32.0f, 128.0f ) * flScale;

		tParticle->m_flWidth		= clamp( random->RandomFloat( 2.0f, 3.0f ) * flScale, 0.1f, 4.0f );
		tParticle->m_flLength		= random->RandomFloat( length*0.25f, length )/* * flScale*/;

		colorRamp = random->RandomFloat( 1.5f, 2.0f );

		FloatToColor32( tParticle->m_color, MIN( 1.0f, m_vecColor[0] * colorRamp ), MIN( 1.0f, m_vecColor[1] * colorRamp ), MIN( 1.0f, m_vecColor[2] * colorRamp ), m_flLuminosity );
	}

#endif

	CSmartPtr<CSplashParticle> pSimple = CSplashParticle::Create( "splish" );
	pSimple->SetSortOrigin( m_vecWaterSurface );
	pSimple->SetClipHeight( m_vecWaterSurface.z );
	pSimple->GetBinding().SetBBox( m_vecWaterSurface-(Vector(32.0f, 32.0f, 32.0f)*flScale), m_vecWaterSurface+(Vector(32.0f, 32.0f, 32.0f)*flScale) );

	SimpleParticle	*pParticle;

	for ( i = 0; i < 16; i++ )
	{
		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, m_vecWaterSurface );

		if ( pParticle == NULL )
			break;

		pParticle->m_flLifetime = 0.0f;
		pParticle->m_flDieTime	= 2.0f;	//NOTENOTE: We use a clip plane to realistically control our lifespan

		pParticle->m_vecVelocity.Random( -0.2f, 0.2f );
		pParticle->m_vecVelocity += ( Vector( 0, 0, random->RandomFloat( 4.0f, 6.0f ) ) );
		
		VectorNormalize( pParticle->m_vecVelocity );

		pParticle->m_vecVelocity *= 50 * flScale * (8-i);
		
		colorRamp = random->RandomFloat( 0.75f, 1.25f );

		pParticle->m_uchColor[0]	= MIN( 1.0f, m_vecColor[0] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[1]	= MIN( 1.0f, m_vecColor[1] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[2]	= MIN( 1.0f, m_vecColor[2] * colorRamp ) * 255.0f;
		
		pParticle->m_uchStartSize	= 24 * flScale * RemapValClamped( i, 7, 0, 1, 0.5f );
		pParticle->m_uchEndSize		= MIN( 255, pParticle->m_uchStartSize * 2 );
		
		pParticle->m_uchStartAlpha	= RemapValClamped( i, 7, 0, 255, 32 ) * m_flLuminosity;
		pParticle->m_uchEndAlpha	= 0;
		
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -4.0f, 4.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WaterExplosionEffect::PlaySound( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOSOUND )
		return;

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "Physics.WaterSplash", &m_vecWaterSurface );

	if ( m_flDepth > 128 )
	{
		C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "WaterExplosionEffect.Sound", &m_vecOrigin );
	}
	else
	{
		C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "BaseExplosionEffect.Sound", &m_vecOrigin );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Intercepts the water explosion dispatch effect
//-----------------------------------------------------------------------------
void WaterSurfaceExplosionCallback( const CEffectData &data )
{
	WaterExplosionEffect().Create( data.m_vOrigin, data.m_flMagnitude, data.m_flScale, data.m_fFlags );
}

DECLARE_CLIENT_EFFECT( "WaterSurfaceExplosion", WaterSurfaceExplosionCallback );

//Singleton static member definition
C_MegaBombExplosionEffect	C_MegaBombExplosionEffect::m_megainstance;

//Singleton accessor
C_MegaBombExplosionEffect &MegaBombExplosionEffect( void )
{ 
	return C_MegaBombExplosionEffect::Instance(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_MegaBombExplosionEffect::CreateCore( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOFIREBALL )
		return;

	Vector	offset;
	int		i;

	//Spread constricts as force rises
	float force = m_flForce;

	//Cap our force
	if ( force < EXPLOSION_FORCE_MIN )
		force = EXPLOSION_FORCE_MIN;
	
	if ( force > EXPLOSION_FORCE_MAX )
		force = EXPLOSION_FORCE_MAX;

	float spread = 1.0f - (0.15f*force);

	CSmartPtr<CExplosionParticle> pSimple = CExplosionParticle::Create( "exp_smoke" );
	pSimple->SetSortOrigin( m_vecOrigin );
	pSimple->SetNearClip( 32, 64 );

	SimpleParticle	*pParticle;

	if ( m_Material_FireCloud == NULL )
	{
		m_Material_FireCloud = pSimple->GetPMaterial( "effects/fire_cloud2" );
	}

	//
	// Fireballs
	//

	for ( i = 0; i < 32; i++ )
	{
		offset.Random( -48.0f, 48.0f );
		offset += m_vecOrigin;

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), m_Material_FireCloud, offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 0.2f, 0.4f );

			pParticle->m_vecVelocity.Random( -spread*0.75f, spread*0.75f );
			pParticle->m_vecVelocity += m_vecDirection;
			
			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 400.0f, 800.0f );

			//Scale the force down as we fall away from our main direction
			float	vDev = ScaleForceByDeviation( pParticle->m_vecVelocity, m_vecDirection, spread );

			pParticle->m_vecVelocity *= fForce * ( 16.0f * (vDev*vDev*0.5f) );

			#if __EXPLOSION_DEBUG
			debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
			#endif

			int nColor = random->RandomInt( 128, 255 );
			pParticle->m_uchColor[0]	= pParticle->m_uchColor[1] = pParticle->m_uchColor[2] = nColor;
			
			pParticle->m_uchStartSize	= random->RandomInt( 32, 85 ) * vDev;

			pParticle->m_uchStartSize	= clamp( pParticle->m_uchStartSize, (uint8) 32, (uint8) 85 );

			pParticle->m_uchEndSize		= (int)((float)pParticle->m_uchStartSize * 1.5f);
			
			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -16.0f, 16.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void HelicopterMegaBombCallback( const CEffectData &data )
{
	C_MegaBombExplosionEffect().Create( data.m_vOrigin, 1.0f, 1.0f, 0 );
}

DECLARE_CLIENT_EFFECT( "HelicopterMegaBomb", HelicopterMegaBombCallback );
