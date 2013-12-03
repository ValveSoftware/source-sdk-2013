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
#include "clienteffectprecachesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ==============================================
//  Rotorwash particle emitter
// ==============================================

#ifndef _XBOX

class WashEmitter : public CSimpleEmitter
{
public:
	
	WashEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}

	static WashEmitter *Create( const char *pDebugName )
	{
		return new WashEmitter( pDebugName );
	}

	void UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		// Float up when lifetime is half gone.
		pParticle->m_vecVelocity[ 2 ] += 64 * timeDelta;

		// FIXME: optimize this....
		pParticle->m_vecVelocity *= ExponentialDecay( 0.8, 0.05, timeDelta );
	}

	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -2.0f );

		//Cap the minimum roll
		if ( fabs( pParticle->m_flRollDelta ) < 0.5f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.5f : -0.5f;
		}

		return pParticle->m_flRoll;
	}

	virtual float UpdateAlpha( const SimpleParticle *pParticle )
	{
		return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
	}

private:
	WashEmitter( const WashEmitter & );
};

#endif // !_XBOX

// ==============================================
//  Rotorwash entity
// ==============================================

#define	ROTORWASH_THINK_INTERVAL	0.1f

class C_RotorWashEmitter : public C_BaseEntity
{
public:

	DECLARE_CLASS( C_RotorWashEmitter, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_RotorWashEmitter( void );

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ClientThink( void );
	
protected:

	float	m_flAltitude;

	PMaterialHandle m_hWaterMaterial[2];

#ifndef _XBOX
	void InitSpawner( void );
	CSmartPtr<WashEmitter>	m_pSimple;
#endif // !XBOX
};

IMPLEMENT_CLIENTCLASS_DT( C_RotorWashEmitter, DT_RotorWashEmitter, CRotorWashEmitter)
	RecvPropFloat(RECVINFO(m_flAltitude)),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_RotorWashEmitter::C_RotorWashEmitter( void )
{
#ifndef _XBOX
	m_pSimple =  NULL;
	m_hWaterMaterial[0] = NULL;
	m_hWaterMaterial[1] = NULL;
#endif // !_XBOX
}

#ifndef _XBOX
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_RotorWashEmitter::InitSpawner( void )
{
	if ( m_pSimple.IsValid() )
		return;

	m_pSimple = WashEmitter::Create( "wash" );
	m_pSimple->SetNearClip( 128, 256 );
}
#endif // !XBOX

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_RotorWashEmitter::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( gpGlobals->curtime + ROTORWASH_THINK_INTERVAL );

#ifndef _XBOX
		InitSpawner();
#endif // !XBOX
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_RotorWashEmitter::ClientThink( void )
{
	SetNextClientThink( gpGlobals->curtime + ROTORWASH_THINK_INTERVAL );

	trace_t	tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin()+(Vector(0, 0, -1024)), (MASK_SOLID_BRUSHONLY|CONTENTS_WATER|CONTENTS_SLIME), NULL, COLLISION_GROUP_NONE, &tr );

	if ( /*!m_bIgnoreSolid && */(tr.fraction == 1.0f || tr.startsolid || tr.allsolid) )
		return;

	// If we hit the skybox, don't do it either
	if ( tr.surface.flags & SURF_SKY )
		return;

	float heightScale = RemapValClamped( tr.fraction * 1024, 512, 1024, 1.0f, 0.0f );

	Vector vecDustColor;

	if ( tr.contents & CONTENTS_WATER )
	{
		vecDustColor.x = 0.8f;
		vecDustColor.y = 0.8f;
		vecDustColor.z = 0.75f;
	}
	else if ( tr.contents & CONTENTS_SLIME )
	{
		vecDustColor.x = 0.6f;
		vecDustColor.y = 0.5f;
		vecDustColor.z = 0.15f;
	}
	else
	{
		vecDustColor.x = 0.35f;
		vecDustColor.y = 0.3f;
		vecDustColor.z = 0.25f;
	}

#ifndef _XBOX

