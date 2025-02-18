//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws CSPort's death notices
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "iclientmode.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "vgui_controls/TextImage.h"
#include <KeyValues.h>
#include "c_baseplayer.h"
#include "c_team.h"
#include "gcsdk/gcclientsdk.h"
#include "tf_gcmessages.h"
#include "tf_item_inventory.h"

#include "hud_basedeathnotice.h"

#include "tf_shareddefs.h"
#include "clientmode_tf.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"
#include "tf_hud_freezepanel.h"
#include "engine/IEngineSound.h"
#include "tf_controls.h"
#include "tf_gamerules.h"
#include "econ_notifications.h"
//#include "econ/econ_controls.h"
#include "passtime_game_events.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;

// Must match resource/tf_objects.txt!!!
const char *szLocalizedObjectNames[OBJ_LAST] =
{
	"#TF_Object_Dispenser",
	"#TF_Object_Tele",
	"#TF_Object_Sentry",
	"#TF_object_Sapper"
};

ConVar cl_hud_killstreak_display_time( "cl_hud_killstreak_display_time", "3", FCVAR_ARCHIVE, "How long a killstreak notice stays on the screen (in seconds).  Range is from 0 to 100."  );
ConVar cl_hud_killstreak_display_fontsize( "cl_hud_killstreak_display_fontsize", "0", FCVAR_ARCHIVE, "Adjusts font size of killstreak notices.  Range is from 0 to 2 (default is 1)." );
ConVar cl_hud_killstreak_display_alpha( "cl_hud_killstreak_display_alpha", "120", FCVAR_ARCHIVE, "Adjusts font alpha value of killstreak notices.  Range is from 0 to 255 (default is 200)." );

const int STREAK_MIN = 5;
const int STREAK_MIN_MVM = 20;
const int STREAK_MIN_DUCKS = 10;

static int MinStreakForType( CTFPlayerShared::ETFStreak eStreakType )
{
	bool bIsMvM = TFGameRules() && TFGameRules()->IsMannVsMachineMode();
	if ( eStreakType == CTFPlayerShared::kTFStreak_Ducks )
	{
		return STREAK_MIN_DUCKS;
	}
	if ( eStreakType == CTFPlayerShared::kTFStreak_Duck_levelup )
	{
		return 1;
	}
	if ( bIsMvM )
	{
		return STREAK_MIN_MVM;
	}
	return STREAK_MIN;
}

//=========================================================
// CTFStreakNotice
//=========================================================
class CTFStreakNotice : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFStreakNotice, vgui::EditablePanel );
public:
	CTFStreakNotice( const char *pName );

	virtual bool ShouldDraw( void ) OVERRIDE;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void Paint( void ) OVERRIDE;

	void StreakEnded( CTFPlayerShared::ETFStreak eStreakType, int iKillerID, int iVictimID, int iStreak );
	void StreakUpdated( CTFPlayerShared::ETFStreak eStreakType, int iPlayerID, int iStreak, int iStreakIncrement );

	bool IsCurrentStreakHigherPriority( CTFPlayerShared::ETFStreak eStreakType, int iStreak );
	HFont GetStreakFont( void );

private:
	CExLabel *m_pLabel;
	EditablePanel *m_pBackground;

	float                      m_flLastMessageTime;
	int                        m_nCurrStreakCount;
	CTFPlayerShared::ETFStreak m_nCurrStreakType;

	int m_nLabelXPos;
	int m_nLabelYPos;

	CHudTexture *m_iconKillStreak;
	CHudTexture *m_iconDuckStreak;
};

//-----------------------------------------------------------------------------
CTFStreakNotice::CTFStreakNotice( const char *pName ) : CHudElement( pName ), vgui::EditablePanel( NULL, pName )
{
	SetParent( g_pClientMode->GetViewport() );

	m_pBackground = new EditablePanel( this, "Background" );
	m_pLabel = new CExLabel( this, "SplashLabel", "" );

	m_flLastMessageTime = -10.0f;
	m_nCurrStreakCount = 0;
	m_nCurrStreakType = (CTFPlayerShared::ETFStreak)0;

	m_iconKillStreak = gHUD.GetIcon( "leaderboard_streak" );
	m_iconDuckStreak = gHUD.GetIcon( "eotl_duck" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFStreakNotice::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return m_nCurrStreakCount > 0;
}

//-----------------------------------------------------------------------------
void CTFStreakNotice::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/HudKillStreakNotice.res" );

	m_pLabel->GetPos( m_nLabelXPos, m_nLabelYPos );

	SetSize( XRES(640), YRES(480) );
}

//-----------------------------------------------------------------------------
void CTFStreakNotice::Paint( void )
{
	int nDisplayTime = clamp( cl_hud_killstreak_display_time.GetInt(), 1, 100 );
	if ( m_flLastMessageTime + nDisplayTime < gpGlobals->realtime )
	{
		SetVisible( false );
		m_nCurrStreakCount = 0;
		return;
	}

	float flFadeTime = 1.5f;
	float flTimeRemaining = (float)nDisplayTime - ( gpGlobals->realtime - m_flLastMessageTime );

	SetVisible( true );
	if ( flTimeRemaining > flFadeTime )
	{
		SetAlpha( 255 );
	}
	else
	{
		float flAlpha = RemapValClamped( flTimeRemaining, flFadeTime, 0.f, 255.f, 0.f );
		SetAlpha( flAlpha );
	}

	// Move labels down when in spectator
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	CHudTexture *pIcon = ( m_nCurrStreakType == CTFPlayerShared::kTFStreak_Ducks || m_nCurrStreakType == CTFPlayerShared::kTFStreak_Duck_levelup ) ? m_iconDuckStreak : m_iconKillStreak;
	if ( pPlayer && pIcon )
	{
		int nYOffset = ( pPlayer->GetObserverMode() > OBS_MODE_FREEZECAM ? YRES(40) : 0 );

		int iWide, iTall;
		m_pLabel->GetContentSize( iWide, iTall );
		m_pLabel->SizeToContents();
		m_pLabel->SetPos( XRES(320) - iWide / 2, m_nLabelYPos + nYOffset );

		m_pBackground->SetSize( iWide + iTall / 2, iTall ); // add in icon width
		m_pBackground->SetPos( XRES(315) - iWide / 2, m_nLabelYPos + nYOffset);

		wchar_t szTitle[256];
		m_pLabel->GetText( szTitle, 256 );
		HFont hFont = GetStreakFont();
		int iTextWide= UTIL_ComputeStringWidth( hFont, szTitle );
		pIcon->DrawSelf( XRES(320) - (iWide / 2) + iTextWide, m_nLabelYPos + nYOffset, iTall, iTall, Color(235, 226, 202, GetAlpha() ) );
	}

	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
void CTFStreakNotice::StreakEnded( CTFPlayerShared::ETFStreak eStreakType, int iKillerID, int iVictimID, int iStreak )
{
	if ( iStreak < 10 )
		return;

	if ( IsCurrentStreakHigherPriority( eStreakType, iStreak ) )
		return;

	// Temp override all messages
	// Add New message
	m_flLastMessageTime = gpGlobals->realtime;
	
	// Generate the String
	const wchar_t *wzMsg = NULL;
	bool bSelfKill = false;
	if ( iKillerID == iVictimID )
	{
		wzMsg = g_pVGuiLocalize->Find( ( eStreakType == CTFPlayerShared::kTFStreak_Ducks ) ? "#Msg_DuckStreakEndSelf" : "#Msg_KillStreakEndSelf" );
		bSelfKill = true;
	}
	else
	{
		wzMsg = g_pVGuiLocalize->Find( ( eStreakType == CTFPlayerShared::kTFStreak_Ducks ) ? "#Msg_DuckStreakEnd" : "#Msg_KillStreakEnd" );
	}
	
	if ( !wzMsg )
		return;

	// m_nCurrStreakCount = iStreak;

	// Killer Name
	wchar_t wszKillerName[MAX_PLAYER_NAME_LENGTH / 2];
	g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iKillerID ), wszKillerName, sizeof(wszKillerName) );

	// Victim Name
	wchar_t wszVictimName[MAX_PLAYER_NAME_LENGTH / 2];
	g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iVictimID ), wszVictimName, sizeof(wszVictimName) );

	// Count
	wchar_t wzCount[10];
	_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", iStreak );

	wchar_t	wTemp[256];
	if ( bSelfKill )
	{
		g_pVGuiLocalize->ConstructString_safe( wTemp, wzMsg, 2, wszKillerName, wzCount );
	}
	else
	{
		g_pVGuiLocalize->ConstructString_safe( wTemp, wzMsg, 3, wszKillerName, wszVictimName, wzCount );
	}
	
	HFont hFont = GetStreakFont();
	if ( m_pLabel->GetFont() != hFont )
	{
		m_pLabel->SetFont( hFont );
	}
	m_pLabel->SetText( wTemp );

	// Get player Team for color
	Color cKillerColor(235, 226, 202, 255);
	if ( g_PR->GetTeam( iKillerID ) == TF_TEAM_RED )
	{
		cKillerColor = COLOR_RED;
	}
	else if ( g_PR->GetTeam( iKillerID ) == TF_TEAM_BLUE )
	{
		cKillerColor = COLOR_BLUE;
	}

	Color cVictimColor(235, 226, 202, 255);
	if ( g_PR->GetTeam( iVictimID ) == TF_TEAM_RED )
	{
		cVictimColor = COLOR_RED;
	}
	else if ( g_PR->GetTeam( iVictimID ) == TF_TEAM_BLUE )
	{
		cVictimColor = COLOR_BLUE;
	}

	m_pLabel->GetTextImage()->ClearColorChangeStream();

	// We change the title's text color to match the colors of the matching model panel backgrounds
	wchar_t *txt = wTemp;
	int iWChars = 0;
	while ( txt && *txt )
	{
		switch ( *txt )
		{
		case 0x01:	// Normal color
			m_pLabel->GetTextImage()->AddColorChange( Color(235, 226, 202, cl_hud_killstreak_display_alpha.GetInt() ), iWChars );
			break;
		case 0x02:	// Team color
			m_pLabel->GetTextImage()->AddColorChange( Color( cKillerColor.r(), cKillerColor.g(), cKillerColor.b(), cl_hud_killstreak_display_alpha.GetInt() ), iWChars );
			break;
		case 0x03:	// Item 2 color
			m_pLabel->GetTextImage()->AddColorChange( Color( cVictimColor.r(), cVictimColor.g(), cVictimColor.b(), cl_hud_killstreak_display_alpha.GetInt() ), iWChars );
			break;
		default:
			break;
		}

		txt++;
		iWChars++;
	}

	m_flLastMessageTime = gpGlobals->realtime;
	SetVisible( true );
}

