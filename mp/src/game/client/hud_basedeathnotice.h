//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_BASEDEATHNOTICE_H
#define HUD_BASEDEATHNOTICE_H
#ifdef _WIN32
#pragma once
#endif

// Player entries in a death notice
struct DeathNoticePlayer
{
	DeathNoticePlayer()
	{
		szName[0] = 0;
		iTeam = TEAM_UNASSIGNED;
	}
	char		szName[MAX_PLAYER_NAME_LENGTH*2];	// big enough for player name and additional information
	int			iTeam;								// team #	
};

// Contents of each entry in our list of death notices
struct DeathNoticeItem 
{
	DeathNoticeItem() 
	{
		szIcon[0]=0;
		wzInfoText[0]=0;
		wzInfoTextEnd[0]=0;
		iconDeath = NULL;
		iconCritDeath = NULL;
		bSelfInflicted = false;
		bLocalPlayerInvolved = false;
		bCrit = false;
		flCreationTime = 0;
		iCount = 0;
		iWeaponID = -1;
		iKillerID = -1;
		iVictimID = -1;

		iconPreKillerName = NULL;
		iconPostKillerName = NULL;
		wzPreKillerText[0] = 0;
		iconPostVictimName = NULL;
	}

	float GetExpiryTime();

	DeathNoticePlayer	Killer;
	DeathNoticePlayer   Victim;
	char		szIcon[32];		// name of icon to display
	wchar_t		wzInfoText[32];	// any additional text to display next to icon
	wchar_t		wzInfoTextEnd[32];	// any additional text to display next to victim name
	CHudTexture *iconDeath;
	CHudTexture *iconCritDeath;	// crit background icon

	CHudTexture *iconPreKillerName;

	CHudTexture *iconPostKillerName;
	wchar_t		wzPreKillerText[32];

	CHudTexture *iconPostVictimName;

	bool		bSelfInflicted;
	bool		bLocalPlayerInvolved;
	bool		bCrit;
	float		flCreationTime;
	int			iWeaponID;
	int			iKillerID;
	int			iVictimID;
	int			iCount;
};

#define NUM_CORNER_COORD 10
#define NUM_BACKGROUND_COORD NUM_CORNER_COORD*4

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudBaseDeathNotice : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudBaseDeathNotice, vgui::Panel );
public:
	CHudBaseDeathNotice( const char *pElementName );

	void VidInit( void );
	virtual void Init( void );
	virtual bool ShouldDraw( void );
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

	void RetireExpiredDeathNotices( void );

	virtual void FireGameEvent( IGameEvent *event );
	virtual bool ShouldShowDeathNotice( IGameEvent *event ){ return true; }

protected:
	virtual Color GetTeamColor( int iTeamNumber, bool bLocalPlayerInvolved = false );
	virtual void OnGameEvent( IGameEvent *event, int iDeathNoticeMsg ) {};
	void DrawText( int x, int y, vgui::HFont hFont, Color clr, const wchar_t *szText );
	int AddDeathNoticeItem();
	void GetBackgroundPolygonVerts( int x0, int y0, int x1, int y1, int iVerts, vgui::Vertex_t vert[] );
	void CalcRoundedCorners();

	enum EDeathNoticeIconFormat
	{
		kDeathNoticeIcon_Standard,
		kDeathNoticeIcon_Inverted,			// used for display on lighter background when kill involved the local player
	};

	CHudTexture *GetIcon( const char *szIcon, EDeathNoticeIconFormat eIconFormat );

	virtual bool EventIsPlayerDeath( const char *eventName );

	virtual int UseExistingNotice( IGameEvent *event ) { return -1; }

	void GetLocalizedControlPointName( IGameEvent *event, char *namebuf, int namelen );
	virtual Color GetInfoTextColor( int iDeathNoticeMsg ){ return Color( 255, 255, 255, 255 ); }
	virtual Color GetBackgroundColor ( int iDeathNoticeMsg ) { return m_DeathNotices[iDeathNoticeMsg].bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor; }

	CPanelAnimationVarAliasType( float, m_flLineHeight, "LineHeight", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flLineSpacing, "LineSpacing", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flCornerRadius, "CornerRadius", "3", "proportional_float" );
	CPanelAnimationVar( float, m_flMaxDeathNotices, "MaxDeathNotices", "4" );
	CPanelAnimationVar( bool, m_bRightJustify, "RightJustify", "1" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVar( Color, m_clrIcon, "IconColor", "255 80 0 255" );
	CPanelAnimationVar( Color, m_clrBaseBGColor, "BaseBackgroundColor", "46 43 42 220" );
	CPanelAnimationVar( Color, m_clrLocalBGColor, "LocalBackgroundColor", "245 229 196 200" );
	CPanelAnimationVar( Color, m_clrKillStreakBg, "KillStreakBackgroundColor", "224 223 219 200" );

	CUtlVector<DeathNoticeItem> m_DeathNotices;

	Vector2D	m_CornerCoord[NUM_CORNER_COORD];
};

#endif	// HUD_BASEDEATHNOTICE_H
