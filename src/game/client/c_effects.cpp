//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "c_tracer.h"
#include "view.h"
#include "initializer.h"
#include "particles_simple.h"
#include "env_wind_shared.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"
#include "precipitation_shared.h"
#include "fx_water.h"
#include "c_world.h"
#include "iviewrender.h"
#include "engine/ivdebugoverlay.h"
#include "clienteffectprecachesystem.h"
#include "collisionutils.h"
#include "tier0/vprof.h"
#include "viewrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	cl_winddir			( "cl_winddir", "0", FCVAR_CHEAT, "Weather effects wind direction angle" );
ConVar	cl_windspeed		( "cl_windspeed", "0", FCVAR_CHEAT, "Weather effects wind speed scalar" );

Vector g_vSplashColor( 0.5, 0.5, 0.5 );
float g_flSplashScale = 0.15;
float g_flSplashLifetime = 0.5f;
float g_flSplashAlpha = 0.3f;
ConVar r_RainSplashPercentage( "r_RainSplashPercentage", "20", FCVAR_CHEAT ); // N% chance of a rain particle making a splash.


float GUST_INTERVAL_MIN = 1;
float GUST_INTERVAL_MAX = 2;

float GUST_LIFETIME_MIN = 1;
float GUST_LIFETIME_MAX = 3;

float MIN_SCREENSPACE_RAIN_WIDTH = 1;

#ifndef _XBOX
ConVar r_RainHack( "r_RainHack", "0", FCVAR_CHEAT );
ConVar r_RainRadius( "r_RainRadius", "1500", FCVAR_CHEAT );
ConVar r_RainSideVel( "r_RainSideVel", "130", FCVAR_CHEAT, "How much sideways velocity rain gets." );

ConVar r_RainSimulate( "r_RainSimulate", "1", FCVAR_CHEAT, "Enable/disable rain simulation." );
ConVar r_DrawRain( "r_DrawRain", "1", FCVAR_CHEAT, "Enable/disable rain rendering." );
ConVar r_RainProfile( "r_RainProfile", "0", FCVAR_CHEAT, "Enable/disable rain profiling." );


//Precahce the effects
CLIENTEFFECT_REGISTER_BEGIN( PrecachePrecipitation )
CLIENTEFFECT_MATERIAL( "particle/rain" )
CLIENTEFFECT_MATERIAL( "particle/snow" )
CLIENTEFFECT_REGISTER_END()

//-----------------------------------------------------------------------------
// Precipitation particle type
//-----------------------------------------------------------------------------

class CPrecipitationParticle
{
public:
	Vector	m_Pos;
	Vector	m_Velocity;
	float	m_SpawnTime;				// Note: Tweak with this to change lifetime
	float	m_Mass;
	float	m_Ramp;
	
	float	m_flCurLifetime;
	float	m_flMaxLifetime;
};
						  

class CClient_Precipitation;
static CUtlVector<CClient_Precipitation*> g_Precipitations;

//===========
// Snow fall
//===========
class CSnowFallManager;
static CSnowFallManager *s_pSnowFallMgr = NULL;
bool SnowFallManagerCreate( CClient_Precipitation *pSnowEntity );
void SnowFallManagerDestroy( void );

class AshDebrisEffect : public CSimpleEmitter
{
public:
	AshDebrisEffect( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}

	static AshDebrisEffect* Create( const char *pDebugName );

	virtual float UpdateAlpha( const SimpleParticle *pParticle );
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta );

private:
	AshDebrisEffect( const AshDebrisEffect & );
};

//-----------------------------------------------------------------------------
// Precipitation base entity
//-----------------------------------------------------------------------------

