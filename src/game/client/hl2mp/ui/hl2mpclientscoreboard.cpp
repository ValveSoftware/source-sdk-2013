//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hl2mpclientscoreboard.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "c_hl2mp_player.h"
#include "hl2mp_gamerules.h"

#include <KeyValues.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/SectionedListPanel.h>

#include "voice_status.h"

using namespace vgui;

#define TEAM_MAXCOUNT			5

// id's of sections used in the scoreboard
enum EScoreboardSections
{
	SCORESECTION_COMBINE = 1,
	SCORESECTION_REBELS = 2,
	SCORESECTION_FREEFORALL = 3,
	SCORESECTION_SPECTATOR = 4
};

const int NumSegments = 7;
static int coord[NumSegments+1] = {
	0,
	1,
	2,
	3,
	4,
	6,
	9,
	10
};

//-----------------------------------------------------------------------------
// Purpose: Konstructor
//-----------------------------------------------------------------------------
CHL2MPClientScoreBoardDialog::CHL2MPClientScoreBoardDialog(IViewPort *pViewPort):CClientScoreBoardDialog(pViewPort)
{
	SetProportional( true );
	m_bAllowGrowth = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CHL2MPClientScoreBoardDialog::~CHL2MPClientScoreBoardDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: Paint background for rounded corners
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::PaintBackground()
{
	m_pPlayerList->SetBgColor( Color(0, 0, 0, 0) );
	m_pPlayerList->SetBorder(NULL);

	int x1, x2, y1, y2;
	surface()->DrawSetColor(m_bgColor);
	surface()->DrawSetTextColor(m_bgColor);

	int wide, tall;
	GetSize( wide, tall );

	int i;

	// top-left corner --------------------------------------------------------
	int xDir = 1;
	int yDir = -1;
	int xIndex = 0;
	int yIndex = NumSegments - 1;
	int xMult = 1;
	int yMult = 1;
	int x = 0;
	int y = 0;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = y + coord[NumSegments];
		surface()->DrawFilledRect( x1, y1, x2, y2 );

		xIndex += xDir;
		yIndex += yDir;
	}

	// top-right corner -------------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = wide;
	y = 0;
	xMult = -1;
	yMult = 1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = y + coord[NumSegments];
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// bottom-right corner ----------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = wide;
	y = tall;
	xMult = -1;
	yMult = -1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = y - coord[NumSegments];
		y2 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// bottom-left corner -----------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = 0;
	y = tall;
	xMult = 1;
	yMult = -1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = y - coord[NumSegments];
		y2 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// paint between top left and bottom left ---------------------------------
	x1 = 0;
	x2 = coord[NumSegments];
	y1 = coord[NumSegments];
	y2 = tall - coord[NumSegments];
	surface()->DrawFilledRect( x1, y1, x2, y2 );

	// paint between left and right -------------------------------------------
	x1 = coord[NumSegments];
	x2 = wide - coord[NumSegments];
	y1 = 0;
	y2 = tall;
	surface()->DrawFilledRect( x1, y1, x2, y2 );
	
	// paint between top right and bottom right -------------------------------
	x1 = wide - coord[NumSegments];
	x2 = wide;
	y1 = coord[NumSegments];
	y2 = tall - coord[NumSegments];
	surface()->DrawFilledRect( x1, y1, x2, y2 );
}

