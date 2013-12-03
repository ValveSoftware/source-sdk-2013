//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "fx.h"
#include "c_func_dust.h"
#include "func_dust_shared.h"
#include "c_te_particlesystem.h"
#include "env_wind_shared.h"
#include "engine/IEngineTrace.h"
#include "tier0/vprof.h"
#include "clienteffectprecachesystem.h"
#include "particles_ez.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_Func_Dust, DT_Func_Dust, CFunc_Dust )
	RecvPropInt( RECVINFO(m_Color) ),
	RecvPropInt( RECVINFO(m_SpawnRate) ),
	RecvPropFloat( RECVINFO(m_flSizeMin) ),
	RecvPropFloat( RECVINFO(m_flSizeMax) ),
	RecvPropInt( RECVINFO(m_LifetimeMin) ),
	RecvPropInt( RECVINFO(m_LifetimeMax) ),
	RecvPropInt( RECVINFO(m_DustFlags) ),
	RecvPropInt( RECVINFO(m_SpeedMax) ),
	RecvPropInt( RECVINFO(m_DistMax) ),
	RecvPropInt( RECVINFO( m_nModelIndex ) ),
	RecvPropFloat( RECVINFO( m_FallSpeed ) ),
	RecvPropDataTable( RECVINFO_DT( m_Collision ), 0, &REFERENCE_RECV_TABLE(DT_CollisionProperty) ),
END_RECV_TABLE()



// ------------------------------------------------------------------------------------ //
// CDustEffect implementation.
// ------------------------------------------------------------------------------------ //
#define DUST_ACCEL 50


void CDustEffect::RenderParticles( CParticleRenderIterator *pIterator )
{
	const CFuncDustParticle *pParticle = (const CFuncDustParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Velocity.
		float flAlpha;
		if( m_pDust->m_DustFlags & DUSTFLAGS_FROZEN )
		{
			flAlpha = 1;
		}
		else
		{
			// Alpha.
			float flAngle = (pParticle->m_flLifetime / pParticle->m_flDieTime) * M_PI * 2;
			flAlpha = sin( flAngle - (M_PI * 0.5f) ) * 0.5f + 0.5f;
		}

		Vector tPos;
		TransformParticle( ParticleMgr()->GetModelView(), pParticle->m_Pos, tPos );
		float sortKey = (int) tPos.z;

		if( -tPos.z <= m_pDust->m_DistMax )
		{
			flAlpha *= 1 + (tPos.z / m_pDust->m_DistMax);

			// Draw it.
			float flSize = pParticle->m_flSize;
			if( m_pDust->m_DustFlags & DUSTFLAGS_SCALEMOTES )
				flSize *= -tPos.z;

			RenderParticle_Color255Size(
				pIterator->GetParticleDraw(),
				tPos,
				Vector( m_pDust->m_Color.r, m_pDust->m_Color.g, m_pDust->m_Color.b ),
				flAlpha * m_pDust->m_Color.a,
				flSize
				);
		}

		pParticle = (const CFuncDustParticle*)pIterator->GetNext( sortKey );
	}
}

void CDustEffect::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	Vector vecWind;
	GetWindspeedAtTime( gpGlobals->curtime, vecWind );


	CFuncDustParticle *pParticle = (CFuncDustParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Velocity.
		if( !(m_pDust->m_DustFlags & DUSTFLAGS_FROZEN) )
		{
			// Kill the particle?
			pParticle->m_flLifetime += pIterator->GetTimeDelta();
			if( pParticle->m_flLifetime >= pParticle->m_flDieTime )
			{
				pIterator->RemoveParticle( pParticle );
			}
			else
			{
				for ( int i = 0 ; i < 2 ; i++ )
				{
					if ( pParticle->m_vVelocity[i] < vecWind[i] )
					{
						pParticle->m_vVelocity[i] += ( gpGlobals->frametime * DUST_ACCEL );

						// clamp
						if ( pParticle->m_vVelocity[i] > vecWind[i] )
							pParticle->m_vVelocity[i] = vecWind[i];
					}
					else if (pParticle->m_vVelocity[i] > vecWind[i] )
					{
						pParticle->m_vVelocity[i] -= ( gpGlobals->frametime * DUST_ACCEL );

						// clamp.
						if ( pParticle->m_vVelocity[i] < vecWind[i] )
							pParticle->m_vVelocity[i] = vecWind[i];
					}
				}

				// Apply velocity.
				pParticle->m_Pos.MulAdd( pParticle->m_Pos, pParticle->m_vVelocity, pIterator->GetTimeDelta() );
			}
		}

		pParticle = (CFuncDustParticle*)pIterator->GetNext();
	}
}


