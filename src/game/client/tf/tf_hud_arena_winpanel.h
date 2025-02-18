//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_ARENA_WINPANEL_H
#define TF_ARENA_WINPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "tf_gamerules.h"

using namespace vgui;

#define NUM_NOTABLE_PLAYERS	3
#define NUM_CATEGORIES	2
#define NUM_ITEMS_PER_CATEGORY	3

class CTFArenaWinPanel : public EditablePanel, public CGameEventListener, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFArenaWinPanel, EditablePanel );

public:
	CTFArenaWinPanel( IViewPort *pViewPort );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void FireGameEvent( IGameEvent * event );
	virtual void OnTick();

	virtual int GetRenderGroupPriority() { return 70; }
	void	SetupPlayerStats( void );

	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
	virtual const char *GetName( void ){ return PANEL_ARENA_WIN; }
	virtual void SetData( KeyValues *data ){ return; }
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow ){ };
	virtual void SetVisible( bool state ); 
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	bool	IsFlawlessVictory( void );

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_NONE; }

private:
	EditablePanel *m_pTeamScorePanel;
	EditablePanel *m_pWinnerPanel;
	EditablePanel *m_pLoserPanel;

	Panel		*m_pArenaStreakPanel;
	Panel		*m_pArenaStreakLabel;

	float	m_flTimeUpdateTeamScore;
	float	m_flFlipScoresTimes;
	int		m_iBlueTeamScore;
	int		m_iRedTeamScore;
	
	bool	m_bShouldBeVisible;
	bool	m_bWasFlawlessVictory;

	int		m_iWinningPlayerTeam;

	CUtlVector<PlayerArenaRoundScore_t> m_vecPlayerScore;

	wchar_t m_wzTeamLose[256];
};

#endif //TF_ARENA_WINPANEL_H
