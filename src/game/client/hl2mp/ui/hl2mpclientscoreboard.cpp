//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2mpclientscoreboard.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "hl2mp_gamerules.h"

#include <KeyValues.h>
#include <tier1/strtools.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/SectionedListPanel.h>

using namespace vgui;

// id's of sections used in the scoreboard
enum EScoreboardSections
{
	SCORESECTION_HEADER = 0,
	SCORESECTION_COMBINE,
	SCORESECTION_REBELS,
	SCORESECTION_FREEFORALL,
	SCORESECTION_SPECTATOR
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

class CHL2MPScoreboardHeader : public SectionedListPanelHeader
{
public:
	CHL2MPScoreboardHeader( SectionedListPanel *parent, const char *name, int sectionID ) :
		SectionedListPanelHeader( parent, name, sectionID )
	{
	}

	void PerformLayout() OVERRIDE
	{
		SectionedListPanelHeader::PerformLayout();

		int x = 0;
		const int columnCount = m_pListPanel->GetColumnCountBySection( m_iSectionID );
		for ( int i = 0; i < columnCount; ++i )
		{
			const int columnWidth = m_pListPanel->GetColumnWidthBySection( m_iSectionID, i );
			const int columnFlags = m_pListPanel->GetColumnFlagsBySection( m_iSectionID, i );
			IImage *image = GetImageAtIndex( i );

			if ( image && ( columnFlags & SectionedListPanel::COLUMN_CENTER ) )
			{
				int contentWide, contentTall;
				image->GetContentSize( contentWide, contentTall );
				const int offset = ( columnWidth / 2 ) - ( contentWide / 2 );
				SetImageBounds( i, x + offset, columnWidth - offset - SectionedListPanel::COLUMN_DATA_GAP );
			}

			x += columnWidth;
		}
	}
};

//-----------------------------------------------------------------------------
// Purpose: Konstructor
//-----------------------------------------------------------------------------
CHL2MPClientScoreBoardDialog::CHL2MPClientScoreBoardDialog( IViewPort *pViewPort ) :
	CClientScoreBoardDialog( pViewPort ),
	m_pServerName( NULL ),
	m_bgColor( 18, 19, 18, 238 ),
	m_borderColor( 92, 88, 78, 190 ),
	m_spectatorTextColor( 148, 151, 146, 255 ),
	m_dividerColor( 128, 128, 128, 96 )
{
	SetProportional( true );
	m_bAllowGrowth = false;

	m_pServerName = dynamic_cast<Label *>( FindChildByName( "ServerName" ) );

	ListenForGameEvent( "server_cvar" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CHL2MPClientScoreBoardDialog::~CHL2MPClientScoreBoardDialog()
{
}

void CHL2MPClientScoreBoardDialog::Update()
{
	BaseClass::Update();
	UpdateMatchInfo();
	InvalidateLayout();
}

void CHL2MPClientScoreBoardDialog::FireGameEvent( IGameEvent *event )
{
	if ( event && !Q_strcmp( event->GetName(), "server_cvar" ) && !Q_strcmp( event->GetString( "cvarname" ), "hostname" ) )
	{
		if ( !m_pServerName )
		{
			m_pServerName = dynamic_cast<Label *>( FindChildByName( "ServerName" ) );
		}

		if ( m_pServerName )
		{
			m_pServerName->SetText( event->GetString( "cvarvalue" ) );
			m_pServerName->MoveToFront();
		}
	}

	BaseClass::FireGameEvent( event );
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
void CHL2MPClientScoreBoardDialog::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	const Color classicBackground = GetSchemeColor( "SectionedListPanel.BgColor", GetBgColor(), pScheme );

	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetBorder( pScheme->GetBorder( "BaseBorder" ) );
	m_bgColor = classicBackground;
	m_borderColor = pScheme->GetColor( "FgColor", Color( 0, 0, 0, 0 ) );

	m_pPlayerList->SetProportional( true );
	m_pPlayerList->SetBgColor( Color(0, 0, 0, 0) );
	m_pPlayerList->SetBorder(NULL);

	HFont headerFont = pScheme->GetFont( "DefaultSmall", true );
	if ( headerFont == INVALID_FONT )
	{
		headerFont = pScheme->GetFont( "Default", true );
	}

	HFont rowFont = pScheme->GetFont( "Default", true );
	HFont titleFont = pScheme->GetFont( "DefaultLarge", true );
	if ( titleFont == INVALID_FONT )
	{
		titleFont = rowFont;
	}

	m_pPlayerList->SetHeaderFont( headerFont );
	m_pPlayerList->SetRowFont( rowFont );

	if ( !m_pServerName )
	{
		m_pServerName = dynamic_cast<Label *>( FindChildByName( "ServerName" ) );
	}

	if ( m_pServerName )
	{
		m_pServerName->SetFont( titleFont );
		m_pServerName->SetFgColor( pScheme->GetColor( "FgColor", Color( 255, 255, 255, 255 ) ) );
		m_pServerName->SetPaintBackgroundEnabled( false );
		m_pServerName->SetContentAlignment( Label::a_west );
	}

}

void CHL2MPClientScoreBoardDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	int workspaceX, workspaceY, workspaceWide, workspaceTall;
	surface()->GetWorkspaceBounds( workspaceX, workspaceY, workspaceWide, workspaceTall );

	const int targetWide = (int)( workspaceWide * 0.66f );
	const int minimumWide = MIN( (int)( workspaceWide * 0.90f ), scheme()->GetProportionalScaledValueEx( GetScheme(), 460 ) );
	const int maximumWide = MIN( (int)( workspaceWide * 0.70f ), (int)( workspaceTall * 1.50f ) );
	const int panelWide = MIN( MAX( targetWide, minimumWide ), maximumWide );

	const int horizontalPadding = scheme()->GetProportionalScaledValueEx( GetScheme(), 12 );
	const int topPadding = scheme()->GetProportionalScaledValueEx( GetScheme(), 12 );
	const int titleTall = scheme()->GetProportionalScaledValueEx( GetScheme(), 20 );
	const int titleGap = scheme()->GetProportionalScaledValueEx( GetScheme(), 2 );
	const int bottomPadding = scheme()->GetProportionalScaledValueEx( GetScheme(), 10 );
	const int listTop = topPadding + titleTall + titleGap;
	const int listWide = panelWide - horizontalPadding * 2;
	const int maximumPanelTall = (int)( workspaceTall * 0.88f );
	const int maximumListTall = MAX( scheme()->GetProportionalScaledValueEx( GetScheme(), 80 ), maximumPanelTall - listTop - bottomPadding );

	if ( m_pServerName )
	{
		m_pServerName->SetBounds( horizontalPadding, topPadding, listWide, titleTall );
		m_pServerName->MoveToFront();
	}

	m_pPlayerList->SetBounds( horizontalPadding, listTop, listWide, maximumListTall );
	UpdateColumnWidths( listWide );

	int contentWide, contentTall;
	m_pPlayerList->GetContentSize( contentWide, contentTall );

	const int minimumListTall = scheme()->GetProportionalScaledValueEx( GetScheme(), 80 );
	const int listTall = MIN( MAX( contentTall, minimumListTall ), maximumListTall );
	const int panelTall = listTop + listTall + bottomPadding;

	SetSize( panelWide, panelTall );
	m_pPlayerList->SetBounds( horizontalPadding, listTop, listWide, listTall );
	SetPos( workspaceX + ( workspaceWide - panelWide ) / 2, workspaceY + ( workspaceTall - panelTall ) / 2 );
}

//-----------------------------------------------------------------------------
// Purpose: sets up base sections
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::InitScoreboardSections()
{
	m_pPlayerList->SetBgColor( Color(0, 0, 0, 0) );
	m_pPlayerList->SetBorder(NULL);
	m_pPlayerList->ClearSelection();

	// fill out the structure of the scoreboard
	AddHeader();
	AddSection( TYPE_TEAM, TEAM_COMBINE );
	AddSection( TYPE_TEAM, TEAM_REBELS );
	AddSection( TYPE_TEAM, TEAM_UNASSIGNED );
	AddSection( TYPE_SPECTATORS, TEAM_SPECTATOR );
}

void CHL2MPClientScoreBoardDialog::SetSectionHeader( int teamNumber, const wchar_t *teamName, int playerCount, int score, bool showScore )
{
	const int sectionID = GetSectionFromTeamNumber( teamNumber );
	wchar_t sectionText[256];
	V_snwprintf( sectionText, ARRAYSIZE( sectionText ), L"%ls  \x2022  %d", teamName, playerCount );

	m_pPlayerList->ModifyColumn( sectionID, "name", sectionText );
	if ( teamNumber == TEAM_SPECTATOR )
	{
		return;
	}

	m_pPlayerList->ModifyColumn( sectionID, "deaths", L"" );
	m_pPlayerList->ModifyColumn( sectionID, "ping", L"" );

	if ( showScore )
	{
		wchar_t scoreText[16];
		V_snwprintf( scoreText, ARRAYSIZE( scoreText ), L"%d", score );
		m_pPlayerList->ModifyColumn( sectionID, "frags", scoreText );
	}
	else
	{
		m_pPlayerList->ModifyColumn( sectionID, "frags", L"" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: resets the scoreboard team info
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::UpdateTeamInfo()
{
	if ( !g_PR )
	{
		return;
	}

	int activePlayers = 0;
	int spectatorPlayers = 0;

	for ( int playerIndex = 1; playerIndex <= gpGlobals->maxClients; ++playerIndex )
	{
		if ( !g_PR->IsConnected( playerIndex ) )
		{
			continue;
		}

		if ( g_PR->GetTeam( playerIndex ) == TEAM_SPECTATOR )
		{
			++spectatorPlayers;
		}
		else
		{
			++activePlayers;
		}
	}

	if ( HL2MPRules()->IsTeamplay() )
	{
		for ( int teamNumber = TEAM_COMBINE; teamNumber <= TEAM_REBELS; ++teamNumber )
		{
			C_Team *team = GetGlobalTeam( teamNumber );
			if ( !team )
			{
				continue;
			}

			wchar_t teamName[64];
			g_pVGuiLocalize->ConvertANSIToUnicode( team->Get_Name(), teamName, sizeof( teamName ) );
			SetSectionHeader( teamNumber, teamName, team->Get_Number_Players(), team->Get_Score(), true );
		}
	}
	else
	{
		const wchar_t *deathmatchName = g_pVGuiLocalize->Find( "#ScoreBoard_Deathmatch" );
		wchar_t fallbackName[64];
		if ( !deathmatchName )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( "Deathmatch", fallbackName, sizeof( fallbackName ) );
			deathmatchName = fallbackName;
		}
		SetSectionHeader( TEAM_UNASSIGNED, deathmatchName, activePlayers, 0, false );
	}

	C_Team *spectatorTeam = GetGlobalTeam( TEAM_SPECTATOR );
	wchar_t spectatorName[64];
	if ( spectatorTeam )
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( spectatorTeam->Get_Name(), spectatorName, sizeof( spectatorName ) );
	}
	else
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( "Spectators", spectatorName, sizeof( spectatorName ) );
	}
	SetSectionHeader( TEAM_SPECTATOR, spectatorName, spectatorPlayers, 0, false );
}

//-----------------------------------------------------------------------------
// Purpose: adds the top header of the scoreboars
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::AddHeader()
{
	const int timeWide = scheme()->GetProportionalScaledValueEx( GetScheme(), 80 );
	const int infoWide = MAX( 0, scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_NAME_WIDTH ) - timeWide );
	m_pPlayerList->AddSection( SCORESECTION_HEADER, new CHL2MPScoreboardHeader( m_pPlayerList, "", SCORESECTION_HEADER ) );
	m_pPlayerList->SetSectionAlwaysVisible( SCORESECTION_HEADER );
	m_pPlayerList->SetSectionFgColor( SCORESECTION_HEADER, scheme()->GetIScheme( GetScheme() )->GetColor( "FgColor", Color( 255, 255, 255, 255 ) ) );
	m_pPlayerList->SetSectionDividerColor( SCORESECTION_HEADER, m_dividerColor );
	m_pPlayerList->SetSectionDrawDividerBar( SCORESECTION_HEADER, true );

	m_pPlayerList->AddColumnToSection( SCORESECTION_HEADER, "info", "", SectionedListPanel::COLUMN_BRIGHT, infoWide );
	m_pPlayerList->AddColumnToSection( SCORESECTION_HEADER, "time", "", SectionedListPanel::COLUMN_BRIGHT | SectionedListPanel::COLUMN_CENTER, timeWide );
	m_pPlayerList->AddColumnToSection( SCORESECTION_HEADER, "timepad", L" ", 0, 0 );
	m_pPlayerList->AddColumnToSection( SCORESECTION_HEADER, "frags", "#PlayerScore", SectionedListPanel::COLUMN_BRIGHT | SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_SCORE_WIDTH ) );
	m_pPlayerList->AddColumnToSection( SCORESECTION_HEADER, "deaths", "#PlayerDeath", SectionedListPanel::COLUMN_BRIGHT | SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_DEATH_WIDTH ) );
	m_pPlayerList->AddColumnToSection( SCORESECTION_HEADER, "ping", "#PlayerPing", SectionedListPanel::COLUMN_BRIGHT | SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_PING_WIDTH ) );
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new section to the scoreboard (i.e the team header)
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::AddSection(int teamType, int teamNumber)
{
	const int sectionID = GetSectionFromTeamNumber( teamNumber );
	m_pPlayerList->AddSection( sectionID, new CHL2MPScoreboardHeader( m_pPlayerList, "", sectionID ), StaticPlayerSortFunc );
	m_pPlayerList->SetSectionDrawDividerBar( sectionID, true );

	if ( teamType == TYPE_SPECTATORS )
	{
		m_pPlayerList->AddColumnToSection( sectionID, "name", "", 0, scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_NAME_WIDTH ) );
		m_pPlayerList->SetSectionFgColor( sectionID, m_spectatorTextColor );
		m_pPlayerList->SetSectionDividerColor( sectionID, m_dividerColor );
		m_pPlayerList->SetSectionAlwaysVisible( sectionID, false );
		return;
	}

	if ( ShowAvatars() )
	{
		m_pPlayerList->AddColumnToSection( sectionID, "avatar", "", SectionedListPanel::COLUMN_IMAGE, m_iAvatarWidth * 2 );
	}

	m_pPlayerList->AddColumnToSection( sectionID, "name", "", 0, scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_NAME_WIDTH ) );
	m_pPlayerList->AddColumnToSection( sectionID, "frags", "", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_SCORE_WIDTH ) );
	m_pPlayerList->AddColumnToSection( sectionID, "deaths", "", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_DEATH_WIDTH ) );
	m_pPlayerList->AddColumnToSection( sectionID, "ping", "", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_PING_WIDTH ) );

	IGameResources *resources = GameResources();
	const Color sectionColor = resources ? resources->GetTeamColor( teamNumber ) : scheme()->GetIScheme( GetScheme() )->GetColor( "FgColor", Color( 255, 255, 255, 255 ) );
	m_pPlayerList->SetSectionFgColor( sectionID, sectionColor );
	if ( teamNumber == TEAM_COMBINE || teamNumber == TEAM_REBELS )
	{
		m_pPlayerList->SetSectionDividerColor( sectionID, Color( sectionColor.r(), sectionColor.g(), sectionColor.b(), m_dividerColor.a() ) );
	}
	else
	{
		m_pPlayerList->SetSectionDividerColor( sectionID, m_dividerColor );
	}

	m_pPlayerList->SetSectionAlwaysVisible( sectionID, false );
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
		kv->SetString( "ping", g_PR->IsFakePlayer( playerIndex ) ? "BOT" : "" );
	}
	else
	{
		kv->SetInt("ping", g_PR->GetPing( playerIndex ));
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPClientScoreBoardDialog::UpdatePlayerInfo()
{
	CBasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !localPlayer || !g_PR )
	{
		return;
	}

	m_pPlayerList->ClearSelection();
	int selectedRow = -1;

	const bool teamplay = HL2MPRules() && HL2MPRules()->IsTeamplay();
	IGameResources *resources = GameResources();

	for ( int playerIndex = 1; playerIndex <= gpGlobals->maxClients; ++playerIndex )
	{
		if ( !g_PR->IsConnected( playerIndex ) )
		{
			const int itemID = FindItemIDForPlayerIndex( playerIndex );
			if ( itemID != -1 )
			{
				m_pPlayerList->RemoveItem( itemID );
			}
			continue;
		}

		KeyValues *playerData = new KeyValues( "data" );
		GetPlayerScoreInfo( playerIndex, playerData );

		const bool spectator = g_PR->GetTeam( playerIndex ) == TEAM_SPECTATOR;
		if ( !spectator )
		{
			UpdatePlayerAvatar( playerIndex, playerData );
		}

		int itemID = FindItemIDForPlayerIndex( playerIndex );
		const int sectionID = GetSectionFromTeamNumber( g_PR->GetTeam( playerIndex ) );

		if ( itemID == -1 )
		{
			itemID = m_pPlayerList->AddItem( sectionID, playerData );
		}
		else
		{
			m_pPlayerList->ModifyItem( itemID, sectionID, playerData );
		}

		Color playerColor = spectator ? m_spectatorTextColor : scheme()->GetIScheme( GetScheme() )->GetColor( "FgColor", Color( 255, 255, 255, 255 ) );
		if ( !spectator && resources )
		{
			playerColor = resources->GetTeamColor( teamplay ? g_PR->GetTeam( playerIndex ) : TEAM_UNASSIGNED );
		}

		m_pPlayerList->SetItemFgColor( itemID, playerColor );
		m_pPlayerList->SetItemBgColor( itemID, Color( 0, 0, 0, 0 ) );
		if ( playerIndex == localPlayer->entindex() )
		{
			selectedRow = itemID;
		}
		playerData->deleteThis();
	}

	if ( selectedRow != -1 )
	{
		m_pPlayerList->SetSelectedItem( selectedRow );
	}
}

