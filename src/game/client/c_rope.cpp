//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=====================================================================================//
#include "cbase.h"
#include "c_rope.h"
#include "beamdraw.h"
#include "view.h"
#include "env_wind_shared.h"
#include "input.h"
#ifdef TF_CLIENT_DLL
#include "cdll_util.h"
#include "tf_gamerules.h"
#endif
#include "rope_helpers.h"
#include "engine/ivmodelinfo.h"
#include "tier0/vprof.h"
#include "c_te_effect_dispatch.h"
#include "collisionutils.h"
#include <KeyValues.h>
#include <bitbuf.h>
#include "utllinkedlist.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "tier1/callqueue.h"
#include "tier1/memstack.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void RecvProxy_RecomputeSprings( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// Have the regular proxy store the data.
	RecvProxy_Int32ToInt32( pData, pStruct, pOut );

	C_RopeKeyframe *pRope = (C_RopeKeyframe*)pStruct;
	pRope->RecomputeSprings();
}


IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_RopeKeyframe, DT_RopeKeyframe, CRopeKeyframe )
	RecvPropInt( RECVINFO(m_iRopeMaterialModelIndex) ),
	RecvPropEHandle( RECVINFO(m_hStartPoint) ),
	RecvPropEHandle( RECVINFO(m_hEndPoint) ),
	RecvPropInt( RECVINFO(m_iStartAttachment) ),
	RecvPropInt( RECVINFO(m_iEndAttachment) ),

	RecvPropInt( RECVINFO(m_fLockedPoints) ),
	RecvPropInt( RECVINFO(m_Slack), 0, RecvProxy_RecomputeSprings ),
	RecvPropInt( RECVINFO(m_RopeLength), 0, RecvProxy_RecomputeSprings ),
	RecvPropInt( RECVINFO(m_RopeFlags) ),
	RecvPropFloat( RECVINFO(m_TextureScale) ),
	RecvPropInt( RECVINFO(m_nSegments) ),
	RecvPropBool( RECVINFO(m_bConstrainBetweenEndpoints) ),
	RecvPropInt( RECVINFO(m_Subdiv) ),

	RecvPropFloat( RECVINFO(m_Width) ),
	RecvPropFloat( RECVINFO(m_flScrollSpeed) ),
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropInt( RECVINFO_NAME(m_hNetworkMoveParent, moveparent), 0, RecvProxy_IntToMoveParent ),
	
	RecvPropInt( RECVINFO( m_iParentAttachment ) ),
END_RECV_TABLE()

#define ROPE_IMPULSE_SCALE	20
#define ROPE_IMPULSE_DECAY	0.95

static ConVar rope_shake( "rope_shake", "0" );
static ConVar rope_subdiv( "rope_subdiv", "2", 0, "Rope subdivision amount", true, 0, true, MAX_ROPE_SUBDIVS );
static ConVar rope_collide( "rope_collide", "1", 0, "Collide rope with the world" );

static ConVar rope_smooth( "rope_smooth", "1", 0, "Do an antialiasing effect on ropes" );
static ConVar rope_smooth_enlarge( "rope_smooth_enlarge", "1.4", 0, "How much to enlarge ropes in screen space for antialiasing effect" );

static ConVar rope_smooth_minwidth( "rope_smooth_minwidth", "0.3", 0, "When using smoothing, this is the min screenspace width it lets a rope shrink to" );
static ConVar rope_smooth_minalpha( "rope_smooth_minalpha", "0.2", 0, "Alpha for rope antialiasing effect" );

static ConVar rope_smooth_maxalphawidth( "rope_smooth_maxalphawidth", "1.75" );
static ConVar rope_smooth_maxalpha( "rope_smooth_maxalpha", "0.5", 0, "Alpha for rope antialiasing effect" );

static ConVar mat_fullbright( "mat_fullbright", "0", FCVAR_CHEAT ); // get it from the engine
static ConVar r_drawropes( "r_drawropes", "1", FCVAR_CHEAT );
static ConVar r_queued_ropes( "r_queued_ropes", "1" );
static ConVar r_ropetranslucent( "r_ropetranslucent", "1");
static ConVar r_rope_holiday_light_scale( "r_rope_holiday_light_scale", "0.055", FCVAR_DEVELOPMENTONLY );
static ConVar r_ropes_holiday_lights_allowed( "r_ropes_holiday_lights_allowed", "1", FCVAR_DEVELOPMENTONLY );

static ConVar rope_wind_dist( "rope_wind_dist", "1000", 0, "Don't use CPU applying small wind gusts to ropes when they're past this distance." );
static ConVar rope_averagelight( "rope_averagelight", "1", 0, "Makes ropes use average of cubemap lighting instead of max intensity." );


static ConVar rope_rendersolid( "rope_rendersolid", "1" );

static ConVar rope_solid_minwidth( "rope_solid_minwidth", "0.3" );
static ConVar rope_solid_maxwidth( "rope_solid_maxwidth", "1" );

static ConVar rope_solid_minalpha( "rope_solid_minalpha", "0.0" );
static ConVar rope_solid_maxalpha( "rope_solid_maxalpha", "1" );


static CCycleCount	g_RopeCollideTicks;
static CCycleCount	g_RopeDrawTicks;
static CCycleCount	g_RopeSimulateTicks;
static int			g_nRopePointsSimulated;

// Active ropes.
CUtlLinkedList<C_RopeKeyframe*, int> g_Ropes;


static Vector	g_FullBright_LightValues[ROPE_MAX_SEGMENTS];
class CFullBrightLightValuesInit
{
public:
	CFullBrightLightValuesInit()
	{
		for( int i=0; i < ROPE_MAX_SEGMENTS; i++ )
			g_FullBright_LightValues[i].Init( 1, 1, 1 );
	}
} g_FullBrightLightValuesInit;

// Precalculated info for rope subdivision.
static Vector	g_RopeSubdivs[MAX_ROPE_SUBDIVS][MAX_ROPE_SUBDIVS];
class CSubdivInit
{
public:
	CSubdivInit()
	{
		for ( int iSubdiv=0; iSubdiv < MAX_ROPE_SUBDIVS; iSubdiv++ )
		{
			for( int i=0; i <= iSubdiv; i++ )
			{
				float t = (float)(i+1) / (iSubdiv+1);
				g_RopeSubdivs[iSubdiv][i].Init( t, t*t, t*t*t );
			}
		}
	}
} g_SubdivInit;

//interesting barbed-wire-looking effect
static int		g_nBarbedSubdivs = 3;
static Vector	g_BarbedSubdivs[MAX_ROPE_SUBDIVS] = {	Vector(1.5,		1.5*1.5,		1.5*1.5*1.5),
														Vector(-0.5,	-0.5 * -0.5,	-0.5*-0.5*-0.5),
														Vector(0.5,		0.5*0.5,		0.5*0.5*0.5) };

// This can be exposed through the entity if we ever care.
static float g_flLockAmount = 0.1;
static float g_flLockFalloff = 0.3;




class CQueuedRopeMemoryManager
{
public:
	CQueuedRopeMemoryManager( void )
	{
		m_nCurrentStack = 0;
		MEM_ALLOC_CREDIT();
		m_QueuedRopeMemory[0].Init( 131072, 0, 16384 );
		m_QueuedRopeMemory[1].Init( 131072, 0, 16384 );
	}
	~CQueuedRopeMemoryManager( void )
	{
		m_QueuedRopeMemory[0].FreeAll( true );
		m_QueuedRopeMemory[1].FreeAll( true );
		for( int i = 0; i != 2; ++i )
		{
			for( int j = m_DeleteOnSwitch[i].Count(); --j >= 0; )
			{
				free( m_DeleteOnSwitch[i].Element(j) );
			}

			m_DeleteOnSwitch[i].RemoveAll();
		}
	}

	void SwitchStack( void )
	{
		m_nCurrentStack = 1 - m_nCurrentStack;
		m_QueuedRopeMemory[m_nCurrentStack].FreeAll( false );

		for( int i = m_DeleteOnSwitch[m_nCurrentStack].Count(); --i >= 0; )
		{
			free( m_DeleteOnSwitch[m_nCurrentStack].Element(i) );
		}
		m_DeleteOnSwitch[m_nCurrentStack].RemoveAll();
	}

	inline void *Alloc( size_t bytes )
	{
		MEM_ALLOC_CREDIT();
		void *pReturn = m_QueuedRopeMemory[m_nCurrentStack].Alloc( bytes, false );
		if( pReturn == NULL )
		{
			int iMaxSize = m_QueuedRopeMemory[m_nCurrentStack].GetMaxSize();
			Warning( "Overflowed rope queued rendering memory stack. Needed %llu, have %d/%d\n", (uint64)bytes, iMaxSize - m_QueuedRopeMemory[m_nCurrentStack].GetUsed(), iMaxSize );
			pReturn = malloc( bytes );
			m_DeleteOnSwitch[m_nCurrentStack].AddToTail( pReturn );
		}
		return pReturn;
	}

	CMemoryStack	m_QueuedRopeMemory[2];
	int				m_nCurrentStack;
	CUtlVector<void *>	m_DeleteOnSwitch[2]; //when we overflow the stack, we do new/delete
};

//=============================================================================
//
// Rope mananger.
//
struct RopeSegData_t
{
	int			m_nSegmentCount;
	BeamSeg_t	m_Segments[MAX_ROPE_SEGMENTS];
	float		m_BackWidths[MAX_ROPE_SEGMENTS];

	// If this is less than rope_solid_minwidth and rope_solid_minalpha is 0, then we can avoid drawing..
	float		m_flMaxBackWidth;
};

class CRopeManager : public IRopeManager
{
public:

	CRopeManager();
	~CRopeManager();

	void ResetRenderCache( void );
	void AddToRenderCache( C_RopeKeyframe *pRope );
	void DrawRenderCache( bool bShadowDepth );
	void OnRenderStart( void )
	{
		m_QueuedModeMemory.SwitchStack();
	}

