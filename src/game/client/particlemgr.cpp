//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//


#include "cbase.h"
#include "particlemgr.h"
#include "particledraw.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "mempool.h"
#include "iclientmode.h"
#include "view_scene.h"
#include "tier0/vprof.h"
#include "engine/ivdebugoverlay.h"
#include "view.h"
#include "KeyValues.h"
#include "particles/particles.h"							// get new particle system access
#include "tier1/utlintrusivelist.h"
#include "particles_new.h"
#include "vstdlib/jobthread.h"
#include "filesystem.h"
#include "particle_parse.h"
#include "model_types.h"
#ifdef TF_CLIENT_DLL
#include "rtime.h"
#endif
#include "tier0/icommandline.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IParticleSystemQuery *g_pParticleSystemQuery;

static int g_nParticlesDrawn;
// CCycleCount	g_ParticleTimer;

ConVar r_DrawParticles("r_drawparticles", "1", FCVAR_CHEAT, "Enable/disable particle rendering");
static ConVar particle_simulateoverflow( "particle_simulateoverflow", "0", FCVAR_CHEAT, "Used for stress-testing particle systems. Randomly denies creation of particles." );
ConVar cl_particleeffect_aabb_buffer( "cl_particleeffect_aabb_buffer", "2", FCVAR_CHEAT, "Add this amount to a particle effect's bbox in the leaf system so if it's growing slowly, it won't have to be reinserted as often." );
ConVar cl_particle_show_bbox( "cl_particle_show_bbox", "0", FCVAR_CHEAT );
ConVar cl_particle_show_bbox_cost( "cl_particle_show_bbox_cost", "0", FCVAR_CHEAT, "Show # of particles: green->blue->red. Use a negative number to show ALL particles even cheap ones" );

// These reflect the convars so we don't parse the string every particle!
bool g_cl_particle_show_bbox = false;
int g_cl_particle_show_bbox_cost = 0;


static void StatsParticlesStart();
static void StatsParticlesStop();

static ConCommand cl_particle_stats_start( "cl_particle_stats_start", StatsParticlesStart, "Start or restart particle stats - also dumps to particle_stats.csv") ;
static ConCommand cl_particle_stats_stop( "cl_particle_stats_stop", StatsParticlesStop, "Stop particle stats, or snapshot this frame - also dumps to particle_stats.csv") ;
static ConVar cl_particle_stats_trigger_count( "cl_particle_stats_trigger_count", "0", 0, "Dump stats if the particle count exceeds this number." );



#define BUCKET_SORT_EVERY_N		8			// It does a bucket sort for each material approximately every N times.
#define BBOX_UPDATE_EVERY_N		8			// It does a full bbox update (checks all particles instead of every eighth one).

//-----------------------------------------------------------------------------
//
// Particle manager implementation
//
//-----------------------------------------------------------------------------

#define PARTICLE_SIZE	96

CParticleMgr *ParticleMgr()
{
	static CParticleMgr s_ParticleMgr;
	return &s_ParticleMgr;
}


//-----------------------------------------------------------------------------
// CParticleSubTextureGroup implementation.
//-----------------------------------------------------------------------------

CParticleSubTextureGroup::CParticleSubTextureGroup()
{
	m_pPageMaterial = NULL;
}


CParticleSubTextureGroup::~CParticleSubTextureGroup()
{
}


//-----------------------------------------------------------------------------
// CParticleSubTexture implementation.
//-----------------------------------------------------------------------------

CParticleSubTexture::CParticleSubTexture()
{
	m_tCoordMins[0] = m_tCoordMins[1] = 0;
	m_tCoordMaxs[0] = m_tCoordMaxs[1] = 1;
	m_pGroup = &m_DefaultGroup;
	m_pMaterial = NULL;

#ifdef _DEBUG
	m_szDebugName = NULL;
#endif
}


//-----------------------------------------------------------------------------
// CEffectMaterial.
//-----------------------------------------------------------------------------

CEffectMaterial::CEffectMaterial()
{
	m_Particles.m_pNext = m_Particles.m_pPrev = &m_Particles;
	m_pGroup = NULL;
}

					
//-----------------------------------------------------------------------------
// CParticleEffectBinding.
//-----------------------------------------------------------------------------
CParticleEffectBinding::CParticleEffectBinding()
{
	m_pParticleMgr = NULL;
	m_pSim = NULL;

	m_LocalSpaceTransform.Identity();
	m_bLocalSpaceTransformIdentity = true;
	
	m_Flags = 0;
	SetAutoUpdateBBox( true );
	SetFirstFrameFlag( true );
	SetNeedsBBoxUpdate( true );
	SetAlwaysSimulate( true );
	SetEffectCameraSpace( true );
	SetDrawThruLeafSystem( true );
	SetAutoApplyLocalTransform( true );

	// default bbox
	m_Min.Init( -50, -50, -50 );
	m_Max.Init( 50, 50, 50 );

	m_LastMin = m_Min;
	m_LastMax = m_Max;

	SetParticleCullRadius( 0.0f );
	m_nActiveParticles = 0;

	m_FrameCode = 0;
	m_ListIndex = 0xFFFF; 

	m_UpdateBBoxCounter = 0;

	memset( m_EffectMaterialHash, 0, sizeof( m_EffectMaterialHash ) );
}


CParticleEffectBinding::~CParticleEffectBinding()
{
	if( m_pParticleMgr )
		m_pParticleMgr->RemoveEffect( this );

	Term();
}

// The is the max size of the particles for use in bounding	computation
void CParticleEffectBinding::SetParticleCullRadius( float flMaxParticleRadius )
{
	if ( m_flParticleCullRadius != flMaxParticleRadius )
	{
		m_flParticleCullRadius = flMaxParticleRadius;

		if ( m_hRenderHandle != INVALID_CLIENT_RENDER_HANDLE )
		{
			ClientLeafSystem()->RenderableChanged( m_hRenderHandle );
		}
	}
}

const Vector& CParticleEffectBinding::GetRenderOrigin( void )
{
	return m_pSim->GetSortOrigin();
}


const QAngle& CParticleEffectBinding::GetRenderAngles( void )
{
	return vec3_angle;
}

const matrix3x4_t &	CParticleEffectBinding::RenderableToWorldTransform()
{
	static matrix3x4_t mat;
	SetIdentityMatrix( mat );
	PositionMatrix( GetRenderOrigin(), mat );
	return mat;
}

void CParticleEffectBinding::GetRenderBounds( Vector& mins, Vector& maxs )
{
	const Vector &vSortOrigin = m_pSim->GetSortOrigin();

	// Convert to local space (around the sort origin).
	mins = m_Min - vSortOrigin;
	mins.x -= m_flParticleCullRadius; mins.y -= m_flParticleCullRadius; mins.z -= m_flParticleCullRadius;
	maxs = m_Max - vSortOrigin;
	maxs.x += m_flParticleCullRadius; maxs.y += m_flParticleCullRadius; maxs.z += m_flParticleCullRadius;
}

bool CParticleEffectBinding::ShouldDraw( void )
{
	return GetFlag( FLAGS_DRAW_THRU_LEAF_SYSTEM ) != 0;
}


bool CParticleEffectBinding::IsTransparent( void )
{
	return true;
}


inline void CParticleEffectBinding::StartDrawMaterialParticles(
	CEffectMaterial *pMaterial,
	float flTimeDelta,
	IMesh* &pMesh,
	CMeshBuilder &builder,
	ParticleDraw &particleDraw,
	bool bWireframe )
{
	CMatRenderContextPtr pRenderContext( m_pParticleMgr->m_pMaterialSystem );

	// Setup the ParticleDraw and bind the material.
	if( bWireframe )
	{
		IMaterial *pMaterialWire = m_pParticleMgr->m_pMaterialSystem->FindMaterial( "debug/debugparticlewireframe", TEXTURE_GROUP_OTHER );
		pRenderContext->Bind( pMaterialWire, NULL );
	}
	else
	{
		pRenderContext->Bind( pMaterial->m_pGroup->m_pPageMaterial, m_pParticleMgr );
	}

	pMesh = pRenderContext->GetDynamicMesh( true );

	builder.Begin( pMesh, MATERIAL_QUADS, NUM_PARTICLES_PER_BATCH * 4 );
	particleDraw.Init( &builder, pMaterial->m_pGroup->m_pPageMaterial, flTimeDelta );
}


void CParticleEffectBinding::BBoxCalcStart( Vector &bbMin, Vector &bbMax )
{
	if ( !GetAutoUpdateBBox() )
		return;

	// We're going to fully recompute the bbox.
	bbMin.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	bbMax.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
}


void CParticleEffectBinding::BBoxCalcEnd( bool bboxSet, Vector &bbMin, Vector &bbMax )
{
	if ( !GetAutoUpdateBBox() )
		return;

	// Get the bbox into world space.
	Vector bbMinWorld, bbMaxWorld;
	if ( m_bLocalSpaceTransformIdentity )
	{
		bbMinWorld = bbMin;
		bbMaxWorld = bbMax;
	}
	else
	{
		TransformAABB( m_LocalSpaceTransform.As3x4(), bbMin, bbMax, bbMinWorld, bbMaxWorld );
	}

	// If there were ANY particles in the system, then we've got a valid bbox here. Otherwise,
	// we don't have anything, so leave m_Min and m_Max at the sort origin.
	if ( bboxSet )
	{
		m_Min = bbMinWorld;
		m_Max = bbMaxWorld;
	}
	else
	{
		m_Min = m_Max = m_pSim->GetSortOrigin();
	}
}


