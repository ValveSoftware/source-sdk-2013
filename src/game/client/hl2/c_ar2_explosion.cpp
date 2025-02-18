//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "particlemgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "surfinfo.h"
#include "baseparticleentity.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------- //
// Definitions
// ------------------------------------------------------------------------- //

#define NUM_AR2_EXPLOSION_PARTICLES	70
#define AR2_DUST_RADIUS				240 // 340
#define AR2_DUST_LIFETIME			4
#define AR2_DUST_LIFETIME_DELTA		6
#define AR2_DUST_SPEED				10000
#define AR2_DUST_STARTSIZE			8
#define AR2_DUST_ENDSIZE			32
#define	AR2_DUST_ALPHA				0.5f
#define	AR2_DUST_FADE_IN_TIME		0.25f

static Vector g_AR2DustColor1(0.35, 0.345, 0.33 );
static Vector g_AR2DustColor2(0.75, 0.75, 0.7);


// ------------------------------------------------------------------------- //
// Classes
// ------------------------------------------------------------------------- //

class C_AR2Explosion : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLASS( C_AR2Explosion, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();

					C_AR2Explosion();
					~C_AR2Explosion();

private:
	
	class AR2ExplosionParticle : public StandardParticle_t
	{
	public:
		float				m_Dist;
		Vector				m_Start;
		float				m_Roll;
		float				m_RollSpeed;
		float				m_Dwell;
	};

// C_BaseEntity.
public:
	virtual void	OnDataChanged(DataUpdateType_t updateType);

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

private:

	char m_szMaterialName[255];

	C_AR2Explosion( const C_AR2Explosion & );
};

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(AR2Explosion, C_AR2Explosion);

IMPLEMENT_CLIENTCLASS_DT(C_AR2Explosion, DT_AR2Explosion, AR2Explosion)
	RecvPropString( RECVINFO( m_szMaterialName ) ),
END_RECV_TABLE()



// ------------------------------------------------------------------------- //
// Helpers.
// ------------------------------------------------------------------------- //

// Given a line segment from vStart to vEnd
// and a list of convex polygons in pSurfInfos and nSurfInfos, 
// fill in the list of which polygons the segment intersects.
// Returns the number of intersected surfaces.
static int IntersectSegmentWithSurfInfos(
	const Vector &vStart,
	const Vector &vEnd,
	SurfInfo *pSurfInfos,
	const int nSurfInfos,
	SurfInfo ** pIntersections,
	Vector *pIntersectionPositions,
	int nMaxIntersections)
{
	if(nMaxIntersections == 0)
		return 0;

	int nIntersections = 0;
	for(int i=0; i < nSurfInfos; i++)
	{
		SurfInfo *pSurf = &pSurfInfos[i];

		// Does it intersect the plane?
		float dot1 = pSurf->m_Plane.DistTo(vStart);
		float dot2 = pSurf->m_Plane.DistTo(vEnd);
		if((dot1 > 0) != (dot2 > 0))
		{
			float t = dot1 / (dot1 - dot2);
			Vector vIntersection = vStart + (vEnd - vStart) * t;
			
			// If the intersection is behind any edge plane, then it's not inside the polygon.
			unsigned long iEdge;
			for(iEdge=0; iEdge < pSurf->m_nVerts; iEdge++)
			{
				VPlane edgePlane;
				edgePlane.m_Normal = pSurf->m_Plane.m_Normal.Cross(pSurf->m_Verts[iEdge] - pSurf->m_Verts[(iEdge+1)%pSurf->m_nVerts]);
				VectorNormalize( edgePlane.m_Normal );
				edgePlane.m_Dist = edgePlane.m_Normal.Dot(pSurf->m_Verts[iEdge]);

				if(edgePlane.DistTo(vIntersection) < 0.0f)
					break;
			}

			if(iEdge == pSurf->m_nVerts)
			{
				pIntersections[nIntersections] = pSurf;
				pIntersectionPositions[nIntersections] = vIntersection;
				++nIntersections;
				if(nIntersections >= nMaxIntersections)
					break;
			}
		}
	}

	return nIntersections;
}