//-----------------------------------------------------------------------------
void CTFStreakNotice::StreakUpdated( CTFPlayerShared::ETFStreak eStreakType, int iKillerID, int iStreak, int iStreakIncrement )
{
	// Temp override all messages
	// Add New message

	bool bIsMvM = TFGameRules() && TFGameRules()->IsMannVsMachineMode();
	int iStreakMin = MinStreakForType( eStreakType );

	if ( IsCurrentStreakHigherPriority( eStreakType, iStreak ) )
		return;

	// Is this message worth responding to
	int iStreakTier = 0;
	if ( eStreakType == CTFPlayerShared::kTFStreak_Ducks)
	{
		// Notices at 15, 30, then increments of 50. We may increment by multiple ducks per kill, so check if we passed over a milestone.
		if ( iStreak >= 15 && ( iStreak - iStreakIncrement < 15 ) )
		{
			iStreakTier = 1;
			iStreak = 15;
		}
		else if ( iStreak >= 30 && ( iStreak - iStreakIncrement <30 ) )
		{
			iStreakTier = 2;
			iStreak = 30;
		}
		else if ( iStreak > 50 && iStreak % 50 < iStreakIncrement )
		{
			iStreakTier = Min( 2 + ( iStreak / 50 ), 5 );
			iStreak -= iStreak % 50;
		}
		else
		{
			return;
		}
	}
	else if ( eStreakType == CTFPlayerShared::kTFStreak_Duck_levelup )
	{
		iStreakTier = 5;
	}
	else if ( bIsMvM )
	{
		if ( iStreak % iStreakMin != 0 )
			return;

		iStreakTier = iStreak / iStreakMin;
	}
	else
	{
		if ( iStreak == 5 )
		{
			iStreakTier = 1;
		}
		else if ( iStreak == 10 )
		{
			iStreakTier = 2;
		}
		else if ( iStreak == 15 )
		{
			iStreakTier = 3;
		}
		else if ( iStreak == 20 )
		{
			iStreakTier = 4;
		}
		else if ( iStreak % 10 == 0 || iStreak % 10 == 5 )
		{
			iStreakTier = 5;
		}
		else
		{
			return;
		}
	}

	m_nCurrStreakCount = iStreak;
	m_nCurrStreakType = eStreakType;

	const wchar_t *wzMsg = NULL;
	const char *pszSoundName = "Game.KillStreak";
	Color cCustomColor(235, 226, 202, 255);
	if ( eStreakType == CTFPlayerShared::kTFStreak_Ducks )
	{
		// Duckstreak tiers
		switch ( iStreakTier )
		{
		case 1:
			// TODO duckier colors?
			cCustomColor = Color( 112, 176, 74, 255);		// Green
			wzMsg = g_pVGuiLocalize->Find( "#Msg_DuckStreak1" );
			//pszSoundName = "Announcer.DuckStreak_Level1";
			break;
		case 2:
			cCustomColor = Color( 207, 106, 50, 255);		// Orange
			wzMsg = g_pVGuiLocalize->Find( "#Msg_DuckStreak2" );
			//pszSoundName = "Announcer.DuckStreak_Level2";
			break;
		case 3:
			cCustomColor = Color( 134, 80, 172, 255);		// Purple
			wzMsg = g_pVGuiLocalize->Find( "#Msg_DuckStreak3" );
			//pszSoundName = "Announcer.DuckStreak_Level3";
			break;
		case 4:
			cCustomColor = Color(255, 215, 0, 255);			// Gold
			wzMsg = g_pVGuiLocalize->Find( "#Msg_DuckStreak4" );
			//pszSoundName = "Announcer.DuckStreak_Level4";
			break;
		default:
			cCustomColor = Color(255, 215, 0, 255);			// Still Gold
			wzMsg = g_pVGuiLocalize->Find( "#Msg_DuckStreak5" );
			//pszSoundName = "Announcer.DuckStreak_Level4";
			break;
		}
	}
	else if ( eStreakType == CTFPlayerShared::kTFStreak_Duck_levelup )
	{
		cCustomColor = Color( 255, 215, 0, 255 );			// Gold
		switch ( RandomInt( 1, 3 ) )
		{
			case 1:		wzMsg = g_pVGuiLocalize->Find( "#Msg_DuckLevelup1" );	break;
			case 2:		wzMsg = g_pVGuiLocalize->Find( "#Msg_DuckLevelup2" );	break;
			default:	wzMsg = g_pVGuiLocalize->Find( "#Msg_DuckLevelup3" );	break;
		}
	}
	else
	{
		// Killstreak tiers
		switch ( iStreakTier )
		{
		case 1:
			cCustomColor = Color( 112, 176, 74, 255);		// Green
			wzMsg = g_pVGuiLocalize->Find( "#Msg_KillStreak1" );
			//pszSoundName = "Announcer.KillStreak_Level1";
			break;
		case 2:
			cCustomColor = Color( 207, 106, 50, 255);		// Orange
			wzMsg = g_pVGuiLocalize->Find( "#Msg_KillStreak2" );
			//pszSoundName = "Announcer.KillStreak_Level2";
			break;
		case 3:
			cCustomColor = Color( 134, 80, 172, 255);		// Purple
			wzMsg = g_pVGuiLocalize->Find( "#Msg_KillStreak3" );
			//pszSoundName = "Announcer.KillStreak_Level3";
			break;
		case 4:
			cCustomColor = Color(255, 215, 0, 255);			// Gold
			wzMsg = g_pVGuiLocalize->Find( "#Msg_KillStreak4" );
			//pszSoundName = "Announcer.KillStreak_Level4";
			break;
		default:
			cCustomColor = Color(255, 215, 0, 255);			// Still Gold
			wzMsg = g_pVGuiLocalize->Find( "#Msg_KillStreak5" );
			//pszSoundName = "Announcer.KillStreak_Level4";
			break;
		}
	}

	if ( !wzMsg )
		return;

	// Get player Team for color
	Color cTeamColor(235, 226, 202, 255);
	if ( g_PR->GetTeam( iKillerID ) == TF_TEAM_RED )
	{
		cTeamColor = COLOR_RED;
	}
	else if ( g_PR->GetTeam( iKillerID ) == TF_TEAM_BLUE )
	{
		cTeamColor = COLOR_BLUE;
	}
	
	// Generate the String
	// Count
	wchar_t wzCount[10];
	_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", iStreak );

	// Name
	wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH / 2];
	g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iKillerID ), wszPlayerName, sizeof(wszPlayerName) );

	wchar_t	wTemp[256];
	g_pVGuiLocalize->ConstructString_safe( wTemp, wzMsg, 2, wszPlayerName, wzCount );
	
	HFont hFont = GetStreakFont();
	if ( m_pLabel->GetFont() != hFont )
	{
		m_pLabel->SetFont( hFont );
	}
	m_pLabel->SetText( wTemp );

	// Now go through the string and find the escape characters telling us where the color changes are
	m_pLabel->GetTextImage()->ClearColorChangeStream();

	// We change the title's text color to match the colors of the matching model panel backgrounds
	wchar_t *txt = wTemp;
	int iWChars = 0;
	while ( txt && *txt )
	{
		switch ( *txt )
		{
		case 0x01:	// Normal color
			m_pLabel->GetTextImage()->AddColorChange( Color(235,226,202,cl_hud_killstreak_display_alpha.GetInt() ), iWChars );
			break;
		case 0x02:	// Team color
			m_pLabel->GetTextImage()->AddColorChange( Color( cTeamColor.r(), cTeamColor.g(), cTeamColor.b(), cl_hud_killstreak_display_alpha.GetInt() ), iWChars );
			break;
		case 0x03:	// Item 2 color
			m_pLabel->GetTextImage()->AddColorChange( Color( cCustomColor.r(), cCustomColor.g(), cCustomColor.b(), cl_hud_killstreak_display_alpha.GetInt() ), iWChars );
			break;
		default:
			break;
		}

		txt++;
		iWChars++;
	}

	// Play Local Sound
	int iLocalPlayerIndex = GetLocalPlayerIndex();
	if ( iLocalPlayerIndex == iKillerID && pszSoundName )
	{
		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, pszSoundName );
	}

	m_flLastMessageTime = gpGlobals->realtime + (float)iStreakTier / 2.0;
	SetVisible( true );
}