int CParticleEffectBinding::DrawModel( int flags )
{
	VPROF_BUDGET( "CParticleEffectBinding::DrawModel", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
#ifndef PARTICLEPROTOTYPE_APP
	if ( !r_DrawParticles.GetInt() )
		return 0;
#endif

	Assert( flags != 0 );

	// If we're in commander mode and it's trying to draw the effect,
	// exit out. If the effect has FLAGS_ALWAYSSIMULATE set, then it'll come back
	// in here and simulate at the end of the frame.

	// NOTE: We do not check ParticleMgr()->ShouldRenderParticleSystems()
	// here as a sort of hack: the SFM currently plays back Tempents, which create
	// old-style particle systems back during playback, which means we want
	// them to display always
	if( !g_pClientMode->ShouldDrawParticles() )
		return 0;

	//Avoid drawing particles while building depth textures. Perf win.
	//At the very least, we absolutely should not do refraction updates below. So if this gets removed, be sure to wrap the refract/screen texture updates.
	if( flags & ( STUDIO_SHADOWDEPTHTEXTURE | STUDIO_SSAODEPTHTEXTURE ) )
	{
		return 0;
	}

	SetDrawn( true );
	
	// Don't do anything if there are no particles.
	if( !m_nActiveParticles )
		return 1;

	// Reset the transformation matrix to identity.
	VMatrix mTempModel, mTempView;
	RenderStart( mTempModel, mTempView );

	bool bBucketSort = random->RandomInt( 0, BUCKET_SORT_EVERY_N ) == 0;

	// Set frametime to zero if we've already rendered this frame.
	float flFrameTime = 0;
	if ( m_FrameCode != m_pParticleMgr->m_FrameCode )
	{
		m_FrameCode = m_pParticleMgr->m_FrameCode;
		flFrameTime = Helper_GetFrameTime();
	}

	// For each material, render...
	// This does an incremental bubble sort. It only does one pass every frame, and it will shuffle 
	// unsorted particles one step towards where they should be.
	bool bWireframe = false;
	FOR_EACH_LL( m_Materials, iMaterial )
	{
		CEffectMaterial *pMaterial = m_Materials[iMaterial];
		
		if ( pMaterial->m_pGroup->m_pPageMaterial && pMaterial->m_pGroup->m_pPageMaterial->NeedsPowerOfTwoFrameBufferTexture() )
		{
			UpdateRefractTexture();
		}

		if ( pMaterial->m_pGroup->m_pPageMaterial && pMaterial->m_pGroup->m_pPageMaterial->NeedsFullFrameBufferTexture() )
		{
			UpdateScreenEffectTexture();
		}
		
		DrawMaterialParticles( 
			bBucketSort,
			pMaterial, 
			flFrameTime,
			bWireframe );
	}

	if ( ShouldDrawInWireFrameMode() )
	{
		bWireframe = true;
		FOR_EACH_LL( m_Materials, iDrawMaterial )
		{
			CEffectMaterial *pMaterial = m_Materials[iDrawMaterial];
			
			DrawMaterialParticles( 
				bBucketSort,
				pMaterial, 
				flFrameTime,
				bWireframe );
		}
	}


	if ( !IsRetail() )
	{
		CParticleMgr *pMgr = ParticleMgr();
		if ( pMgr->m_bStatsRunning )
		{
			pMgr->StatsOldParticleEffectDrawn ( this );
		}

		if ( g_cl_particle_show_bbox || ( g_cl_particle_show_bbox_cost != 0 ) )
		{
			int nParticlesShowBboxCost = g_cl_particle_show_bbox_cost;
			bool bShowCheapSystems = false;
			if ( nParticlesShowBboxCost < 0 )
			{
				nParticlesShowBboxCost = -nParticlesShowBboxCost;
				bShowCheapSystems = true;
			}

			Vector center = (m_Min + m_Max)/2;
			Vector mins   = m_Min - center;
			Vector maxs   = m_Max - center;

			int r, g, b;
			bool bDraw = true;
			if ( nParticlesShowBboxCost > 0 )
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
				if ( m_Flags & FLAGS_AUTOUPDATEBBOX )
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
				if ( debugoverlay )
				{
					debugoverlay->AddBoxOverlay( center, mins, maxs, QAngle( 0, 0, 0 ), r, g, b, 16, 0 );
					debugoverlay->AddTextOverlayRGB( center, 0, 0, r, g, b, 64, "%s:(%d)", m_pSim->GetEffectName(), m_nActiveParticles );
				}
			}
		}
	}

	RenderEnd( mTempModel, mTempView );
	return 1;
}


PMaterialHandle CParticleEffectBinding::FindOrAddMaterial( const char *pMaterialName )
{
	if ( !m_pParticleMgr )
	{
		return NULL;
	}

	return m_pParticleMgr->GetPMaterial( pMaterialName );
}


Particle* CParticleEffectBinding::AddParticle( int sizeInBytes, PMaterialHandle hMaterial )
{
	m_pParticleMgr->RepairPMaterial( hMaterial ); //HACKHACK: Remove this when we can stop leaking handles from level to level.

	// We've currently clamped the particle size to PARTICLE_SIZE,
	// we may need to change this algorithm if we get particles with
	// widely varying size
	if ( sizeInBytes > PARTICLE_SIZE )
	{
		Assert( sizeInBytes <= PARTICLE_SIZE );
		return NULL;
	}

	// This is for testing - simulate it running out of memory.
	if ( particle_simulateoverflow.GetInt() )
	{
		if ( rand() % 10 <= 6 )
			return NULL;
	}
	
	// Allocate the puppy. We are actually allocating space for the
	// internals + the actual data
	Particle* pParticle = m_pParticleMgr->AllocParticle( PARTICLE_SIZE );
	if( !pParticle )
		return NULL;

	// Link it in
	CEffectMaterial *pEffectMat = GetEffectMaterial( hMaterial );
	InsertParticleAfter( pParticle, &pEffectMat->m_Particles );
	
	if ( hMaterial )
		pParticle->m_pSubTexture = hMaterial;
	else
		pParticle->m_pSubTexture = &m_pParticleMgr->m_DefaultInvalidSubTexture;

	++m_nActiveParticles;
	return pParticle;
}

void CParticleEffectBinding::SetBBox( const Vector &bbMin, const Vector &bbMax, bool bDisableAutoUpdate )
{
	m_Min = bbMin;
	m_Max = bbMax;
	
	if ( bDisableAutoUpdate )
		SetAutoUpdateBBox( false );
}

void CParticleEffectBinding::GetWorldspaceBounds( Vector *pMins, Vector *pMaxs )
{
	*pMins = m_Min;
	*pMaxs = m_Max;
}

void CParticleEffectBinding::SetLocalSpaceTransform( const matrix3x4_t &transform )
{
	m_LocalSpaceTransform.CopyFrom3x4( transform );
	if ( m_LocalSpaceTransform.IsIdentity() )
	{
		m_bLocalSpaceTransformIdentity = true;
	}
	else
	{
		m_bLocalSpaceTransformIdentity = false;
	}
}

bool CParticleEffectBinding::EnlargeBBoxToContain( const Vector &pt )
{
	if ( m_nActiveParticles == 0 )
	{
		m_Min = m_Max = pt;
		return true;
	}

	bool bHasChanged = false;

	// check min bounds
	if ( pt.x < m_Min.x ) 
		{ m_Min.x = pt.x; bHasChanged = true; }

	if ( pt.y < m_Min.y ) 
		{ m_Min.y = pt.y; bHasChanged = true; }

	if ( pt.z < m_Min.z ) 
		{ m_Min.z = pt.z; bHasChanged = true; }

	// check max bounds
	if ( pt.x > m_Max.x ) 
		{ m_Max.x = pt.x; bHasChanged = true; }

	if ( pt.y > m_Max.y ) 
		{ m_Max.y = pt.y; bHasChanged = true; }

	if ( pt.z > m_Max.z ) 
		{ m_Max.z = pt.z; bHasChanged = true; }

	return bHasChanged;
}


void CParticleEffectBinding::DetectChanges()
{
	// if we have no render handle, return
	if ( m_hRenderHandle == INVALID_CLIENT_RENDER_HANDLE )
		return;

	float flBuffer = cl_particleeffect_aabb_buffer.GetFloat();
	float flExtraBuffer = flBuffer * 1.3f;

	// if nothing changed, return
	if ( m_Min.x < m_LastMin.x || 
		 m_Min.y < m_LastMin.y || 
		 m_Min.z < m_LastMin.z || 

		 m_Min.x > (m_LastMin.x + flExtraBuffer) ||
		 m_Min.y > (m_LastMin.y + flExtraBuffer) ||
		 m_Min.z > (m_LastMin.z + flExtraBuffer) ||

		 m_Max.x > m_LastMax.x || 
		 m_Max.y > m_LastMax.y || 
		 m_Max.z > m_LastMax.z || 

		 m_Max.x < (m_LastMax.x - flExtraBuffer) ||
		 m_Max.y < (m_LastMax.y - flExtraBuffer) ||
		 m_Max.z < (m_LastMax.z - flExtraBuffer)
		 )
	{
		// call leafsystem to updated this guy
		ClientLeafSystem()->RenderableChanged( m_hRenderHandle );

		// remember last parameters
		// Add some padding in here so we don't reinsert it into the leaf system if it just changes a tiny amount.
		m_LastMin = m_Min - Vector( flBuffer, flBuffer, flBuffer );
		m_LastMax = m_Max + Vector( flBuffer, flBuffer, flBuffer );
	}
}


void CParticleEffectBinding::GrowBBoxFromParticlePositions( CEffectMaterial *pMaterial, bool &bboxSet, Vector &bbMin, Vector &bbMax )
{
	// If its bbox is manually set, don't bother updating it here.
	if ( !GetAutoUpdateBBox() )
		return;

	for( Particle *pCur=pMaterial->m_Particles.m_pNext; pCur != &pMaterial->m_Particles; pCur=pCur->m_pNext )
	{
		// Update bounding box 
		VectorMin( bbMin, pCur->m_Pos, bbMin );
		VectorMax( bbMax, pCur->m_Pos, bbMax );
		bboxSet = true;
	}
}


