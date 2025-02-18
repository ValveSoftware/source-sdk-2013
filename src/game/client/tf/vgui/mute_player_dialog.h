//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef MUTE_PLAYER_DIALOG_H
#define MUTE_PLAYER_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"
#include "vgui_controls/ListPanel.h"
#include "vgui_controls/Button.h"
#include "tf_badge_panel.h"

class CMutePlayerDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CMutePlayerDialog, vgui::Frame );

public:
	CMutePlayerDialog( vgui::Panel *parent );
	~CMutePlayerDialog();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void Activate() OVERRIDE;

private:
	MESSAGE_FUNC( OnItemSelected, "ItemSelected" );
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnThink() OVERRIDE;

	void ToggleMuteStateOfSelectedUser();
	void RefreshPlayerStatus();
	void UpdateBadgePanels();

	virtual void OnKeyCodePressed( vgui::KeyCode code ) OVERRIDE
	{
		if ( code == KEY_XBUTTON_B )
		{
			Close();
		}
		else
		{
			BaseClass::OnKeyCodePressed( code );
		}
	}

	vgui::ListPanel *m_pPlayerList = nullptr;
	vgui::Button *m_pMuteButton = nullptr;

	vgui::ImageList				*m_pImageList = nullptr;
	CUtlMap<CSteamID, int>		m_mapAvatarsToImageList;
	CUtlVector< CTFBadgePanel* > m_pBadgePanels;

	int m_nExtraSpace = 0;

	CPanelAnimationVarAliasType( int, m_iMedalWidth, "medal_width", "20", "proportional_int" );
	CPanelAnimationVar( int, m_iAvatarWidth, "avatar_width", "65" );		// Avatar width doesn't scale with resolution
	CPanelAnimationVarAliasType( int, m_iNameWidth, "name_width", "94", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iScoreWidth, "score_width", "30", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTimeWidth, "time_width", "60", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iStatusWidth, "status_width", "60", "proportional_int" );
};

#endif // MUTE_PLAYER_DIALOG_H
