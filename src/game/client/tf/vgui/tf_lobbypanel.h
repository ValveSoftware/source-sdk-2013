//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_LOBBYPANEL_H
#define TF_LOBBYPANEL_H


#include "cbase.h"
//#include "tf_pvelobbypanel.h"
#include "game/client/iviewport.h"
#include "vgui_controls/TextEntry.h"
#include "tf_matchmaking_shared.h"
#include "vgui_controls/RichText.h"
#include "vgui_controls/CheckButton.h"
#include "tf_gc_client.h"
#include "vgui_controls/PropertySheet.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>
class CBaseLobbyContainerFrame;
class CAvatarImage;
class CTFBadgePanel;
class CMainMenuToolTip;

class CBaseLobbyPanel : public vgui::PropertySheet, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CBaseLobbyPanel, PropertySheet );

	friend class CBaseLobbyContainerFrame;
public:
	CBaseLobbyPanel( vgui::Panel *pParent, CBaseLobbyContainerFrame* pLobbyContainer );
	
	virtual ~CBaseLobbyPanel();

	//
	// Panel overrides
	//
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;

	//
	// CGameEventListener overrides
	//
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	virtual void OnCommand( const char *command ) OVERRIDE;

	int NumPlayersInParty( void ) { return m_vecPlayers.Count(); }

	void ToggleJoinLateCheckButton( void ) { m_pJoinLateCheckButton->SetSelected( !m_pJoinLateCheckButton->IsSelected() );}

	bool IsPartyActiveGroupBoxVisible() { return m_pPartyActiveGroupBox != NULL && m_pPartyActiveGroupBox->IsVisible(); }
	bool IsAnyoneBanned( RTime32 &rtimeExpire ) const;
	bool IsAnyoneLowPriority( RTime32 &rtimeExpire ) const;

	void UpdateControls();

	virtual ETFMatchGroup GetMatchGroup( void ) const = 0;

protected:

	virtual void WriteGameSettingsControls();

	MESSAGE_FUNC_PTR( OnItemLeftClick, "ItemLeftClick", panel );
	MESSAGE_FUNC_PTR( OnItemContextMenu, "ItemContextMenu", panel );
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel );
	MESSAGE_FUNC_PARAMS( OnTradeWithUser, "TradeWithUser", params );

	CPanelAnimationVarAliasType( int, m_iNewWidth, "challenge_new_width", "19", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iChallengeCheckBoxWidth, "challenge_check_box_width", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iAvatarWidth, "avatar_width", "16", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iPlayerNameWidth, "player_name_width", "110", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBannedWidth, "squad_surplus_width", "12", "proportional_int" );

	// Sectioned list panels are the worst
	struct ChatModelPanel_t
	{
		CTFBadgePanel*	m_pBadgeModel;
		CSteamID		m_steamIDOwner;
		uint32			m_nShownLevel;
	};

	CUtlVector< ChatModelPanel_t > m_vecChatBadges;
	vgui::SectionedListPanel	*m_pChatPlayerList;
	vgui::ImageList				*m_pImageList;
	CBaseLobbyContainerFrame		*m_pContainer;

	int m_iWritingPanel;

	int m_iImageIsBanned;
	int m_iImageNew;
	int m_iImageNo;
	int m_iImageRadioButtonYes;
	int m_iImageRadioButtonNo;
	int m_iImageCheckBoxDisabled;
	int m_iImageCheckBoxYes;
	int m_iImageCheckBoxNo;
	int m_iImageCheckBoxMixed;

	struct LobbyPlayerInfo
	{
		CSteamID m_steamID;
		CUtlString m_sName;
		bool m_bHasTicket;
		bool m_bSquadSurplus;
		uint32 m_nBadgeLevel;
		CAvatarImage *m_pAvatarImage;
		uint32 m_nCompletedChallenges; // bitmask of badge slots (not related to the challenge index in the schema!)
		bool m_bIsBanned;
		RTime32 m_rtimeBanExpire;
		RTime32 m_rtimeLowPriorityExpire;
		bool m_bHasCompetitiveAccess;
		uint32 m_unLadderRank;
		bool m_bIsLowPriority;
		uint32 m_unExperienceLevel;
	};

	const LobbyPlayerInfo* GetLobbyPlayerInfo( CSteamID &steamID ) const;
	
	virtual void OnThink() OVERRIDE
	{
		BaseClass::OnThink();
		WriteStatusControls();

		if ( gpGlobals->curtime > m_flRefreshPlayerListTime )
		{
			UpdatePlayerList();
			m_flRefreshPlayerListTime = gpGlobals->curtime + 1.f;
		}
	}


