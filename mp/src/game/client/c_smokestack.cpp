//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements a particle system steam jet.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "particle_prototype.h"
#include "baseparticleentity.h"
#include "particles_simple.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef HL2_EPISODIC
	#define SMOKESTACK_MAX_MATERIALS 8
#else
	#define SMOKESTACK_MAX_MATERIALS 1
#endif

//==================================================
// C_SmokeStack
//==================================================

class C_SmokeStack : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_SmokeStack, C_BaseParticleEntity );

	C_SmokeStack();
	~C_SmokeStack();

	class SmokeStackParticle : public Particle
	{
	public:
		Vector		m_Velocity;
		Vector		m_vAccel;
		float		m_Lifetime;
		float		m_flAngle;
		float		m_flRollDelta;
		float		m_flSortPos;
	};

//C_BaseEntity
public:
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink();

//IPrototypeAppEffect
public:
	virtual void	Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs);
	virtual bool	GetPropEditInfo(RecvTable **ppTable, void **ppObj);


//IParticleEffect
public:
	virtual void	Update(float fTimeDelta);
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );
	virtual void	StartRender( VMatrix &effectMatrix );


private:

	void QueueLightParametersInRenderer();


//Stuff from the datatable
public:

	CParticleSphereRenderer	m_Renderer;

	float			m_SpreadSpeed;
	float			m_Speed;
	float			m_StartSize;
	float			m_EndSize;
	float			m_Rate;
	float			m_JetLength;	// Length of the jet. Lifetime is derived from this.

	int				m_bEmit;		// Emit particles?
	float			m_flBaseSpread;

	class CLightInfo
	{
	public:
		Vector		m_vPos;
		Vector		m_vColor;
		float		m_flIntensity;
	};

	// Note: there are two ways the directional light can be specified. The default is to use 
	// DirLightColor and a default dirlight source (from above or below).
	//		In this case, m_DirLight.m_vPos and m_DirLight.m_flIntensity are ignored.
	//
	// The other is to attach a directional env_particlelight to us.
	//		In this case, m_DirLightSource is ignored and all the m_DirLight parameters are used.
	CParticleLightInfo	m_AmbientLight;
	CParticleLightInfo	m_DirLight;

	Vector			m_vBaseColor;
	
	Vector			m_vWind;
	float			m_flTwist;
	int				m_iMaterialModel;

private:
	C_SmokeStack( const C_SmokeStack & );

	float			m_TwistMat[2][2];
	int				m_bTwist;

	float			m_flAlphaScale;
	float			m_InvLifetime;		// Calculated from m_JetLength / m_Speed;

	CParticleMgr		*m_pParticleMgr;
	PMaterialHandle	m_MaterialHandle[SMOKESTACK_MAX_MATERIALS];
	TimedEvent		m_ParticleSpawn;
	int				m_iMaxFrames;
	bool			m_bInView;
	float			m_flRollSpeed;
};


// ------------------------------------------------------------------------- //
// Tables.
// ------------------------------------------------------------------------- //

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(SmokeStack, C_SmokeStack);


IMPLEMENT_CLIENTCLASS_DT(C_SmokeStack, DT_SmokeStack, CSmokeStack)
	RecvPropFloat(RECVINFO(m_SpreadSpeed), 0),
	RecvPropFloat(RECVINFO(m_Speed), 0),
	RecvPropFloat(RECVINFO(m_StartSize), 0),
	RecvPropFloat(RECVINFO(m_EndSize), 0),
	RecvPropFloat(RECVINFO(m_Rate), 0),
	RecvPropFloat(RECVINFO(m_JetLength), 0),
	RecvPropInt(RECVINFO(m_bEmit), 0),
	RecvPropFloat(RECVINFO(m_flBaseSpread)),
	RecvPropFloat(RECVINFO(m_flTwist)),
	RecvPropFloat(RECVINFO(m_flRollSpeed )),
	RecvPropIntWithMinusOneFlag( RECVINFO( m_iMaterialModel ) ),

	RecvPropVector( RECVINFO(m_AmbientLight.m_vPos) ),
	RecvPropVector( RECVINFO(m_AmbientLight.m_vColor) ),
	RecvPropFloat( RECVINFO(m_AmbientLight.m_flIntensity) ),

	RecvPropVector( RECVINFO(m_DirLight.m_vPos) ),
	RecvPropVector( RECVINFO(m_DirLight.m_vColor) ),
	RecvPropFloat( RECVINFO(m_DirLight.m_flIntensity) ),

	RecvPropVector(RECVINFO(m_vWind))