// ------------------------------------------------------------------------- //
// C_AR2Explosion
// ------------------------------------------------------------------------- //
C_AR2Explosion::C_AR2Explosion()
{
	m_pParticleMgr = NULL;
	m_MaterialHandle = INVALID_MATERIAL_HANDLE;
}


C_AR2Explosion::~C_AR2Explosion()
{
}


void C_AR2Explosion::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if(updateType == DATA_UPDATE_CREATED)
	{
		Start(ParticleMgr(), NULL);
	}
}

static ConVar mat_reduceparticles( "mat_reduceparticles", "0" );

void C_AR2Explosion::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	m_pParticleMgr = pParticleMgr;
	if(!pParticleMgr->AddEffect(&m_ParticleEffect, this))
		return;

	if (!m_szMaterialName[0])
	{
		Q_strncpy(m_szMaterialName, "particle/particle_noisesphere", sizeof( m_szMaterialName ) );
	}

	m_MaterialHandle = m_ParticleEffect.FindOrAddMaterial(m_szMaterialName);

	// Precalculate stuff for the particle spawning..
	#define NUM_DUSTEMITTER_SURFINFOS	128
	SurfInfo surfInfos[NUM_DUSTEMITTER_SURFINFOS];
	int nSurfInfos;

	// Center of explosion.
	Vector vCenter = GetAbsOrigin(); // HACKHACK.. when the engine bug is fixed, use origin.

	if ( IsXbox() )
	{
		m_ParticleEffect.SetBBox( vCenter-Vector(300,300,300), vCenter+Vector(300,300,300) );
	}

	#ifdef PARTICLEPROTOTYPE_APP
		float surfSize = 10000;
		nSurfInfos = 1;
		surfInfos[0].m_Verts[0].Init(-surfSize,-surfSize,0);
		surfInfos[0].m_Verts[1].Init(-surfSize,surfSize,0);
		surfInfos[0].m_Verts[2].Init(surfSize, surfSize,0);
		surfInfos[0].m_Verts[3].Init(surfSize,-surfSize,0);
		surfInfos[0].m_nVerts = 4;
		surfInfos[0].m_Plane.m_Normal.Init(0,0,1);
		surfInfos[0].m_Plane.m_Dist = -3;
	#else
		{
			nSurfInfos = 0;
			C_BaseEntity *ent = cl_entitylist->GetEnt( 0 );
			if ( ent )
			{
				nSurfInfos = engine->GetIntersectingSurfaces(
					ent->GetModel(),
					vCenter,
					AR2_DUST_RADIUS,
					true,
					surfInfos,
					NUM_DUSTEMITTER_SURFINFOS);
			}
		}
	#endif

	int nParticles = 0;

	int iParticlesToSpawn = NUM_AR2_EXPLOSION_PARTICLES;

	// In DX7, much fewer particles
	if ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 80 )
	{
		iParticlesToSpawn *= 0.25;
	}
	else if ( mat_reduceparticles.GetBool() )
	{
		iParticlesToSpawn *= 0.025;
	}

	if( nSurfInfos > 0 )
	{
		// For nParticles*N, generate a ray and cast it out. If it hits anything, spawn a particle there.
		int nTestsPerParticle=3;
		for(int i=0; i < iParticlesToSpawn; i++)
		{
			for(int iTest=0; iTest < nTestsPerParticle; iTest++)
			{
				Vector randVec = RandomVector(-1,1);
				VectorNormalize( randVec );
				Vector startPos = vCenter + randVec * AR2_DUST_RADIUS;

				randVec = RandomVector(-1,1);
				VectorNormalize( randVec );
				Vector endPos = vCenter + randVec * AR2_DUST_RADIUS;

				#define MAX_SURFINFO_INTERSECTIONS	4
				SurfInfo *pIntersected[MAX_SURFINFO_INTERSECTIONS];
				Vector vIntersections[MAX_SURFINFO_INTERSECTIONS];
				int nIntersections;
				nIntersections = IntersectSegmentWithSurfInfos(
					startPos, 
					endPos, 
					surfInfos, 
					nSurfInfos, 
					pIntersected,
					vIntersections,
					MAX_SURFINFO_INTERSECTIONS);
				
				if(nIntersections)
				{
					int iIntersection = rand() % nIntersections;

					Vector velocity;
					//velocity.Init(-1.0f + ((float)rand()/VALVE_RAND_MAX) * 2.0f, -1.0f + ((float)rand()/VALVE_RAND_MAX) * 2.0f, -1.0f + ((float)rand()/VALVE_RAND_MAX) * 2.0f);
					//velocity = velocity * FRand(m_MinSpeed, m_MaxSpeed);
					Vector direction = (vIntersections[iIntersection] - vCenter );
					float dist = VectorNormalize( direction );
					if(dist > AR2_DUST_RADIUS)
						dist = AR2_DUST_RADIUS;

					static float power = 2.0f;
					float falloffMul = pow(1.0f - dist / AR2_DUST_RADIUS, power);

					Vector reflection = direction - 2 * DotProduct( direction, pIntersected[iIntersection]->m_Plane.m_Normal ) * pIntersected[iIntersection]->m_Plane.m_Normal;
					VectorNormalize( reflection );

					velocity = reflection * AR2_DUST_SPEED * falloffMul;
					// velocity = velocity + (vIntersections[iIntersection] - vCenter) * falloffMul;

					
					/*
					debugoverlay->AddLineOverlay( vIntersections[iIntersection], 
												  vIntersections[iIntersection] + reflection * 64,
												  128, 128, 255, false, 15.0 );
					*/
#if 1
					AR2ExplosionParticle *pParticle = 
						(AR2ExplosionParticle*)m_ParticleEffect.AddParticle( sizeof(AR2ExplosionParticle), m_MaterialHandle );

					if(pParticle)
					{
						pParticle->m_Pos = vIntersections[iIntersection];
						pParticle->m_Start = pParticle->m_Pos;
						pParticle->m_Dist = 8.0;
						pParticle->m_Velocity = velocity;
						// sound == 13031.496062992125984251968503937ips
						pParticle->m_Lifetime = -dist / 13031.5f - 0.1;
						pParticle->m_Roll = FRand( 0, M_PI * 2 );
						pParticle->m_RollSpeed = FRand( -1, 1 ) * 0.4;
						pParticle->m_Dwell = AR2_DUST_LIFETIME + random->RandomFloat( 0, AR2_DUST_LIFETIME_DELTA );
						nParticles++;
						break;
					}
#endif
				}
			}
		}
	}	

	// build interior smoke particles
	for(int i=nParticles; i < iParticlesToSpawn; i++)
	{
		Vector randVec = RandomVector(-1,1);
		VectorNormalize( randVec );
		Vector endPos = vCenter + randVec * AR2_DUST_RADIUS / 4.0;

		Vector direction = (endPos - vCenter );
		float dist = VectorNormalize( direction ) + random->RandomFloat( 0, AR2_DUST_RADIUS / 4.0 );
		if(dist > AR2_DUST_RADIUS)
			dist = AR2_DUST_RADIUS;

		static float power = 2.0f;
		float falloffMul = pow(1.0f - dist / (AR2_DUST_RADIUS / 2), power);

		Vector velocity = direction * AR2_DUST_SPEED * falloffMul;
		AR2ExplosionParticle *pParticle = 
			(AR2ExplosionParticle*)m_ParticleEffect.AddParticle( sizeof(AR2ExplosionParticle), m_MaterialHandle );

		if(pParticle)
		{
			pParticle->m_Pos = endPos;
			pParticle->m_Start = pParticle->m_Pos;
			pParticle->m_Dist = 8.0;
			pParticle->m_Velocity = velocity;
			// sound == 13031.496062992125984251968503937ips
			pParticle->m_Lifetime = -dist / 13031.5f - 0.1;
			pParticle->m_Roll = FRand( 0, M_PI * 2 );
			pParticle->m_RollSpeed = FRand( -1, 1 ) * 4.0;
			pParticle->m_Dwell = 0.5 * (AR2_DUST_LIFETIME + random->RandomFloat( 0, AR2_DUST_LIFETIME_DELTA ));
		}
	}
}


