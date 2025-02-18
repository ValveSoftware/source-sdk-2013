//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_GIVEAWAYITEMPANEL_H
#define TF_GIVEAWAYITEMPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include "GameEventListener.h"
#include "basemodel_panel.h"
#include "basemodelpanel.h"
#include "tf_shareddefs.h"
#include "econ_item_inventory.h"
#include "econ_item_view.h"
#include "item_model_panel.h"
#include "c_tf_player.h"

class CEconItemView;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CGiveawayPlayerPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CGiveawayPlayerPanel, vgui::EditablePanel );
public:
	CGiveawayPlayerPanel( vgui::Panel *parent, const char *name );

	virtual void	PerformLayout( void );

	void			SetPlayer( CTFPlayer *pPlayer );
	int				GetPlayerIndex( void ) { return m_iPlayerIndex; }
	int				GetBonus( void ) { return m_iBonus; }
	void			SpinBonus( void );
	void			LockBonus( int iRoll );

private:
	vgui::Label		*m_pNameLabel;
	vgui::Label		*m_pScoreLabel;
	Color			m_PlayerColorLocal;
	Color			m_PlayerColorOther;
	int				m_iBonus;
	int				m_iRoll;
	int				m_iPlayerIndex;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFGiveawayItemPanel : public vgui::Frame, public IViewPortPanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFGiveawayItemPanel, vgui::Frame );
public:
	CTFGiveawayItemPanel( IViewPort *pViewPort );
	~CTFGiveawayItemPanel( void );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	ApplySettings( KeyValues *inResourceData );
	virtual void	PerformLayout( void );
	virtual void	OnCommand( const char *command );
	virtual void	FireGameEvent( IGameEvent *event );

	void			SetItem( CEconItemView *pItem );
	void			BuildPlayerList( void );

	// IViewPortPanel overrides
	virtual const char *GetName( void ){ return PANEL_GIVEAWAY_ITEM; }
	virtual void SetData( KeyValues *data ) { return; }
	virtual void Reset(){ Update(); }
	virtual void Update();
	virtual void ShowPanel( bool bShow );
	virtual bool NeedsUpdate( void ){ return true; }
	virtual bool HasInputElements( void ){ return true; }

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ){ return BaseClass::GetVPanel(); }
	virtual bool IsVisible(){ return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ){ BaseClass::SetParent( parent ); }

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_MENUCONTROLS; }

private:
	IViewPort						*m_pViewPort;
	CItemModelPanel					*m_pModelPanel;
	CUtlVector< CGiveawayPlayerPanel * >	m_aPlayerList;
	KeyValues						*m_pPlayerListPanelKVs;
	int								m_iNumActivePlayers;

	// Animation
	bool							m_bBuiltPlayerList;
	float							m_flNextRollStart;
	int								m_iRollingForPlayer;

	CPanelAnimationVarAliasType( int, m_iPlayerYPos, "player_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iPlayerXOffset, "player_xoffset", "0", "proportional_int" );
};

CTFGiveawayItemPanel *OpenGiveawayItemPanel( CEconItemView *pItem );

#endif // TF_GIVEAWAYITEMPANEL_H
