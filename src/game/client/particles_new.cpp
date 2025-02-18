//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "particlemgr.h"
#include "particles_new.h"
#include "iclientmode.h"
#include "engine/ivdebugoverlay.h"
#include "particle_property.h"
#include "toolframework/itoolframework.h"
#include "toolframework_client.h"
#include "tier1/KeyValues.h"
#include "model_types.h"
#include "vprof.h"
#include "input.h"

extern ConVar cl_particleeffect_aabb_buffer;

//extern ConVar cl_particle_show_bbox;
//extern ConVar cl_particle_show_bbox_cost;
extern bool g_cl_particle_show_bbox;
extern int g_cl_particle_show_bbox_cost;


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CNewParticleEffect::CNewParticleEffect( CBaseEntity *pOwner, CParticleSystemDefinition *pEffect )
{
	m_hOwner = pOwner;
	Init( pEffect );
	Construct();
}

CNewParticleEffect::CNewParticleEffect( CBaseEntity *pOwner, const char* pEffectName )
{
	m_hOwner = pOwner;
	Init( pEffectName );
	Construct();
}

void CNewParticleEffect::Construct()
{
	m_vSortOrigin.Init();

	m_bDontRemove = false;
	m_bRemove = false;
	m_bDrawn = false;
	m_bNeedsBBoxUpdate = false;
	m_bIsFirstFrame = true;
	m_bAutoUpdateBBox = false;
	m_bAllocated = true;
	m_bSimulate = true;
	m_bShouldPerformCullCheck = false;

	m_nToolParticleEffectId = TOOLPARTICLESYSTEMID_INVALID;
	m_RefCount = 0;
	ParticleMgr()->AddEffect( this );
	m_LastMax = Vector( -1.0e6, -1.0e6, -1.0e6 );
	m_LastMin = Vector( 1.0e6, 1.0e6, 1.0e6 );
	m_MinBounds = Vector( 1.0e6, 1.0e6, 1.0e6 );
	m_MaxBounds = Vector( -1.0e6, -1.0e6, -1.0e6 );
	m_pDebugName = NULL;

	m_bViewModelEffect = m_pDef ? m_pDef->IsViewModelEffect() : false;

	if ( IsValid() && clienttools->IsInRecordingMode() )
	{
		int nId = AllocateToolParticleEffectId();	

		static ParticleSystemCreatedState_t state;
		state.m_nParticleSystemId = nId;
		state.m_flTime = gpGlobals->curtime;
		state.m_pName = GetName();
		state.m_nOwner = m_hOwner.Get() ? m_hOwner->entindex() : -1;

		KeyValues *msg = new KeyValues( "ParticleSystem_Create" );
		msg->SetPtr( "state", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
	}
}

CNewParticleEffect::~CNewParticleEffect(void)
{
	if ( m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID && clienttools->IsInRecordingMode() )
	{
		static ParticleSystemDestroyedState_t state;
		state.m_nParticleSystemId = gpGlobals->curtime;
		state.m_flTime = gpGlobals->curtime;

		KeyValues *msg = new KeyValues( "ParticleSystem_Destroy" );
		msg->SetPtr( "state", &state );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		m_nToolParticleEffectId = TOOLPARTICLESYSTEMID_INVALID; 
	}

	m_bAllocated = false;
	if ( m_hOwner )
	{
		// NOTE: This can provoke another NotifyRemove call which is why flags is set to 0
		m_hOwner->ParticleProp()->OnParticleSystemDeleted( this );
	}
}


//-----------------------------------------------------------------------------
// Refcounting
//-----------------------------------------------------------------------------
void CNewParticleEffect::AddRef()
{
	++m_RefCount;
}

void CNewParticleEffect::Release()
{
	Assert( m_RefCount > 0 );
	--m_RefCount;

	// If all the particles are already gone, delete ourselves now.
	// If there are still particles, wait for the last NotifyDestroyParticle.
	if ( m_RefCount == 0 )
	{
		if ( m_bAllocated )
		{
			if ( IsFinished() )
			{
				SetRemoveFlag();
			}
		}
	}
}

void CNewParticleEffect::NotifyRemove()
{
	if ( m_bAllocated )
	{
		delete this;
	}
}

int CNewParticleEffect::IsReleased()
{
	return m_RefCount == 0;
}


//-----------------------------------------------------------------------------
// Refraction and soft particle support
//-----------------------------------------------------------------------------
bool CNewParticleEffect::UsesPowerOfTwoFrameBufferTexture( void )
{
	// NOTE: Need to do this because CParticleCollection's version is non-virtual
	return CParticleCollection::UsesPowerOfTwoFrameBufferTexture( true );
}

bool CNewParticleEffect::UsesFullFrameBufferTexture( void )
{
	// NOTE: Need to do this because CParticleCollection's version is non-virtual
	return CParticleCollection::UsesFullFrameBufferTexture( true );
}

bool CNewParticleEffect::IsTwoPass( void )
{
	// NOTE: Need to do this because CParticleCollection's version is non-virtual
	return CParticleCollection::IsTwoPass();
}


//-----------------------------------------------------------------------------
// Overrides for recording
//-----------------------------------------------------------------------------
void CNewParticleEffect::StopEmission( bool bInfiniteOnly, bool bRemoveAllParticles, bool bWakeOnStop )
{
	if ( m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID && clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "ParticleSystem_StopEmission" );

		static ParticleSystemStopEmissionState_t state;
		state.m_nParticleSystemId = GetToolParticleEffectId();
		state.m_flTime = gpGlobals->curtime;
		state.m_bInfiniteOnly = bInfiniteOnly;

		msg->SetPtr( "state", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
	}

	CParticleCollection::StopEmission( bInfiniteOnly, bRemoveAllParticles, bWakeOnStop );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNewParticleEffect::SetDormant( bool bDormant )
{
	CParticleCollection::SetDormant( bDormant );
}

void CNewParticleEffect::SetControlPointEntity( int nWhichPoint, CBaseEntity *pEntity )
{
	if ( m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID && clienttools->IsInRecordingMode() )
	{
		static ParticleSystemSetControlPointObjectState_t state;
		state.m_nParticleSystemId = GetToolParticleEffectId();
		state.m_flTime = gpGlobals->curtime;
		state.m_nControlPoint = nWhichPoint;
		state.m_nObject = pEntity ? pEntity->entindex() : -1;

		KeyValues *msg = new KeyValues( "ParticleSystem_SetControlPointObject" );
		msg->SetPtr( "state", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
	}

	if ( pEntity )
	{
		CParticleCollection::SetControlPointObject( nWhichPoint, &m_hControlPointOwners[ nWhichPoint ] );
		m_hControlPointOwners[ nWhichPoint ] = pEntity;
	}
	else
		CParticleCollection::SetControlPointObject( nWhichPoint, NULL );
}


void CNewParticleEffect::SetControlPoint( int nWhichPoint, const Vector &v )
{
	if ( m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID && clienttools->IsInRecordingMode() )
	{
		static ParticleSystemSetControlPointPositionState_t state;
		state.m_nParticleSystemId = GetToolParticleEffectId();
		state.m_flTime = gpGlobals->curtime;
		state.m_nControlPoint = nWhichPoint;
		state.m_vecPosition = v;

		KeyValues *msg = new KeyValues( "ParticleSystem_SetControlPointPosition" );
		msg->SetPtr( "state", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
	}

	CParticleCollection::SetControlPoint( nWhichPoint, v );
}


void CNewParticleEffect::RecordControlPointOrientation( int nWhichPoint )
{
	if ( m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID && clienttools->IsInRecordingMode() )
	{
		// FIXME: Make a more direct way of getting 
		QAngle angles;
		VectorAngles( m_ControlPoints[nWhichPoint].m_ForwardVector, m_ControlPoints[nWhichPoint].m_UpVector, angles );

		static ParticleSystemSetControlPointOrientationState_t state;
		state.m_nParticleSystemId = GetToolParticleEffectId();
		state.m_flTime = gpGlobals->curtime;
		state.m_nControlPoint = nWhichPoint;
		AngleQuaternion( angles, state.m_qOrientation );

		KeyValues *msg = new KeyValues( "ParticleSystem_SetControlPointOrientation" );
		msg->SetPtr( "state", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
	}
}

void CNewParticleEffect::SetControlPointOrientation( int nWhichPoint, 
	const Vector &forward, const Vector &right, const Vector &up )
{
	CParticleCollection::SetControlPointOrientation( nWhichPoint, forward, right, up );
	RecordControlPointOrientation( nWhichPoint );
}

void CNewParticleEffect::SetControlPointOrientation( int nWhichPoint, const Quaternion &q )
{
	CParticleCollection::SetControlPointOrientation( nWhichPoint, q );
	RecordControlPointOrientation( nWhichPoint );
}

void CNewParticleEffect::SetControlPointForwardVector( int nWhichPoint, const Vector &v )
{
	CParticleCollection::SetControlPointForwardVector( nWhichPoint, v );
	RecordControlPointOrientation( nWhichPoint );
}

void CNewParticleEffect::SetControlPointUpVector( int nWhichPoint, const Vector &v )
{
	CParticleCollection::SetControlPointUpVector( nWhichPoint, v );
	RecordControlPointOrientation( nWhichPoint );
}

void CNewParticleEffect::SetControlPointRightVector( int nWhichPoint, const Vector &v )
{
	CParticleCollection::SetControlPointRightVector( nWhichPoint, v );
	RecordControlPointOrientation( nWhichPoint );
}


//-----------------------------------------------------------------------------
// Called when the particle effect is about to update
//-----------------------------------------------------------------------------
void CNewParticleEffect::Update( float flTimeDelta )
{
	if ( m_hOwner )
	{
		m_hOwner->ParticleProp()->OnParticleSystemUpdated( this, flTimeDelta );
	}
}


//-----------------------------------------------------------------------------
// Bounding box
//-----------------------------------------------------------------------------
CNewParticleEffect* CNewParticleEffect::ReplaceWith( const char *pParticleSystemName )
{
	StopEmission( false, true, true );
	if ( !pParticleSystemName || !pParticleSystemName[0] )
		return NULL;

	CSmartPtr< CNewParticleEffect > pNewEffect = CNewParticleEffect::Create( GetOwner(), pParticleSystemName, pParticleSystemName );
	if ( !pNewEffect->IsValid() )
		return pNewEffect.GetObject();

	// Copy over the control point data
	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		if ( !ReadsControlPoint( i ) )
			continue;

		Vector vecForward, vecRight, vecUp;
		pNewEffect->SetControlPoint( i, GetControlPointAtCurrentTime( i ) );
		GetControlPointOrientationAtCurrentTime( i, &vecForward, &vecRight, &vecUp );
		pNewEffect->SetControlPointOrientation( i, vecForward, vecRight, vecUp );
		pNewEffect->SetControlPointParent( i, GetControlPointParent( i ) );
	}

	if ( !m_hOwner )
		return pNewEffect.GetObject();

	m_hOwner->ParticleProp()->ReplaceParticleEffect( this, pNewEffect.GetObject() );
	return pNewEffect.GetObject();
}


//-----------------------------------------------------------------------------
// Bounding box
//-----------------------------------------------------------------------------
void CNewParticleEffect::SetParticleCullRadius( float radius )
{
}

bool CNewParticleEffect::RecalculateBoundingBox()
{
	BloatBoundsUsingControlPoint();
	if ( m_MaxBounds.x < m_MinBounds.x )
	{
		m_MaxBounds = m_MinBounds = GetSortOrigin();
		return false;
	}

	return true;
}


void CNewParticleEffect::GetRenderBounds( Vector& mins, Vector& maxs )
{
	VectorSubtract( m_MinBounds, GetRenderOrigin(), mins );
	VectorSubtract( m_MaxBounds, GetRenderOrigin(), maxs );
}

void CNewParticleEffect::DetectChanges()
{
	// if we have no render handle, return
	if ( m_hRenderHandle == INVALID_CLIENT_RENDER_HANDLE )
		return;

	float flBuffer = cl_particleeffect_aabb_buffer.GetFloat();
	float flExtraBuffer = flBuffer * 1.3f;

	// if nothing changed, return
	if ( m_MinBounds.x < m_LastMin.x || 
		 m_MinBounds.y < m_LastMin.y || 
		 m_MinBounds.z < m_LastMin.z || 

		 m_MinBounds.x > (m_LastMin.x + flExtraBuffer) ||
		 m_MinBounds.y > (m_LastMin.y + flExtraBuffer) ||
		 m_MinBounds.z > (m_LastMin.z + flExtraBuffer) ||

		 m_MaxBounds.x > m_LastMax.x || 
		 m_MaxBounds.y > m_LastMax.y || 
		 m_MaxBounds.z > m_LastMax.z || 

		 m_MaxBounds.x < (m_LastMax.x - flExtraBuffer) ||
		 m_MaxBounds.y < (m_LastMax.y - flExtraBuffer) ||
		 m_MaxBounds.z < (m_LastMax.z - flExtraBuffer)
		 )
	{
		// call leafsystem to updated this guy
		ClientLeafSystem()->RenderableChanged( m_hRenderHandle );

		// remember last parameters
		// Add some padding in here so we don't reinsert it into the leaf system if it just changes a tiny amount.
		m_LastMin = m_MinBounds - Vector( flBuffer, flBuffer, flBuffer );
		m_LastMax = m_MaxBounds + Vector( flBuffer, flBuffer, flBuffer );
	}
}

extern ConVar r_DrawParticles;


void CNewParticleEffect::DebugDrawBbox ( bool bCulled )
{
	int nParticlesShowBboxCost = g_cl_particle_show_bbox_cost;
	bool bShowCheapSystems = false;
	if ( nParticlesShowBboxCost < 0 )
	{
		nParticlesShowBboxCost = -nParticlesShowBboxCost;
		bShowCheapSystems = true;
	}

	Vector center = GetRenderOrigin();
	Vector mins   = m_MinBounds - center;
	Vector maxs   = m_MaxBounds - center;
	
	int r, g, b;
	bool bDraw = true;
	if ( bCulled )
	{
		r = 64;
		g = 64;
		b = 64;
	}
	else if ( nParticlesShowBboxCost > 0 )
	{
		float fAmount = (float)m_nActiveParticles / (float)nParticlesShowBboxCost;
		if ( fAmount < 0.5f )
		{
			if ( bShowCheapSystems )
			{
				r = 0;
				g = 255;
				b = 0;
			}
			else
			{
				// Prevent the screen getting spammed with low-count particles which aren't that expensive.
				bDraw = false;
				r = 0;
				g = 0;
				b = 0;
			}
		}
		else if ( fAmount < 1.0f )
		{
			// green 0.5-1.0 blue
			int nBlend = (int)( 512.0f * ( fAmount - 0.5f ) );
			nBlend = MIN ( 255, MAX ( 0, nBlend ) );
			r = 0;
			g = 255 - nBlend;
			b = nBlend;
		}
		else if ( fAmount < 2.0f )
		{
			// blue 1.0-2.0 red
			int nBlend = (int)( 256.0f * ( fAmount - 1.0f ) );
			nBlend = MIN ( 255, MAX ( 0, nBlend ) );
			r = nBlend;
			g = 0;
			b = 255 - nBlend;
		}
		else
		{
			r = 255;
			g = 0;
			b = 0;
		}
	}
	else
	{
		if ( GetAutoUpdateBBox() )
		{
			// red is bad, the bbox update is costly
			r = 255;
			g = 0;
			b = 0;
		}
		else
		{
			// green, this effect presents less cpu load 
			r = 0;
			g = 255;
			b = 0;
		}
	}
		 
	if ( bDraw )
	{
		debugoverlay->AddBoxOverlay( center, mins, maxs, QAngle( 0, 0, 0 ), r, g, b, 16, 0 );
		debugoverlay->AddTextOverlayRGB( center, 0, 0, r, g, b, 64, "%s:(%d)", GetEffectName(), m_nActiveParticles );
	}
}


//-----------------------------------------------------------------------------
// Rendering
//-----------------------------------------------------------------------------
int CNewParticleEffect::DrawModel( int flags )
{
	VPROF_BUDGET( "CNewParticleEffect::DrawModel", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	if ( r_DrawParticles.GetBool() == false )
		return 0;

	if ( !g_pClientMode->ShouldDrawParticles() || !ParticleMgr()->ShouldRenderParticleSystems() )
		return 0;
	
	if ( ( flags & ( STUDIO_SHADOWDEPTHTEXTURE | STUDIO_SSAODEPTHTEXTURE ) ) != 0 )
	{
		return 0;
	}
	
	// do distance cull check here. We do it here instead of in particles so we can easily only do
	// it for root objects, not bothering to cull children individually
	CMatRenderContextPtr pRenderContext( materials );
	Vector vecCamera;
	pRenderContext->GetWorldSpaceCameraPosition( &vecCamera );
	if ( CalcSqrDistanceToAABB( m_MinBounds, m_MaxBounds, vecCamera ) > ( m_pDef->m_flMaxDrawDistance * m_pDef->m_flMaxDrawDistance ) )
	{
		if ( !IsRetail() && ( g_cl_particle_show_bbox || ( g_cl_particle_show_bbox_cost != 0 ) ) )
		{
			DebugDrawBbox ( true );
		}

		// Still need to make sure we set this or they won't follow their attachemnt points.
		m_flNextSleepTime = Max ( m_flNextSleepTime, ( g_pParticleSystemMgr->GetLastSimulationTime() + m_pDef->m_flNoDrawTimeToGoToSleep ));

		return 0;
	}

	if ( flags & STUDIO_TRANSPARENCY )
	{
		int viewentity = render->GetViewEntity();
		C_BaseEntity *pCameraObject = cl_entitylist->GetEnt( viewentity );
		// apply logic that lets you skip rendering a system if the camera is attached to its entity
		if ( pCameraObject &&
			 ( m_pDef->m_nSkipRenderControlPoint != -1 ) &&
			 ( m_pDef->m_nSkipRenderControlPoint <= m_nHighestCP ) )
		{
			C_BaseEntity *pEntity = (EHANDLE)GetControlPointEntity( m_pDef->m_nSkipRenderControlPoint );
			if ( pEntity )
			{
				// If we're in thirdperson, we still see it
				if ( !input->CAM_IsThirdPerson() )
				{
					if ( pEntity == pCameraObject )
						return 0;
					C_BaseEntity *pRootMove = pEntity->GetRootMoveParent();
					if ( pRootMove == pCameraObject )
						return 0;

					// If we're spectating in-eyes of the camera object, we don't see it
					C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
					if ( pPlayer == pCameraObject )
					{
						C_BaseEntity *pObTarget = pPlayer->GetObserverTarget();
						if ( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE && (pObTarget == pEntity || pRootMove == pObTarget ) )
							return 0;
					}
				}
			}
		}

		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->PushMatrix();
		pRenderContext->LoadIdentity();
		Render( pRenderContext, IsTwoPass(), pCameraObject );
		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->PopMatrix();
	}
	else
	{
		g_pParticleSystemMgr->AddToRenderCache( this );
	}

	if ( !IsRetail() )
	{
		CParticleMgr *pMgr = ParticleMgr();
		if ( pMgr->m_bStatsRunning )
		{
			pMgr->StatsNewParticleEffectDrawn ( this );
		}

		if ( g_cl_particle_show_bbox || ( g_cl_particle_show_bbox_cost != 0 ) )
		{
			DebugDrawBbox ( false );
		}
	}

	return 1;
}

static void DumpParticleStats_f( void )
{
	g_pParticleSystemMgr->DumpProfileInformation();
}

static ConCommand cl_dump_particle_stats( "cl_dump_particle_stats", DumpParticleStats_f, "dump particle profiling info to particle_profile.csv") ;