class CClient_Precipitation : public C_BaseEntity
{
class CPrecipitationEffect;
friend class CClient_Precipitation::CPrecipitationEffect;

public:
	DECLARE_CLASS( CClient_Precipitation, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	
	CClient_Precipitation();
	virtual ~CClient_Precipitation();

	// Inherited from C_BaseEntity
	virtual void Precache( );

	void Render();

private:

	// Creates a single particle
	CPrecipitationParticle* CreateParticle();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ClientThink();

	void Simulate( float dt );

	// Renders the particle
	void RenderParticle( CPrecipitationParticle* pParticle, CMeshBuilder &mb );

	void CreateWaterSplashes();

	// Emits the actual particles
	void EmitParticles( float fTimeDelta );
	
	// Computes where we're gonna emit
	bool ComputeEmissionArea( Vector& origin, Vector2D& size );

	// Gets the tracer width and speed
	float GetWidth() const;
	float GetLength() const;
	float GetSpeed() const;

	// Gets the remaining lifetime of the particle
	float GetRemainingLifetime( CPrecipitationParticle* pParticle ) const;

	// Computes the wind vector
	static void ComputeWindVector( );

	// simulation methods
	bool SimulateRain( CPrecipitationParticle* pParticle, float dt );
	bool SimulateSnow( CPrecipitationParticle* pParticle, float dt );

	void CreateAshParticle( void );
	void CreateRainOrSnowParticle( Vector vSpawnPosition, Vector vVelocity );

	// Information helpful in creating and rendering particles
	IMaterial		*m_MatHandle;	// material used 

	float			m_Color[4];		// precip color
	float			m_Lifetime;		// Precip lifetime
	float			m_InitialRamp;	// Initial ramp value
	float			m_Speed;		// Precip speed
	float			m_Width;		// Tracer width
	float			m_Remainder;	// particles we should render next time
	PrecipitationType_t	m_nPrecipType;			// Precip type
	float			m_flHalfScreenWidth;	// Precalculated each frame.

	float			m_flDensity;

	// Some state used in rendering and simulation
	// Used to modify the rain density and wind from the console
	static ConVar s_raindensity;
	static ConVar s_rainwidth;
	static ConVar s_rainlength;
	static ConVar s_rainspeed;

	static Vector s_WindVector;			// Stores the wind speed vector
	
	CUtlLinkedList<CPrecipitationParticle> m_Particles;
	CUtlVector<Vector> m_Splashes;

	CSmartPtr<AshDebrisEffect>		m_pAshEmitter;
	TimedEvent						m_tAshParticleTimer;
	TimedEvent						m_tAshParticleTraceTimer;
	bool							m_bActiveAshEmitter;
	Vector							m_vAshSpawnOrigin;

	int								m_iAshCount;

private:
	CClient_Precipitation( const CClient_Precipitation & ); // not defined, not accessible
};


// Just receive the normal data table stuff
IMPLEMENT_CLIENTCLASS_DT(CClient_Precipitation, DT_Precipitation, CPrecipitation)
	RecvPropInt( RECVINFO( m_nPrecipType ) )
END_RECV_TABLE()

static ConVar r_SnowEnable( "r_SnowEnable", "1", FCVAR_CHEAT, "Snow Enable" );
static ConVar r_SnowParticles( "r_SnowParticles", "500", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowInsideRadius( "r_SnowInsideRadius", "256", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowOutsideRadius( "r_SnowOutsideRadius", "1024", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowSpeedScale( "r_SnowSpeedScale", "1", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowPosScale( "r_SnowPosScale", "1", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowFallSpeed( "r_SnowFallSpeed", "1.5", FCVAR_CHEAT, "Snow fall speed scale." );
static ConVar r_SnowWindScale( "r_SnowWindScale", "0.0035", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowDebugBox( "r_SnowDebugBox", "0", FCVAR_CHEAT, "Snow Debug Boxes." );
static ConVar r_SnowZoomOffset( "r_SnowZoomOffset", "384.0f", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowZoomRadius( "r_SnowZoomRadius", "512.0f", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowStartAlpha( "r_SnowStartAlpha", "25", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowEndAlpha( "r_SnowEndAlpha", "255", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowColorRed( "r_SnowColorRed", "150", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowColorGreen( "r_SnowColorGreen", "175", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowColorBlue( "r_SnowColorBlue", "200", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowStartSize( "r_SnowStartSize", "1", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowEndSize( "r_SnowEndSize", "0", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowRayLength( "r_SnowRayLength", "8192.0f", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowRayRadius( "r_SnowRayRadius", "256", FCVAR_CHEAT, "Snow." );
static ConVar r_SnowRayEnable( "r_SnowRayEnable", "1", FCVAR_CHEAT, "Snow." );

void DrawPrecipitation()
{
	for ( int i=0; i < g_Precipitations.Count(); i++ )
	{
		g_Precipitations[i]->Render();
	}
}


//-----------------------------------------------------------------------------
// determines if a weather particle has hit something other than air
//-----------------------------------------------------------------------------
static bool IsInAir( const Vector& position )
{
	int contents = enginetrace->GetPointContents( position ); 	
	return (contents & CONTENTS_SOLID) == 0;
}


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

ConVar CClient_Precipitation::s_raindensity( "r_raindensity","0.001", FCVAR_CHEAT);
ConVar CClient_Precipitation::s_rainwidth( "r_rainwidth", "0.5", FCVAR_CHEAT );
ConVar CClient_Precipitation::s_rainlength( "r_rainlength", "0.1f", FCVAR_CHEAT );
ConVar CClient_Precipitation::s_rainspeed( "r_rainspeed", "600.0f", FCVAR_CHEAT );
ConVar r_rainalpha( "r_rainalpha", "0.4", FCVAR_CHEAT );
ConVar r_rainalphapow( "r_rainalphapow", "0.8", FCVAR_CHEAT );


Vector CClient_Precipitation::s_WindVector;		// Stores the wind speed vector


void CClient_Precipitation::OnDataChanged( DataUpdateType_t updateType )
{
	// Simulate every frame.
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		if ( m_nPrecipType == PRECIPITATION_TYPE_SNOWFALL )
		{
			SnowFallManagerCreate( this );
		}
	}

	m_flDensity = RemapVal( m_clrRender->a, 0, 255, 0, 0.001 );

	BaseClass::OnDataChanged( updateType );
}


void CClient_Precipitation::ClientThink()
{
	Simulate( gpGlobals->frametime );
}


//-----------------------------------------------------------------------------
//
// Utility methods for the various simulation functions
//
//-----------------------------------------------------------------------------
inline bool CClient_Precipitation::SimulateRain( CPrecipitationParticle* pParticle, float dt )
{
	if (GetRemainingLifetime( pParticle ) < 0.0f)
		return false;

	Vector vOldPos = pParticle->m_Pos;

	// Update position
	VectorMA( pParticle->m_Pos, dt, pParticle->m_Velocity, 
				pParticle->m_Pos );

		// wind blows rain around
	for ( int i = 0 ; i < 2 ; i++ )
		{
		if ( pParticle->m_Velocity[i] < s_WindVector[i] )
			{
			pParticle->m_Velocity[i] += ( 5 / pParticle->m_Mass );

			// clamp
			if ( pParticle->m_Velocity[i] > s_WindVector[i] )
				pParticle->m_Velocity[i] = s_WindVector[i];
			}
		else if (pParticle->m_Velocity[i] > s_WindVector[i] )
			{
			pParticle->m_Velocity[i] -= ( 5 / pParticle->m_Mass );

			// clamp.
			if ( pParticle->m_Velocity[i] < s_WindVector[i] )
				pParticle->m_Velocity[i] = s_WindVector[i];
		}
	}

		// No longer in the air? punt.
		if ( !IsInAir( pParticle->m_Pos ) )
		{
			// Possibly make a splash if we hit a water surface and it's in front of the view.
			if ( m_Splashes.Count() < 20 )
			{
				if ( RandomInt( 0, 100 ) < r_RainSplashPercentage.GetInt() )
				{
					trace_t trace;
					UTIL_TraceLine(vOldPos, pParticle->m_Pos, MASK_WATER, NULL, COLLISION_GROUP_NONE, &trace);
					if( trace.fraction < 1 )
					{
						m_Splashes.AddToTail( trace.endpos );
					}
				}
			}

			// Tell the framework it's time to remove the particle from the list
			return false;
		}

	// We still want this particle
	return true;
}


inline bool CClient_Precipitation::SimulateSnow( CPrecipitationParticle* pParticle, float dt )
{
	if ( IsInAir( pParticle->m_Pos ) )
	{
		// Update position
		VectorMA( pParticle->m_Pos, dt, pParticle->m_Velocity, 
					pParticle->m_Pos );

		// wind blows rain around
		for ( int i = 0 ; i < 2 ; i++ )
		{
			if ( pParticle->m_Velocity[i] < s_WindVector[i] )
			{
				pParticle->m_Velocity[i] += ( 5.0f / pParticle->m_Mass );

				// accelerating flakes get a trail
				pParticle->m_Ramp = 0.5f;

				// clamp
				if ( pParticle->m_Velocity[i] > s_WindVector[i] )
					pParticle->m_Velocity[i] = s_WindVector[i];
			}
			else if (pParticle->m_Velocity[i] > s_WindVector[i] )
			{
				pParticle->m_Velocity[i] -= ( 5.0f / pParticle->m_Mass );

				// accelerating flakes get a trail
				pParticle->m_Ramp = 0.5f;

				// clamp.
				if ( pParticle->m_Velocity[i] < s_WindVector[i] )
					pParticle->m_Velocity[i] = s_WindVector[i];
			}
		}

		return true;
	}


	// Kill the particle immediately!
	return false;
}


void CClient_Precipitation::Simulate( float dt )
{
	// NOTE: When client-side prechaching works, we need to remove this
	Precache();

	m_flHalfScreenWidth = (float)ScreenWidth() / 2;

	// Our sim methods needs dt	and wind vector
	if ( dt )
	{
		ComputeWindVector( );
	}

	if ( m_nPrecipType == PRECIPITATION_TYPE_ASH )
	{
		CreateAshParticle();
		return;
	}

	// The snow fall manager handles the simulation.
	if ( m_nPrecipType == PRECIPITATION_TYPE_SNOWFALL )
		return;

	// calculate the max amount of time it will take this flake to fall.
	// This works if we assume the wind doesn't have a z component
	if ( r_RainHack.GetInt() )
		m_Lifetime = (GetClientWorldEntity()->m_WorldMaxs[2] - GetClientWorldEntity()->m_WorldMins[2]) / m_Speed;
	else
		m_Lifetime = (WorldAlignMaxs()[2] - WorldAlignMins()[2]) / m_Speed;


	if ( !r_RainSimulate.GetInt() )
		return;

	CFastTimer timer;
	timer.Start();

	// Emit new particles
	EmitParticles( dt );

	// Simulate all the particles.
	int iNext;
	if ( m_nPrecipType == PRECIPITATION_TYPE_RAIN )
	{
		for ( int i=m_Particles.Head(); i != m_Particles.InvalidIndex(); i=iNext )
		{
			iNext = m_Particles.Next( i );
			if ( !SimulateRain( &m_Particles[i], dt ) )
				m_Particles.Remove( i );
		}
	}
	else if ( m_nPrecipType == PRECIPITATION_TYPE_SNOW )
	{
		for ( int i=m_Particles.Head(); i != m_Particles.InvalidIndex(); i=iNext )
		{
			iNext = m_Particles.Next( i );
			if ( !SimulateSnow( &m_Particles[i], dt ) )
				m_Particles.Remove( i );
		}
	}

	if ( r_RainProfile.GetInt() )
	{
		timer.End();
		engine->Con_NPrintf( 15, "Rain simulation: %du (%d tracers)", timer.GetDuration().GetMicroseconds(), m_Particles.Count() );
	}
}


//-----------------------------------------------------------------------------
// tracer rendering
//-----------------------------------------------------------------------------

inline void CClient_Precipitation::RenderParticle( CPrecipitationParticle* pParticle, CMeshBuilder &mb )
{
	float scale;
	Vector start, delta;

	if ( m_nPrecipType == PRECIPITATION_TYPE_ASH )
		 return;

	if ( m_nPrecipType == PRECIPITATION_TYPE_SNOWFALL )
		 return;


	// make streaks 0.1 seconds long, but prevent from going past end
	float lifetimeRemaining = GetRemainingLifetime( pParticle );
	if (lifetimeRemaining >= GetLength())
		scale = GetLength() * pParticle->m_Ramp;
	else
		scale = lifetimeRemaining * pParticle->m_Ramp;
	
	// NOTE: We need to do everything in screen space
	Vector3DMultiplyPosition( CurrentWorldToViewMatrix(), pParticle->m_Pos, start );
	if ( start.z > -1 )
		return;

	Vector3DMultiply( CurrentWorldToViewMatrix(), pParticle->m_Velocity, delta );

	// give a spiraling pattern to snow particles
	if ( m_nPrecipType == PRECIPITATION_TYPE_SNOW )
	{
		Vector spiral, camSpiral;
		float s, c;

		if ( pParticle->m_Mass > 1.0f )
		{
			SinCos( gpGlobals->curtime * M_PI * (1+pParticle->m_Mass * 0.1f) + 
					pParticle->m_Mass * 5.0f, &s , &c );

			// only spiral particles with a mass > 1, so some fall straight down
			spiral[0] = 28 * c;
			spiral[1] = 28 * s;
			spiral[2] = 0.0f;

			Vector3DMultiply( CurrentWorldToViewMatrix(), spiral, camSpiral );

			// X and Y are measured in world space; need to convert to camera space
			VectorAdd( start, camSpiral, start );
			VectorAdd( delta, camSpiral, delta );
		}

		// shrink the trails on spiraling flakes.
		pParticle->m_Ramp = 0.3f;
	}

	delta[0] *= scale;
	delta[1] *= scale;
	delta[2] *= scale;

	// See c_tracer.* for this method
	float flAlpha = r_rainalpha.GetFloat();
	float flWidth = GetWidth();

	float flScreenSpaceWidth = flWidth * m_flHalfScreenWidth / -start.z;
	if ( flScreenSpaceWidth < MIN_SCREENSPACE_RAIN_WIDTH )
	{
		// Make the rain tracer at least the min size, but fade its alpha the smaller it gets.
		flAlpha *= flScreenSpaceWidth / MIN_SCREENSPACE_RAIN_WIDTH;
		flWidth = MIN_SCREENSPACE_RAIN_WIDTH * -start.z / m_flHalfScreenWidth;
	}
	flAlpha = pow( flAlpha, r_rainalphapow.GetFloat() );

	float flColor[4] = { 1, 1, 1, flAlpha };
	Tracer_Draw( &mb, start, delta, flWidth, flColor, 1 );
}


void CClient_Precipitation::CreateWaterSplashes()
{
	for ( int i=0; i < m_Splashes.Count(); i++ )
	{
		Vector vSplash = m_Splashes[i];
		
		if ( CurrentViewForward().Dot( vSplash - CurrentViewOrigin() ) > 1 )
		{
			FX_WaterRipple( vSplash, g_flSplashScale, &g_vSplashColor, g_flSplashLifetime, g_flSplashAlpha );
		}
	}
	m_Splashes.Purge();
}


void CClient_Precipitation::Render()
{
	if ( !r_DrawRain.GetInt() )
		return;

	// Don't render in monitors or in reflections or refractions.
	if ( CurrentViewID() == VIEW_MONITOR )
		return;

	if ( view->GetDrawFlags() & (DF_RENDER_REFLECTION | DF_RENDER_REFRACTION) )
		return;

	if ( m_nPrecipType == PRECIPITATION_TYPE_ASH )
		return;

	if ( m_nPrecipType == PRECIPITATION_TYPE_SNOWFALL )
		return;

	// Create any queued up water splashes.
	CreateWaterSplashes();

	
	CFastTimer timer;
	timer.Start();

	CMatRenderContextPtr pRenderContext( materials );
	
	// We want to do our calculations in view space.
	VMatrix	tempView;
	pRenderContext->GetMatrix( MATERIAL_VIEW, &tempView );
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->LoadIdentity();

	// Force the user clip planes to use the old view matrix
	pRenderContext->EnableUserClipTransformOverride( true );
	pRenderContext->UserClipTransform( tempView );

	// Draw all the rain tracers.
	pRenderContext->Bind( m_MatHandle );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	if ( pMesh )
	{
		CMeshBuilder mb;
		mb.Begin( pMesh, MATERIAL_QUADS, m_Particles.Count() );

		for ( int i=m_Particles.Head(); i != m_Particles.InvalidIndex(); i=m_Particles.Next( i ) )
		{
			CPrecipitationParticle *p = &m_Particles[i];
			RenderParticle( p, mb );
		}

		mb.End( false, true );
	}

	pRenderContext->EnableUserClipTransformOverride( false );
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->LoadMatrix( tempView );

	if ( r_RainProfile.GetInt() )
	{
		timer.End();
		engine->Con_NPrintf( 16, "Rain render    : %du", timer.GetDuration().GetMicroseconds() );
	}
}


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------

CClient_Precipitation::CClient_Precipitation() : m_Remainder(0.0f)
{
	m_nPrecipType = PRECIPITATION_TYPE_RAIN;
	m_MatHandle = INVALID_MATERIAL_HANDLE;
	m_flHalfScreenWidth = 1;
	
	g_Precipitations.AddToTail( this );
}

CClient_Precipitation::~CClient_Precipitation()
{
	g_Precipitations.FindAndRemove( this );
	SnowFallManagerDestroy();
}

//-----------------------------------------------------------------------------
// Precache data
//-----------------------------------------------------------------------------

#define SNOW_SPEED	80.0f
#define RAIN_SPEED	425.0f

#define RAIN_TRACER_WIDTH 0.35f
#define SNOW_TRACER_WIDTH 0.7f

void CClient_Precipitation::Precache( )
{
	if ( !m_MatHandle )
	{
		// Compute precipitation emission speed
		switch( m_nPrecipType )
		{
		case PRECIPITATION_TYPE_SNOW:
			m_Speed	= SNOW_SPEED;
			m_MatHandle = materials->FindMaterial( "particle/snow", TEXTURE_GROUP_CLIENT_EFFECTS );
			m_InitialRamp = 0.6f;
			m_Width = SNOW_TRACER_WIDTH;
			break;

		case PRECIPITATION_TYPE_RAIN:
			Assert( m_nPrecipType == PRECIPITATION_TYPE_RAIN );
			m_Speed	= RAIN_SPEED;
			m_MatHandle = materials->FindMaterial( "particle/rain", TEXTURE_GROUP_CLIENT_EFFECTS );
			m_InitialRamp = 1.0f;
			m_Color[3] = 1.0f;	// make translucent
			m_Width = RAIN_TRACER_WIDTH;
			break;
		default:
			m_InitialRamp = 1.0f;
			m_Color[3] = 1.0f;	// make translucent
			break;
		}

		// Store off the color
		m_Color[0] = 1.0f;
		m_Color[1] = 1.0f;
		m_Color[2] = 1.0f;
	}
}


//-----------------------------------------------------------------------------
// Gets the tracer width and speed
//-----------------------------------------------------------------------------

inline float CClient_Precipitation::GetWidth() const
{
//	return m_Width;
	return s_rainwidth.GetFloat();
}

inline float CClient_Precipitation::GetLength() const
{
//	return m_Length;
	return s_rainlength.GetFloat();
}

inline float CClient_Precipitation::GetSpeed() const
{
//	return m_Speed;
	return s_rainspeed.GetFloat();
}


//-----------------------------------------------------------------------------
// Gets the remaining lifetime of the particle
//-----------------------------------------------------------------------------

inline float CClient_Precipitation::GetRemainingLifetime( CPrecipitationParticle* pParticle ) const
{
	float timeSinceSpawn = gpGlobals->curtime - pParticle->m_SpawnTime;
	return m_Lifetime - timeSinceSpawn;
}

//-----------------------------------------------------------------------------
// Creates a particle
//-----------------------------------------------------------------------------

inline CPrecipitationParticle* CClient_Precipitation::CreateParticle()
{
	int i = m_Particles.AddToTail();
	CPrecipitationParticle* pParticle = &m_Particles[i];

	pParticle->m_SpawnTime = gpGlobals->curtime;
	pParticle->m_Ramp = m_InitialRamp;

	return pParticle;
}


//-----------------------------------------------------------------------------
// Compute the emission area
//-----------------------------------------------------------------------------

bool CClient_Precipitation::ComputeEmissionArea( Vector& origin, Vector2D& size )
{
	// FIXME: Compute the precipitation area based on computational power
	float emissionSize = r_RainRadius.GetFloat();	// size of box to emit particles in

	Vector vMins = WorldAlignMins();
	Vector vMaxs = WorldAlignMaxs();
	if ( r_RainHack.GetInt() )
	{
		vMins = GetClientWorldEntity()->m_WorldMins;
		vMaxs = GetClientWorldEntity()->m_WorldMaxs;
	}

	// calculate a volume around the player to snow in. Intersect this big magic
	// box around the player with the volume of the current environmental ent.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	// Determine how much time it'll take a falling particle to hit the player
	float emissionHeight = MIN( vMaxs[2], pPlayer->GetAbsOrigin()[2] + 512 );
	float distToFall = emissionHeight - pPlayer->GetAbsOrigin()[2];
	float fallTime = distToFall / GetSpeed();
	
	// Based on the windspeed, figure out the center point of the emission
	Vector2D center;
	center[0] = pPlayer->GetAbsOrigin()[0] - fallTime * s_WindVector[0];
	center[1] = pPlayer->GetAbsOrigin()[1] - fallTime * s_WindVector[1];

	Vector2D lobound, hibound;
	lobound[0] = center[0] - emissionSize * 0.5f;
	lobound[1] = center[1] - emissionSize * 0.5f;
	hibound[0] = lobound[0] + emissionSize;
	hibound[1] = lobound[1] + emissionSize;

	// Cull non-intersecting.
	if ( ( vMaxs[0] < lobound[0] ) || ( vMaxs[1] < lobound[1] ) ||
		 ( vMins[0] > hibound[0] ) || ( vMins[1] > hibound[1] ) )
		return false;

	origin[0] = MAX( vMins[0], lobound[0] );
	origin[1] = MAX( vMins[1], lobound[1] );
	origin[2] = emissionHeight;

	hibound[0] = MIN( vMaxs[0], hibound[0] );
	hibound[1] = MIN( vMaxs[1], hibound[1] );

	size[0] = hibound[0] - origin[0];
	size[1] = hibound[1] - origin[1];

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pDebugName - 
// Output : AshDebrisEffect*
//-----------------------------------------------------------------------------
AshDebrisEffect* AshDebrisEffect::Create( const char *pDebugName )
{
	return new AshDebrisEffect( pDebugName );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float AshDebrisEffect::UpdateAlpha( const SimpleParticle *pParticle )
{
	return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
}

#define ASH_PARTICLE_NOISE 0x4

float AshDebrisEffect::UpdateRoll( SimpleParticle *pParticle, float timeDelta )
{
	float flRoll = CSimpleEmitter::UpdateRoll(pParticle, timeDelta );

	if ( pParticle->m_iFlags & ASH_PARTICLE_NOISE )
	{
		Vector vTempEntVel = pParticle->m_vecVelocity;
		float fastFreq = gpGlobals->curtime * 1.5;

		float s, c;
		SinCos( fastFreq, &s, &c );

		pParticle->m_Pos = ( pParticle->m_Pos + Vector(
			vTempEntVel[0] * timeDelta * s,
			vTempEntVel[1] * timeDelta * s, 0 ) );
	}

	return flRoll;
}

void CClient_Precipitation::CreateAshParticle( void )
{
		// Make sure the emitter is setup
	if ( m_pAshEmitter == NULL )
		{
		if ( ( m_pAshEmitter = AshDebrisEffect::Create( "ashtray" ) ) == NULL )
			return;

		m_tAshParticleTimer.Init( 192 );
		m_tAshParticleTraceTimer.Init( 15 );
		m_bActiveAshEmitter = false;
		m_iAshCount = 0;
		}

		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

		if ( pPlayer == NULL )
		return;

		Vector vForward;
		pPlayer->GetVectors( &vForward, NULL, NULL );
		vForward.z = 0.0f;

		float curTime = gpGlobals->frametime;

		Vector vPushOrigin;

		Vector absmins = WorldAlignMins();
		Vector absmaxs = WorldAlignMaxs();

		//15 Traces a second.
	while ( m_tAshParticleTraceTimer.NextEvent( curTime ) )
		{
			trace_t tr;

			Vector vTraceStart = pPlayer->EyePosition();
			Vector vTraceEnd = pPlayer->EyePosition() + vForward * MAX_TRACE_LENGTH;

			UTIL_TraceLine( vTraceStart, vTraceEnd, MASK_SHOT_HULL & (~CONTENTS_GRATE), pPlayer, COLLISION_GROUP_NONE, &tr );

			//debugoverlay->AddLineOverlay( vTraceStart, tr.endpos, 255, 0, 0, 0, 0.2 );

			if ( tr.fraction != 1.0f )
			{
				trace_t tr2;

				UTIL_TraceModel( vTraceStart, tr.endpos, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), this, COLLISION_GROUP_NONE, &tr2 );

				if ( tr2.m_pEnt == this )
				{
				m_bActiveAshEmitter = true;

					if ( tr2.startsolid == false )
					{
					m_vAshSpawnOrigin = tr2.endpos + vForward * 256;
					}
					else
					{
					m_vAshSpawnOrigin = vTraceStart;
					}
				}
				else
				{
				m_bActiveAshEmitter = false;
				}
			}
		}

	if ( m_bActiveAshEmitter == false )
		 return;

		Vector vecVelocity = pPlayer->GetAbsVelocity();


		float flVelocity = VectorNormalize( vecVelocity );
	Vector offset = m_vAshSpawnOrigin;

	m_pAshEmitter->SetSortOrigin( offset );

		PMaterialHandle	hMaterial[4];
		hMaterial[0] = ParticleMgr()->GetPMaterial( "effects/fleck_ash1" );
		hMaterial[1] = ParticleMgr()->GetPMaterial( "effects/fleck_ash2" );
		hMaterial[2] = ParticleMgr()->GetPMaterial( "effects/fleck_ash3" );
		hMaterial[3] = ParticleMgr()->GetPMaterial( "effects/ember_swirling001" );

		SimpleParticle	*pParticle;

		Vector vSpawnOrigin = vec3_origin;

		if ( flVelocity > 0 )
		{
			vSpawnOrigin = ( vForward * 256 ) + ( vecVelocity * ( flVelocity * 2 ) );
		}

		// Add as many particles as we need
	while ( m_tAshParticleTimer.NextEvent( curTime ) )
		{
			int iRandomAltitude = RandomInt( 0, 128 );

		offset = m_vAshSpawnOrigin + vSpawnOrigin + RandomVector( -256, 256 );
		offset.z = m_vAshSpawnOrigin.z + iRandomAltitude;

			if  (  offset[0] > absmaxs[0]
			|| offset[1] > absmaxs[1]
			|| offset[2] > absmaxs[2]
			|| offset[0] < absmins[0]
			|| offset[1] < absmins[1]
			|| offset[2] < absmins[2] )
				continue;

		m_iAshCount++;

			bool bEmberTime = false;
			
		if ( m_iAshCount >= 250 )
			{
				bEmberTime = true;
			m_iAshCount = 0;
			}

			int iRandom = random->RandomInt(0,2);

			if ( bEmberTime == true )
			{
			offset = m_vAshSpawnOrigin + (vForward * 256) + RandomVector( -128, 128 );
				offset.z = pPlayer->EyePosition().z + RandomFloat( -16, 64 );

				iRandom = 3;
			}

		pParticle = (SimpleParticle *) m_pAshEmitter->AddParticle( sizeof(SimpleParticle), hMaterial[iRandom], offset );

			if (pParticle == NULL)
				continue; 

			pParticle->m_flLifetime	= 0.0f;
			pParticle->m_flDieTime	= RemapVal( iRandomAltitude, 0, 128, 4, 8 );

			if ( bEmberTime == true )
			{
				Vector vGoal = pPlayer->EyePosition() + RandomVector( -64, 64 );
				Vector vDir = vGoal - offset;
				VectorNormalize( vDir );

				pParticle->m_vecVelocity = vDir * 75;
				pParticle->m_flDieTime = 2.5f;
			}
			else
			{
				pParticle->m_vecVelocity = Vector( RandomFloat( -20.0f, 20.0f ), RandomFloat( -20.0f, 20.0f ), RandomFloat( -10, -15 ) );
			}

			float color = random->RandomInt( 125, 225 );
			pParticle->m_uchColor[0] = color;
			pParticle->m_uchColor[1] = color;
			pParticle->m_uchColor[2] = color;

			pParticle->m_uchStartSize	= 1;
			pParticle->m_uchEndSize		= 1;

			pParticle->m_uchStartAlpha	= 255;

			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -0.15f, 0.15f );

			pParticle->m_iFlags			= SIMPLE_PARTICLE_FLAG_WINDBLOWN;

			if ( random->RandomInt( 0, 10 ) <= 1 )
			{
				pParticle->m_iFlags |= ASH_PARTICLE_NOISE;
			}
		}
	}

void CClient_Precipitation::CreateRainOrSnowParticle( Vector vSpawnPosition, Vector vVelocity )
{
	// Create the particle
	CPrecipitationParticle* p = CreateParticle();
	if (!p) 
		return;

	VectorCopy( vVelocity, p->m_Velocity );
	p->m_Pos = vSpawnPosition;

	p->m_Velocity[ 0 ] += random->RandomFloat(-r_RainSideVel.GetInt(), r_RainSideVel.GetInt());
	p->m_Velocity[ 1 ] += random->RandomFloat(-r_RainSideVel.GetInt(), r_RainSideVel.GetInt());

	p->m_Mass = random->RandomFloat( 0.5, 1.5 );
}

//-----------------------------------------------------------------------------
// emit the precipitation particles
//-----------------------------------------------------------------------------

void CClient_Precipitation::EmitParticles( float fTimeDelta )
{
	Vector2D size;
	Vector vel, org;

		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pPlayer )
		return;
		Vector vPlayerCenter = pPlayer->WorldSpaceCenter();

		// Compute where to emit
	if (!ComputeEmissionArea( org, size ))
		return;

		// clamp this to prevent creating a bunch of rain or snow at one time.
		if( fTimeDelta > 0.075f )
			fTimeDelta = 0.075f;

		// FIXME: Compute the precipitation density based on computational power
	float density = m_flDensity;

		if (density > 0.01f) 
			density = 0.01f;

		// Compute number of particles to emit based on precip density and emission area and dt
		float fParticles = size[0] * size[1] * density * fTimeDelta + m_Remainder; 
		int cParticles = (int)fParticles;
		m_Remainder = fParticles - cParticles;

		// calculate the max amount of time it will take this flake to fall.
		// This works if we assume the wind doesn't have a z component
		VectorCopy( s_WindVector, vel );
		vel[2] -= GetSpeed();

		// Emit all the particles
		for ( int i = 0 ; i < cParticles ; i++ )
		{
			Vector vParticlePos = org;
			vParticlePos[ 0 ] += size[ 0 ] * random->RandomFloat(0, 1);
			vParticlePos[ 1 ] += size[ 1 ] * random->RandomFloat(0, 1);

			// Figure out where the particle should lie in Z by tracing a line from the player's height up to the 
			// desired height and making sure it doesn't hit a wall.
			Vector vPlayerHeight = vParticlePos;
			vPlayerHeight.z = vPlayerCenter.z;

			trace_t trace;
			UTIL_TraceLine( vPlayerHeight, vParticlePos, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &trace );
			if ( trace.fraction < 1 )
			{
				// If we hit a brush, then don't spawn the particle.
				if ( trace.surface.flags & SURF_SKY )
				{
					vParticlePos = trace.endpos;
				}
				else
				{
					continue;
				}
			}

		CreateRainOrSnowParticle( vParticlePos, vel );
	}
}


//-----------------------------------------------------------------------------
// Computes the wind vector
//-----------------------------------------------------------------------------

void CClient_Precipitation::ComputeWindVector( )
{
	// Compute the wind direction
	QAngle windangle( 0, cl_winddir.GetFloat(), 0 );	// used to turn wind yaw direction into a vector

	// Randomize the wind angle and speed slightly to get us a little variation
	windangle[1] = windangle[1] + random->RandomFloat( -10, 10 );
	float windspeed = cl_windspeed.GetFloat() * (1.0 + random->RandomFloat( -0.2, 0.2 ));

	AngleVectors( windangle, &s_WindVector );
	VectorScale( s_WindVector, windspeed, s_WindVector );
}


CHandle<CClient_Precipitation> g_pPrecipHackEnt;

class CPrecipHack : public CAutoGameSystemPerFrame
{
public:
	CPrecipHack( char const *name ) : CAutoGameSystemPerFrame( name )
	{
		m_bLevelInitted = false;
	}

	virtual void LevelInitPostEntity()
	{
		if ( r_RainHack.GetInt() )
		{
			CClient_Precipitation *pPrecipHackEnt = new CClient_Precipitation;
			pPrecipHackEnt->InitializeAsClientEntity( NULL, RENDER_GROUP_TRANSLUCENT_ENTITY );
			g_pPrecipHackEnt = pPrecipHackEnt;
		}
		m_bLevelInitted = true;
	}
	
	virtual void LevelShutdownPreEntity()
	{
		if ( r_RainHack.GetInt() && g_pPrecipHackEnt )
		{
			g_pPrecipHackEnt->Release();
		}
		m_bLevelInitted = false;
	}

	virtual void Update( float frametime )
	{
		// Handle changes to the cvar at runtime.
		if ( m_bLevelInitted )
		{
			if ( r_RainHack.GetInt() && !g_pPrecipHackEnt )
				LevelInitPostEntity();
			else if ( !r_RainHack.GetInt() && g_pPrecipHackEnt )
				LevelShutdownPreEntity();
		}
	}

	bool m_bLevelInitted;
};
CPrecipHack g_PrecipHack( "CPrecipHack" );

#else

void DrawPrecipitation()
{
}

#endif	// _XBOX

//-----------------------------------------------------------------------------
// EnvWind - global wind info
//-----------------------------------------------------------------------------
class C_EnvWind : public C_BaseEntity
{
public:
	C_EnvWind();

	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_EnvWind, C_BaseEntity );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw( void ) { return false; }

	virtual void	ClientThink( );

private:
	C_EnvWind( const C_EnvWind & );

	CEnvWindShared m_EnvWindShared;
};

// Receive datatables
BEGIN_RECV_TABLE_NOBASE(CEnvWindShared, DT_EnvWindShared)
	RecvPropInt		(RECVINFO(m_iMinWind)),
	RecvPropInt		(RECVINFO(m_iMaxWind)),
	RecvPropInt		(RECVINFO(m_iMinGust)),
	RecvPropInt		(RECVINFO(m_iMaxGust)),
	RecvPropFloat	(RECVINFO(m_flMinGustDelay)),
	RecvPropFloat	(RECVINFO(m_flMaxGustDelay)),
	RecvPropInt		(RECVINFO(m_iGustDirChange)),
	RecvPropInt		(RECVINFO(m_iWindSeed)),
	RecvPropInt		(RECVINFO(m_iInitialWindDir)),
	RecvPropFloat	(RECVINFO(m_flInitialWindSpeed)),
	RecvPropFloat	(RECVINFO(m_flStartTime)),
	RecvPropFloat	(RECVINFO(m_flGustDuration)),
//	RecvPropInt		(RECVINFO(m_iszGustSound)),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_EnvWind, DT_EnvWind, CEnvWind )
	RecvPropDataTable(RECVINFO_DT(m_EnvWindShared), 0, &REFERENCE_RECV_TABLE(DT_EnvWindShared)),
END_RECV_TABLE()


C_EnvWind::C_EnvWind()
{
}

//-----------------------------------------------------------------------------
// Post data update!
//-----------------------------------------------------------------------------
void C_EnvWind::OnDataChanged( DataUpdateType_t updateType )
{
	// Whenever we get an update, reset the entire state.
	// Note that the fields have already been stored by the datatables,
	// but there's still work to be done in the init block
	m_EnvWindShared.Init( entindex(), m_EnvWindShared.m_iWindSeed, 
		m_EnvWindShared.m_flStartTime, m_EnvWindShared.m_iInitialWindDir,
		m_EnvWindShared.m_flInitialWindSpeed );

	SetNextClientThink(0.0f);

	BaseClass::OnDataChanged( updateType );
}

void C_EnvWind::ClientThink( )
{
	// Update the wind speed
	float flNextThink = m_EnvWindShared.WindThink( gpGlobals->curtime );
	SetNextClientThink(flNextThink);
}



//==================================================
// EmberParticle
//==================================================

class CEmberEmitter : public CSimpleEmitter
{
public:
							CEmberEmitter( const char *pDebugName );
	static CSmartPtr<CEmberEmitter>	Create( const char *pDebugName );
	virtual void			UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	virtual Vector			UpdateColor( const SimpleParticle *pParticle );

private:
							CEmberEmitter( const CEmberEmitter & );
};


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
CEmberEmitter::CEmberEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName )
{
}


CSmartPtr<CEmberEmitter> CEmberEmitter::Create( const char *pDebugName )
{
	return new CEmberEmitter( pDebugName );
}


void CEmberEmitter::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
	float	speed = VectorNormalize( pParticle->m_vecVelocity );
	Vector	offset;

	speed -= ( 1.0f * timeDelta );

	offset.Random( -0.025f, 0.025f );
	offset[2] = 0.0f;

	pParticle->m_vecVelocity += offset;
	VectorNormalize( pParticle->m_vecVelocity );

	pParticle->m_vecVelocity *= speed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
Vector CEmberEmitter::UpdateColor( const SimpleParticle *pParticle )
{
	Vector	color;
	float	ramp = 1.0f - ( pParticle->m_flLifetime / pParticle->m_flDieTime );

	color[0] = ( (float) pParticle->m_uchColor[0] * ramp ) / 255.0f;
	color[1] = ( (float) pParticle->m_uchColor[1] * ramp ) / 255.0f;
	color[2] = ( (float) pParticle->m_uchColor[2] * ramp ) / 255.0f;

	return color;
}

//==================================================
// C_Embers
//==================================================

class C_Embers : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_Embers, C_BaseEntity );

					C_Embers();
					~C_Embers();

	void	Start( void );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw( void );
	virtual void	AddEntity( void );

	//Server-side
	int		m_nDensity;
	int		m_nLifetime;
	int		m_nSpeed;
	bool	m_bEmit;

protected:

	void	SpawnEmber( void );

	PMaterialHandle		m_hMaterial;
	TimedEvent			m_tParticleSpawn;
	CSmartPtr<CEmberEmitter> m_pEmitter;

};

//Receive datatable
IMPLEMENT_CLIENTCLASS_DT( C_Embers, DT_Embers, CEmbers )
	RecvPropInt( RECVINFO( m_nDensity ) ),
	RecvPropInt( RECVINFO( m_nLifetime ) ),
	RecvPropInt( RECVINFO( m_nSpeed ) ),
	RecvPropInt( RECVINFO( m_bEmit ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
C_Embers::C_Embers()
{
	m_pEmitter = CEmberEmitter::Create( "C_Embers" );
}

C_Embers::~C_Embers()
{
}

void C_Embers::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_pEmitter->SetSortOrigin( GetAbsOrigin() );

		Start();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_Embers::ShouldDraw()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Embers::Start( void )
{
	//Various setup info
	m_tParticleSpawn.Init( m_nDensity );
	
	m_hMaterial	= m_pEmitter->GetPMaterial( "particle/fire" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Embers::AddEntity( void ) 
{
	if ( m_bEmit == false )
		return;

	float tempDelta = gpGlobals->frametime;

	while( m_tParticleSpawn.NextEvent( tempDelta ) )
	{
		SpawnEmber();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Embers::SpawnEmber( void )
{
	Vector	offset, mins, maxs;
	
	modelinfo->GetModelBounds( GetModel(), mins, maxs );

	//Setup our spawn position
	offset[0] = random->RandomFloat( mins[0], maxs[0] );
	offset[1] = random->RandomFloat( mins[1], maxs[1] );
	offset[2] = random->RandomFloat( mins[2], maxs[2] );

	//Spawn the particle
	SimpleParticle	*sParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof( SimpleParticle ), m_hMaterial, offset );

	if (sParticle == NULL)
		return;

	float	cScale = random->RandomFloat( 0.75f, 1.0f );

	//Set it up
	sParticle->m_flLifetime = 0.0f;
	sParticle->m_flDieTime	= m_nLifetime;

	sParticle->m_uchColor[0]	= m_clrRender->r * cScale;
	sParticle->m_uchColor[1]	= m_clrRender->g * cScale;
	sParticle->m_uchColor[2]	= m_clrRender->b * cScale;
	sParticle->m_uchStartAlpha	= 255;
	sParticle->m_uchEndAlpha	= 0;
	sParticle->m_uchStartSize	= 1;
	sParticle->m_uchEndSize		= 0;
	sParticle->m_flRollDelta	= 0;
	sParticle->m_flRoll			= 0;

	//Set the velocity
	Vector	velocity;

	AngleVectors( GetAbsAngles(), &velocity );

	sParticle->m_vecVelocity = velocity * m_nSpeed;

	sParticle->m_vecVelocity[0]	+= random->RandomFloat( -(m_nSpeed/8), (m_nSpeed/8) );
	sParticle->m_vecVelocity[1]	+= random->RandomFloat( -(m_nSpeed/8), (m_nSpeed/8) );
	sParticle->m_vecVelocity[2]	+= random->RandomFloat( -(m_nSpeed/8), (m_nSpeed/8) );

	UpdateVisibility();
}

//-----------------------------------------------------------------------------
// Quadratic spline beam effect 
//-----------------------------------------------------------------------------
#include "beamdraw.h"

class C_QuadraticBeam : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_QuadraticBeam, C_BaseEntity );

	//virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw( void ) { return true; }
	virtual int		DrawModel( int );

	virtual void	GetRenderBounds( Vector& mins, Vector& maxs )
	{
		ClearBounds( mins, maxs );
		AddPointToBounds( vec3_origin, mins, maxs );
		AddPointToBounds( m_targetPosition, mins, maxs );
		AddPointToBounds( m_controlPosition, mins, maxs );
		mins -= GetRenderOrigin();
		maxs -= GetRenderOrigin();
	}

protected:

	Vector		m_targetPosition;
	Vector		m_controlPosition;
	float		m_scrollRate;
	float		m_flWidth;
};

//Receive datatable
IMPLEMENT_CLIENTCLASS_DT( C_QuadraticBeam, DT_QuadraticBeam, CEnvQuadraticBeam )
	RecvPropVector( RECVINFO(m_targetPosition) ),
	RecvPropVector( RECVINFO(m_controlPosition) ),
	RecvPropFloat( RECVINFO(m_scrollRate) ),
	RecvPropFloat( RECVINFO(m_flWidth) ),
END_RECV_TABLE()

Vector Color32ToVector( const color32 &color )
{
	return Vector( color.r * (1.0/255.0f), color.g * (1.0/255.0f), color.b * (1.0/255.0f) );
}

int	C_QuadraticBeam::DrawModel( int )
{
	Draw_SetSpriteTexture( GetModel(), 0, GetRenderMode() );
	Vector color = Color32ToVector( GetRenderColor() );
	DrawBeamQuadratic( GetRenderOrigin(), m_controlPosition, m_targetPosition, m_flWidth, color, gpGlobals->curtime*m_scrollRate );
	return 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class SnowFallEffect : public CSimpleEmitter
{
public:

	SnowFallEffect( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	static SnowFallEffect* Create( const char *pDebugName )
	{
		return new SnowFallEffect( pDebugName );
	}

	void UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		float flSpeed = VectorNormalize( pParticle->m_vecVelocity );
		flSpeed -= timeDelta;

		pParticle->m_vecVelocity.x += RandomFloat( -0.025f, 0.025f );
		pParticle->m_vecVelocity.y += RandomFloat( -0.025f, 0.025f );
		VectorNormalize( pParticle->m_vecVelocity );

		pParticle->m_vecVelocity *= flSpeed;

		Vector vecWindVelocity;
		GetWindspeedAtTime( gpGlobals->curtime, vecWindVelocity );
		pParticle->m_vecVelocity += ( vecWindVelocity * r_SnowWindScale.GetFloat() );
	}

	void SimulateParticles( CParticleSimulateIterator *pIterator )
	{
		float timeDelta = pIterator->GetTimeDelta();
	
		SimpleParticle *pParticle = (SimpleParticle*)pIterator->GetFirst();
		while ( pParticle )
		{
			//Update velocity
			UpdateVelocity( pParticle, timeDelta );
			pParticle->m_Pos += pParticle->m_vecVelocity * timeDelta;

			//Should this particle die?
			pParticle->m_flLifetime += timeDelta;
			UpdateRoll( pParticle, timeDelta );

			if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
			{
				pIterator->RemoveParticle( pParticle );
			}
			else if ( !IsInAir( pParticle->m_Pos ) )
			{
				pIterator->RemoveParticle( pParticle );
			}

			pParticle = (SimpleParticle*)pIterator->GetNext();
		}
	}

	int	GetParticleCount( void )
	{	
		return GetBinding().GetNumActiveParticles(); 
	}

	void SetBounds( const Vector &vecMin, const Vector &vecMax )
	{
		GetBinding().SetBBox( vecMin, vecMax, true );
	}

	bool IsTransparent( void )		{ return false; }

private:

	SnowFallEffect( const SnowFallEffect & );
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CSnowFallManager : public C_BaseEntity
{
public:

	CSnowFallManager();
	~CSnowFallManager();

	bool CreateEmitter( void );

	void SpawnClientEntity( void );
	void ClientThink();

	void AddSnowFallEntity( CClient_Precipitation *pSnowEntity );

	// Snow Effect
	enum
	{
		SNOWFALL_NONE = 0,
		SNOWFALL_AROUND_PLAYER,
		SNOWFALL_IN_ENTITY,
	};

	bool IsTransparent( void )		{ return false; }

private:

	bool CreateSnowFallEmitter( void );
	void CreateSnowFall( void );
	void CreateSnowFallParticles( float flCurrentTime, float flRadius, const Vector &vecEyePos, const Vector &vecForward, float flZoomScale );
	void CreateOutsideVolumeSnowParticles( float flCurrentTime, float flRadius, float flZoomScale );
	void CreateInsideVolumeSnowParticles( float flCurrentTime, float flRadius, const Vector &vecEyePos, const Vector &vecForward, float flZoomScale );
	void CreateSnowParticlesSphere( float flRadius );
	void CreateSnowParticlesRay( float flRadius, const Vector &vecEyePos, const Vector &vecForward );
	void CreateSnowFallParticle( const Vector &vecParticleSpawn, int iBBox );

	int StandingInSnowVolume( Vector &vecPoint );
	void FindSnowVolumes( Vector &vecCenter, float flRadius, Vector &vecEyePos, Vector &vecForward );

	void UpdateBounds( const Vector &vecSnowMin, const Vector &vecSnowMax );

private:

	enum { MAX_SNOW_PARTICLES = 500 };
	enum { MAX_SNOW_LIST = 32 };

	TimedEvent						m_tSnowFallParticleTimer;
	TimedEvent						m_tSnowFallParticleTraceTimer;

	int								m_iSnowFallArea;
	CSmartPtr<SnowFallEffect>		m_pSnowFallEmitter;
	Vector							m_vecSnowFallEmitOrigin;
	float							m_flSnowRadius;

	Vector							m_vecMin;
	Vector							m_vecMax;

	int								m_nActiveSnowCount;
	int								m_aActiveSnow[MAX_SNOW_LIST];

	bool							m_bRayParticles;

	struct SnowFall_t
	{
		PMaterialHandle			m_hMaterial;
		CClient_Precipitation	*m_pEntity;
		SnowFallEffect			*m_pEffect;
		Vector					m_vecMin;
		Vector					m_vecMax;
	};

	CUtlVector<SnowFall_t>		m_aSnow;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSnowFallManager::CSnowFallManager( void )
{
	m_iSnowFallArea = SNOWFALL_NONE;
	m_pSnowFallEmitter = NULL;
	m_vecSnowFallEmitOrigin.Init();
	m_flSnowRadius = 0.0f;
	m_vecMin.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	m_vecMax.Init( FLT_MIN, FLT_MIN, FLT_MIN );
	m_nActiveSnowCount = 0;
	m_aSnow.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSnowFallManager::~CSnowFallManager( void )
{
	m_aSnow.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSnowFallManager::CreateEmitter( void )
{
	return CreateSnowFallEmitter();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSnowFallManager::SpawnClientEntity( void )
{
	m_tSnowFallParticleTimer.Init( 500 );
	m_tSnowFallParticleTraceTimer.Init( 6 );
	m_iSnowFallArea = SNOWFALL_NONE;

	// Have the Snow Fall Manager think for all the snow fall entities.
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSnowFallManager::CreateSnowFallEmitter( void )
{
	if ( ( m_pSnowFallEmitter = SnowFallEffect::Create( "snowfall" ) ) == NULL )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSnowFallManager::ClientThink( void )
{
	if ( !r_SnowEnable.GetBool() )
		return;

	// Make sure we have a snow fall emitter.
	if ( !m_pSnowFallEmitter )
	{
		if ( !CreateSnowFallEmitter() )
			return;
	}

	CreateSnowFall();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSnowEntity - 
//-----------------------------------------------------------------------------
void CSnowFallManager::AddSnowFallEntity( CClient_Precipitation *pSnowEntity )
{
	if ( !pSnowEntity )
		return;

	int nSnowCount = m_aSnow.Count();
	int iSnow = 0;
	for ( iSnow = 0; iSnow < nSnowCount; ++iSnow )
	{
		if ( m_aSnow[iSnow].m_pEntity == pSnowEntity )
			break;
	}

	if ( iSnow != nSnowCount )
		return;

	iSnow = m_aSnow.AddToTail();
	m_aSnow[iSnow].m_pEntity = pSnowEntity;
	m_aSnow[iSnow].m_pEffect = SnowFallEffect::Create( "snowfall" );
	m_aSnow[iSnow].m_hMaterial = ParticleMgr()->GetPMaterial( "particle/snow" );

	VectorCopy( pSnowEntity->WorldAlignMins(), m_aSnow[iSnow].m_vecMin );
	VectorCopy( pSnowEntity->WorldAlignMaxs(), m_aSnow[iSnow].m_vecMax );

	UpdateBounds( m_aSnow[iSnow].m_vecMin, m_aSnow[iSnow].m_vecMax );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSnowFallManager::UpdateBounds( const Vector &vecSnowMin, const Vector &vecSnowMax )
{
	int iAxis = 0;
	for ( iAxis = 0; iAxis < 3; ++iAxis )
	{
		if ( vecSnowMin[iAxis] < m_vecMin[iAxis] )
		{
			m_vecMin[iAxis] = vecSnowMin[iAxis];
		}

		if ( vecSnowMax[iAxis] > m_vecMax[iAxis] )
		{
			m_vecMax[iAxis] = vecSnowMax[iAxis];
		}
	}

	Assert( m_pSnowFallEmitter );
	m_pSnowFallEmitter->SetBounds( m_vecMin, m_vecMax );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecPoint - 
// Output : int
//-----------------------------------------------------------------------------
int CSnowFallManager::StandingInSnowVolume( Vector &vecPoint )
{
	trace_t traceSnow;

	int nSnowCount = m_aSnow.Count();
	int iSnow = 0;
	for ( iSnow = 0; iSnow < nSnowCount; ++iSnow )
	{
		UTIL_TraceModel( vecPoint, vecPoint, vec3_origin, vec3_origin, static_cast<C_BaseEntity*>( m_aSnow[iSnow].m_pEntity ), COLLISION_GROUP_NONE, &traceSnow );
		if ( traceSnow.startsolid )
			return iSnow;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecCenter - 
//			flRadius - 
//-----------------------------------------------------------------------------
void CSnowFallManager::FindSnowVolumes( Vector &vecCenter, float flRadius, Vector &vecEyePos, Vector &vecForward )
{
	// Reset.
	m_nActiveSnowCount = 0;
	m_bRayParticles = false;

	int nSnowCount = m_aSnow.Count();
	int iSnow = 0;
	for ( iSnow = 0; iSnow < nSnowCount; ++iSnow )
	{
		// Check to see if the volume is in the PVS.
		bool bInPVS = g_pClientLeafSystem->IsRenderableInPVS( m_aSnow[iSnow].m_pEntity->GetClientRenderable() );
		if ( !bInPVS )
			continue;

		// Check to see if a snow volume is inside the given radius.
		if ( IsBoxIntersectingSphere( m_aSnow[iSnow].m_vecMin, m_aSnow[iSnow].m_vecMax, vecCenter, flRadius ) )
		{
			m_aActiveSnow[m_nActiveSnowCount] = iSnow;
			++m_nActiveSnowCount;
			if ( m_nActiveSnowCount >= MAX_SNOW_LIST )
			{
				DevWarning( 1, "Max Active Snow Volume Count!\n" );
				break;
			}
		}
		// Check to see if a snow volume is outside of the sphere radius, but is along line-of-sight.
		else		
		{
			CBaseTrace trace;
			Vector vecNewForward;
			vecNewForward = vecForward * r_SnowRayLength.GetFloat();
			vecNewForward.z = 0.0f;
			IntersectRayWithBox( vecEyePos, vecNewForward, m_aSnow[iSnow].m_vecMin, m_aSnow[iSnow].m_vecMax, 0.325f, &trace );
			if ( trace.fraction < 1.0f )
			{
				m_aActiveSnow[m_nActiveSnowCount] = iSnow;
				++m_nActiveSnowCount;
				if ( m_nActiveSnowCount >= MAX_SNOW_LIST )
				{
					DevWarning( 1, "Max Active Snow Volume Count!\n" );
					break;
				}

				m_bRayParticles = true;
			}
		}
	}

	// Debugging code!
#ifdef _DEBUG
	if ( r_SnowDebugBox.GetFloat() != 0.0f )
	{
		for ( iSnow = 0; iSnow < m_nActiveSnowCount; ++iSnow )
		{
			Vector vecMin, vecMax;
			vecCenter = ( m_aSnow[iSnow].m_vecMin, m_aSnow[iSnow].m_vecMax ) * 0.5;
			vecMin = m_aSnow[iSnow].m_vecMin - vecCenter;
			vecMax = m_aSnow[iSnow].m_vecMax - vecCenter;
			if ( debugoverlay )
			{
				debugoverlay->AddBoxOverlay( vecCenter, vecMin, vecMax, QAngle( 0, 0, 0 ), 200, 0, 0, 25, r_SnowDebugBox.GetFloat() );
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSnowFallManager::CreateSnowFall( void )
{
#if 1
	VPROF_BUDGET( "SnowFall", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
#endif

	// Check to see if we have a local player before starting the snow around a local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer == NULL )
		return;

	// Get the current frame time.
	float flCurrentTime = gpGlobals->frametime;

	// Get the players data to determine where the snow emitter should reside.
	VectorCopy( pPlayer->EyePosition(), m_vecSnowFallEmitOrigin );
	Vector vecForward;
	pPlayer->GetVectors( &vecForward, NULL, NULL );
	vecForward.z = 0.0f;
	Vector vecVelocity = pPlayer->GetAbsVelocity();
	float flSpeed = VectorNormalize( vecVelocity );
	m_vecSnowFallEmitOrigin += ( vecForward * ( 64.0f + ( flSpeed * 0.4f * r_SnowPosScale.GetFloat() ) ) );
	m_vecSnowFallEmitOrigin += ( vecVelocity * ( flSpeed * 1.25f * r_SnowSpeedScale.GetFloat() ) );

	// Check to see if the player is zoomed.
	bool bZoomed = ( pPlayer->GetFOV() != pPlayer->GetDefaultFOV() );
	float flZoomScale = 1.0f;
	if ( bZoomed )
	{
		flZoomScale = pPlayer->GetDefaultFOV() / pPlayer->GetFOV();
		flZoomScale *= 0.5f;
	}

	// Time to test for a snow volume yet?  (Only do this 6 times a second!)
	if ( m_tSnowFallParticleTraceTimer.NextEvent( flCurrentTime ) )
	{
		// Reset the active snow emitter.
		m_iSnowFallArea = SNOWFALL_NONE;

		// Set the trace start and the emit origin.
		Vector vecTraceStart;
		VectorCopy( pPlayer->EyePosition(), vecTraceStart );

		int iSnowVolume = StandingInSnowVolume( vecTraceStart );
		if ( iSnowVolume != -1 )
		{
			m_flSnowRadius = r_SnowInsideRadius.GetFloat() + ( flSpeed * 0.5f );
			m_iSnowFallArea = SNOWFALL_AROUND_PLAYER;
		}
		else
		{
			m_flSnowRadius = r_SnowOutsideRadius.GetFloat();
		}

		float flRadius = m_flSnowRadius;
		if ( bZoomed )
		{
			if ( m_iSnowFallArea == SNOWFALL_AROUND_PLAYER )
			{
				flRadius = r_SnowOutsideRadius.GetFloat() * flZoomScale;
			}
			else
			{
				flRadius *= flZoomScale;
			}
		}

		Vector vecEyePos = pPlayer->EyePosition();
		FindSnowVolumes( m_vecSnowFallEmitOrigin, flRadius, vecEyePos, vecForward );
		if ( m_nActiveSnowCount != 0 && m_iSnowFallArea != SNOWFALL_AROUND_PLAYER )
		{
			// We found an active snow emitter.
			m_iSnowFallArea = SNOWFALL_IN_ENTITY;

		}
	}

	if ( m_iSnowFallArea == SNOWFALL_NONE )
		 return;

	// Set the origin in the snow emitter.
	m_pSnowFallEmitter->SetSortOrigin( m_vecSnowFallEmitOrigin );

	// Create snow fall particles.
	CreateSnowFallParticles( flCurrentTime, m_flSnowRadius, pPlayer->EyePosition(), vecForward, flZoomScale );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flCurrentTime - 
//			flRadius - 
//			&vecEyePos - 
//			&vecForward - 
//			flZoomScale - 
//-----------------------------------------------------------------------------
void CSnowFallManager::CreateSnowFallParticles( float flCurrentTime, float flRadius, const Vector &vecEyePos, const Vector &vecForward, float flZoomScale )
		{
	// Outside of a snow volume.
	if ( m_iSnowFallArea == SNOWFALL_IN_ENTITY )
	{
		CreateOutsideVolumeSnowParticles( flCurrentTime, flRadius, flZoomScale );
	}
	// Inside of a snow volume.
	else
	{
		CreateInsideVolumeSnowParticles( flCurrentTime, flRadius, vecEyePos, vecForward, flZoomScale );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flCurrentTime - 
//			flRadius - 
//			flZoomScale - 
//-----------------------------------------------------------------------------
void CSnowFallManager::CreateOutsideVolumeSnowParticles( float flCurrentTime, float flRadius, float flZoomScale )
{
	Vector vecParticleSpawn;

	// Outside of a snow volume.
	int iSnow = 0;
	float flRadiusScaled = flRadius * flZoomScale;
	float flRadius2 = flRadiusScaled * flRadiusScaled;

	// Add as many particles as we need
	while ( m_tSnowFallParticleTimer.NextEvent( flCurrentTime ) )
	{
		// Check for a max particle count.
		if ( m_pSnowFallEmitter->GetParticleCount() >= r_SnowParticles.GetInt() )
			continue;

		vecParticleSpawn.x = RandomFloat( m_aSnow[m_aActiveSnow[iSnow]].m_vecMin.x, m_aSnow[m_aActiveSnow[iSnow]].m_vecMax.x );
		vecParticleSpawn.y = RandomFloat( m_aSnow[m_aActiveSnow[iSnow]].m_vecMin.y, m_aSnow[m_aActiveSnow[iSnow]].m_vecMax.y );
		vecParticleSpawn.z = RandomFloat( m_aSnow[m_aActiveSnow[iSnow]].m_vecMin.z, m_aSnow[m_aActiveSnow[iSnow]].m_vecMax.z );

		float flDistance2 = ( m_vecSnowFallEmitOrigin - vecParticleSpawn ).LengthSqr();
		if ( flDistance2 < flRadius2 )
		{
			CreateSnowFallParticle( vecParticleSpawn, m_aActiveSnow[iSnow] );
		}

		iSnow = ( iSnow + 1 ) % m_nActiveSnowCount;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flCurrentTime - 
//			flRadius - 
//			&vecEyePos - 
//			&vecForward - 
//			flZoomScale - 
//-----------------------------------------------------------------------------
void CSnowFallManager::CreateInsideVolumeSnowParticles( float flCurrentTime, float flRadius, const Vector &vecEyePos, const Vector &vecForward, float flZoomScale )
{
	Vector vecParticleSpawn;

	// Check/Setup for zoom.
	bool bZoomed = ( flZoomScale > 1.0f );
	float flZoomRadius = 0.0f;
	Vector vecZoomEmitOrigin;
	if ( bZoomed )
	{
		vecZoomEmitOrigin = m_vecSnowFallEmitOrigin + ( vecForward * ( r_SnowZoomOffset.GetFloat() * flZoomScale ) );
		flZoomRadius = flRadius * flZoomScale;
	}

	int iIndex = 0;

	// Add as many particles as we need
	while ( m_tSnowFallParticleTimer.NextEvent( flCurrentTime ) )
	{
		// Check for a max particle count.
		if ( m_pSnowFallEmitter->GetParticleCount() >= r_SnowParticles.GetInt() )
			continue;

		// Create particle inside of sphere.
		if ( iIndex > 0 )
		{
			CreateSnowParticlesSphere( flZoomRadius );
			CreateSnowParticlesRay( flZoomRadius, vecEyePos, vecForward );
		}
		else
		{
			CreateSnowParticlesSphere( flRadius );
			CreateSnowParticlesRay( flRadius, vecEyePos, vecForward );
		}

		// Increment if zoomed.
		if ( bZoomed )
		{
			iIndex = ( iIndex + 1 ) % 3;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flRadius - 
//-----------------------------------------------------------------------------
void CSnowFallManager::CreateSnowParticlesSphere( float flRadius )
{
	Vector vecParticleSpawn;

	vecParticleSpawn.x = m_vecSnowFallEmitOrigin.x + RandomFloat( -flRadius, flRadius );
	vecParticleSpawn.y = m_vecSnowFallEmitOrigin.y + RandomFloat( -flRadius, flRadius );
	vecParticleSpawn.z = m_vecSnowFallEmitOrigin.z + RandomFloat( -flRadius, flRadius );

	int iSnow = 0;
	for ( iSnow = 0; iSnow < m_nActiveSnowCount; ++iSnow )
	{
		if ( ( vecParticleSpawn.x < m_aSnow[m_aActiveSnow[iSnow]].m_vecMin.x ) || ( vecParticleSpawn.x > m_aSnow[m_aActiveSnow[iSnow]].m_vecMax.x ) )
			continue;
		if ( ( vecParticleSpawn.y < m_aSnow[m_aActiveSnow[iSnow]].m_vecMin.y ) || ( vecParticleSpawn.y > m_aSnow[m_aActiveSnow[iSnow]].m_vecMax.y ) )
			continue;
		if ( ( vecParticleSpawn.z < m_aSnow[m_aActiveSnow[iSnow]].m_vecMin.z ) || ( vecParticleSpawn.z > m_aSnow[m_aActiveSnow[iSnow]].m_vecMax.z ) )
			continue;

		break;
	}

	if ( iSnow == m_nActiveSnowCount )
		return;

	CreateSnowFallParticle( vecParticleSpawn, m_aActiveSnow[iSnow] );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecEyePos - 
//			&vecForward - 
//-----------------------------------------------------------------------------
void CSnowFallManager::CreateSnowParticlesRay( float flRadius, const Vector &vecEyePos, const Vector &vecForward )
{
	// Check to see if we should create particles along line-of-sight.
	if ( !m_bRayParticles && r_SnowRayEnable.GetBool() )
		return;

	Vector vecParticleSpawn;

	// Create a particle down the player's view beyond the radius.
	float flRayRadius = r_SnowRayRadius.GetFloat();

	Vector vecNewForward;
	vecNewForward = vecForward * RandomFloat( flRadius, r_SnowRayLength.GetFloat() );

	vecParticleSpawn.x = vecEyePos.x + vecNewForward.x;
	vecParticleSpawn.y = vecEyePos.y + vecNewForward.y;
	vecParticleSpawn.z = vecEyePos.z + RandomFloat( 72, flRayRadius );
	vecParticleSpawn.x += RandomFloat( -flRayRadius, flRayRadius );
	vecParticleSpawn.y += RandomFloat( -flRayRadius, flRayRadius );

	int iSnow = 0;
	for ( iSnow = 0; iSnow < m_nActiveSnowCount; ++iSnow )
	{
		if ( ( vecParticleSpawn.x < m_aSnow[m_aActiveSnow[iSnow]].m_vecMin.x ) || ( vecParticleSpawn.x > m_aSnow[m_aActiveSnow[iSnow]].m_vecMax.x ) )
			continue;
		if ( ( vecParticleSpawn.y < m_aSnow[m_aActiveSnow[iSnow]].m_vecMin.y ) || ( vecParticleSpawn.y > m_aSnow[m_aActiveSnow[iSnow]].m_vecMax.y ) )
			continue;
		if ( ( vecParticleSpawn.z < m_aSnow[m_aActiveSnow[iSnow]].m_vecMin.z ) || ( vecParticleSpawn.z > m_aSnow[m_aActiveSnow[iSnow]].m_vecMax.z ) )
			continue;

		break;
	}

	if ( iSnow == m_nActiveSnowCount )
		return;

	CreateSnowFallParticle( vecParticleSpawn, m_aActiveSnow[iSnow] );
}

void CSnowFallManager::CreateSnowFallParticle( const Vector &vecParticleSpawn, int iSnow )
{	
	SimpleParticle *pParticle = ( SimpleParticle* )m_pSnowFallEmitter->AddParticle( sizeof( SimpleParticle ), m_aSnow[iSnow].m_hMaterial, vecParticleSpawn );
	if ( pParticle == NULL )
		return; 

	pParticle->m_flLifetime	= 0.0f;
	pParticle->m_vecVelocity = Vector( RandomFloat( -5.0f, 5.0f ), RandomFloat( -5.0f, 5.0f ), ( RandomFloat( -25, -35 ) * r_SnowFallSpeed.GetFloat() ) );
	pParticle->m_flDieTime	= fabs( ( vecParticleSpawn.z - m_aSnow[iSnow].m_vecMin.z ) / ( pParticle->m_vecVelocity.z - 0.1 ) );

	// Probably want to put the color in the snow entity.
//	pParticle->m_uchColor[0] = 150;//color;
//	pParticle->m_uchColor[1] = 175;//color;
//	pParticle->m_uchColor[2] = 200;//color;
	pParticle->m_uchColor[0] = r_SnowColorRed.GetInt();
	pParticle->m_uchColor[1] = r_SnowColorGreen.GetInt();
	pParticle->m_uchColor[2] = r_SnowColorBlue.GetInt();

	pParticle->m_uchStartSize = r_SnowStartSize.GetInt();
	pParticle->m_uchEndSize = r_SnowEndSize.GetInt();

//	pParticle->m_uchStartAlpha	= 255;
	pParticle->m_uchStartAlpha	= r_SnowStartAlpha.GetInt();
	pParticle->m_uchEndAlpha	= r_SnowEndAlpha.GetInt();

	pParticle->m_flRoll			= random->RandomInt( 0, 360 );
	pParticle->m_flRollDelta	= random->RandomFloat( -0.15f, 0.15f );

	pParticle->m_iFlags			= SIMPLE_PARTICLE_FLAG_WINDBLOWN;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool SnowFallManagerCreate( CClient_Precipitation *pSnowEntity )
{
	if ( !s_pSnowFallMgr )
		{
		s_pSnowFallMgr = new CSnowFallManager();
		s_pSnowFallMgr->CreateEmitter();
		s_pSnowFallMgr->InitializeAsClientEntity( NULL, RENDER_GROUP_OTHER );
		if ( !s_pSnowFallMgr )
			return false;
		}

	s_pSnowFallMgr->AddSnowFallEntity( pSnowEntity );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SnowFallManagerDestroy( void )
{
	if ( s_pSnowFallMgr )
	{
		delete s_pSnowFallMgr;
		s_pSnowFallMgr = NULL;
	}
}