	void SetHolidayLightMode( bool bHoliday ) { m_bDrawHolidayLights = bHoliday; }
	bool IsHolidayLightMode( void );
	int GetHolidayLightStyle( void );
	
private:
	struct RopeRenderData_t;
public:
	void DrawRenderCache_NonQueued( bool bShadowDepth, RopeRenderData_t *pRenderCache, int nRenderCacheCount, const Vector &vCurrentViewForward, const Vector &vCurrentViewOrigin, C_RopeKeyframe::BuildRopeQueuedData_t *pBuildRopeQueuedData );
	
	void			ResetSegmentCache( int nMaxSegments );
	RopeSegData_t	*GetNextSegmentFromCache( void );

	enum { MAX_ROPE_RENDERCACHE	= 128 };

	void RemoveRopeFromQueuedRenderCaches( C_RopeKeyframe *pRope );
	
private:

	void RenderNonSolidRopes( IMatRenderContext *pRenderContext, IMaterial *pMaterial, int nVertCount, int nIndexCount );
	void RenderSolidRopes( IMatRenderContext *pRenderContext, IMaterial *pMaterial, int nVertCount, int nIndexCount, bool bRenderNonSolid );

private:

	struct RopeRenderData_t
	{
		IMaterial			*m_pSolidMaterial;
		IMaterial			*m_pBackMaterial;
		int					m_nCacheCount;
		C_RopeKeyframe		*m_aCache[MAX_ROPE_RENDERCACHE];
	};

	CUtlVector<RopeRenderData_t>	m_aRenderCache;
	int								m_nSegmentCacheCount;
	CUtlVector<RopeSegData_t>		m_aSegmentCache;
	CThreadFastMutex				m_RenderCacheMutex; //there's only any contention during the switch from r_queued_ropes on to off
	
	//in queued material system mode we need to store off data for later use.
	CQueuedRopeMemoryManager		m_QueuedModeMemory;

	IMaterial* m_pDepthWriteMaterial;


	struct RopeQueuedRenderCache_t
	{
		RopeRenderData_t *pCaches;
		int iCacheCount;
		RopeQueuedRenderCache_t( void ) : pCaches(NULL), iCacheCount(0) { };
	};

	CUtlLinkedList<RopeQueuedRenderCache_t> m_RopeQueuedRenderCaches;

	bool m_bDrawHolidayLights;
	bool m_bHolidayInitialized;
	int m_nHolidayLightsStyle;
};

static CRopeManager s_RopeManager;

IRopeManager *RopeManager()
{
	return &s_RopeManager;
}


