//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "particles_simple.h"
#include "env_wind_shared.h"
#include "KeyValues.h"
#include "toolframework_client.h"
#include "toolframework/itoolframework.h"
#include "vstdlib/IKeyValuesSystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Used for debugging to make sure all particle effects get freed when we exit.
CUtlLinkedList<CParticleEffect*,int> g_ParticleEffects;
class CEffectChecker
{
public:
	~CEffectChecker()
	{
		Assert( g_ParticleEffects.Count() == 0 );
	}
} g_EffectChecker;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CParticleEffect::CParticleEffect( const char *pName )
{
	m_pDebugName = pName;
	m_vSortOrigin.Init();
	m_Flags = FLAG_ALLOCATED;
	m_nToolParticleEffectId = TOOLPARTICLESYSTEMID_INVALID;
	m_RefCount = 0;
	m_bSimulate = true;
	ParticleMgr()->AddEffect( &m_ParticleEffect, this );
#if defined( _DEBUG )
	g_ParticleEffects.AddToTail( this );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CParticleEffect::~CParticleEffect( void )
{
#if defined( _DEBUG )
	int index = g_ParticleEffects.Find( this );
	Assert( g_ParticleEffects.IsValidIndex(index) );
	g_ParticleEffects.Remove( index );
#endif
	// HACKHACK: Prevent re-entering the destructor, clear m_Flags.
	// For some reason we'll get a callback into NotifyRemove() after being deleted!
	// Investigate dangling pointer
	m_Flags = 0;

#if !defined( _XBOX )
	if ( ( m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID ) && clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "OldParticleSystem_Destroy" );
		msg->SetInt( "id", m_nToolParticleEffectId );
		msg->SetFloat( "time", gpGlobals->curtime );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		m_nToolParticleEffectId = TOOLPARTICLESYSTEMID_INVALID; 
	}
#endif
}


void CParticleEffect::SetDynamicallyAllocated( bool bDynamic )
{
	if( bDynamic )
		m_Flags |= FLAG_ALLOCATED;
	else
		m_Flags &= ~FLAG_ALLOCATED;
}