void C_AR2Explosion::Update(float fTimeDelta)
{
	if(!m_pParticleMgr)
		return;
}


void C_AR2Explosion::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	float dt = pIterator->GetTimeDelta();

	AR2ExplosionParticle *pParticle = (AR2ExplosionParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		if (dt > 0.05)
			dt = 0.05; // yuck, air resistance function craps out at less then 20fps

		// Update its lifetime.
		pParticle->m_Lifetime += dt; // pDraw->GetTimeDelta();
		if(pParticle->m_Lifetime > pParticle->m_Dwell)
		{
			// faded to nothing....
			pIterator->RemoveParticle( pParticle );
		}
		else
		{
			// Spin the thing
			pParticle->m_Roll += pParticle->m_RollSpeed * pIterator->GetTimeDelta();

			// delayed?
			if ( pParticle->m_Lifetime >= 0.0f )
			{
				// Move it (this comes after rendering to make it clear that moving the particle here won't change
				// its rendering for this frame since m_TransformedPos has already been set).
				pParticle->m_Pos = pParticle->m_Pos + pParticle->m_Velocity * dt;

				// keep track of distance traveled
				pParticle->m_Dist = pParticle->m_Dist + pParticle->m_Velocity.Length() * dt;

				// Dampen velocity.
				float dist = pParticle->m_Velocity.Length()	* dt;
				float r = dist * dist;
				// FIXME: this is a really screwy air-resistance function....
				pParticle->m_Velocity = pParticle->m_Velocity * (100 / (100 + r )); 

				// dampen roll
				static float dtime;
				static float decay;
				if (dtime != dt)
				{
					dtime = dt;
					decay = ExponentialDecay( 0.3, 1.0, dtime );
				}
				if (fabs(pParticle->m_RollSpeed) > 0.2)
					pParticle->m_RollSpeed = pParticle->m_RollSpeed * decay;
			}
		}

		pParticle = (AR2ExplosionParticle*)pIterator->GetNext();
	}
}