//-----------------------------------------------------------------------------
// Simulate particles
//-----------------------------------------------------------------------------
void CParticleEffectBinding::SimulateParticles( float flTimeDelta )
{
	if ( !m_pSim->ShouldSimulate() )
		return;

	if ( GetFlag( FLAGS_NEW_PARTICLE_SYSTEM ) )
	{
		CParticleSimulateIterator simulateIterator;
		simulateIterator.m_pEffectBinding = this;
		simulateIterator.m_pMaterial = NULL; //pMaterial;
		simulateIterator.m_flTimeDelta = flTimeDelta;
		m_pSim->SimulateParticles( &simulateIterator );
	}
	else
	{
		Vector bbMin(0,0,0), bbMax(0,0,0);
		bool bboxSet = false;

		// slow the expensive update operation for particle systems that use auto-update-bbox
		// auto update the bbox after N frames then randomly 1/N or after 2*N frames 
		bool bFullBBoxUpdate = false;
		++m_UpdateBBoxCounter;
		if ( ( m_UpdateBBoxCounter >= BBOX_UPDATE_EVERY_N && random->RandomInt( 0, BBOX_UPDATE_EVERY_N ) == 0 ) ||
			 ( m_UpdateBBoxCounter >= 2*BBOX_UPDATE_EVERY_N ) )
		{
			bFullBBoxUpdate = true;

			// reset watchdog
			m_UpdateBBoxCounter = 0;
		}

		if ( bFullBBoxUpdate )
		{
			BBoxCalcStart( bbMin, bbMax );
		}
		FOR_EACH_LL( m_Materials, i )
		{
			CEffectMaterial *pMaterial = m_Materials[i];

			CParticleSimulateIterator simulateIterator;

			simulateIterator.m_pEffectBinding = this;
			simulateIterator.m_pMaterial = pMaterial;
			simulateIterator.m_flTimeDelta = flTimeDelta;

			m_pSim->SimulateParticles( &simulateIterator );

			// Update the bbox.
			if ( bFullBBoxUpdate )
			{
				GrowBBoxFromParticlePositions( pMaterial, bboxSet, bbMin, bbMax );
			}
		}
		if ( bFullBBoxUpdate )
		{
			BBoxCalcEnd( bboxSet, bbMin, bbMax );
		}
	}
}


void CParticleEffectBinding::SetDrawThruLeafSystem( int bDraw )
{
	// NOTE (2012/11/27, TomF) - this whole system seems to be deprecated - nothing ever checks these flags, and CParticleMgr::DrawBeforeViewModelEffects is never called by anything!

	if ( bDraw )
	{
		// If SetDrawBeforeViewModel was called, then they shouldn't be telling it to draw through
		// the leaf system too.
		Assert( !( m_Flags & FLAGS_DRAW_BEFORE_VIEW_MODEL) );
	}

	SetFlag( FLAGS_DRAW_THRU_LEAF_SYSTEM, bDraw ); 
}


void CParticleEffectBinding::SetDrawBeforeViewModel( int bDraw )
{
	// NOTE (2012/11/27, TomF) - this whole system seems to be deprecated - nothing ever checks these flags, and CParticleMgr::DrawBeforeViewModelEffects is never called by anything!

	// Don't draw through the leaf system if they want it to specifically draw before the view model.
	if ( bDraw )
		m_Flags &= ~FLAGS_DRAW_THRU_LEAF_SYSTEM;
	
	SetFlag( FLAGS_DRAW_BEFORE_VIEW_MODEL, bDraw ); 
}


int CParticleEffectBinding::GetNumActiveParticles()
{
	return m_nActiveParticles;
}

// Build a list of all active particles
int CParticleEffectBinding::GetActiveParticleList( int nCount, Particle **ppParticleList )
{
	int nCurrCount = 0;

	FOR_EACH_LL( m_Materials, i )
	{
		CEffectMaterial *pMaterial = m_Materials[i];
		Particle *pParticle = pMaterial->m_Particles.m_pNext;
		for ( ; pParticle != &pMaterial->m_Particles; pParticle = pParticle->m_pNext )
		{
			ppParticleList[nCurrCount] = pParticle;
			if ( ++nCurrCount == nCount )
				return nCurrCount;
		}
	}

	return nCurrCount;
}


int CParticleEffectBinding::DrawMaterialParticles( 
	bool bBucketSort,
	CEffectMaterial *pMaterial, 
	float flTimeDelta,
	bool bWireframe
	 )
{
	// Setup everything.
	CMeshBuilder builder;
	ParticleDraw particleDraw;
	IMesh *pMesh = NULL;
	StartDrawMaterialParticles( pMaterial, flTimeDelta, pMesh, builder, particleDraw, bWireframe );

	if ( m_nActiveParticles > MAX_TOTAL_PARTICLES )
		Error( "CParticleEffectBinding::DrawMaterialParticles: too many particles (%d should be less than %d)", m_nActiveParticles, MAX_TOTAL_PARTICLES );

	// Simluate and render all the particles.
	CParticleRenderIterator renderIterator;

	renderIterator.m_pEffectBinding = this;
	renderIterator.m_pMaterial = pMaterial;
	renderIterator.m_pParticleDraw = &particleDraw;
	renderIterator.m_pMeshBuilder = &builder;
	renderIterator.m_pMesh = pMesh;
	renderIterator.m_bBucketSort = bBucketSort;

	m_pSim->RenderParticles( &renderIterator );
	g_nParticlesDrawn += m_nActiveParticles;

	if( bBucketSort )
	{
		DoBucketSort( pMaterial, renderIterator.m_zCoords, renderIterator.m_nZCoords, renderIterator.m_MinZ, renderIterator.m_MaxZ );
	}

	// Flush out any remaining particles.
	builder.End( false, true );
	
	return m_nActiveParticles;
}


void CParticleEffectBinding::RenderStart( VMatrix &tempModel, VMatrix &tempView )
{
	if( IsEffectCameraSpace() )
	{
		CMatRenderContextPtr pRenderContext( m_pParticleMgr->m_pMaterialSystem );

		// Store matrices off so we can restore them in RenderEnd().
		pRenderContext->GetMatrix(MATERIAL_VIEW, &tempView);
		pRenderContext->GetMatrix(MATERIAL_MODEL, &tempModel);

		// We're gonna assume the model matrix was identity and blow it off
		// This means that the particle positions are all specified in world space
		// which makes bounding box computations faster. 
		m_pParticleMgr->m_mModelView = tempView;

		// Force the user clip planes to use the old view matrix
		pRenderContext->EnableUserClipTransformOverride( true );
		pRenderContext->UserClipTransform( tempView );

		// The particle renderers want to do things in camera space
		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->LoadIdentity();

		pRenderContext->MatrixMode( MATERIAL_VIEW );
		pRenderContext->LoadIdentity();
	}
	else
	{
		m_pParticleMgr->m_mModelView.Identity();
	}

	// Add their local space transform if they have one and they want it applied.
	if ( GetAutoApplyLocalTransform() && !m_bLocalSpaceTransformIdentity )
	{
		m_pParticleMgr->m_mModelView = m_pParticleMgr->m_mModelView * m_LocalSpaceTransform;
	}

	// Let the particle effect do any per-frame setup/processing here
	m_pSim->StartRender( m_pParticleMgr->m_mModelView );
}


void CParticleEffectBinding::RenderEnd( VMatrix &tempModel, VMatrix &tempView )
{
	if( IsEffectCameraSpace() )
	{
		CMatRenderContextPtr pRenderContext( m_pParticleMgr->m_pMaterialSystem );

		// Make user clip planes work normally
		pRenderContext->EnableUserClipTransformOverride( false );

		// Reset the model matrix.
		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->LoadMatrix( tempModel );

		// Reset the view matrix.
		pRenderContext->MatrixMode( MATERIAL_VIEW );
		pRenderContext->LoadMatrix( tempView );
	}
}


void CParticleEffectBinding::DoBucketSort( CEffectMaterial *pMaterial, float *zCoords, int nZCoords, float minZ, float maxZ )
{
	// Do an O(N) bucket sort. This helps the sort when there are lots of particles.
	#define NUM_BUCKETS	32
	Particle buckets[NUM_BUCKETS];
	for( int iBucket=0; iBucket < NUM_BUCKETS; iBucket++ )
	{
		buckets[iBucket].m_pPrev = buckets[iBucket].m_pNext = &buckets[iBucket];
	}
	
	// Sort into buckets.
	int iCurParticle = 0;
	Particle *pNext, *pCur;
	for( pCur=pMaterial->m_Particles.m_pNext; pCur != &pMaterial->m_Particles; pCur=pNext )
	{
		pNext = pCur->m_pNext;
		if( iCurParticle >= nZCoords )
			break;

		// Remove it..
		UnlinkParticle( pCur );

		// Add it to the appropriate bucket.
		float flPercent;
		if (maxZ == minZ)
			flPercent = 0;
		else
			flPercent = (zCoords[iCurParticle] - minZ) / (maxZ - minZ);

		int iAddBucket = (int)( flPercent * (NUM_BUCKETS - 0.0001f) );
		iAddBucket = NUM_BUCKETS - iAddBucket - 1;
		Assert( iAddBucket >= 0 && iAddBucket < NUM_BUCKETS );

		InsertParticleAfter( pCur, &buckets[iAddBucket] );

		++iCurParticle;
	}

	// Put the buckets back into the main list.
	for( int iReAddBucket=0; iReAddBucket < NUM_BUCKETS; iReAddBucket++ )
	{
		Particle *pListHead = &buckets[iReAddBucket];
		for( pCur=pListHead->m_pNext; pCur != pListHead; pCur=pNext )
		{
			pNext = pCur->m_pNext;
			InsertParticleAfter( pCur, &pMaterial->m_Particles );
			--iCurParticle;
		}
	}

	Assert(iCurParticle==0);
}


