//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"	   
#include "c_te_particlesystem.h"
#include "movevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int		ramp1[][3] =
{
	{ 255, 243, 227 },
	{ 223, 171, 39 },
	{ 191, 119, 47 },
	{ 127, 59, 43 },
	{ 99, 47, 31 },
	{ 75, 35, 19 },
	{ 47, 23, 11 },
	{ 175, 103, 35 },
};

int		ramp2[][3] =
{
	{ 255, 243, 227 },
	{ 239, 203, 31 },
	{ 223, 171, 39 },
	{ 207, 143, 43 },
	{ 191, 119, 47 },
	{ 175, 99, 47 },
	{ 143, 67, 51 },
	{ 115, 55, 35 },
};

int		ramp3[][3] =
{
	{ 223, 171, 39 },
	{ 191, 119, 47 },
	{ 91, 91, 91 },
	{ 75, 75, 75 },
	{ 63, 63, 63 },
	{ 47, 47, 47 },
};

#define SPARK_COLORCOUNT	9

int		gSparkRamp[ SPARK_COLORCOUNT ][3] =
{
	{ 255, 255, 255 },
	{ 255, 247, 199 },
	{ 255, 243, 147 },
	{ 255, 243, 27 },
	{ 239, 203, 31 },
	{ 223, 171, 39 },
	{ 207, 143, 43 },
	{ 127, 59, 43 },
	{ 35, 19, 7 }
};



// ------------------------------------------------------------------------ //
// C_TEParticleSystem.
// ------------------------------------------------------------------------ //

IMPLEMENT_CLIENTCLASS_DT(C_TEParticleSystem, DT_TEParticleSystem, CTEParticleSystem)
	RecvPropFloat( RECVINFO(m_vecOrigin[0]) ),
	RecvPropFloat( RECVINFO(m_vecOrigin[1]) ),
	RecvPropFloat( RECVINFO(m_vecOrigin[2]) ),
END_RECV_TABLE()


C_TEParticleSystem::C_TEParticleSystem()
{
	m_vecOrigin.Init();
}



// ------------------------------------------------------------------------ //
// CTEParticleRenderer implementation.
// ------------------------------------------------------------------------ //

CTEParticleRenderer::CTEParticleRenderer( const char *pDebugName ) :
	CParticleEffect( pDebugName )
{
	m_ParticleSize = 1.5f;
	m_MaterialHandle = INVALID_MATERIAL_HANDLE;
}


CTEParticleRenderer::~CTEParticleRenderer()
{
}


CSmartPtr<CTEParticleRenderer> CTEParticleRenderer::Create( const char *pDebugName, const Vector &vOrigin )
{
	CTEParticleRenderer *pRet = new CTEParticleRenderer( pDebugName );
	if( pRet )
	{
		pRet->SetDynamicallyAllocated( true );
		pRet->SetSortOrigin( vOrigin );
	}

	return pRet;
}


StandardParticle_t* CTEParticleRenderer::AddParticle()
{
	if(m_MaterialHandle == INVALID_MATERIAL_HANDLE)
	{
		m_MaterialHandle = m_ParticleEffect.FindOrAddMaterial("particle/particledefault");
	}

	StandardParticle_t *pParticle = 
		(StandardParticle_t*)BaseClass::AddParticle( sizeof(StandardParticle_t), m_MaterialHandle, m_vSortOrigin );

	if(pParticle)
		pParticle->m_EffectDataWord = 0; // (ramp)

	return pParticle;
}


void CTEParticleRenderer::RenderParticles( CParticleRenderIterator *pIterator )
{
	const StandardParticle_t *pParticle = (const StandardParticle_t*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Render.
		Vector tPos;
		TransformParticle(ParticleMgr()->GetModelView(), pParticle->m_Pos, tPos);
		float sortKey = tPos.z;

		Vector vColor(pParticle->m_Color[0]/255.9f, pParticle->m_Color[1]/255.9f, pParticle->m_Color[2]/255.9f);
		RenderParticle_ColorSize(
			pIterator->GetParticleDraw(),
			tPos,
			vColor,
			pParticle->m_Color[3]/255.9f,
			m_ParticleSize);

		pParticle = (const StandardParticle_t*)pIterator->GetNext( sortKey );
	}
}

