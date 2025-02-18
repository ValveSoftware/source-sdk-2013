//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "c_smoke_trail.h"
#include "smoke_fog_overlay.h"
#include "engine/IEngineTrace.h"
#include "view.h"
#include "dlight.h"
#include "iefx.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "engine/ivdebugoverlay.h"

#if CSTRIKE_DLL
#include "c_cs_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------- //
// Definitions
// ------------------------------------------------------------------------- //

static Vector s_FadePlaneDirections[] =
{
	Vector( 1,0,0),
	Vector(-1,0,0),
	Vector(0, 1,0),
	Vector(0,-1,0),
	Vector(0,0, 1),
	Vector(0,0,-1)
};
#define NUM_FADE_PLANES	(sizeof(s_FadePlaneDirections)/sizeof(s_FadePlaneDirections[0]))

// This is used to randomize the direction it chooses to move a particle in.
int g_OffsetLookup[3] = {-1,0,1};


// ------------------------------------------------------------------------- //
// Classes
// ------------------------------------------------------------------------- //
class C_ParticleSmokeGrenade : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLASS( C_ParticleSmokeGrenade, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();

					C_ParticleSmokeGrenade();
					~C_ParticleSmokeGrenade();

private:
	
	class SmokeGrenadeParticle : public Particle
	{
	public:
		float				m_RotationSpeed;
		float				m_CurRotation;
		float				m_FadeAlpha;		// Set as it moves around.
		unsigned char		m_ColorInterp;		// Amount between min and max colors.
		unsigned char		m_Color[4];
	};


public:

	// Optional call. It will use defaults if you don't call this.
	void			SetParams(
		);

	// Call this to move the source..
	void			SetPos(const Vector &pos);


// C_BaseEntity.
public:
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual void	CleanupToolRecordingState( KeyValues *msg );

// IPrototypeAppEffect.
public:
	virtual void	Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs);


// IParticleEffect.
public:
	virtual void	Update(float fTimeDelta);
	virtual void	RenderParticles( CParticleRenderIterator *pIterator );
	virtual void	SimulateParticles( CParticleSimulateIterator *pIterator );
	virtual void	NotifyRemove();
	virtual void	GetParticlePosition( Particle *pParticle, Vector& worldpos );
	virtual void	ClientThink();


// Proxies.
public:

	static void		RecvProxy_CurrentStage(  const CRecvProxyData *pData, void *pStruct, void *pOut );


private:

	// The SmokeEmitter represents a grid in 3D space.
	class SmokeParticleInfo
	{
	public:
		SmokeGrenadeParticle	*m_pParticle;
		int						m_TradeIndex;		// -1 if not exchanging yet.
		float					m_TradeClock;		// How long since they started trading.
		float					m_TradeDuration;	// How long the trade will take to finish.
		float					m_FadeAlpha;		// Calculated from nearby world geometry.
		unsigned char			m_Color[4];
	};

	void ApplyDynamicLight( const Vector &vParticlePos, Vector &color );
	void UpdateDynamicLightList( const Vector &vMins, const Vector &vMaxs );

	void UpdateSmokeTrail( float fTimeDelta );
	
	void UpdateParticleAndFindTrade( int iParticle, float fTimeDelta );
	void UpdateParticleDuringTrade( int iParticle, float flTimeDelta );

	inline int					GetSmokeParticleIndex(int x, int y, int z)	{return z*m_xCount*m_yCount+y*m_yCount+x;}
	inline SmokeParticleInfo*	GetSmokeParticleInfo(int x, int y, int z)	{return &m_SmokeParticleInfos[GetSmokeParticleIndex(x,y,z)];}
	inline void					GetParticleInfoXYZ(int index_, int &x, int &y, int &z)
	{
		z = index_ / (m_xCount*m_yCount);
		int zIndex = z*m_xCount*m_yCount;
		y = (index_ - zIndex) / m_yCount;
		int yIndex = y*m_yCount;
		x = index_ - zIndex - yIndex;
	}

	inline bool					IsValidXYZCoords(int x, int y, int z)
	{
		return x >= 0 && y >= 0 && z >= 0 && x < m_xCount && y < m_yCount && z < m_zCount;
	}

	inline Vector				GetSmokeParticlePos(int x, int y, int z)	
	{
		return m_SmokeBasePos + 
			Vector( ((float)x / (m_xCount-1)) * m_SpacingRadius * 2 - m_SpacingRadius,
				((float)y / (m_yCount-1)) * m_SpacingRadius * 2 - m_SpacingRadius,
				((float)z / (m_zCount-1)) * m_SpacingRadius * 2 - m_SpacingRadius);
	}

	inline Vector				GetSmokeParticlePosIndex(int index_)
	{
		int x, y, z;
		GetParticleInfoXYZ( index_, x, y, z);
		return GetSmokeParticlePos(x, y, z);
	}

	inline const Vector&		GetPos()	{ return GetAbsOrigin(); }

	// Start filling the smoke volume (and stop the smoke trail).
	void						FillVolume();


