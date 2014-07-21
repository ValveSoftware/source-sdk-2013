//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements a particle system steam jet.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "baseparticleentity.h"
#include "clienteffectprecachesystem.h"
#include "fx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//NOTENOTE: Mirrored in dlls\steamjet.h
#define	STEAM_NORMAL	0
#define	STEAM_HEATWAVE	1

#define STEAMJET_NUMRAMPS			5
#define SF_EMISSIVE					0x00000001


//==================================================
// C_SteamJet
//==================================================

class C_SteamJet : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_SteamJet, C_BaseParticleEntity );

	C_SteamJet();
	~C_SteamJet();

	class SteamJetParticle : public Particle
	{
	public:
		Vector			m_Velocity;
		float			m_flRoll;
		float			m_flRollDelta;
		float			m_Lifetime;
		float			m_DieTime;
		unsigned char	m_uchStartSize;
		unsigned char	m_uchEndSize;
	};

	int IsEmissive( void ) { return ( m_spawnflags & SF_EMISSIVE ); }

//C_BaseEntity
public:

	virtual void	OnDataChanged( DataUpdateType_t updateType );


//IPrototypeAppEffect
public:
	virtual void		Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs);
	virtual bool		GetPropEditInfo(RecvTable **ppTable, void **ppObj);


//IParticleEffect
public:
	virtual void	Update(float fTimeDelta);
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );


//Stuff from the datatable
public:

	float			m_SpreadSpeed;
	float			m_Speed;
	float			m_StartSize;
	float			m_EndSize;
	float			m_Rate;
	float			m_JetLength;	// Length of the jet. Lifetime is derived from this.

	int				m_bEmit;		// Emit particles?
	int				m_nType;		// Type of particles to emit
	bool			m_bFaceLeft;	// For support of legacy env_steamjet entity, which faced left instead of forward.

	int				m_spawnflags;
	float			m_flRollSpeed;

private:

	void			UpdateLightingRamp();

private:

	// Stored the last time it updates the lighting ramp, so it can cache the values.
	Vector			m_vLastRampUpdatePos;
	QAngle			m_vLastRampUpdateAngles;

	float			m_Lifetime;		// Calculated from m_JetLength / m_Speed;

	// We sample the world to get these colors and ramp the particles.
	Vector			m_Ramps[STEAMJET_NUMRAMPS];

	CParticleMgr		*m_pParticleMgr;
	PMaterialHandle	m_MaterialHandle;
	TimedEvent		m_ParticleSpawn;

private:
					C_SteamJet( const C_SteamJet & );
};


// ------------------------------------------------------------------------- //
// Tables.
// ------------------------------------------------------------------------- //

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(SteamJet, C_SteamJet);


// Datatable..
IMPLEMENT_CLIENTCLASS_DT(C_SteamJet, DT_SteamJet, CSteamJet)
	RecvPropFloat(RECVINFO(m_SpreadSpeed), 0),
	RecvPropFloat(RECVINFO(m_Speed), 0),
	RecvPropFloat(RECVINFO(m_StartSize), 0),
	RecvPropFloat(RECVINFO(m_EndSize), 0),
	RecvPropFloat(RECVINFO(m_Rate), 0),
	RecvPropFloat(RECVINFO(m_JetLength), 0),
	RecvPropInt(RECVINFO(m_bEmit), 0),
	RecvPropInt(RECVINFO(m_bFaceLeft), 0),
	RecvPropInt(RECVINFO(m_nType), 0),
	RecvPropInt( RECVINFO( m_spawnflags ) ),
	RecvPropFloat(RECVINFO(m_flRollSpeed), 0 ),
END_RECV_TABLE()

// ------------------------------------------------------------------------- //
// C_SteamJet implementation.
// ------------------------------------------------------------------------- //
C_SteamJet::C_SteamJet()
{
	m_pParticleMgr = NULL;
	m_MaterialHandle = INVALID_MATERIAL_HANDLE;
	
	m_SpreadSpeed = 15;
	m_Speed = 120;
	m_StartSize = 10;
	m_EndSize = 25;
	m_Rate = 26;
	m_JetLength = 80;
	m_bEmit = true;
	m_bFaceLeft = false;
	m_ParticleEffect.SetAlwaysSimulate( false ); // Don't simulate outside the PVS or frustum.

	m_vLastRampUpdatePos.Init( 1e24, 1e24, 1e24 );
	m_vLastRampUpdateAngles.Init( 1e24, 1e24, 1e24 );
}


