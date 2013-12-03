//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "baseparticleentity.h"
#include "engine/IEngineTrace.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------- //
// Defines.
// ------------------------------------------------------------------------- //

#define MAX_FIRE_EMITTERS		128
#define FIRE_PARTICLE_LIFETIME	2

Vector g_FireSpreadDirection(0,0,1);


class FireRamp
{
public:
			FireRamp(const Vector &s, const Vector &e)
			{
				m_Start=s;
				m_End=e;
			}

	Vector	m_Start;
	Vector	m_End;
};

FireRamp g_FireRamps[] =
{
	FireRamp(Vector(1,0,0), Vector(1,1,0)),
	FireRamp(Vector(0.5,0.5,0), Vector(0,0,0))
};
#define NUM_FIRE_RAMPS	(sizeof(g_FireRamps) / sizeof(g_FireRamps[0]))


#define NUM_FIREGRID_OFFSETS	8
Vector g_Offsets[NUM_FIREGRID_OFFSETS] =
{
	Vector(-1,-1,-1),
	Vector( 1,-1,-1),
	Vector(-1, 1,-1),
	Vector( 1, 1,-1),

	Vector(-1,-1, 1),
	Vector( 1,-1, 1),
	Vector(-1, 1, 1),
	Vector( 1, 1, 1),
};

// If you follow g_Offset[index], you can follow g_Offsets[GetOppositeOffset(index)] to get back.
inline int GetOppositeOffset(int offset)
{
	return NUM_FIREGRID_OFFSETS - offset - 1;
}


// ------------------------------------------------------------------------- //
// Classes.
// ------------------------------------------------------------------------- //

class C_ParticleFire : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLASS( C_ParticleFire, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();					

					C_ParticleFire();
					~C_ParticleFire();

	class FireEmitter
	{
	public:
		Vector			m_Pos;
		TimedEvent		m_SpawnEvent;
		float			m_Lifetime;			// How long it's been emitting.
		unsigned char	m_DirectionsTested;	// 1 bit for each of g_Offsets.
	};

	class FireParticle : public Particle
	{
	public:
		Vector			m_StartPos;		// The particle moves from m_StartPos to (m_StartPos+m_Direction) over its lifetime.
		Vector			m_Direction;
		
		float			m_Lifetime;
		float			m_SpinAngle;
		unsigned char	m_iRamp;		// Which fire ramp are we using?
	};


// C_BaseEntity.
public:

	virtual void	OnDataChanged( DataUpdateType_t updateType );


// IPrototypeAppEffect.
public:
	virtual void	Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs);


// IParticleEffect.
public:
	virtual void	Update(float fTimeDelta);
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );


public:
	CParticleMgr		*m_pParticleMgr;
	PMaterialHandle	m_MaterialHandle;

	// Controls where the initial fire goes.
	Vector			m_vOrigin;
	Vector			m_vDirection;

	TimedEvent		m_EmitterSpawn;
	FireEmitter		m_Emitters[MAX_FIRE_EMITTERS];
	int				m_nEmitters;

private:
					C_ParticleFire( const C_ParticleFire & );
};


// ------------------------------------------------------------------------- //
// Tables.
// ------------------------------------------------------------------------- //

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(ParticleFire, C_ParticleFire);


// Datatable..
IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_ParticleFire, DT_ParticleFire, CParticleFire)
	RecvPropVector(RECVINFO(m_vOrigin)),
	RecvPropVector(RECVINFO(m_vDirection)),
END_RECV_TABLE()



// ------------------------------------------------------------------------- //
// C_FireSmoke implementation.
// ------------------------------------------------------------------------- //
C_ParticleFire::C_ParticleFire()
{
	m_pParticleMgr = NULL;
	m_MaterialHandle = INVALID_MATERIAL_HANDLE;
}


C_ParticleFire::~C_ParticleFire()
{
	if(m_pParticleMgr)
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
}


void C_ParticleFire::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if(updateType == DATA_UPDATE_CREATED)
	{
		Start(ParticleMgr(), NULL);
	}
}


void C_ParticleFire::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	m_pParticleMgr = pParticleMgr;
	m_pParticleMgr->AddEffect( &m_ParticleEffect, this );
	m_MaterialHandle = m_ParticleEffect.FindOrAddMaterial("particle/particle_fire");

	// Start 
	m_nEmitters = 0;
	m_EmitterSpawn.Init(15);
}

static float fireSpreadDist = 15;
static float size = 20;