// State variables from server.
public:
	
	unsigned char		m_CurrentStage;
	Vector				m_SmokeBasePos;

	// What time the effect was initially created
	float				m_flSpawnTime;

	// It will fade out during this time.
	float				m_FadeStartTime;
	float				m_FadeEndTime;
	float				m_FadeAlpha;	// Calculated from the fade start/end times each frame.

	// Used during rendering.. active dlights.
	class CActiveLight
	{
	public:
		Vector m_vColor;
		Vector m_vOrigin;
		float m_flRadiusSqr;
	};
	CActiveLight		m_ActiveLights[MAX_DLIGHTS];
	int					m_nActiveLights;


private:
						C_ParticleSmokeGrenade( const C_ParticleSmokeGrenade & );

	bool				m_bStarted;
	bool				m_bVolumeFilled;
	PMaterialHandle		m_MaterialHandles[NUM_MATERIAL_HANDLES];

	SmokeParticleInfo	m_SmokeParticleInfos[NUM_PARTICLES_PER_DIMENSION*NUM_PARTICLES_PER_DIMENSION*NUM_PARTICLES_PER_DIMENSION];
	int					m_xCount, m_yCount, m_zCount;
	float				m_SpacingRadius;

	Vector				m_MinColor;
	Vector				m_MaxColor;

	float				m_ExpandTimeCounter;	// How long since we started expanding.	
	float				m_ExpandRadius;			// How large is our radius.

	C_SmokeTrail		m_SmokeTrail;
};


// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(SmokeGrenade, C_ParticleSmokeGrenade);


// Datatable..
IMPLEMENT_CLIENTCLASS_DT(C_ParticleSmokeGrenade, DT_ParticleSmokeGrenade, ParticleSmokeGrenade)
	RecvPropTime(RECVINFO(m_flSpawnTime)),
	RecvPropFloat(RECVINFO(m_FadeStartTime)),
	RecvPropFloat(RECVINFO(m_FadeEndTime)),
	RecvPropInt(RECVINFO(m_CurrentStage), 0, &C_ParticleSmokeGrenade::RecvProxy_CurrentStage),
END_RECV_TABLE()


// ------------------------------------------------------------------------- //
// Helpers.
// ------------------------------------------------------------------------- //

static inline void InterpColor(unsigned char dest[4], unsigned char src1[4], unsigned char src2[4], float percent)
{
	dest[0] = (unsigned char)(src1[0] + (src2[0] - src1[0]) * percent);
	dest[1] = (unsigned char)(src1[1] + (src2[1] - src1[1]) * percent);
	dest[2] = (unsigned char)(src1[2] + (src2[2] - src1[2]) * percent);
}


static inline int GetWorldPointContents(const Vector &vPos)
{
	#if defined(PARTICLEPROTOTYPE_APP)
		return 0;
	#else
		return enginetrace->GetPointContents( vPos );
	#endif
}

static inline void WorldTraceLine( const Vector &start, const Vector &end, int contentsMask, trace_t *trace )
{
	#if defined(PARTICLEPROTOTYPE_APP)
		trace->fraction = 1;
	#else
		UTIL_TraceLine(start, end, contentsMask, NULL, COLLISION_GROUP_NONE, trace);
	#endif
}

static inline Vector EngineGetLightForPoint(const Vector &vPos)
{
	#if defined(PARTICLEPROTOTYPE_APP)
		return Vector(1,1,1);
	#else
		return engine->GetLightForPoint(vPos, true);
	#endif
}

static inline const Vector& EngineGetVecRenderOrigin()
{
	#if defined(PARTICLEPROTOTYPE_APP)
		static Vector dummy(0,0,0);
		return dummy;
	#else
		return CurrentViewOrigin();
	#endif
}

static inline float& EngineGetSmokeFogOverlayAlpha()
{
	#if defined(PARTICLEPROTOTYPE_APP)
		static float dummy;
		return dummy;
	#else
		return g_SmokeFogOverlayAlpha;
	#endif
}

static inline C_BaseEntity* ParticleGetEntity(int index)
{
	#if defined(PARTICLEPROTOTYPE_APP)
		return NULL;
	#else
		return cl_entitylist->GetEnt(index);
	#endif
}



// ------------------------------------------------------------------------- //
// ParticleMovieExplosion
// ------------------------------------------------------------------------- //
C_ParticleSmokeGrenade::C_ParticleSmokeGrenade()
{
	memset(m_MaterialHandles, 0, sizeof(m_MaterialHandles));

	m_MinColor.Init(0.5, 0.5, 0.5);
	m_MaxColor.Init(0.6, 0.6, 0.6 );

	m_nActiveLights = 0;
	m_ExpandRadius = 0;
	m_ExpandTimeCounter = 0;
	m_FadeStartTime = 0;
	m_FadeEndTime = 0;
	m_flSpawnTime = 0;
	m_bVolumeFilled = false;
	m_CurrentStage = 0;

	m_bStarted = false;
}


