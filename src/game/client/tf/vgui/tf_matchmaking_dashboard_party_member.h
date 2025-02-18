//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCHMAKING_DASHBOARD_PARTY_MEMBER_H
#define TF_MATCHMAKING_DASHBOARD_PARTY_MEMBER_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "tf_controls.h"

void CreateSwoop( int nX, int nY, int nWide, int nTall, float flDelay, bool bDown );

class CDashboardPartyMember : public vgui::EditablePanel 
							, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CDashboardPartyMember, EditablePanel );
public:
	enum EMemberState
	{
		MEMBER_NONE,
		MEMBER_PENDING_OUTGOING_INVITE,
		MEMBER_PENDING_INCOMING_JOIN_REQUEST,
		MEMBER_PRESENT
	};

	CDashboardPartyMember( vgui::Panel *parent, const char *panelName );

	// Panel overrides
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnCommand( const char *command );

	// GameEventListener override
	virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE;

	virtual void PostChildPaint() OVERRIDE;

	// Message handlers
	MESSAGE_FUNC( DoLeavyParty, "Context_LeaveParty" );
	MESSAGE_FUNC( DoKickFromParty, "Context_KickFromParty" );
	MESSAGE_FUNC( DoJoinServer, "Context_JoinServer" );
	MESSAGE_FUNC( DoSendMessage, "Context_SendMessage" );
	MESSAGE_FUNC( DoOpenSettings, "Context_OpenSettings" );
	MESSAGE_FUNC( DoPromoteToLeader, "Context_PromoteToLeader" );
private:

	bool BIsLocalPlayerSlot() const { return m_nDisplayPartySlot == 0; }
	bool BMemberIsLeader() const;
	void UpdatePartyMemberSteamID();

	CExImageButton* m_pInteractButton;
	EMemberState m_eMemberState = MEMBER_NONE;
	CSteamID m_steamIDPartyMember;
	int m_nDisplayPartySlot = 0;
	int m_nBackendPartySlot = 0;
	class CAvatarImagePanel* m_pAvatar;
};

#endif // TF_MATCHMAKING_DASHBOARD_PARTY_MEMBER_H