int CParticleEffect::IsReleased()
{
	return m_RefCount == 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleEffect::AddRef()
{
	++m_RefCount;
}


void CParticleEffect::Release()
{
	Assert( m_RefCount > 0 );
	--m_RefCount;

	// If all the particles are already gone, delete ourselves now.
	// If there are still particles, wait for the last NotifyDestroyParticle.
	if ( m_RefCount == 0 )
	{
		if ( m_Flags & FLAG_ALLOCATED )
		{
			if ( m_ParticleEffect.GetNumActiveParticles() == 0 )
			{
				m_ParticleEffect.SetRemoveFlag();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vSortOrigin - 
//-----------------------------------------------------------------------------
const Vector &CParticleEffect::GetSortOrigin()
{
	Assert(m_vSortOrigin.IsValid());
	return m_vSortOrigin;
}

const char *CParticleEffect::GetEffectName()
{
	return m_pDebugName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pParticle - 
//-----------------------------------------------------------------------------
void CParticleEffect::NotifyDestroyParticle( Particle* pParticle )
{
	// Go away if we're released and there are no more particles.
	if( m_ParticleEffect.GetNumActiveParticles() == 0 && IsReleased() && (m_Flags & FLAG_ALLOCATED) && !(m_Flags & FLAG_DONT_REMOVE) )
	{
		m_ParticleEffect.SetRemoveFlag();
	}
}


void CParticleEffect::Update( float flTimeDelta )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleEffect::NotifyRemove()
{
	if( m_Flags & FLAG_ALLOCATED )
	{
		Assert( IsReleased() );
		delete this;
	}
}


void CParticleEffect::SetSortOrigin( const Vector &vSortOrigin )
{
	if ( GetBinding().GetAutoUpdateBBox() )
	{
		if ( m_ParticleEffect.EnlargeBBoxToContain( vSortOrigin ) )
		{
			m_vSortOrigin = vSortOrigin;
		}
	}
	else
	{
		// If not auto-updating bbox, don't change the bbox, just set the sort origin.
		m_vSortOrigin = vSortOrigin;
	}
}

void CParticleEffect::SetParticleCullRadius( float radius )
{
	m_ParticleEffect.SetParticleCullRadius( radius );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : PMaterialHandle
//-----------------------------------------------------------------------------
PMaterialHandle CParticleEffect::GetPMaterial(const char *name)
{
	return m_ParticleEffect.FindOrAddMaterial(name);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : particleSize - 
//			material - 
// Output : SimpleParticle
//-----------------------------------------------------------------------------
Particle *CParticleEffect::AddParticle( unsigned int particleSize, PMaterialHandle material, const Vector &origin )
{
	// If you get here, then you must call SetSortOrigin before adding particles.
	Assert( m_vSortOrigin.IsValid() );

	Particle *pParticle = (Particle *) m_ParticleEffect.AddParticle( particleSize, material );

	if( pParticle == NULL )
		return NULL;

	pParticle->m_Pos = origin;
	return pParticle;
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------

REGISTER_EFFECT_USING_CREATE( CSimpleEmitter );

CSimpleEmitter::CSimpleEmitter( const char *pDebugName ) : CParticleEffect( pDebugName )
{
	m_flNearClipMin	= 16.0f;
	m_flNearClipMax	= 64.0f;
}


CSimpleEmitter::~CSimpleEmitter()
{
}

CSmartPtr<CSimpleEmitter> CSimpleEmitter::Create( const char *pDebugName )
{
	CSimpleEmitter *pRet = new CSimpleEmitter( pDebugName );
	pRet->SetDynamicallyAllocated( true );
	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: Set the internal near clip range for this particle system
// Input  : nearClipMin - beginning of clip range
//			nearClipMax - end of clip range
//-----------------------------------------------------------------------------
void CSimpleEmitter::SetNearClip( float nearClipMin, float nearClipMax )
{ 
	m_flNearClipMin = nearClipMin;
	m_flNearClipMax = nearClipMax;
}


SimpleParticle*	CSimpleEmitter::AddSimpleParticle( 
	PMaterialHandle hMaterial, 
	const Vector &vOrigin,
	float flDieTime,
	unsigned char uchSize )
{
	SimpleParticle *pRet = (SimpleParticle*)AddParticle( sizeof( SimpleParticle ), hMaterial, vOrigin );
	if ( pRet )
	{
		pRet->m_Pos = vOrigin;
		pRet->m_vecVelocity.Init();
		pRet->m_flRoll = 0;
		pRet->m_flRollDelta = 0;
		pRet->m_flLifetime = 0;
		pRet->m_flDieTime = flDieTime;
		pRet->m_uchColor[0] = pRet->m_uchColor[1] = pRet->m_uchColor[2] = 0;
		pRet->m_uchStartAlpha = pRet->m_uchEndAlpha = 255;
		pRet->m_uchStartSize = pRet->m_uchEndSize = uchSize;
		pRet->m_iFlags = 0;
	}

	return pRet;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float CSimpleEmitter::UpdateAlpha( const SimpleParticle *pParticle )
{
	return (pParticle->m_uchStartAlpha/255.0f) + ( (float)(pParticle->m_uchEndAlpha/255.0f) - (float)(pParticle->m_uchStartAlpha/255.0f) ) * (pParticle->m_flLifetime / pParticle->m_flDieTime);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float CSimpleEmitter::UpdateScale( const SimpleParticle *pParticle )
{
	return	(float)pParticle->m_uchStartSize + ( (float)pParticle->m_uchEndSize - (float)pParticle->m_uchStartSize ) * (pParticle->m_flLifetime / pParticle->m_flDieTime);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
// Output : Vector
//-----------------------------------------------------------------------------

#define WIND_ACCEL 50

void CSimpleEmitter::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
	if (pParticle->m_iFlags & SIMPLE_PARTICLE_FLAG_WINDBLOWN)
	{
		Vector vecWind;
		GetWindspeedAtTime( gpGlobals->curtime, vecWind );

		for ( int i = 0 ; i < 2 ; i++ )
		{
			if ( pParticle->m_vecVelocity[i] < vecWind[i] )
			{
				pParticle->m_vecVelocity[i] += ( timeDelta * WIND_ACCEL );

				// clamp
				if ( pParticle->m_vecVelocity[i] > vecWind[i] )
					pParticle->m_vecVelocity[i] = vecWind[i];
			}
			else if (pParticle->m_vecVelocity[i] > vecWind[i] )
			{
				pParticle->m_vecVelocity[i] -= ( timeDelta * WIND_ACCEL );

				// clamp.
				if ( pParticle->m_vecVelocity[i] < vecWind[i] )
					pParticle->m_vecVelocity[i] = vecWind[i];
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float CSimpleEmitter::UpdateRoll( SimpleParticle *pParticle, float timeDelta )
{
	pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;

	return pParticle->m_flRoll;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
Vector CSimpleEmitter::UpdateColor( const SimpleParticle *pParticle )
{
	static Vector	cColor;

	cColor[0] = pParticle->m_uchColor[0] / 255.0f;
	cColor[1] = pParticle->m_uchColor[1] / 255.0f;
	cColor[2] = pParticle->m_uchColor[2] / 255.0f;

	return cColor;
}

void CSimpleEmitter::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	float timeDelta = pIterator->GetTimeDelta();

	SimpleParticle *pParticle = (SimpleParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		//Update velocity
		UpdateVelocity( pParticle, timeDelta );
		pParticle->m_Pos += pParticle->m_vecVelocity * timeDelta;

		//Should this particle die?
		pParticle->m_flLifetime += timeDelta;
		UpdateRoll( pParticle, timeDelta );

		if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
			pIterator->RemoveParticle( pParticle );

		pParticle = (SimpleParticle*)pIterator->GetNext();
	}
}

void CSimpleEmitter::RenderParticles( CParticleRenderIterator *pIterator )
{
	const SimpleParticle *pParticle = (const SimpleParticle *)pIterator->GetFirst();
	while ( pParticle )
	{
		//Render
		Vector	tPos;

		TransformParticle( ParticleMgr()->GetModelView(), pParticle->m_Pos, tPos );
		float sortKey = (int) tPos.z;

		//Render it
		RenderParticle_ColorSizeAngle(
			pIterator->GetParticleDraw(),
			tPos,
			UpdateColor( pParticle ),
			UpdateAlpha( pParticle ) * GetAlphaDistanceFade( tPos, m_flNearClipMin, m_flNearClipMax ),
			UpdateScale( pParticle ),
			pParticle->m_flRoll
			);

		pParticle = (const SimpleParticle *)pIterator->GetNext( sortKey );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CSimpleEmitter::SetDrawBeforeViewModel( bool state )
{
	m_ParticleEffect.SetDrawBeforeViewModel( state );
}

//==================================================
// Particle Library
//==================================================

CEmberEffect::CEmberEffect( const char *pDebugName ) : CSimpleEmitter( pDebugName )
{
}


CSmartPtr<CEmberEffect> CEmberEffect::Create( const char *pDebugName )
{
	CEmberEffect *pRet = new CEmberEffect( pDebugName );
	pRet->SetDynamicallyAllocated( true );
	return pRet;
}


void CEmberEffect::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
	float	speed = VectorNormalize( pParticle->m_vecVelocity );
	Vector	offset;

	speed -= ( 12.0f * timeDelta );

	offset.Random( -0.125f, 0.125f );

	pParticle->m_vecVelocity += offset;
	VectorNormalize( pParticle->m_vecVelocity );

	pParticle->m_vecVelocity *= speed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
Vector CEmberEffect::UpdateColor( const SimpleParticle *pParticle )
{
	Vector	color;
	float	ramp = 1.0f - ( pParticle->m_flLifetime / pParticle->m_flDieTime );

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
CFireSmokeEffect::CFireSmokeEffect( const char *pDebugName ) : CSimpleEmitter( pDebugName )
{
}


CSmartPtr<CFireSmokeEffect> CFireSmokeEffect::Create( const char *pDebugName )
{
	CFireSmokeEffect *pRet = new CFireSmokeEffect( pDebugName );
	pRet->SetDynamicallyAllocated( true );
	return pRet;
}


float CFireSmokeEffect::UpdateAlpha( const SimpleParticle *pParticle )
{
	return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
void CFireSmokeEffect::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
CFireParticle::CFireParticle( const char *pDebugName ) : CSimpleEmitter( pDebugName )
{
}


CSmartPtr<CFireParticle> CFireParticle::Create( const char *pDebugName )
{
	CFireParticle *pRet = new CFireParticle( pDebugName );
	pRet->SetDynamicallyAllocated( true );
	return pRet;
}


Vector CFireParticle::UpdateColor( const SimpleParticle *pParticle )
{
	for ( int i = 0; i < 3; i++ )
	{
		//FIXME: This is frame dependant... but I don't want to store off start/end colors yet
		//pParticle->m_uchColor[i] = MAX( 0, pParticle->m_uchColor[i]-2 );
	}

	return CSimpleEmitter::UpdateColor( pParticle );
}