C_ParticleSmokeGrenade::~C_ParticleSmokeGrenade()
{
	ParticleMgr()->RemoveEffect( &m_ParticleEffect );
}


void C_ParticleSmokeGrenade::SetParams(
	)
{
}


void C_ParticleSmokeGrenade::OnDataChanged( DataUpdateType_t updateType )
{
	C_BaseEntity::OnDataChanged(updateType);

	if(updateType == DATA_UPDATE_CREATED )
	{
		Start(ParticleMgr(), NULL);
	}
}


void C_ParticleSmokeGrenade::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	if(!pParticleMgr->AddEffect( &m_ParticleEffect, this ))
		return;
	
	m_SmokeTrail.Start(pParticleMgr, pArgs);

	m_SmokeTrail.m_ParticleLifetime = 0.5;
	m_SmokeTrail.SetSpawnRate(40);
	m_SmokeTrail.m_MinSpeed = 0;
	m_SmokeTrail.m_MaxSpeed = 0;
	m_SmokeTrail.m_StartSize = 3;
	m_SmokeTrail.m_EndSize = 10;
	m_SmokeTrail.m_SpawnRadius = 0;

	m_SmokeTrail.SetLocalOrigin( GetAbsOrigin() );

	for(int i=0; i < NUM_MATERIAL_HANDLES; i++)
	{
		char str[256];
		Q_snprintf(str, sizeof( str ), "particle/particle_smokegrenade%d", i+1);
		m_MaterialHandles[i] = m_ParticleEffect.FindOrAddMaterial(str);
	}

	if( m_CurrentStage == 2 )
	{
		FillVolume();
	}

	// Go straight into "fill volume" mode if they want.
	if(pArgs)
	{
		if(pArgs->FindArg("-FillVolume"))
		{
			FillVolume();
		}
	}

	m_bStarted = true;
	SetNextClientThink( CLIENT_THINK_ALWAYS );

#if CSTRIKE_DLL
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();

	if ( pPlayer )
	{
		 pPlayer->m_SmokeGrenades.AddToTail( this );
	}
#endif
		 
}

void C_ParticleSmokeGrenade::ClientThink()
{
	if ( m_CurrentStage == 1 )
	{
		// Add our influence to the global smoke fog alpha.
		
		float testDist = (MainViewOrigin() - m_SmokeBasePos ).Length();

		float fadeEnd = m_ExpandRadius;

		// The center of the smoke cloud that always gives full fog overlay
		float flCoreDistance = fadeEnd * 0.3;
		
		if(testDist < fadeEnd)
		{			
			if( testDist < flCoreDistance )
			{
				EngineGetSmokeFogOverlayAlpha() += m_FadeAlpha;
			}
			else
			{
				EngineGetSmokeFogOverlayAlpha() += (1 - ( testDist - flCoreDistance ) / ( fadeEnd - flCoreDistance ) ) * m_FadeAlpha;
			}
		}	
	}
}


void C_ParticleSmokeGrenade::UpdateSmokeTrail( float fTimeDelta )
{
	C_BaseEntity *pAimEnt = GetFollowedEntity();
	if ( pAimEnt )
	{
		Vector forward, right, up;

		// Update the smoke particle color.
		if(m_CurrentStage == 0)
		{
			m_SmokeTrail.m_StartColor = EngineGetLightForPoint(GetAbsOrigin()) * 0.5f;
			m_SmokeTrail.m_EndColor = m_SmokeTrail.m_StartColor;
		}

		// Spin the smoke trail.
		AngleVectors(pAimEnt->GetAbsAngles(), &forward, &right, &up);
		m_SmokeTrail.m_VelocityOffset = forward * 30 + GetAbsVelocity();

		m_SmokeTrail.SetLocalOrigin( GetAbsOrigin() );
		m_SmokeTrail.Update(fTimeDelta);
	}	
}