void CParticleEffectBinding::Init( CParticleMgr *pMgr, IParticleEffect *pSim )
{
	// Must Term before reinitializing.
	Assert( !m_pSim && !m_pParticleMgr );

	m_pSim = pSim;
	m_pParticleMgr = pMgr;
}


void CParticleEffectBinding::Term()
{
	if ( !m_pParticleMgr )
		return;

	// Free materials.
	FOR_EACH_LL( m_Materials, iMaterial )
	{
		CEffectMaterial *pMaterial = m_Materials[iMaterial];

		// Remove all particles tied to this effect.
		Particle *pNext = NULL;
		for(Particle *pCur = pMaterial->m_Particles.m_pNext; pCur != &pMaterial->m_Particles; pCur=pNext )
		{
			pNext = pCur->m_pNext;
			
			RemoveParticle( pCur );
		}
		
		delete pMaterial;
	}	
	m_Materials.Purge();

	memset( m_EffectMaterialHash, 0, sizeof( m_EffectMaterialHash ) );
}


void CParticleEffectBinding::RemoveParticle( Particle *pParticle )
{
	UnlinkParticle( pParticle );
	
	// Important that this is updated BEFORE NotifyDestroyParticle is called.
	--m_nActiveParticles;
	Assert( m_nActiveParticles >= 0 );

	// Let the effect do any necessary cleanup
	m_pSim->NotifyDestroyParticle(pParticle);

	// Remove it from the list of particles and deallocate
	m_pParticleMgr->FreeParticle(pParticle);
}


bool CParticleEffectBinding::RecalculateBoundingBox()
{
	if ( m_nActiveParticles == 0 )
	{
		m_Max = m_Min = m_pSim->GetSortOrigin();
		return false;
	}

	Vector bbMin(  1e28,  1e28,  1e28 );
	Vector bbMax( -1e28, -1e28, -1e28 );

	FOR_EACH_LL( m_Materials, iMaterial )
	{
		CEffectMaterial *pMaterial = m_Materials[iMaterial];
		
		for( Particle *pCur=pMaterial->m_Particles.m_pNext; pCur != &pMaterial->m_Particles; pCur=pCur->m_pNext )
		{
			VectorMin( bbMin, pCur->m_Pos, bbMin );
			VectorMax( bbMax, pCur->m_Pos, bbMax );
		}
	}

	// Get the bbox into world space.
	if ( m_bLocalSpaceTransformIdentity )
	{
		m_Min = bbMin;
		m_Max = bbMax;
	}
	else
	{
		TransformAABB( m_LocalSpaceTransform.As3x4(), bbMin, bbMax, m_Min, m_Max );
	}

	return true;
}


CEffectMaterial* CParticleEffectBinding::GetEffectMaterial( CParticleSubTexture *pSubTexture )
{
	// Hash the IMaterial pointer.
	unsigned long index = (((uintp)pSubTexture->m_pGroup) >> 6) % EFFECT_MATERIAL_HASH_SIZE;
	for ( CEffectMaterial *pCur=m_EffectMaterialHash[index]; pCur; pCur = pCur->m_pHashedNext )
	{
		if ( pCur->m_pGroup == pSubTexture->m_pGroup )
			return pCur;
	}

	CEffectMaterial *pEffectMat = new CEffectMaterial;
	pEffectMat->m_pGroup = pSubTexture->m_pGroup;
	pEffectMat->m_pHashedNext = m_EffectMaterialHash[index];
	m_EffectMaterialHash[index] = pEffectMat;

	m_Materials.AddToTail( pEffectMat );
	return pEffectMat;
}


//-----------------------------------------------------------------------------
// CParticleMgr
//-----------------------------------------------------------------------------
CParticleMgr::CParticleMgr()
{
	m_nToolParticleEffectId = 0;
	m_bUpdatingEffects = false;
	m_bRenderParticleEffects = true;
	m_pMaterialSystem = NULL;
	m_pThreadPool[0] = 0;
	m_pThreadPool[1] = 0;
	memset( &m_DirectionalLight, 0, sizeof( m_DirectionalLight ) );

	m_FrameCode = 1;

	m_DefaultInvalidSubTexture.m_pGroup = &m_DefaultInvalidSubTexture.m_DefaultGroup;
	m_DefaultInvalidSubTexture.m_pMaterial = NULL;
	m_DefaultInvalidSubTexture.m_tCoordMins[0] = m_DefaultInvalidSubTexture.m_tCoordMins[1] = 0;
	m_DefaultInvalidSubTexture.m_tCoordMaxs[0] = m_DefaultInvalidSubTexture.m_tCoordMaxs[1] = 1;
	
	m_nCurrentParticlesAllocated = 0;

	SetDefLessFunc( m_effectFactories );
}

CParticleMgr::~CParticleMgr()
{
	Term();
}


//-----------------------------------------------------------------------------
// Initialization and shutdown
//-----------------------------------------------------------------------------
bool CParticleMgr::Init(unsigned long count, IMaterialSystem *pMaterials)
{
	Term();

	m_bStatsRunning = false;
	m_nStatsFramesSinceLastAlert = 0;

	m_pMaterialSystem = pMaterials;

	// Initialize the particle system
	g_pParticleSystemMgr->Init( g_pParticleSystemQuery );
	// tell particle mgr to add the default simulation + rendering ops
	g_pParticleSystemMgr->AddBuiltinSimulationOperators();
	g_pParticleSystemMgr->AddBuiltinRenderingOperators();

	// Send true to load the sheets
	ParseParticleEffects( true, false );

#ifdef TF_CLIENT_DLL
	if ( IsX360() )
	{
		//m_pThreadPool[0] = CreateThreadPool();
		m_pThreadPool[1] = CreateThreadPool();

		ThreadPoolStartParams_t startParams;
		startParams.nThreads = 3;
		startParams.nStackSize = 128*1024;
		startParams.fDistribute = TRS_TRUE;
		startParams.bUseAffinityTable = true;    
		startParams.iAffinityTable[0] = XBOX_PROCESSOR_1;
		startParams.iAffinityTable[1] = XBOX_PROCESSOR_3;
		startParams.iAffinityTable[2] = XBOX_PROCESSOR_5;
		//m_pThreadPool[0]->Start( startParams );

		startParams.nThreads = 2;
		startParams.iAffinityTable[1] = CommandLine()->FindParm( "-swapcores" ) ? XBOX_PROCESSOR_5 : XBOX_PROCESSOR_3;
		m_pThreadPool[1]->Start( startParams );
	}
#endif

	return true;
}

void CParticleMgr::Term()
{
	// Free all the effects.
	int iNext;
	for ( int i = m_Effects.Head(); i != m_Effects.InvalidIndex(); i = iNext )
	{
		iNext = m_Effects.Next( i );
		m_Effects[i]->m_pSim->NotifyRemove();
	}
	m_Effects.Purge();
	m_NewEffects.Purge();

	for( int i = m_SubTextures.First(); i != m_SubTextures.InvalidIndex(); i = m_SubTextures.Next( i ) )
	{	
		IMaterial *pMaterial = m_SubTextures[i]->m_pMaterial;
		if ( pMaterial )
			pMaterial->Release();
	}
	m_SubTextures.PurgeAndDeleteElements();

	for( int i = m_SubTextureGroups.Count(); --i >= 0; )
	{	
		IMaterial *pMaterial = m_SubTextureGroups[i]->m_pPageMaterial;
		if ( pMaterial )
			pMaterial->Release();
	}
	m_SubTextureGroups.PurgeAndDeleteElements();

	g_pParticleSystemMgr->UncacheAllParticleSystems();
	if ( m_pMaterialSystem )
	{
		m_pMaterialSystem->UncacheUnusedMaterials();
	}
	m_pMaterialSystem = NULL;
	
	if ( m_pThreadPool[0] )
	{
		m_pThreadPool[0]->Stop();
		DestroyThreadPool( m_pThreadPool[0] );
		m_pThreadPool[0] = NULL;
	}
	if ( m_pThreadPool[1] )
	{
		m_pThreadPool[1]->Stop();
		DestroyThreadPool( m_pThreadPool[1] );
		m_pThreadPool[1] = NULL;
	}

	Assert( m_nCurrentParticlesAllocated == 0 );
}


void CParticleMgr::LevelInit()
{
	g_pParticleSystemMgr->SetLastSimulationTime( gpGlobals->curtime );
}


Particle *CParticleMgr::AllocParticle( int size )
{
	// Enforce max particle limit.
	if ( m_nCurrentParticlesAllocated >= MAX_TOTAL_PARTICLES )
		return NULL;
		
	Particle *pRet = (Particle *)malloc( size );
	if ( pRet )
		++m_nCurrentParticlesAllocated;

	return pRet;
}

void CParticleMgr::FreeParticle( Particle *pParticle )
{
	Assert( m_nCurrentParticlesAllocated > 0 );
	if ( pParticle )
		--m_nCurrentParticlesAllocated;
	
	free( pParticle );
}


//-----------------------------------------------------------------------------
// Should particle effects be rendered?
//-----------------------------------------------------------------------------
void CParticleMgr::RenderParticleSystems( bool bEnable )
{
	m_bRenderParticleEffects = bEnable;
}

bool CParticleMgr::ShouldRenderParticleSystems() const
{
	return m_bRenderParticleEffects;
}