inline bool ShouldUseFakeAA( IMaterial *pBackMaterial )
{
	return pBackMaterial && rope_smooth.GetInt() && engine->GetDXSupportLevel() > 70 && !g_pMaterialSystemHardwareConfig->IsAAEnabled();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CRopeManager::CRopeManager()
{
	m_aRenderCache.Purge();
	m_aSegmentCache.Purge();
	m_nSegmentCacheCount = 0;
	m_pDepthWriteMaterial = NULL;
	m_bDrawHolidayLights = false;
	m_bHolidayInitialized = false;
	m_nHolidayLightsStyle = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CRopeManager::~CRopeManager()
{
	int nRenderCacheCount = m_aRenderCache.Count();
	for ( int iRenderCache = 0; iRenderCache < nRenderCacheCount; ++iRenderCache )
	{
		if ( m_aRenderCache[iRenderCache].m_pSolidMaterial )
		{
			m_aRenderCache[iRenderCache].m_pSolidMaterial->DecrementReferenceCount();
		}
		if ( m_aRenderCache[iRenderCache].m_pBackMaterial )
		{
			m_aRenderCache[iRenderCache].m_pBackMaterial->DecrementReferenceCount();
		}
	}

	m_aRenderCache.Purge();
	m_aSegmentCache.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRopeManager::ResetRenderCache( void )
{
	int nRenderCacheCount = m_aRenderCache.Count();
	for ( int iRenderCache = 0; iRenderCache < nRenderCacheCount; ++iRenderCache )
	{
		m_aRenderCache[iRenderCache].m_nCacheCount = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRopeManager::AddToRenderCache( C_RopeKeyframe *pRope )
{
	if( !pRope->GetSolidMaterial() )
	{
		return;
	}
	
	// Find the current rope list.
	int iRenderCache = 0;
	int nRenderCacheCount = m_aRenderCache.Count();
	for ( ; iRenderCache < nRenderCacheCount; ++iRenderCache )
	{
		if ( ( pRope->GetSolidMaterial() == m_aRenderCache[iRenderCache].m_pSolidMaterial ) &&
			 ( pRope->GetBackMaterial() == m_aRenderCache[iRenderCache].m_pBackMaterial ) )
			break;
	}

	// A full rope list should have been generate in CreateRenderCache
	// If we didn't find one, then allocate the mofo.
	if ( iRenderCache == nRenderCacheCount )
	{
		int iRenderCache = m_aRenderCache.AddToTail();
		m_aRenderCache[iRenderCache].m_pSolidMaterial = pRope->GetSolidMaterial();
		if ( m_aRenderCache[iRenderCache].m_pSolidMaterial )
		{
			m_aRenderCache[iRenderCache].m_pSolidMaterial->IncrementReferenceCount();
		}
		m_aRenderCache[iRenderCache].m_pBackMaterial = pRope->GetBackMaterial();
		if ( m_aRenderCache[iRenderCache].m_pBackMaterial )
		{
			m_aRenderCache[iRenderCache].m_pBackMaterial->IncrementReferenceCount();
		}
		m_aRenderCache[iRenderCache].m_nCacheCount = 0;
	}

	if ( m_aRenderCache[iRenderCache].m_nCacheCount >= MAX_ROPE_RENDERCACHE )
	{
		Warning( "CRopeManager::AddToRenderCache count to large for cache!\n" );
		return;
	}

	m_aRenderCache[iRenderCache].m_aCache[m_aRenderCache[iRenderCache].m_nCacheCount] = pRope;
	++m_aRenderCache[iRenderCache].m_nCacheCount;
}

void CRopeManager::DrawRenderCache_NonQueued( bool bShadowDepth, RopeRenderData_t *pRenderCache, int nRenderCacheCount, const Vector &vCurrentViewForward, const Vector &vCurrentViewOrigin, C_RopeKeyframe::BuildRopeQueuedData_t *pBuildRopeQueuedData )
{
	VPROF_BUDGET( "CRopeManager::DrawRenderCache", VPROF_BUDGETGROUP_ROPES );
	AUTO_LOCK( m_RenderCacheMutex ); //contention cases: Toggling from queued mode on to off. Rope deletion from the cache.

	// Check to see if we want to render the ropes.
	if( !r_drawropes.GetBool() )
	{
		if( pBuildRopeQueuedData && (m_RopeQueuedRenderCaches.Count() != 0) )
		{
			m_RopeQueuedRenderCaches.Remove( m_RopeQueuedRenderCaches.Head() );
		}

		return;
	}

	if ( bShadowDepth && !m_pDepthWriteMaterial && g_pMaterialSystem )
	{
		KeyValues *pVMTKeyValues = new KeyValues( "DepthWrite" );
		pVMTKeyValues->SetInt( "$no_fullbright", 1 );
		pVMTKeyValues->SetInt( "$alphatest", 0 );
		pVMTKeyValues->SetInt( "$nocull", 1 );
		m_pDepthWriteMaterial = g_pMaterialSystem->FindProceduralMaterial( "__DepthWrite01", TEXTURE_GROUP_OTHER, pVMTKeyValues );
		m_pDepthWriteMaterial->IncrementReferenceCount();
	}

	CMatRenderContextPtr pRenderContext( materials );

	C_RopeKeyframe::BuildRopeQueuedData_t stackQueuedData;
	Vector vStackPredictedPositions[MAX_ROPE_SEGMENTS];	
	
	for ( int iRenderCache = 0; iRenderCache < nRenderCacheCount; ++iRenderCache )
	{
		int nCacheCount = pRenderCache[iRenderCache].m_nCacheCount;

		if ( nCacheCount == 0 )
			continue;		

		ResetSegmentCache( nCacheCount );

		for ( int iCache = 0; iCache < nCacheCount; ++iCache )
		{
			C_RopeKeyframe *pRope = pRenderCache[iRenderCache].m_aCache[iCache];
			if ( pRope )
			{
				RopeSegData_t *pRopeSegment = GetNextSegmentFromCache();
				
				if( pBuildRopeQueuedData )
				{
					pRope->BuildRope( pRopeSegment, vCurrentViewForward, vCurrentViewOrigin, pBuildRopeQueuedData, true );
					++pBuildRopeQueuedData;
				}
				else
				{
					//to unify the BuildRope code, emulate the queued data
					stackQueuedData.m_iNodeCount = pRope->m_RopePhysics.NumNodes();
					stackQueuedData.m_pLightValues = pRope->m_LightValues;
					stackQueuedData.m_vColorMod = pRope->m_vColorMod;
					stackQueuedData.m_pPredictedPositions = vStackPredictedPositions;
					stackQueuedData.m_RopeLength = pRope->m_RopeLength;
					stackQueuedData.m_Slack = pRope->m_Slack;

					for( int i = 0; i != stackQueuedData.m_iNodeCount; ++i )
					{
						vStackPredictedPositions[i] = pRope->m_RopePhysics.GetNode( i )->m_vPredicted;
					}
					
					pRope->BuildRope( pRopeSegment, vCurrentViewForward, vCurrentViewOrigin, &stackQueuedData, false );
				}
			}
			else
			{
				if( pBuildRopeQueuedData )
				{
					//we should only be here if a rope was in the queue and then deleted. We still have it's relevant data (and need to skip over it).
					++pBuildRopeQueuedData;
				}
			}
		}

		if ( materials->GetRenderContext()->GetCallQueue() != NULL && pBuildRopeQueuedData == NULL )
		{
			// We build ropes outside of queued mode for holidy lights
			// But we don't want to render them
			continue;
		}

		int nVertCount = 0;
		int nIndexCount = 0;
		for ( int iSegmentCache = 0; iSegmentCache < m_nSegmentCacheCount; ++iSegmentCache )
		{
			nVertCount += ( m_aSegmentCache[iSegmentCache].m_nSegmentCount * 2 );
			nIndexCount += ( ( m_aSegmentCache[iSegmentCache].m_nSegmentCount - 1 )  * 6 );
		}

		// Render the non-solid portion of the ropes.
		bool bRenderNonSolid = !bShadowDepth && ShouldUseFakeAA( pRenderCache[iRenderCache].m_pBackMaterial );
		if ( bRenderNonSolid )
		{
			RenderNonSolidRopes( pRenderContext, pRenderCache[iRenderCache].m_pBackMaterial, nVertCount, nIndexCount );
		}

		// Render the solid portion of the ropes.
		if ( rope_rendersolid.GetInt() )
		{
			if ( bShadowDepth )
				RenderSolidRopes( pRenderContext, m_pDepthWriteMaterial, nVertCount, nIndexCount, bRenderNonSolid );
			else
				RenderSolidRopes( pRenderContext, pRenderCache[iRenderCache].m_pSolidMaterial, nVertCount, nIndexCount, bRenderNonSolid );
		}
	}
	ResetSegmentCache( 0 );

	if( pBuildRopeQueuedData && (m_RopeQueuedRenderCaches.Count() != 0) )
	{
		m_RopeQueuedRenderCaches.Remove( m_RopeQueuedRenderCaches.Head() );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRopeManager::DrawRenderCache( bool bShadowDepth )
{
	int iRenderCacheCount = m_aRenderCache.Count();

	if( iRenderCacheCount == 0 )
		return;

	Vector vForward = CurrentViewForward();
	Vector vOrigin = CurrentViewOrigin();

	ICallQueue *pCallQueue;
	if( r_queued_ropes.GetBool() && (pCallQueue = materials->GetRenderContext()->GetCallQueue()) != NULL )
	{
		//material queue available and desired
		CRopeManager::RopeRenderData_t *pRenderCache = m_aRenderCache.Base();
		AUTO_LOCK( m_RenderCacheMutex );
		
		int iRopeCount = 0;
		int iNodeCount = 0;
		for( int i = 0; i != iRenderCacheCount; ++i )
		{
			CRopeManager::RopeRenderData_t *pCache = &pRenderCache[i];
			int iCacheCount = pCache->m_nCacheCount;
			iRopeCount += iCacheCount;
			for( int j = 0; j != iCacheCount; ++j )
			{
				C_RopeKeyframe *pRope = pCache->m_aCache[j];
				if( pRope )
					iNodeCount += pRope->m_RopePhysics.NumNodes();
				else
					--iRopeCount;
			}
		}

		if( iRopeCount == 0 )
			return; //nothing to draw

		size_t iMemoryNeeded = (iRenderCacheCount * sizeof(CRopeManager::RopeRenderData_t)) + 
								(iRopeCount * sizeof(C_RopeKeyframe::BuildRopeQueuedData_t)) +
								(iNodeCount * (sizeof(Vector) * 2));

		void *pMemory = m_QueuedModeMemory.Alloc( iMemoryNeeded );

		CRopeManager::RopeRenderData_t *pRenderCachesStart = (CRopeManager::RopeRenderData_t *)pMemory;
		C_RopeKeyframe::BuildRopeQueuedData_t *pBuildRopeQueuedDataStart = (C_RopeKeyframe::BuildRopeQueuedData_t *)(pRenderCachesStart + iRenderCacheCount);
		Vector *pVectorDataStart = (Vector *)(pBuildRopeQueuedDataStart + iRopeCount);
		
		//memcpy( pRenderCachesStart, m_aRenderCache.Base(), iRenderCacheCount * sizeof( CRopeManager::RopeRenderData_t ) );

		RopeQueuedRenderCache_t cache;
		cache.pCaches = pRenderCachesStart;
		cache.iCacheCount = iRenderCacheCount;
		m_RopeQueuedRenderCaches.AddToTail( cache );
		
		C_RopeKeyframe::BuildRopeQueuedData_t *pWriteRopeQueuedData = pBuildRopeQueuedDataStart;
		Vector *pVectorWrite = (Vector *)pVectorDataStart;

		//Setup the rest of our data. This writes to two separate areas of memory at the same time. One area for the C_RopeKeyframe::BuildRopeQueuedData_t array, the other for mini-arrays of vector data
		for( int i = 0; i != iRenderCacheCount; ++i )
		{
			CRopeManager::RopeRenderData_t *pReadCache = &pRenderCache[i];
			CRopeManager::RopeRenderData_t *pWriteCache = &pRenderCachesStart[i];
			int iCacheCount = pReadCache->m_nCacheCount;
			pWriteCache->m_nCacheCount = 0;
			pWriteCache->m_pSolidMaterial = pReadCache->m_pSolidMaterial;
			pWriteCache->m_pBackMaterial = pReadCache->m_pBackMaterial;
			for( int j = 0; j != iCacheCount; ++j )
			{
				C_RopeKeyframe *pRope = pReadCache->m_aCache[j];
				if( pRope == NULL )
					continue;

				pWriteCache->m_aCache[pWriteCache->m_nCacheCount] = pRope;
				++pWriteCache->m_nCacheCount;

				int iNodes = pRope->m_RopePhysics.NumNodes();

				//setup the C_RopeKeyframe::BuildRopeQueuedData_t struct
				pWriteRopeQueuedData->m_iNodeCount = pRope->m_RopePhysics.NumNodes();
				pWriteRopeQueuedData->m_vColorMod = pRope->m_vColorMod;
				pWriteRopeQueuedData->m_RopeLength = pRope->m_RopeLength;
				pWriteRopeQueuedData->m_Slack = pRope->m_Slack;
				pWriteRopeQueuedData->m_pPredictedPositions = pVectorWrite;
				pWriteRopeQueuedData->m_pLightValues = pVectorWrite + iNodes;
				++pWriteRopeQueuedData;

				//make two arrays, one of predicted positions followed immediately by light values
				for( int k = 0; k != iNodes; ++k )
				{					
					pVectorWrite[0] = pRope->m_RopePhysics.GetNode( k )->m_vPredicted;
					pVectorWrite[iNodes] = pRope->m_LightValues[k];
					++pVectorWrite;
				}
				pVectorWrite += iNodes; //so we don't overwrite the light values with the next rope's predicted positions
			}
		}
		Assert( ((void *)pVectorWrite == (void *)(((uint8 *)pMemory) + iMemoryNeeded)) && ((void *)pWriteRopeQueuedData == (void *)pVectorDataStart));		
		pCallQueue->QueueCall( this, &CRopeManager::DrawRenderCache_NonQueued, bShadowDepth, pRenderCachesStart, iRenderCacheCount, vForward, vOrigin, pBuildRopeQueuedDataStart );

		if ( IsHolidayLightMode() )
		{
			// With holiday lights we need to also build the ropes non-queued without rendering them
			DrawRenderCache_NonQueued( bShadowDepth, m_aRenderCache.Base(), iRenderCacheCount, vForward, vOrigin, NULL );
		}
	}
	else
	{
		DrawRenderCache_NonQueued( bShadowDepth, m_aRenderCache.Base(), iRenderCacheCount, vForward, vOrigin, NULL );
	}
}

bool CRopeManager::IsHolidayLightMode( void )
{
	if ( !r_ropes_holiday_lights_allowed.GetBool() )
	{
		return false;
	}

#ifdef TF_CLIENT_DLL
	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		// We don't want to draw the lights for the grapple.
		// They get left behind for a while and look bad.
		return false;
	}
#endif

	bool bDrawHolidayLights = false;

#ifdef USES_ECON_ITEMS
	if ( !m_bHolidayInitialized && GameRules() )
	{
		m_bHolidayInitialized = true;
		m_bDrawHolidayLights = GameRules()->IsHolidayActive( kHoliday_Christmas );
	}

	bDrawHolidayLights = m_bDrawHolidayLights;
	m_nHolidayLightsStyle = 0;

#ifdef TF_CLIENT_DLL
	// Turn them on in Pyro-vision too
	if ( IsLocalPlayerUsingVisionFilterFlags( TF_VISION_FILTER_PYRO ) )
	{
		bDrawHolidayLights = true;
		m_nHolidayLightsStyle = 1;
	}
#endif // TF_CLIENT_DLL

#endif // USES_ECON_ITEMS

	return bDrawHolidayLights;
}

int CRopeManager::GetHolidayLightStyle( void )
{
	return m_nHolidayLightsStyle;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRopeManager::RenderNonSolidRopes( IMatRenderContext *pRenderContext, IMaterial *pMaterial, int nVertCount, int nIndexCount )
{
	// Render the solid portion of the ropes.
	CMeshBuilder meshBuilder;
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, nVertCount, nIndexCount );

	CBeamSegDraw beamSegment;

	int nVerts = 0;
	for ( int iSegmentCache = 0; iSegmentCache < m_nSegmentCacheCount; ++iSegmentCache )
	{
		int nSegmentCount = m_aSegmentCache[iSegmentCache].m_nSegmentCount;
		beamSegment.Start( pRenderContext, nSegmentCount, pMaterial, &meshBuilder, nVerts );
		for ( int iSegment = 0; iSegment < nSegmentCount; ++iSegment )
		{
			beamSegment.NextSeg( &m_aSegmentCache[iSegmentCache].m_Segments[iSegment] );
		}
		beamSegment.End();
		nVerts += ( m_aSegmentCache[iSegmentCache].m_nSegmentCount * 2 );
	}
	
	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRopeManager::RenderSolidRopes( IMatRenderContext *pRenderContext, IMaterial *pMaterial, int nVertCount, int nIndexCount, bool bRenderNonSolid )
{
	// Render the solid portion of the ropes.
	CMeshBuilder meshBuilder;
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, nVertCount, nIndexCount );

	CBeamSegDraw beamSegment;

	if ( bRenderNonSolid )
	{
		int nVerts = 0;
		for ( int iSegmentCache = 0; iSegmentCache < m_nSegmentCacheCount; ++iSegmentCache )
		{
			RopeSegData_t *pSegData = &m_aSegmentCache[iSegmentCache];
			
			// If it's all going to be 0 alpha, then just skip drawing this one.
			if ( rope_solid_minalpha.GetFloat() == 0.0 && pSegData->m_flMaxBackWidth <= rope_solid_minwidth.GetFloat() )
				continue;

			int nSegmentCount = m_aSegmentCache[iSegmentCache].m_nSegmentCount;
			beamSegment.Start( pRenderContext, nSegmentCount, pMaterial, &meshBuilder, nVerts );
			for ( int iSegment = 0; iSegment < nSegmentCount; ++iSegment )
			{
				BeamSeg_t *pSeg = &m_aSegmentCache[iSegmentCache].m_Segments[iSegment];
				pSeg->m_flWidth = m_aSegmentCache[iSegmentCache].m_BackWidths[iSegment];

				// To avoid aliasing, the "solid" version of the rope on xbox is just "more solid",
				// and it has its own values controlling its alpha.
				pSeg->m_flAlpha = RemapVal( pSeg->m_flWidth, 
					rope_solid_minwidth.GetFloat(),
					rope_solid_maxwidth.GetFloat(),
					rope_solid_minalpha.GetFloat(),
					rope_solid_maxalpha.GetFloat() );
				
				pSeg->m_flAlpha = clamp( pSeg->m_flAlpha, 0.0f, 1.0f );
				
				beamSegment.NextSeg( &m_aSegmentCache[iSegmentCache].m_Segments[iSegment] );
			}
			beamSegment.End();
			nVerts += ( m_aSegmentCache[iSegmentCache].m_nSegmentCount * 2 );
		}
	}
	else
	{
		int nVerts = 0;
		for ( int iSegmentCache = 0; iSegmentCache < m_nSegmentCacheCount; ++iSegmentCache )
		{
			int nSegmentCount = m_aSegmentCache[iSegmentCache].m_nSegmentCount;
			beamSegment.Start( pRenderContext, nSegmentCount, pMaterial, &meshBuilder, nVerts );
			for ( int iSegment = 0; iSegment < nSegmentCount; ++iSegment )
			{
				beamSegment.NextSeg( &m_aSegmentCache[iSegmentCache].m_Segments[iSegment] );
			}
			beamSegment.End();
			nVerts += ( m_aSegmentCache[iSegmentCache].m_nSegmentCount * 2 );
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRopeManager::ResetSegmentCache( int nMaxSegments )
{
	MEM_ALLOC_CREDIT();
	m_nSegmentCacheCount = 0;
	if ( nMaxSegments )
		m_aSegmentCache.EnsureCount( nMaxSegments );
	else
		m_aSegmentCache.Purge();

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
RopeSegData_t *CRopeManager::GetNextSegmentFromCache( void )
{
	if ( m_nSegmentCacheCount >= m_aSegmentCache.Count() )
	{
		Warning( "CRopeManager::GetNextSegmentFromCache too many segments for cache!\n" );
		return NULL;
	}

	++m_nSegmentCacheCount;
	return &m_aSegmentCache[m_nSegmentCacheCount-1];
}



void CRopeManager::RemoveRopeFromQueuedRenderCaches( C_RopeKeyframe *pRope )
{
	//remove this rope from queued render caches	
	AUTO_LOCK( m_RenderCacheMutex );
	int index = m_RopeQueuedRenderCaches.Head();
	while( m_RopeQueuedRenderCaches.IsValidIndex( index ) )
	{
		RopeQueuedRenderCache_t &RenderCacheData = m_RopeQueuedRenderCaches[index];
		for( int i = 0; i != RenderCacheData.iCacheCount; ++i )
		{
			RopeRenderData_t *pCache = &RenderCacheData.pCaches[i];
			for( int j = 0; j != pCache->m_nCacheCount; ++j )
			{
				if( pCache->m_aCache[j] == pRope )
				{
					pCache->m_aCache[j] = NULL;
				}
			}
		}

		index = m_RopeQueuedRenderCaches.Next( index );
	}	
}

//=============================================================================

// ------------------------------------------------------------------------------------ //
// Global functions.
// ------------------------------------------------------------------------------------ //

void Rope_ResetCounters()
{
	g_RopeCollideTicks.Init();
	g_RopeDrawTicks.Init();
	g_RopeSimulateTicks.Init();
	g_nRopePointsSimulated = 0;
}


// ------------------------------------------------------------------------------------ //
// This handles the rope shake command.
// ------------------------------------------------------------------------------------ //

void ShakeRopesCallback( const CEffectData &data )
{
	Vector vCenter = data.m_vOrigin;
	float flRadius = data.m_flRadius;
	float flMagnitude = data.m_flMagnitude;

	// Now find any nearby ropes and shake them.
	FOR_EACH_LL( g_Ropes, i )
	{
		C_RopeKeyframe *pRope = g_Ropes[i];
	
		pRope->ShakeRope( vCenter, flRadius, flMagnitude );
	}
}

DECLARE_CLIENT_EFFECT( "ShakeRopes", ShakeRopesCallback );




// ------------------------------------------------------------------------------------ //
// C_RopeKeyframe::CPhysicsDelegate
// ------------------------------------------------------------------------------------ //
#define WIND_FORCE_FACTOR 10

void C_RopeKeyframe::CPhysicsDelegate::GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel )
{
	// Gravity.
	if ( !( m_pKeyframe->GetRopeFlags() & ROPE_NO_GRAVITY ) )
	{
		pAccel->Init( ROPE_GRAVITY );
	}

	if( !m_pKeyframe->m_LinksTouchingSomething[iNode] && m_pKeyframe->m_bApplyWind)
	{
		Vector vecWindVel;
		GetWindspeedAtTime(gpGlobals->curtime, vecWindVel);
		if ( vecWindVel.LengthSqr() > 0 )
		{
			Vector vecWindAccel;
			VectorMA( *pAccel, WIND_FORCE_FACTOR, vecWindVel, *pAccel );
		}
		else
		{
			if (m_pKeyframe->m_flCurrentGustTimer < m_pKeyframe->m_flCurrentGustLifetime )
			{
				float div = m_pKeyframe->m_flCurrentGustTimer / m_pKeyframe->m_flCurrentGustLifetime;
				float scale = 1 - cos( div * M_PI );

				*pAccel += m_pKeyframe->m_vWindDir * scale;
			}
		}
	}

	// HACK.. shake the rope around.
	static float scale=15000;
	if( rope_shake.GetInt() )
	{
		*pAccel += RandomVector( -scale, scale );
	}

	// Apply any instananeous forces and reset
	*pAccel += ROPE_IMPULSE_SCALE * m_pKeyframe->m_flImpulse;
	m_pKeyframe->m_flImpulse *= ROPE_IMPULSE_DECAY;
}


void LockNodeDirection( 
	CSimplePhysics::CNode *pNodes, 
	int parity, 
	int nFalloffNodes,
	float flLockAmount,
	float flLockFalloff,
	const Vector &vIdealDir ) 
{
	for ( int i=0; i < nFalloffNodes; i++ )
	{
		Vector &v0 = pNodes[i*parity].m_vPos;
		Vector &v1 = pNodes[(i+1)*parity].m_vPos;

		Vector vDir = v1 - v0;
		float len = vDir.Length();
		if ( len > 0.0001f )
		{
			vDir /= len;

			Vector vActual;
			VectorLerp( vDir, vIdealDir, flLockAmount, vActual );
			v1 = v0 + vActual * len;

			flLockAmount *= flLockFalloff;
		}
	}
}


void C_RopeKeyframe::CPhysicsDelegate::ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes )
{
	VPROF( "CPhysicsDelegate::ApplyConstraints" );

	CTraceFilterWorldOnly traceFilter;

	// Collide with the world.
	if( ((m_pKeyframe->m_RopeFlags & ROPE_COLLIDE) && 
		rope_collide.GetInt()) || 
		(rope_collide.GetInt() == 2) )
	{
		CTimeAdder adder( &g_RopeCollideTicks );

		for( int i=0; i < nNodes; i++ )
		{
			CSimplePhysics::CNode *pNode = &pNodes[i];

			int iIteration;
			int nIterations = 10;
			for( iIteration=0; iIteration < nIterations; iIteration++ )
			{
				trace_t trace;
				UTIL_TraceHull( pNode->m_vPrevPos, pNode->m_vPos, 
					Vector(-2,-2,-2), Vector(2,2,2), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

				if( trace.fraction == 1 )
					break;

				if( trace.fraction == 0 || trace.allsolid || trace.startsolid )
				{
					m_pKeyframe->m_LinksTouchingSomething[i] = true;
					pNode->m_vPos = pNode->m_vPrevPos;
					break;
				}

				// Apply some friction.
				static float flSlowFactor = 0.3f;
				pNode->m_vPos -= (pNode->m_vPos - pNode->m_vPrevPos) * flSlowFactor;

				// Move it out along the face normal.
				float distBehind = trace.plane.normal.Dot( pNode->m_vPos ) - trace.plane.dist;
				pNode->m_vPos += trace.plane.normal * (-distBehind + 2.2);
				m_pKeyframe->m_LinksTouchingSomething[i] = true;
			}

			if( iIteration == nIterations )
				pNodes[i].m_vPos = pNodes[i].m_vPrevPos;
		}
	}

	// Lock the endpoints.
	QAngle angles;
	if( m_pKeyframe->m_fLockedPoints & ROPE_LOCK_START_POINT )
	{
		m_pKeyframe->GetEndPointAttachment( 0, pNodes[0].m_vPos, angles );
		if (( m_pKeyframe->m_fLockedPoints & ROPE_LOCK_START_DIRECTION ) && (nNodes > 3))
		{
			Vector forward;
			AngleVectors( angles, &forward );

			int parity = 1;
			int nFalloffNodes = MIN( 2, nNodes - 2 );
			LockNodeDirection( pNodes, parity, nFalloffNodes, g_flLockAmount, g_flLockFalloff, forward );
		}
	}

	if( m_pKeyframe->m_fLockedPoints & ROPE_LOCK_END_POINT )
	{
		m_pKeyframe->GetEndPointAttachment( 1, pNodes[nNodes-1].m_vPos, angles );
		if( m_pKeyframe->m_fLockedPoints & ROPE_LOCK_END_DIRECTION && (nNodes > 3))
		{
			Vector forward;
			AngleVectors( angles, &forward );
			
			int parity = -1;
			int nFalloffNodes = MIN( 2, nNodes - 2 );
			LockNodeDirection( &pNodes[nNodes-1], parity, nFalloffNodes, g_flLockAmount, g_flLockFalloff, forward );
		}
	}
}


// ------------------------------------------------------------------------------------ //
// C_RopeKeyframe
// ------------------------------------------------------------------------------------ //

C_RopeKeyframe::C_RopeKeyframe()
{
	m_bEndPointAttachmentPositionsDirty = true;
	m_bEndPointAttachmentAnglesDirty = true;
	m_PhysicsDelegate.m_pKeyframe = this;
	m_pMaterial = NULL;
	m_bPhysicsInitted = false;
	m_RopeFlags = 0;
	m_TextureHeight = 1;
	m_hStartPoint = m_hEndPoint = NULL;
	m_iStartAttachment = m_iEndAttachment = 0;
	m_vColorMod.Init( 1, 1, 1 );
	m_nLinksTouchingSomething = 0;
	m_Subdiv = 255; // default to using the cvar
	
	m_fLockedPoints = 0;
	m_fPrevLockedPoints = 0;
	
	m_iForcePointMoveCounter = 0;
	m_flCurScroll = m_flScrollSpeed = 0;
	m_TextureScale = 4;	// 4:1
	m_flImpulse.Init();

	g_Ropes.AddToTail( this );
}


C_RopeKeyframe::~C_RopeKeyframe()
{
	s_RopeManager.RemoveRopeFromQueuedRenderCaches( this );	
	g_Ropes.FindAndRemove( this );

	if ( m_pBackMaterial )
	{
		m_pBackMaterial->DecrementReferenceCount();
		m_pBackMaterial = NULL;
	}
}


C_RopeKeyframe* C_RopeKeyframe::Create(
	C_BaseEntity *pStartEnt,
	C_BaseEntity *pEndEnt,
	int iStartAttachment,
	int iEndAttachment,
	float ropeWidth,
	const char *pMaterialName,
	int numSegments,
	int ropeFlags
	)
{
	C_RopeKeyframe *pRope = new C_RopeKeyframe;

	pRope->InitializeAsClientEntity( NULL, RENDER_GROUP_OPAQUE_ENTITY );
	
	if ( pStartEnt )
	{
		pRope->m_hStartPoint = pStartEnt;
		pRope->m_fLockedPoints |= ROPE_LOCK_START_POINT;
	}

	if ( pEndEnt )
	{
		pRope->m_hEndPoint = pEndEnt;
		pRope->m_fLockedPoints |= ROPE_LOCK_END_POINT;
	}

	pRope->m_iStartAttachment = iStartAttachment;
	pRope->m_iEndAttachment = iEndAttachment;
	pRope->m_Width = ropeWidth;
	pRope->m_nSegments = clamp( numSegments, 2, ROPE_MAX_SEGMENTS );
	pRope->m_RopeFlags = ropeFlags;

	pRope->FinishInit( pMaterialName );
	return pRope;
}


C_RopeKeyframe* C_RopeKeyframe::CreateFromKeyValues( C_BaseAnimating *pEnt, KeyValues *pValues )
{
	C_RopeKeyframe *pRope = C_RopeKeyframe::Create( 
		pEnt,
		pEnt,
		pEnt->LookupAttachment( pValues->GetString( "StartAttachment" ) ),
		pEnt->LookupAttachment( pValues->GetString( "EndAttachment" ) ),
		pValues->GetFloat( "Width", 0.5 ),
		pValues->GetString( "Material" ),
		pValues->GetInt( "NumSegments" ),
		0 );

	if ( pRope )
	{
		if ( pValues->GetInt( "Gravity", 1 ) == 0 )
		{
			pRope->m_RopeFlags |= ROPE_NO_GRAVITY;
		}

		pRope->m_RopeLength = pValues->GetInt( "Length" );
		pRope->m_TextureScale = pValues->GetFloat( "TextureScale", pRope->m_TextureScale );
		pRope->m_Slack = 0;
		pRope->m_RopeFlags |= ROPE_SIMULATE;
	}

	return pRope;
}


int C_RopeKeyframe::GetRopesIntersectingAABB( C_RopeKeyframe **pRopes, int nMaxRopes, const Vector &vAbsMin, const Vector &vAbsMax )
{
	if ( nMaxRopes == 0 )
		return 0;
	
	int nRopes = 0;
	FOR_EACH_LL( g_Ropes, i )
	{
		C_RopeKeyframe *pRope = g_Ropes[i];
	
		Vector v1, v2;
		if ( pRope->GetEndPointPos( 0, v1 ) && pRope->GetEndPointPos( 1, v2 ) )
		{
			if ( IsBoxIntersectingRay( v1, v2-v1, vAbsMin, vAbsMax, 0.1f ) )
			{
				pRopes[nRopes++] = pRope;
				if ( nRopes == nMaxRopes )
					break;
			}
		}
	}

	return nRopes;
}


void C_RopeKeyframe::SetSlack( int slack )
{
	m_Slack = slack;
	RecomputeSprings();
}


void C_RopeKeyframe::SetRopeFlags( int flags )
{
	m_RopeFlags = flags;
	UpdateVisibility();
}

	
int C_RopeKeyframe::GetRopeFlags() const
{
	return m_RopeFlags;
}


void C_RopeKeyframe::SetupHangDistance( float flHangDist )
{
	C_BaseEntity *pEnt1 = m_hStartPoint;
	C_BaseEntity *pEnt2 = m_hEndPoint;
	if ( !pEnt1 || !pEnt2 )
		return;

	QAngle dummyAngles;

	// Calculate starting conditions so we can force it to hang down N inches.
	Vector v1 = pEnt1->GetAbsOrigin();
	pEnt1->GetAttachment( m_iStartAttachment, v1, dummyAngles );
		
	Vector v2 = pEnt2->GetAbsOrigin();
	pEnt2->GetAttachment( m_iEndAttachment, v2, dummyAngles );

	float flSlack, flLen;
	CalcRopeStartingConditions( v1, v2, ROPE_MAX_SEGMENTS, flHangDist, &flLen, &flSlack );

	m_RopeLength = (int)flLen;
	m_Slack = (int)flSlack;

	RecomputeSprings();
}


void C_RopeKeyframe::SetStartEntity( C_BaseEntity *pEnt )
{
	m_hStartPoint = pEnt;
}


void C_RopeKeyframe::SetEndEntity( C_BaseEntity *pEnt )
{
	m_hEndPoint = pEnt;
}


C_BaseEntity* C_RopeKeyframe::GetStartEntity() const
{
	return m_hStartPoint;
}


C_BaseEntity* C_RopeKeyframe::GetEndEntity() const
{
	return m_hEndPoint;
}


CSimplePhysics::IHelper* C_RopeKeyframe::HookPhysics( CSimplePhysics::IHelper *pHook )
{
	m_RopePhysics.SetDelegate( pHook );
	return &m_PhysicsDelegate;
}


void C_RopeKeyframe::SetColorMod( const Vector &vColorMod )
{
	m_vColorMod = vColorMod;
}


void C_RopeKeyframe::RecomputeSprings()
{
	m_RopePhysics.ResetSpringLength(
		(m_RopeLength + m_Slack + ROPESLACK_FUDGEFACTOR) / (m_RopePhysics.NumNodes() - 1) );
}


void C_RopeKeyframe::ShakeRope( const Vector &vCenter, float flRadius, float flMagnitude )
{
	// Sum up whatever it would apply to all of our points.
	for ( int i=0; i < m_nSegments; i++ )
	{
		CSimplePhysics::CNode *pNode = m_RopePhysics.GetNode( i );

		float flDist = (pNode->m_vPos - vCenter).Length();
	
		float flShakeAmount = 1.0f - flDist / flRadius;
		if ( flShakeAmount >= 0 )
		{
			m_flImpulse.z += flShakeAmount * flMagnitude;
		}
	}
}


void C_RopeKeyframe::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	m_bNewDataThisFrame = true;

	if( updateType != DATA_UPDATE_CREATED )
		return;

	// Figure out the material name.
	char str[512];
	const model_t *pModel = modelinfo->GetModel( m_iRopeMaterialModelIndex );
	if ( pModel )
	{
		Q_strncpy( str, modelinfo->GetModelName( pModel ), sizeof( str ) );

		// Get rid of the extension because the material system doesn't want it.
		char *pExt = Q_stristr( str, ".vmt" );
		if ( pExt )
			pExt[0] = 0;
	}
	else
	{
		Q_strncpy( str, "asdf", sizeof( str ) );
	}
	
	FinishInit( str );
}


void C_RopeKeyframe::FinishInit( const char *pMaterialName )
{
	// Get the material from the material system.	
	m_pMaterial = materials->FindMaterial( pMaterialName, TEXTURE_GROUP_OTHER );
	if( m_pMaterial )
		m_TextureHeight = m_pMaterial->GetMappingHeight();
	else
		m_TextureHeight = 1;

	char backName[512];
	Q_snprintf( backName, sizeof( backName ), "%s_back", pMaterialName );
	
	m_pBackMaterial = materials->FindMaterial( backName, TEXTURE_GROUP_OTHER, false );
	if ( IsErrorMaterial( m_pBackMaterial ) )
		m_pBackMaterial = NULL;

	if ( m_pBackMaterial )
	{
		m_pBackMaterial->IncrementReferenceCount();
		m_pBackMaterial->GetMappingWidth();
	}
	
	// Init rope physics.
	m_nSegments = clamp( m_nSegments, 2, ROPE_MAX_SEGMENTS );
	m_RopePhysics.SetNumNodes( m_nSegments );

	SetCollisionBounds( Vector( -10, -10, -10 ), Vector( 10, 10, 10 ) );

	// We want to think every frame.
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

void C_RopeKeyframe::RunRopeSimulation( float flSeconds )
{
	// First, forget about links touching things.
	for ( int i=0; i < m_nSegments; i++ )
		m_LinksTouchingSomething[i] = false;

	// Simulate, and it will mark which links touched things.
	m_RopePhysics.Simulate( flSeconds );

	// Now count how many links touched something.
	m_nLinksTouchingSomething = 0;
	for ( int i=0; i < m_nSegments; i++ )
	{
		if ( m_LinksTouchingSomething[i] )
			++m_nLinksTouchingSomething;
	}
}

Vector C_RopeKeyframe::ConstrainNode( const Vector &vNormal, const Vector &vNodePosition, const Vector &vMidpiont, float fNormalLength )
{
	// Get triangle edges formed
	Vector vMidpointToNode = vNodePosition - vMidpiont;
	Vector vMidpointToNodeProjected = vMidpointToNode.Dot( vNormal ) * vNormal;
	float fMidpointToNodeLengh = VectorNormalize( vMidpointToNode );
	float fMidpointToNodeProjectedLengh = VectorNormalize( vMidpointToNodeProjected );

	// See if it's past an endpoint
	if ( fMidpointToNodeProjectedLengh < fNormalLength + 1.0f )
		return vNodePosition;

	// Apply the ratio between the triangles
	return vMidpiont + vMidpointToNode * fMidpointToNodeLengh * ( fNormalLength / fMidpointToNodeProjectedLengh );
}

void C_RopeKeyframe::ConstrainNodesBetweenEndpoints( void )
{
	if ( !m_bConstrainBetweenEndpoints )
		return;

	// Get midpoint and normals
	Vector vMidpiont = ( m_vCachedEndPointAttachmentPos[ 0 ] + m_vCachedEndPointAttachmentPos[ 1 ] ) / 2.0f;
	Vector vNormal = vMidpiont - m_vCachedEndPointAttachmentPos[ 0 ];
	float fNormalLength = VectorNormalize( vNormal );

	// Loop through all the middle segments and ensure their positions are constrained between the endpoints
	for ( int i = 1; i < m_RopePhysics.NumNodes() - 1; ++i )
	{
		// Fix the current position
		m_RopePhysics.GetNode( i )->m_vPos = ConstrainNode( vNormal, m_RopePhysics.GetNode( i )->m_vPos, vMidpiont, fNormalLength );

		// Fix the predicted position
		m_RopePhysics.GetNode( i )->m_vPredicted = ConstrainNode( vNormal, m_RopePhysics.GetNode( i )->m_vPredicted, vMidpiont, fNormalLength );
	}
}

void C_RopeKeyframe::ClientThink()
{
	// Only recalculate the endpoint attachments once per frame.
	m_bEndPointAttachmentPositionsDirty = true;
	m_bEndPointAttachmentAnglesDirty = true;
	
	if( !r_drawropes.GetBool() )
		return;

	if( !InitRopePhysics() ) // init if not already
		return;

	if( !DetectRestingState( m_bApplyWind ) )
	{
		// Update the simulation.
		CTimeAdder adder( &g_RopeSimulateTicks );
		
		RunRopeSimulation( gpGlobals->frametime );

		g_nRopePointsSimulated += m_RopePhysics.NumNodes();

		m_bNewDataThisFrame = false;

		// Setup a new wind gust?
		m_flCurrentGustTimer += gpGlobals->frametime;
		m_flTimeToNextGust -= gpGlobals->frametime;
		if( m_flTimeToNextGust <= 0 )
		{
			m_vWindDir = RandomVector( -1, 1 );
			VectorNormalize( m_vWindDir );

			static float basicScale = 50;
			m_vWindDir *= basicScale;
			m_vWindDir *= RandomFloat( -1.0f, 1.0f );
			
			m_flCurrentGustTimer = 0;
			m_flCurrentGustLifetime = RandomFloat( 2.0f, 3.0f );

			m_flTimeToNextGust = RandomFloat( 3.0f, 4.0f );
		}

		UpdateBBox();
	}
}


int C_RopeKeyframe::DrawModel( int flags )
{
	VPROF_BUDGET( "C_RopeKeyframe::DrawModel", VPROF_BUDGETGROUP_ROPES );
	if( !InitRopePhysics() )
		return 0;

	if ( !m_bReadyToDraw )
		return 0;

	// Resize the rope
	if( m_RopeFlags & ROPE_RESIZE )
	{
		RecomputeSprings();
	}

	// If our start & end entities have models, but are nodraw, then we don't draw
	if ( m_hStartPoint && m_hStartPoint->IsDormant() && m_hEndPoint && m_hEndPoint->IsDormant() )
	{
		// Check models because rope endpoints are point entities
		if ( m_hStartPoint->GetModelIndex() && m_hEndPoint->GetModelIndex() )
			return 0;
	}

	ConstrainNodesBetweenEndpoints();

	RopeManager()->AddToRenderCache( this );
	return 1;
}

bool C_RopeKeyframe::ShouldDraw()
{
	if( !r_ropetranslucent.GetBool() ) 
		return false;

	if( !(m_RopeFlags & ROPE_SIMULATE) )
		return false;

	return true;
}

const Vector& C_RopeKeyframe::WorldSpaceCenter( ) const
{
	return GetAbsOrigin();
}

bool C_RopeKeyframe::GetAttachment( int number, matrix3x4_t &matrix )
{
	int nNodes = m_RopePhysics.NumNodes();
	if ( (number != ROPE_ATTACHMENT_START_POINT && number != ROPE_ATTACHMENT_END_POINT) || nNodes < 2 )
		return false;

	// Now setup the orientation based on the last segment.
	Vector vForward, origin;
	if ( number == ROPE_ATTACHMENT_START_POINT )
	{
		origin = m_RopePhysics.GetNode( 0 )->m_vPredicted;
		vForward = m_RopePhysics.GetNode( 0 )->m_vPredicted - m_RopePhysics.GetNode( 1 )->m_vPredicted;
	}
	else
	{
		origin = m_RopePhysics.GetNode( nNodes-1 )->m_vPredicted;
		vForward = m_RopePhysics.GetNode( nNodes-1 )->m_vPredicted - m_RopePhysics.GetNode( nNodes-2 )->m_vPredicted;
	}
	VectorMatrix( vForward, matrix );
	PositionMatrix( origin, matrix );
	return true;
}

bool C_RopeKeyframe::GetAttachment( int number, Vector &origin )
{
	int nNodes = m_RopePhysics.NumNodes();
	if ( (number != ROPE_ATTACHMENT_START_POINT && number != ROPE_ATTACHMENT_END_POINT) || nNodes < 2 )
		return false;

	// Now setup the orientation based on the last segment.
	if ( number == ROPE_ATTACHMENT_START_POINT )
	{
		origin = m_RopePhysics.GetNode( 0 )->m_vPredicted;
	}
	else
	{
		origin = m_RopePhysics.GetNode( nNodes-1 )->m_vPredicted;
	}
	return true;
}

bool C_RopeKeyframe::GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel )
{
	Assert(0);
	return false;
}

bool C_RopeKeyframe::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	int nNodes = m_RopePhysics.NumNodes();
	if ( (number == ROPE_ATTACHMENT_START_POINT || number == ROPE_ATTACHMENT_END_POINT) && nNodes >= 2 )
	{
		// Now setup the orientation based on the last segment.
		Vector vForward;
		if ( number == ROPE_ATTACHMENT_START_POINT )
		{
			origin = m_RopePhysics.GetNode( 0 )->m_vPredicted;
			vForward = m_RopePhysics.GetNode( 0 )->m_vPredicted - m_RopePhysics.GetNode( 1 )->m_vPredicted;
		}
		else
		{
			origin = m_RopePhysics.GetNode( nNodes-1 )->m_vPredicted;
			vForward = m_RopePhysics.GetNode( nNodes-1 )->m_vPredicted - m_RopePhysics.GetNode( nNodes-2 )->m_vPredicted;
		}
		VectorAngles( vForward, angles );

		return true;
	}
	
	return false;
}

bool C_RopeKeyframe::AnyPointsMoved()
{
	for( int i=0; i < m_RopePhysics.NumNodes(); i++ )
	{
		CSimplePhysics::CNode *pNode = m_RopePhysics.GetNode( i );
		float flMoveDistSqr = (pNode->m_vPos - pNode->m_vPrevPos).LengthSqr();
		if( flMoveDistSqr > 0.03f )
			return true;
	}

	if( --m_iForcePointMoveCounter > 0 )
		return true;

	return false;
}


inline bool C_RopeKeyframe::DidEndPointMove( int iPt )
{
	// If this point isn't locked anyway, just break out.
	if( !( m_fLockedPoints & (1 << iPt) ) )
		return false;

	bool bOld = m_bPrevEndPointPos[iPt];
	Vector vOld = m_vPrevEndPointPos[iPt];

	m_bPrevEndPointPos[iPt] = GetEndPointPos( iPt, m_vPrevEndPointPos[iPt] );
	
	// If it wasn't and isn't attached to anything, don't register a change.
	if( !bOld && !m_bPrevEndPointPos[iPt] )
		return true;

	// Register a change if the endpoint moves.
	if( !VectorsAreEqual( vOld, m_vPrevEndPointPos[iPt], 0.1 ) )
		return true;

	return false;
}


bool C_RopeKeyframe::DetectRestingState( bool &bApplyWind )
{
	bApplyWind = false;

	if( m_fPrevLockedPoints != m_fLockedPoints )
	{
		// Force it to move the points for some number of frames when they get detached or
		// after we get new data. This allows them to accelerate from gravity.
		m_iForcePointMoveCounter = 10; 
		m_fPrevLockedPoints = m_fLockedPoints;
		return false;
	}

	if( m_bNewDataThisFrame )
	{
		// Simulate if anything about us changed this frame, such as our position due to hierarchy.
		// FIXME: this won't work when hierarchy is client side
		return false;
	}

	// Make sure our attachment points haven't moved.
	if( DidEndPointMove( 0 ) || DidEndPointMove( 1 ) )
		return false;

	// See how close we are to the line.
	Vector &vEnd1 = m_RopePhysics.GetFirstNode()->m_vPos;
	Vector &vEnd2 = m_RopePhysics.GetLastNode()->m_vPos;
	
	if ( !( m_RopeFlags & ROPE_NO_WIND ) )
	{
		// Don't apply wind if more than half of the nodes are touching something.
		float flDist1 = CalcDistanceToLineSegment( MainViewOrigin(), vEnd1, vEnd2 );
		if( m_nLinksTouchingSomething < (m_RopePhysics.NumNodes() >> 1) )
			bApplyWind = flDist1 < rope_wind_dist.GetFloat();
	}

	if ( m_flPreviousImpulse != m_flImpulse )
	{
		m_flPreviousImpulse = m_flImpulse;
		return false;
	}

	return !AnyPointsMoved() && !bApplyWind && !rope_shake.GetInt();
}

// simple struct to precompute basis for catmull rom splines for faster evaluation
struct catmull_t
{
	Vector t3;
	Vector t2;
	Vector t;
	Vector c;
};

// bake out the terms of the catmull rom spline
void Catmull_Rom_Spline_Matrix( const Vector &vecP1, const Vector &vecP2, const Vector &vecP3, const Vector &vecP4, catmull_t &output )
{
	output.t3 = 0.5f * ( ( -1 * vecP1 ) + ( 3 * vecP2 ) + ( -3 * vecP3 ) + vecP4 );	// 0.5 t^3 * [ (-1*p1) + ( 3*p2) + (-3*p3) + p4 ]
	output.t2 = 0.5f * ( ( 2 * vecP1 ) + ( -5 * vecP2 ) + ( 4 * vecP3 ) - vecP4 );		// 0.5 t^2 * [ ( 2*p1) + (-5*p2) + ( 4*p3) - p4 ]
	output.t = 0.5f * ( ( -1 * vecP1 ) + vecP3 );						// 0.5 t * [ (-1*p1) + p3 ]
	output.c = vecP2;											// p2
}

// evaluate one point on the spline, t is a vector of (t, t^2, t^3)
inline void Catmull_Rom_Eval( const catmull_t &spline, const Vector &t, Vector &output )
{
	Assert(spline.c.IsValid());
	Assert(spline.t.IsValid());
	Assert(spline.t2.IsValid());
	Assert(spline.t3.IsValid());
	output = spline.c + (t.x * spline.t) + (t.y*spline.t2) + (t.z * spline.t3);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_RopeKeyframe::BuildRope( RopeSegData_t *pSegmentData, const Vector &vCurrentViewForward, const Vector &vCurrentViewOrigin, C_RopeKeyframe::BuildRopeQueuedData_t *pQueuedData, bool bQueued )
{
	if ( !pSegmentData )
		return;

	// Get the lighting values.
	Vector *pLightValues = ( mat_fullbright.GetInt() == 1 ) ? g_FullBright_LightValues : pQueuedData->m_pLightValues;

	// Update the rope subdivisions if necessary.
	int nSubdivCount;
	Vector *pSubdivVecList = GetRopeSubdivVectors( &nSubdivCount );

	int nSegmentCount = 0;
	int iPrevNode = 0;
	const float subdivScale = 1.0f / (nSubdivCount+1);
	const int nodeCount = pQueuedData->m_iNodeCount;
	const int lastNode = nodeCount-1;
	catmull_t spline;

	Vector *pPredictedPositions = pQueuedData->m_pPredictedPositions;
	Vector vColorMod = pQueuedData->m_vColorMod;

	for( int iNode = 0; iNode < nodeCount; ++iNode )
	{
		pSegmentData->m_Segments[nSegmentCount].m_vPos = pPredictedPositions[iNode];
		pSegmentData->m_Segments[nSegmentCount].m_vColor = pLightValues[iNode] * vColorMod;

		CEffectData data;

		if ( !bQueued && RopeManager()->IsHolidayLightMode() && r_rope_holiday_light_scale.GetFloat() > 0.0f )
		{
			data.m_nMaterial = reinterpret_cast< int >( this );
			data.m_nHitBox = ( iNode << 8 );
			data.m_flScale = r_rope_holiday_light_scale.GetFloat();
			data.m_vOrigin = pSegmentData->m_Segments[nSegmentCount].m_vPos;
			DispatchEffect( "TF_HolidayLight", data );
		}

		++nSegmentCount;

		if ( iNode < lastNode )
		{
			// Draw a midpoint to the next segment.
			int iNext = iNode + 1;
			int iNextNext = iNode + 2;
			if ( iNext >= nodeCount )
			{
				iNext = iNextNext = lastNode;
			}
			else if ( iNextNext >= nodeCount )
			{
				iNextNext = lastNode;
			}

			Vector vecColorInc = subdivScale * ( ( pLightValues[iNode+1] - pLightValues[iNode] ) * vColorMod );
			// precompute spline basis
			Catmull_Rom_Spline_Matrix( pPredictedPositions[iPrevNode], pPredictedPositions[iNode], 
				pPredictedPositions[iNext], pPredictedPositions[iNextNext], spline );
			for( int iSubdiv = 0; iSubdiv < nSubdivCount; ++iSubdiv )
			{
				pSegmentData->m_Segments[nSegmentCount].m_vColor = pSegmentData->m_Segments[nSegmentCount-1].m_vColor + vecColorInc;
				// simple eval using precomputed basis
				Catmull_Rom_Eval( spline, pSubdivVecList[iSubdiv], pSegmentData->m_Segments[nSegmentCount].m_vPos );

				if ( !bQueued && RopeManager()->IsHolidayLightMode() && r_rope_holiday_light_scale.GetFloat() > 0.0f )
				{
					data.m_nHitBox++;
					data.m_flScale = r_rope_holiday_light_scale.GetFloat();
					data.m_vOrigin = pSegmentData->m_Segments[nSegmentCount].m_vPos;
					DispatchEffect( "TF_HolidayLight", data );
				}

				++nSegmentCount;
				Assert( nSegmentCount <= MAX_ROPE_SEGMENTS );
			}

			iPrevNode = iNode;
		}
	}
	pSegmentData->m_nSegmentCount = nSegmentCount;
	pSegmentData->m_flMaxBackWidth = 0;

	// Figure out texture scale.
	float flPixelsPerInch = 4.0f / m_TextureScale;
	float flTotalTexCoord = flPixelsPerInch * ( pQueuedData->m_RopeLength + pQueuedData->m_Slack + ROPESLACK_FUDGEFACTOR );
	int nTotalPoints = ( nodeCount - 1 ) * nSubdivCount + 1;
	float flActualInc = ( flTotalTexCoord / nTotalPoints ) / ( float )m_TextureHeight;

	// First draw a translucent rope underneath the solid rope for an antialiasing effect.
	if ( ShouldUseFakeAA( m_pBackMaterial ) )
	{
		// Compute screen width
		float flScreenWidth = ScreenWidth();
		float flHalfScreenWidth = flScreenWidth / 2.0f;

		float flExtraScreenSpaceWidth = rope_smooth_enlarge.GetFloat();

		float flMinAlpha = rope_smooth_minalpha.GetFloat();
		float flMaxAlpha = rope_smooth_maxalpha.GetFloat();

		float flMinScreenSpaceWidth = rope_smooth_minwidth.GetFloat();
		float flMaxAlphaScreenSpaceWidth = rope_smooth_maxalphawidth.GetFloat();
		
		float flTexCoord = m_flCurScroll;
		for ( int iSegment = 0; iSegment < nSegmentCount; ++iSegment )
		{
			pSegmentData->m_Segments[iSegment].m_flTexCoord = flTexCoord;			

			// Right here, we need to specify a width that will be 1 pixel larger in screen space.
			float zCoord = vCurrentViewForward.Dot( pSegmentData->m_Segments[iSegment].m_vPos - vCurrentViewOrigin );
			zCoord = MAX( zCoord, 0.1f );
							
			float flScreenSpaceWidth = m_Width * flHalfScreenWidth / zCoord;
			if ( flScreenSpaceWidth < flMinScreenSpaceWidth )
			{
				pSegmentData->m_Segments[iSegment].m_flAlpha = flMinAlpha;
				pSegmentData->m_Segments[iSegment].m_flWidth = flMinScreenSpaceWidth * zCoord / flHalfScreenWidth;
				pSegmentData->m_BackWidths[iSegment] = 0.0f;
			}
			else
			{
				if ( flScreenSpaceWidth > flMaxAlphaScreenSpaceWidth )
				{
					pSegmentData->m_Segments[iSegment].m_flAlpha = flMaxAlpha;
				}
				else
				{
					pSegmentData->m_Segments[iSegment].m_flAlpha = RemapVal( flScreenSpaceWidth, flMinScreenSpaceWidth, flMaxAlphaScreenSpaceWidth, flMinAlpha, flMaxAlpha );
				}

				pSegmentData->m_Segments[iSegment].m_flWidth = m_Width;
				pSegmentData->m_BackWidths[iSegment] = m_Width - ( zCoord * flExtraScreenSpaceWidth ) / flScreenWidth;
				if ( pSegmentData->m_BackWidths[iSegment] < 0.0f )
				{
					pSegmentData->m_BackWidths[iSegment] = 0.0f;
				}
				else
				{
					pSegmentData->m_flMaxBackWidth = MAX( pSegmentData->m_flMaxBackWidth, pSegmentData->m_BackWidths[iSegment] );
				}
			}

			// Get the next texture coordinate.
			flTexCoord += flActualInc;
		}
	}
	else
	{
		float flTexCoord = m_flCurScroll;

		// Build the data with no smoothing.
		for ( int iSegment = 0; iSegment < nSegmentCount; ++iSegment )
		{
			pSegmentData->m_Segments[iSegment].m_flTexCoord = flTexCoord;
			pSegmentData->m_Segments[iSegment].m_flAlpha = 0.3f;
			pSegmentData->m_Segments[iSegment].m_flWidth = m_Width;
			pSegmentData->m_BackWidths[iSegment] = -1.0f;

			// Get the next texture coordinate.
			flTexCoord += flActualInc;
		}
	}
}

void C_RopeKeyframe::UpdateBBox()
{
	Vector &vStart = m_RopePhysics.GetFirstNode()->m_vPos;
	Vector &vEnd   = m_RopePhysics.GetLastNode()->m_vPos;

	Vector mins, maxs;

	VectorMin( vStart, vEnd, mins );
	VectorMax( vStart, vEnd, maxs );

	for( int i=1; i < m_RopePhysics.NumNodes()-1; i++ )
	{
		const Vector &vPos = m_RopePhysics.GetNode(i)->m_vPos;
		AddPointToBounds( vPos, mins, maxs );
	}
	
	mins -= GetAbsOrigin();
	maxs -= GetAbsOrigin();
	SetCollisionBounds( mins, maxs );
}


bool C_RopeKeyframe::InitRopePhysics()
{
	if( !(m_RopeFlags & ROPE_SIMULATE) )
		return 0;

	if( m_bPhysicsInitted )
	{
		return true;
	}

	// Must have both entities to work.
	m_bPrevEndPointPos[0] = GetEndPointPos( 0, m_vPrevEndPointPos[0] );
	if( !m_bPrevEndPointPos[0] )
		return false;

	// They're allowed to not have an end attachment point so the rope can dangle.
	m_bPrevEndPointPos[1] = GetEndPointPos( 1, m_vPrevEndPointPos[1] );
	if( !m_bPrevEndPointPos[1] )
		m_vPrevEndPointPos[1] = m_vPrevEndPointPos[0];

	const Vector &vStart = m_vPrevEndPointPos[0];
	const Vector &vAttached = m_vPrevEndPointPos[1];

	m_RopePhysics.SetupSimulation( 0, &m_PhysicsDelegate );
	RecomputeSprings();
	m_RopePhysics.Restart();

	// Initialize the positions of the nodes.
	for( int i=0; i < m_RopePhysics.NumNodes(); i++ )
	{
		CSimplePhysics::CNode *pNode = m_RopePhysics.GetNode( i );
		float t = (float)i / (m_RopePhysics.NumNodes() - 1);
		
		VectorLerp( vStart, vAttached, t, pNode->m_vPos );
		pNode->m_vPrevPos = pNode->m_vPos;
	}

	// Simulate for a bit to let it sag.
	if ( m_RopeFlags & ROPE_INITIAL_HANG )
	{
		RunRopeSimulation( 5 );
	}

	CalcLightValues();

	// Set our bounds for visibility.
	UpdateBBox();

	m_flTimeToNextGust = RandomFloat( 1.0f, 3.0f );
	m_bPhysicsInitted = true;
	
	return true;
}


bool C_RopeKeyframe::CalculateEndPointAttachment( C_BaseEntity *pEnt, int iAttachment, Vector &vPos, QAngle *pAngles )
{
	VPROF_BUDGET( "C_RopeKeyframe::CalculateEndPointAttachment", VPROF_BUDGETGROUP_ROPES );

	if( !pEnt )
		return false;

	if ( m_RopeFlags & ROPE_PLAYER_WPN_ATTACH )
	{
		C_BasePlayer *pPlayer = dynamic_cast< C_BasePlayer* >( pEnt );
		if ( pPlayer )
		{
			C_BaseAnimating *pModel = pPlayer->GetRenderedWeaponModel();
			if ( !pModel )
				return false;

			int iAttachment = pModel->LookupAttachment( "buff_attach" );
			if ( pAngles )
				return pModel->GetAttachment( iAttachment, vPos, *pAngles );
			return pModel->GetAttachment( iAttachment, vPos );
		}
	}

	if( iAttachment > 0 )
	{
		bool bOk;
		if ( pAngles )
		{
			bOk = pEnt->GetAttachment( iAttachment, vPos, *pAngles );
		}
		else
		{
			bOk = pEnt->GetAttachment( iAttachment, vPos );
		}
		if ( bOk )
			return true;
	}

	vPos = pEnt->WorldSpaceCenter( );
	if ( pAngles )
	{
		*pAngles = pEnt->GetAbsAngles();
	}
	return true;
}

bool C_RopeKeyframe::GetEndPointPos( int iPt, Vector &vPos )
{
	// By caching the results here, we avoid doing this a bunch of times per frame.
	if ( m_bEndPointAttachmentPositionsDirty )
	{
		CalculateEndPointAttachment( m_hStartPoint, m_iStartAttachment, m_vCachedEndPointAttachmentPos[0], NULL );
		CalculateEndPointAttachment( m_hEndPoint, m_iEndAttachment, m_vCachedEndPointAttachmentPos[1], NULL );
		m_bEndPointAttachmentPositionsDirty = false;
	}

	Assert( iPt == 0 || iPt == 1 );
	vPos = m_vCachedEndPointAttachmentPos[iPt];
	return true;
}

IMaterial* C_RopeKeyframe::GetSolidMaterial( void )
{
#ifdef TF_CLIENT_DLL
	if ( RopeManager()->IsHolidayLightMode() )
	{
		if ( RopeManager()->GetHolidayLightStyle() == 1 )
		{
			return materials->FindMaterial( "cable/pure_white", TEXTURE_GROUP_OTHER );
		}
	}
#endif

	return m_pMaterial;
}
IMaterial* C_RopeKeyframe::GetBackMaterial( void )
{
	return m_pBackMaterial;
}

bool C_RopeKeyframe::GetEndPointAttachment( int iPt, Vector &vPos, QAngle &angle )
{
	// By caching the results here, we avoid doing this a bunch of times per frame.
	if ( m_bEndPointAttachmentPositionsDirty || m_bEndPointAttachmentAnglesDirty )
	{
		CalculateEndPointAttachment( m_hStartPoint, m_iStartAttachment, m_vCachedEndPointAttachmentPos[0], &m_vCachedEndPointAttachmentAngle[0] );
		CalculateEndPointAttachment( m_hEndPoint, m_iEndAttachment, m_vCachedEndPointAttachmentPos[1], &m_vCachedEndPointAttachmentAngle[1] );
		m_bEndPointAttachmentPositionsDirty = false;
		m_bEndPointAttachmentAnglesDirty = false;
	}

	Assert( iPt == 0 || iPt == 1 );
	vPos = m_vCachedEndPointAttachmentPos[iPt];
	angle = m_vCachedEndPointAttachmentAngle[iPt];
	return true;
}


// Look at the global cvar and recalculate rope subdivision data if necessary.
Vector *C_RopeKeyframe::GetRopeSubdivVectors( int *nSubdivs )
{
	if( m_RopeFlags & ROPE_BARBED )
	{
		*nSubdivs = g_nBarbedSubdivs;
		return g_BarbedSubdivs;
	}
	else
	{
		int subdiv = m_Subdiv;
		if ( subdiv == 255 )
		{
			subdiv = rope_subdiv.GetInt();
		}

		if ( subdiv >= MAX_ROPE_SUBDIVS )
			subdiv = MAX_ROPE_SUBDIVS-1;

		*nSubdivs = subdiv;
		return g_RopeSubdivs[subdiv];
	}
}


void C_RopeKeyframe::CalcLightValues()
{
	Vector boxColors[6];

	for( int i=0; i < m_RopePhysics.NumNodes(); i++ )
	{
		const Vector &vPos = m_RopePhysics.GetNode(i)->m_vPredicted;
		engine->ComputeLighting( vPos, NULL, true, m_LightValues[i], boxColors );

		if ( !rope_averagelight.GetInt() )
		{
			// The engine averages the lighting across the 6 box faces, but we would rather just get the MAX intensity
			// since we do our own half-lambert lighting in the rope shader to simulate directionality.
			//
			// So here, we take the average of all the incoming light, and scale it to use the max intensity of all the box sides.
			float flMaxIntensity = 0;
			for ( int iSide=0; iSide < 6; iSide++ )
			{
				float flLen = boxColors[iSide].Length();
				flMaxIntensity = MAX( flMaxIntensity, flLen );
			}

			VectorNormalize( m_LightValues[i] );
			m_LightValues[i] *= flMaxIntensity;
			float flMax = MAX( m_LightValues[i].x, MAX( m_LightValues[i].y, m_LightValues[i].z ) );
			if ( flMax > 1 )
				m_LightValues[i] /= flMax;
		}
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_RopeKeyframe::ReceiveMessage( int classID, bf_read &msg )
{
	if ( classID != GetClientClass()->m_ClassID )
	{
		// message is for subclass
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}

	// Read instantaneous fore data
	m_flImpulse.x   = msg.ReadFloat();
	m_flImpulse.y   = msg.ReadFloat();
	m_flImpulse.z   = msg.ReadFloat();
}