inline void C_ParticleSmokeGrenade::UpdateParticleDuringTrade( int iParticle, float fTimeDelta )
{
	SmokeParticleInfo *pInfo = &m_SmokeParticleInfos[iParticle];
	SmokeParticleInfo *pOther = &m_SmokeParticleInfos[pInfo->m_TradeIndex];
	Assert(pOther->m_TradeIndex == iParticle);
	
	// This makes sure the trade only gets updated once per frame.
	if(pInfo < pOther)
	{
		// Increment the trade clock..
		pInfo->m_TradeClock = (pOther->m_TradeClock += fTimeDelta);
		int x, y, z;
		GetParticleInfoXYZ(iParticle, x, y, z);
		Vector myPos = GetSmokeParticlePos(x, y, z) - m_SmokeBasePos;
		
		int otherX, otherY, otherZ;
		GetParticleInfoXYZ(pInfo->m_TradeIndex, otherX, otherY, otherZ);
		Vector otherPos = GetSmokeParticlePos(otherX, otherY, otherZ) - m_SmokeBasePos;

		// Is the trade finished?
		if(pInfo->m_TradeClock >= pInfo->m_TradeDuration)
		{
			pInfo->m_TradeIndex = pOther->m_TradeIndex = -1;
			
			pInfo->m_pParticle->m_Pos = otherPos;
			pOther->m_pParticle->m_Pos = myPos;

			SmokeGrenadeParticle *temp = pInfo->m_pParticle;
			pInfo->m_pParticle = pOther->m_pParticle;
			pOther->m_pParticle = temp;
		}
		else
		{			
			// Ok, move them closer.
			float percent = (float)cos(pInfo->m_TradeClock * 2 * 1.57079632f / pInfo->m_TradeDuration);
			percent = percent * 0.5 + 0.5;
			
			pInfo->m_pParticle->m_FadeAlpha  = pInfo->m_FadeAlpha + (pOther->m_FadeAlpha - pInfo->m_FadeAlpha) * (1 - percent);
			pOther->m_pParticle->m_FadeAlpha = pInfo->m_FadeAlpha + (pOther->m_FadeAlpha - pInfo->m_FadeAlpha) * percent;

			InterpColor(pInfo->m_pParticle->m_Color,  pInfo->m_Color, pOther->m_Color, 1-percent);
			InterpColor(pOther->m_pParticle->m_Color, pInfo->m_Color, pOther->m_Color, percent);

			pInfo->m_pParticle->m_Pos  = myPos + (otherPos - myPos) * (1 - percent);
			pOther->m_pParticle->m_Pos = myPos + (otherPos - myPos) * percent;
		}
	}
}


void C_ParticleSmokeGrenade::UpdateParticleAndFindTrade( int iParticle, float fTimeDelta )
{
	SmokeParticleInfo *pInfo = &m_SmokeParticleInfos[iParticle];

	pInfo->m_pParticle->m_FadeAlpha = pInfo->m_FadeAlpha;
	pInfo->m_pParticle->m_Color[0] = pInfo->m_Color[0];
	pInfo->m_pParticle->m_Color[1] = pInfo->m_Color[1];
	pInfo->m_pParticle->m_Color[2] = pInfo->m_Color[2];

	// Is there an adjacent one that's not trading?
	int x, y, z;
	GetParticleInfoXYZ(iParticle, x, y, z);

	int xCountOffset = rand();
	int yCountOffset = rand();
	int zCountOffset = rand();

	bool bFound = false;
	for(int xCount=0; xCount < 3 && !bFound; xCount++)
	{
		for(int yCount=0; yCount < 3 && !bFound; yCount++)
		{
			for(int zCount=0; zCount < 3; zCount++)
			{
				int testX = x + g_OffsetLookup[(xCount+xCountOffset) % 3];
				int testY = y + g_OffsetLookup[(yCount+yCountOffset) % 3];
				int testZ = z + g_OffsetLookup[(zCount+zCountOffset) % 3];

				if(testX == x && testY == y && testZ == z)
					continue;

				if(IsValidXYZCoords(testX, testY, testZ))
				{
					SmokeParticleInfo *pOther = GetSmokeParticleInfo(testX, testY, testZ);
					if(pOther->m_pParticle && pOther->m_TradeIndex == -1)
					{
						// Ok, this one is looking to trade also.
						pInfo->m_TradeIndex = GetSmokeParticleIndex(testX, testY, testZ);
						pOther->m_TradeIndex = iParticle;
						pInfo->m_TradeClock = pOther->m_TradeClock = 0;
						pInfo->m_TradeDuration = FRand(TRADE_DURATION_MIN, TRADE_DURATION_MAX);
						
						bFound = true;
						break;
					}
				}
			}
		}
	}
}


