//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose : Singleton manager for color correction on the client
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "tier0/vprof.h"
#include "colorcorrectionmgr.h"
#ifdef MAPBASE // From Alien Swarm SDK
#include "clientmode_shared.h" //"clientmode.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"
#endif


//------------------------------------------------------------------------------
// Singleton access
//------------------------------------------------------------------------------
static CColorCorrectionMgr s_ColorCorrectionMgr;
CColorCorrectionMgr *g_pColorCorrectionMgr = &s_ColorCorrectionMgr;

#ifdef MAPBASE // From Alien Swarm SDK
static ConVar mat_colcorrection_editor( "mat_colcorrection_editor", "0" );

static CUtlVector<C_ColorCorrection *> g_ColorCorrectionList;
static CUtlVector<C_ColorCorrectionVolume *> g_ColorCorrectionVolumeList;
#endif


//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
CColorCorrectionMgr::CColorCorrectionMgr()
{
	m_nActiveWeightCount = 0;
}


//------------------------------------------------------------------------------
// Creates, destroys color corrections
//------------------------------------------------------------------------------
ClientCCHandle_t CColorCorrectionMgr::AddColorCorrection( const char *pName, const char *pFileName )
{
	if ( !pFileName )
	{
		pFileName = pName;
	}

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	ColorCorrectionHandle_t ccHandle = pRenderContext->AddLookup( pName );
	if ( ccHandle )
	{
		pRenderContext->LockLookup( ccHandle );
		pRenderContext->LoadLookup( ccHandle, pFileName );
		pRenderContext->UnlockLookup( ccHandle );
	}
	else
	{
		Warning("Cannot find color correction lookup file: '%s'\n", pFileName );
	}

	return (ClientCCHandle_t)ccHandle;
}

void CColorCorrectionMgr::RemoveColorCorrection( ClientCCHandle_t h )
{
	if ( h != INVALID_CLIENT_CCHANDLE )
	{
		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
		ColorCorrectionHandle_t ccHandle = (ColorCorrectionHandle_t)h;
		pRenderContext->RemoveLookup( ccHandle );
	}
}

#ifdef MAPBASE // From Alien Swarm SDK
ClientCCHandle_t CColorCorrectionMgr::AddColorCorrectionEntity( C_ColorCorrection *pEntity, const char *pName, const char *pFileName )
{
	ClientCCHandle_t h = AddColorCorrection(pName, pFileName);
	if ( h != INVALID_CLIENT_CCHANDLE )
	{
		Assert(g_ColorCorrectionList.Find(pEntity) == -1);
		g_ColorCorrectionList.AddToTail(pEntity);
	}
	return h;
}

void CColorCorrectionMgr::RemoveColorCorrectionEntity( C_ColorCorrection *pEntity, ClientCCHandle_t h)
{
	RemoveColorCorrection(h);
	g_ColorCorrectionList.FindAndFastRemove(pEntity);
}

ClientCCHandle_t CColorCorrectionMgr::AddColorCorrectionVolume( C_ColorCorrectionVolume *pVolume, const char *pName, const char *pFileName )
{
	ClientCCHandle_t h = AddColorCorrection(pName, pFileName);
	if ( h != INVALID_CLIENT_CCHANDLE )
	{
		Assert(g_ColorCorrectionVolumeList.Find(pVolume) == -1);
		g_ColorCorrectionVolumeList.AddToTail(pVolume);
	}
	return h;
}

void CColorCorrectionMgr::RemoveColorCorrectionVolume( C_ColorCorrectionVolume *pVolume, ClientCCHandle_t h)
{
	RemoveColorCorrection(h);
	g_ColorCorrectionVolumeList.FindAndFastRemove(pVolume);
}
#endif

//------------------------------------------------------------------------------
// Modify color correction weights
//------------------------------------------------------------------------------
#ifdef MAPBASE // From Alien Swarm SDK
void CColorCorrectionMgr::SetColorCorrectionWeight( ClientCCHandle_t h, float flWeight, bool bExclusive )
{
	if ( h != INVALID_CLIENT_CCHANDLE )
	{
		SetWeightParams_t params = { h, flWeight, bExclusive };
		m_colorCorrectionWeights.AddToTail( params );
		if( bExclusive && m_bHaveExclusiveWeight && ( flWeight != 0.0f ) )
		{
			DevWarning( "Found multiple active color_correction entities with exclusive setting enabled. This is invalid.\n" );
		}
		if ( bExclusive )
		{
			m_bHaveExclusiveWeight = true;
			m_flExclusiveWeight = flWeight;
		}
	}
}

