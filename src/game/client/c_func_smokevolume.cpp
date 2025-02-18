//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_smoke_trail.h"
#include "smoke_fog_overlay.h"
#include "engine/IEngineTrace.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_EMISSIVE	0x00000001

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

// ------------------------------------------------------------------------- //
// Classes
// ------------------------------------------------------------------------- //
class C_FuncSmokeVolume : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLASS( C_FuncSmokeVolume, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();

	C_FuncSmokeVolume();
	~C_FuncSmokeVolume();

	int IsEmissive( void ) { return ( m_spawnflags & SF_EMISSIVE ); }

private:
	class SmokeGrenadeParticle : public Particle
	{
	public:
		float				m_RotationFactor;
		float				m_CurRotation;
		float				m_FadeAlpha;		// Set as it moves around.
		unsigned char		m_ColorInterp;		// Amount between min and max colors.
		unsigned char		m_Color[4];
	};

// C_BaseEntity.
public:
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	
// IPrototypeAppEffect.
public:
	virtual void	Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs );

// IParticleEffect.
public:
	virtual void	Update(float fTimeDelta);
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );
	virtual void	NotifyRemove();

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

	inline int GetSmokeParticleIndex(int x, int y, int z)	
	{
		Assert( IsValidXYZCoords( x, y, z ) );
		return z*m_xCount*m_yCount+y*m_xCount+x;
	}

	inline SmokeParticleInfo *GetSmokeParticleInfo(int x, int y, int z)	
	{
		Assert( IsValidXYZCoords( x, y, z ) );
		return &m_pSmokeParticleInfos[GetSmokeParticleIndex(x,y,z)];
	}

	inline void	GetParticleInfoXYZ(int index_, int &x, int &y, int &z)
	{
		Assert( index_ >= 0 && index_ < m_xCount * m_yCount * m_zCount );
		z = index_ / (m_xCount*m_yCount);
		int zIndex = z*m_xCount*m_yCount;
		y = (index_ - zIndex) / m_xCount;
		int yIndex = y*m_xCount;
		x = index_ - zIndex - yIndex;
		Assert( IsValidXYZCoords( x, y, z ) );
	}

	inline bool IsValidXYZCoords(int x, int y, int z)
	{
		return x >= 0 && y >= 0 && z >= 0 && x < m_xCount && y < m_yCount && z < m_zCount;
	}

	inline Vector GetSmokeParticlePos(int x, int y, int z )	
	{
		return WorldAlignMins() + 
			Vector( x * m_SpacingRadius * 2 + m_SpacingRadius,
				    y * m_SpacingRadius * 2 + m_SpacingRadius,
				    z * m_SpacingRadius * 2 + m_SpacingRadius );
	}

	inline Vector GetSmokeParticlePosIndex(int index_ )
	{
		int x, y, z;
		GetParticleInfoXYZ( index_, x, y, z);
		return GetSmokeParticlePos(x, y, z);
	}

	// Start filling the smoke volume
	void FillVolume();

private:
// State variables from server.
	color32 m_Color1;
	color32 m_Color2;
	char m_MaterialName[255];
	float m_ParticleDrawWidth;
	float m_ParticleSpacingDistance;
	float m_DensityRampSpeed;
	float m_RotationSpeed;
	float m_MovementSpeed;
	float m_Density;
	int	  m_spawnflags;

private:
	C_FuncSmokeVolume( const C_FuncSmokeVolume & );

	float				m_CurrentDensity;
	float				m_ParticleRadius;
	bool				m_bStarted;

	PMaterialHandle		m_MaterialHandle;

	SmokeParticleInfo	*m_pSmokeParticleInfos;
	int					m_xCount, m_yCount, m_zCount;
	float				m_SpacingRadius;

	Vector				m_MinColor;
	Vector				m_MaxColor;

	Vector m_vLastOrigin;
	QAngle m_vLastAngles;
	bool m_bFirstUpdate;
};

IMPLEMENT_CLIENTCLASS_DT( C_FuncSmokeVolume, DT_FuncSmokeVolume, CFuncSmokeVolume )
	RecvPropInt( RECVINFO( m_Color1 ), 0, RecvProxy_IntToColor32 ),
	RecvPropInt( RECVINFO( m_Color2 ), 0, RecvProxy_IntToColor32 ),
	RecvPropString( RECVINFO( m_MaterialName ) ),
	RecvPropFloat( RECVINFO( m_ParticleDrawWidth ) ),
	RecvPropFloat( RECVINFO( m_ParticleSpacingDistance ) ),
	RecvPropFloat( RECVINFO( m_DensityRampSpeed ) ),
	RecvPropFloat( RECVINFO( m_RotationSpeed ) ),
	RecvPropFloat( RECVINFO( m_MovementSpeed ) ),
	RecvPropFloat( RECVINFO( m_Density ) ),
	RecvPropInt( RECVINFO( m_spawnflags ) ),
	RecvPropDataTable( RECVINFO_DT( m_Collision ), 0, &REFERENCE_RECV_TABLE(DT_CollisionProperty) ),