//-----------------------------------------------------------------------------
// add a class that gets notified of entity events
//-----------------------------------------------------------------------------
void CParticleMgr::AddEffectListener( IClientParticleListener *pListener )
{
	int i = m_effectListeners.Find( pListener );
	if ( !m_effectListeners.IsValidIndex( i ) )
	{
		m_effectListeners.AddToTail( pListener );
	}
}

void CParticleMgr::RemoveEffectListener( IClientParticleListener *pListener )
{
	int i = m_effectListeners.Find( pListener );
	if ( m_effectListeners.IsValidIndex( i ) )
	{
		m_effectListeners.Remove( i );
	}
}



//-----------------------------------------------------------------------------
// registers effects classes, and create instances of these effects classes
//-----------------------------------------------------------------------------

void CParticleMgr::RegisterEffect( const char *pEffectType, CreateParticleEffectFN func )
{
#ifdef _DEBUG
	int i = m_effectFactories.Find( pEffectType );
	Assert( !m_effectFactories.IsValidIndex( i ) );
#endif

	m_effectFactories.Insert( pEffectType, func );
}

IParticleEffect *CParticleMgr::CreateEffect( const char *pEffectType )
{
	int i = m_effectFactories.Find( pEffectType );
	if ( !m_effectFactories.IsValidIndex( i ) )
	{
		Msg( "CParticleMgr::CreateEffect: factory not found for effect '%s'\n", pEffectType );
		return NULL;
	}

	CreateParticleEffectFN func = m_effectFactories[ i ];
	if ( func == NULL )
	{
		Msg( "CParticleMgr::CreateEffect: NULL factory for effect '%s'\n", pEffectType );
		return NULL;
	}

	return func();
}


//-----------------------------------------------------------------------------
// Adds and removes effects from our global list
//-----------------------------------------------------------------------------
void CParticleMgr::AddEffect( CNewParticleEffect *pEffect )
{
	m_NewEffects.AddToHead( pEffect );

#if !defined( PARTICLEPROTOTYPE_APP )
	ClientLeafSystem()->CreateRenderableHandle( pEffect );
#endif
	if ( pEffect->IsValid() && pEffect->m_pDef->IsViewModelEffect() )
	{
		ClientLeafSystem()->SetRenderGroup( pEffect->RenderHandle(), RENDER_GROUP_VIEW_MODEL_TRANSLUCENT );
	}
}


bool CParticleMgr::AddEffect( CParticleEffectBinding *pEffect, IParticleEffect *pSim )
{
#ifdef _DEBUG
	FOR_EACH_LL( m_Effects, i )
	{
		if( m_Effects[i]->m_pSim == pSim )
		{
			Assert( !"CParticleMgr::AddEffect: added same effect twice" );
			return false;
		}
	}
#endif

	pEffect->Init( this, pSim );

	// Add it to the leaf system.
#if !defined( PARTICLEPROTOTYPE_APP )
	ClientLeafSystem()->CreateRenderableHandle( pEffect );
#endif

	pEffect->m_ListIndex = m_Effects.AddToTail( pEffect );

	Assert( pEffect->m_ListIndex != 0xFFFF );

	// notify listeners
	int nListeners = m_effectListeners.Count();
	for ( int i = 0; i < nListeners; ++i )
	{
		m_effectListeners[ i ]->OnParticleEffectAdded( pSim );
	}

	return true;
}


void CParticleMgr::RemoveEffect( CParticleEffectBinding *pEffect )
{
	// This prevents certain recursive situations where a NotifyRemove
	// call can wind up triggering another one, usually in an effect's
	// destructor.
	if( pEffect->GetRemovalInProgressFlag() )
		return;
	pEffect->SetRemovalInProgressFlag();

	// Don't call RemoveEffect while inside an IParticleEffect's Update() function.
	// Return false from the Update function instead.
	Assert( !m_bUpdatingEffects );

	// notify listeners
	int nListeners = m_effectListeners.Count();
	for ( int i = 0; i < nListeners; ++i )
	{
		m_effectListeners[ i ]->OnParticleEffectRemoved( pEffect->m_pSim );
	}

	// Take it out of the leaf system.
	ClientLeafSystem()->RemoveRenderable( pEffect->m_hRenderHandle );

	int listIndex = pEffect->m_ListIndex;
	if ( pEffect->m_pSim )
	{
		pEffect->m_pSim->NotifyRemove();
		m_Effects.Remove( listIndex );
		
	}
	else
	{
		Assert( listIndex == 0xFFFF );
	}
}

void CParticleMgr::RemoveEffect( CNewParticleEffect *pEffect )
{
	// Don't call RemoveEffect while inside an IParticleEffect's Update() function.
	// Return false from the Update function instead.
	Assert( !m_bUpdatingEffects );

#if !defined( PARTICLEPROTOTYPE_APP )
	// Take it out of the leaf system.
	ClientLeafSystem()->RemoveRenderable( pEffect->m_hRenderHandle );
#endif

	m_NewEffects.RemoveNode( pEffect );
	pEffect->NotifyRemove();
}


void CParticleMgr::RemoveAllNewEffects()
{
	// Remove any of the new effects that were flagged to be removed.
	for( CNewParticleEffect *pNewEffect = m_NewEffects.m_pHead; pNewEffect;  )
	{
		CNewParticleEffect *pNextEffect = pNewEffect->m_pNext;
		// see it any entitiy has a particle prop pointing at this one. this loop through all
		// entities shouldn't be important perf-wise because it only happens on reload
		C_BaseEntityIterator iterator;
		C_BaseEntity *pEnt;
		while ( (pEnt = iterator.Next()) != NULL )
		{
			if ( pEnt->ParticleProp() )
			{
				pEnt->ParticleProp()->OnParticleSystemDeleted( pNewEffect );
			}
		}		
		RemoveEffect( pNewEffect );
		pNewEffect = pNextEffect;
	}
}

void CParticleMgr::RemoveAllEffects()
{
	int iNext;
	for ( int i = m_Effects.Head(); i != m_Effects.InvalidIndex(); i = iNext )
	{
		iNext = m_Effects.Next( i );
		RemoveEffect( m_Effects[i] );

	}

	RemoveAllNewEffects();

	for( int i = m_SubTextures.First(); i != m_SubTextures.InvalidIndex(); i = m_SubTextures.Next( i ) )
	{	
		IMaterial *pMaterial = m_SubTextures[i]->m_pMaterial;
		if ( pMaterial )
			pMaterial->Release();

		m_SubTextures[i]->m_pMaterial = NULL;
	}
	//HACKHACK: commented out because we need to keep leaking handles until every piece of code that grabs one ditches it at level end
	//m_SubTextures.PurgeAndDeleteElements();

	for( int i = m_SubTextureGroups.Count(); --i >= 0; )
	{	
		IMaterial *pMaterial = m_SubTextureGroups[i]->m_pPageMaterial;
		if ( pMaterial )
			pMaterial->Release();

		m_SubTextureGroups[i]->m_pPageMaterial = NULL;
	}
	//HACKHACK: commented out because we need to keep leaking handles until every piece of code that grabs one ditches it at level end
	//m_SubTextureGroups.PurgeAndDeleteElements();
}



void CParticleMgr::IncrementFrameCode()
{
	VPROF( "CParticleMgr::IncrementFrameCode()" );

	++m_FrameCode;
	if ( m_FrameCode == 0 )
	{
		// Reset all the CParticleEffectBindings..
		FOR_EACH_LL( m_Effects, i )
		{
			m_Effects[i]->m_FrameCode = 0;
		}

		m_FrameCode = 1;
	}
	//!!new!!
}


//-----------------------------------------------------------------------------
// Main rendering loop
//-----------------------------------------------------------------------------
void CParticleMgr::Simulate( float flTimeDelta )
{
	g_nParticlesDrawn = 0;

	if(!m_pMaterialSystem)
	{
		Assert(false);
		return;
	}

	// Update all the effects.
	UpdateAllEffects( flTimeDelta );
}

bool g_bMeasureParticlePerformance;
bool g_bDisplayParticlePerformance;

static int64 g_nNumParticlesSimulated;
static int64 g_nNumUSSpentSimulatingParticles;
static double g_flStartSimTime;

int GetParticlePerformance()
{
	if (! g_nNumUSSpentSimulatingParticles )
		return 0;
	return (1000*g_nNumParticlesSimulated) / g_nNumUSSpentSimulatingParticles;
}

void CParticleMgr::PostRender()
{
	VPROF("CParticleMgr::SimulateUndrawnEffects");

	// Simulate all effects that weren't drawn (if they have their 'always simulate' flag set).
	FOR_EACH_LL( m_Effects, i )
	{
		CParticleEffectBinding *pEffect = m_Effects[i];
		
		// Tell the effect if it was drawn or not.
		pEffect->SetWasDrawnPrevFrame( pEffect->WasDrawn() );

		// Now that we've rendered, clear this flag so it'll simulate next frame.
		pEffect->SetFlag( CParticleEffectBinding::FLAGS_FIRST_FRAME, false );	
	}
}


void CParticleMgr::DrawBeforeViewModelEffects()
{
	// NOTE (2012/11/27, TomF) - this whole system seems to be deprecated - nothing ever checks these flags, and CParticleMgr::DrawBeforeViewModelEffects is never called by anything!
	Assert ( !"Nothing ever calls CParticleMgr::DrawBeforeViewModelEffects any more!" );

	FOR_EACH_LL( m_Effects, i )
	{
		CParticleEffectBinding *pEffect = m_Effects[i];

		if ( pEffect->GetFlag( CParticleEffectBinding::FLAGS_DRAW_BEFORE_VIEW_MODEL ) )
		{
			Assert( !pEffect->WasDrawn() );
			pEffect->DrawModel( 1 );
		}
	}

}



