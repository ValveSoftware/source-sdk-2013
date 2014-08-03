//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose : Singleton manager for color correction on the client
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "tier0/vprof.h"
#include "colorcorrectionmgr.h"


//------------------------------------------------------------------------------
// Singleton access
//------------------------------------------------------------------------------
static CColorCorrectionMgr s_ColorCorrectionMgr;
CColorCorrectionMgr *g_pColorCorrectionMgr = &s_ColorCorrectionMgr;


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


//------------------------------------------------------------------------------
// Modify color correction weights
//------------------------------------------------------------------------------
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

void CColorCorrectionMgr::ResetColorCorrectionWeights()
{
	VPROF_("ResetColorCorrectionWeights", 2, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, false, 0);
	// FIXME: Where should I put this? It needs to happen prior to SimulateEntities()
	// which is where the client thinks for c_colorcorrection + c_colorcorrectionvolumes
	// update the color correction weights.
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->ResetLookupWeights();
	m_nActiveWeightCount = 0;
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
bool CColorCorrectionMgr::HasNonZeroColorCorrectionWeights() const
{
	return ( m_nActiveWeightCount != 0 );
}