END_RECV_TABLE()

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

static inline Vector& EngineGetVecRenderOrigin()
{
#if defined(PARTICLEPROTOTYPE_APP)
	static Vector dummy(0,0,0);
	return dummy;
#else
	extern Vector g_vecRenderOrigin;
	return g_vecRenderOrigin;
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

static inline C_BaseEntity* ParticleGetEntity( int index )
{
#if defined(PARTICLEPROTOTYPE_APP)
	return NULL;
#else
	return cl_entitylist->GetEnt( index );
#endif
}

// ------------------------------------------------------------------------- //
// C_FuncSmokeVolume
// ------------------------------------------------------------------------- //
C_FuncSmokeVolume::C_FuncSmokeVolume()
{
	m_bFirstUpdate = true;
	m_vLastOrigin.Init();
	m_vLastAngles.Init();

	m_pSmokeParticleInfos = NULL;
	m_SpacingRadius = 0.0f;
	m_ParticleRadius = 0.0f;
	m_MinColor.Init( 1.0, 1.0, 1.0 );
	m_MaxColor.Init( 1.0, 1.0, 1.0 );
}

C_FuncSmokeVolume::~C_FuncSmokeVolume()
{
	delete [] m_pSmokeParticleInfos;
}

static ConVar mat_reduceparticles( "mat_reduceparticles", "0" );

void C_FuncSmokeVolume::OnDataChanged( DataUpdateType_t updateType )
{		
	m_MinColor[0] = ( 1.0f / 255.0f ) * m_Color1.r;
	m_MinColor[1] = ( 1.0f / 255.0f ) * m_Color1.g;
	m_MinColor[2] = ( 1.0f / 255.0f ) * m_Color1.b;

	m_MaxColor[0] = ( 1.0f / 255.0f ) * m_Color2.r;
	m_MaxColor[1] = ( 1.0f / 255.0f ) * m_Color2.g;
	m_MaxColor[2] = ( 1.0f / 255.0f ) * m_Color2.b;

	if ( mat_reduceparticles.GetBool() )
	{
		m_Density *= 0.5f;
		m_ParticleSpacingDistance *= 1.5f;
	}

	m_ParticleRadius = m_ParticleDrawWidth * 0.5f;
	m_SpacingRadius = m_ParticleSpacingDistance * 0.5f;

	m_ParticleEffect.SetParticleCullRadius( m_ParticleRadius );

//	Warning( "m_Density: %f\n", m_Density );
//	Warning( "m_MovementSpeed: %f\n", m_MovementSpeed );
	
	if( updateType == DATA_UPDATE_CREATED )
	{
		Vector size = WorldAlignMaxs() - WorldAlignMins();
		m_xCount = 0.5f + ( size.x / ( m_SpacingRadius * 2.0f ) );
		m_yCount = 0.5f + ( size.y / ( m_SpacingRadius * 2.0f ) );
		m_zCount = 0.5f + ( size.z / ( m_SpacingRadius * 2.0f ) );
		m_CurrentDensity = m_Density;

		delete [] m_pSmokeParticleInfos;
		m_pSmokeParticleInfos = new SmokeParticleInfo[m_xCount * m_yCount * m_zCount];
		Start( ParticleMgr(), NULL );
	}
	BaseClass::OnDataChanged( updateType );
}

void C_FuncSmokeVolume::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	if( !pParticleMgr->AddEffect( &m_ParticleEffect, this ) )
		return;

	m_MaterialHandle = m_ParticleEffect.FindOrAddMaterial( m_MaterialName );
	FillVolume();

	m_bStarted = true;
}