void C_AR2Explosion::RenderParticles( CParticleRenderIterator *pIterator )
{
	const AR2ExplosionParticle *pParticle = (const AR2ExplosionParticle *)pIterator->GetFirst();
	while ( pParticle )
	{
		float sortKey = 0;
		if ( pParticle->m_Lifetime >= 0.0f )
		{
			// Draw.
			float lifetimePercent = ( pParticle->m_Lifetime - AR2_DUST_FADE_IN_TIME ) / pParticle->m_Dwell;

			// FIXME: base color should be a dirty version of the material color
			Vector color = g_AR2DustColor1 * (1.0 - lifetimePercent) + g_AR2DustColor2 * lifetimePercent;
			
			Vector tPos;
			TransformParticle(m_pParticleMgr->GetModelView(), pParticle->m_Pos, tPos);
			sortKey = tPos.z;
			
			float	alpha;

			if ( pParticle->m_Lifetime < AR2_DUST_FADE_IN_TIME )
			{
				alpha = AR2_DUST_ALPHA * ( pParticle->m_Lifetime / AR2_DUST_FADE_IN_TIME );
			}
			else
			{
				alpha = AR2_DUST_ALPHA * ( 1.0f - lifetimePercent );
			}

			alpha *= GetAlphaDistanceFade( tPos, IsXbox() ? 100 : 50, IsXbox() ? 200 : 150 );

			RenderParticle_ColorSizeAngle(
				pIterator->GetParticleDraw(),
				tPos,
				color,
				alpha,
				pParticle->m_Dist, // size based on how far it's traveled
				pParticle->m_Roll);
		}

		pParticle = (const AR2ExplosionParticle *)pIterator->GetNext( sortKey );
	}
}


