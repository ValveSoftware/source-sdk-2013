//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================//

#ifndef REPLAY_GAMESTATS_H
#define REPLAY_GAMESTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "replay/replayhandle.h"

class CReplayRenderDialog;
struct RenderMovieParams_t;

class CReplayGameStatsHelper
{
public:
	CReplayGameStatsHelper();

	// Adding "Time" before uploading.
	void UploadError( KeyValues *pData, bool bIncludeTimeField );

#if defined( CLIENT_DLL )
	void SW_ReplayStats_WriteRenderDataStart( const RenderMovieParams_t& RenderParams, const CReplayRenderDialog *pDlg );
	void SW_ReplayStats_WriteRenderDataEnd( const RenderMovieParams_t& RenderParams, const char *pEndReason );

private:
	void SW_ReplayStats_WriteRenderData( bool bStarting, const RenderMovieParams_t& RenderParams, const CReplayRenderDialog *pDlg, const char *pEndReason = NULL );
#endif
};

CReplayGameStatsHelper &GetReplayGameStatsHelper();

#endif // REPLAY_GAMESTATS_H