void C_FuncSmokeVolume::Update( float fTimeDelta )
{
	// Update our world space bbox if we've moved at all.
	// We do this manually because sometimes people make HUGE bboxes, and if they're constantly changing because their
	// particles wander outside the current bounds sometimes, it'll be linking them into all the leaves repeatedly.
	const Vector &curOrigin = GetAbsOrigin();
	const QAngle &curAngles = GetAbsAngles();
	if ( !VectorsAreEqual( curOrigin, m_vLastOrigin, 0.1 ) || 
		fabs( curAngles.x - m_vLastAngles.x ) > 0.1 || 
		fabs( curAngles.y - m_vLastAngles.y ) > 0.1 || 
		fabs( curAngles.z - m_vLastAngles.z ) > 0.1 ||
		m_bFirstUpdate )
	{
		m_bFirstUpdate = false;
		m_vLastAngles = curAngles;
		m_vLastOrigin = curOrigin;

		Vector vWorldMins, vWorldMaxs;
		CollisionProp()->WorldSpaceAABB( &vWorldMins, &vWorldMaxs );
		vWorldMins -= Vector( m_ParticleRadius, m_ParticleRadius, m_ParticleRadius );
		vWorldMaxs += Vector( m_ParticleRadius, m_ParticleRadius, m_ParticleRadius );

		m_ParticleEffect.SetBBox( vWorldMins, vWorldMaxs );
	}
		
	// lerp m_CurrentDensity towards m_Density at a rate of m_DensityRampSpeed
	if( m_CurrentDensity < m_Density )
	{
		m_CurrentDensity += m_DensityRampSpeed * fTimeDelta;
		if( m_CurrentDensity > m_Density )
		{
			m_CurrentDensity = m_Density;
		}
	}
	else if( m_CurrentDensity > m_Density )
	{
		m_CurrentDensity -= m_DensityRampSpeed * fTimeDelta;
		if( m_CurrentDensity < m_Density )
		{
			m_CurrentDensity = m_Density;
		}
	}

	if( m_CurrentDensity == 0.0f )
	{
		return;
	}
	
	// This is used to randomize the direction it chooses to move a particle in.

	int offsetLookup[3] = {-1,0,1};

	float tradeDurationMax = m_ParticleSpacingDistance / ( m_MovementSpeed + 0.1f );
	float tradeDurationMin = tradeDurationMax * 0.5f;

	if ( IS_NAN( tradeDurationMax ) || IS_NAN( tradeDurationMin ) )
		return;

//	Warning( "tradeDuration: [%f,%f]\n", tradeDurationMin, tradeDurationMax );
	
	// Update all the moving traders and establish new ones.
	int nTotal = m_xCount * m_yCount * m_zCount;
	for( int i=0; i < nTotal; i++ )
	{
		SmokeParticleInfo *pInfo = &m_pSmokeParticleInfos[i];

		if(!pInfo->m_pParticle)
			continue;
	
		if(pInfo->m_TradeIndex == -1)
		{
			pInfo->m_pParticle->m_FadeAlpha = pInfo->m_FadeAlpha;
			pInfo->m_pParticle->m_Color[0] = pInfo->m_Color[0];
			pInfo->m_pParticle->m_Color[1] = pInfo->m_Color[1];
			pInfo->m_pParticle->m_Color[2] = pInfo->m_Color[2];

			// Is there an adjacent one that's not trading?
			int x, y, z;
			GetParticleInfoXYZ(i, x, y, z);

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
						int testX = x + offsetLookup[(xCount+xCountOffset) % 3];
						int testY = y + offsetLookup[(yCount+yCountOffset) % 3];
						int testZ = z + offsetLookup[(zCount+zCountOffset) % 3];

						if(testX == x && testY == y && testZ == z)
							continue;

						if(IsValidXYZCoords(testX, testY, testZ))
						{
							SmokeParticleInfo *pOther = GetSmokeParticleInfo(testX, testY, testZ);
							if(pOther->m_pParticle && pOther->m_TradeIndex == -1)
							{
								// Ok, this one is looking to trade also.
								pInfo->m_TradeIndex = GetSmokeParticleIndex(testX, testY, testZ);
								pOther->m_TradeIndex = i;
								pInfo->m_TradeClock = pOther->m_TradeClock = 0;
								pOther->m_TradeDuration = pInfo->m_TradeDuration = FRand( tradeDurationMin, tradeDurationMax );
								
								bFound = true;
								break;
							}
						}
					}
				}
			}
		}
		else
		{
			SmokeParticleInfo *pOther = &m_pSmokeParticleInfos[pInfo->m_TradeIndex];
			assert(pOther->m_TradeIndex == i);
			
			// This makes sure the trade only gets updated once per frame.
			if(pInfo < pOther)
			{
				// Increment the trade clock..
				pInfo->m_TradeClock = (pOther->m_TradeClock += fTimeDelta);
				int x, y, z;
				GetParticleInfoXYZ(i, x, y, z);
				Vector myPos = GetSmokeParticlePos(x, y, z);
				
				int otherX, otherY, otherZ;
				GetParticleInfoXYZ(pInfo->m_TradeIndex, otherX, otherY, otherZ);
				Vector otherPos = GetSmokeParticlePos(otherX, otherY, otherZ);

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
	}
}


