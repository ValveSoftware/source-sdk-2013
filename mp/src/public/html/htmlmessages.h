//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//=============================================================================//

#ifndef HTMLMESSAGES_H
#define HTMLMESSAGES_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Commands we IPC across to the html thread and get responses for
//-----------------------------------------------------------------------------
enum EHTMLCommands
{
	// input events
	eHTMLCommands_KeyUp,
	eHTMLCommands_KeyDown,
	eHTMLCommands_KeyChar,
	eHTMLCommands_MouseDown,
	eHTMLCommands_MouseUp,
	eHTMLCommands_MouseDblClick,
	eHTMLCommands_MouseWheel,	
	eHTMLCommands_MouseMove,
	eHTMLCommands_MouseLeave,

	// command events
	eHTMLCommands_BrowserCreate,
	eHTMLCommands_BrowserRemove,
	eHTMLCommands_BrowserErrorStrings,
	eHTMLCommands_BrowserSize,
	eHTMLCommands_BrowserPosition,
	eHTMLCommands_PostURL,
	eHTMLCommands_StopLoad,
	eHTMLCommands_Reload,
	eHTMLCommands_GoForward,
	eHTMLCommands_GoBack,
	eHTMLCommands_Copy,
	eHTMLCommands_Paste,
	eHTMLCommands_ExecuteJavaScript,
	eHTMLCommands_SetFocus,
	eHTMLCommands_HorizontalScrollBarSize,
	eHTMLCommands_VerticalScrollBarSize,
	eHTMLCommands_Find,
	eHTMLCommands_StopFind,
	eHTMLCommands_SetHorizontalScroll,
	eHTMLCommands_SetVerticalScroll,
	eHTMLCommands_SetZoomLevel,
	eHTMLCommands_ViewSource,
	eHTMLCommands_NeedsPaintResponse,
	eHTMLCommands_AddHeader,
	eHTMLCommands_GetZoom,
	eHTMLCommands_FileLoadDialogResponse,
	eHTMLCommands_LinkAtPosition,
	eHTMLCommands_ZoomToElementAtPosition,
	eHTMLCommands_SavePageToJPEG,
	eHTMLCommands_JSAlert,
	eHTMLCommands_JSConfirm,
	eHTMLCommands_CanGoBackandForward,
	eHTMLCommands_OpenSteamURL,
	eHTMLCommands_SizePopup,
	eHTMLCommands_SetCookie,
	eHTMLCommands_SetTargetFrameRate,
	eHTMLCommands_FullRepaint,
	eHTMLCommands_SetPageScale,
	eHTMLCommands_RequestFullScreen,
	eHTMLCommands_ExitFullScreen,
	eHTMLCommands_GetCookiesForURL,
	eHTMLCommands_ZoomToCurrentlyFocusedNode,
	eHTMLCommands_CloseFullScreenFlashIfOpen,
	eHTMLCommands_PauseFullScreenFlashMovieIfOpen,
	eHTMLCommands_GetFocusedNodeValue,

	// output back to the main thread
	eHTMLCommands_BrowserCreateResponse,
	eHTMLCommands_BrowserReady,
	eHTMLCommands_URLChanged,
	eHTMLCommands_FinishedRequest,
	eHTMLCommands_StartRequest,
	eHTMLCommands_ShowPopup,
	eHTMLCommands_HidePopup,
	eHTMLCommands_OpenNewTab,
	eHTMLCommands_PopupHTMLWindow,
	eHTMLCommands_PopupHTMLWindowResponse,
	eHTMLCommands_SetHTMLTitle,
	eHTMLCommands_LoadingResource,
	eHTMLCommands_StatusText,
	eHTMLCommands_SetCursor,
	eHTMLCommands_FileLoadDialog,
	eHTMLCommands_ShowToolTip,
	eHTMLCommands_UpdateToolTip,
	eHTMLCommands_HideToolTip,
	eHTMLCommands_SearchResults,
	eHTMLCommands_Close,
	eHTMLCommands_VerticalScrollBarSizeResponse,
	eHTMLCommands_HorizontalScrollBarSizeResponse,
	eHTMLCommands_GetZoomResponse,
	eHTMLCommands_StartRequestResponse,
	eHTMLCommands_NeedsPaint,
	eHTMLCommands_LinkAtPositionResponse,
	eHTMLCommands_ZoomToElementAtPositionResponse,
	eHTMLCommands_JSDialogResponse,
	eHTMLCommands_ScaleToValueResponse,
	eHTMLCommands_RequestFullScreenResponse,
	eHTMLCommands_GetCookiesForURLResponse,
	eHTMLCommands_NodeGotFocus,
	eHTMLCommands_SavePageToJPEGResponse,
	eHTMLCommands_GetFocusedNodeValueResponse,

	eHTMLCommands_None,

};

#endif // HTMLMESSAGES_H