void CTEParticleRenderer::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	StandardParticle_t *pParticle = (StandardParticle_t*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Remove the particle?
		SetParticleLifetime(pParticle, GetParticleLifetime(pParticle) - pIterator->GetTimeDelta());
		if(GetParticleLifetime(pParticle) < 0)
		{
			pIterator->RemoveParticle( pParticle );
		}
		else
		{
			float	ft = pIterator->GetTimeDelta();
			float	time3 = 15.0 * ft;
			float	time2 = 10.0 * ft;
			float	time1 = 5.0 * ft;
			float	dvel = 4* ft ;

			float grav = ft * GetCurrentGravity() * 0.05f;

			int		(*colorIndex)[3];
			int		iRamp;

			switch(GetParticleType(pParticle))
			{
			case pt_static:
				break;

			case pt_fire:
				pParticle->m_EffectDataWord += (unsigned short)(time1 * (1 << SIMSHIFT));
				iRamp = pParticle->m_EffectDataWord >> SIMSHIFT;
				if(iRamp >= 6)
				{
					pParticle->m_Lifetime = -1;
				}
				else
				{
					colorIndex = &ramp3[ iRamp ];
					pParticle->SetColor((float)(*colorIndex)[0] / 255.0f, (float)(*colorIndex)[1] / 255.0f, (float)(*colorIndex)[2] / 255.0f);
				}
				pParticle->m_Velocity[2] += grav;
				break;

			case pt_explode:
				pParticle->m_EffectDataWord += (unsigned short)(time2 * (1 << SIMSHIFT));
				iRamp = pParticle->m_EffectDataWord >> SIMSHIFT;
				if(iRamp >= 8)
				{
					pParticle->m_Lifetime = -1;
				}
				else
				{
					colorIndex = &ramp1[ iRamp ];
					pParticle->SetColor((float)(*colorIndex)[0] / 255.0f, (float)(*colorIndex)[1] / 255.0f, (float)(*colorIndex)[2] / 255.0f);
				}
				pParticle->m_Velocity = pParticle->m_Velocity + pParticle->m_Velocity * dvel;
				pParticle->m_Velocity[2] -= grav;
				break;

			case pt_explode2:
				pParticle->m_EffectDataWord += (unsigned short)(time3 * (1 << SIMSHIFT));
				iRamp = pParticle->m_EffectDataWord >> SIMSHIFT;
				if(iRamp >= 8)
				{
					pParticle->m_Lifetime = -1;
				}
				else
				{
					colorIndex = &ramp2[ iRamp ];
					pParticle->SetColor((float)(*colorIndex)[0] / 255.0f, (float)(*colorIndex)[1] / 255.0f, (float)(*colorIndex)[2] / 255.0f);
				}
				pParticle->m_Velocity = pParticle->m_Velocity - pParticle->m_Velocity * ft;
				pParticle->m_Velocity[2] -= grav;
				break;

			case pt_grav:
				pParticle->m_Velocity[2] -= grav * 20;
				break;
			case pt_slowgrav:
				pParticle->m_Velocity[2] = grav;
				break;

			case pt_vox_grav:
				pParticle->m_Velocity[2] -= grav * 8;
				break;
				
			case pt_vox_slowgrav:
				pParticle->m_Velocity[2] -= grav * 4;
				break;

				
			case pt_blob:
			case pt_blob2:
				pParticle->m_EffectDataWord += (unsigned short)(time2 * (1 << SIMSHIFT));
				iRamp = pParticle->m_EffectDataWord >> SIMSHIFT;
				if(iRamp >= SPARK_COLORCOUNT)
				{
					pParticle->m_EffectDataWord = 0;
					iRamp = 0;
				}
				
				colorIndex = &gSparkRamp[ iRamp ];
				pParticle->SetColor((float)(*colorIndex)[0] / 255.0f, (float)(*colorIndex)[1] / 255.0f, (float)(*colorIndex)[2] / 255.0f);
				
				pParticle->m_Velocity[0] -= pParticle->m_Velocity[0]*0.5*ft;
				pParticle->m_Velocity[1] -= pParticle->m_Velocity[1]*0.5*ft;
				pParticle->m_Velocity[2] -= grav * 5;

				if ( random->RandomInt(0,3) )
				{
					SetParticleType(pParticle, pt_blob);
					pParticle->SetAlpha(0);
				}
				else
				{
					SetParticleType(pParticle, pt_blob2);
					pParticle->SetAlpha(255.9f);
				}
				break;
			}
			// Update position.
			pParticle->m_Pos = pParticle->m_Pos + pParticle->m_Velocity * ft;
		}

		pParticle = (StandardParticle_t*)pIterator->GetNext();
	}
}