C_SteamJet::~C_SteamJet()
{
	if(m_pParticleMgr)
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
}


//-----------------------------------------------------------------------------
// Purpose: Called after a data update has occured
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_SteamJet::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if(updateType == DATA_UPDATE_CREATED)
	{
		Start(ParticleMgr(), NULL);
	}

	// Recalulate lifetime in case length or speed changed.
	m_Lifetime = m_JetLength / m_Speed;
	m_ParticleEffect.SetParticleCullRadius( MAX(m_StartSize, m_EndSize) );
}


//-----------------------------------------------------------------------------
// Purpose: Starts the effect
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_SteamJet::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	pParticleMgr->AddEffect( &m_ParticleEffect, this );
	
	switch(m_nType)
	{
	case STEAM_NORMAL:
	default:
		m_MaterialHandle = g_Mat_DustPuff[0];
		break;

	case STEAM_HEATWAVE:
		m_MaterialHandle = m_ParticleEffect.FindOrAddMaterial("sprites/heatwave");
		break;
	}

	m_ParticleSpawn.Init(m_Rate);
	m_Lifetime = m_JetLength / m_Speed;
	m_pParticleMgr = pParticleMgr;

	UpdateLightingRamp();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : **ppTable - 
//			**ppObj - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SteamJet::GetPropEditInfo( RecvTable **ppTable, void **ppObj )
{
	*ppTable = &REFERENCE_RECV_TABLE(DT_SteamJet);
	*ppObj = this;
	return true;
}


// This might be useful someday.
/*
void CalcFastApproximateRenderBoundsAABB( C_BaseEntity *pEnt, float flBloatSize, Vector *pMin, Vector *pMax )
{
	C_BaseEntity *pParent = pEnt->GetMoveParent();
	if ( pParent )
	{
		// Get the parent's abs space world bounds.
		CalcFastApproximateRenderBoundsAABB( pParent, 0, pMin, pMax );

		// Add the maximum of our local render bounds. This is making the assumption that we can be at any
		// point and at any angle within the parent's world space bounds.
		Vector vAddMins, vAddMaxs;
		pEnt->GetRenderBounds( vAddMins, vAddMaxs );

		flBloatSize += MAX( vAddMins.Length(), vAddMaxs.Length() );
	}
	else
	{
		// Start out with our own render bounds. Since we don't have a parent, this won't incur any nasty 
		pEnt->GetRenderBoundsWorldspace( *pMin, *pMax );
	}

	// Bloat the box.
	if ( flBloatSize )
	{
		*pMin -= Vector( flBloatSize, flBloatSize, flBloatSize );
		*pMax += Vector( flBloatSize, flBloatSize, flBloatSize );
	}
}
*/


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------