//-----------------------------------------------------------------------------
bool CTFStreakNotice::IsCurrentStreakHigherPriority( CTFPlayerShared::ETFStreak eStreakType, int iStreak )
{
	// duck level ups are highest priority
	if ( eStreakType == CTFPlayerShared::kTFStreak_Duck_levelup )
		return false;

	if ( !m_nCurrStreakCount )
		return false;

	// Ducks never override kills
	if ( m_nCurrStreakType == CTFPlayerShared::kTFStreak_Kills && eStreakType == CTFPlayerShared::kTFStreak_Ducks )
		return true;

	// But kills always override ducks
	if ( m_nCurrStreakType == CTFPlayerShared::kTFStreak_Ducks && eStreakType == CTFPlayerShared::kTFStreak_Kills )
		return false;

	// Don't stomp a higher streak with a lower, unless it's been around long enough
	float flElapsedTime = gpGlobals->realtime - m_flLastMessageTime;
	float flDisplayMinTime = Max( ( cl_hud_killstreak_display_time.GetFloat() / 3.f ), 1.f );
	return ( iStreak < m_nCurrStreakCount && flElapsedTime < flDisplayMinTime );
}

//-----------------------------------------------------------------------------
HFont CTFStreakNotice::GetStreakFont( void )
{
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );

	const char *pszFontName = "HudFontSmallestBold";
	int nFontSize = cl_hud_killstreak_display_fontsize.GetInt();	// Default is 1: HudFontSmallBold
	if ( nFontSize == 1 )
	{
		pszFontName = "HudFontSmallBold";
	}
	else if ( nFontSize == 2 )
	{
		pszFontName = "HudFontMediumSmallBold";
	}

	return pScheme->GetFont( pszFontName, true );
}

DECLARE_HUDELEMENT( CTFStreakNotice );


//-----------------------------------------------------------------------------
// TFDeathNotice
//-----------------------------------------------------------------------------
class CTFHudDeathNotice : public CHudBaseDeathNotice
{
	DECLARE_CLASS_SIMPLE( CTFHudDeathNotice, CHudBaseDeathNotice );
public:
	CTFHudDeathNotice( const char *pElementName ) : CHudBaseDeathNotice( pElementName ) {};
	virtual void Init( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual bool IsVisible( void );
	virtual bool ShouldDraw( void );

	virtual void FireGameEvent( IGameEvent *event );
	void PlayRivalrySounds( int iKillerIndex, int iVictimIndex, int iType  );
	virtual bool ShouldShowDeathNotice( IGameEvent *event );

protected:	
	virtual void OnGameEvent( IGameEvent *event, int iDeathNoticeMsg );
	virtual Color GetTeamColor( int iTeamNumber, bool bLocalPlayerInvolved = false );
	virtual Color GetInfoTextColor( int iDeathNoticeMsg );
	virtual Color GetBackgroundColor ( int iDeathNoticeMsg );
	virtual bool EventIsPlayerDeath( const char *eventName );

	virtual int UseExistingNotice( IGameEvent *event );

private:
	void AddAdditionalMsg( int iKillerID, int iVictimID, const char *pMsgKey );
	void AddStreakMsg( CTFPlayerShared::ETFStreak eStreakType, int iKillerID, int iKillerStreak, int iStreakIncrement, int iVictimID, int iDeathNoticeMsg );
	void AddStreakEndedMsg( CTFPlayerShared::ETFStreak eStreakType, int iKillerID, int iVictimID, int iVictimStreak, int iDeathNoticeMsg );

	CHudTexture*	GetMannPowerIcon( RuneTypes_t tRuneType, bool bRedTeam );

	CHudTexture		*m_iconDomination;
	CHudTexture		*m_iconKillStreak;
	CHudTexture		*m_iconDuckStreak;
	CHudTexture		*m_iconDuckStreakDNeg;
	CHudTexture		*m_iconKillStreakDNeg;

	CPanelAnimationVar( Color, m_clrBlueText, "TeamBlue", "153 204 255 255" );
	CPanelAnimationVar( Color, m_clrRedText, "TeamRed", "255 64 64 255" );
	CPanelAnimationVar( Color, m_clrPurpleText, "PurpleText", "134 80 172 255" );
	CPanelAnimationVar( Color, m_clrGreenText, "GreenText", "112 176 74 255" );
	CPanelAnimationVar( Color, m_clrLocalPlayerText, "LocalPlayerColor", "65 65 65 255" );

	CTFStreakNotice *m_pStreakNotice;

	bool m_bShowItemOnKill;
};

DECLARE_HUDELEMENT( CTFHudDeathNotice );

void CTFHudDeathNotice::Init()
{
	BaseClass::Init();

	ListenForGameEvent( "fish_notice" );
	ListenForGameEvent( "fish_notice__arm" );
	ListenForGameEvent( "duck_xp_level_up" );
	ListenForGameEvent( "slap_notice" );
	//ListenForGameEvent( "throwable_hit" );

	m_bShowItemOnKill = true;

	// PASSTIME if this is called at level load or something we should check mode before this block
	ListenForGameEvent( PasstimeGameEvents::BallGet::s_eventName );
	ListenForGameEvent( PasstimeGameEvents::BallStolen::s_eventName );
	ListenForGameEvent( PasstimeGameEvents::Score::s_eventName );
	ListenForGameEvent( PasstimeGameEvents::PassCaught::s_eventName );
	ListenForGameEvent( PasstimeGameEvents::BallBlocked::s_eventName );
}

void CTFHudDeathNotice::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_iconDomination = gHUD.GetIcon( "leaderboard_dominated" );
	
	m_iconKillStreak = gHUD.GetIcon( "leaderboard_streak" );
	m_iconKillStreakDNeg = gHUD.GetIcon( "leaderboard_streak_dneg" );
	m_iconDuckStreak = gHUD.GetIcon( "eotl_duck" );
	m_iconDuckStreakDNeg = gHUD.GetIcon( "eotl_duck_dneg" );
	m_pStreakNotice = new CTFStreakNotice( "KillStreakNotice" );
}

bool CTFHudDeathNotice::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

bool CTFHudDeathNotice::ShouldDraw( void )
{
	return true;
}

bool CTFHudDeathNotice::ShouldShowDeathNotice( IGameEvent *event )
{ 
	if ( event->GetBool( "silent_kill" ) )
	{
		// Don't show a kill event for the team of the silent kill victim.
		int iVictimID = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		C_TFPlayer* pVictim = ToTFPlayer( UTIL_PlayerByIndex( iVictimID ) );
		if ( pVictim && pVictim->GetTeamNumber() == GetLocalPlayerTeam() && iVictimID != GetLocalPlayerIndex() )
		{
			return false;
		}
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && ( event->GetInt( "death_flags" ) & TF_DEATH_MINIBOSS ) == 0 )
	{
		int iLocalPlayerIndex = GetLocalPlayerIndex();

		if ( iLocalPlayerIndex != engine->GetPlayerForUserID( event->GetInt( "attacker" ) ) && 
			 iLocalPlayerIndex != engine->GetPlayerForUserID( event->GetInt( "assister" ) ) )
		{
			C_TFPlayer* pVictim = ToTFPlayer( UTIL_PlayerByIndex( engine->GetPlayerForUserID( event->GetInt( "userid" ) ) ) );
			if ( pVictim && pVictim->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
			{
				return false;
			}
		}
	}

	return true;
}

void CTFHudDeathNotice::PlayRivalrySounds( int iKillerIndex, int iVictimIndex, int iType )
{
	int iLocalPlayerIndex = GetLocalPlayerIndex();

	//We're not involved in this kill
	if ( iKillerIndex != iLocalPlayerIndex && iVictimIndex != iLocalPlayerIndex )
		return;

	const char *pszSoundName = NULL;

	if ( iType == TF_DEATH_DOMINATION )
	{
		if ( iKillerIndex == iLocalPlayerIndex )
		{
			pszSoundName = "Game.Domination";
		}
		else if ( iVictimIndex == iLocalPlayerIndex )
		{
			pszSoundName = "Game.Nemesis";
		}
	}
	else if ( iType == TF_DEATH_REVENGE )
	{
		pszSoundName = "Game.Revenge";
	}

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, pszSoundName );
}


