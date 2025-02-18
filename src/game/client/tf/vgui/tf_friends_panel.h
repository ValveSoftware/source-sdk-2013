//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_FRIENDS_PANEL_H
#define TF_FRIENDS_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "tf_controls.h"

namespace vgui
{
	class Label;
	class Button;
	class Menu;
}

class CExButton;
class CAvatarImagePanel;

class CSteamFriendPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CSteamFriendPanel, vgui::EditablePanel );
public:
	CSteamFriendPanel( Panel *parent, const char *panelName );
	void SetSteamID( const CSteamID& steamID );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	const CSteamID& GetFriendSteamID() const { return m_steamID; }

	MESSAGE_FUNC( DoInviteToParty, "Context_InviteParty" );
	MESSAGE_FUNC( DoJoinParty, "Context_JoinParty" );
	MESSAGE_FUNC( DoJoinServer, "Context_JoinServer" );
	MESSAGE_FUNC( DoSendMessage, "Context_SendMessage" );

private:
	void UpdateControls();

	CSteamID m_steamID;
	CAvatarImagePanel* m_pAvatar;
	vgui::Label* m_pStatusLabel;
	CExButton* m_pInteractButton;
	vgui::Menu* m_pContextMenu = NULL;
};

class CSteamFriendsListPanel : public CExScrollingEditablePanel
{
	DECLARE_CLASS_SIMPLE( CSteamFriendsListPanel, CExScrollingEditablePanel );
public:
	CSteamFriendsListPanel( Panel *parent, const char *panelName );
	virtual ~CSteamFriendsListPanel();

	virtual void PerformLayout() OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void OnThink();
private:

	CCallback<CSteamFriendsListPanel, PersonaStateChange_t, false> m_sPersonaStateChangedCallback;
	void OnPersonaStateChanged( PersonaStateChange_t *info );

	CCallback<CSteamFriendsListPanel, FriendRichPresenceUpdate_t, false> m_sRichPresenceStateChangedCallback;
	void OnRichPresenceChanged( FriendRichPresenceUpdate_t *info );

	void PositionFriendsList();
	void UpdateFriendsList();
	void GetSortedFriends( CUtlVector< CSteamFriendPanel* >& vecSortedFriends ) const;
	void DirtyPotentialFriendsList();
	void DirtyFriendsPanelsList();
	void ProcessFriends();
	void PruneKnownFriends();
	void FriendStateChange( CSteamID steamIDFriend );

	CPanelAnimationVar( int, m_nNumColumns, "columns_count", "1" );
	CPanelAnimationVarAliasType( int, m_nXInset, "inset_x", "10", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nYInset, "inset_y", "10", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nColumnGap, "column_gap", "10", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nRowGap, "row_gap", "5", "proportional_int" );

	CUtlMap< CSteamID, CSteamFriendPanel* > m_mapFriendsPanels;
	KeyValues* m_pKVFriendPanel = NULL;
	struct PotentialFriend_t
	{
		CSteamID m_steamID;
		EPersonaState eState;
		bool bShow = true;
	};
	CUtlMap< CSteamID, PotentialFriend_t > m_mapKnownFriends;
	uint32 m_nLastProcessedFriendIndex = 0;
	uint32 m_nLastProcessedPotentialFriend = 0;
	bool m_bListDirty = true;
	bool m_bPanelsDirty = false;
	bool m_bListNeedsResort = false;
	double m_fNextSortTime = FLT_MAX;
};

#endif // TF_FRIENDS_PANEL_H