void C_SteamJet::Update(float fTimeDelta)
{
	if(!m_pParticleMgr)
	{
		assert(false);
		return;
	}

	if( m_bEmit )
	{
		// Add new particles.
		int nToEmit = 0;
		float tempDelta = fTimeDelta;
		while( m_ParticleSpawn.NextEvent(tempDelta) )
			++nToEmit;

		if ( nToEmit > 0 )
		{
			Vector forward, right, up;
			AngleVectors(GetAbsAngles(), &forward, &right, &up);			

			// Legacy env_steamjet entities faced left instead of forward.
			if (m_bFaceLeft)
			{
				Vector temp = forward;
				forward = -right;
				right = temp;
			}

			// EVIL: Ideally, we could tell the renderer our OBB, and let it build a big box that encloses
			// the entity with its parent so it doesn't have to setup its parent's bones here.
			Vector vEndPoint = GetAbsOrigin() + forward * m_Speed;
			Vector vMin, vMax;
			VectorMin( GetAbsOrigin(), vEndPoint, vMin );
			VectorMax( GetAbsOrigin(), vEndPoint, vMax );
			m_ParticleEffect.SetBBox( vMin, vMax );

			if ( m_ParticleEffect.WasDrawnPrevFrame() )
			{
				while ( nToEmit-- )
				{
					// Make a new particle.
					if( SteamJetParticle *pParticle = (SteamJetParticle*) m_ParticleEffect.AddParticle( sizeof(SteamJetParticle), m_MaterialHandle ) )
					{
						pParticle->m_Pos = GetAbsOrigin();
						
						pParticle->m_Velocity = 
							FRand(-m_SpreadSpeed,m_SpreadSpeed) * right +
							FRand(-m_SpreadSpeed,m_SpreadSpeed) * up +
							m_Speed * forward;
						
						pParticle->m_Lifetime	= 0;
						pParticle->m_DieTime	= m_Lifetime;

						pParticle->m_uchStartSize	= m_StartSize;
						pParticle->m_uchEndSize		= m_EndSize;

						pParticle->m_flRoll = random->RandomFloat( 0, 360 );
						pParticle->m_flRollDelta = random->RandomFloat( -m_flRollSpeed, m_flRollSpeed );
					}
				}
			}

			UpdateLightingRamp();
		}	
	}
}


// Render a quad on the screen where you pass in color and size.
// Normal is random and "flutters"
inline void RenderParticle_ColorSizePerturbNormal(
	ParticleDraw* pDraw,									
	const Vector &pos,
	const Vector &color,
	const float alpha,
	const float size
	)
{
	// Don't render totally transparent particles.
	if( alpha < 0.001f )
		return;

	CMeshBuilder *pBuilder = pDraw->GetMeshBuilder();
	if( !pBuilder )
		return;

	unsigned char ubColor[4];
	ubColor[0] = (unsigned char)RoundFloatToInt( color.x * 254.9f );
	ubColor[1] = (unsigned char)RoundFloatToInt( color.y * 254.9f );
	ubColor[2] = (unsigned char)RoundFloatToInt( color.z * 254.9f );
	ubColor[3] = (unsigned char)RoundFloatToInt( alpha * 254.9f );

	Vector vNorm;
	
	vNorm.Random( -1.0f, 1.0f );
	
	// Add the 4 corner vertices.
	pBuilder->Position3f( pos.x-size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->Normal3fv( vNorm.Base() );
	pBuilder->TexCoord2f( 0, 0, 1.0f );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x-size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->Normal3fv( vNorm.Base() );
	pBuilder->TexCoord2f( 0, 0, 0 );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y+size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->Normal3fv( vNorm.Base() );
	pBuilder->TexCoord2f( 0, 1.0f, 0 );
 	pBuilder->AdvanceVertex();

	pBuilder->Position3f( pos.x+size, pos.y-size, pos.z );
	pBuilder->Color4ubv( ubColor );
	pBuilder->Normal3fv( vNorm.Base() );
	pBuilder->TexCoord2f( 0, 1.0f, 1.0f );
 	pBuilder->AdvanceVertex();
}


void C_SteamJet::RenderParticles( CParticleRenderIterator *pIterator )
{
	const SteamJetParticle *pParticle = (const SteamJetParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Render.
		Vector tPos;
		TransformParticle(m_pParticleMgr->GetModelView(), pParticle->m_Pos, tPos);
		float sortKey = tPos.z;

		float lifetimeT = pParticle->m_Lifetime / (pParticle->m_DieTime + 0.001);
		float fRamp = lifetimeT * (STEAMJET_NUMRAMPS-1);
		int iRamp = (int)fRamp;
		float fraction = fRamp - iRamp;
		
		Vector vRampColor = m_Ramps[iRamp] + (m_Ramps[iRamp+1] - m_Ramps[iRamp]) * fraction;

		vRampColor[0] = MIN( 1.0f, vRampColor[0] );
		vRampColor[1] = MIN( 1.0f, vRampColor[1] );
		vRampColor[2] = MIN( 1.0f, vRampColor[2] );

		float sinLifetime = sin(pParticle->m_Lifetime * 3.14159f / pParticle->m_DieTime);

		if ( m_nType == STEAM_HEATWAVE )
		{
			RenderParticle_ColorSizePerturbNormal(
				pIterator->GetParticleDraw(),
				tPos,
				vRampColor,
				sinLifetime * (m_clrRender->a/255.0f),
				FLerp(m_StartSize, m_EndSize, pParticle->m_Lifetime));
		}
		else
		{
			RenderParticle_ColorSizeAngle(
				pIterator->GetParticleDraw(),
				tPos,
				vRampColor,
				sinLifetime * (m_clrRender->a/255.0f),
				FLerp(pParticle->m_uchStartSize, pParticle->m_uchEndSize, pParticle->m_Lifetime),
				pParticle->m_flRoll );
		}

		pParticle = (const SteamJetParticle*)pIterator->GetNext( sortKey );
	}
}


void C_SteamJet::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	//Don't simulate if we're emiting particles...
	//This fixes the cases where looking away from a steam jet and then looking back would cause a break on the stream.
	if ( m_ParticleEffect.WasDrawnPrevFrame() == false && m_bEmit )
		return;

	SteamJetParticle *pParticle = (SteamJetParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Should this particle die?
		pParticle->m_Lifetime += pIterator->GetTimeDelta();
		
		if( pParticle->m_Lifetime > pParticle->m_DieTime )
		{
			pIterator->RemoveParticle( pParticle );
		}
		else
		{
			pParticle->m_flRoll += pParticle->m_flRollDelta * pIterator->GetTimeDelta();
			pParticle->m_Pos = pParticle->m_Pos + pParticle->m_Velocity * pIterator->GetTimeDelta();
		}

		pParticle = (SteamJetParticle*)pIterator->GetNext();
	}
}