END_RECV_TABLE()



// ------------------------------------------------------------------------- //
// C_SmokeStack implementation.
// ------------------------------------------------------------------------- //
C_SmokeStack::C_SmokeStack()
{
	m_pParticleMgr = NULL;
	m_MaterialHandle[0] = INVALID_MATERIAL_HANDLE;
	m_iMaterialModel = -1;
	
	m_SpreadSpeed = 15;
	m_Speed = 30;
	m_StartSize = 10;
	m_EndSize = 15;
	m_Rate = 80;
	m_JetLength = 180;
	m_bEmit = true;

	m_flBaseSpread = 20;
	m_bInView = false;

	// Lighting is (base color) + (ambient / dist^2) + bump(directional / dist^2)
	// By default, we use bottom-up lighting for the directional.
	SetRenderColor( 0, 0, 0, 255 );

	m_AmbientLight.m_vPos.Init(0,0,-100);
	m_AmbientLight.m_vColor.Init( 40, 40, 40 );
	m_AmbientLight.m_flIntensity = 8000;

	m_DirLight.m_vColor.Init( 255, 128, 0 );
	
	m_vWind.Init();

	m_flTwist = 0;
}


C_SmokeStack::~C_SmokeStack()
{
	if(m_pParticleMgr)
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
}


//-----------------------------------------------------------------------------
// Purpose: Called after a data update has occured
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_SmokeStack::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if(updateType == DATA_UPDATE_CREATED)
	{
		Start(ParticleMgr(), NULL);
	}

	// Recalulate lifetime in case length or speed changed.
	m_InvLifetime = m_Speed / m_JetLength;
}


static ConVar mat_reduceparticles( "mat_reduceparticles", "0" );

//-----------------------------------------------------------------------------
// Purpose: Starts the effect
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_SmokeStack::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	pParticleMgr->AddEffect( &m_ParticleEffect, this );
	
	// Figure out the material name.
	char str[512] = "unset_material";
	const model_t *pModel = modelinfo->GetModel( m_iMaterialModel );
	if ( pModel )
	{
		Q_strncpy( str, modelinfo->GetModelName( pModel ), sizeof( str ) );

		// Get rid of the extension because the material system doesn't want it.
		char *pExt = Q_stristr( str, ".vmt" );
		if ( pExt )
			pExt[0] = 0;
	}

	m_MaterialHandle[0] = m_ParticleEffect.FindOrAddMaterial( str );

#ifdef HL2_EPISODIC
	int iCount = 1;
	char szNames[512];

	int iLength = Q_strlen( str );
	str[iLength-1] = '\0';

	Q_snprintf( szNames, sizeof( szNames ), "%s%d.vmt", str, iCount );

	while ( filesystem->FileExists( VarArgs( "materials/%s", szNames ) ) && iCount < SMOKESTACK_MAX_MATERIALS )
	{
		char *pExt = Q_stristr( szNames, ".vmt" );
		if ( pExt )
			pExt[0] = 0;

		m_MaterialHandle[iCount] = m_ParticleEffect.FindOrAddMaterial( szNames );
		iCount++;
	}

	m_iMaxFrames = iCount-1;

	m_ParticleSpawn.Init( mat_reduceparticles.GetBool() ? m_Rate / 4 : m_Rate ); // Obey mat_reduceparticles in episodic
#else
	m_ParticleSpawn.Init( m_Rate );
