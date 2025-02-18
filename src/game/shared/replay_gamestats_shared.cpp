//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================

#include "cbase.h"
#include "replay_gamestats_shared.h"
#include "steamworks_gamestats.h"

#if defined( CLIENT_DLL )
#include "../client/replay/genericclassbased_replay.h"
#include "replay/replayvideo.h"
#include "replay/vgui/replaybrowserrenderdialog.h"
#endif

#include "replay/rendermovieparams.h"
#include "replay/performance.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


CReplayGameStatsHelper::CReplayGameStatsHelper()
{
}

void CReplayGameStatsHelper::UploadError( KeyValues *pData, bool bIncludeTimeField )
{
	if ( bIncludeTimeField )
	{
//		pData->SetInt( "Time", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pData );
}

CReplayGameStatsHelper &GetReplayGameStatsHelper()
{
	static CReplayGameStatsHelper s_Instance;
	return s_Instance;
}

#if defined( CLIENT_DLL )
void CReplayGameStatsHelper::SW_ReplayStats_WriteRenderDataStart( const RenderMovieParams_t& RenderParams, const CReplayRenderDialog *pDlg )
{
	SW_ReplayStats_WriteRenderData( true, RenderParams, pDlg, NULL );
}

void CReplayGameStatsHelper::SW_ReplayStats_WriteRenderDataEnd( const RenderMovieParams_t& RenderParams, const char *pEndReason )
{
	SW_ReplayStats_WriteRenderData( false, RenderParams, NULL, pEndReason );
}

void CReplayGameStatsHelper::SW_ReplayStats_WriteRenderData( bool bStarting, const RenderMovieParams_t& RenderParams,
															 const CReplayRenderDialog *pDlg, const char *pEndReason/*=NULL*/ )
{
#if !defined( NO_STEAM )

#if defined( REPLAY_ENABLED )
	static uint32 s_iReplayRenderCounter = 0;

	ReplayHandle_t hReplay = RenderParams.m_hReplay;
	CGenericClassBasedReplay *pReplay = GetGenericClassBasedReplay( hReplay );
	if ( !pReplay )
		return;

	KeyValues* pKVData = new KeyValues( "TF2ReplayRenders" );

	// Base settings/DB key.
	pKVData->SetInt( "RenderCounter", s_iReplayRenderCounter++ );
	pKVData->SetInt( "ReplayHandle", hReplay );
	pKVData->SetInt( "PeformanceIndex", RenderParams.m_iPerformance );

	// Dialog-specific
	if ( pDlg )
	{
		pKVData->SetInt( "ShowAdvancedChecked", pDlg->m_pShowAdvancedOptionsCheck->IsSelected() );
		pKVData->SetString( "CodecID", ReplayVideo_GetCodec( pDlg->m_pCodecCombo->GetActiveItem() ).m_pName );
		pKVData->SetInt( "RenderQualityPreset", pDlg->m_iQualityPreset );
	}

	// Render settings
	CFmtStr fmtResolution( "%ix%i", RenderParams.m_Settings.m_nWidth, RenderParams.m_Settings.m_nHeight );
	pKVData->SetInt( "QuitWhenDoneChecked", RenderParams.m_bQuitWhenFinished );
	pKVData->SetString( "ResolutionID", fmtResolution.Access() );
	pKVData->SetInt( "AAEnabled", RenderParams.m_Settings.m_bAAEnabled );
	pKVData->SetInt( "MotionBlurEnabled", RenderParams.m_Settings.m_bMotionBlurEnabled );
	pKVData->SetInt( "MotionBlurQuality", RenderParams.m_Settings.m_nMotionBlurQuality );
	pKVData->SetInt( "RenderQuality", RenderParams.m_Settings.m_nEncodingQuality);
	pKVData->SetInt( "ExportRawChecked", RenderParams.m_bExportRaw );
	pKVData->SetInt( "FPSUPF", RenderParams.m_Settings.m_FPS.GetUnitsPerFrame() );
	pKVData->SetInt( "FPSUPS", RenderParams.m_Settings.m_FPS.GetUnitsPerSecond() );

	// Replay content.
	pKVData->SetString( "MapID", pReplay->GetMapName() );
	pKVData->SetString( "PlayerClassID", pReplay->GetPlayerClass() );
	pKVData->SetInt( "ReplayLengthRealtime", pReplay->GetDeathTick() - pReplay->GetSpawnTick() );

	CReplayPerformance *pPerformance = pReplay->GetPerformance( RenderParams.m_iPerformance );
	if ( pPerformance )
	{
		int iPerformanceStartTick = pPerformance->HasInTick() ? pPerformance->GetTickIn() : pReplay->GetSpawnTick(),
			iPeformanceEndTick	  = pPerformance->HasOutTick() ? pPerformance->GetTickOut() : pReplay->GetDeathTick();
		pKVData->SetInt( "TakeLengthRealtime", iPeformanceEndTick - iPerformanceStartTick );
	}
	else
	{
		pKVData->SetInt( "TakeLengthRealtime", pReplay->GetDeathTick() - pReplay->GetSpawnTick() );
	}

	// Start or end time
	pKVData->SetInt( bStarting ? "StartRenderTime" : "EndRenderTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );

	// Write end reason
	if ( !bStarting && pEndReason )
	{
		pKVData->SetString( "EndRenderReasonID", pEndReason );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif // REPLAY_ENABLED

#endif	// !defined( NO_STEAM )
}
#endif // defined( CLIENT_DLL )