//-----------------------------------------------------------------------------
// Purpose: Paint border for rounded corners
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::PaintBorder()
{
	int x1, x2, y1, y2;
	surface()->DrawSetColor(m_borderColor);
	surface()->DrawSetTextColor(m_borderColor);

	int wide, tall;
	GetSize( wide, tall );

	int i;

	// top-left corner --------------------------------------------------------
	int xDir = 1;
	int yDir = -1;
	int xIndex = 0;
	int yIndex = NumSegments - 1;
	int xMult = 1;
	int yMult = 1;
	int x = 0;
	int y = 0;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );

		xIndex += xDir;
		yIndex += yDir;
	}

	// top-right corner -------------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = wide;
	y = 0;
	xMult = -1;
	yMult = 1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// bottom-right corner ----------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = wide;
	y = tall;
	xMult = -1;
	yMult = -1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// bottom-left corner -----------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = 0;
	y = tall;
	xMult = 1;
	yMult = -1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// top --------------------------------------------------------------------
	x1 = coord[NumSegments];
	x2 = wide - coord[NumSegments];
	y1 = 0;
	y2 = 1;
	surface()->DrawFilledRect( x1, y1, x2, y2 );

	// bottom -----------------------------------------------------------------
	x1 = coord[NumSegments];
	x2 = wide - coord[NumSegments];
	y1 = tall - 1;
	y2 = tall;
	surface()->DrawFilledRect( x1, y1, x2, y2 );

	// left -------------------------------------------------------------------
	x1 = 0;
	x2 = 1;
	y1 = coord[NumSegments];
	y2 = tall - coord[NumSegments];
	surface()->DrawFilledRect( x1, y1, x2, y2 );

	// right ------------------------------------------------------------------
	x1 = wide - 1;
	x2 = wide;
	y1 = coord[NumSegments];
	y2 = tall - coord[NumSegments];
	surface()->DrawFilledRect( x1, y1, x2, y2 );
}

//-----------------------------------------------------------------------------
// Purpose: Apply scheme settings
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_bgColor = GetSchemeColor("SectionedListPanel.BgColor", GetBgColor(), pScheme);
	m_borderColor = pScheme->GetColor( "FgColor", Color( 0, 0, 0, 0 ) );

	SetBgColor( Color(0, 0, 0, 0) );
	SetBorder( pScheme->GetBorder( "BaseBorder" ) );

	m_pPlayerList->SetProportional( true );
}


//-----------------------------------------------------------------------------
// Purpose: sets up base sections
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::InitScoreboardSections()
{
	m_pPlayerList->SetBgColor( Color(0, 0, 0, 0) );
	m_pPlayerList->SetBorder(NULL);

	// fill out the structure of the scoreboard
	AddHeader();

	if ( HL2MPRules()->IsTeamplay() )
	{
		// add the team sections
		AddSection( TYPE_TEAM, TEAM_COMBINE );
		AddSection( TYPE_TEAM, TEAM_REBELS );
	}
	else
	{
		AddSection( TYPE_TEAM, TEAM_UNASSIGNED );
	}
	AddSection( TYPE_TEAM, TEAM_SPECTATOR );
}

