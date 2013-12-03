//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "engine/IEngineTrace.h"
#include "fx_sparks.h"
#include "particles_ez.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_starfield_diameter( "cl_starfield_diameter", "128.0", FCVAR_NONE );
ConVar cl_starfield_distance( "cl_starfield_distance", "256.0", FCVAR_NONE );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_EnvStarfield : public C_BaseEntity
{
	DECLARE_CLASS( C_EnvStarfield, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	C_EnvStarfield();

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink( void );
	
private:
	// Emitter
	CSmartPtr<CTrailParticles> m_pEmitter;
	bool	m_bOn;
	float	m_flDensity;
	float	m_flNumParticles;

private:
	C_EnvStarfield( const C_EnvStarfield & );
};

IMPLEMENT_CLIENTCLASS_DT( C_EnvStarfield, DT_EnvStarfield, CEnvStarfield )
	RecvPropInt( RECVINFO(m_bOn) ),
	RecvPropFloat( RECVINFO(m_flDensity) ),
END_RECV_TABLE()

// ------------------------------------------------------------------------- //
// C_EnvStarfield
// ------------------------------------------------------------------------- //
C_EnvStarfield::C_EnvStarfield()
{
	m_bOn = false;
	m_flDensity = 1.0;
	m_flNumParticles = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_EnvStarfield::OnDataChanged( DataUpdateType_t updateType )
{		
	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_pEmitter = CTrailParticles::Create( "EnvStarfield" );
		Vector vecCenter = MainViewOrigin() + (MainViewForward() * cl_starfield_distance.GetFloat() );
		m_pEmitter->Setup( (Vector &) vecCenter, 
			NULL, 
			0.0, 
			0, 
			64, 
			0, 
			0, 
			bitsPARTICLE_TRAIL_VELOCITY_DAMPEN | bitsPARTICLE_TRAIL_FADE | bitsPARTICLE_TRAIL_FADE_IN );

		// Start thinking
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	BaseClass::OnDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EnvStarfield::ClientThink( void )
{
	if ( !m_bOn || !m_flDensity ) 
		return;

	PMaterialHandle	hParticleMaterial = m_pEmitter->GetPMaterial( "effects/spark_noz" );

	// Find a start & end point for the particle
	// Start particles straight ahead of the client
	Vector vecViewOrigin = MainViewOrigin();

	// Determine the number of particles
	m_flNumParticles += 1.0 * (m_flDensity);
	int iNumParticles = floor(m_flNumParticles);
	m_flNumParticles -= iNumParticles;

	// Add particles
	for ( int i = 0; i < iNumParticles; i++ )
	{
		float flDiameter = cl_starfield_diameter.GetFloat();

		Vector vecStart = vecViewOrigin + (MainViewForward() * cl_starfield_distance.GetFloat() );
		Vector vecEnd = vecViewOrigin + (MainViewRight() * RandomFloat(-flDiameter,flDiameter)) + (MainViewUp() * RandomFloat(-flDiameter,flDiameter));
		Vector vecDir = (vecEnd - vecStart);
		float flDistance = VectorNormalize( vecDir );
		float flTravelTime = 2.0;
		
		// Start a random amount along the path
		vecStart += vecDir * ( RandomFloat(0.1,0.3) * flDistance );

		TrailParticle *pParticle = (TrailParticle *) m_pEmitter->AddParticle( sizeof(TrailParticle), hParticleMaterial, vecStart );
		if ( pParticle )
		{
			pParticle->m_vecVelocity = vecDir * (flDistance / flTravelTime);
			pParticle->m_flDieTime = flTravelTime;
			pParticle->m_flLifetime = 0;
			pParticle->m_flWidth = RandomFloat( 1, 3 );
			pParticle->m_flLength = RandomFloat( 0.05, 0.4 );
			pParticle->m_color.r	= 255;
			pParticle->m_color.g	= 255;
			pParticle->m_color.b	= 255;
			pParticle->m_color.a	= 255;
		}
	}
}