void C_SteamJet::UpdateLightingRamp()
{
	if( VectorsAreEqual( m_vLastRampUpdatePos, GetAbsOrigin(), 0.1 ) && 
		QAnglesAreEqual( m_vLastRampUpdateAngles, GetAbsAngles(), 0.1 ) )
	{
		return;
	}

	m_vLastRampUpdatePos = GetAbsOrigin();
	m_vLastRampUpdateAngles = GetAbsAngles();

	// Sample the world lighting where we think the particles will be.
	Vector forward, right, up;
	AngleVectors(GetAbsAngles(), &forward, &right, &up);

	// Legacy env_steamjet entities faced left instead of forward.
	if (m_bFaceLeft)
	{
		Vector temp = forward;
		forward = -right;
		right = temp;
	}

	Vector startPos = GetAbsOrigin();
	Vector endPos = GetAbsOrigin() + forward * (m_Speed * m_Lifetime);

	for(int iRamp=0; iRamp < STEAMJET_NUMRAMPS; iRamp++)
	{
		float t = (float)iRamp / (STEAMJET_NUMRAMPS-1);
		Vector vTestPos = startPos + (endPos - startPos) * t;
		
		Vector *pRamp = &m_Ramps[iRamp];
		*pRamp = WorldGetLightForPoint(vTestPos, false);
		
		if ( IsEmissive() )
		{
			pRamp->x += (m_clrRender->r/255.0f);
			pRamp->y += (m_clrRender->g/255.0f);
			pRamp->z += (m_clrRender->b/255.0f);

			pRamp->x = clamp( pRamp->x, 0.0f, 1.0f );
			pRamp->y = clamp( pRamp->y, 0.0f, 1.0f );
			pRamp->z = clamp( pRamp->z, 0.0f, 1.0f );
		}
		else
		{
			pRamp->x *= (m_clrRender->r/255.0f);
			pRamp->y *= (m_clrRender->g/255.0f);
			pRamp->z *= (m_clrRender->b/255.0f);
		}

		// Renormalize?
		float maxVal = MAX(pRamp->x, MAX(pRamp->y, pRamp->z));
		if(maxVal > 1)
		{
			*pRamp = *pRamp / maxVal;
		}
	}
}