// ------------------------------------------------------------------------------------ //
// C_Func_Dust implementation.
// ------------------------------------------------------------------------------------ //

C_Func_Dust::C_Func_Dust() : m_Effect( "C_Func_Dust" )
{
	m_Effect.m_pDust = this;
	m_Effect.SetDynamicallyAllocated( false ); // So it doesn't try to delete itself.
}


C_Func_Dust::~C_Func_Dust()
{
}

void C_Func_Dust::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if( updateType == DATA_UPDATE_CREATED )
	{
		m_hMaterial = m_Effect.GetPMaterial( "particle/sparkles" );

		m_Effect.SetSortOrigin( WorldSpaceCenter( ) );

		// Let us think each frame.
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		
		// If we're setup to be frozen, just make a bunch of particles initially.
		if( m_DustFlags & DUSTFLAGS_FROZEN )
		{
			for( int i=0; i < m_SpawnRate; i++ )
			{
				AttemptSpawnNewParticle();
			}
		}
	}

	m_Spawner.Init( m_SpawnRate ); // N particles per second
}


void C_Func_Dust::ClientThink()
{
	// If frozen, don't make new particles.
	if( m_DustFlags & DUSTFLAGS_FROZEN )
		return;

	// Spawn particles?
	if( m_DustFlags & DUSTFLAGS_ON )
	{
		float flDelta = MIN( gpGlobals->frametime, 0.1f );
		while( m_Spawner.NextEvent( flDelta ) )
		{
			AttemptSpawnNewParticle();
		}
	}

	// Tell the particle manager our bbox.
	Vector vWorldMins, vWorldMaxs;
	CollisionProp()->WorldSpaceAABB( &vWorldMins, &vWorldMaxs );
	vWorldMins -= Vector( m_flSizeMax, m_flSizeMax, m_flSizeMax );
	vWorldMaxs += Vector( m_flSizeMax, m_flSizeMax, m_flSizeMax );
	m_Effect.GetBinding().SetBBox( vWorldMins, vWorldMaxs );
}


bool C_Func_Dust::ShouldDraw()
{
	return false;
}


void C_Func_Dust::AttemptSpawnNewParticle()
{
	// Find a random spot inside our bmodel.
	static int nTests=10;

	for( int iTest=0; iTest < nTests; iTest++ )
	{
		Vector vPercent = RandomVector( 0, 1 );
		Vector vTest = WorldAlignMins() + (WorldAlignMaxs() - WorldAlignMins()) * vPercent;

		int contents = enginetrace->GetPointContents_Collideable( GetCollideable(), vTest );
		if( contents & CONTENTS_SOLID )
		{
			CFuncDustParticle *pParticle = (CFuncDustParticle*)m_Effect.AddParticle( 10, m_hMaterial, vTest );
			if( pParticle )
			{
				pParticle->m_vVelocity = RandomVector( -m_SpeedMax, m_SpeedMax );
				pParticle->m_vVelocity.z -= m_FallSpeed;

				pParticle->m_flLifetime = 0;
				pParticle->m_flDieTime = RemapVal( rand(), 0, VALVE_RAND_MAX, m_LifetimeMin, m_LifetimeMax );

				if( m_DustFlags & DUSTFLAGS_SCALEMOTES )
					pParticle->m_flSize = RemapVal( rand(), 0, VALVE_RAND_MAX, m_flSizeMin/10000.0f, m_flSizeMax/10000.0f );
				else
					pParticle->m_flSize = RemapVal( rand(), 0, VALVE_RAND_MAX, m_flSizeMin, m_flSizeMax );
			
				pParticle->m_Color = m_Color;
			}

			break;
		}
	}
}

