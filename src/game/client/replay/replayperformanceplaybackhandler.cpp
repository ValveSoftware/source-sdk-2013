//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replayperformanceplaybackhandler.h"
#include "replay/replaycamera.h"
#include "replay/vgui/replayperformanceeditor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

class CReplayPerformancePlaybackHandler : public IReplayPerformancePlaybackHandler
{
public:
	CReplayPerformancePlaybackHandler() {}

private:
	//
	// IReplayPerformancePlaybackController
	//
	virtual void OnEvent_Camera_Change_FirstPerson( float flTime, int nEntityIndex )
	{
		ReplayCamera()->SetMode( OBS_MODE_IN_EYE );
		Editor_UpdateCameraModeIcon( CAM_FIRST );
	}

	virtual void OnEvent_Camera_Change_ThirdPerson( float flTime, int nEntityIndex )
	{
		ReplayCamera()->SetMode( OBS_MODE_CHASE );
		Editor_UpdateCameraModeIcon( CAM_THIRD );
	}

	virtual void OnEvent_Camera_Change_Free( float flTime )
	{
		ReplayCamera()->SetMode( OBS_MODE_ROAMING );
		Editor_UpdateCameraModeIcon( CAM_FREE );
	}

	virtual void OnEvent_Camera_ChangePlayer( float flTime, int nEntIndex )
	{
		ReplayCamera()->SetPrimaryTarget( nEntIndex );
	}

	virtual void OnEvent_Camera_SetView( const SetViewParams_t &params )
	{
		ReplayCamera()->OverrideView( params.m_pOrigin, params.m_pAngles, params.m_flFov );
		Editor_UpdateFreeCamSettings( params );
	}

	virtual void OnEvent_TimeScale( float flTime, float flScale )
	{
		// Update the slider position
		Editor_UpdateTimeScale( flScale );
	}

	// ---

	void Editor_UpdateCameraModeIcon( CameraMode_t nMode )
	{
		CReplayPerformanceEditorPanel *pEditor = ReplayUI_GetPerformanceEditor();
		if ( !pEditor )
			return;

		pEditor->UpdateCameraSelectionPosition( nMode );
	}

	void Editor_UpdateFreeCamSettings( const SetViewParams_t &params )
	{
		CReplayPerformanceEditorPanel *pEditor = ReplayUI_GetPerformanceEditor();
		if ( !pEditor )
			return;

		pEditor->UpdateFreeCamSettings( params );
	}

	void Editor_UpdateTimeScale( float flScale )
	{
		CReplayPerformanceEditorPanel *pEditor = ReplayUI_GetPerformanceEditor();
		if ( !pEditor )
			return;

		pEditor->UpdateTimeScale( flScale );
	}
};

//-----------------------------------------------------------------------------

CReplayPerformancePlaybackHandler s_ReplayPerformancePlaybackHandler;
IReplayPerformancePlaybackHandler *g_pReplayPerformancePlaybackHandler = &s_ReplayPerformancePlaybackHandler;

//-----------------------------------------------------------------------------

#endif