void C_ParticleSmokeGrenade::Update(float fTimeDelta)
{
	float flLifetime = gpGlobals->curtime - m_flSpawnTime;

	// Update the smoke trail.
	UpdateSmokeTrail( fTimeDelta );
	
	// Update our fade alpha.
	if(flLifetime < m_FadeStartTime)
	{
		m_FadeAlpha = 1;
	}
	else if(flLifetime < m_FadeEndTime)
	{
		float fadePercent = (flLifetime - m_FadeStartTime) / (m_FadeEndTime - m_FadeStartTime);
		m_FadeAlpha = cos(fadePercent * 3.14159) * 0.5 + 0.5;
	}
	else
	{
		m_FadeAlpha = 0;
	}

	// Scale by the amount the sphere has grown.
	m_FadeAlpha *= m_ExpandRadius / (m_SpacingRadius*2);

	
	// Update our bbox.

	Vector vMins = m_SmokeBasePos - Vector( m_SpacingRadius + SMOKEGRENADE_PARTICLERADIUS, m_SpacingRadius + SMOKEGRENADE_PARTICLERADIUS, m_SpacingRadius + SMOKEGRENADE_PARTICLERADIUS );
	Vector vMaxs = m_SmokeBasePos + Vector( m_SpacingRadius + SMOKEGRENADE_PARTICLERADIUS, m_SpacingRadius + SMOKEGRENADE_PARTICLERADIUS, m_SpacingRadius + SMOKEGRENADE_PARTICLERADIUS );
	m_ParticleEffect.SetBBox( vMins, vMaxs );


	// Update the current light list.
	UpdateDynamicLightList( vMins, vMaxs );


	if(m_CurrentStage == 1)
	{
		// Update the expanding sphere.
		m_ExpandTimeCounter = flLifetime;
		if(m_ExpandTimeCounter > SMOKESPHERE_EXPAND_TIME)
			m_ExpandTimeCounter = SMOKESPHERE_EXPAND_TIME;

		m_ExpandRadius = (m_SpacingRadius*2) * (float)sin(m_ExpandTimeCounter * M_PI * 0.5 / SMOKESPHERE_EXPAND_TIME);

//		debugoverlay->AddBoxOverlay( GetPos(), Vector( -m_ExpandRadius, -m_ExpandRadius, -m_ExpandRadius), Vector( m_ExpandRadius, m_ExpandRadius, m_ExpandRadius), vec3_angle, 0, 255, 0, 1, 1.0f );


		// Update all the moving traders and establish new ones.
		int nTotal = m_xCount * m_yCount * m_zCount;
		for(int i=0; i < nTotal; i++)
		{
			SmokeParticleInfo *pInfo = &m_SmokeParticleInfos[i];

			if(!pInfo->m_pParticle)
				continue;
		
			if(pInfo->m_TradeIndex == -1)
			{
				UpdateParticleAndFindTrade( i, fTimeDelta );
			}
			else
			{
				UpdateParticleDuringTrade( i, fTimeDelta );
			}
		}
	}

	m_SmokeBasePos = GetPos();
}


void C_ParticleSmokeGrenade::UpdateDynamicLightList( const Vector &vMins, const Vector &vMaxs )
{
	dlight_t *lights[MAX_DLIGHTS];
	int nLights = effects->CL_GetActiveDLights( lights );
	m_nActiveLights = 0;
	for ( int i=0; i < nLights; i++ )
	{
		dlight_t *pIn = lights[i];
		if ( pIn->origin.x + pIn->radius <= vMins.x || 
			 pIn->origin.y + pIn->radius <= vMins.y || 
			 pIn->origin.z + pIn->radius <= vMins.z || 
			 pIn->origin.x - pIn->radius >= vMaxs.x || 
			 pIn->origin.y - pIn->radius >= vMaxs.y || 
			 pIn->origin.z - pIn->radius >= vMaxs.z )
		{
		}
		else
		{
			CActiveLight *pOut = &m_ActiveLights[m_nActiveLights];
			if ( (pIn->color.r != 0 || pIn->color.g != 0 || pIn->color.b != 0) && pIn->color.exponent != 0 )
			{
				ColorRGBExp32ToVector( pIn->color, pOut->m_vColor );
				pOut->m_vColor /= 255.0f;
				pOut->m_flRadiusSqr = (pIn->radius + SMOKEPARTICLE_SIZE) * (pIn->radius + SMOKEPARTICLE_SIZE);
				pOut->m_vOrigin = pIn->origin;
				++m_nActiveLights;
			}
		}
	}
}


inline void C_ParticleSmokeGrenade::ApplyDynamicLight( const Vector &vParticlePos, Vector &color )
{
	if ( m_nActiveLights )
	{
		for ( int i=0; i < m_nActiveLights; i++ )
		{
			CActiveLight *pLight = &m_ActiveLights[i];

			float flDistSqr = (vParticlePos - pLight->m_vOrigin).LengthSqr();
			if ( flDistSqr < pLight->m_flRadiusSqr )
			{
				color += pLight->m_vColor * (1 - flDistSqr / pLight->m_flRadiusSqr) * 0.1f;
			}
		}
	
		// Rescale the color..
		float flMax = MAX( color.x, MAX( color.y, color.z ) );
		if ( flMax > 1 )
		{
			color /= flMax;
		}
	}
}