#endif

	m_InvLifetime = m_Speed / m_JetLength;

	m_pParticleMgr = pParticleMgr;

	// Figure out how we need to draw.
	IMaterial *pMaterial = pParticleMgr->PMaterialToIMaterial( m_MaterialHandle[0] );
	if( pMaterial )
	{
		m_Renderer.Init( pParticleMgr, pMaterial );
	}
	
	QueueLightParametersInRenderer();

	// For the first N seconds, always simulate so it can build up the smokestack.
	// Afterwards, we set it to freeze when it's not being rendered.
	m_ParticleEffect.SetAlwaysSimulate( true );
	SetNextClientThink( gpGlobals->curtime + 5 );
}


void C_SmokeStack::ClientThink()
{
	m_ParticleEffect.SetAlwaysSimulate( false );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : **ppTable - 
//			**ppObj - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SmokeStack::GetPropEditInfo( RecvTable **ppTable, void **ppObj )
{
	*ppTable = &REFERENCE_RECV_TABLE(DT_SmokeStack);
	*ppObj = this;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_SmokeStack::Update(float fTimeDelta)
{
	if( !m_pParticleMgr )
	{
		assert(false);
		return;
	}

	// Don't spawn particles unless we're visible.
	if( m_bEmit && (m_ParticleEffect.WasDrawnPrevFrame() || m_ParticleEffect.GetAlwaysSimulate()) )
	{
		// Add new particles.																	
		Vector forward, right, up;
		AngleVectors(GetAbsAngles(), &forward, &right, &up);			

		float tempDelta = fTimeDelta;
		while(m_ParticleSpawn.NextEvent(tempDelta))
		{
			int iRandomFrame = random->RandomInt( 0, m_iMaxFrames );

#ifndef HL2_EPISODIC
			iRandomFrame = 0;
#endif
	
			// Make a new particle.
			if(SmokeStackParticle *pParticle = (SmokeStackParticle*)m_ParticleEffect.AddParticle(sizeof(SmokeStackParticle), m_MaterialHandle[iRandomFrame]))
			{
				float angle = FRand( 0, 2.0f*M_PI_F );
				
				pParticle->m_Pos = GetAbsOrigin() +
					right * (cos( angle ) * m_flBaseSpread) +
					forward * (sin( angle ) * m_flBaseSpread);

				pParticle->m_Velocity = 
					FRand(-m_SpreadSpeed,m_SpreadSpeed) * right +
					FRand(-m_SpreadSpeed,m_SpreadSpeed) * forward +
					m_Speed * up;

				pParticle->m_vAccel = m_vWind;
				pParticle->m_Lifetime = 0;
				pParticle->m_flAngle = 0.0f;

#ifdef HL2_EPISODIC
				pParticle->m_flAngle = RandomFloat( 0, 360 );
#endif
				pParticle->m_flRollDelta = random->RandomFloat( -m_flRollSpeed, m_flRollSpeed );
				pParticle->m_flSortPos = pParticle->m_Pos.z;
			}
		}
	}

	// Setup the twist matrix.
	float flTwist = (m_flTwist * (M_PI_F * 2.f) / 360.0f) * Helper_GetFrameTime();
	if( ( m_bTwist = !!flTwist ) )
	{
		m_TwistMat[0][0] =  cos(flTwist);
		m_TwistMat[0][1] =  sin(flTwist);
		m_TwistMat[1][0] = -sin(flTwist);
		m_TwistMat[1][1] =  cos(flTwist);
	}

	QueueLightParametersInRenderer();
}


void C_SmokeStack::StartRender( VMatrix &effectMatrix )
{
	m_Renderer.StartRender( effectMatrix );
}


void C_SmokeStack::QueueLightParametersInRenderer()
{
	m_Renderer.SetBaseColor( Vector( m_clrRender->r / 255.0f, m_clrRender->g / 255.0f, m_clrRender->b / 255.0f ) );
	m_Renderer.SetAmbientLight( m_AmbientLight );
	m_Renderer.SetDirectionalLight( m_DirLight );
	m_flAlphaScale = (float)m_clrRender->a;
} 


void C_SmokeStack::RenderParticles( CParticleRenderIterator *pIterator )
{
	const SmokeStackParticle *pParticle = (const SmokeStackParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Transform.						   
		Vector tPos;
		TransformParticle( m_pParticleMgr->GetModelView(), pParticle->m_Pos, tPos );

		// Figure out its alpha. Squaring it after it gets halfway through its lifetime
		// makes it get translucent and fade out for a longer time.
		//float alpha = cosf( -M_PI_F + tLifetime * M_PI_F * 2.f ) * 0.5f + 0.5f;
		float tLifetime = pParticle->m_Lifetime * m_InvLifetime;
		float alpha = TableCos( -M_PI_F + tLifetime * M_PI_F * 2.f ) * 0.5f + 0.5f;
		if( tLifetime > 0.5f )
			alpha *= alpha;

		m_Renderer.RenderParticle(
			pIterator->GetParticleDraw(),
			pParticle->m_Pos,
			tPos,
			alpha * m_flAlphaScale,
			FLerp(m_StartSize, m_EndSize, tLifetime),
			DEG2RAD( pParticle->m_flAngle )
		);
		
		pParticle = (const SmokeStackParticle*)pIterator->GetNext( pParticle->m_flSortPos );
	}
}


void C_SmokeStack::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	bool bSortNow = true; // Change this to false if we see sorting issues.
	bool bQuickTest = false;

	bool bDrawn = m_ParticleEffect.WasDrawnPrevFrame();

	if ( bDrawn == true && m_bInView == false )
	{
		bSortNow = true;
	}

	if ( bDrawn == false && m_bInView == true )
	{
		bQuickTest = true;
	}

#ifndef HL2_EPISODIC
	bQuickTest = false;
	bSortNow = true;
#endif

	if( bQuickTest == false && m_bEmit && (!m_ParticleEffect.WasDrawnPrevFrame() && !m_ParticleEffect.GetAlwaysSimulate()) )
		return;

	SmokeStackParticle *pParticle = (SmokeStackParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Should this particle die?
		pParticle->m_Lifetime += pIterator->GetTimeDelta();

		float tLifetime = pParticle->m_Lifetime * m_InvLifetime;
		if( tLifetime >= 1 )
		{
			pIterator->RemoveParticle( pParticle );
		}
		else
		{
			// Transform.						   
			Vector tPos;
			if( m_bTwist )
			{
				Vector vTwist(
					pParticle->m_Pos.x - GetAbsOrigin().x,
					pParticle->m_Pos.y - GetAbsOrigin().y,
					0);

				pParticle->m_Pos.x = vTwist.x * m_TwistMat[0][0] + vTwist.y * m_TwistMat[0][1] + GetAbsOrigin().x;
				pParticle->m_Pos.y = vTwist.x * m_TwistMat[1][0] + vTwist.y * m_TwistMat[1][1] + GetAbsOrigin().y;
			}

#ifndef HL2_EPISODIC
			pParticle->m_Pos = pParticle->m_Pos + 
				pParticle->m_Velocity * pIterator->GetTimeDelta() + 
				pParticle->m_vAccel * (0.5f * pIterator->GetTimeDelta() * pIterator->GetTimeDelta());

			pParticle->m_Velocity += pParticle->m_vAccel * pIterator->GetTimeDelta();
#else
			pParticle->m_Pos = pParticle->m_Pos + pParticle->m_Velocity * pIterator->GetTimeDelta() + pParticle->m_vAccel * pIterator->GetTimeDelta();
#endif

			pParticle->m_flAngle += pParticle->m_flRollDelta * pIterator->GetTimeDelta();

			if ( bSortNow == true )
			{
				Vector tPos;
				TransformParticle( m_pParticleMgr->GetModelView(), pParticle->m_Pos, tPos );
				pParticle->m_flSortPos = tPos.z;
			}
		}

		pParticle = (SmokeStackParticle*)pIterator->GetNext();
	}

	m_bInView = bDrawn;
}