void ResetParticlePerformanceCounters( void )
{
	g_nNumUSSpentSimulatingParticles = 0;
	g_nNumParticlesSimulated = 0;
}

void BeginSimulateParticles( void )
{
	g_flStartSimTime = Plat_FloatTime();
}


static ConVar r_particle_sim_spike_threshold_ms( "r_particle_sim_spike_threshold_ms", "5" );

void EndSimulateParticles( void )
{
	float flETime = Plat_FloatTime() - g_flStartSimTime;
	if ( g_bMeasureParticlePerformance )
	{
		g_nNumUSSpentSimulatingParticles += 1.0e6 * flETime;
	}
	g_pParticleSystemMgr->CommitProfileInformation( flETime > .001 * r_particle_sim_spike_threshold_ms.GetInt() );
}


static ConVar r_threaded_particles( "r_threaded_particles", "1" );

static float s_flThreadedPSystemTimeStep;

static void ProcessPSystem( ParticleSimListEntry_t& pSimListEntry )
{
	// Enable FP exceptions here when FP_EXCEPTIONS_ENABLED is defined,
	// to help track down bad math.
	FPExceptionEnabler enableExceptions;

	CNewParticleEffect* pNewEffect = pSimListEntry.m_pNewParticleEffect;
	bool updateBboxOnly = pSimListEntry.m_bBoundingBoxOnly;

	// If this is a new effect, then update its bbox so it goes in the
	// right leaves (if it has particles).
	int bFirstUpdate = pNewEffect->GetNeedsBBoxUpdate();
	if ( bFirstUpdate )
	{
		// If the effect already disabled auto-updating of the bbox, then it should have
		// set the bbox by now and we can ignore this responsibility here.
		if ( !pNewEffect->GetAutoUpdateBBox() || pNewEffect->RecalculateBoundingBox() )
		{
			pNewEffect->SetNeedsBBoxUpdate( false );
		}
	}

	// This flag will get set to true if the effect is drawn through the leaf system.
	pNewEffect->SetDrawn( false );

	if ( pNewEffect->GetFirstFrameFlag() )
	{
		pNewEffect->Simulate( 0.0f, updateBboxOnly );
		pNewEffect->SetFirstFrameFlag( false );
	}
	else if ( pNewEffect->ShouldSimulate() )
	{
		pNewEffect->Simulate( s_flThreadedPSystemTimeStep, updateBboxOnly );
	}

	if ( pNewEffect->IsFinished() )
	{
		pNewEffect->SetRemoveFlag();
	}
}


int CParticleMgr::ComputeParticleDefScreenArea( int nInfoCount, RetireInfo_t *pInfo, float *pTotalArea, CParticleSystemDefinition* pDef, 
	const CViewSetup& viewParticle, const VMatrix &worldToPixels, float flFocalDist )
{
	int nCollection = 0;
	float flCullCost = pDef->GetCullFillCost();
	float flCullRadius = pDef->GetCullRadius();
	float flCullRadiusSqr = flCullRadius * flCullRadius;
	*pTotalArea = 0.0f;

#ifdef DBGFLAG_ASSERT
	float flMaxPixels = viewParticle.width * viewParticle.height;
#endif

	CParticleCollection *pCollection = pDef->FirstCollection();
	for ( ; pCollection; pCollection = pCollection->GetNextCollectionUsingSameDef() )
	{
		CNewParticleEffect *pEffect = static_cast< CNewParticleEffect* >( pCollection );
		if ( !pEffect->ShouldPerformCullCheck() )
			continue;

		// Don't count parents
		Assert( !pCollection->m_pParent );
		Assert( nCollection < nInfoCount && pDef == pCollection->m_pDef );

		pInfo[nCollection].m_flScreenArea = 0.0f;
		pInfo[nCollection].m_pCollection = pCollection;
		pInfo[nCollection].m_bFirstFrame = false;

		Vector vecCenter, vecScreenCenter, vecCenterCam;
		vecCenter = pCollection->GetControlPointAtCurrentTime( pDef->GetCullControlPoint() );

		Vector3DMultiplyPositionProjective( worldToPixels, vecCenter, vecScreenCenter );
		float lSqr = vecCenter.DistToSqr( viewParticle.origin );

		float flProjRadius = ( lSqr > flCullRadiusSqr ) ? 0.5f * flFocalDist * flCullRadius / sqrt( lSqr - flCullRadiusSqr ) : 1.0f;
		flProjRadius *= viewParticle.width;

		float flMinX = MAX( viewParticle.x, vecScreenCenter.x - flProjRadius );
		float flMaxX = MIN( viewParticle.x + viewParticle.width, vecScreenCenter.x + flProjRadius );

		float flMinY = MAX( viewParticle.y, vecScreenCenter.y - flProjRadius );
		float flMaxY = MIN( viewParticle.y + viewParticle.height, vecScreenCenter.y + flProjRadius );

		float flArea = ( flMaxX - flMinX ) * ( flMaxY - flMinY );
		Assert( flArea <= flMaxPixels );
		flArea *= flCullCost;
		*pTotalArea += flArea; 

		pInfo[nCollection].m_flScreenArea = flArea;
		pInfo[nCollection].m_pCollection = pCollection;
		pInfo[nCollection].m_bFirstFrame = pEffect->GetFirstFrameFlag();
		++nCollection;
	}

	return nCollection;
}

int CParticleMgr::RetireSort( const void *p1, const void *p2 ) 
{
	RetireInfo_t *pRetire1 = (RetireInfo_t*)p1;
	RetireInfo_t *pRetire2 = (RetireInfo_t*)p2;
	float flArea = pRetire1->m_flScreenArea - pRetire2->m_flScreenArea;
	if ( flArea == 0.0f )
		return 0;
	return ( flArea > 0 ) ? -1 : 1;
}

bool CParticleMgr::RetireParticleCollections( CParticleSystemDefinition* pDef, 
	int nCount, RetireInfo_t *pInfo, float flScreenArea, float flMaxTotalArea )
{
	bool bRetirementOccurred = false;

	// Don't cull out the particle system if there's only 1 and no replacement
	const char *pReplacementDef = pDef->GetCullReplacementDefinition();
	if ( ( !pReplacementDef || !pReplacementDef[0] ) && ( nCount <= 1 ) )
		return false;

	// Quicksort the retirement info
	qsort( pInfo, nCount, sizeof(RetireInfo_t), RetireSort );

	for ( int i = 0; i < nCount; ++i )
	{
		if ( flScreenArea <= flMaxTotalArea )
			break;

		// We can only replace stuff that's being emitted this frame
		if ( !pInfo[i].m_bFirstFrame )
			continue;

		CNewParticleEffect* pRetireEffect = static_cast< CNewParticleEffect* >( pInfo[i].m_pCollection );
		CNewParticleEffect* pNewEffect = pRetireEffect->ReplaceWith( pReplacementDef );
		if ( pNewEffect )
		{
			pNewEffect->Update( s_flThreadedPSystemTimeStep );
		}
		bRetirementOccurred = true;
		flScreenArea -= pInfo[i].m_flScreenArea;
	}

	return bRetirementOccurred;
}

// Next, see if there are new particle systems that need early retirement
static ConVar cl_particle_retire_cost( "cl_particle_retire_cost", "0", FCVAR_CHEAT | FCVAR_ALLOWED_IN_COMPETITIVE );

bool CParticleMgr::EarlyRetireParticleSystems( int nCount, ParticleSimListEntry_t *ppEffects )
{
	// NOTE: Doing a cheap and hacky estimate of worst-case fillrate
	const CViewSetup *pViewSetup = view->GetPlayerViewSetup();
	if ( pViewSetup->width == 0 || pViewSetup->height == 0 )
		return false;

	float flMaxScreenArea = cl_particle_retire_cost.GetFloat() * 1000.0f;
	if ( flMaxScreenArea == 0.0f )
		return false;

	int nDefCount = 0;
	CParticleSystemDefinition **ppDefs = (CParticleSystemDefinition**)stackalloc( nCount * sizeof(CParticleSystemDefinition*) );
	for ( int i = 0; i < nCount; ++i )
	{
		CParticleSystemDefinition *pDef = ppEffects[i].m_pNewParticleEffect->m_pDef;

		// Skip stuff that doesn't have a cull radius set
		if ( pDef->GetCullRadius() == 0.0f )
			continue;

		// Only perform the cull check on creation
		if ( !ppEffects[i].m_pNewParticleEffect->GetFirstFrameFlag() )
			continue;

		if ( pDef->HasRetirementBeenChecked( gpGlobals->framecount ) )
			continue;

		pDef->MarkRetirementCheck( gpGlobals->framecount );

		ppDefs[nDefCount++] = ppEffects[i].m_pNewParticleEffect->m_pDef;
	}

	if ( nDefCount == 0 )
		return false;

	for ( int i = 0; i < nCount; ++i )
	{
		ppEffects[i].m_pNewParticleEffect->MarkShouldPerformCullCheck( true );
	}

	Vector vecCameraForward;
	VMatrix worldToView, viewToProjection, worldToProjection, worldToScreen;
	render->GetMatricesForView( *pViewSetup, &worldToView, &viewToProjection, &worldToProjection, &worldToScreen );
	float flFocalDist = tan( DEG2RAD( pViewSetup->fov * 0.5f ) );

	bool bRetiredCollections = true;
	float flScreenArea;
	int nSize = nCount * sizeof(RetireInfo_t);
	RetireInfo_t *pInfo = (RetireInfo_t*)stackalloc( nSize );
	for ( int i = 0; i < nDefCount; ++i )
	{
		CParticleSystemDefinition* pDef = ppDefs[i];
		int nActualCount = ComputeParticleDefScreenArea( nCount, pInfo, &flScreenArea, pDef, *pViewSetup, worldToScreen, flFocalDist );
		if ( flScreenArea > flMaxScreenArea )
		{
			if ( RetireParticleCollections( pDef, nActualCount, pInfo, flScreenArea, flMaxScreenArea ) )
			{
				bRetiredCollections = true;
			}
		}
	}

	for ( int i = 0; i < nCount; ++i )
	{
		ppEffects[i].m_pNewParticleEffect->MarkShouldPerformCullCheck( false );
	}
	return bRetiredCollections;
}

