//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CS_HUD_CHAT_H
#define CS_HUD_CHAT_H
#ifdef _WIN32
#pragma once
#endif

#include <hud_basechat.h>

class CHudChatLine : public CBaseHudChatLine
{
	DECLARE_CLASS_SIMPLE( CHudChatLine, CBaseHudChatLine );

public:
	CHudChatLine( vgui::Panel *parent, const char *panelName ) : CBaseHudChatLine( parent, panelName ) {}

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);

	void			MsgFunc_SayText(bf_read &msg);



private:
	CHudChatLine( const CHudChatLine & ); // not defined, not accessible
};

//-----------------------------------------------------------------------------
// Purpose: The prompt and text entry area for chat messages
//-----------------------------------------------------------------------------
class CHudChatInputLine : public CBaseHudChatInputLine
{
	DECLARE_CLASS_SIMPLE( CHudChatInputLine, CBaseHudChatInputLine );
	
public:
	CHudChatInputLine( CBaseHudChat *parent, char const *panelName ) : CBaseHudChatInputLine( parent, panelName ) {}

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);
};

class CHudChat : public CBaseHudChat
{
	DECLARE_CLASS_SIMPLE( CHudChat, CBaseHudChat );

public:
	CHudChat( const char *pElementName );

	virtual void	CreateChatInputLine( void );
	virtual void	CreateChatLines( void );

	virtual void	Init( void );
	virtual void	Reset( void );
	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PerformLayout( void );
	virtual void OnTick( void );
	virtual void StartMessageMode( int iMessageModeType );
	virtual void StopMessageMode( void );
	virtual void OnCursorMoved( int x, int y );
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnMouseReleased( vgui::MouseCode code );
	virtual void OnMouseCaptureLost( void );
	virtual void ChatPrintf( int iPlayerIndex, int iFilter, PRINTF_FORMAT_STRING const char *fmt, ... ) FMTFUNCTION( 4, 5 );

	int				GetChatInputOffset( void );

	virtual Color	GetClientColor( int clientIndex );

private:
	enum WindowDragMode
	{
		WINDOW_DRAG_NONE = 0,
		WINDOW_DRAG_LEFT = 1,
		WINDOW_DRAG_RIGHT = 2,
		WINDOW_DRAG_TOP = 4,
		WINDOW_DRAG_BOTTOM = 8,
		WINDOW_DRAG_MOVE = 16
};

	void UpdateLayout( void );
	void UpdateMessageModePrompt( void );
	void RestoreWindowBounds( int screenWide, int screenTall, float scale );
	void SaveWindowBounds( void );
	int GetWindowDragMode( int x, int y );
	void SetWindowCursor( int dragMode );
	void FinishWindowDrag( bool releaseCapture );

	int m_iLastScreenWide;
	int m_iLastScreenTall;
	bool m_bLastActive;
	bool m_bWindowBoundsInitialized;
	int m_iWindowDragMode;
	int m_iDragStartCursorX;
	int m_iDragStartCursorY;
	int m_iDragStartX;
	int m_iDragStartY;
	int m_iDragStartWide;
	int m_iDragStartTall;
	int m_iWindowMargin;
	int m_iMinWindowWide;
	int m_iMinWindowTall;
	int m_iHeaderTall;
	int m_iResizeGripSize;
};

#endif	//CS_HUD_CHAT_H
