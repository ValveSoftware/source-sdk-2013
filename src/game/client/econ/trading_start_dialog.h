//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TRADING_START_DIALOG_H
#define TRADING_START_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "econ_controls.h"
#include "vgui_avatarimage.h"

// Trading Dialog states
enum
{
	TDS_SELECTING_PLAYER,
	TDS_SELECTING_FROM_FRIENDS,
	TDS_SELECTING_FROM_SERVER,
	TDS_SELECTING_FROM_PROFILE,

	TDS_NUM_STATES,
};

// Button that displays the name & avatar image of a potential trade target
class CTradeTargetPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTradeTargetPanel, vgui::EditablePanel );
public:
	CTradeTargetPanel( vgui::Panel *parent, const char *name ) : vgui::EditablePanel( parent, name )
	{
		m_pAvatar = new CAvatarImagePanel( this, "avatar" );
		m_pButton = new CExButton( this, "button", "", parent );
	}
	~CTradeTargetPanel( void )
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
// A dialog that allows users to select who they want to trade with.
//-----------------------------------------------------------------------------
class CTradingStartDialog : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTradingStartDialog, vgui::EditablePanel );
public:
	CTradingStartDialog( vgui::Panel *parent );
	~CTradingStartDialog( void );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );
	virtual void FireGameEvent( IGameEvent *event );

	void	Close( void );
	void	Reset( void );
	void	UpdateState( void );
	void	SetupSelectFriends( void );
	void	SetupSelectServer( void );
	void	SetupSelectProfile( void );
	void	UpdatePlayerList( void );

	void	SendGiftTo( CSteamID steamID );
	void	StartTradeWith( CSteamID steamID );
	bool	ExtractSteamIDFromURL( char *inputURL );
	void	OnLookupAccountResponse( uint64 iAccountID );
	bool	IsInGiftMode( ) const { return m_bGiftMode; }
	void	SetGift( CEconItemView* pGiftItem );

	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );

private:
	struct trade_partner_info_t
	{
		CSteamID m_steamID;
		CUtlString m_name;
	};
	vgui::EditablePanel				*m_pStatePanels[TDS_NUM_STATES];
	int								m_iCurrentState;
	CExButton						*m_pSelectFromServerButton;
	CExButton						*m_pCancelButton;

	vgui::EditablePanel				*m_pPlayerList;
	vgui::ScrollableEditablePanel	*m_pPlayerListScroller;
	CUtlVector<trade_partner_info_t> m_PlayerInfoList;
	CUtlVector<CTradeTargetPanel*>	m_pPlayerPanels;
	KeyValues						*m_pButtonKV;
	bool							m_bReapplyButtonKVs;
	vgui::Label						*m_pURLFailLabel;
	vgui::Label						*m_pURLSearchingLabel;
	CEconItemView					m_giftItem;
	bool							m_bGiftMode;
};

CTradingStartDialog *OpenTradingStartDialog( vgui::Panel *pParent, CEconItemView* pOptGiftItem = NULL );

#endif // TRADING_START_DIALOG_H
