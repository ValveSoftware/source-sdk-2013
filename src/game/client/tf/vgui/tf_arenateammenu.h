//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_ARENATEAMMENU_H
#define TF_ARENATEAMMENU_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"
#include <teammenu.h>
#include "tf_teammenu.h"


//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CTFArenaTeamMenu : public CTeamMenu
{
private:
	DECLARE_CLASS_SIMPLE( CTFArenaTeamMenu, CTeamMenu );
		
public:
	CTFArenaTeamMenu( IViewPort *pViewPort );
	~CTFArenaTeamMenu();

	void Update();
	void ShowPanel( bool bShow );

#ifdef _X360
	CON_COMMAND_MEMBER_F( CTFTeamMenu, "join_team", Join_Team, "Send a jointeam command", 0 );
#endif

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodePressed( vgui::KeyCode code );

	// command callbacks
	virtual void OnCommand( const char *command );

	virtual void LoadMapPage( const char *mapName );

	virtual void OnTick( void );

	virtual const char *GetName( void ) { return PANEL_ARENA_TEAM; }

private:
	
	CTFTeamButton	*m_pAutoTeamButton;
	CTFTeamButton	*m_pSpecTeamButton;
	CExLabel		*m_pSpecLabel;

#ifdef _X360
	CTFFooter		*m_pFooter;
#else
	CExButton		*m_pCancelButton;
#endif

	CSCHintIcon		*m_pCancelHintIcon;
	CSCHintIcon		*m_pJoinAutoHintIcon;
	CSCHintIcon		*m_pJoinSpectatorsHintIcon;

	bool m_bRedDisabled;
	bool m_bBlueDisabled;


private:
	enum { NUM_TEAMS = 3 };

	ButtonCode_t m_iTeamMenuKey;

	void ActivateSelectIconHint( int focus_group_number );
};

#endif // TF_ARENATEAMMENU_H