//-----------------------------------------------------------------------------
// Purpose: resets the scoreboard team info
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::UpdateTeamInfo()
{
	if ( g_PR == NULL )
		return;

	int iNumPlayersInGame = 0;

	for ( int j = 1; j <= gpGlobals->maxClients; j++ )
	{	
		if ( g_PR->IsConnected( j ) )
		{
			iNumPlayersInGame++;
		}
	}

	// update the team sections in the scoreboard
	for ( int i = TEAM_SPECTATOR; i < TEAM_MAXCOUNT; i++ )
	{
		wchar_t *teamName = NULL;
		int sectionID = 0;
		C_Team *team = GetGlobalTeam(i);

		if ( team )
		{
			sectionID = GetSectionFromTeamNumber( i );
	
			// update team name
			wchar_t name[64];
			wchar_t string1[1024];
			wchar_t wNumPlayers[6];

			if ( HL2MPRules()->IsTeamplay() == false )
			{
				_snwprintf( wNumPlayers, ARRAYSIZE(wNumPlayers), L"%i", iNumPlayersInGame );
#ifdef WIN32
				_snwprintf( name, ARRAYSIZE(name), L"%s", g_pVGuiLocalize->Find("#ScoreBoard_Deathmatch") );
#else
				_snwprintf( name, ARRAYSIZE(name), L"%S", g_pVGuiLocalize->Find("#ScoreBoard_Deathmatch") );
#endif
				
				teamName = name;

				if ( iNumPlayersInGame == 1)
				{
					g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#ScoreBoard_Player"), 2, teamName, wNumPlayers );
				}
				else
				{
					g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#ScoreBoard_Players"), 2, teamName, wNumPlayers );
				}
			}
			else
			{
				_snwprintf(wNumPlayers, ARRAYSIZE(wNumPlayers), L"%i", team->Get_Number_Players());

				if (!teamName && team)
				{
					g_pVGuiLocalize->ConvertANSIToUnicode(team->Get_Name(), name, sizeof(name));
					teamName = name;
				}

				if (team->Get_Number_Players() == 1)
				{
					g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#ScoreBoard_Player"), 2, teamName, wNumPlayers );
				}
				else
				{
					g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#ScoreBoard_Players"), 2, teamName, wNumPlayers );
				}

				// update stats
				wchar_t val[6];
				V_snwprintf(val, ARRAYSIZE(val), L"%d", team->Get_Score());
				m_pPlayerList->ModifyColumn(sectionID, "frags", val);
				if (team->Get_Ping() < 1)
				{
					m_pPlayerList->ModifyColumn(sectionID, "ping", L"");
				}
				else
				{
					V_snwprintf(val, ARRAYSIZE(val), L"%d", team->Get_Ping());
					m_pPlayerList->ModifyColumn(sectionID, "ping", val);
				}

			}
		
			m_pPlayerList->ModifyColumn(sectionID, "name", string1);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds the top header of the scoreboars
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::AddHeader()
{
	// add the top header
	m_pPlayerList->AddSection(0, "");
	m_pPlayerList->SetSectionAlwaysVisible(0);
	m_pPlayerList->AddColumnToSection(0, "avatar", "", 0, m_iAvatarWidth * 2 );
	m_pPlayerList->AddColumnToSection(0, "name", "", 0, scheme()->GetProportionalScaledValueEx( GetScheme(), CSTRIKE_NAME_WIDTH ) - ( m_iAvatarWidth * 2 ) );
	m_pPlayerList->AddColumnToSection(0, "frags", "#PlayerScore", 0 | SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValueEx( GetScheme(), CSTRIKE_SCORE_WIDTH ) );
	m_pPlayerList->AddColumnToSection(0, "deaths", "#PlayerDeath", 0 | SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValueEx( GetScheme(), CSTRIKE_DEATH_WIDTH ) );
	m_pPlayerList->AddColumnToSection(0, "ping", "#PlayerPing", 0 | SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValueEx( GetScheme(), CSTRIKE_PING_WIDTH ) );
//	m_pPlayerList->AddColumnToSection(0, "voice", "#PlayerVoice", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::HEADER_TEXT| SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), CSTRIKE_VOICE_WIDTH ) );
//	m_pPlayerList->AddColumnToSection(0, "tracker", "#PlayerTracker", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::HEADER_TEXT, scheme()->GetProportionalScaledValueEx( GetScheme(), CSTRIKE_FRIENDS_WIDTH ) );
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new section to the scoreboard (i.e the team header)
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::AddSection(int teamType, int teamNumber)
{
	int sectionID = GetSectionFromTeamNumber( teamNumber );
	if ( teamType == TYPE_TEAM )
	{
 		m_pPlayerList->AddSection(sectionID, "", StaticPlayerSortFunc);

		// setup the columns
		if ( ShowAvatars() )
		{
			m_pPlayerList->AddColumnToSection( sectionID, "avatar", "", SectionedListPanel::COLUMN_IMAGE, ( m_iAvatarWidth * 2 ) );
		}

		m_pPlayerList->AddColumnToSection(sectionID, "name", "", 0, scheme()->GetProportionalScaledValueEx( GetScheme(), CSTRIKE_NAME_WIDTH ) - ( m_iAvatarWidth * 2 ) );
		m_pPlayerList->AddColumnToSection(sectionID, "frags", "", SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValueEx( GetScheme(), CSTRIKE_SCORE_WIDTH ) );
		m_pPlayerList->AddColumnToSection(sectionID, "deaths", "", SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValueEx( GetScheme(), CSTRIKE_DEATH_WIDTH ) );
		m_pPlayerList->AddColumnToSection(sectionID, "ping", "", SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValueEx( GetScheme(), CSTRIKE_PING_WIDTH ) );

		// set the section to have the team color
		if ( teamNumber )
		{
			if ( GameResources() )
				m_pPlayerList->SetSectionFgColor(sectionID,  GameResources()->GetTeamColor(teamNumber));
		}

		m_pPlayerList->SetSectionAlwaysVisible(sectionID);
	}
	else if ( teamType == TYPE_SPECTATORS )
	{
		m_pPlayerList->AddSection(sectionID, "");
		if ( ShowAvatars() )
		{
			m_pPlayerList->AddColumnToSection( sectionID, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, ( m_iAvatarWidth * 2 ) );
		}
		m_pPlayerList->AddColumnToSection(sectionID, "name", "#Spectators", 0, scheme()->GetProportionalScaledValueEx( GetScheme(), CSTRIKE_NAME_WIDTH ) - ( m_iAvatarWidth * 2 ) );
	}
}

int CHL2MPClientScoreBoardDialog::GetSectionFromTeamNumber( int teamNumber )
{
	switch ( teamNumber )
	{
	case TEAM_COMBINE:
		return SCORESECTION_COMBINE;
	case TEAM_REBELS:
		return SCORESECTION_REBELS;
	case TEAM_SPECTATOR:
		return SCORESECTION_SPECTATOR;
	default:
		return SCORESECTION_FREEFORALL;
	}
	return SCORESECTION_FREEFORALL;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
bool CHL2MPClientScoreBoardDialog::GetPlayerScoreInfo(int playerIndex, KeyValues *kv)
{
	kv->SetInt("playerIndex", playerIndex);
	kv->SetInt("team", g_PR->GetTeam( playerIndex ) );
	kv->SetString("name", g_PR->GetPlayerName(playerIndex) );
	kv->SetInt("deaths", g_PR->GetDeaths( playerIndex ));
	kv->SetInt("frags", g_PR->GetPlayerScore( playerIndex ));
	
	if (g_PR->GetPing( playerIndex ) < 1)
	{
		if ( g_PR->IsFakePlayer( playerIndex ) )
		{
			kv->SetString("ping", "BOT");
		}
		else
		{
			kv->SetString("ping", "");
		}
	}
	else
	{
		kv->SetInt("ping", g_PR->GetPing( playerIndex ));
	}
	
	return true;
}

enum {
	MAX_PLAYERS_PER_TEAM = 16,
	MAX_SCOREBOARD_PLAYERS = 32
};
struct PlayerScoreInfo
{
	int index;
	int frags;
	int deaths;
	bool important;
	bool alive;
};

int PlayerScoreInfoSort( const PlayerScoreInfo *p1, const PlayerScoreInfo *p2 )
{
	// check local
	if ( p1->important )
		return -1;
	if ( p2->important )
		return 1;

	// check alive
	if ( p1->alive && !p2->alive )
		return -1;
	if ( p2->alive && !p1->alive )
		return 1;

	// check frags
	if ( p1->frags > p2->frags )
		return -1;
	if ( p2->frags > p1->frags )
		return 1;

	// check deaths
	if ( p1->deaths < p2->deaths )
		return -1;
	if ( p2->deaths < p1->deaths )
		return 1;

	// check index
	if ( p1->index < p2->index )
		return -1;

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::UpdatePlayerInfo()
{
	m_iSectionId = 0; // 0'th row is a header
	int selectedRow = -1;
	int i;

	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !g_PR )
		return;

	// walk all the players and make sure they're in the scoreboard
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		bool shouldShow = g_PR->IsConnected( i );
		if ( shouldShow )
		{
			// add the player to the list
			KeyValues *playerData = new KeyValues("data");
			GetPlayerScoreInfo( i, playerData );
			UpdatePlayerAvatar( i, playerData );
			int itemID = FindItemIDForPlayerIndex( i );
  			int sectionID = GetSectionFromTeamNumber( g_PR->GetTeam( i ) );
						
			if (itemID == -1)
			{
				// add a new row
				itemID = m_pPlayerList->AddItem( sectionID, playerData );
			}
			else
			{
				// modify the current row
				m_pPlayerList->ModifyItem( itemID, sectionID, playerData );
			}

			if ( i == pPlayer->entindex() )
			{
				selectedRow = itemID;	// this is the local player, hilight this row
			}

			// set the row color based on the players team
			m_pPlayerList->SetItemFgColor( itemID, g_PR->GetTeamColor( g_PR->GetTeam( i ) ) );

			playerData->deleteThis();
		}
		else
		{
			// remove the player
			int itemID = FindItemIDForPlayerIndex( i );
			if (itemID != -1)
			{
				m_pPlayerList->RemoveItem(itemID);
			}
		}
	}

	if ( selectedRow != -1 )
	{
		m_pPlayerList->SetSelectedItem(selectedRow);
	}

	
}
