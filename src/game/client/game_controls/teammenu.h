//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEAMMENU_H
#define TEAMMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>

#include <game/client/iviewport.h>

#include <vgui/KeyCode.h>
#include <utlvector.h>

namespace vgui
{
	class RichText;
	class HTML;
}
class TeamFortressViewport;


//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CTeamMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CTeamMenu, vgui::Frame );

public:
	CTeamMenu(IViewPort *pViewPort);
	virtual ~CTeamMenu();

	virtual const char *GetName( void ) { return PANEL_TEAM; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset() {};
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_IN_GAME_HUD; }

public:
	
	void AutoAssign();
	
protected:

	// int GetNumTeams() { return m_iNumTeams; }
	
	// VGUI2 overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodePressed(vgui::KeyCode code);

	// helper functions
	virtual void SetLabelText(const char *textEntryName, const char *text);
	virtual void LoadMapPage( const char *mapName );
	// virtual void MakeTeamButtons( void );
	
	// command callbacks
	// MESSAGE_FUNC_INT( OnTeamButton, "TeamButton", team );

	IViewPort	*m_pViewPort;
	vgui::RichText *m_pMapInfo;
	vgui::HTML *m_pMapInfoHTML;
//	int m_iNumTeams;
	ButtonCode_t m_iJumpKey;
	ButtonCode_t m_iScoreBoardKey;

	char m_szMapName[ MAX_PATH ];
};


#endif // TEAMMENU_H
