//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include "cbase.h"
#include "c_smoke_trail.h"
#include "fx.h"
#include "engine/ivdebugoverlay.h"
#include "engine/IEngineSound.h"
#include "c_te_effect_dispatch.h"
#include "glow_overlay.h"
#include "fx_explosion.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "view.h"
#include "clienteffectprecachesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//
// CRocketTrailParticle
//

class CRocketTrailParticle : public CSimpleEmitter
{
public:
	
	CRocketTrailParticle( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	//Create
	static CRocketTrailParticle *Create( const char *pDebugName )
	{
		return new CRocketTrailParticle( pDebugName );
	}

	//Roll
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -8.0f );

		//Cap the minimum roll
		if ( fabs( pParticle->m_flRollDelta ) < 0.5f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.5f : -0.5f;
		}

		return pParticle->m_flRoll;
	}

	//Alpha
	virtual float UpdateAlpha( const SimpleParticle *pParticle )
	{
		return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
	}

private:
	CRocketTrailParticle( const CRocketTrailParticle & );
};

//
// CSmokeParticle
//

class CSmokeParticle : public CSimpleEmitter
{
public:
	
	CSmokeParticle( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	//Create
	static CSmokeParticle *Create( const char *pDebugName )
	{
		return new CSmokeParticle( pDebugName );
	}

	//Alpha
	virtual float UpdateAlpha( const SimpleParticle *pParticle )
	{
		return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
	}

	//Color
	virtual Vector UpdateColor( const SimpleParticle *pParticle )
	{
		Vector	color;

		float	tLifetime = pParticle->m_flLifetime / pParticle->m_flDieTime;
		float	ramp = 1.0f - tLifetime;

		color[0] = ( (float) pParticle->m_uchColor[0] * ramp ) / 255.0f;
		color[1] = ( (float) pParticle->m_uchColor[1] * ramp ) / 255.0f;
		color[2] = ( (float) pParticle->m_uchColor[2] * ramp ) / 255.0f;

		return color;
	}

	//Roll
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -8.0f );

		//Cap the minimum roll
		if ( fabs( pParticle->m_flRollDelta ) < 0.5f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.5f : -0.5f;
		}

		return pParticle->m_flRoll;
	}

private:
	CSmokeParticle( const CSmokeParticle & );
};

// Datatable.. this can have all the smoketrail parameters when we need it to.
IMPLEMENT_CLIENTCLASS_DT(C_SmokeTrail, DT_SmokeTrail, SmokeTrail)
	RecvPropFloat(RECVINFO(m_SpawnRate)),
	RecvPropVector(RECVINFO(m_StartColor)),
	RecvPropVector(RECVINFO(m_EndColor)),
	RecvPropFloat(RECVINFO(m_ParticleLifetime)),
	RecvPropFloat(RECVINFO(m_StopEmitTime)),
	RecvPropFloat(RECVINFO(m_MinSpeed)),
	RecvPropFloat(RECVINFO(m_MaxSpeed)),
	RecvPropFloat(RECVINFO(m_MinDirectedSpeed)),
	RecvPropFloat(RECVINFO(m_MaxDirectedSpeed)),
	RecvPropFloat(RECVINFO(m_StartSize)),
	RecvPropFloat(RECVINFO(m_EndSize)),
	RecvPropFloat(RECVINFO(m_SpawnRadius)),
	RecvPropInt(RECVINFO(m_bEmit)),
	RecvPropInt(RECVINFO(m_nAttachment)),	
	RecvPropFloat(RECVINFO(m_Opacity)),
END_RECV_TABLE()

// ------------------------------------------------------------------------- //
// ParticleMovieExplosion
// ------------------------------------------------------------------------- //
C_SmokeTrail::C_SmokeTrail()
{
	m_MaterialHandle[0] = NULL;
	m_MaterialHandle[1] = NULL;

	m_SpawnRate = 10;
	m_ParticleSpawn.Init(10);
	m_StartColor.Init(0.5, 0.5, 0.5);
	m_EndColor.Init(0,0,0);
	m_ParticleLifetime = 5;
	m_StopEmitTime = 0;	// No end time
	m_MinSpeed = 2;
	m_MaxSpeed = 4;
	m_MinDirectedSpeed = m_MaxDirectedSpeed = 0;
	m_StartSize = 35;
	m_EndSize = 55;
	m_SpawnRadius = 2;
	m_VelocityOffset.Init();
	m_Opacity = 0.5f;

	m_bEmit = true;

	m_nAttachment	= -1;

	m_pSmokeEmitter = NULL;
	m_pParticleMgr	= NULL;
}