private:

	void SetMatchmakingModeBackground();
	virtual const char* GetResFile() const = 0;
	virtual void ApplyChatUserSettings( const LobbyPlayerInfo& player, KeyValues* pSettings ) const = 0;
	void OnClickedOnPlayer();

	class ChatTextEntry : public vgui::TextEntry
	{
	public:
		ChatTextEntry( vgui::Panel *parent, const char *name ) : vgui::TextEntry( parent, name )
		{
			SetCatchEnterKey( true );
			SetAllowNonAsciiCharacters( true );
			SetDrawLanguageIDAtLeft( true );
		}

		virtual void OnKeyCodeTyped(vgui::KeyCode code)
		{
			if ( code == KEY_ENTER || code == KEY_PAD_ENTER )
			{
				if ( GetTextLength() > 0 )
				{
					int nBufSizeBytes = ( GetTextLength() + 4 ) * sizeof( wchar_t );
					wchar_t *wText = (wchar_t *)stackalloc( nBufSizeBytes );
					GetText( wText, nBufSizeBytes );
					TextEntry::SetText("");

					// Convert to UTF8, which is really what we should
					// use for everything
					int nUtf8BufferSizeBytes = nBufSizeBytes * 2;
					char *szText = (char *)stackalloc( nUtf8BufferSizeBytes );
					V_UnicodeToUTF8( wText, szText, nUtf8BufferSizeBytes );
					// TODO(Universal Parties):
					// GTFGCClientSystem()->SendSteamLobbyChat( CTFGCClientSystem::k_eLobbyMsg_UserChat, szText );
				}
			}
			else if ( code == KEY_TAB )
			{
				// Ignore tab, otherwise vgui will screw up the focus.
				return;
			}
			else
			{
				vgui::TextEntry::OnKeyCodeTyped( code );
			}
		}
	};

	class ChatLog : public vgui::RichText
	{
	public:
		ChatLog( vgui::Panel *parent, const char *name ) : vgui::RichText( parent, name )
		{
		}

		virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
		{
			vgui::RichText::ApplySchemeSettings(pScheme);

			vgui::HFont hFont = pScheme->GetFont( "ChatMiniFont" );
			SetFont( hFont );
		}
	};

	CUtlVector<vgui::Label *> m_vecSearchCriteriaLabels;

	vgui::EditablePanel *m_pSearchActiveGroupBox;
		vgui::Label *m_pSearchActiveTitleLabel;
		vgui::Label *m_pSearchActivePenaltyLabel;
		vgui::EditablePanel *m_pPartyHasLowPriority;

	vgui::CheckButton *m_pJoinLateCheckButton;
	vgui::Label *m_pJoinLateValueLabel;

	vgui::EditablePanel *m_pPartyActiveGroupBox;
		vgui::Button *m_pInviteButton;
		ChatLog *m_pChatLog;
		ChatTextEntry *m_pChatTextEntry;

	int					m_iImageAvatars[MAX_PLAYERS_ARRAY_SAFE];
	CUtlMap<int,int>	m_mapAvatarsToImageList;

	vgui::HFont m_fontPlayerListItem;

	CSOTFParty_State m_eCurrentPartyState;
	float m_flRefreshPlayerListTime;

	CMainMenuToolTip*	m_pToolTip;

	void UpdatePlayerList();
	void WriteStatusControls(); // MM status
	virtual bool ShouldShowLateJoin() const = 0;

	CUtlVector<LobbyPlayerInfo> m_vecPlayers;

	CCallback<CBaseLobbyPanel, PersonaStateChange_t, false> m_sPersonaStateChangedCallback;

	void OnPersonaStateChanged( PersonaStateChange_t *info )
	{
		UpdatePlayerList();
	}
};

#endif //TF_LOBBYPANEL_H