void C_ParticleFire::Update(float fTimeDelta)
{
	if(!m_pParticleMgr)
	{
		assert(false);
		return;
	}


	// Add new emitters.
	if(m_nEmitters < MAX_FIRE_EMITTERS)
	{
		float tempDelta = fTimeDelta;
		while(m_EmitterSpawn.NextEvent(tempDelta))
		{
			FireEmitter *pEmitter = NULL;

			if(m_nEmitters == 0)
			{
				// Make the first emitter.
				trace_t trace;
				UTIL_TraceLine(m_vOrigin, m_vOrigin+m_vDirection*1000, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &trace);
				if(trace.fraction < 1)
				{
					pEmitter = &m_Emitters[m_nEmitters];
					pEmitter->m_Pos = trace.endpos + trace.plane.normal * (size - 1);
					pEmitter->m_DirectionsTested = 0;
				}
			}
			else
			{
				static int nTries = 50;
				for(int iTry=0; iTry < nTries; iTry++)
				{
					FireEmitter *pSourceEmitter = &m_Emitters[rand() % m_nEmitters];
					
					int iOffset = rand() % NUM_FIREGRID_OFFSETS;
					if(pSourceEmitter->m_DirectionsTested & (1 << iOffset))
						continue;

					// Test the corners of the new cube. If some points are solid and some are not, then
					// we can put fire here.
					Vector basePos = pSourceEmitter->m_Pos + g_Offsets[iOffset] * fireSpreadDist;
					int nSolidCorners = 0;
					for(int iCorner=0; iCorner < NUM_FIREGRID_OFFSETS; iCorner++)
					{
						Vector vCorner = basePos + g_Offsets[iCorner]*fireSpreadDist;
						if ( enginetrace->GetPointContents(vCorner) & CONTENTS_SOLID )
							++nSolidCorners;
					}

					// Don't test this square again.
					pSourceEmitter->m_DirectionsTested |= 1 << iOffset;

					if(nSolidCorners != 0 && nSolidCorners != NUM_FIREGRID_OFFSETS)
					{
						pEmitter = &m_Emitters[m_nEmitters];
						pEmitter->m_Pos = basePos;
						pEmitter->m_DirectionsTested = 1 << GetOppositeOffset(iOffset);
					}
				}
			}
			
			if(pEmitter)
			{
				pEmitter->m_Lifetime = 0;
				pEmitter->m_SpawnEvent.Init(1);
				++m_nEmitters;
			}
		}
	}

	// Spawn particles out of the emitters.
	for(int i=0; i < m_nEmitters; i++)
	{
		FireEmitter *pEmitter = &m_Emitters[i];

		float tempDelta = fTimeDelta;
		while(pEmitter->m_SpawnEvent.NextEvent(tempDelta))
		{
			FireParticle *pParticle = (FireParticle*)m_ParticleEffect.AddParticle(sizeof(FireParticle), m_MaterialHandle);
			if(pParticle)
			{
				static float particleSpeed = 15;
				pParticle->m_StartPos = pEmitter->m_Pos;
				pParticle->m_Direction = g_FireSpreadDirection * particleSpeed + RandomVector(0, particleSpeed*0.5);
				pParticle->m_iRamp = rand() % NUM_FIRE_RAMPS;
				pParticle->m_Lifetime = 0;
			}
		}
	}
}


void C_ParticleFire::RenderParticles( CParticleRenderIterator *pIterator )
{
	const FireParticle *pParticle = (const FireParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		float smooth01 = 1 - (cos(pParticle->m_Lifetime * 3.14159 / FIRE_PARTICLE_LIFETIME) * 0.5 + 0.5);
		float smooth00 = 1 - (cos(pParticle->m_Lifetime * 3.14159 * 2 / FIRE_PARTICLE_LIFETIME) * 0.5 + 0.5);
		
		FireRamp *pRamp = &g_FireRamps[pParticle->m_iRamp];
		Vector curColor = pRamp->m_Start + (pRamp->m_End - pRamp->m_Start) * smooth01;
		
		// Render.
		Vector tPos;
		TransformParticle(m_pParticleMgr->GetModelView(), pParticle->m_Pos, tPos);
		float sortKey = (int)tPos.z;

		RenderParticle_ColorSize(
			pIterator->GetParticleDraw(),
			tPos,
			curColor,
			smooth00,
			size);

		pParticle = (const FireParticle*)pIterator->GetNext( sortKey );
	}
}


void C_ParticleFire::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	FireParticle *pParticle = (FireParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Should this particle die?
		pParticle->m_Lifetime += pIterator->GetTimeDelta();
		if(pParticle->m_Lifetime > FIRE_PARTICLE_LIFETIME)
		{
			pIterator->RemoveParticle( pParticle );
		}
		else
		{
			float smooth01 = 1 - (cos(pParticle->m_Lifetime * 3.14159 / FIRE_PARTICLE_LIFETIME) * 0.5 + 0.5);
			pParticle->m_Pos = pParticle->m_StartPos + pParticle->m_Direction * smooth01;
		}

		pParticle = (FireParticle*)pIterator->GetNext();
	}
}