static ConVar particle_sim_alt_cores( "particle_sim_alt_cores", "2" );

void CParticleMgr::BuildParticleSimList( CUtlVector< ParticleSimListEntry_t > &list )
{
	float flNow = g_pParticleSystemMgr->GetLastSimulationTime();
	for( CNewParticleEffect *pNewEffect=m_NewEffects.m_pHead; pNewEffect;
		pNewEffect=pNewEffect->m_pNext )
	{
		bool bSkip = false;
		bool bNeedsBboxUpdate = false;

		if ( flNow >= pNewEffect->m_flNextSleepTime && pNewEffect->m_nActiveParticles > 0 )
			bSkip = true;
		if ( pNewEffect->GetRemoveFlag() )
			bSkip = true;

		if ( !bSkip && g_bMeasureParticlePerformance )
		{
			g_nNumParticlesSimulated += pNewEffect->m_nActiveParticles;
		}
		
		// Particles that are attached to moving things will need to update their bboxes even if they
		// otherwise would like to skip the updates. Check that here.
		if (bSkip)
		{
			bNeedsBboxUpdate = pNewEffect->HasMoved();
			bSkip = !bNeedsBboxUpdate;
		}

		if (!bSkip)
		{
			ParticleSimListEntry_t entry = { pNewEffect, bNeedsBboxUpdate };
			list.AddToTail( entry );
		}
	}
}