void CColorCorrectionMgr::CommitColorCorrectionWeights()
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	for ( int i = 0; i < m_colorCorrectionWeights.Count(); i++ )
	{
		ColorCorrectionHandle_t ccHandle = reinterpret_cast<ColorCorrectionHandle_t>( m_colorCorrectionWeights[i].handle );
		float flWeight = m_colorCorrectionWeights[i].flWeight;
		if ( !m_colorCorrectionWeights[i].bExclusive )
		{
			flWeight = (1.0f - m_flExclusiveWeight ) * m_colorCorrectionWeights[i].flWeight;
		}
		pRenderContext->SetLookupWeight( ccHandle, flWeight );

		// FIXME: NOTE! This doesn't work if the same handle has
		// its weight set twice with no intervening calls to ResetColorCorrectionWeights
		// which, at the moment, is true
		if ( flWeight != 0.0f )
		{
			++m_nActiveWeightCount;
		}
	}
	m_colorCorrectionWeights.RemoveAll();
}
#else
void CColorCorrectionMgr::SetColorCorrectionWeight( ClientCCHandle_t h, float flWeight )
{
	if ( h != INVALID_CLIENT_CCHANDLE )
	{
		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
		ColorCorrectionHandle_t ccHandle = (ColorCorrectionHandle_t)h;
		pRenderContext->SetLookupWeight( ccHandle, flWeight );

		// FIXME: NOTE! This doesn't work if the same handle has
		// its weight set twice with no intervening calls to ResetColorCorrectionWeights
		// which, at the moment, is true
		if ( flWeight != 0.0f )
		{
			++m_nActiveWeightCount;
		}
	}
}
#endif

void CColorCorrectionMgr::ResetColorCorrectionWeights()
{
	VPROF_("ResetColorCorrectionWeights", 2, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, false, 0);
	// FIXME: Where should I put this? It needs to happen prior to SimulateEntities()
	// which is where the client thinks for c_colorcorrection + c_colorcorrectionvolumes
	// update the color correction weights.
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->ResetLookupWeights();
	m_nActiveWeightCount = 0;
#ifdef MAPBASE // From Alien Swarm SDK
	m_bHaveExclusiveWeight = false;
	m_flExclusiveWeight = 0.0f;
	m_colorCorrectionWeights.RemoveAll();
#endif
}

void CColorCorrectionMgr::SetResetable( ClientCCHandle_t h, bool bResetable )
{
	// NOTE: Setting stuff to be not resettable doesn't work when in queued mode
	// because the logic that sets m_nActiveWeightCount to 0 in ResetColorCorrectionWeights
	// is no longer valid when stuff is not resettable.
	Assert( bResetable || !g_pMaterialSystem->GetThreadMode() == MATERIAL_SINGLE_THREADED );
	if ( h != INVALID_CLIENT_CCHANDLE )
	{
		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
		ColorCorrectionHandle_t ccHandle = (ColorCorrectionHandle_t)h;
		pRenderContext->SetResetable( ccHandle, bResetable );
	}
}


//------------------------------------------------------------------------------
// Is color correction active?
//------------------------------------------------------------------------------
#ifdef MAPBASE // From Alien Swarm SDK
bool CColorCorrectionMgr::HasNonZeroColorCorrectionWeights() const
{
	return ( m_nActiveWeightCount != 0 ) || mat_colcorrection_editor.GetBool();
}

void CColorCorrectionMgr::UpdateColorCorrection()
{
	ResetColorCorrectionWeights();
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	IClientMode *pClientMode = GetClientModeNormal(); //GetClientMode();

	Assert( pClientMode );
	if ( !pPlayer || !pClientMode )
	{
		return;
	}

	pClientMode->OnColorCorrectionWeightsReset();
	float ccScale = pClientMode->GetColorCorrectionScale();

	UpdateColorCorrectionEntities( pPlayer, ccScale, g_ColorCorrectionList.Base(), g_ColorCorrectionList.Count() );
	UpdateColorCorrectionVolumes( pPlayer, ccScale, g_ColorCorrectionVolumeList.Base(), g_ColorCorrectionVolumeList.Count() );
	CommitColorCorrectionWeights();
}
#else
bool CColorCorrectionMgr::HasNonZeroColorCorrectionWeights() const
{
	return ( m_nActiveWeightCount != 0 );
}
#endif