	InitSpawner();

	if ( m_pSimple.IsValid() == false )
		return;

	m_pSimple->SetSortOrigin( GetAbsOrigin() );

	PMaterialHandle	*hMaterial;
	
	// Cache and set our material based on the surface we're over (ie. water)
	if ( tr.contents & (CONTENTS_WATER|CONTENTS_SLIME) )
	{
		if ( m_hWaterMaterial[0] == NULL )
		{
			m_hWaterMaterial[0] = m_pSimple->GetPMaterial("effects/splash1");
			m_hWaterMaterial[1] = m_pSimple->GetPMaterial("effects/splash2");
		}
		hMaterial = m_hWaterMaterial;
	}
	else
	{
		hMaterial = g_Mat_DustPuff;
	}

#endif // !XBOX

	// If we're above water, make ripples
	if ( tr.contents & (CONTENTS_WATER|CONTENTS_SLIME) )
	{
		float flScale = random->RandomFloat( 7.5f, 8.5f );

		Vector	color = Vector( 0.8f, 0.8f, 0.75f );
		Vector startPos = tr.endpos + Vector(0,0,8);
		Vector endPos = tr.endpos + Vector(0,0,-64);

		if ( tr.fraction < 1.0f )
		{
			//Add a ripple quad to the surface
			FX_AddQuad( tr.endpos + ( tr.plane.normal * 0.5f ), 
						tr.plane.normal, 
						64.0f * flScale, 
						128.0f * flScale, 
						0.8f,
						0.75f * heightScale, 
						0.0f,
						0.75f,
						random->RandomFloat( 0, 360 ),
						random->RandomFloat( -2.0f, 2.0f ),
						vecDustColor, 
						0.2f,  
						"effects/splashwake3",
						(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );
		}
	}

#ifndef _XBOX
	int		numRingSprites = 32;
	float	yaw = random->RandomFloat( 0, 2*M_PI ); // Randomly placed on the unit circle
	float	yawIncr = (2*M_PI) / numRingSprites;
	Vector	vecForward;
	Vector	offset;
	SimpleParticle	*pParticle;

	// Draw the rings
	for ( int i = 0; i < numRingSprites; i++ )
	{
		// Get our x,y on the unit circle
		SinCos( yaw, &vecForward.y, &vecForward.x );
		
		// Increment ahead
		yaw += yawIncr;

		// @NOTE toml (3-28-07): broke out following expression because vc2005 optimizer was screwing up in presence of SinCos inline assembly. Would also
		// go away if offset were referenced below as in the AddLineOverlay()
		//offset = ( RandomVector( -4.0f, 4.0f ) + tr.endpos ) + ( vecForward * 128.0f );

		offset = vecForward * 128.0f;
		offset += tr.endpos + RandomVector( -4.0f, 4.0f );


		pParticle = (SimpleParticle *) m_pSimple->AddParticle( sizeof(SimpleParticle), hMaterial[random->RandomInt(0,1)], offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 0.25f, 1.0f );

			pParticle->m_vecVelocity = vecForward * random->RandomFloat( 1000, 1500 );
		
			#if __EXPLOSION_DEBUG
			debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
			#endif

			if ( tr.contents & CONTENTS_SLIME )
			{
				vecDustColor.x = random->RandomFloat( 0.4f, 0.6f );
				vecDustColor.y = random->RandomFloat( 0.3f, 0.5f );
				vecDustColor.z = random->RandomFloat( 0.1f, 0.2f );
			}

			pParticle->m_uchColor[0] = vecDustColor.x * 255.0f;
			pParticle->m_uchColor[1] = vecDustColor.y * 255.0f;
			pParticle->m_uchColor[2] = vecDustColor.z * 255.0f;

			pParticle->m_uchStartSize	= random->RandomInt( 16, 64 );
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4;

			pParticle->m_uchStartAlpha	= random->RandomFloat( 16, 32 ) * heightScale;
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -16.0f, 16.0f );
		}
	}
#endif // !XBOX
}