C_SmokeTrail::~C_SmokeTrail()
{
	if ( ToolsEnabled() && clienttools->IsInRecordingMode() && m_pSmokeEmitter.IsValid() && m_pSmokeEmitter->GetToolParticleEffectId() != TOOLPARTICLESYSTEMID_INVALID )
	{
		KeyValues *msg = new KeyValues( "OldParticleSystem_ActivateEmitter" );
		msg->SetInt( "id", m_pSmokeEmitter->GetToolParticleEffectId() );
		msg->SetInt( "emitter", 0 );
		msg->SetInt( "active", false );
		msg->SetFloat( "time", gpGlobals->curtime );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}

	if ( m_pParticleMgr )
	{
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SmokeTrail::GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	C_BaseEntity *pEnt = pAttachedTo->GetBaseEntity();
	if (pEnt && (m_nAttachment > 0))
	{
		pEnt->GetAttachment( m_nAttachment, *pAbsOrigin, *pAbsAngles );
	}
	else
	{
		BaseClass::GetAimEntOrigin( pAttachedTo, pAbsOrigin, pAbsAngles );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEmit - 
//-----------------------------------------------------------------------------
void C_SmokeTrail::SetEmit(bool bEmit)
{
	m_bEmit = bEmit;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rate - 
//-----------------------------------------------------------------------------
void C_SmokeTrail::SetSpawnRate(float rate)
{
	m_SpawnRate = rate;
	m_ParticleSpawn.Init(rate);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_SmokeTrail::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start( ParticleMgr(), NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_SmokeTrail::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	if(!pParticleMgr->AddEffect( &m_ParticleEffect, this ))
		return;

	m_pParticleMgr	= pParticleMgr;
	m_pSmokeEmitter = CSmokeParticle::Create("smokeTrail");
	
	if ( !m_pSmokeEmitter )
	{
		Assert( false );
		return;
	}

	m_pSmokeEmitter->SetSortOrigin( GetAbsOrigin() );
	m_pSmokeEmitter->SetNearClip( 64.0f, 128.0f );

	m_MaterialHandle[0] = g_Mat_DustPuff[0];
	m_MaterialHandle[1] = g_Mat_DustPuff[1];
	
	m_ParticleSpawn.Init( m_SpawnRate );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_SmokeTrail::Update( float fTimeDelta )
{
	if ( !m_pSmokeEmitter )
		return;

	Vector	offsetColor;

	// Add new particles
	if ( !m_bEmit )
		return;

	if ( ( m_StopEmitTime != 0 ) && ( m_StopEmitTime <= gpGlobals->curtime ) )
		return;

	float tempDelta = fTimeDelta;

	SimpleParticle	*pParticle;
	Vector			offset;

	Vector vecOrigin;
	VectorMA( GetAbsOrigin(), -fTimeDelta, GetAbsVelocity(), vecOrigin );

	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );

	while( m_ParticleSpawn.NextEvent( tempDelta ) )
	{
		float fldt = fTimeDelta - tempDelta;

		offset.Random( -m_SpawnRadius, m_SpawnRadius );
		offset += vecOrigin;
		VectorMA( offset, fldt, GetAbsVelocity(), offset );

		pParticle = (SimpleParticle *) m_pSmokeEmitter->AddParticle( sizeof( SimpleParticle ), m_MaterialHandle[random->RandomInt(0,1)], offset );

		if ( pParticle == NULL )
			continue;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= m_ParticleLifetime;

		pParticle->m_vecVelocity.Random( -1.0f, 1.0f );
		pParticle->m_vecVelocity *= random->RandomFloat( m_MinSpeed, m_MaxSpeed );

		pParticle->m_vecVelocity = pParticle->m_vecVelocity + GetAbsVelocity();
		
		float flDirectedVel = random->RandomFloat( m_MinDirectedSpeed, m_MaxDirectedSpeed );
		VectorMA( pParticle->m_vecVelocity, flDirectedVel, vecForward, pParticle->m_vecVelocity );

		offsetColor = m_StartColor;
		float flMaxVal = MAX( m_StartColor[0], m_StartColor[1] );
		if ( flMaxVal < m_StartColor[2] )
		{
			flMaxVal = m_StartColor[2];
		}
		offsetColor /= flMaxVal;

		offsetColor *= random->RandomFloat( -0.2f, 0.2f );
		offsetColor += m_StartColor;

		offsetColor[0] = clamp( offsetColor[0], 0.0f, 1.0f );
		offsetColor[1] = clamp( offsetColor[1], 0.0f, 1.0f );
		offsetColor[2] = clamp( offsetColor[2], 0.0f, 1.0f );

		pParticle->m_uchColor[0]	= offsetColor[0]*255.0f;
		pParticle->m_uchColor[1]	= offsetColor[1]*255.0f;
		pParticle->m_uchColor[2]	= offsetColor[2]*255.0f;
		
		pParticle->m_uchStartSize	= m_StartSize;
		pParticle->m_uchEndSize		= m_EndSize;
		
		float alpha = random->RandomFloat( m_Opacity*0.75f, m_Opacity*1.25f );
		alpha = clamp( alpha, 0.0f, 1.0f );

		pParticle->m_uchStartAlpha	= alpha * 255; 
		pParticle->m_uchEndAlpha	= 0;
			
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -1.0f, 1.0f );
    }
}


void C_SmokeTrail::RenderParticles( CParticleRenderIterator *pIterator )
{
}


void C_SmokeTrail::SimulateParticles( CParticleSimulateIterator *pIterator )
{
}


//-----------------------------------------------------------------------------
// This is called after sending this entity's recording state
//-----------------------------------------------------------------------------

void C_SmokeTrail::CleanupToolRecordingState( KeyValues *msg )
{
	if ( !ToolsEnabled() )
		return;

	BaseClass::CleanupToolRecordingState( msg );

	// Generally, this is used to allow the entity to clean up
	// allocated state it put into the message, but here we're going
	// to use it to send particle system messages because we
	// know the grenade has been recorded at this point
	if ( !clienttools->IsInRecordingMode() || !m_pSmokeEmitter.IsValid() )
		return;
	
	// For now, we can't record smoketrails that don't have a moveparent
	C_BaseEntity *pEnt = GetMoveParent();
	if ( !pEnt )
		return;

	bool bEmitterActive = m_bEmit && ( ( m_StopEmitTime == 0 ) || ( m_StopEmitTime > gpGlobals->curtime ) );

	// NOTE: Particle system destruction message will be sent by the particle effect itself.
	if ( m_pSmokeEmitter->GetToolParticleEffectId() == TOOLPARTICLESYSTEMID_INVALID )
	{
		int nId = m_pSmokeEmitter->AllocateToolParticleEffectId();

		KeyValues *oldmsg = new KeyValues( "OldParticleSystem_Create" );
		oldmsg->SetString( "name", "C_SmokeTrail" );
		oldmsg->SetInt( "id", nId );
		oldmsg->SetFloat( "time", gpGlobals->curtime );

		KeyValues *pRandomEmitter = oldmsg->FindKey( "DmeRandomEmitter", true );
		pRandomEmitter->SetInt( "count", m_SpawnRate );	// particles per second, when duration is < 0
		pRandomEmitter->SetFloat( "duration", -1 );
		pRandomEmitter->SetInt( "active", bEmitterActive );

		KeyValues *pEmitterParent1 = pRandomEmitter->FindKey( "emitter1", true );
		pEmitterParent1->SetFloat( "randomamount", 0.5f );
		KeyValues *pEmitterParent2 = pRandomEmitter->FindKey( "emitter2", true );
		pEmitterParent2->SetFloat( "randomamount", 0.5f );

		KeyValues *pEmitter = pEmitterParent1->FindKey( "DmeSpriteEmitter", true );
		pEmitter->SetString( "material", "particle/particle_smokegrenade" );

		KeyValues *pInitializers = pEmitter->FindKey( "initializers", true );

		// FIXME: Until we can interpolate ent logs during emission, this can't work
		KeyValues *pPosition = pInitializers->FindKey( "DmePositionPointToEntityInitializer", true );
		pPosition->SetPtr( "entindex", (void*)(intp)pEnt->entindex() );
		pPosition->SetInt( "attachmentIndex", m_nAttachment );
		pPosition->SetFloat( "randomDist", m_SpawnRadius );
		pPosition->SetFloat( "startx", pEnt->GetAbsOrigin().x );
		pPosition->SetFloat( "starty", pEnt->GetAbsOrigin().y );
		pPosition->SetFloat( "startz", pEnt->GetAbsOrigin().z );

		KeyValues *pLifetime = pInitializers->FindKey( "DmeRandomLifetimeInitializer", true );
		pLifetime->SetFloat( "minLifetime", m_ParticleLifetime );
 		pLifetime->SetFloat( "maxLifetime", m_ParticleLifetime );

		KeyValues *pVelocity = pInitializers->FindKey( "DmeAttachmentVelocityInitializer", true );
		pVelocity->SetPtr( "entindex", (void*)(intp)entindex() );
		pVelocity->SetFloat( "minAttachmentSpeed", m_MinDirectedSpeed );
 		pVelocity->SetFloat( "maxAttachmentSpeed", m_MaxDirectedSpeed );
 		pVelocity->SetFloat( "minRandomSpeed", m_MinSpeed );
 		pVelocity->SetFloat( "maxRandomSpeed", m_MaxSpeed );

		KeyValues *pRoll = pInitializers->FindKey( "DmeRandomRollInitializer", true );
		pRoll->SetFloat( "minRoll", 0.0f );
 		pRoll->SetFloat( "maxRoll", 360.0f );

		KeyValues *pRollSpeed = pInitializers->FindKey( "DmeRandomRollSpeedInitializer", true );
		pRollSpeed->SetFloat( "minRollSpeed", -1.0f );
 		pRollSpeed->SetFloat( "maxRollSpeed", 1.0f );

		KeyValues *pColor = pInitializers->FindKey( "DmeRandomValueColorInitializer", true );
		Color c(
			FastFToC( clamp( m_StartColor.x, 0.f, 1.f ) ),
			FastFToC( clamp( m_StartColor.y, 0.f, 1.f ) ),
			FastFToC( clamp( m_StartColor.z, 0.f, 1.f ) ),
			255 );
		pColor->SetColor( "startColor", c );
		pColor->SetFloat( "minStartValueDelta", -0.2f );
 		pColor->SetFloat( "maxStartValueDelta", 0.2f );
		pColor->SetColor( "endColor", Color( 0, 0, 0, 255 ) );

		KeyValues *pAlpha = pInitializers->FindKey( "DmeRandomAlphaInitializer", true );
		int nMinAlpha = 255 * m_Opacity * 0.75f;
		int nMaxAlpha = 255 * m_Opacity * 1.25f;
		pAlpha->SetInt( "minStartAlpha", 0 );
		pAlpha->SetInt( "maxStartAlpha", 0 );
		pAlpha->SetInt( "minEndAlpha", clamp( nMinAlpha, 0, 255 ) );
		pAlpha->SetInt( "maxEndAlpha", clamp( nMaxAlpha, 0, 255 ) );

		KeyValues *pSize = pInitializers->FindKey( "DmeRandomSizeInitializer", true );
		pSize->SetFloat( "minStartSize", m_StartSize );
		pSize->SetFloat( "maxStartSize", m_StartSize );
		pSize->SetFloat( "minEndSize", m_EndSize );
		pSize->SetFloat( "maxEndSize", m_EndSize );

		KeyValues *pUpdaters = pEmitter->FindKey( "updaters", true );
	    
		pUpdaters->FindKey( "DmePositionVelocityUpdater", true );
		pUpdaters->FindKey( "DmeRollUpdater", true );

		KeyValues *pRollSpeedUpdater = pUpdaters->FindKey( "DmeRollSpeedAttenuateUpdater", true );
		pRollSpeedUpdater->SetFloat( "attenuation", 1.0f - 8.0f / 30.0f );
		pRollSpeedUpdater->SetFloat( "attenuationTme", 1.0f / 30.0f );
		pRollSpeedUpdater->SetFloat( "minRollSpeed", 0.5f );

		pUpdaters->FindKey( "DmeAlphaSineUpdater", true );
		pUpdaters->FindKey( "DmeColorUpdater", true );
		pUpdaters->FindKey( "DmeSizeUpdater", true );

		KeyValues *pEmitter2 = pEmitter->MakeCopy();
		pEmitter2->SetString( "material", "particle/particle_noisesphere" );
		pEmitterParent2->AddSubKey( pEmitter2 );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, oldmsg );
		oldmsg->deleteThis();
	}
	else 
	{
		KeyValues *oldmsg = new KeyValues( "OldParticleSystem_ActivateEmitter" );
		oldmsg->SetInt( "id", m_pSmokeEmitter->GetToolParticleEffectId() );
		oldmsg->SetInt( "emitter", 0 );
		oldmsg->SetInt( "active", bEmitterActive );
		oldmsg->SetFloat( "time", gpGlobals->curtime );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, oldmsg );
		oldmsg->deleteThis();
	}
}


//==================================================
// RocketTrail
//==================================================

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(RocketTrail, C_RocketTrail);

// Datatable.. this can have all the smoketrail parameters when we need it to.
IMPLEMENT_CLIENTCLASS_DT(C_RocketTrail, DT_RocketTrail, RocketTrail)
	RecvPropFloat(RECVINFO(m_SpawnRate)),
	RecvPropVector(RECVINFO(m_StartColor)),
	RecvPropVector(RECVINFO(m_EndColor)),
	RecvPropFloat(RECVINFO(m_ParticleLifetime)),
	RecvPropFloat(RECVINFO(m_StopEmitTime)),
	RecvPropFloat(RECVINFO(m_MinSpeed)),
	RecvPropFloat(RECVINFO(m_MaxSpeed)),
	RecvPropFloat(RECVINFO(m_StartSize)),
	RecvPropFloat(RECVINFO(m_EndSize)),
	RecvPropFloat(RECVINFO(m_SpawnRadius)),
	RecvPropInt(RECVINFO(m_bEmit)),
	RecvPropInt(RECVINFO(m_nAttachment)),	
	RecvPropFloat(RECVINFO(m_Opacity)),
	RecvPropInt(RECVINFO(m_bDamaged)),
	RecvPropFloat(RECVINFO(m_flFlareScale)),
END_RECV_TABLE()

// ------------------------------------------------------------------------- //
// ParticleMovieExplosion
// ------------------------------------------------------------------------- //
C_RocketTrail::C_RocketTrail()
{
	m_MaterialHandle[0] = NULL;
	m_MaterialHandle[1] = NULL;
	
	m_SpawnRate = 10;
	m_ParticleSpawn.Init(10);
	m_StartColor.Init(0.5, 0.5, 0.5);
	m_EndColor.Init(0,0,0);
	m_ParticleLifetime = 5;
	m_StopEmitTime = 0;	// No end time
	m_MinSpeed = 2;
	m_MaxSpeed = 4;
	m_StartSize = 35;
	m_EndSize = 55;
	m_SpawnRadius = 2;
	m_VelocityOffset.Init();
	m_Opacity = 0.5f;

	m_bEmit		= true;
	m_bDamaged	= false;

	m_nAttachment	= -1;

	m_pRocketEmitter = NULL;
	m_pParticleMgr	= NULL;
}

C_RocketTrail::~C_RocketTrail()
{
	if ( m_pParticleMgr )
	{
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_RocketTrail::GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	C_BaseEntity *pEnt = pAttachedTo->GetBaseEntity();
	if (pEnt && (m_nAttachment > 0))
	{
		pEnt->GetAttachment( m_nAttachment, *pAbsOrigin, *pAbsAngles );
	}
	else
	{
		BaseClass::GetAimEntOrigin( pAttachedTo, pAbsOrigin, pAbsAngles );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEmit - 
//-----------------------------------------------------------------------------
void C_RocketTrail::SetEmit(bool bEmit)
{
	m_bEmit = bEmit;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rate - 
//-----------------------------------------------------------------------------
void C_RocketTrail::SetSpawnRate(float rate)
{
	m_SpawnRate = rate;
	m_ParticleSpawn.Init(rate);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_RocketTrail::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start( ParticleMgr(), NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_RocketTrail::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	if(!pParticleMgr->AddEffect( &m_ParticleEffect, this ))
		return;

	m_pParticleMgr	= pParticleMgr;
	m_pRocketEmitter = CRocketTrailParticle::Create("smokeTrail");
	if ( !m_pRocketEmitter )
	{
		Assert( false );
		return;
	}

	m_pRocketEmitter->SetSortOrigin( GetAbsOrigin() );
	m_pRocketEmitter->SetNearClip( 64.0f, 128.0f );

	m_MaterialHandle[0] = g_Mat_DustPuff[0];
	m_MaterialHandle[1] = g_Mat_DustPuff[1];
	
	m_ParticleSpawn.Init( m_SpawnRate );

	m_vecLastPosition = GetAbsOrigin();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_RocketTrail::Update( float fTimeDelta )
{
	if ( !m_pRocketEmitter )
		return;

	if ( gpGlobals->frametime == 0.0f )
		return;

	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "MuzzleFlash" );
	pSimple->SetSortOrigin( GetAbsOrigin() );
	
	SimpleParticle *pParticle;
	Vector			forward, offset;

	AngleVectors( GetAbsAngles(), &forward );
	
	forward.Negate();

	float flScale = random->RandomFloat( m_flFlareScale-0.5f, m_flFlareScale+0.5f );

	//
	// Flash
	//

	int i;

	for ( i = 1; i < 9; i++ )
	{
		offset = GetAbsOrigin() + (forward * (i*2.0f*m_flFlareScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/muzzleflash%d", random->RandomInt(1,4) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.01f;

		pParticle->m_vecVelocity.Init();

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 128;

		pParticle->m_uchStartSize	= (random->RandomFloat( 5.0f, 6.0f ) * (12-(i))/9) * flScale;
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	// Add new particles (undamaged version)
	if ( m_bEmit )
	{
		Vector	moveDiff	= GetAbsOrigin() - m_vecLastPosition;
		float	moveLength	= VectorNormalize( moveDiff );

		int	numPuffs = moveLength / ( m_StartSize / 2.0f );

		//debugoverlay->AddLineOverlay( m_vecLastPosition, GetAbsOrigin(), 255, 0, 0, true, 2.0f ); 
		
		//FIXME: More rational cap here, perhaps
		if ( numPuffs > 50 )
			numPuffs = 50;

		Vector			offsetColor;
		float			step = moveLength / numPuffs;

		//Fill in the gaps
		for ( i = 1; i < numPuffs+1; i++ )
		{
			offset = m_vecLastPosition + ( moveDiff * step * i );

			//debugoverlay->AddBoxOverlay( offset, -Vector(2,2,2), Vector(2,2,2), vec3_angle, i*4, i*4, i*4, true, 4.0f );
			
			pParticle = (SimpleParticle *) m_pRocketEmitter->AddParticle( sizeof( SimpleParticle ), m_MaterialHandle[random->RandomInt(0,1)], offset );

			if ( pParticle != NULL )
			{
				pParticle->m_flLifetime		= 0.0f;
				pParticle->m_flDieTime		= m_ParticleLifetime + random->RandomFloat(m_ParticleLifetime*0.9f,m_ParticleLifetime*1.1f);

				pParticle->m_vecVelocity.Random( -1.0f, 1.0f );
				pParticle->m_vecVelocity *= random->RandomFloat( m_MinSpeed, m_MaxSpeed );
				
				offsetColor = m_StartColor * random->RandomFloat( 0.75f, 1.25f );

				offsetColor[0] = clamp( offsetColor[0], 0.0f, 1.0f );
				offsetColor[1] = clamp( offsetColor[1], 0.0f, 1.0f );
				offsetColor[2] = clamp( offsetColor[2], 0.0f, 1.0f );

				pParticle->m_uchColor[0]	= offsetColor[0]*255.0f;
				pParticle->m_uchColor[1]	= offsetColor[1]*255.0f;
				pParticle->m_uchColor[2]	= offsetColor[2]*255.0f;
				
				pParticle->m_uchStartSize	= m_StartSize * random->RandomFloat( 0.75f, 1.25f );
				pParticle->m_uchEndSize		= m_EndSize * random->RandomFloat( 1.0f, 1.25f );
				
				float alpha = random->RandomFloat( m_Opacity*0.75f, m_Opacity*1.25f );

				if ( alpha > 1.0f )
					alpha = 1.0f;
				if ( alpha < 0.0f )
					alpha = 0.0f;

				pParticle->m_uchStartAlpha	= alpha * 255; 
				pParticle->m_uchEndAlpha	= 0;
				
				pParticle->m_flRoll			= random->RandomInt( 0, 360 );
				pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
			}
		}
	}
	
	if ( m_bDamaged )
	{
		Vector			offsetColor;

		CSmartPtr<CEmberEffect>	pEmitter = CEmberEffect::Create("C_RocketTrail::damaged");

		pEmitter->SetSortOrigin( GetAbsOrigin() );

		PMaterialHandle flameMaterial = m_pRocketEmitter->GetPMaterial( VarArgs( "sprites/flamelet%d", random->RandomInt( 1, 4 ) ) );
		
		// Flames from the rocket
		for ( i = 0; i < 8; i++ )
		{
			offset = RandomVector( -8, 8 ) + GetAbsOrigin();

			pParticle = (SimpleParticle *) pEmitter->AddParticle( sizeof( SimpleParticle ), flameMaterial, offset );

			if ( pParticle != NULL )
			{
				pParticle->m_flLifetime		= 0.0f;
				pParticle->m_flDieTime		= 0.25f;

				pParticle->m_vecVelocity.Random( -1.0f, 1.0f );
				pParticle->m_vecVelocity *= random->RandomFloat( 32, 128 );
				
				offsetColor = m_StartColor * random->RandomFloat( 0.75f, 1.25f );

				offsetColor[0] = clamp( offsetColor[0], 0.0f, 1.0f );
				offsetColor[1] = clamp( offsetColor[1], 0.0f, 1.0f );
				offsetColor[2] = clamp( offsetColor[2], 0.0f, 1.0f );

				pParticle->m_uchColor[0]	= offsetColor[0]*255.0f;
				pParticle->m_uchColor[1]	= offsetColor[1]*255.0f;
				pParticle->m_uchColor[2]	= offsetColor[2]*255.0f;
				
				pParticle->m_uchStartSize	= 8.0f;
				pParticle->m_uchEndSize		= 32.0f;
				
				pParticle->m_uchStartAlpha	= 255;
				pParticle->m_uchEndAlpha	= 0;
				
				pParticle->m_flRoll			= random->RandomInt( 0, 360 );
				pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
			}
		}
	}

	m_vecLastPosition = GetAbsOrigin();
}

void C_RocketTrail::RenderParticles( CParticleRenderIterator *pIterator )
{
}

void C_RocketTrail::SimulateParticles( CParticleSimulateIterator *pIterator )
{
}

SporeEffect::SporeEffect( const char *pDebugName ) : CSimpleEmitter( pDebugName )
{
}


SporeEffect* SporeEffect::Create( const char *pDebugName )
{
	return new SporeEffect( pDebugName );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
void SporeEffect::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
	float	speed = VectorNormalize( pParticle->m_vecVelocity );
	Vector	offset;

	speed -= ( 64.0f * timeDelta );

	offset.Random( -0.5f, 0.5f );

	pParticle->m_vecVelocity += offset;
	VectorNormalize( pParticle->m_vecVelocity );

	pParticle->m_vecVelocity *= speed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
Vector SporeEffect::UpdateColor( const SimpleParticle *pParticle )
{
	Vector	color;
	float	ramp = ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) );//1.0f - ( pParticle->m_flLifetime / pParticle->m_flDieTime );

	color[0] = ( (float) pParticle->m_uchColor[0] * ramp ) / 255.0f;
	color[1] = ( (float) pParticle->m_uchColor[1] * ramp ) / 255.0f;
	color[2] = ( (float) pParticle->m_uchColor[2] * ramp ) / 255.0f;

	return color;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float SporeEffect::UpdateAlpha( const SimpleParticle *pParticle )
{
	return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
}

//==================================================
// C_SporeExplosion
//==================================================

EXPOSE_PROTOTYPE_EFFECT( SporeExplosion, C_SporeExplosion );

IMPLEMENT_CLIENTCLASS_DT( C_SporeExplosion, DT_SporeExplosion, SporeExplosion )
	RecvPropFloat(RECVINFO(m_flSpawnRate)),
	RecvPropFloat(RECVINFO(m_flParticleLifetime)),
	RecvPropFloat(RECVINFO(m_flStartSize)),
	RecvPropFloat(RECVINFO(m_flEndSize)),
	RecvPropFloat(RECVINFO(m_flSpawnRadius)),
	RecvPropBool(RECVINFO(m_bEmit)),
	RecvPropBool(RECVINFO(m_bDontRemove)),
END_RECV_TABLE()

C_SporeExplosion::C_SporeExplosion( void )
{
	m_pParticleMgr			= NULL;

	m_flSpawnRate			= 32;
	m_flParticleLifetime	= 5;
	m_flStartSize			= 32;
	m_flEndSize				= 64;
	m_flSpawnRadius			= 32;
	m_pSporeEffect			= NULL;

	m_teParticleSpawn.Init( 32 );

	m_bEmit = true;
	m_bDontRemove = false;
}

C_SporeExplosion::~C_SporeExplosion()
{
	if ( m_pParticleMgr != NULL )
	{
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_SporeExplosion::OnDataChanged( DataUpdateType_t updateType )
{
	C_BaseEntity::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_flPreviousSpawnRate = m_flSpawnRate;
		m_teParticleSpawn.Init( m_flSpawnRate );
		Start( ParticleMgr(), NULL );
	}
	else if( m_bEmit )
	{
		// Just been turned on by the server.
		m_flPreviousSpawnRate = m_flSpawnRate;
		m_teParticleSpawn.Init( m_flSpawnRate );
	}

	m_pSporeEffect->SetDontRemove( m_bDontRemove );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_SporeExplosion::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	//Add us into the effect manager
	if( pParticleMgr->AddEffect( &m_ParticleEffect, this ) == false )
		return;

	//Create our main effect
	m_pSporeEffect = SporeEffect::Create( "C_SporeExplosion" );
	
	if ( m_pSporeEffect == NULL )
		return;

	m_hMaterial	= m_pSporeEffect->GetPMaterial( "particle/fire" );

	m_pSporeEffect->SetSortOrigin( GetAbsOrigin() );
	m_pSporeEffect->SetNearClip( 64, 128 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SporeExplosion::AddParticles( void )
{
	//Spores
	Vector	offset;
	Vector	dir;

	//Get our direction
	AngleVectors( GetAbsAngles(), &dir );

	SimpleParticle	*sParticle;

	for ( int i = 0; i < 4; i++ )
	{
		//Add small particle to the effect's origin
		offset.Random( -m_flSpawnRadius, m_flSpawnRadius );
		sParticle = (SimpleParticle *) m_pSporeEffect->AddParticle( sizeof(SimpleParticle), m_hMaterial, GetAbsOrigin()+offset );

		if ( sParticle == NULL )
			return;

		sParticle->m_flLifetime		= 0.0f;
		sParticle->m_flDieTime		= 2.0f;

		sParticle->m_flRoll			= 0;
		sParticle->m_flRollDelta	= 0;

		sParticle->m_uchColor[0]	= 225;
		sParticle->m_uchColor[1]	= 140;
		sParticle->m_uchColor[2]	= 64;
		sParticle->m_uchStartAlpha	= Helper_RandomInt( 128, 255 );
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchStartSize	= Helper_RandomInt( 1, 2 );
		sParticle->m_uchEndSize		= 1;

		sParticle->m_vecVelocity	= dir * Helper_RandomFloat( 128.0f, 256.0f );
	}

	//Add smokey bits
	offset.Random( -(m_flSpawnRadius * 0.5), (m_flSpawnRadius * 0.5) );
	sParticle = (SimpleParticle *) m_pSporeEffect->AddParticle( sizeof(SimpleParticle), g_Mat_DustPuff[1], GetAbsOrigin()+offset );

	if ( sParticle == NULL )
		return;

	sParticle->m_flLifetime		= 0.0f;
	sParticle->m_flDieTime		= 1.0f;

	sParticle->m_flRoll			= Helper_RandomFloat( 0, 360 );
	sParticle->m_flRollDelta	= Helper_RandomFloat( -2.0f, 2.0f );

	sParticle->m_uchColor[0]	= 225;
	sParticle->m_uchColor[1]	= 140;
	sParticle->m_uchColor[2]	= 64;
	sParticle->m_uchStartAlpha	= Helper_RandomInt( 32, 64 );
	sParticle->m_uchEndAlpha	= 0;
	sParticle->m_uchStartSize	= m_flStartSize;
	sParticle->m_uchEndSize		= m_flEndSize;

	sParticle->m_vecVelocity	= dir * Helper_RandomFloat( 64.0f, 128.0f );
}


ConVar cl_sporeclipdistance( "cl_sporeclipdistance", "512", FCVAR_CHEAT | FCVAR_CLIENTDLL );
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_SporeExplosion::Update( float fTimeDelta )
{
	if( m_bEmit )
	{
		float tempDelta = fTimeDelta;

		float flDist = (MainViewOrigin() - GetAbsOrigin()).Length();

		//Lower the spawnrate by half if we're far away from it.
		if ( cl_sporeclipdistance.GetFloat() <= flDist )
		{
			if ( m_flSpawnRate == m_flPreviousSpawnRate )
			{
				m_flPreviousSpawnRate = m_flSpawnRate * 0.5f;
				m_teParticleSpawn.ResetRate( m_flPreviousSpawnRate );
			}
		}
		else
		{
			if ( m_flSpawnRate != m_flPreviousSpawnRate )
			{
				m_flPreviousSpawnRate = m_flSpawnRate;
				m_teParticleSpawn.ResetRate( m_flPreviousSpawnRate );
			}
		}

		while ( m_teParticleSpawn.NextEvent( tempDelta ) )
		{
			AddParticles();
		}
	}
}


void C_SporeExplosion::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	StandardParticle_t *pParticle = (StandardParticle_t*)pIterator->GetFirst();
	while ( pParticle )
	{
		pParticle->m_Lifetime += pIterator->GetTimeDelta();
		
		if( pParticle->m_Lifetime > m_flParticleLifetime )
		{
			pIterator->RemoveParticle( pParticle );
		}

		pParticle = (StandardParticle_t*)pIterator->GetNext();
	}
}

void C_SporeExplosion::RenderParticles( CParticleRenderIterator *pIterator )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RPGShotDownCallback( const CEffectData &data )
{
	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "Missile.ShotDown", &data.m_vOrigin );

	if ( CExplosionOverlay *pOverlay = new CExplosionOverlay )
	{
		pOverlay->m_flLifetime	= 0;
		pOverlay->m_vPos		= data.m_vOrigin;
		pOverlay->m_nSprites	= 1;
		
		pOverlay->m_vBaseColors[0].Init( 1.0f, 0.9f, 0.7f );

		pOverlay->m_Sprites[0].m_flHorzSize = 0.01f;
		pOverlay->m_Sprites[0].m_flVertSize = pOverlay->m_Sprites[0].m_flHorzSize*0.5f;

		pOverlay->Activate();
	}
}

DECLARE_CLIENT_EFFECT( "RPGShotDown", RPGShotDownCallback );



//==================================================
// C_SporeTrail
//==================================================

class C_SporeTrail : public C_BaseParticleEntity
{
public:
	DECLARE_CLASS( C_SporeTrail, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();
	
	C_SporeTrail( void );
	virtual	~C_SporeTrail( void );

public:
	void	SetEmit( bool bEmit );


// C_BaseEntity
public:
	virtual	void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles );

// IPrototypeAppEffect
public:
	virtual void	Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs );

// IParticleEffect
public:
	virtual void	Update( float fTimeDelta );
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );
	virtual void	StartRender( VMatrix &effectMatrix );

public:
	Vector	m_vecEndColor;

	float	m_flSpawnRate;
	float	m_flParticleLifetime;
	float	m_flStartSize;
	float	m_flEndSize;
	float	m_flSpawnRadius;

	Vector	m_vecVelocityOffset;

	bool	m_bEmit;

private:
	C_SporeTrail( const C_SporeTrail & );

	void			AddParticles( void );

	PMaterialHandle		m_hMaterial;
	TimedEvent			m_teParticleSpawn;
	//CSmartPtr<SporeSmokeEffect> m_pSmokeEffect;

	Vector			m_vecPos;
	Vector			m_vecLastPos;	// This is stored so we can spawn particles in between the previous and new position
									// to eliminate holes in the trail.

	VMatrix			m_mAttachmentMatrix;
	CParticleMgr		*m_pParticleMgr;
};



//==================================================
// C_SporeTrail
//==================================================

IMPLEMENT_CLIENTCLASS_DT( C_SporeTrail, DT_SporeTrail, SporeTrail )
	RecvPropFloat(RECVINFO(m_flSpawnRate)),
	RecvPropVector(RECVINFO(m_vecEndColor)),
	RecvPropFloat(RECVINFO(m_flParticleLifetime)),
	RecvPropFloat(RECVINFO(m_flStartSize)),
	RecvPropFloat(RECVINFO(m_flEndSize)),
	RecvPropFloat(RECVINFO(m_flSpawnRadius)),
	RecvPropInt(RECVINFO(m_bEmit)),
END_RECV_TABLE()

C_SporeTrail::C_SporeTrail( void )
{
	m_pParticleMgr			= NULL;
	//m_pSmokeEffect			= SporeSmokeEffect::Create( "C_SporeTrail" );

	m_flSpawnRate			= 10;
	m_flParticleLifetime	= 5;
	m_flStartSize			= 35;
	m_flEndSize				= 55;
	m_flSpawnRadius			= 2;

	m_teParticleSpawn.Init( 5 );
	m_vecEndColor.Init();
	m_vecPos.Init();
	m_vecLastPos.Init();
	m_vecVelocityOffset.Init();

	m_bEmit = true;
}

C_SporeTrail::~C_SporeTrail()
{
	if( m_pParticleMgr )
	{
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEmit - 
//-----------------------------------------------------------------------------
void C_SporeTrail::SetEmit( bool bEmit )
{
	m_bEmit = bEmit;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_SporeTrail::OnDataChanged( DataUpdateType_t updateType )
{
	C_BaseEntity::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start( ParticleMgr(), NULL );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_SporeTrail::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	if( pParticleMgr->AddEffect( &m_ParticleEffect, this ) == false )
		return;

	m_hMaterial	= g_Mat_DustPuff[1];
	m_pParticleMgr = pParticleMgr;
	m_teParticleSpawn.Init( 64 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SporeTrail::AddParticles( void )
{
	Vector	offset = RandomVector( -4.0f, 4.0f );

	//Make a new particle
	SimpleParticle *sParticle = (SimpleParticle *) m_ParticleEffect.AddParticle( sizeof(SimpleParticle), m_hMaterial );//m_pSmokeEffect->AddParticle( sizeof(SimpleParticle), m_hMaterial, GetAbsOrigin()+offset );

	if ( sParticle == NULL )
		return;

	sParticle->m_Pos			= offset;
	sParticle->m_flRoll			= Helper_RandomInt( 0, 360 );
	sParticle->m_flRollDelta	= Helper_RandomFloat( -2.0f, 2.0f );

	sParticle->m_flLifetime		= 0.0f;
	sParticle->m_flDieTime		= 0.5f;

	sParticle->m_uchColor[0]	= 225;
	sParticle->m_uchColor[1]	= 140;
	sParticle->m_uchColor[2]	= 64;
	sParticle->m_uchStartAlpha	= Helper_RandomInt( 64, 128 );
	sParticle->m_uchEndAlpha	= 0;

	sParticle->m_uchStartSize	= 1.0f;
	sParticle->m_uchEndSize		= 1.0f;
	
	sParticle->m_vecVelocity	= RandomVector( -8.0f, 8.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_SporeTrail::Update( float fTimeDelta )
{
	if ( m_pParticleMgr == NULL )
		return;

	//Add new particles
	if ( m_bEmit )
	{
		float tempDelta = fTimeDelta;
		
		while ( m_teParticleSpawn.NextEvent( tempDelta ) )
		{
			AddParticles();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &effectMatrix - 
//-----------------------------------------------------------------------------
void C_SporeTrail::StartRender( VMatrix &effectMatrix )
{
	effectMatrix = effectMatrix * m_mAttachmentMatrix;
}

void C_SporeTrail::RenderParticles( CParticleRenderIterator *pIterator )
{
	if ( m_bEmit == false )
		return;

	const SimpleParticle *pParticle = (const SimpleParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		//Render
		Vector	tPos;
		TransformParticle( m_pParticleMgr->GetModelView(), pParticle->m_Pos, tPos );
		float sortKey = tPos.z;

		Vector	color = Vector( 1.0f, 1.0f, 1.0f );

		//Render it
		RenderParticle_ColorSize(
			pIterator->GetParticleDraw(),
			tPos,
			color,
			1.0f,
			4 );

		pParticle = (const SimpleParticle*)pIterator->GetNext( sortKey );
	}
}


void C_SporeTrail::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	if ( m_bEmit == false )
		return;

	SimpleParticle *pParticle = (SimpleParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		//UpdateVelocity( pParticle, timeDelta );
		pParticle->m_Pos += pParticle->m_vecVelocity * pIterator->GetTimeDelta();

		//Should this particle die?
		pParticle->m_flLifetime += pIterator->GetTimeDelta();

		if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
		{
			pIterator->RemoveParticle( pParticle );
		}

		pParticle = (SimpleParticle*)pIterator->GetNext();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SporeTrail::GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	C_BaseEntity *pEnt = pAttachedTo->GetBaseEntity();
	
	pEnt->GetAttachment( 1, *pAbsOrigin, *pAbsAngles );

	matrix3x4_t	matrix;

	AngleMatrix( *pAbsAngles, *pAbsOrigin, matrix );

	m_mAttachmentMatrix = matrix;
}

//==================================================
// FireTrailhou
//==================================================

// Datatable.. this can have all the smoketrail parameters when we need it to.
IMPLEMENT_CLIENTCLASS_DT(C_FireTrail, DT_FireTrail, CFireTrail)
	RecvPropInt(RECVINFO(m_nAttachment)),	
	RecvPropFloat(RECVINFO(m_flLifetime)),
END_RECV_TABLE()

// ------------------------------------------------------------------------- //
// ParticleMovieExplosion
// ------------------------------------------------------------------------- //
C_FireTrail::C_FireTrail()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_FireTrail::~C_FireTrail( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_FireTrail::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	BaseClass::Start( pParticleMgr, pArgs );

	m_pTrailEmitter = CSimpleEmitter::Create( "FireTrail" );
	
	if ( !m_pTrailEmitter )
	{
		Assert( false );
		return;
	}

	m_pTrailEmitter->SetSortOrigin( GetAbsOrigin() );

	// Setup our materials
	m_hMaterial[FTRAIL_SMOKE1] = g_Mat_DustPuff[0];
	m_hMaterial[FTRAIL_SMOKE2] = g_Mat_DustPuff[1];
	
	m_hMaterial[FTRAIL_FLAME1] = m_pTrailEmitter->GetPMaterial( "sprites/flamelet1" );
	m_hMaterial[FTRAIL_FLAME2] = m_pTrailEmitter->GetPMaterial( "sprites/flamelet2" );
	m_hMaterial[FTRAIL_FLAME3] = m_pTrailEmitter->GetPMaterial( "sprites/flamelet3" );
	m_hMaterial[FTRAIL_FLAME4] = m_pTrailEmitter->GetPMaterial( "sprites/flamelet4" );
	m_hMaterial[FTRAIL_FLAME5] = m_pTrailEmitter->GetPMaterial( "sprites/flamelet5" );

	// Setup our smoke emitter
	m_pSmokeEmitter = CSmokeParticle::Create( "FireTrail_Smoke" );
	
	m_pSmokeEmitter->SetSortOrigin( GetAbsOrigin() );
	m_pSmokeEmitter->SetNearClip( 64.0f, 128.0f );

	if ( !m_pSmokeEmitter )
	{
		Assert( false );
		return;
	}

	// Seed our first position as the last known one
	m_vecLastPosition = GetAbsOrigin();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_FireTrail::Update( float fTimeDelta )
{
	if ( !m_pTrailEmitter )
		return;

	if ( ( m_flLifetime != 0 ) && ( m_flLifetime <= gpGlobals->curtime ) )
		return;

	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "FireTrail" );
	pSimple->SetSortOrigin( GetAbsOrigin() );
	
	Vector			offset;

#define	STARTSIZE			8
#define	ENDSIZE				16
#define	PARTICLE_LIFETIME	0.075f
#define	MIN_SPEED			32
#define	MAX_SPEED			64

	// Add new particles
	//if ( ShouldEmit() )
	{
		Vector	moveDiff	= GetAbsOrigin() - m_vecLastPosition;
		float	moveLength	= VectorNormalize( moveDiff );

		int	numPuffs = moveLength / ( STARTSIZE / 2.0f );

		//FIXME: More rational cap here, perhaps
		numPuffs = clamp( numPuffs, 1, 32 );

		SimpleParticle	*pParticle;
		Vector			offsetColor;
		float			step = moveLength / numPuffs;

		//Fill in the gaps
		for ( int i = 1; i < numPuffs+1; i++ )
		{
			offset = m_vecLastPosition + ( moveDiff * step * i ) + RandomVector( -4.0f, 4.0f );

			//debugoverlay->AddBoxOverlay( offset, -Vector(2,2,2), Vector(2,2,2), vec3_angle, i*4, i*4, i*4, true, 1.0f );
			
			pParticle = (SimpleParticle *) m_pSmokeEmitter->AddParticle( sizeof( SimpleParticle ), m_hMaterial[random->RandomInt( FTRAIL_FLAME1,FTRAIL_FLAME5 )], offset );

			if ( pParticle != NULL )
			{
				pParticle->m_flLifetime		= 0.0f;
				pParticle->m_flDieTime		= /*PARTICLE_LIFETIME*/ 0.5f;// + random->RandomFloat(PARTICLE_LIFETIME*0.75f, PARTICLE_LIFETIME*1.25f);

				pParticle->m_vecVelocity.Random( 0.0f, 1.0f );
				pParticle->m_vecVelocity *= random->RandomFloat( MIN_SPEED, MAX_SPEED );
				pParticle->m_vecVelocity[2] += 50;//random->RandomFloat( 32, 64 );
				
				pParticle->m_uchColor[0]	= 255.0f;
				pParticle->m_uchColor[1]	= 255.0f;
				pParticle->m_uchColor[2]	= 255.0f;
				
				pParticle->m_uchStartSize	= STARTSIZE * 2.0f;
				pParticle->m_uchEndSize		= STARTSIZE * 0.5f;
				
				pParticle->m_uchStartAlpha	= 255; 
				pParticle->m_uchEndAlpha	= 0;
				
				pParticle->m_flRoll			= 0.0f;//random->RandomInt( 0, 360 );
				pParticle->m_flRollDelta	= random->RandomFloat( -16.0f, 16.0f );
			}
		}

		//
		// Smoke
		//

		offset = RandomVector( -STARTSIZE*0.5f, STARTSIZE*0.5f ) + GetAbsOrigin();

		pParticle = (SimpleParticle *) m_pSmokeEmitter->AddParticle( sizeof( SimpleParticle ), m_hMaterial[random->RandomInt( FTRAIL_SMOKE1, FTRAIL_SMOKE2 )], offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime		= 0.0f;
			pParticle->m_flDieTime		= ( PARTICLE_LIFETIME * 10.0f ) + random->RandomFloat(PARTICLE_LIFETIME*0.75f, PARTICLE_LIFETIME*1.25f);

			pParticle->m_vecVelocity.Random( 0.0f, 1.0f );
			pParticle->m_vecVelocity *= random->RandomFloat( MIN_SPEED, MAX_SPEED );
			pParticle->m_vecVelocity[2] += random->RandomFloat( 50, 100 );
			
			pParticle->m_uchColor[0]	= 255.0f * 0.5f;
			pParticle->m_uchColor[1]	= 245.0f * 0.5f;
			pParticle->m_uchColor[2]	= 205.0f * 0.5f;
			
			pParticle->m_uchStartSize	= 16 * random->RandomFloat( 0.75f, 1.25f );
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 2.5f;
			
			pParticle->m_uchStartAlpha	= 64; 
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -16.0f, 16.0f );
		}
	}

	// Save off this position
	m_vecLastPosition = GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose:  High drag, non color changing particle
//-----------------------------------------------------------------------------


class CDustFollower : public CSimpleEmitter
{
public:
	
	CDustFollower( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	//Create
	static CDustFollower *Create( const char *pDebugName )
	{
		return new CDustFollower( pDebugName );
	}

	//Alpha
	virtual float UpdateAlpha( const SimpleParticle *pParticle )
	{
		return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
	}

	virtual	void	UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_vecVelocity = pParticle->m_vecVelocity * ExponentialDecay( 0.3, timeDelta );
	}

	//Roll
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta *= ExponentialDecay( 0.5, timeDelta );

		return pParticle->m_flRoll;
	}

private:
	CDustFollower( const CDustFollower & );
};


// Datatable.. this can have all the smoketrail parameters when we need it to.
IMPLEMENT_CLIENTCLASS_DT(C_DustTrail, DT_DustTrail, DustTrail)
	RecvPropFloat(RECVINFO(m_SpawnRate)),
	RecvPropVector(RECVINFO(m_Color)),
	RecvPropFloat(RECVINFO(m_ParticleLifetime)),
	RecvPropFloat(RECVINFO(m_StopEmitTime)),
	RecvPropFloat(RECVINFO(m_MinSpeed)),
	RecvPropFloat(RECVINFO(m_MaxSpeed)),
	RecvPropFloat(RECVINFO(m_MinDirectedSpeed)),
	RecvPropFloat(RECVINFO(m_MaxDirectedSpeed)),
	RecvPropFloat(RECVINFO(m_StartSize)),
	RecvPropFloat(RECVINFO(m_EndSize)),
	RecvPropFloat(RECVINFO(m_SpawnRadius)),
	RecvPropInt(RECVINFO(m_bEmit)),
	RecvPropFloat(RECVINFO(m_Opacity)),
END_RECV_TABLE()


// ------------------------------------------------------------------------- //
// ParticleMovieExplosion
// ------------------------------------------------------------------------- //
C_DustTrail::C_DustTrail()
{
	for (int i = 0; i < DUSTTRAIL_MATERIALS; i++)
	{
        m_MaterialHandle[i] = NULL;
	}

	m_SpawnRate = 10;
	m_ParticleSpawn.Init(10);
	m_Color.Init(0.5, 0.5, 0.5);
	m_ParticleLifetime = 5;
	m_StartEmitTime = gpGlobals->curtime;
	m_StopEmitTime = 0;	// No end time
	m_MinSpeed = 2;
	m_MaxSpeed = 4;
	m_MinDirectedSpeed = m_MaxDirectedSpeed = 0;
	m_StartSize = 35;
	m_EndSize = 55;
	m_SpawnRadius = 2;
	m_VelocityOffset.Init();
	m_Opacity = 0.5f;

	m_bEmit = true;

	m_pDustEmitter = NULL;
	m_pParticleMgr	= NULL;
}

C_DustTrail::~C_DustTrail()
{
	if ( ToolsEnabled() && clienttools->IsInRecordingMode() && m_pDustEmitter.IsValid() && m_pDustEmitter->GetToolParticleEffectId() != TOOLPARTICLESYSTEMID_INVALID )
	{
		KeyValues *msg = new KeyValues( "OldParticleSystem_ActivateEmitter" );
		msg->SetInt( "id", m_pDustEmitter->GetToolParticleEffectId() );
		msg->SetInt( "emitter", 0 );
		msg->SetInt( "active", false );
		msg->SetFloat( "time", gpGlobals->curtime );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}

	if ( m_pParticleMgr )
	{
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEmit - 
//-----------------------------------------------------------------------------
void C_DustTrail::SetEmit(bool bEmit)
{
	m_bEmit = bEmit;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rate - 
//-----------------------------------------------------------------------------
void C_DustTrail::SetSpawnRate(float rate)
{
	m_SpawnRate = rate;
	m_ParticleSpawn.Init(rate);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_DustTrail::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start( ParticleMgr(), NULL );
	}
}


// FIXME: These all have to be moved out of this old system and into the new to leverage art assets!
CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectDusttrail )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0001" )
/*
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0002" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0003" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0004" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0005" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0006" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0007" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0008" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0009" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0010" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0011" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0012" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0013" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0014" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0015" )
CLIENTEFFECT_MATERIAL( "particle/smokesprites_0016" )
*/
CLIENTEFFECT_REGISTER_END()


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_DustTrail::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	if(!pParticleMgr->AddEffect( &m_ParticleEffect, this ))
		return;

	m_pParticleMgr	= pParticleMgr;
	m_pDustEmitter = CDustFollower::Create("DustTrail");
	
	if ( !m_pDustEmitter )
	{
		Assert( false );
		return;
	}

	m_pDustEmitter->SetSortOrigin( GetAbsOrigin() );
	m_pDustEmitter->SetNearClip( 64.0f, 128.0f );

	for (int i = 0; i < DUSTTRAIL_MATERIALS; i++)
	{
		//char name[256];
		//Q_snprintf( name, sizeof( name ), "particle/smokesprites_%04d", i + 1 );
		m_MaterialHandle[i] = m_pDustEmitter->GetPMaterial( "particle/smokesprites_0001" );
	}
	
	m_ParticleSpawn.Init( m_SpawnRate );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_DustTrail::Update( float fTimeDelta )
{
	if ( !m_pDustEmitter )
		return;

	Vector	offsetColor;

	// Add new particles
	if ( !m_bEmit )
		return;

	if ( ( m_StopEmitTime != 0 ) && ( m_StopEmitTime <= gpGlobals->curtime ) )
		return;

	float tempDelta = fTimeDelta;

	SimpleParticle	*pParticle;
	Vector			offset;

	Vector vecOrigin;
	VectorMA( GetAbsOrigin(), -fTimeDelta, GetAbsVelocity(), vecOrigin );

	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );

	while( m_ParticleSpawn.NextEvent( tempDelta ) )
	{
		float fldt = fTimeDelta - tempDelta;

		offset.Random( -m_SpawnRadius, m_SpawnRadius );
		offset += vecOrigin;
		VectorMA( offset, fldt, GetAbsVelocity(), offset );

		//if ( random->RandomFloat( 0.f, 5.0f ) > GetAbsVelocity().Length())
		//	continue;

		pParticle = (SimpleParticle *) m_pDustEmitter->AddParticle( sizeof( SimpleParticle ), m_MaterialHandle[random->RandomInt(0,0)], offset ); // FIXME: the other sprites look bad

		if ( pParticle == NULL )
			continue;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= m_ParticleLifetime;

		pParticle->m_vecVelocity.Random( -1.0f, 1.0f );
		pParticle->m_vecVelocity *= random->RandomFloat( m_MinSpeed, m_MaxSpeed );

		pParticle->m_vecVelocity = pParticle->m_vecVelocity + GetAbsVelocity();
		
		float flDirectedVel = random->RandomFloat( m_MinDirectedSpeed, m_MaxDirectedSpeed );
		VectorMA( pParticle->m_vecVelocity, flDirectedVel, vecForward, pParticle->m_vecVelocity );

		offsetColor = m_Color;
		float flMaxVal = MAX( m_Color[0], m_Color[1] );
		if ( flMaxVal < m_Color[2] )
		{
			flMaxVal = m_Color[2];
		}
		offsetColor /= flMaxVal;

		offsetColor *= random->RandomFloat( -0.2f, 0.2f );
		offsetColor += m_Color;

		offsetColor[0] = clamp( offsetColor[0], 0.0f, 1.0f );
		offsetColor[1] = clamp( offsetColor[1], 0.0f, 1.0f );
		offsetColor[2] = clamp( offsetColor[2], 0.0f, 1.0f );

		pParticle->m_uchColor[0]	= offsetColor[0]*255.0f;
		pParticle->m_uchColor[1]	= offsetColor[1]*255.0f;
		pParticle->m_uchColor[2]	= offsetColor[2]*255.0f;
		
		pParticle->m_uchStartSize	= m_StartSize;
		pParticle->m_uchEndSize		= m_EndSize;
		
		float alpha = random->RandomFloat( m_Opacity*0.75f, m_Opacity*1.25f );
		alpha = clamp( alpha, 0.0f, 1.0f );

		if ( m_StopEmitTime != 0 && m_StopEmitTime > m_StartEmitTime )
		{
			alpha *= sqrt( (m_StopEmitTime - gpGlobals->curtime) /(m_StopEmitTime - m_StartEmitTime) );
		}

		pParticle->m_uchStartAlpha	= alpha * 255; 
		pParticle->m_uchEndAlpha	= 0;
			
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -1.0f, 1.0f );
    }
}


void C_DustTrail::RenderParticles( CParticleRenderIterator *pIterator )
{
}


void C_DustTrail::SimulateParticles( CParticleSimulateIterator *pIterator )
{
}


//-----------------------------------------------------------------------------
// This is called after sending this entity's recording state
//-----------------------------------------------------------------------------

void C_DustTrail::CleanupToolRecordingState( KeyValues *msg )
{
	if ( !ToolsEnabled() )
		return;

	BaseClass::CleanupToolRecordingState( msg );

	// Generally, this is used to allow the entity to clean up
	// allocated state it put into the message, but here we're going
	// to use it to send particle system messages because we
	// know the grenade has been recorded at this point
	if ( !clienttools->IsInRecordingMode() || !m_pDustEmitter.IsValid() )
		return;
	
	// For now, we can't record Dusttrails that don't have a moveparent
	C_BaseEntity *pEnt = GetMoveParent();
	if ( !pEnt )
		return;

	bool bEmitterActive = m_bEmit && ( ( m_StopEmitTime == 0 ) || ( m_StopEmitTime > gpGlobals->curtime ) );

	// NOTE: Particle system destruction message will be sent by the particle effect itself.
	if ( m_pDustEmitter->GetToolParticleEffectId() == TOOLPARTICLESYSTEMID_INVALID )
	{
		int nId = m_pDustEmitter->AllocateToolParticleEffectId();

		KeyValues *oldmsg = new KeyValues( "OldParticleSystem_Create" );
		oldmsg->SetString( "name", "C_DustTrail" );
		oldmsg->SetInt( "id", nId );
		oldmsg->SetFloat( "time", gpGlobals->curtime );

		KeyValues *pEmitter = oldmsg->FindKey( "DmeSpriteEmitter", true );
		pEmitter->SetString( "material", "particle/smokesprites_0001" );
		pEmitter->SetInt( "count", m_SpawnRate );	// particles per second, when duration is < 0
		pEmitter->SetFloat( "duration", -1 ); // FIXME
		pEmitter->SetInt( "active", bEmitterActive );

		KeyValues *pInitializers = pEmitter->FindKey( "initializers", true );

		// FIXME: Until we can interpolate ent logs during emission, this can't work
		KeyValues *pPosition = pInitializers->FindKey( "DmePositionPointToEntityInitializer", true );
		pPosition->SetPtr( "entindex", (void*)(intp)pEnt->entindex() );
		pPosition->SetInt( "attachmentIndex", GetParentAttachment() );
		pPosition->SetFloat( "randomDist", m_SpawnRadius );
		pPosition->SetFloat( "startx", pEnt->GetAbsOrigin().x );
		pPosition->SetFloat( "starty", pEnt->GetAbsOrigin().y );
		pPosition->SetFloat( "startz", pEnt->GetAbsOrigin().z );

		KeyValues *pVelocity = pInitializers->FindKey( "DmeDecayVelocityInitializer", true );
		pVelocity->SetFloat( "velocityX", pEnt->GetAbsVelocity().x );
		pVelocity->SetFloat( "velocityY", pEnt->GetAbsVelocity().y );
		pVelocity->SetFloat( "velocityZ", pEnt->GetAbsVelocity().z );
		pVelocity->SetFloat( "decayto", 0.5 );
		pVelocity->SetFloat( "decaytime", 0.3 );

		KeyValues *pLifetime = pInitializers->FindKey( "DmeRandomLifetimeInitializer", true );
		pLifetime->SetFloat( "minLifetime", m_ParticleLifetime );
 		pLifetime->SetFloat( "maxLifetime", m_ParticleLifetime );

		KeyValues *pRoll = pInitializers->FindKey( "DmeRandomRollInitializer", true );
		pRoll->SetFloat( "minRoll", 0.0f );
 		pRoll->SetFloat( "maxRoll", 360.0f );

		KeyValues *pRollSpeed = pInitializers->FindKey( "DmeRandomRollSpeedInitializer", true );
		pRollSpeed->SetFloat( "minRollSpeed", -1.0f );
 		pRollSpeed->SetFloat( "maxRollSpeed", 1.0f );

		KeyValues *pColor = pInitializers->FindKey( "DmeRandomValueColorInitializer", true );
		Color c( 
			FastFToC( clamp( m_Color.x, 0.f, 1.f ) ),
			FastFToC( clamp( m_Color.y, 0.f, 1.f ) ),
			FastFToC( clamp( m_Color.z, 0.f, 1.f ) ),
			255 );
		pColor->SetColor( "startColor", c );
		pColor->SetFloat( "minStartValueDelta", 0.0f );
 		pColor->SetFloat( "maxStartValueDelta", 0.0f );
		pColor->SetColor( "endColor", c );

		KeyValues *pAlpha = pInitializers->FindKey( "DmeRandomAlphaInitializer", true );
		int nMinAlpha = 255 * m_Opacity * 0.75f;
		int nMaxAlpha = 255 * m_Opacity * 1.25f;
		pAlpha->SetInt( "minStartAlpha", clamp( nMinAlpha, 0, 255 ) );
		pAlpha->SetInt( "maxStartAlpha", clamp( nMaxAlpha, 0, 255 ) );
		pAlpha->SetInt( "minEndAlpha", clamp( nMinAlpha, 0, 255 ) );
		pAlpha->SetInt( "maxEndAlpha", clamp( nMaxAlpha, 0, 255 ) );

		KeyValues *pSize = pInitializers->FindKey( "DmeRandomSizeInitializer", true );
		pSize->SetFloat( "minStartSize", m_StartSize );
		pSize->SetFloat( "maxStartSize", m_StartSize );
		pSize->SetFloat( "minEndSize", m_EndSize );
		pSize->SetFloat( "maxEndSize", m_EndSize );

		KeyValues *pUpdaters = pEmitter->FindKey( "updaters", true );
		pUpdaters->FindKey( "DmePositionVelocityDecayUpdater", true );
		pUpdaters->FindKey( "DmeRollUpdater", true );

		KeyValues *pRollSpeedUpdater = pUpdaters->FindKey( "DmeRollSpeedAttenuateUpdater", true );
		pRollSpeedUpdater->SetFloat( "attenuation", 1.0f - 8.0f / 30.0f );
		pRollSpeedUpdater->SetFloat( "attenuationTme", 1.0f / 30.0f );
		pRollSpeedUpdater->SetFloat( "minRollSpeed", 0.5f );

		pUpdaters->FindKey( "DmeAlphaSineRampUpdater", true );
		pUpdaters->FindKey( "DmeColorUpdater", true );
		pUpdaters->FindKey( "DmeSizeUpdater", true );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, oldmsg );
		oldmsg->deleteThis();
	}
	else 
	{
		KeyValues *oldmsg = new KeyValues( "OldParticleSystem_ActivateEmitter" );
		oldmsg->SetInt( "id", m_pDustEmitter->GetToolParticleEffectId() );
		oldmsg->SetInt( "emitter", 0 );
		oldmsg->SetInt( "active", bEmitterActive );
		oldmsg->SetFloat( "time", gpGlobals->curtime );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, oldmsg );
		oldmsg->deleteThis();
	}
}