int CHL2MPClientScoreBoardDialog::GetConnectedPlayerCount() const
{
	if ( !g_PR )
	{
		return 0;
	}

	int playerCount = 0;
	for ( int playerIndex = 1; playerIndex <= gpGlobals->maxClients; ++playerIndex )
	{
		if ( g_PR->IsConnected( playerIndex ) )
		{
			++playerCount;
		}
	}
	return playerCount;
}

void CHL2MPClientScoreBoardDialog::UpdateMatchInfo()
{
	wchar_t mapName[128];
	char shortLevelName[MAX_PATH] = "-";
	const char *levelName = engine->GetLevelName();
	if ( levelName && levelName[0] )
	{
		V_FileBase( levelName, shortLevelName, sizeof( shortLevelName ) );
	}
	g_pVGuiLocalize->ConvertANSIToUnicode( shortLevelName, mapName, sizeof( mapName ) );

	const bool teamplay = HL2MPRules() && HL2MPRules()->IsTeamplay();
	const wchar_t *modeName = g_pVGuiLocalize->Find( teamplay ? "#ScoreBoard_TeamDeathmatch" : "#ScoreBoard_Deathmatch" );
	wchar_t fallbackMode[64];
	if ( !modeName )
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( teamplay ? "Team Deathmatch" : "Deathmatch", fallbackMode, sizeof( fallbackMode ) );
		modeName = fallbackMode;
	}

	wchar_t timeValue[16];
	static ConVarRef mpTimeLimit( "mp_timelimit" );
	if ( mpTimeLimit.IsValid() && mpTimeLimit.GetFloat() > 0.0f )
	{
		const float remainingTime = HL2MPRules() ? MAX( 0.0f, HL2MPRules()->GetMapRemainingTime() ) : 0.0f;
		int remainingSeconds = (int)remainingTime;
		if ( (float)remainingSeconds < remainingTime )
		{
			++remainingSeconds;
		}
		if ( remainingSeconds >= 3600 )
		{
			V_snwprintf( timeValue, ARRAYSIZE( timeValue ), L"%02d:%02d:%02d", remainingSeconds / 3600, remainingSeconds / 60 % 60, remainingSeconds % 60 );
		}
		else
		{
			V_snwprintf( timeValue, ARRAYSIZE( timeValue ), L"%02d:%02d", remainingSeconds / 60, remainingSeconds % 60 );
		}
	}
	else
	{
		V_wcsncpy( timeValue, L"--:--", sizeof( timeValue ) );
	}

	wchar_t fragLimitText[64];
	static ConVarRef fragLimit( "mp_fraglimit" );
	if ( fragLimit.IsValid() && fragLimit.GetInt() > 0 )
	{
		V_snwprintf( fragLimitText, ARRAYSIZE( fragLimitText ), L"Frag limit %d", fragLimit.GetInt() );
	}
	else
	{
		V_wcsncpy( fragLimitText, L"No frag limit", sizeof( fragLimitText ) );
	}

	wchar_t matchInfo[512];
	V_snwprintf(
		matchInfo,
		ARRAYSIZE( matchInfo ),
		L"%ls  \x2022  %ls  \x2022  %d/%d  \x2022  %ls",
		mapName,
		modeName,
		GetConnectedPlayerCount(),
		gpGlobals->maxClients,
		fragLimitText );
	m_pPlayerList->ModifyColumn( SCORESECTION_HEADER, "info", matchInfo );
	m_pPlayerList->ModifyColumn( SCORESECTION_HEADER, "time", timeValue );
}

