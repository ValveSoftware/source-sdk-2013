//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYPERFORMANCESAVEDLG_H
#define REPLAYPERFORMANCESAVEDLG_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------

class CReplay;

//-----------------------------------------------------------------------------

typedef void (*OnConfirmSaveCallback)( bool bConfirmed, wchar_t *pTitle, void *pContext );

//-----------------------------------------------------------------------------

void ReplayUI_ShowPerformanceSaveDlg( OnConfirmSaveCallback pfnCallback, void *pContext, CReplay *pReplay,
									 bool bExitEditorWhenDone );
bool ReplayUI_IsPerformanceSaveDlgOpen();

//-----------------------------------------------------------------------------

#endif // REPLAYPERFORMANCESAVEDLG_H