void C_ParticleSmokeGrenade::RenderParticles( CParticleRenderIterator *pIterator )
{
	const SmokeGrenadeParticle *pParticle = (const SmokeGrenadeParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		Vector vWorldSpacePos = m_SmokeBasePos + pParticle->m_Pos;

		float sortKey;

		// Draw.
		float len = pParticle->m_Pos.Length();
		if ( len > m_ExpandRadius )
		{
			Vector vTemp;
			TransformParticle(ParticleMgr()->GetModelView(), vWorldSpacePos, vTemp);
			sortKey = vTemp.z;		
		}
		else
		{
			// This smooths out the growing sphere. Rather than having particles appear in one spot as the sphere
			// expands, they stay at the borders.
			Vector renderPos;
			if(len > m_ExpandRadius * 0.5f)
			{
				renderPos = m_SmokeBasePos + (pParticle->m_Pos * (m_ExpandRadius * 0.5f)) / len;
			}
			else
			{
				renderPos = vWorldSpacePos;
			}		

			// Figure out the alpha based on where it is in the sphere.
			float alpha = 1 - len / m_ExpandRadius;
			
			// This changes the ramp to be very solid in the core, then taper off.
			static float testCutoff=0.3;
			if(alpha > testCutoff)
			{
				alpha = 1;
			}
			else
			{
				// at testCutoff it's 1, at 0, it's 0
				alpha = alpha / testCutoff;
			}

			// Fade out globally.
			alpha *= m_FadeAlpha;

			// Apply the precalculated fade alpha from world geometry.
			alpha *= pParticle->m_FadeAlpha;

			// TODO: optimize this whole routine!
			Vector color = m_MinColor + (m_MaxColor - m_MinColor) * (pParticle->m_ColorInterp / 255.1f);
			color.x *= pParticle->m_Color[0] / 255.0f;
			color.y *= pParticle->m_Color[1] / 255.0f;
			color.z *= pParticle->m_Color[2] / 255.0f;

			// Lighting.
			ApplyDynamicLight( renderPos, color );

			color = (color + Vector( 0.5, 0.5, 0.5 )) / 2;   //Desaturate
			
			Vector tRenderPos;
			TransformParticle(ParticleMgr()->GetModelView(), renderPos, tRenderPos);
			sortKey = tRenderPos.z;

			//debugoverlay->AddBoxOverlay( renderPos, Vector( -2, -2, -2), Vector( 2, 2, 2), vec3_angle, 255, 255, 255, 255, 1.0f );

			RenderParticle_ColorSizeAngle(
				pIterator->GetParticleDraw(),
				tRenderPos,
				color,
				alpha * GetAlphaDistanceFade(tRenderPos, 0, 10),	// Alpha
				SMOKEPARTICLE_SIZE,
				pParticle->m_CurRotation
				);
		}

		pParticle = (SmokeGrenadeParticle*)pIterator->GetNext( sortKey );
	}
}


void C_ParticleSmokeGrenade::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	SmokeGrenadeParticle *pParticle = (SmokeGrenadeParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		pParticle->m_CurRotation += pParticle->m_RotationSpeed * pIterator->GetTimeDelta();
		pParticle = (SmokeGrenadeParticle*)pIterator->GetNext();
	}
}


void C_ParticleSmokeGrenade::NotifyRemove()
{
	m_xCount = m_yCount = m_zCount = 0;

#if CSTRIKE_DLL
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();

	if ( pPlayer )
	{
		 pPlayer->m_SmokeGrenades.FindAndRemove( this );
	}
#endif

}


void C_ParticleSmokeGrenade::GetParticlePosition( Particle *pParticle, Vector& worldpos )
{
	worldpos = pParticle->m_Pos + m_SmokeBasePos;
}


void C_ParticleSmokeGrenade::RecvProxy_CurrentStage(  const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_ParticleSmokeGrenade *pGrenade = (C_ParticleSmokeGrenade*)pStruct;
	Assert( pOut == &pGrenade->m_CurrentStage );

	if ( pGrenade && pGrenade->m_CurrentStage == 0 && pData->m_Value.m_Int == 1 )
	{
		if( pGrenade->m_bStarted )
			pGrenade->FillVolume();
		else
			pGrenade->m_CurrentStage = 2;
	}
}

