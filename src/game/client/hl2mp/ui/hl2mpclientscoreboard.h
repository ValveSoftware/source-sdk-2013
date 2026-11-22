//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CHL2MPCLIENTSCOREBOARDDIALOG_H
#define CHL2MPCLIENTSCOREBOARDDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <clientscoreboarddialog.h>

namespace vgui
{
class Label;
}

//-----------------------------------------------------------------------------
// Purpose: Game ScoreBoard
//-----------------------------------------------------------------------------
class CHL2MPClientScoreBoardDialog : public CClientScoreBoardDialog
{
private:
	DECLARE_CLASS_SIMPLE(CHL2MPClientScoreBoardDialog, CClientScoreBoardDialog);

public:
	CHL2MPClientScoreBoardDialog(IViewPort *pViewPort);
	~CHL2MPClientScoreBoardDialog() OVERRIDE;

	void Update() OVERRIDE;
	void FireGameEvent( IGameEvent *event ) OVERRIDE;

protected:
	// scoreboard overrides
	void InitScoreboardSections() OVERRIDE;
	void UpdateTeamInfo() OVERRIDE;
	bool GetPlayerScoreInfo( int playerIndex, KeyValues *outPlayerInfo ) OVERRIDE;
	void UpdatePlayerInfo() OVERRIDE;

	// vgui overrides for rounded corner background
	void PaintBackground() OVERRIDE;
	void PaintBorder() OVERRIDE;
	void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	void PerformLayout() OVERRIDE;

private:
	void AddHeader() OVERRIDE; // add the start header of the scoreboard
	void AddSection( int teamType, int teamNumber ) OVERRIDE; // add a new section header for a team
	int GetSectionFromTeamNumber( int teamNumber );
	void SetSectionHeader( int teamNumber, const wchar_t *teamName, int playerCount, int score, bool showScore );
	void UpdateMatchInfo();
	void UpdateColumnWidths( int listWide );
	int GetConnectedPlayerCount() const;

	enum 
	{ 
		SCOREBOARD_NAME_WIDTH = 320,
		SCOREBOARD_SCORE_WIDTH = 52,
		SCOREBOARD_DEATH_WIDTH = 58,
		SCOREBOARD_PING_WIDTH = 54,
	};

	vgui::Label *m_pServerName;
	// rounded corners
	Color m_bgColor;
	Color					 m_borderColor;
	Color m_spectatorTextColor;
	Color m_dividerColor;
};

#endif // CHL2MPCLIENTSCOREBOARDDIALOG_H
