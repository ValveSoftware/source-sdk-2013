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

	int				GetChatInputOffset( void );

	virtual Color	GetClientColor( int clientIndex );
};

#endif	//CS_HUD_CHAT_H