void C_ParticleSmokeGrenade::FillVolume()
{
	m_CurrentStage = 1;
	m_SmokeBasePos = GetPos();
	m_SmokeTrail.SetEmit(false);
	m_ExpandTimeCounter = m_ExpandRadius = 0;
	m_bVolumeFilled = true;

	// Spawn all of our particles.
	float overlap = SMOKEPARTICLE_OVERLAP;

	m_SpacingRadius = (SMOKEGRENADE_PARTICLERADIUS - overlap) * NUM_PARTICLES_PER_DIMENSION * 0.5f;
	m_xCount = m_yCount = m_zCount = NUM_PARTICLES_PER_DIMENSION;

	float invNumPerDimX = 1.0f / (m_xCount-1);
	float invNumPerDimY = 1.0f / (m_yCount-1);
	float invNumPerDimZ = 1.0f / (m_zCount-1);

	Vector vPos;
	for(int x=0; x < m_xCount; x++)
	{
		vPos.x = m_SmokeBasePos.x + ((float)x * invNumPerDimX) * m_SpacingRadius * 2 - m_SpacingRadius;

		for(int y=0; y < m_yCount; y++)
		{
			vPos.y = m_SmokeBasePos.y + ((float)y * invNumPerDimY) * m_SpacingRadius * 2 - m_SpacingRadius;
							  
			for(int z=0; z < m_zCount; z++)
			{
				vPos.z = m_SmokeBasePos.z + ((float)z * invNumPerDimZ) * m_SpacingRadius * 2 - m_SpacingRadius;

				// Don't spawn and simulate particles that are inside a wall
//				int contents = enginetrace->GetPointContents( vPos );

				// Culling out particles in solid makes smoke not fill up small passageways.
				//if( contents & CONTENTS_SOLID )
				//{
				//	continue;
				//}

				if(SmokeParticleInfo *pInfo = GetSmokeParticleInfo(x,y,z))
				{
					// MD 11/10/03: disabled this because we weren't getting coverage near the ground.
					// If we want it back in certain cases, we can make it a flag.
					/*int contents = GetWorldPointContents(vPos);
					if(false && (contents & CONTENTS_SOLID))
					{
						pInfo->m_pParticle = NULL;
					}
					else
					*/
					{
						SmokeGrenadeParticle *pParticle = 
							(SmokeGrenadeParticle*)m_ParticleEffect.AddParticle(sizeof(SmokeGrenadeParticle), m_MaterialHandles[rand() % NUM_MATERIAL_HANDLES]);

						if(pParticle)
						{
							pParticle->m_Pos = vPos - m_SmokeBasePos; // store its position in local space
							pParticle->m_ColorInterp = (unsigned char)((rand() * 255) / VALVE_RAND_MAX);
							pParticle->m_RotationSpeed = FRand(-ROTATION_SPEED, ROTATION_SPEED); // Rotation speed.
							pParticle->m_CurRotation = FRand(-6, 6);

							//debugoverlay->AddBoxOverlay( vPos, Vector( -2, -2, -2), Vector( 2, 2, 2), vec3_angle, 255, 0, 0, 255, 5.0f );
						}

						

						#ifdef _DEBUG
							int testX, testY, testZ;
							int index_ = GetSmokeParticleIndex(x,y,z);
							GetParticleInfoXYZ( index_, testX, testY, testZ);
							assert(testX == x && testY == y && testZ == z);
						#endif

						Vector vColor = EngineGetLightForPoint(vPos);
						pInfo->m_Color[0] = (unsigned char)(vColor.x * 255.9f);
						pInfo->m_Color[1] = (unsigned char)(vColor.y * 255.9f);
						pInfo->m_Color[2] = (unsigned char)(vColor.z * 255.9f);

						// Cast some rays and if it's too close to anything, fade its alpha down.
						pInfo->m_FadeAlpha = 1;

						/*for(int i=0; i < NUM_FADE_PLANES; i++)
						{
							trace_t trace;
							WorldTraceLine(vPos, vPos + s_FadePlaneDirections[i] * 100, MASK_SOLID_BRUSHONLY, &trace);
							if(trace.fraction < 1.0f)
							{
								float dist = DotProduct(trace.plane.normal, vPos) - trace.plane.dist;
								if(dist < 0)
								{
									pInfo->m_FadeAlpha = 0;
								}
								else if(dist < SMOKEPARTICLE_SIZE)
								{
									float alphaScale = dist / SMOKEPARTICLE_SIZE;
									alphaScale *= alphaScale * alphaScale;
									pInfo->m_FadeAlpha *= alphaScale;
								}
							}
						}*/

						pInfo->m_pParticle = pParticle;
						pInfo->m_TradeIndex = -1;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// This is called after sending this entity's recording state
//-----------------------------------------------------------------------------
void C_ParticleSmokeGrenade::CleanupToolRecordingState( KeyValues *msg )
{
	if ( !ToolsEnabled() )
		return;

	BaseClass::CleanupToolRecordingState( msg );
	m_SmokeTrail.CleanupToolRecordingState( msg );

	// Generally, this is used to allow the entity to clean up
	// allocated state it put into the message, but here we're going
	// to use it to send particle system messages because we
	// know the grenade has been recorded at this point
	if ( !clienttools->IsInRecordingMode() )
		return;
	
	// NOTE: Particle system destruction message will be sent by the particle effect itself.
	if ( m_bVolumeFilled && GetToolParticleEffectId() == TOOLPARTICLESYSTEMID_INVALID )
	{
		// Needed for retriggering of the smoke grenade
		m_bVolumeFilled = false;

		int nId = AllocateToolParticleEffectId();

		KeyValues *oldmsg = new KeyValues( "OldParticleSystem_Create" );
		oldmsg->SetString( "name", "C_ParticleSmokeGrenade" );
		oldmsg->SetInt( "id", nId );
		oldmsg->SetFloat( "time", gpGlobals->curtime );

		KeyValues *pEmitter = oldmsg->FindKey( "DmeSpriteEmitter", true );
		pEmitter->SetInt( "count", NUM_PARTICLES_PER_DIMENSION * NUM_PARTICLES_PER_DIMENSION * NUM_PARTICLES_PER_DIMENSION );
		pEmitter->SetFloat( "duration", 0 );
		pEmitter->SetString( "material", "particle/particle_smokegrenade1" );
		pEmitter->SetInt( "active", true );

		KeyValues *pInitializers = pEmitter->FindKey( "initializers", true );

		KeyValues *pPosition = pInitializers->FindKey( "DmeVoxelPositionInitializer", true );
		pPosition->SetFloat( "centerx", m_SmokeBasePos.x );
		pPosition->SetFloat( "centery", m_SmokeBasePos.y );
		pPosition->SetFloat( "centerz", m_SmokeBasePos.z );
		pPosition->SetFloat( "particlesPerDimension", m_xCount );
		pPosition->SetFloat( "particleSpacing", m_SpacingRadius );

		KeyValues *pLifetime = pInitializers->FindKey( "DmeRandomLifetimeInitializer", true );
		pLifetime->SetFloat( "minLifetime", m_FadeEndTime );
 		pLifetime->SetFloat( "maxLifetime", m_FadeEndTime );

		KeyValues *pVelocity = pInitializers->FindKey( "DmeAttachmentVelocityInitializer", true );
		pVelocity->SetPtr( "entindex", (void*)(intp)entindex() );
 		pVelocity->SetFloat( "minRandomSpeed", 10 );
 		pVelocity->SetFloat( "maxRandomSpeed", 20 );

		KeyValues *pRoll = pInitializers->FindKey( "DmeRandomRollInitializer", true );
		pRoll->SetFloat( "minRoll", -6.0f );
 		pRoll->SetFloat( "maxRoll", 6.0f );

		KeyValues *pRollSpeed = pInitializers->FindKey( "DmeRandomRollSpeedInitializer", true );
		pRollSpeed->SetFloat( "minRollSpeed", -ROTATION_SPEED );
 		pRollSpeed->SetFloat( "maxRollSpeed", ROTATION_SPEED );

		KeyValues *pColor = pInitializers->FindKey( "DmeRandomInterpolatedColorInitializer", true );
		Color c1( 
			FastFToC( clamp( m_MinColor.x, 0.f, 1.f ) ),
			FastFToC( clamp( m_MinColor.y, 0.f, 1.f ) ),
			FastFToC( clamp( m_MinColor.z, 0.f, 1.f ) ), 255 );
		Color c2( 
			FastFToC( clamp( m_MaxColor.x, 0.f, 1.f ) ),
			FastFToC( clamp( m_MaxColor.y, 0.f, 1.f ) ),
			FastFToC( clamp( m_MaxColor.z, 0.f, 1.f ) ), 255 );
		pColor->SetColor( "color1", c1 );
		pColor->SetColor( "color2", c2 );

		KeyValues *pAlpha = pInitializers->FindKey( "DmeRandomAlphaInitializer", true );
		pAlpha->SetInt( "minStartAlpha", 255 );
		pAlpha->SetInt( "maxStartAlpha", 255 );
		pAlpha->SetInt( "minEndAlpha", 0 );
		pAlpha->SetInt( "maxEndAlpha", 0 );

		KeyValues *pSize = pInitializers->FindKey( "DmeRandomSizeInitializer", true );
		pSize->SetFloat( "minStartSize", SMOKEPARTICLE_SIZE );
		pSize->SetFloat( "maxStartSize", SMOKEPARTICLE_SIZE );
		pSize->SetFloat( "minEndSize", SMOKEPARTICLE_SIZE );
		pSize->SetFloat( "maxEndSize", SMOKEPARTICLE_SIZE );

		pInitializers->FindKey( "DmeSolidKillInitializer", true );

		KeyValues *pUpdaters = pEmitter->FindKey( "updaters", true );

		pUpdaters->FindKey( "DmeRollUpdater", true );
		pUpdaters->FindKey( "DmeColorUpdater", true );

		KeyValues *pAlphaCosineUpdater = pUpdaters->FindKey( "DmeAlphaCosineUpdater", true );
		pAlphaCosineUpdater->SetFloat( "duration", m_FadeEndTime - m_FadeStartTime );
		
		pUpdaters->FindKey( "DmeColorDynamicLightUpdater", true );

		KeyValues *pSmokeGrenadeUpdater = pUpdaters->FindKey( "DmeSmokeGrenadeUpdater", true );
 		pSmokeGrenadeUpdater->SetFloat( "centerx", m_SmokeBasePos.x );
		pSmokeGrenadeUpdater->SetFloat( "centery", m_SmokeBasePos.y );
		pSmokeGrenadeUpdater->SetFloat( "centerz", m_SmokeBasePos.z );
		pSmokeGrenadeUpdater->SetFloat( "particlesPerDimension", m_xCount );
		pSmokeGrenadeUpdater->SetFloat( "particleSpacing", m_SpacingRadius );
		pSmokeGrenadeUpdater->SetFloat( "radiusExpandTime", SMOKESPHERE_EXPAND_TIME );
		pSmokeGrenadeUpdater->SetFloat( "cutoffFraction", 0.7f );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, oldmsg );
		oldmsg->deleteThis();
	}
}