//
// Dust
//

//-----------------------------------------------------------------------------
// Spew out dust!
//-----------------------------------------------------------------------------
void FX_Dust( const Vector &vecOrigin, const Vector &vecDirection, float flSize, float flSpeed )
{
	VPROF_BUDGET( "FX_Dust", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	
	int	numPuffs = (flSize*0.5f);

	if ( numPuffs < 1 )
		numPuffs = 1;
	if ( numPuffs > 32 )
		numPuffs = 32;

	float speed = flSpeed * 0.1f;

	if ( speed < 0 )
		speed = 1.0f;
	if (speed > 48.0f )
		speed = 48.0f;

	//FIXME: Better sampling area
	Vector offset = vecOrigin + ( vecDirection * flSize );

	//Find area ambient light color and use it to tint smoke
	Vector	worldLight = WorldGetLightForPoint( offset, true );

	// Throw puffs
	SimpleParticle particle;
	for ( int i = 0; i < numPuffs; i++ )
	{
		offset.Random( -(flSize*0.25f), flSize*0.25f );
		offset += vecOrigin + ( vecDirection * flSize );

		particle.m_Pos = offset;
		particle.m_flLifetime = 0.0f;
		particle.m_flDieTime  = random->RandomFloat( 0.4f, 1.0f );
		
		particle.m_vecVelocity = vecDirection * random->RandomFloat( speed*0.5f, speed ) * i;
		particle.m_vecVelocity[2] = 0.0f;

		int	color = random->RandomInt( 48, 64 );

		particle.m_uchColor[0] = (color+16) + ( worldLight[0] * (float) color );
		particle.m_uchColor[1] = (color+8) + ( worldLight[1] * (float) color );
		particle.m_uchColor[2] = color + ( worldLight[2] * (float) color );

		particle.m_uchStartAlpha= random->RandomInt( 64, 128 );
		particle.m_uchEndAlpha	= 0;
		particle.m_uchStartSize = random->RandomInt( 2, 8 );
		particle.m_uchEndSize	= random->RandomInt( 24, 48 );
		particle.m_flRoll		= random->RandomInt( 0, 360 );
		particle.m_flRollDelta	= random->RandomFloat( -0.5f, 0.5f );

		AddSimpleParticle( &particle, g_Mat_DustPuff[random->RandomInt(0,1)] );
	}
}


class C_TEDust: public C_TEParticleSystem
{
public:
	DECLARE_CLASS( C_TEDust, C_TEParticleSystem );
	DECLARE_CLIENTCLASS();

				C_TEDust();
	virtual		~C_TEDust();

public:
	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	virtual	bool	ShouldDraw() { return true; }

public:

	float		m_flSize;
	float		m_flSpeed;
	Vector		m_vecDirection;

protected:
	void		GetDustColor( Vector &color );
};

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TEDust, DT_TEDust, CTEDust )
	RecvPropFloat(RECVINFO(m_flSize)),
	RecvPropFloat(RECVINFO(m_flSpeed)),
	RecvPropVector(RECVINFO(m_vecDirection)),
END_RECV_TABLE()

//==================================================
// C_TEDust
//==================================================

C_TEDust::C_TEDust()
{
}

C_TEDust::~C_TEDust()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bNewEntity - whether or not to start a new entity
//-----------------------------------------------------------------------------
void C_TEDust::PostDataUpdate( DataUpdateType_t updateType )
{
	FX_Dust( m_vecOrigin, m_vecDirection, m_flSize, m_flSpeed );
}


void TE_Dust( IRecipientFilter& filter, float delay,
			 const Vector &pos, const Vector &dir, float size, float speed )
{
	FX_Dust( pos, dir, size, speed );
}
