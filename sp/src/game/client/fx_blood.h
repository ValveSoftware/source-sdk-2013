//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FX_BLOOD_H
#define FX_BLOOD_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/memdbgon.h"

class CBloodSprayEmitter : public CSimpleEmitter
{
public:
	
	CBloodSprayEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	static CBloodSprayEmitter *Create( const char *pDebugName )
	{
		return new CBloodSprayEmitter( pDebugName );
	}

	inline void SetGravity( float flGravity )
	{
		m_flGravity = flGravity;
	}

	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -4.0f );

		//Cap the minimum roll
		/*
		if ( fabs( pParticle->m_flRollDelta ) < 0.5f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.5f : -0.5f;
		}
		*/

		return pParticle->m_flRoll;
	}

	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		if ( !( pParticle->m_iFlags & SIMPLE_PARTICLE_FLAG_NO_VEL_DECAY ) )
		{
			//Decelerate
			static float dtime;
			static float decay;

			if ( dtime != timeDelta )
			{
				decay = ExponentialDecay( 0.1, 0.4f, dtime );
				dtime = timeDelta;
			}

			pParticle->m_vecVelocity *= decay;
			pParticle->m_vecVelocity[2] -= ( m_flGravity * timeDelta );
		}
	}

private:

	float m_flGravity;

	CBloodSprayEmitter( const CBloodSprayEmitter & );
};

#include "tier0/memdbgoff.h"

#endif // FX_BLOOD_H