//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::FireGameEvent( IGameEvent *event )
{
	const char * pszEventName = event->GetName();
	if ( FStrEq( "duck_xp_level_up", pszEventName ) )
	{
		int level = event->GetInt( "level" );
		AddStreakMsg( CTFPlayerShared::kTFStreak_Duck_levelup, GetLocalPlayerIndex(), level, 1, -1, 0 );
		return;
	}

	BaseClass::FireGameEvent( event );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFHudDeathNotice::EventIsPlayerDeath( const char* eventName )
{
	return FStrEq( eventName, "fish_notice" )
		|| FStrEq( eventName, "fish_notice__arm" )
		|| FStrEq( eventName, "slap_notice" )
		//|| FStrEq( eventName, "throwable_hit" )
		|| BaseClass::EventIsPlayerDeath( eventName );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a game event happens and a death notice is about to be 
//			displayed.  This method can examine the event and death notice and
//			make game-specific tweaks to it before it is displayed
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::OnGameEvent( IGameEvent *event, int iDeathNoticeMsg )
{
	const bool bIsSillyPyroVision = IsLocalPlayerUsingVisionFilterFlags( TF_VISION_FILTER_PYRO );

	const char *pszEventName = event->GetName();

	if ( FStrEq( pszEventName, "player_death" ) || FStrEq( pszEventName, "object_destroyed" ) )
	{
		bool bIsObjectDestroyed = FStrEq( pszEventName, "object_destroyed" );
		int iCustomDamage = event->GetInt( "customkill" );
		int iLocalPlayerIndex = GetLocalPlayerIndex();
		
		const int iKillerID = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		const int iVictimID = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		// if there was an assister, put both the killer's and assister's names in the death message
		int iAssisterID = engine->GetPlayerForUserID( event->GetInt( "assister" ) );

		EHorriblePyroVisionHack ePyroVisionHack = kHorriblePyroVisionHack_KillAssisterType_Default;
		CUtlConstString sAssisterNameScratch;
		const char *assister_name = ( iAssisterID > 0 ? g_PR->GetPlayerName( iAssisterID ) : NULL );
		
		// If we don't have a real assister (would have been passed in to us as a player index) and
		// we're in crazy pyrovision mode and we got a dummy assister, than fall back and display
		// that just for giggles. We use this so the Balloonicorn and friends can get the assist
		// credit they so rightly deserve.
		if ( !assister_name && bIsSillyPyroVision )
		{
			// Ignore this for self-kills.
			if ( bIsObjectDestroyed || (iKillerID != iVictimID) )
			{
				const char *pszMaybeFallbackAssisterName = event->GetString( "assister_fallback" );
				if ( pszMaybeFallbackAssisterName && pszMaybeFallbackAssisterName[0] )
				{
					// We store the type of silly assist in the first byte of the string because we
					// are terrible people.
					ePyroVisionHack = (EHorriblePyroVisionHack)pszMaybeFallbackAssisterName[0];
					Assert( ePyroVisionHack != kHorriblePyroVisionHack_KillAssisterType_Default );
					pszMaybeFallbackAssisterName = &pszMaybeFallbackAssisterName[1];

					// If we pass in a localization string, we need to convert it back to ANSI temporarily.
					// This won't localize "The" Balloonicorn because we don't have a real item with a real
					// quality, etc., just a single localization token.
					switch ( ePyroVisionHack )
					{
						case kHorriblePyroVisionHack_KillAssisterType_LocalizationString:
						case kHorriblePyroVisionHack_KillAssisterType_LocalizationString_First:
						{
							wchar_t *wszLocalizedItemName = GLocalizationProvider()->Find( pszMaybeFallbackAssisterName );
							char szANSIConvertedItemName[ MAX_PLAYER_NAME_LENGTH ];
							g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedItemName, szANSIConvertedItemName, MAX_PLAYER_NAME_LENGTH );
							sAssisterNameScratch = szANSIConvertedItemName;
							assister_name = sAssisterNameScratch.Get();
							break;
						}
						case kHorriblePyroVisionHack_KillAssisterType_CustomName:
						case kHorriblePyroVisionHack_KillAssisterType_CustomName_First:
						{
							sAssisterNameScratch = pszMaybeFallbackAssisterName;
							assister_name = sAssisterNameScratch.Get();
							break;
						}
						default:
							assert( !"Unknown pyro item hack type! Something has gone horribly, horribly worse." );
					}
				}
			}
		}

		bool bMultipleKillers = false;

		if ( assister_name )
		{
			DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];
			const char *pszKillerName = msg.Killer.szName;
			const char *pszAssisterName = assister_name;

			// Check to see if we're swapping the killer and the assister. We use this so the brain slug can get the kill
			// credit for the HUD death notices, with the player being the assister.
			if ( pszAssisterName && (ePyroVisionHack == kHorriblePyroVisionHack_KillAssisterType_CustomName_First ||
									 ePyroVisionHack == kHorriblePyroVisionHack_KillAssisterType_LocalizationString_First) )
			{
				std::swap( pszKillerName, pszAssisterName );
			}

			char szKillerBuf[MAX_PLAYER_NAME_LENGTH*2];
			Q_snprintf( szKillerBuf, ARRAYSIZE(szKillerBuf), "%s + %s", pszKillerName, pszAssisterName );
			Q_strncpy( msg.Killer.szName, szKillerBuf, ARRAYSIZE( msg.Killer.szName ) );
			if ( iLocalPlayerIndex == iAssisterID )
			{
				msg.bLocalPlayerInvolved = true;
			}

			bMultipleKillers = true;
		}

		// play an exciting sound if a sniper pulls off any sort of penetration kill
		const int iPlayerPenetrationCount = !event->IsEmpty( "playerpenetratecount" ) ? event->GetInt( "playerpenetratecount" ) : 0;

		bool bPenetrateSound = iPlayerPenetrationCount > 0;

		// This happens too frequently in Coop/TD
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			bPenetrateSound = false;
		}

		if ( bPenetrateSound )
		{
			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Game.PenetrationKill" );
		}


		int deathFlags = event->GetInt( "death_flags" );

		if ( !bIsObjectDestroyed )
		{
			// if this death involved a player dominating another player or getting revenge on another player, add an additional message
			// mentioning that
			
			// WARNING: AddAdditionalMsg will grow and potentially realloc the m_DeathNotices array. So be careful
			//	using pointers to m_DeathNotices elements...

			if ( deathFlags & TF_DEATH_DOMINATION )
			{
				AddAdditionalMsg( iKillerID, iVictimID, bIsSillyPyroVision ? "#Msg_Dominating_What" : "#Msg_Dominating" );
				PlayRivalrySounds( iKillerID, iVictimID, TF_DEATH_DOMINATION );
			}
			if ( deathFlags & TF_DEATH_ASSISTER_DOMINATION && ( iAssisterID > 0 ) )
			{
				AddAdditionalMsg( iAssisterID, iVictimID, bIsSillyPyroVision ? "#Msg_Dominating_What" : "#Msg_Dominating" );
				PlayRivalrySounds( iAssisterID, iVictimID, TF_DEATH_DOMINATION );
			}
			if ( deathFlags & TF_DEATH_REVENGE ) 
			{
				AddAdditionalMsg( iKillerID, iVictimID, bIsSillyPyroVision ? "#Msg_Revenge_What" : "#Msg_Revenge" );
				PlayRivalrySounds( iKillerID, iVictimID, TF_DEATH_REVENGE );
			}
			if ( deathFlags & TF_DEATH_ASSISTER_REVENGE && ( iAssisterID > 0 ) ) 
			{
				AddAdditionalMsg( iAssisterID, iVictimID, bIsSillyPyroVision ? "#Msg_Revenge_What" : "#Msg_Revenge" );
				PlayRivalrySounds( iAssisterID, iVictimID, TF_DEATH_REVENGE );
			}
		}
		else
		{
			// if this is an object destroyed message, set the victim name to "<object type> (<owner>)"
			int iObjectType = event->GetInt( "objecttype" );
			if ( iObjectType >= 0 && iObjectType < OBJ_LAST )
			{
				// get the localized name for the object
				char szLocalizedObjectName[MAX_PLAYER_NAME_LENGTH];
				szLocalizedObjectName[ 0 ] = 0;
				const wchar_t *wszLocalizedObjectName = g_pVGuiLocalize->Find( szLocalizedObjectNames[iObjectType] );
				if ( wszLocalizedObjectName )
				{
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedObjectName, szLocalizedObjectName, ARRAYSIZE( szLocalizedObjectName ) );
				}
				else
				{
					Warning( "Couldn't find localized object name for '%s'\n", szLocalizedObjectNames[iObjectType] );
					Q_strncpy( szLocalizedObjectName, szLocalizedObjectNames[iObjectType], sizeof( szLocalizedObjectName ) );
				}

				// compose the string
				DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];
				if ( msg.Victim.szName[0] )
				{
					char szVictimBuf[MAX_PLAYER_NAME_LENGTH*2];
					Q_snprintf( szVictimBuf, ARRAYSIZE(szVictimBuf), "%s (%s)", szLocalizedObjectName, msg.Victim.szName );
					Q_strncpy( msg.Victim.szName, szVictimBuf, ARRAYSIZE( msg.Victim.szName ) );
				}
				else
				{
					Q_strncpy( msg.Victim.szName, szLocalizedObjectName, ARRAYSIZE( msg.Victim.szName ) );
				}
				
			}
			else
			{
				Assert( false ); // invalid object type
			}
		}

		const wchar_t *pMsg = NULL;
		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

		switch ( iCustomDamage )
		{
		case TF_DMG_CUSTOM_BACKSTAB:
			if ( FStrEq( msg.szIcon, "d_sharp_dresser" ) )
			{
				Q_strncpy( msg.szIcon, "d_sharp_dresser_backstab", ARRAYSIZE( msg.szIcon ) );
			}
			else
			{
				Q_strncpy( msg.szIcon, "d_backstab", ARRAYSIZE( msg.szIcon ) );
			}
			break;
		case TF_DMG_CUSTOM_HEADSHOT_DECAPITATION:
		case TF_DMG_CUSTOM_HEADSHOT:
			{
				if ( FStrEq( event->GetString( "weapon" ), "ambassador" ) )
				{
					Q_strncpy( msg.szIcon, "d_ambassador_headshot", ARRAYSIZE( msg.szIcon ) );
				}
				else if ( FStrEq( event->GetString( "weapon" ), "huntsman" ) )
				{
					Q_strncpy( msg.szIcon, "d_huntsman_headshot", ARRAYSIZE( msg.szIcon ) );
				}
				else
				{
					// Did this headshot penetrate something before the kill? If so, show a fancy icon
					// so the player feels proud.
					if ( iPlayerPenetrationCount > 0 )
					{
						Q_strncpy( msg.szIcon, "d_headshot_player_penetration", ARRAYSIZE( msg.szIcon ) );
					}
					else
					{
						Q_strncpy( msg.szIcon, "d_headshot", ARRAYSIZE( msg.szIcon ) );
					}
				}
				
				break;
			}
		case TF_DMG_CUSTOM_BURNING:
			if ( event->GetInt( "attacker" ) == event->GetInt( "userid" ) )
			{
				// suicide by fire
				Q_strncpy( msg.szIcon, "d_firedeath", ARRAYSIZE( msg.szIcon ) );
				msg.wzInfoText[0] = 0;
			}
			break;
			
		case TF_DMG_CUSTOM_BURNING_ARROW:
			// special-case if the player is killed from a burning arrow after it has already landed
			Q_strncpy( msg.szIcon, "d_huntsman_burning", ARRAYSIZE( msg.szIcon ) );
			msg.wzInfoText[0] = 0;
			break;

		case TF_DMG_CUSTOM_FLYINGBURN:
			// special-case if the player is killed from a burning arrow as the killing blow
			Q_strncpy( msg.szIcon, "d_huntsman_flyingburn", ARRAYSIZE( msg.szIcon ) );
			msg.wzInfoText[0] = 0;
			break;

		case TF_DMG_CUSTOM_PUMPKIN_BOMB:
			// special-case if the player is killed by a pumpkin bomb
			Q_strncpy( msg.szIcon, "d_pumpkindeath", ARRAYSIZE( msg.szIcon ) );
			msg.wzInfoText[0] = 0;
			break;

		case TF_DMG_CUSTOM_SUICIDE:
			{
				// display a different message if this was suicide, or assisted suicide (suicide w/recent damage, kill awarded to damager)
				bool bAssistedSuicide = event->GetInt( "userid" ) != event->GetInt( "attacker" );
				pMsg = g_pVGuiLocalize->Find( ( bAssistedSuicide ) ? ( bMultipleKillers ? "#DeathMsg_AssistedSuicide_Multiple" : "#DeathMsg_AssistedSuicide" ) : ( "#DeathMsg_Suicide" ) );
				if ( pMsg )
				{
					V_wcsncpy( msg.wzInfoText, pMsg, sizeof( msg.wzInfoText ) );
				}			
				break;
			}

		case TF_DMG_CUSTOM_CROC:
		{
			// display a different message if this was suicide, or assisted suicide (suicide w/recent damage, kill awarded to damager)
			bool bAssistedSuicide = event->GetInt( "attacker" ) && ( event->GetInt( "userid" ) != event->GetInt( "attacker" ) );
			if ( bAssistedSuicide )
			{
				pMsg = g_pVGuiLocalize->Find( "#DeathMsg_AssistedSuicide" );
				if ( pMsg )
				{
					V_wcsncpy( msg.wzInfoText, pMsg, sizeof( msg.wzInfoText ) );
				}
			}
			break;
		}

		case TF_DMG_CUSTOM_EYEBALL_ROCKET:
			{
				if ( msg.Killer.iTeam == TEAM_UNASSIGNED )
				{
					char szLocalizedName[MAX_PLAYER_NAME_LENGTH];
					szLocalizedName[ 0 ] = 0;
					const wchar_t *wszLocalizedName = g_pVGuiLocalize->Find( "#TF_HALLOWEEN_EYEBALL_BOSS_DEATHCAM_NAME" );
					if ( wszLocalizedName )
					{
						g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedName, szLocalizedName, ARRAYSIZE( szLocalizedName ) );
						Q_strncpy( msg.Killer.szName, szLocalizedName, ARRAYSIZE( msg.Killer.szName ) );
						msg.Killer.iTeam = TF_TEAM_HALLOWEEN; // This will set the name to purple for MONOCULUS!
					}
				}
				break;
			}
		case TF_DMG_CUSTOM_MERASMUS_ZAP:
		case TF_DMG_CUSTOM_MERASMUS_GRENADE:
		case TF_DMG_CUSTOM_MERASMUS_DECAPITATION:
			{
				if ( msg.Killer.iTeam == TEAM_UNASSIGNED )
				{
					char szLocalizedName[MAX_PLAYER_NAME_LENGTH];
					szLocalizedName[ 0 ] = 0;
					const wchar_t *wszLocalizedName = g_pVGuiLocalize->Find( "#TF_HALLOWEEN_MERASMUS_DEATHCAM_NAME" );
					if ( wszLocalizedName )
					{
						g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedName, szLocalizedName, ARRAYSIZE( szLocalizedName ) );
						Q_strncpy( msg.Killer.szName, szLocalizedName, ARRAYSIZE( msg.Killer.szName ) );
						msg.Killer.iTeam = TF_TEAM_HALLOWEEN; // This will set the name to green for MERASMUS!
					}
				}
				break;
			}
		case TF_DMG_CUSTOM_SPELL_SKELETON:
			{
				if ( msg.Killer.iTeam == TEAM_UNASSIGNED )
				{
					char szLocalizedName[MAX_PLAYER_NAME_LENGTH];
					szLocalizedName[ 0 ] = 0;
					const wchar_t *wszLocalizedName = g_pVGuiLocalize->Find( "#TF_HALLOWEEN_SKELETON_DEATHCAM_NAME" );
					if ( FStrEq( engine->GetLevelName(), "maps/koth_slime.bsp" ) )
					{
						wszLocalizedName = g_pVGuiLocalize->Find( "#koth_slime_salmann" );
					}

					if ( wszLocalizedName )
					{
						g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedName, szLocalizedName, ARRAYSIZE( szLocalizedName ) );
						Q_strncpy( msg.Killer.szName, szLocalizedName, ARRAYSIZE( msg.Killer.szName ) );
						msg.Killer.iTeam = TF_TEAM_HALLOWEEN; // This will set the name to green for THE UNDEAD!
					}
				}
				break;
			}

		case TF_DMG_CUSTOM_KART:
			// special-case if the player is pushed by kart
			Q_strncpy( msg.szIcon, "d_bumper_kart", ARRAYSIZE( msg.szIcon ) );
			msg.wzInfoText[0] = 0;
			break;
		case TF_DMG_CUSTOM_GIANT_HAMMER:
			// special-case Giant hammer
			Q_strncpy( msg.szIcon, "d_necro_smasher", ARRAYSIZE( msg.szIcon ) );
			msg.wzInfoText[0] = 0;
			break;
		case TF_DMG_CUSTOM_KRAMPUS_MELEE:
			{
				char szLocalizedName[ MAX_PLAYER_NAME_LENGTH ];
				szLocalizedName[ 0 ] = 0;
				const wchar_t *wszLocalizedName = g_pVGuiLocalize->Find( "#koth_krampus_boss" );
				if ( wszLocalizedName )
				{
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedName, szLocalizedName, ARRAYSIZE( szLocalizedName ) );
					Q_strncpy( msg.Killer.szName, szLocalizedName, ARRAYSIZE( msg.Killer.szName ) );
					msg.Killer.iTeam = TF_TEAM_HALLOWEEN; // This will set the name to green for THE UNDEAD!
				}
				Q_strncpy( msg.szIcon, "d_krampus_melee", ARRAYSIZE( msg.szIcon ) );
				msg.wzInfoText[ 0 ] = 0;
				break;
			}
		case TF_DMG_CUSTOM_KRAMPUS_RANGED:
			{
				char szLocalizedName[ MAX_PLAYER_NAME_LENGTH ];
				szLocalizedName[ 0 ] = 0;
				const wchar_t *wszLocalizedName = g_pVGuiLocalize->Find( "#koth_krampus_boss" );
				if ( wszLocalizedName )
				{
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedName, szLocalizedName, ARRAYSIZE( szLocalizedName ) );
					Q_strncpy( msg.Killer.szName, szLocalizedName, ARRAYSIZE( msg.Killer.szName ) );
					msg.Killer.iTeam = TF_TEAM_HALLOWEEN; // This will set the name to green for THE UNDEAD!
				}
				Q_strncpy( msg.szIcon, "d_krampus_ranged", ARRAYSIZE( msg.szIcon ) );
				msg.wzInfoText[ 0 ] = 0;
				break;
			}
		default:
			break;
		}

		if ( ( event->GetInt( "damagebits" ) & DMG_NERVEGAS )  )
		{
			// special case icon for hit-by-vehicle death
			Q_strncpy( msg.szIcon, "d_saw_kill", ARRAYSIZE( msg.szIcon ) );
		}

		int iKillStreakTotal = event->GetInt( "kill_streak_total" );
		int iKillStreakWep = event->GetInt( "kill_streak_wep" );
		int iDuckStreakTotal = event->GetInt( "duck_streak_total" );
		int iDucksThisKill = event->GetInt( "ducks_streaked" );

		// if the active weapon is kill streak
		C_TFPlayer* pKiller = ToTFPlayer( UTIL_PlayerByIndex( iKillerID ) );
		C_TFPlayer* pVictim = ToTFPlayer( UTIL_PlayerByIndex( iVictimID ) );
		C_TFPlayer* pAssister = ToTFPlayer( UTIL_PlayerByIndex( iAssisterID ) );

		// Mannpower runes
		if ( pKiller && pKiller->m_Shared.IsCarryingRune() )
		{
			msg.iconPreKillerName = GetMannPowerIcon( pKiller->m_Shared.GetCarryingRuneType(), pKiller->GetTeamNumber() == TF_TEAM_RED );
		}

		if ( pVictim && pVictim->m_Shared.IsCarryingRune() )
		{
			msg.iconPostVictimName = GetMannPowerIcon( pVictim->m_Shared.GetCarryingRuneType(), pVictim->GetTeamNumber() == TF_TEAM_RED );
		}

		if ( iKillStreakWep > 0 )
		{
			// append kill streak count to this notification
			wchar_t wzCount[10];
			_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", iKillStreakWep );
			g_pVGuiLocalize->ConstructString_safe( msg.wzPreKillerText, g_pVGuiLocalize->Find("#Kill_Streak"), 1, wzCount );
			if ( msg.bLocalPlayerInvolved )
			{
				msg.iconPostKillerName = m_iconKillStreakDNeg;
			}
			else
			{
				msg.iconPostKillerName = m_iconKillStreak;
			}
		}
		else if ( iDuckStreakTotal > 0 && iDucksThisKill )
		{
			// Duckstreak icon (always lower priority)
			wchar_t wzCount[10];
			_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", iDuckStreakTotal );
			g_pVGuiLocalize->ConstructString_safe( msg.wzPreKillerText, g_pVGuiLocalize->Find("#Duck_Streak"), 1, wzCount );
			msg.iconPostKillerName = msg.bLocalPlayerInvolved ? m_iconDuckStreakDNeg : m_iconDuckStreak;
		}

		// Check to see if we want a extra notification
		// Attempt to display these in order of descending priority

		// Check Assister for Additional Messages
		int iKillStreakAssist = event->GetInt( "kill_streak_assist" );
		int iKillStreakVictim = event->GetInt( "kill_streak_victim" );

		// Kills
		AddStreakMsg( CTFPlayerShared::kTFStreak_Kills, iKillerID, iKillStreakTotal, 1, iVictimID, iDeathNoticeMsg );
		if ( pAssister && iKillStreakAssist > 1 )
		{
			AddStreakMsg( CTFPlayerShared::kTFStreak_Kills, iAssisterID, iKillStreakAssist, 1, iVictimID, iDeathNoticeMsg );
		}

		if ( pVictim && iKillStreakVictim > 2 )
		{
			AddStreakEndedMsg( CTFPlayerShared::kTFStreak_Kills, iKillerID, iVictimID, iKillStreakVictim, iDeathNoticeMsg );
		}

		// Ducks
		int iDuckStreakAssist = event->GetInt( "duck_streak_assist" );
		int iDuckStreakVictim = event->GetInt( "duck_streak_victim" );
		int iDuckStreakIncrement = event->GetInt( "ducks_streaked" );

		AddStreakMsg( CTFPlayerShared::kTFStreak_Ducks, iKillerID, iDuckStreakTotal, iDuckStreakIncrement, iVictimID, iDeathNoticeMsg );
		if ( pAssister && iDuckStreakAssist > 0 && iDucksThisKill )
		{
			AddStreakMsg( CTFPlayerShared::kTFStreak_Ducks, iAssisterID, iDuckStreakAssist, iDuckStreakIncrement, iVictimID, iDeathNoticeMsg );
		}

		if ( pVictim && iDuckStreakVictim > 2 )
		{
			AddStreakEndedMsg( CTFPlayerShared::kTFStreak_Ducks, iKillerID, iVictimID, iDuckStreakVictim, iDeathNoticeMsg );
		}

		// STAGING ONLY test
		// If Local Player killed someone and they have an item waiting, let them know
	} 
	else if ( FStrEq( "teamplay_point_captured", pszEventName ) ||
			  FStrEq( "teamplay_capture_blocked", pszEventName ) || 
			  FStrEq( "teamplay_flag_event", pszEventName ) )
	{
		bool bDefense = ( FStrEq( "teamplay_capture_blocked", pszEventName ) || ( FStrEq( "teamplay_flag_event", pszEventName ) &&
			TF_FLAGEVENT_DEFEND == event->GetInt( "eventtype" ) ) );

		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];
		const char *szCaptureIcons[] = { "d_redcapture", "d_bluecapture" };
		const char *szDefenseIcons[] = { "d_reddefend", "d_bluedefend" };
		
		int iTeam = msg.Killer.iTeam;
		Assert( iTeam >= FIRST_GAME_TEAM );
		Assert( iTeam < FIRST_GAME_TEAM + TF_TEAM_COUNT );
		if ( iTeam < FIRST_GAME_TEAM || iTeam >= FIRST_GAME_TEAM + TF_TEAM_COUNT )
			return;

		int iIndex = msg.Killer.iTeam - FIRST_GAME_TEAM;
		Assert( iIndex < ARRAYSIZE( szCaptureIcons ) );

		Q_strncpy( msg.szIcon, bDefense ? szDefenseIcons[iIndex] : szCaptureIcons[iIndex], ARRAYSIZE( msg.szIcon ) );
	}
	else if ( FStrEq( "fish_notice", pszEventName ) || FStrEq( "fish_notice__arm", pszEventName ) || FStrEq( "slap_notice", pszEventName ) )
	{
		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];
		int deathFlags = event->GetInt( "death_flags" );
		int iCustomDamage = event->GetInt( "customkill" );

		if ( ( iCustomDamage == TF_DMG_CUSTOM_FISH_KILL ) || ( deathFlags & TF_DEATH_FEIGN_DEATH ) || ( iCustomDamage == TF_DMG_CUSTOM_SLAP_KILL ) )
		{
			const wchar_t *wpszFormat = g_pVGuiLocalize->Find( "#Humiliation_Kill" );
			if ( FStrEq( "fish_notice__arm", pszEventName ) )
			{
				wpszFormat = g_pVGuiLocalize->Find( "#Humiliation_Kill_Arm" );
			}
			else if ( FStrEq( "slap_notice", pszEventName ) )
			{
				wpszFormat = g_pVGuiLocalize->Find( "#Humiliation_Kill_Slap" );
			}
			g_pVGuiLocalize->ConstructString_safe( msg.wzInfoText, wpszFormat, 0 );
		}
		else
		{
			wchar_t wzCount[10];
			_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", ++msg.iCount );
			g_pVGuiLocalize->ConstructString_safe( msg.wzInfoText, g_pVGuiLocalize->Find("#Humiliation_Count"), 1, wzCount );
		}

		// if there was an assister, put both the killer's and assister's names in the death message
		int iAssisterID = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		const char *assister_name = ( iAssisterID > 0 ? g_PR->GetPlayerName( iAssisterID ) : NULL );
		if ( assister_name )
		{
			char szKillerBuf[MAX_PLAYER_NAME_LENGTH*2];
			Q_snprintf( szKillerBuf, ARRAYSIZE(szKillerBuf), "%s + %s", msg.Killer.szName, assister_name );
			Q_strncpy( msg.Killer.szName, szKillerBuf, ARRAYSIZE( msg.Killer.szName ) );
		}
	}
	//else if ( FStrEq( "throwable_hit", pszEventName ) )
	//{
	//	DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];
	//	int deathFlags = event->GetInt( "death_flags" );
	//	int iCustomDamage = event->GetInt( "customkill" );

	//	// Make sure the icon is up to date
	//	m_DeathNotices[iDeathNoticeMsg].iconDeath = GetIcon( m_DeathNotices[ iDeathNoticeMsg ].szIcon, m_DeathNotices[iDeathNoticeMsg].bLocalPlayerInvolved ? kDeathNoticeIcon_Inverted : kDeathNoticeIcon_Standard );

	//	if ( ( iCustomDamage == TF_DMG_CUSTOM_THROWABLE_KILL ) || ( deathFlags & TF_DEATH_FEIGN_DEATH ) )
	//	{
	//		g_pVGuiLocalize->ConstructString_safe( msg.wzInfoText, g_pVGuiLocalize->Find("#Throwable_Kill"), 0 );
	//	}
	//	else
	//	{
	//		wchar_t wzCount[10];
	//		_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", event->GetInt( "totalhits" ) );
	//		g_pVGuiLocalize->ConstructString_safe( msg.wzInfoText, g_pVGuiLocalize->Find("#Humiliation_Count"), 1, wzCount );
	//	}

	//	// if there was an assister, put both the killer's and assister's names in the death message
	//	int iAssisterID = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
	//	const char *assister_name = ( iAssisterID > 0 ? g_PR->GetPlayerName( iAssisterID ) : NULL );
	//	if ( assister_name )
	//	{
	//		char szKillerBuf[MAX_PLAYER_NAME_LENGTH*2];
	//		Q_snprintf( szKillerBuf, ARRAYSIZE(szKillerBuf), "%s + %s", msg.Killer.szName, assister_name );
	//		Q_strncpy( msg.Killer.szName, szKillerBuf, ARRAYSIZE( msg.Killer.szName ) );
	//	}
	//}
	else if ( FStrEq( "rd_robot_killed", pszEventName ) )
	{
		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

		int killer = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		const char *killedwith = event->GetString( "weapon" );

		// flags
		if ( GetLocalPlayerIndex() == killer )
			msg.bLocalPlayerInvolved = true;

		msg.Killer.iTeam = g_PR->GetTeam( killer );
		Q_strncpy( msg.Killer.szName, g_PR->GetPlayerName( killer ), ARRAYSIZE( msg.Killer.szName ) );

		Q_strncpy( msg.Victim.szName, g_PR->GetTeam( killer ) == TF_TEAM_RED ? "BLUE ROBOT" : "RED ROBOT", ARRAYSIZE( msg.Victim.szName ) );
		msg.Victim.iTeam = g_PR->GetTeam( killer ) == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED;

		Q_snprintf( msg.szIcon, sizeof(msg.szIcon), "d_%s", killedwith );
	}
	else if ( FStrEq( PasstimeGameEvents::BallGet::s_eventName, pszEventName ) ) // passtime ball get
	{
		PasstimeGameEvents::BallGet ev( event );
		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

		// info
		V_wcsncpy( msg.wzInfoText, g_pVGuiLocalize->Find("#Msg_PasstimeBallGet"), sizeof( msg.wzInfoText ) );

		// killer
		const char *szPlayerName = g_PR->GetPlayerName( ev.ownerIndex);
		Q_strncpy( msg.Killer.szName, szPlayerName, ARRAYSIZE( msg.Killer.szName ) );
		msg.Killer.iTeam = g_PR->GetTeam( ev.ownerIndex );

		// flags
		if ( GetLocalPlayerIndex() == ev.ownerIndex )
			msg.bLocalPlayerInvolved = true;

		// icon
		const char *const icon = "d_passtime_pass";
		Q_strncpy( msg.szIcon, icon, ARRAYSIZE( msg.szIcon ) );
	}
	else if ( FStrEq( PasstimeGameEvents::BallStolen::s_eventName, pszEventName ) ) // passtime ball stolen
	{
		PasstimeGameEvents::BallStolen ev( event );
		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

		int attackerTeam = g_PR->GetTeam( ev.attackerIndex );
		int victimTeam = g_PR->GetTeam( ev.victimIndex );

		// attacker
		const char *szPlayerName = g_PR->GetPlayerName( ev.attackerIndex );
		Q_strncpy( msg.Killer.szName, szPlayerName, ARRAYSIZE( msg.Killer.szName ) );
		msg.Killer.iTeam = attackerTeam;

		// victim
		szPlayerName = g_PR->GetPlayerName( ev.victimIndex );
		Q_strncpy( msg.Victim.szName, szPlayerName, ARRAYSIZE( msg.Victim.szName ) );
		msg.Victim.iTeam = victimTeam;

		V_wcsncpy( msg.wzInfoText, g_pVGuiLocalize->Find("#Msg_PasstimeSteal"), sizeof( msg.wzInfoText ) );

		// flags
		int localPlayerIndex = GetLocalPlayerIndex();
		msg.bLocalPlayerInvolved = (localPlayerIndex == ev.attackerIndex)
			|| (localPlayerIndex == ev.victimIndex);

		// icon
		Q_strncpy( msg.szIcon, "d_passtime_steal", ARRAYSIZE(msg.szIcon) );
	}
	else if ( FStrEq( PasstimeGameEvents::Score::s_eventName, pszEventName ) ) // passtime score
	{
		PasstimeGameEvents::Score ev( event );
		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

		// info
		if ( ev.numPoints > 1 )
		{
			wchar_t wzCount[10];
			_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", ev.numPoints );
			g_pVGuiLocalize->ConstructString_safe( msg.wzInfoText, g_pVGuiLocalize->Find("#Msg_PasstimeScoreCount"), 1, wzCount );
		}
		else
		{
			V_wcsncpy( msg.wzInfoText, g_pVGuiLocalize->Find("#Msg_PasstimeScore"), sizeof( msg.wzInfoText ) );
		}
		
		// killer
		const char *szPlayerName = g_PR->GetPlayerName( ev.scorerIndex );
		Q_strncpy( msg.Killer.szName, szPlayerName, ARRAYSIZE( msg.Killer.szName ) );
		msg.Killer.iTeam = g_PR->GetTeam( ev.scorerIndex );

		// flags
		if ( GetLocalPlayerIndex() == ev.scorerIndex )
			msg.bLocalPlayerInvolved = true;

		// icon
		const char *const icon = (msg.Killer.iTeam == TF_TEAM_RED)
			? "d_passtime_score_red"
			: "d_passtime_score_blue";
		Q_strncpy( msg.szIcon, icon, ARRAYSIZE( msg.szIcon ) );
	}
	else if ( FStrEq( PasstimeGameEvents::PassCaught::s_eventName, pszEventName ) ) // passtime pass
	{
		PasstimeGameEvents::PassCaught ev( event );
		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

		int passerTeam = g_PR->GetTeam( ev.passerIndex );
		int catcherTeam = g_PR->GetTeam( ev.catcherIndex );

		//
		// Pass or interception?
		//
		int killerIndex, victimIndex, killerTeam, victimTeam;
		const char *pszDesc;
		if ( passerTeam == catcherTeam )
		{
			// pass
			killerIndex = ev.passerIndex;
			killerTeam = passerTeam;
			victimIndex = ev.catcherIndex;
			victimTeam = catcherTeam;
			pszDesc = "#Msg_PasstimePassComplete";
			Q_strncpy( msg.szIcon, "d_passtime_pass", ARRAYSIZE(msg.szIcon) );
		}
		else
		{
			// interception
			victimIndex = ev.passerIndex;
			victimTeam = passerTeam;
			killerIndex = ev.catcherIndex;
			killerTeam = catcherTeam;
			pszDesc = "#Msg_PasstimeInterception";
			Q_strncpy( msg.szIcon, "d_passtime_intercept", ARRAYSIZE(msg.szIcon) );
		}

		// killer
		const char *szPlayerName = g_PR->GetPlayerName( killerIndex );
		Q_strncpy( msg.Killer.szName, szPlayerName, ARRAYSIZE( msg.Killer.szName ) );
		msg.Killer.iTeam = killerTeam;

		// victim
		szPlayerName = g_PR->GetPlayerName( victimIndex );
		Q_strncpy( msg.Victim.szName, szPlayerName, ARRAYSIZE( msg.Victim.szName ) );
		msg.Victim.iTeam = victimTeam;

		V_wcsncpy( msg.wzInfoText, g_pVGuiLocalize->Find( pszDesc ), sizeof( msg.wzInfoText ) );

		// flags
		int localPlayerIndex = GetLocalPlayerIndex();
		msg.bLocalPlayerInvolved = (localPlayerIndex == ev.catcherIndex)
			|| (localPlayerIndex == ev.passerIndex);
	}
	else if ( FStrEq( PasstimeGameEvents::BallBlocked::s_eventName, pszEventName ) ) // passtime ball stolen
	{
		PasstimeGameEvents::BallBlocked ev( event );
		DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

		// blocker
		const char *szPlayerName = g_PR->GetPlayerName( ev.blockerIndex );
		Q_strncpy( msg.Killer.szName, szPlayerName, ARRAYSIZE( msg.Killer.szName ) );
		msg.Killer.iTeam = g_PR->GetTeam( ev.blockerIndex );

		// owner
		szPlayerName = g_PR->GetPlayerName( ev.ownerIndex );
		Q_strncpy( msg.Victim.szName, szPlayerName, ARRAYSIZE( msg.Victim.szName ) );
		msg.Victim.iTeam = g_PR->GetTeam( ev.ownerIndex );

		V_wcsncpy( msg.wzInfoText, g_pVGuiLocalize->Find("#Msg_PasstimeBlock"), sizeof( msg.wzInfoText ) );

		// flags
		int localPlayerIndex = GetLocalPlayerIndex();
		msg.bLocalPlayerInvolved = (localPlayerIndex == ev.blockerIndex)
			|| (localPlayerIndex == ev.ownerIndex);

		// icon
		Q_strncpy( msg.szIcon, "d_ball", ARRAYSIZE(msg.szIcon) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds an additional death message
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::AddAdditionalMsg( int iKillerID, int iVictimID, const char *pMsgKey )
{
	DeathNoticeItem &msg2 = m_DeathNotices[AddDeathNoticeItem()];
	Q_strncpy( msg2.Killer.szName, g_PR->GetPlayerName( iKillerID ), ARRAYSIZE( msg2.Killer.szName ) );
	msg2.Killer.iTeam = g_PR->GetTeam( iKillerID );
	Q_strncpy( msg2.Victim.szName, g_PR->GetPlayerName( iVictimID ), ARRAYSIZE( msg2.Victim.szName ) );
	msg2.Victim.iTeam = g_PR->GetTeam( iVictimID );
	const wchar_t *wzMsg =  g_pVGuiLocalize->Find( pMsgKey );
	if ( wzMsg )
	{
		V_wcsncpy( msg2.wzInfoText, wzMsg, sizeof( msg2.wzInfoText ) );
	}
	msg2.iconDeath = m_iconDomination;
	int iLocalPlayerIndex = GetLocalPlayerIndex();
	if ( iLocalPlayerIndex == iVictimID || iLocalPlayerIndex == iKillerID )
	{
		msg2.bLocalPlayerInvolved = true;
	}
}

//-----------------------------------------------------------------------------
void CTFHudDeathNotice::AddStreakMsg( CTFPlayerShared::ETFStreak eStreakType, int iKillerID, int iKillerStreak, int iStreakIncrement, int iVictimID, int iDeathNoticeMsg )
{
	int nMinStreak = MinStreakForType( eStreakType );
	if ( iKillerStreak < nMinStreak )
		return;

	if ( !m_pStreakNotice )
		return;

	if ( cl_hud_killstreak_display_time.GetInt() <= 0 )
		return;

	m_pStreakNotice->StreakUpdated( eStreakType, iKillerID, iKillerStreak, iStreakIncrement );
}

//-----------------------------------------------------------------------------
void CTFHudDeathNotice::AddStreakEndedMsg( CTFPlayerShared::ETFStreak eStreakType, int iKillerID, int iVictimID, int iVictimStreak, int iDeathNoticeMsg )
{
	int nMinStreak = MinStreakForType( eStreakType );
	if ( iVictimStreak < nMinStreak )
		return;

	if ( !m_pStreakNotice )
		return;

	if ( cl_hud_killstreak_display_time.GetInt() <= 0 )
		return;

	m_pStreakNotice->StreakEnded( eStreakType, iKillerID, iVictimID, iVictimStreak );
}

//-----------------------------------------------------------------------------
// Purpose: returns the color to draw text in for this team.  
//-----------------------------------------------------------------------------
Color CTFHudDeathNotice::GetTeamColor( int iTeamNumber, bool bLocalPlayerInvolved /* = false */ )
{
	switch ( iTeamNumber )
	{
	case TF_TEAM_BLUE:
		return m_clrBlueText;
		break;
	case TF_TEAM_RED:
		return m_clrRedText;
		break;
	case TEAM_UNASSIGNED:
		if ( bLocalPlayerInvolved )
			return m_clrLocalPlayerText;
		else
			return Color( 255, 255, 255, 255 );
		break;
	case TF_TEAM_HALLOWEEN:
		if ( TFGameRules() && ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_LAKESIDE ) || TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) ) )
		{
			return m_clrGreenText;
		}
		else
		{
			return m_clrPurpleText;
		}
		break;
	default:
		AssertOnce( false );	// invalid team
		return Color( 255, 255, 255, 255 );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CTFHudDeathNotice::GetInfoTextColor( int iDeathNoticeMsg )
{ 
	DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

	if ( msg.bLocalPlayerInvolved )
		return m_clrLocalPlayerText;

	return Color( 255, 255, 255, 255 );
}

//-----------------------------------------------------------------------------
Color CTFHudDeathNotice::GetBackgroundColor ( int iDeathNoticeMsg )
{ 
	DeathNoticeItem &msg = m_DeathNotices[ iDeathNoticeMsg ];

	return msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFHudDeathNotice::UseExistingNotice( IGameEvent *event )
{
	// Fish Notices and Throwables
	// Add check for all throwables
	int iTarget = event->GetInt( "weaponid" );
	if ( ( iTarget == TF_WEAPON_BAT_FISH ) || ( iTarget == TF_WEAPON_SLAP ) || ( iTarget == TF_WEAPON_THROWABLE ) || ( iTarget == TF_WEAPON_GRENADE_THROWABLE ) )
	{
		// Look for a matching pre-existing notice.
		for ( int i=0; i<m_DeathNotices.Count(); ++i )
		{
			DeathNoticeItem &msg = m_DeathNotices[i];

			if ( msg.iWeaponID != iTarget )
				continue;

			if ( msg.iKillerID != event->GetInt( "attacker" ) )
				continue;

			if ( msg.iVictimID != event->GetInt( "userid" ) )
				continue;

			return i;
		}
	}

	return BaseClass::UseExistingNotice( event );
}
//-----------------------------------------------------------------------------
CHudTexture* CTFHudDeathNotice::GetMannPowerIcon( RuneTypes_t tRuneType, bool bIsRedTeam )
{
	// Red team is normal file and blue is dNeg file
	switch ( tRuneType )
	{
	case RUNE_STRENGTH:					return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_strength" ) : gHUD.GetIcon( "dneg_mannpower_strength" );
	case RUNE_HASTE:					return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_haste" ) : gHUD.GetIcon( "dneg_mannpower_haste" );
	case RUNE_REGEN:					return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_regen" ) : gHUD.GetIcon( "dneg_mannpower_regen" );
	case RUNE_RESIST:					return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_resist" ) : gHUD.GetIcon( "dneg_mannpower_resist" );
	case RUNE_VAMPIRE:					return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_vamp" ) : gHUD.GetIcon( "dneg_mannpower_vamp" );
	case RUNE_REFLECT:					return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_reflect" ) : gHUD.GetIcon( "dneg_mannpower_reflect" );
	case RUNE_PRECISION:				return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_precision" ) : gHUD.GetIcon( "dneg_mannpower_precision" );
	case RUNE_AGILITY:					return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_agility" ) : gHUD.GetIcon( "dneg_mannpower_agility" );
	case RUNE_KNOCKOUT:					return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_fist" ) : gHUD.GetIcon( "dneg_mannpower_fist" );
	case RUNE_KING:						return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_king" ) : gHUD.GetIcon( "dneg_mannpower_king" ); 
	case RUNE_PLAGUE:					return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_plague" ) : gHUD.GetIcon( "dneg_mannpower_plague" );
	case RUNE_SUPERNOVA:				return bIsRedTeam ? gHUD.GetIcon( "d_mannpower_supernova" ) : gHUD.GetIcon( "dneg_mannpower_supernova" );
	default:							return NULL;
	}
	return NULL;
}