void C_FuncSmokeVolume::RenderParticles( CParticleRenderIterator *pIterator )
{
	if ( m_CurrentDensity == 0 )
		return;

	const SmokeGrenadeParticle *pParticle = (const SmokeGrenadeParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		Vector renderPos = pParticle->m_Pos;

		// Fade out globally.
		float alpha = m_CurrentDensity;

		// Apply the precalculated fade alpha from world geometry.
		alpha *= pParticle->m_FadeAlpha;
		
		// TODO: optimize this whole routine!
		Vector color = m_MinColor + (m_MaxColor - m_MinColor) * (pParticle->m_ColorInterp / 255.1f);
		if ( IsEmissive() )
		{
			color.x += pParticle->m_Color[0] / 255.0f;
			color.y += pParticle->m_Color[1] / 255.0f;
			color.z += pParticle->m_Color[2] / 255.0f;

			color.x = clamp( color.x, 0.0f, 1.0f );
			color.y = clamp( color.y, 0.0f, 1.0f );
			color.z = clamp( color.z, 0.0f, 1.0f );
		}
		else
		{
			color.x *= pParticle->m_Color[0] / 255.0f;
			color.y *= pParticle->m_Color[1] / 255.0f;
			color.z *= pParticle->m_Color[2] / 255.0f;
		}
		
		Vector tRenderPos;
		TransformParticle( ParticleMgr()->GetModelView(), renderPos, tRenderPos );
		float sortKey = 1;//tRenderPos.z;

		// If we're reducing particle cost, only render sufficiently opaque particles 
		if ( ( alpha > 0.05f ) || !mat_reduceparticles.GetBool() )
		{
			RenderParticle_ColorSizeAngle(
				pIterator->GetParticleDraw(),
				tRenderPos,
				color,
				alpha * GetAlphaDistanceFade(tRenderPos, 10, 30),	// Alpha
				m_ParticleRadius,
				pParticle->m_CurRotation
				);
		}

		pParticle = (const SmokeGrenadeParticle*)pIterator->GetNext( sortKey );
	}
}


void C_FuncSmokeVolume::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	if ( m_CurrentDensity == 0 )
		return;

	SmokeGrenadeParticle *pParticle = (SmokeGrenadeParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		pParticle->m_CurRotation += pParticle->m_RotationFactor * ( M_PI / 180.0f ) * m_RotationSpeed * pIterator->GetTimeDelta();
		pParticle = (SmokeGrenadeParticle*)pIterator->GetNext();
	}
}


void C_FuncSmokeVolume::NotifyRemove()
{
	m_xCount = m_yCount = m_zCount = 0;
}


void C_FuncSmokeVolume::FillVolume()
{
	Vector vPos;
	for(int x=0; x < m_xCount; x++)
	{
		for(int y=0; y < m_yCount; y++)
		{
			for(int z=0; z < m_zCount; z++)
			{
				vPos = GetSmokeParticlePos( x, y, z );
				if(SmokeParticleInfo *pInfo = GetSmokeParticleInfo(x,y,z))
				{
					int contents = GetWorldPointContents(vPos);
					if(contents & CONTENTS_SOLID)
					{
						pInfo->m_pParticle = NULL;
					}
					else
					{
						SmokeGrenadeParticle *pParticle = 
							(SmokeGrenadeParticle*)m_ParticleEffect.AddParticle(sizeof(SmokeGrenadeParticle), m_MaterialHandle);

						if(pParticle)
						{
							pParticle->m_Pos = vPos;
							pParticle->m_ColorInterp = (unsigned char)((rand() * 255) / VALVE_RAND_MAX);
							pParticle->m_RotationFactor = FRand( -1.0f, 1.0f ); // Rotation factor.
							pParticle->m_CurRotation = FRand( -m_RotationSpeed, m_RotationSpeed );
						}

#ifdef _DEBUG
						int testX, testY, testZ;
						int index_ = GetSmokeParticleIndex(x,y,z);
						GetParticleInfoXYZ( index_, testX, testY, testZ);
						assert(testX == x && testY == y && testZ == z);
#endif

						Vector vColor = EngineGetLightForPoint(vPos);
						pInfo->m_Color[0] = LinearToTexture( vColor.x );
						pInfo->m_Color[1] = LinearToTexture( vColor.y );
						pInfo->m_Color[2] = LinearToTexture( vColor.z );

						// Cast some rays and if it's too close to anything, fade its alpha down.
						pInfo->m_FadeAlpha = 1;

						for(int i=0; i < NUM_FADE_PLANES; i++)
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
								else if(dist < m_ParticleRadius)
								{
									float alphaScale = dist / m_ParticleRadius;
									alphaScale *= alphaScale * alphaScale;
									pInfo->m_FadeAlpha *= alphaScale;
								}
							}
						}

						pInfo->m_pParticle = pParticle;
						pInfo->m_TradeIndex = -1;
					}
				}
			}
		}
	}
}
