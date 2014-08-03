//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "particlemgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "c_te_particlesystem.h"
#include "fx.h"
#include "fx_quad.h"
#include "c_te_effect_dispatch.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	THUMPER_DUST_LIFETIME		2.0f
#define THUMPER_MAX_PARTICLES		24


extern IPhysicsSurfaceProps *physprops;


class ThumperDustEmitter : public CSimpleEmitter
{
public:
	
	ThumperDustEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}

	static ThumperDustEmitter *Create( const char *pDebugName )
	{
		return new ThumperDustEmitter( pDebugName );
	}

	void UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		// Float up when lifetime is half gone.
		pParticle->m_vecVelocity[2] -= ( 8.0f * timeDelta );


		// FIXME: optimize this....
		pParticle->m_vecVelocity *= ExponentialDecay( 0.9, 0.03, timeDelta );
	}

	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -4.0f );

		//Cap the minimum roll
		if ( fabs( pParticle->m_flRollDelta ) < 0.25f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.25f : -0.25f;
		}

		return pParticle->m_flRoll;
	}

	virtual float UpdateAlpha( const SimpleParticle *pParticle )
	{
		return (pParticle->m_uchStartAlpha/255.0f) + ( (float)(pParticle->m_uchEndAlpha/255.0f) - (float)(pParticle->m_uchStartAlpha/255.0f) ) * (pParticle->m_flLifetime / pParticle->m_flDieTime);
	}

private:
	ThumperDustEmitter( const ThumperDustEmitter & );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bNewEntity - whether or not to start a new entity
//-----------------------------------------------------------------------------

void FX_ThumperDust( const CEffectData &data )
{
	Vector vecDustColor;
	vecDustColor.x = 0.85f;
	vecDustColor.y = 0.75f;
	vecDustColor.z = 0.52f;

	CSmartPtr<ThumperDustEmitter> pSimple = ThumperDustEmitter::Create( "thumperdust" );

	C_BaseEntity *pEnt = C_BaseEntity::Instance( data.m_hEntity );
	if ( pEnt )
	{
		Vector vWorldMins, vWorldMaxs;
		float scale = pEnt->CollisionProp()->BoundingRadius();
		vWorldMins[0] = data.m_vOrigin[0] - scale;
		vWorldMins[1] = data.m_vOrigin[1] - scale;
		vWorldMins[2] = data.m_vOrigin[2] - scale;
		vWorldMaxs[0] = data.m_vOrigin[0] + scale;
		vWorldMaxs[1] = data.m_vOrigin[1] + scale;
		vWorldMaxs[2] = data.m_vOrigin[2] + scale;
		pSimple->GetBinding().SetBBox( vWorldMins, vWorldMaxs, true );
	}

	pSimple->SetSortOrigin( data.m_vOrigin );
	pSimple->SetNearClip( 32, 64 );

	SimpleParticle	*pParticle = NULL;

	Vector	offset;

	//int	numPuffs = IsXbox() ? THUMPER_MAX_PARTICLES/2 : THUMPER_MAX_PARTICLES;
	int	numPuffs = THUMPER_MAX_PARTICLES;

	float flYaw = 0;
	float flIncr = (2*M_PI) / (float) numPuffs; // Radians
	Vector forward;
	Vector vecColor;
	int i = 0;

	float flScale = MIN( data.m_flScale, 255 );

	// Setup the color for these particles
	engine->ComputeLighting( data.m_vOrigin, NULL, true, vecColor );
	VectorLerp( vecColor, vecDustColor, 0.5, vecColor );
	vecColor *= 255;

	for ( i = 0; i < numPuffs; i++ )
	{
		flYaw += flIncr;
		SinCos( flYaw, &forward.y, &forward.x );	
		forward.z = 0.0f;

		offset = ( RandomVector( -4.0f, 4.0f ) + data.m_vOrigin ) + ( forward * 128.0f );

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof(SimpleParticle), g_Mat_DustPuff[random->RandomInt(0,1)], offset );
		if ( pParticle != NULL )
		{	
			pParticle->m_flLifetime		= 0.0f;
			pParticle->m_flDieTime		= 1.5f;
	
			Vector dir = (offset - data.m_vOrigin);
			float length = dir.Length();
			VectorNormalize( dir );

			pParticle->m_vecVelocity	= dir * ( length * 2.0f );
			pParticle->m_vecVelocity[2]	= data.m_flScale / 3;

			pParticle->m_uchColor[0]	= vecColor[0];
			pParticle->m_uchColor[1]	= vecColor[1];
			pParticle->m_uchColor[2]	= vecColor[2];

			pParticle->m_uchStartAlpha	= random->RandomInt( 64, 96 );
			pParticle->m_uchEndAlpha	= 0;

			pParticle->m_uchStartSize	= flScale * 0.25f;
			pParticle->m_uchEndSize		= flScale * 0.5f;

			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -6.0f, 6.0f );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void ThumperDustCallback( const CEffectData &data )
{
	FX_ThumperDust( data );
}

DECLARE_CLIENT_EFFECT( "ThumperDust", ThumperDustCallback );