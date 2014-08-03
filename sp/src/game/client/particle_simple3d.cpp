//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "particle_simple3d.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Defined in pm_math.c
float anglemod( float a );

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CSmartPtr<CSimple3DEmitter> CSimple3DEmitter::Create( const char *pDebugName )	
{
	CSimple3DEmitter* pSimple3DEmitter = new CSimple3DEmitter( pDebugName );

	// Do in world space
	pSimple3DEmitter->m_ParticleEffect.SetEffectCameraSpace( false );
	return pSimple3DEmitter;
}


void CSimple3DEmitter::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	Particle3D *pParticle = (Particle3D*)pIterator->GetFirst();
	while ( pParticle )
	{
		const float	timeDelta = pIterator->GetTimeDelta();

		//Should this particle die?
		pParticle->m_flLifeRemaining -= timeDelta;

		if ( pParticle->IsDead() )
		{
			pIterator->RemoveParticle( pParticle );
		}
		else
		{
			// Angular rotation
			pParticle->m_vAngles.x += pParticle->m_flAngSpeed * timeDelta;
			pParticle->m_vAngles.y += pParticle->m_flAngSpeed * timeDelta;
			pParticle->m_vAngles.z += pParticle->m_flAngSpeed * timeDelta;

			//Simulate the movement with collision
			trace_t trace;
			m_ParticleCollision.MoveParticle( pParticle->m_Pos, pParticle->m_vecVelocity, &pParticle->m_flAngSpeed, timeDelta, &trace );

			// ---------------------------------------
			// Decay towards flat
			// ---------------------------------------
			if (pParticle->m_flAngSpeed == 0 || trace.fraction != 1.0)
			{
				pParticle->m_vAngles.x = anglemod(pParticle->m_vAngles.x);
				if (pParticle->m_vAngles.x < 180)
				{
					if (fabs(pParticle->m_vAngles.x - 90) > 0.5)
					{
						pParticle->m_vAngles.x = 0.5*pParticle->m_vAngles.x + 46;
					}
				}
				else
				{
					if (fabs(pParticle->m_vAngles.x - 270) > 0.5)
					{
						pParticle->m_vAngles.x = 0.5*pParticle->m_vAngles.x + 135;
					}
				}

				pParticle->m_vAngles.y = anglemod(pParticle->m_vAngles.y);
				if (fabs(pParticle->m_vAngles.y) > 0.5)
				{
					pParticle->m_vAngles.y = 0.5*pParticle->m_vAngles.z;
				}
			}
		}

		pParticle = (Particle3D*)pIterator->GetNext();
	}
}


void CSimple3DEmitter::RenderParticles( CParticleRenderIterator *pIterator )
{
	const Particle3D *pParticle = (const Particle3D *)pIterator->GetFirst();
	while ( pParticle )
	{
		float sortKey = CurrentViewForward().Dot( CurrentViewOrigin() - pParticle->m_Pos );

		// -------------------------------------------------------
		//  Set color based on direction towards camera
		// -------------------------------------------------------
		Vector	color;
		Vector vFaceNorm;
		Vector vCameraToFace = (pParticle->m_Pos - CurrentViewOrigin());
		AngleVectors(pParticle->m_vAngles,&vFaceNorm);
		float flFacing = DotProduct(vCameraToFace,vFaceNorm);

		if (flFacing <= 0)
		{
			color[0] = pParticle->m_uchFrontColor[0] / 255.0f;
			color[1] = pParticle->m_uchFrontColor[1] / 255.0f;
			color[2] = pParticle->m_uchFrontColor[2] / 255.0f;
		}
		else
		{
			color[0] = pParticle->m_uchBackColor[0] / 255.0f;
			color[1] = pParticle->m_uchBackColor[1] / 255.0f;
			color[2] = pParticle->m_uchBackColor[2] / 255.0f;
		}

		//Render it in world space
		RenderParticle_ColorSizeAngles(
			pIterator->GetParticleDraw(),
			pParticle->m_Pos,
			color,
			pParticle->GetFadeFraction(),
			pParticle->m_uchSize,
			pParticle->m_vAngles);

		pParticle = (const Particle3D *)pIterator->GetNext( sortKey );
	}
}


