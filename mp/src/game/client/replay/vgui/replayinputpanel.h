//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAY_INPUT_PANEL_H
#define REPLAY_INPUT_PANEL_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------

#include "replay/replayhandle.h"

//-----------------------------------------------------------------------------
// Purpose: Show Replay input panel for entering a title, etc.
//-----------------------------------------------------------------------------
void ShowReplayInputPanel( ReplayHandle_t hReplay );

//-----------------------------------------------------------------------------
// Purpose: Is the panel visible?
//-----------------------------------------------------------------------------
bool IsReplayInputPanelVisible();

#endif // REPLAY_INPUT_PANEL_H
