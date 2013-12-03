//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_IMPACT_EFFECTS_H
#define C_IMPACT_EFFECTS_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: DustParticle emitter 
//-----------------------------------------------------------------------------
class CDustParticle : public CSimpleEmitter
{
public:
	
	CDustParticle( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	//Create
	static CDustParticle *Create( const char *pDebugName="dust" )
	{
		return new CDustParticle( pDebugName );
	}

	//Roll
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -8.0f );

#ifdef _XBOX
		//Cap the minimum roll
		if ( fabs( pParticle->m_flRollDelta ) < 0.1f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.1f : -0.1f;
		}
#else
		if ( fabs( pParticle->m_flRollDelta ) < 0.5f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.5f : -0.5f;
		}
#endif // _XBOX

		return pParticle->m_flRoll;
	}

	//Velocity
	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		Vector	saveVelocity = pParticle->m_vecVelocity;

		//Decellerate
		static float dtime;
		static float decay;

		if ( dtime != timeDelta )
		{
			dtime = timeDelta;
			float expected = 0.5;
			decay = exp( log( 0.0001f ) * dtime / expected );
		}

		pParticle->m_vecVelocity = pParticle->m_vecVelocity * decay;

#ifdef _XBOX
		//Cap the minimum speed
		if ( pParticle->m_vecVelocity.LengthSqr() < (8.0f*8.0f) )
		{
			VectorNormalize( saveVelocity );
			pParticle->m_vecVelocity = saveVelocity * 8.0f;
		}
#else
		if ( pParticle->m_vecVelocity.LengthSqr() < (32.0f*32.0f) )
		{
			VectorNormalize( saveVelocity );
			pParticle->m_vecVelocity = saveVelocity * 32.0f;
		}
#endif // _XBOX
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
	CDustParticle( const CDustParticle & ); // not defined, not accessible
};

void GetColorForSurface( trace_t *trace, Vector *color );

#include "tier0/memdbgoff.h"

#endif // C_IMPACT_EFFECTS_H