static ConVar r_particle_timescale( "r_particle_timescale", "1.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

static int CountChildParticleSystems( CParticleCollection *p )
{
	int nCount = 1;
	for ( CParticleCollection *pChild = p->m_Children.m_pHead; pChild; pChild = pChild->m_pNext )
	{
		nCount += CountChildParticleSystems( pChild );
	}
	return nCount;
}

static int CountParticleSystemActiveParticles( CParticleCollection *p )
{
	int nCount = p->m_nActiveParticles;
	for ( CParticleCollection *pChild = p->m_Children.m_pHead; pChild; pChild = pChild->m_pNext )
	{
		nCount += CountParticleSystemActiveParticles( pChild );
	}
	return nCount;
}


void CParticleMgr::UpdateNewEffects( float flTimeDelta )
{
// #ifdef TF_CLIENT_DLL
// 	extern bool g_bDontMakeSkipToTimeTakeForever;
// 	g_bDontMakeSkipToTimeTakeForever = true;
// #endif
	flTimeDelta *= r_particle_timescale.GetFloat();
	VPROF_BUDGET( "CParticleMSG::UpdateNewEffects", "Particle Simulation" );

	g_pParticleSystemMgr->SetLastSimulationTime( gpGlobals->curtime );

	int nParticleActiveParticlesCount = 0;
	int nParticleStatsTriggerCount = cl_particle_stats_trigger_count.GetInt();

	BeginSimulateParticles();
	s_flThreadedPSystemTimeStep = flTimeDelta;

	// first, run non-reentrant part to get CP updates from entities
	// This is done on all particles, because it updates control point locations which we need to determine whether or not we should 
	// do full simulation later. 
	for (CNewParticleEffect *pNewEffect = m_NewEffects.m_pHead; pNewEffect;
		pNewEffect = pNewEffect->m_pNext)
	{
		// this one can call into random entity code which may not be thread-safe
		pNewEffect->Update( s_flThreadedPSystemTimeStep );
		if ( nParticleStatsTriggerCount > 0 )
		{
			nParticleActiveParticlesCount += CountParticleSystemActiveParticles( pNewEffect );
		}
	}

	CUtlVector<ParticleSimListEntry_t> particlesToSimulate;
	BuildParticleSimList(particlesToSimulate);
	int nCount = particlesToSimulate.Count();


	// See if there are new particle systems that need early retirement
	// This has to happen after the first update
	if ( EarlyRetireParticleSystems( nCount, particlesToSimulate.Base() ) )
	{
		particlesToSimulate.RemoveAll();
		BuildParticleSimList( particlesToSimulate );
		nCount = particlesToSimulate.Count();
	}

	if ( nCount )
	{
		UpdateDirtySpatialPartitionEntities();
		if ( !r_threaded_particles.GetBool() )
		{
			for( int i=0; i<nCount; i++)
			{
				ProcessPSystem( particlesToSimulate[i] );
			}
		}
		else
		{
			int nAltCore = IsX360() && particle_sim_alt_cores.GetInt();
			if ( !m_pThreadPool[1] || nAltCore == 0 )
			{
				ParallelProcess( "CParticleMgr::UpdateNewEffects", particlesToSimulate.Base(), nCount, ProcessPSystem );
			}
			else
			{
				if ( nAltCore > 2 )
				{
					nAltCore = 2;
				}
				CParallelProcessor<ParticleSimListEntry_t, CFuncJobItemProcessor<ParticleSimListEntry_t> > processor( "CParticleMgr::UpdateNewEffects" );
				processor.m_ItemProcessor.Init( ProcessPSystem, NULL, NULL );
				processor.Run( particlesToSimulate.Base(), nCount, INT_MAX, m_pThreadPool[nAltCore-1] );
			}
		}
	}

	// now, run non-reentrant part for updating changes
	for( int i=0; i<nCount; i++)
	{
		// this one can call into random entity code which may not be thread-safe
		particlesToSimulate[i].m_pNewParticleEffect->DetectChanges();
	}

	EndSimulateParticles();

	if ( m_bStatsRunning )
	{
		StatsAccumulateActiveParticleSystems();
	}

	bool bFrameWarningNeeded = g_pParticleSystemMgr->Debug_FrameWarningNeededTestAndReset();

	m_nStatsFramesSinceLastAlert++;
	if ( ( nParticleStatsTriggerCount > 0 && ( nParticleActiveParticlesCount >= nParticleStatsTriggerCount ) ) || bFrameWarningNeeded )
	{
		if ( m_nStatsFramesSinceLastAlert >= ( 300 * 3 ) )		// 3 seconds at the clamp of 300 fps (or 15 secs at 60fps). Just stop it spamming too much.
		{
			m_nStatsFramesSinceLastAlert = 0;
			if ( m_bStatsRunning )
			{
				// Spew out the existing ones.
				StatsSpewResults();
			}
			// And turn the stats gathering on so we'll have more useful results if it does it again.
			m_bStatsRunning = true;

			// This single-frame capture doesn't work that well because the "actual drawn" numbers will just be zero. Ah well - better than nothing.
			StatsReset();
			StatsAccumulateActiveParticleSystems();
			StatsSpewResults();
			StatsReset();
		}
	}
}

void CParticleMgr::UpdateAllEffects( float flTimeDelta )
{
	// These reflect the convars so we don't parse the strings every particle.
	g_cl_particle_show_bbox = cl_particle_show_bbox.GetBool();
	g_cl_particle_show_bbox_cost = cl_particle_show_bbox_cost.GetInt();


	m_bUpdatingEffects = true;

	if( flTimeDelta > 0.1f )
		flTimeDelta = 0.1f;

	FOR_EACH_LL( m_Effects, iEffect )
	{
		CParticleEffectBinding *pEffect = m_Effects[iEffect];

		// Don't update this effect if it will be removed.
		if( pEffect->GetRemoveFlag() )
			continue;

		// If this is a new effect, then update its bbox so it goes in the
		// right leaves (if it has particles).
		int bFirstUpdate = pEffect->GetNeedsBBoxUpdate();
		if ( bFirstUpdate )
		{
			// If the effect already disabled auto-updating of the bbox, then it should have
			// set the bbox by now and we can ignore this responsibility here.
			if ( !pEffect->GetAutoUpdateBBox() || pEffect->RecalculateBoundingBox() )
			{
				pEffect->SetNeedsBBoxUpdate( false );
			}
		}

		// This flag will get set to true if the effect is drawn through the leaf system.
		pEffect->SetDrawn( false );

		// Update the effect.
		pEffect->m_pSim->Update( flTimeDelta );

		if ( pEffect->GetFirstFrameFlag() )
			pEffect->SetFirstFrameFlag( false );
		else
			pEffect->SimulateParticles( flTimeDelta );

		// Update its position in the leaf system if its bbox changed.
		pEffect->DetectChanges();
	}

	if ( g_bMeasureParticlePerformance )					// use fixed time step
	{
		for( float dt=0.0f; dt <= flTimeDelta ; dt+= 0.01f )
		{
			UpdateNewEffects( 0.01f );
		}
	}
	else
	{
		UpdateNewEffects( flTimeDelta );
	}

	m_bUpdatingEffects = false;

	// Remove any effects that were flagged to be removed.
	int iNext;
	for ( int i=m_Effects.Head(); i != m_Effects.InvalidIndex(); i=iNext )
	{
		iNext = m_Effects.Next( i );
		CParticleEffectBinding *pEffect = m_Effects[i];

		if( pEffect->GetRemoveFlag() )
		{
			RemoveEffect( pEffect );
		}
	}

	// Remove any of the new effects that were flagged to be removed.
	for( CNewParticleEffect *pNewEffect=m_NewEffects.m_pHead; pNewEffect;  )
	{
		CNewParticleEffect *pNextEffect = pNewEffect->m_pNext;
		if ( pNewEffect->GetRemoveFlag() )
		{
			RemoveEffect( pNewEffect );
		}

		pNewEffect = pNextEffect;
	}
}

CParticleSubTextureGroup* CParticleMgr::FindOrAddSubTextureGroup( IMaterial *pPageMaterial )
{
	for ( int i=0; i < m_SubTextureGroups.Count(); i++ )
	{
		if ( m_SubTextureGroups[i]->m_pPageMaterial == pPageMaterial )
			return m_SubTextureGroups[i];
	}

	CParticleSubTextureGroup *pGroup = new CParticleSubTextureGroup;
	m_SubTextureGroups.AddToTail( pGroup );
	pGroup->m_pPageMaterial = pPageMaterial;
	pPageMaterial->AddRef();

	return pGroup;
}


PMaterialHandle CParticleMgr::GetPMaterial( const char *pMaterialName )
{
	if( !m_pMaterialSystem )
	{
		Assert(false);
		return NULL;
	}

	int hMat = m_SubTextures.Find( pMaterialName );
	if ( hMat == m_SubTextures.InvalidIndex() )
	{
		IMaterial *pIMaterial = m_pMaterialSystem->FindMaterial( pMaterialName, TEXTURE_GROUP_PARTICLE );
		if ( pIMaterial )
		{
			pIMaterial->AddRef();

			CMatRenderContextPtr pRenderContext( m_pMaterialSystem );

			pRenderContext->Bind( pIMaterial, this );

			hMat = m_SubTextures.Insert( pMaterialName );
			CParticleSubTexture *pSubTexture = new CParticleSubTexture;
			m_SubTextures[hMat] = pSubTexture;
			pSubTexture->m_pMaterial = pIMaterial;

#ifdef _DEBUG
			int iNameLength = V_strlen( pMaterialName ) + 1;
			pSubTexture->m_szDebugName = new char [iNameLength];
			memcpy( pSubTexture->m_szDebugName, pMaterialName, iNameLength );
#endif

			// See if it's got a group name. If not, make a group with a special name.
			IMaterial *pPageMaterial = pIMaterial->GetMaterialPage();
			if ( pIMaterial->InMaterialPage() && pPageMaterial )
			{
				float flOffset[2], flScale[2];
				pIMaterial->GetMaterialOffset( flOffset );
				pIMaterial->GetMaterialScale( flScale );
				
				pSubTexture->m_tCoordMins[0] = (0*flScale[0] + flOffset[0]) * pPageMaterial->GetMappingWidth();
				pSubTexture->m_tCoordMaxs[0] = (1*flScale[0] + flOffset[0]) * pPageMaterial->GetMappingWidth();
				
				pSubTexture->m_tCoordMins[1] = (0*flScale[1] + flOffset[1]) * pPageMaterial->GetMappingHeight();
				pSubTexture->m_tCoordMaxs[1] = (1*flScale[1] + flOffset[1]) * pPageMaterial->GetMappingHeight();

				pSubTexture->m_pGroup = FindOrAddSubTextureGroup( pPageMaterial );
			}
			else
			{
				// Ok, this material isn't part of a group. Give it its own subtexture group.
				pSubTexture->m_pGroup = &pSubTexture->m_DefaultGroup;
				pSubTexture->m_DefaultGroup.m_pPageMaterial = pIMaterial;
				pPageMaterial = pIMaterial; // For tcoord scaling.
				
				pSubTexture->m_tCoordMins[0] = pSubTexture->m_tCoordMins[1] = 0;
				pSubTexture->m_tCoordMaxs[0] = pIMaterial->GetMappingWidth();
				pSubTexture->m_tCoordMaxs[1] = pIMaterial->GetMappingHeight();
			}

			// Rescale the texture coordinates.
			pSubTexture->m_tCoordMins[0] = (pSubTexture->m_tCoordMins[0] + 0.5f) / pPageMaterial->GetMappingWidth();
			pSubTexture->m_tCoordMins[1] = (pSubTexture->m_tCoordMins[1] + 0.5f) / pPageMaterial->GetMappingHeight();
			pSubTexture->m_tCoordMaxs[0] = (pSubTexture->m_tCoordMaxs[0] - 0.5f) / pPageMaterial->GetMappingWidth();
			pSubTexture->m_tCoordMaxs[1] = (pSubTexture->m_tCoordMaxs[1] - 0.5f) / pPageMaterial->GetMappingHeight();
		
			return pSubTexture;
		}
		else
		{
			return NULL;
		} 
	}
	else
	{
		RepairPMaterial( m_SubTextures[hMat] ); //HACKHACK: Remove this when we can stop leaking handles from level to level.

		return m_SubTextures[hMat];
	}
}


//HACKHACK: The old system would leak handles and materials until shutdown. The new system still needs to leak handles until every piece of code that grabs one ditches it at level end.
//This function takes a leaked handle from a previous level and reacquires necessary materials.
void CParticleMgr::RepairPMaterial( PMaterialHandle hMaterial )
{
	if( hMaterial->m_pMaterial != NULL )
		return;

	const char *pMaterialName = NULL;
	for( int i = m_SubTextures.First(); i != m_SubTextures.InvalidIndex(); i = m_SubTextures.Next( i ) )
	{
		if( m_SubTextures[i] == hMaterial )
		{
			pMaterialName = m_SubTextures.GetElementName( i );
			break;
		}
	}
	Assert( pMaterialName != NULL );

	IMaterial *pIMaterial = m_pMaterialSystem->FindMaterial( pMaterialName, TEXTURE_GROUP_PARTICLE );
	hMaterial->m_pMaterial = pIMaterial;
	if ( pIMaterial != NULL )
	{
		pIMaterial->AddRef();
		CMatRenderContextPtr pRenderContext( m_pMaterialSystem );
		pRenderContext->Bind( pIMaterial, this );

		IMaterial *pPageMaterial = pIMaterial->GetMaterialPage();
		if ( pIMaterial->InMaterialPage() && pPageMaterial )
		{
			if ( hMaterial->m_pGroup->m_pPageMaterial == NULL )
			{
				hMaterial->m_pGroup->m_pPageMaterial = pPageMaterial;
				pPageMaterial->AddRef();
			}
		}
		else
		{
			hMaterial->m_pGroup->m_pPageMaterial = pIMaterial;
		}
	}
}


IMaterial* CParticleMgr::PMaterialToIMaterial( PMaterialHandle hMaterial )
{
	if ( hMaterial )
	{
		RepairPMaterial( hMaterial ); //HACKHACK: Remove this when we can stop leaking handles from level to level.
		return hMaterial->m_pMaterial;
	}
	else
		return NULL;
}


void CParticleMgr::GetDirectionalLightInfo( CParticleLightInfo &info ) const
{
	info = m_DirectionalLight;
}


void CParticleMgr::SetDirectionalLightInfo( const CParticleLightInfo &info )
{
	m_DirectionalLight = info;
}

// ------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------ //
float Helper_GetTime()
{
#if defined( PARTICLEPROTOTYPE_APP )
	static bool bStarted = false;
	static CCycleCount startTimer;
	if( !bStarted )
	{
		bStarted = true;
		startTimer.Sample();
	}

	CCycleCount curCount;
	curCount.Sample();

	CCycleCount elapsed;
	CCycleCount::Sub( curCount, startTimer, elapsed );

	return (float)elapsed.GetSeconds();
#else
	return gpGlobals->curtime;
#endif
}


float Helper_RandomFloat( float minVal, float maxVal )
{
#if defined( PARTICLEPROTOTYPE_APP )
	return Lerp( (float)rand() / RAND_MAX, minVal, maxVal );
#else
	return random->RandomFloat( minVal, maxVal );
#endif
}


int Helper_RandomInt( int minVal, int maxVal )
{
#if defined( PARTICLEPROTOTYPE_APP )
	return minVal + (rand() * (maxVal - minVal)) / RAND_MAX;
#else
	return random->RandomInt( minVal, maxVal );
#endif
}


float Helper_GetFrameTime()
{
#if defined( PARTICLEPROTOTYPE_APP )
	extern float g_ParticleAppFrameTime;
	return g_ParticleAppFrameTime;
#else
	return gpGlobals->frametime;
#endif
}





// ------------------------------------------------------------------------------------ //
// Stats-gathering stuff.
// ------------------------------------------------------------------------------------ //

static void StatsParticlesStart()
{
}

static void StatsParticlesStop()
{
}


struct ParticleInfo_t
{
	ParticleInfo_t() : m_nCount(0), m_nChildCount(0), m_nTotalActiveParticles(0), m_nTotalDrawnParticles(0), m_nCountMax(0), m_nChildCountMax(0), m_nTotalActiveParticlesMax(0), m_nTotalDrawnParticlesMax(0), pDef(NULL) {}
	int m_nCount;
	int m_nChildCount;
	int m_nTotalActiveParticles;
	int m_nTotalDrawnParticles;

	// These are only used for the multi-frame stats.
	int m_nCountMax;
	int m_nChildCountMax;
	int m_nTotalActiveParticlesMax;
	int m_nTotalDrawnParticlesMax;

	CParticleSystemDefinition *pDef;
};

CUtlStringMap< ParticleInfo_t > ProfilingHistogram;
CUtlStringMap< ParticleInfo_t > SingleFrameHistogram;
int Profiling_nFrames;
int Profiling_nMaxParticles;


// These functions will be called by the particles as they're actually drawn. (TODO: thread safety?)
void CParticleMgr::StatsNewParticleEffectDrawn ( CNewParticleEffect *pParticles )
{
}

void CParticleMgr::StatsOldParticleEffectDrawn ( CParticleEffectBinding *pParticles )
{
}

void CParticleMgr::StatsAccumulateActiveParticleSystems()
{
}

void CParticleMgr::StatsReset()
{
}

void CParticleMgr::StatsSpewResults()
{
}




