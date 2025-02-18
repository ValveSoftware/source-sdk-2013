//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCHMAKING_DASHBOARD_CHAT_H
#define TF_MATCHMAKING_DASHBOARD_CHAT_H
#ifdef _WIN32
#pragma once
#endif


#include <vgui_controls/EditablePanel.h>
#include "tf_controls.h"
#include "vgui_controls/TextEntry.h"
#include "tf_gc_client.h"
#include "tf_hud_chat.h"

class ChatTextEntry : public vgui::TextEntry
{
	DECLARE_CLASS_SIMPLE( ChatTextEntry, vgui::TextEntry );
public:
	ChatTextEntry( vgui::Panel *parent, const char *name );
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void OnKeyCodeTyped(vgui::KeyCode code) OVERRIDE;
	virtual void OnKillFocus() OVERRIDE;
};

class CPartyChatPanel : public CExpandablePanel
					  , public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CPartyChatPanel, CExpandablePanel );
public:
	CPartyChatPanel( Panel *parent, const char *panelName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnSizeChanged( int wide, int tall ) OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	virtual void OnToggleCollapse( bool bIsExpanded ) OVERRIDE;

private:

	MESSAGE_FUNC_PARAMS( OnShowChatEntry, "ShowChatEntry", params );

	void PositionPopupStack();

	enum EFontSize
	{
		SMALL = 0,
		MEDIUM,
		LARGE,
	};

	EFontSize m_eFontSize = MEDIUM;
	CUtlString m_strChatLogFont[3];
	HFont m_fonts[3];
	vgui::RichText* m_pChatLog;
	ChatTextEntry* m_pChatEntry;

	Color m_colorChatDefault;
	Color m_colorChatPlayerName;
	Color m_colorChatPlayerChatText;
	Color m_colorChatPartyEvent;

	CSteamID m_localSteamID;
	static CUtlVector< ChatMessage_t > sm_vecChatMessages;
};

#endif // TF_MATCHMAKING_DASHBOARD_CHAT_H