void CHL2MPClientScoreBoardDialog::UpdateColumnWidths( int listWide )
{
	const int scoreWide = scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_SCORE_WIDTH );
	const int deathWide = scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_DEATH_WIDTH );
	const int pingWide = scheme()->GetProportionalScaledValueEx( GetScheme(), SCOREBOARD_PING_WIDTH );
	const int avatarWide = ShowAvatars() ? m_iAvatarWidth * 2 : 0;
	ScrollBar *scrollBar = m_pPlayerList->GetScrollBar();
	const int scrollWide = scrollBar && scrollBar->IsVisible() ? scrollBar->GetWide() : 0;
	const int columnAreaWide = MAX( 0, listWide - 10 - scrollWide );
	const int nameWide = MAX( scheme()->GetProportionalScaledValueEx( GetScheme(), 180 ), columnAreaWide - avatarWide - scoreWide - deathWide - pingWide );
	int timeTextWide, timeTextTall;
	surface()->GetTextSize( m_pPlayerList->GetHeaderFont(), L"00:00:00", timeTextWide, timeTextTall );
	const wchar_t *timeText = m_pPlayerList->GetColumnTextBySection( SCORESECTION_HEADER, 1 );
	if ( timeText && timeText[0] )
	{
		int currentTimeWide, currentTimeTall;
		surface()->GetTextSize( m_pPlayerList->GetHeaderFont(), timeText, currentTimeWide, currentTimeTall );
		timeTextWide = MAX( timeTextWide, currentTimeWide );
	}
	const int identityWide = avatarWide + nameWide;
	const int timeWide = MIN( identityWide, timeTextWide + scheme()->GetProportionalScaledValueEx( GetScheme(), 12 ) );
	const int infoWide = MIN( identityWide - timeWide, MAX( 0, columnAreaWide / 2 - timeWide / 2 ) );
	const int timePadWide = identityWide - infoWide - timeWide;

	m_pPlayerList->SetColumnWidthBySection( SCORESECTION_HEADER, "info", infoWide );
	m_pPlayerList->SetColumnWidthBySection( SCORESECTION_HEADER, "time", timeWide );
	m_pPlayerList->SetColumnWidthBySection( SCORESECTION_HEADER, "timepad", timePadWide );
	m_pPlayerList->SetColumnWidthBySection( SCORESECTION_HEADER, "frags", scoreWide );
	m_pPlayerList->SetColumnWidthBySection( SCORESECTION_HEADER, "deaths", deathWide );
	m_pPlayerList->SetColumnWidthBySection( SCORESECTION_HEADER, "ping", pingWide );

	const int activeSections[] =
	{
		SCORESECTION_COMBINE,
		SCORESECTION_REBELS,
		SCORESECTION_FREEFORALL
	};

	for ( int i = 0; i < ARRAYSIZE( activeSections ); ++i )
	{
		const int sectionID = activeSections[i];
		if ( ShowAvatars() )
		{
			m_pPlayerList->SetColumnWidthBySection( sectionID, "avatar", avatarWide );
		}
		m_pPlayerList->SetColumnWidthBySection( sectionID, "name", nameWide );
		m_pPlayerList->SetColumnWidthBySection( sectionID, "frags", scoreWide );
		m_pPlayerList->SetColumnWidthBySection( sectionID, "deaths", deathWide );
		m_pPlayerList->SetColumnWidthBySection( sectionID, "ping", pingWide );
	}

	m_pPlayerList->SetColumnWidthBySection( SCORESECTION_SPECTATOR, "name", columnAreaWide );
	m_pPlayerList->InvalidateLayout();
}
