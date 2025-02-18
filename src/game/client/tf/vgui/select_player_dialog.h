//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SELECT_PLAYER_DIALOG_H
#define SELECT_PLAYER_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "tf_controls.h"
#include "vgui_avatarimage.h"

// Select Player Dialog states
enum
{
	SPDS_SELECTING_PLAYER,
	SPDS_SELECTING_FROM_FRIENDS,
	SPDS_SELECTING_FROM_SERVER,

	SPDS_NUM_STATES,
};

// Button that displays the name & avatar image of a potential target
class CSelectPlayerTargetPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CSelectPlayerTargetPanel, vgui::EditablePanel );
public:
	CSelectPlayerTargetPanel( vgui::Panel *parent, const char *name ) : vgui::EditablePanel( parent, name )
	{
		m_pAvatar = new CAvatarImagePanel( this, "avatar" );
		m_pButton = new CExButton( this, "button", "", parent );
	}
	~CSelectPlayerTargetPanel( void )
	{
		m_pAvatar->MarkForDeletion();
		m_pButton->MarkForDeletion();
	}

	void	SetInfo( const CSteamID &steamID, const char *pszName );

	CAvatarImagePanel	*GetAvatar( void ) { return m_pAvatar; }
	CExButton			*GetButton( void ) { return m_pButton; }

private:
	// Embedded panels
	CAvatarImagePanel	*m_pAvatar;
	CExButton			*m_pButton;
};

//-----------------------------------------------------------------------------
// A dialog that allows users to select who they want to do something with
//-----------------------------------------------------------------------------
class CSelectPlayerDialog : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CSelectPlayerDialog, vgui::EditablePanel );
public:
	CSelectPlayerDialog( vgui::Panel *parent );
	~CSelectPlayerDialog( void );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );

	void	UpdateState( void );
	virtual void UpdatePlayerList( void );
	virtual void Reset( void );
	virtual void SetupSelectFriends( void );
	virtual void SetupSelectServer( bool bFriendsOnly );

	virtual bool AllowOutOfGameFriends() { return false; }

	virtual void OnSelectPlayer( const CSteamID &steamID ) = 0;

protected:
	virtual const char *GetResFile() { return "resource/ui/SelectPlayerDialog.res"; }

	struct partner_info_t
	{
		CSteamID m_steamID;
		CUtlString m_name;
	};
	
	static int SortPartnerInfoFunc( const partner_info_t *pA, const partner_info_t *pB );

	vgui::EditablePanel				*m_pStatePanels[SPDS_NUM_STATES];
	int								m_iCurrentState;
	CExButton						*m_pSelectFromServerButton;
	CExButton						*m_pCancelButton;

	vgui::EditablePanel				*m_pPlayerList;
	vgui::ScrollableEditablePanel	*m_pPlayerListScroller;
	CUtlVector<partner_info_t>		m_PlayerInfoList;
	CUtlVector<CSelectPlayerTargetPanel*>	m_pPlayerPanels;
	KeyValues						*m_pButtonKV;
	bool							m_bReapplyButtonKVs;
	bool							m_bAllowSameTeam;
	bool							m_bAllowOutsideServer;
};

#endif // SELECT_PLAYER_DIALOG_H
