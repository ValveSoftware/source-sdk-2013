//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

// for messaging with the GC
#include "tf_gcmessages.h"
#include "tf_item_inventory.h"
#include "econ_game_account_server.h"
#include "gc_clientsystem.h"
#include "tf_quickplay.h"

// ui related
#include "filesystem.h"
#include "tf_controls.h"
#include "clientmode_tf.h"
#include "confirm_dialog.h"
#include "econ_controls.h"
#include "game/client/iviewport.h"
#include "ienginevgui.h"
#include "tf_hud_mainmenuoverride.h"
#include "tf_hud_statpanel.h"
#include "tf_mouseforwardingpanel.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui_controls/PanelListPanel.h"
#include "vgui_controls/ProgressBar.h"
#include "vgui_controls/RadioButton.h"
#include "ServerBrowser/blacklisted_server_manager.h"
#include "rtime.h"

#include "c_tf_gamestats.h"
#include "tf_gamerules.h"
#include "ServerBrowser/IServerBrowser.h"

// other
#include "tier1/netadr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

const char *COM_GetModDirectory();
extern int g_iQuickplaySessionIndex;

#include "tf_matchmaking_scoring.h"

ConVar tf_matchmaking_debug( "tf_matchmaking_spew_level",
#ifdef _DEBUG
	"3",
#else
	"1",
#endif
FCVAR_NONE, "Set to 1 for basic console spew of quickplay-related decisions.  4 for maximum verbosity." );

ConVar tf_quickplay_pref_community_servers( "tf_quickplay_pref_community_servers", "0", FCVAR_ARCHIVE, "0=Valve only, 1=Community only, 2=Either" );

#define TF_MATCHMAKING_SPEW( lvl, pStrFmt, ...) \
	if ( tf_matchmaking_debug.GetInt() >= lvl ) \
	{ \
		Msg( pStrFmt, ##__VA_ARGS__ ); \
	}

// Hack to force it to search for other app ID's for testing
static inline int GetTfMatchmakingAppID()
{
	// !TEST!
	// return 440;

	return engine->GetAppID();
}

// A server that we recently were matched to and attempted to join
struct RecentlyMatchedServer
{
	uint32 m_ip;
	uint16 m_port;
	RTime32 m_timeMatched;
};

// Just store them in a simple list.  This will be small, and it won't hurt
// to scan the whole thing, so it won't do any good to do anything fancy
static CUtlVector<RecentlyMatchedServer> s_vecRecentlyMatchedServers;

// Search for a recently matched server.  Also process cooldown expiry
static int FindRecentlyMatchedServer( uint32 ip, uint16 port )
{
	RTime32 timeOfOldestEntryToKeep = CRTime::RTime32TimeCur() - tf_matchmaking_retry_cooldown_seconds.GetInt();
	int i = 0;
	int result = -1;
	while ( i < s_vecRecentlyMatchedServers.Count() )
	{
		// Expire?
		if ( s_vecRecentlyMatchedServers[i].m_timeMatched < timeOfOldestEntryToKeep )
		{
			DevLog(" Expiring quickplay recently joined server entry %08X:%d\n", s_vecRecentlyMatchedServers[i].m_ip, (int)s_vecRecentlyMatchedServers[i].m_port );
			s_vecRecentlyMatchedServers.Remove( i );
		}
		else
		{
			// Address match?
			if ( s_vecRecentlyMatchedServers[i].m_ip == ip && s_vecRecentlyMatchedServers[i].m_port == port )
			{
				Assert( result < 0 ); // dups in the list?
				result = i;
			}

			// Record is still active.  Keep it
			++i;
		}
	}

	// Return index of the entry we found, if any
	return result;
}

ConVar tf_quickplay_pref_increased_maxplayers( "tf_quickplay_pref_increased_maxplayers", "0", FCVAR_ARCHIVE, "0=Default only, 1=Yes, 2=Don't care" );
ConVar tf_quickplay_pref_disable_random_crits( "tf_quickplay_pref_disable_random_crits", "0", FCVAR_ARCHIVE, "0=Random crits enabled, 1=Random crits disabled, 2=Don't care" );
ConVar tf_quickplay_pref_enable_damage_spread( "tf_quickplay_pref_enable_damage_spread", "0", FCVAR_ARCHIVE, "0=Damage spread disabled, 1=Damage spread enabled, 2=Don't care" );
ConVar tf_quickplay_pref_respawn_times( "tf_quickplay_pref_respawn_times", "0", FCVAR_ARCHIVE, "0=Default respawn times only, 1=Instant respawn times ('norespawn' tag), 2=Don't care" );
ConVar tf_quickplay_pref_advanced_view( "tf_quickplay_pref_advanced_view", "0", FCVAR_NONE, "0=Default to simplified view, 1=Default to more detailed options view" );
ConVar tf_quickplay_pref_beta_content( "tf_quickplay_pref_beta_content", "0", FCVAR_ARCHIVE, "0=No beta content, 1=Only beta content" );


static bool BHasTag( const CUtlStringList &TagList, const char *tag )
{
	for ( int i = 0; i < TagList.Count(); i++ )
	{
		if ( !Q_stricmp( TagList[i], tag) )
		{
			return true;
		}
	}
	return false;
}

static void GetQuickplayTags( const QuickplaySearchOptions &opt, CUtlStringList &requiredTags, CUtlStringList &illegalTags )
{
	// Always required
	requiredTags.CopyAndAddToTail( "_registered" );

	// Always illegal
	illegalTags.CopyAndAddToTail( "friendlyfire" );
	illegalTags.CopyAndAddToTail( "highlander" );
	illegalTags.CopyAndAddToTail( "noquickplay" );
	illegalTags.CopyAndAddToTail( "trade" );

	switch ( opt.m_eRespawnTimes )
	{
		case QuickplaySearchOptions::eRespawnTimesDefault:
			illegalTags.CopyAndAddToTail( "respawntimes" );
			illegalTags.CopyAndAddToTail( "norespawntime" );
			break;

		case QuickplaySearchOptions::eRespawnTimesInstant:
			requiredTags.CopyAndAddToTail( "norespawntime" );
			break;

		default:
			Assert( false );
		case QuickplaySearchOptions::eRespawnTimesDontCare:
			break;
	}

	switch ( opt.m_eRandomCrits )
	{
		case QuickplaySearchOptions::eRandomCritsYes:
			illegalTags.CopyAndAddToTail( "nocrits" );
			break;

		case QuickplaySearchOptions::eRandomCritsNo:
			requiredTags.CopyAndAddToTail( "nocrits" );
			break;

		default:
			Assert( false );
		case QuickplaySearchOptions::eRandomCritsDontCare:
			break;
	}

	switch ( opt.m_eDamageSpread )
	{
		case QuickplaySearchOptions::eDamageSpreadYes:
			requiredTags.CopyAndAddToTail( "dmgspread" );
			break;

		case QuickplaySearchOptions::eDamageSpreadNo:
			illegalTags.CopyAndAddToTail( "dmgspread" );
			break;

		default:
			Assert( false );
		case QuickplaySearchOptions::eDamageSpreadDontCare:
			break;
	}

	switch ( opt.m_eMaxPlayers )
	{
		case QuickplaySearchOptions::eMaxPlayers24:
			illegalTags.CopyAndAddToTail( "increased_maxplayers" );
			break;

		case QuickplaySearchOptions::eMaxPlayers30Plus:
			requiredTags.CopyAndAddToTail( "increased_maxplayers" );
			break;

		default:
			Assert( false );
		case QuickplaySearchOptions::eMaxPlayersDontCare:
			break;
	}

	switch ( opt.m_eBetaContent )
	{
		case QuickplaySearchOptions::eBetaYes:
			requiredTags.CopyAndAddToTail( "beta" );
			break;

		case QuickplaySearchOptions::eBetaNo:
			illegalTags.CopyAndAddToTail( "beta" );
			break;

		default:
			Assert( false );
			break;
	}
}

//-----------------------------------------------------------------------------

static void OpenQuickplayDialogCallback( bool bConfirmed, void *pContext )
{
	engine->ClientCmd_Unrestricted( "OpenQuickplayDialog" );
}

// Why does it take so many lines of code to do stuff like this?
static CDllDemandLoader g_ServerBrowser( "ServerBrowser" );
static IServerBrowser *GetServerBrowser()
{
	static IServerBrowser *pServerBrowser = NULL;
	if ( pServerBrowser == NULL )
	{
		int iReturnCode;
		pServerBrowser = (IServerBrowser *)g_ServerBrowser.GetFactory()( SERVERBROWSER_INTERFACE_VERSION, &iReturnCode );
		Assert( pServerBrowser );
	}
	return pServerBrowser;
}

//=============================================================================

enum eGameServerFilter
{
	kGameServerListFilter_Internet,
	kGameServerListFilter_Favorites,
};

struct sortable_gameserveritem_t
{
	sortable_gameserveritem_t()
	{
		m_bRegistered = false;
		m_bValve = false;
		m_bNewUserFriendly = false;
		m_bMapIsQuickPlayOK = false;
		userScore = -999.9f;
		serverScore = -999.9f;
		m_fRecentMatchPenalty = -999.9f;
		m_fTotalScoreFromGC = -9999.9f;
		m_eStatus = TF_Gamestats_QuickPlay_t::k_Server_Invalid;
		m_nOptionsScoreFromGC = INT_MAX;
	}

	gameserveritem_t server;
	bool m_bRegistered;
	bool m_bValve;
	bool m_bNewUserFriendly;
	bool m_bMapIsQuickPlayOK;
	float userScore;
	float serverScore;
	float m_fRecentMatchPenalty;
	float m_fTotalScoreFromGC;
	TF_Gamestats_QuickPlay_t::eServerStatus m_eStatus;
	int m_nOptionsScoreFromGC;

	inline float TotalScore() const { return userScore + serverScore; }
};

// Store a server that has been scored by the GC and
// is ready to receive the final confirmation ping
struct gameserver_ping_queue_entry_t
{
	float m_fTotalScore;
	netadr_t m_adr;
};

//-----------------------------------------------------------------------------
// Purpose: Score a game server based on number of players and ping
//-----------------------------------------------------------------------------
class CGameServerItemSort
{
public:
	bool Less( const sortable_gameserveritem_t *src1, const sortable_gameserveritem_t *src2, void *pCtx )
	{
		// we want higher scores sorted to the front
		return src1->TotalScore() > src2->TotalScore();
	}
	bool Less( const gameserver_ping_queue_entry_t &src1, const gameserver_ping_queue_entry_t &src2, void *pCtx )
	{
		// we want higher scores sorted to the front
		return src1.m_fTotalScore > src2.m_fTotalScore;
	}
};

// !KLUDGE! This is duplicated from serverbrowserQuickListPanel.h
class CQuickListPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CQuickListPanel, vgui::EditablePanel );

public:
	CQuickListPanel( vgui::Panel *parent, const char *panelName );

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	void SetMapName( const char *pMapName );
	void SetImage( const char *pMapName );
	void SetGameType( const char *pGameType );
	const char *GetMapName( void ) { return m_szMapName; }
	void	SetRefreshing( void );

	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnMouseDoublePressed( vgui::MouseCode code );
	void	SetServerInfo ( KeyValues *pKV, int iListID, int iTotalServers );
	int		GetListID( void ) { return m_iListID; }

	virtual void PerformLayout( void )
	{
		BaseClass::PerformLayout();
	}

	MESSAGE_FUNC_INT( OnPanelSelected, "PanelSelected", state )
	{
		if ( state )
		{
			vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );

			if ( pScheme && m_pBGroundPanel )
			{
				m_pBGroundPanel->SetBgColor( pScheme->GetColor("QuickListBGSelected", Color(255, 255, 255, 0 ) ) );
			}
		}
		else
		{
			vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );

			if ( pScheme && m_pBGroundPanel )
			{
				m_pBGroundPanel->SetBgColor( pScheme->GetColor("QuickListBGDeselected", Color(255, 255, 255, 0 ) ) );
			}
		}

		PostMessage( GetParent()->GetVParent(), new KeyValues("PanelSelected") );
	}

private:

	char m_szMapName[64];

	vgui::ImagePanel *m_pLatencyImage;
	vgui::Label	*m_pLatencyLabel;
	vgui::Label	*m_pPlayerCountLabel;
	vgui::Label	*m_pOtherServersLabel;
	vgui::Label	*m_pServerNameLabel;
	vgui::Panel *m_pBGroundPanel;
	vgui::ImagePanel *m_pMapImage;

	vgui::Panel *m_pListPanelParent;
	vgui::Label *m_pGameTypeLabel;
	vgui::Label *m_pMapNameLabel;

	vgui::ImagePanel *m_pReplayImage;

	int m_iListID;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuickListPanel::CQuickListPanel( vgui::Panel* pParent, const char *pElementName ) : BaseClass( pParent, pElementName )
{
	SetParent( pParent );

	m_pListPanelParent = pParent;

	SetScheme( "SourceScheme" );
	SetProportional( false );

	CMouseMessageForwardingPanel *panel = new CMouseMessageForwardingPanel(this, NULL);
	panel->SetZPos(3);

	m_pLatencyImage = new ImagePanel( this, "latencyimage" );
	m_pPlayerCountLabel = new Label( this, "playercount", "" );
	m_pOtherServersLabel = new Label( this, "otherservercount", "" );
	m_pServerNameLabel = new Label( this, "servername", "" );
	m_pBGroundPanel = new Panel( this, "background" );
	m_pMapImage = new ImagePanel( this, "mapimage" );
	m_pGameTypeLabel = new Label( this, "gametype", "" );
	m_pMapNameLabel = new Label( this, "mapname", "" );
	m_pLatencyLabel = new Label( this, "latencytext", "" );
	m_pReplayImage = new ImagePanel( this, "replayimage" );

	const char *pPathID = "PLATFORM";

	if ( g_pFullFileSystem->FileExists( "Servers/QuickListPanel.res", "MOD" ) )
	{
		pPathID = "MOD";
	}
	
	LoadControlSettings( "Servers/QuickListPanel.res", pPathID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuickListPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	if ( pScheme && m_pBGroundPanel )
	{
		m_pBGroundPanel->SetBgColor( pScheme->GetColor("QuickListBGDeselected", Color(255, 255, 255, 0 ) ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuickListPanel::SetRefreshing( void )
{
	if ( m_pServerNameLabel )
	{
		m_pServerNameLabel->SetText( g_pVGuiLocalize->Find("#ServerBrowser_QuickListRefreshing") );
	}

	if ( m_pPlayerCountLabel )
	{
		m_pPlayerCountLabel->SetVisible( false );
	}
	if ( m_pOtherServersLabel )
	{
		m_pOtherServersLabel->SetVisible( false );
	}

	if ( m_pLatencyImage )
	{
		m_pLatencyImage->SetVisible( false );
	}

	if ( m_pReplayImage )
	{
		m_pReplayImage->SetVisible( false );
	}

	if ( m_pLatencyLabel )
	{
		m_pLatencyLabel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuickListPanel::SetMapName( const char *pMapName )
{
	Q_strncpy( m_szMapName, pMapName, sizeof( m_szMapName ) );

	if ( m_pMapNameLabel )
	{
		m_pMapNameLabel->SetText( pMapName );
		m_pMapNameLabel->SizeToContents();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuickListPanel::SetGameType( const char *pGameType )
{
	if ( strlen ( pGameType ) == 0 )
	{
		m_pGameTypeLabel->SetVisible( false );
		return;
	}

	char gametype[ 512 ];
	Q_snprintf( gametype, sizeof( gametype ), "(%s)", pGameType );

	m_pGameTypeLabel->SetText( gametype );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuickListPanel::SetServerInfo ( KeyValues *pKV, int iListID, int iTotalServers )
{
	if ( pKV == NULL )
		return;

	m_iListID = iListID;

	m_pServerNameLabel->SetText( pKV->GetString( "name", " " ) );

	int iPing = pKV->GetInt( "ping", 0 );

	if ( iPing <= 100 )
	{
		m_pLatencyImage->SetImage( "../vgui/icon_con_high.vmt" );
	}
	else if ( iPing <= 150 )
	{
		m_pLatencyImage->SetImage( "../vgui/icon_con_medium.vmt" );
	}
	else
	{
		m_pLatencyImage->SetImage( "../vgui/icon_con_low.vmt" );
	}

	m_pLatencyImage->SetVisible( false );

	//if ( GameSupportsReplay() )
	{
		m_pReplayImage->SetVisible( pKV->GetInt( "Replay", 0 ) > 0 );
	}

	char ping[ 512 ];
	Q_snprintf( ping, sizeof( ping ), "%d ms", iPing );

	m_pLatencyLabel->SetText( ping );
	m_pLatencyLabel->SetVisible( true );

	wchar_t players[ 512 ];
	wchar_t playercount[16];
	wchar_t *pwszPlayers = g_pVGuiLocalize->Find("#ServerBrowser_Players");

	g_pVGuiLocalize->ConvertANSIToUnicode( pKV->GetString( "players", " " ), playercount,  sizeof( playercount ) );

	_snwprintf( players, ARRAYSIZE( players ), L"%ls %ls",  playercount, pwszPlayers );
	
	m_pPlayerCountLabel->SetText( players );
	m_pPlayerCountLabel->SetVisible( true );


	// Now setup the other server count
	if ( iTotalServers == 2 )
	{
		m_pOtherServersLabel->SetText( g_pVGuiLocalize->Find("#ServerBrowser_QuickListOtherServer") );
		m_pOtherServersLabel->SetVisible( true );
	}
	else if ( iTotalServers > 2 )
	{
		wchar_t *pwszServers = g_pVGuiLocalize->Find("#ServerBrowser_QuickListOtherServers");
		_snwprintf( playercount, Q_ARRAYSIZE(playercount), L"%d", (iTotalServers-1) );
		g_pVGuiLocalize->ConstructString_safe( players, pwszServers, 1, playercount );
		m_pOtherServersLabel->SetText( players );
		m_pOtherServersLabel->SetVisible( true );
	}
	else
	{
		m_pOtherServersLabel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuickListPanel::SetImage( const char *pMapName )
{
	char path[ 512 ];
	Q_snprintf( path, sizeof( path ), "materials/vgui/maps/menu_thumb_%s.vmt", pMapName );

	char map[ 512 ];
	Q_snprintf( map, sizeof( map ), "maps/%s.bsp", pMapName );

	if ( g_pFullFileSystem->FileExists( map, "MOD" ) == false  )
	{
		pMapName = "default_download";
	}
	else
	{
		if ( g_pFullFileSystem->FileExists( path, "MOD" ) == false  )
		{
			pMapName = "default";
		}
	}

	if ( m_pMapImage )
	{
		char imagename[ 512 ];
		Q_snprintf( imagename, sizeof( imagename ), "..\\vgui\\maps\\menu_thumb_%s", pMapName );

		m_pMapImage->SetImage ( imagename );
		m_pMapImage->SetMouseInputEnabled( false );
	}							
}

void CQuickListPanel::OnMousePressed( vgui::MouseCode code )
{
	if ( m_pListPanelParent )
	{
		vgui::PanelListPanel *pParent = dynamic_cast < vgui::PanelListPanel *> ( m_pListPanelParent );

		if ( pParent )
		{
			pParent->SetSelectedPanel( this );
			m_pListPanelParent->CallParentFunction( new KeyValues("ItemSelected", "itemID", -1 ) );
		}

		if ( code == MOUSE_RIGHT )
		{
			m_pListPanelParent->CallParentFunction( new KeyValues("OpenContextMenu", "itemID", -1 ) );
		}

	}
}

void CQuickListPanel::OnMouseDoublePressed( vgui::MouseCode code )
{
	if ( code == MOUSE_RIGHT )
		return;

	// call the panel
	OnMousePressed( code );

	m_pListPanelParent->CallParentFunction( new KeyValues("ConnectToServer", "code", code) );
}

CUtlString GetCommaDelimited( const CUtlStringList &list )
{
	CUtlString sResult;
	FOR_EACH_VEC( list, idx )
	{
		if ( idx != 0 )
			sResult.Append( "," );
		sResult.Append( list[idx] );
	}
	return sResult;
}

class CQuickplayWaitDialog;
static CQuickplayWaitDialog *s_pQuickPlayWaitingDialog;

// Busy indicator and also server list if the didn't use "I'm feeling lucky"
class CQuickplayWaitDialog : public CGenericWaitingDialog, public ISteamMatchmakingServerListResponse, public ISteamMatchmakingPingResponse
{
	DECLARE_CLASS_SIMPLE( CQuickplayWaitDialog, CGenericWaitingDialog );
public:	
	CQuickplayWaitDialog( bool bFeelingLucky, const QuickplaySearchOptions &opt )
		: CGenericWaitingDialog( NULL )
		, m_options( opt )
		, m_fHoursPlayed( 0 )
		, m_nAppID( GetTfMatchmakingAppID() )
		, m_hServerListRequest( NULL )
		, m_hServerQueryRequest( HSERVERQUERY_INVALID )
		, m_fPingA( tf_matchmaking_ping_a.GetFloat() )
		, m_fPingAScore( tf_matchmaking_ping_a_score.GetFloat() )
		, m_fPingB( tf_matchmaking_ping_b.GetFloat() )
		, m_fPingBScore( tf_matchmaking_ping_b_score.GetFloat() )
		, m_fPingC( tf_matchmaking_ping_c.GetFloat() )
		, m_fPingCScore( tf_matchmaking_ping_c_score.GetFloat() )
		, m_bFeelingLucky( bFeelingLucky )
		, m_eCurrentStep( k_EStep_GMSQuery )
		, m_timeGCScoreTimeout( 0.0 )
		, m_timeGMSSearchStarted( 0.0 )
		, m_timePingServerTimeout( 0.0 )
		, m_pBusyContainer( NULL )
		, m_pResultsContainer( NULL )
		, m_pServerListPanel( NULL )
		, m_pProgressBar( NULL )
		, m_ScoringNumbers( steamapicontext ? steamapicontext->SteamUser()->GetSteamID() : CSteamID() )
	{

		// Check the panel name since we are changing the resource
		SetName("QuickPlayBusyDialog");
		
		Assert( s_pQuickPlayWaitingDialog == NULL );
		s_pQuickPlayWaitingDialog = this;

		m_blackList.LoadServersFromFile( BLACKLIST_DEFAULT_SAVE_FILE, false );

		m_fHoursPlayed = CTFStatPanel::GetTotalHoursPlayed();
		m_Stats.m_fUserHoursPlayed = m_fHoursPlayed;
		m_Stats.m_iExperimentGroup = m_ScoringNumbers.m_eExperimentGroup;

		// Determine bonus we'll give to valve servers, using linear
		// interpolation with clamped endpoints
		float m_fValveBonusHrsA = tf_matchmaking_numbers_valve_bonus_hrs_a.GetFloat();
		float m_fValveBonusPtsA = tf_matchmaking_numbers_valve_bonus_pts_a.GetFloat();
		float m_fValveBonusHrsB = tf_matchmaking_numbers_valve_bonus_hrs_b.GetFloat();
		float m_fValveBonusPtsB = tf_matchmaking_numbers_valve_bonus_pts_b.GetFloat();
		Assert( m_fValveBonusHrsA < m_fValveBonusHrsB );
		if ( m_fHoursPlayed < m_fValveBonusHrsA )
		{
			m_fValveBonus = m_fValveBonusPtsA;
		}
		else if ( m_fHoursPlayed < m_fValveBonusHrsB )
		{
			m_fValveBonus = lerp( m_fValveBonusHrsA, m_fValveBonusPtsA, m_fValveBonusHrsB, m_fValveBonusPtsB, m_fHoursPlayed );
		}
		else
		{
			m_fValveBonus = m_fValveBonusPtsB;
		}

		// Calc bonus given if the map is noob friendly
		float fNoobTime = tf_matchmaking_noob_hours_played.GetFloat();
		m_fNoobMapBonus = 0.0f;
		if ( m_fHoursPlayed < fNoobTime )
		{
			float fNoobPct = 1.0f - ( fNoobTime / fNoobTime );
			m_fNoobMapBonus = fNoobPct * tf_matchmaking_noob_map_score_boost.GetFloat();
		}

		switch ( m_options.m_eSelectedGameType )
		{
			case kGameCategory_Quickplay: m_Stats.m_sUserGameMode = "(any)"; break;
			case kGameCategory_Escort: m_Stats.m_sUserGameMode = "escort"; break;
			case kGameCategory_CTF: m_Stats.m_sUserGameMode = "ctf"; break;
			case kGameCategory_AttackDefense: m_Stats.m_sUserGameMode = "attackdefense"; break;
			case kGameCategory_Koth: m_Stats.m_sUserGameMode = "koth"; break;
			case kGameCategory_CP: m_Stats.m_sUserGameMode = "cp"; break;
			case kGameCategory_EscortRace: m_Stats.m_sUserGameMode = "escortrace"; break;
			case kGameCategory_EventMix: m_Stats.m_sUserGameMode = "eventmix"; break;
			case kGameCategory_Event247: m_Stats.m_sUserGameMode = "event247"; break;
			case kGameCategory_SD: m_Stats.m_sUserGameMode = "sd"; break;
			case kGameCategory_RobotDestruction: m_Stats.m_sUserGameMode = "rd"; break;
			case kGameCategory_Powerup: m_Stats.m_sUserGameMode = "powerup"; break;
			case kGameCategory_Featured: m_Stats.m_sUserGameMode = "featured"; break;
			case kGameCategory_Passtime: m_Stats.m_sUserGameMode = "passtime"; break;
			case kGameCategory_Community_Update: m_Stats.m_sUserGameMode = "community_update"; break;
			case kGameCategory_Misc: m_Stats.m_sUserGameMode = "misc"; break;
			//case kQuickplayGameType_Arena: stats.m_sUserGameMode = "arena"; break;
			//case kQuickplayGameType_Specialty: stats.m_sUserGameMode = "specialty"; break;
			default:
				Assert( false );
				m_Stats.m_sUserGameMode.Format( "%d", m_options.m_eSelectedGameType );
				break;
		}

		SetDefLessFunc( m_mapServers );

		// Sanity
		Assert( 0.0 < m_fPingA ); Assert( m_fPingA < m_fPingB ); Assert( m_fPingB < m_fPingC );
		Assert( 1.0 > m_fPingAScore ); Assert( m_fPingAScore > m_fPingBScore ); Assert( m_fPingBScore > m_fPingCScore );

		// Setup search filters
		CUtlVector<MatchMakingKeyValuePair_t> vecServerFilters;
		AddFilter( vecServerFilters, "gamedir", COM_GetModDirectory() );
		if ( GetUniverse() == k_EUniversePublic )
		{
			AddFilter( vecServerFilters, "secure", "1" );
			AddFilter( vecServerFilters, "dedicated", "1" );
		}
		AddFilter( vecServerFilters, "full", "1" ); // actually means "not full"

		GetQuickplayTags( m_options, m_vecStrRequiredTags, m_vecStrRejectTags );

		// Specified map filter if one is specified
		if ( !m_options.m_strMapName.IsEmpty() )
		{
			AddFilter( vecServerFilters, "map", m_options.m_strMapName );
		}
		else
		{
			// Add filter for the game mode tag
			switch ( m_options.m_eSelectedGameType )
			{
				case kGameCategory_Escort:
				case kGameCategory_EscortRace:
					m_vecStrRequiredTags.CopyAndAddToTail( "payload" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_Koth:
				case kGameCategory_AttackDefense:
				case kGameCategory_CP:
					m_vecStrRequiredTags.CopyAndAddToTail( "cp" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_CTF:
					m_vecStrRequiredTags.CopyAndAddToTail( "ctf" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_EventMix:
					m_vecStrRequiredTags.CopyAndAddToTail( "eventmix" );
					m_vecStrRejectTags.CopyAndAddToTail( "event247" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_Event247:
					m_vecStrRequiredTags.CopyAndAddToTail( "event247" );
					m_vecStrRejectTags.CopyAndAddToTail( "eventmix" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_SD:
					m_vecStrRequiredTags.CopyAndAddToTail( "sd" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_RobotDestruction:
					m_vecStrRequiredTags.CopyAndAddToTail( "rd" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_Powerup:
					m_vecStrRequiredTags.CopyAndAddToTail( "powerup" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_Featured:
					m_vecStrRequiredTags.CopyAndAddToTail( "featured" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_Passtime:
					m_vecStrRequiredTags.CopyAndAddToTail( "passtime" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_Community_Update:
					m_vecStrRequiredTags.CopyAndAddToTail( "community_update" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_Misc:
					m_vecStrRequiredTags.CopyAndAddToTail( "misc" );
					AddMapsFilter( vecServerFilters, m_options.m_eSelectedGameType );
					break;
				case kGameCategory_Quickplay:
					// Grrrrr.  Anything we can do here?
					// The list of all the maps won't fit.  We do not yet have an "or" syntax.
					// So we're stuck filtering them client side
					break;
				default:
					Assert( false );
					break;
			}
		}

		AddFilter( vecServerFilters, "ngametype", GetCommaDelimited( m_vecStrRejectTags ) );
		AddFilter( vecServerFilters, "gametype", GetCommaDelimited( m_vecStrRequiredTags ) );
		AddFilter( vecServerFilters, "steamblocking", "1" );

		if ( m_options.m_eServers == QuickplaySearchOptions::eServersOfficial )
		{
			AddFilter( vecServerFilters, "white", "1" );
		}
		else if ( m_options.m_eServers == QuickplaySearchOptions::eServersCommunity )
		{
			// community servers *ONLY*
			AddFilter( vecServerFilters, "nor", "1" );
			AddFilter( vecServerFilters, "white", "1" );
		}

		// Remember when we started
		m_timeGMSSearchStarted = Plat_FloatTime();

		// Print for debugging
		if ( tf_matchmaking_debug.GetInt() >= 2 )
		{
			CUtlString sFilter;
			FOR_EACH_VEC( vecServerFilters, idx )
			{
				sFilter.Append( vecServerFilters[idx].m_szKey );
				sFilter.Append( " - " );
				sFilter.Append( vecServerFilters[idx].m_szValue );
				sFilter.Append( "\n" );
			}
			Msg( "Using GMS filter: %s\n", sFilter.String() );
		}

		// Initiate the search
		MatchMakingKeyValuePair_t *pFilters = vecServerFilters.Base(); // <<<< Note, this is weird, but correct.
		m_hServerListRequest = steamapicontext->SteamMatchmakingServers()->RequestInternetServerList( GetTfMatchmakingAppID(), &pFilters, vecServerFilters.Count(), this );
	}

	virtual ~CQuickplayWaitDialog()
	{
		Assert( s_pQuickPlayWaitingDialog == this );
		s_pQuickPlayWaitingDialog = NULL;

		DestroyServerQueryRequest();
		DestroyServerListRequest();
	}

	virtual const char* GetResFile() const { return "Resource/UI/QuickPlayBusyDialog.res"; }

	void OnReceivedGCScores( CMsgTFQuickplay_ScoreServersResponse &msg )
	{

		// Make sure we're expecting them
		if ( m_eCurrentStep != k_EStep_GCScore )
		{
			Warning(" Received CGCTFQuickplay_ScoreServers_Response, but not expecting them (current step = %d)?\n", m_eCurrentStep );
			return;
		}

		m_vecServerJoinQueue.RemoveAll();

		// Do we have any GC scores?
		if ( msg.servers_size() > 0 )
		{
			TF_MATCHMAKING_SPEW( 1, "Received %d server scores from GC\n", msg.servers_size());

			// Put them in a queue.  We will try them in score order
			TF_MATCHMAKING_SPEW( 2, "SteamID, IP, score\n" );
			for ( int i = 0; i < msg.servers_size(); ++i )
			{
				const CMsgTFQuickplay_ScoreServersResponse_ServerInfo &info = msg.servers( i );
				netadr_t adr( info.server_address(), info.server_port() );
				CSteamID steamID( info.steam_id() );

				TF_MATCHMAKING_SPEW( 2, "\"%s\", \"%s\", %.2f\n",
					steamID.Render(), adr.ToString(), info.total_score() );

				gameserver_ping_queue_entry_t item;
				item.m_adr.SetIPAndPort( info.server_address(), info.server_port() );
				item.m_fTotalScore = info.total_score();
				int nOptionsScore = RemapValClamped( info.options_score(), 0.f, 30000.f, tf_mm_options_bonus.GetFloat(), tf_mm_options_penalty.GetFloat() );
				item.m_fTotalScore += nOptionsScore;
				m_vecServerJoinQueue.InsertNoSort( item );

				// Locate the original entry.  Try by IP first,
				// and if that fails, try steam ID.  (Can happen due
				// to mismatch of public/private IP)
				int idx = m_mapServers.Find( adr );
				if ( m_mapServers.IsValidIndex( idx ) )
				{
					// Might actually fail in race condition, but should
					// be extremely rare.  If it's happening frequently,
					// we probably have a bug or have misunderstood something
					Assert( m_mapServers[idx].server.m_steamID == steamID );
				}
				else
				{
					bool bFound = false;
					FOR_EACH_MAP_FAST( m_mapServers, j )
					{
						if ( m_mapServers[j].server.m_steamID == steamID )
						{
							Assert( !m_mapServers.IsValidIndex( idx ) ); // duplicate?
							idx = j;
							bFound = true;
							break;
						}
					}

					if ( bFound )
					{
						// Remove it and re-insert, updating the address
						// we got from the GC, which is probably the public IP.
						// this IP is more useful for scoring purposes
						sortable_gameserveritem_t temp = m_mapServers[idx];
						temp.server.m_NetAdr.SetIP( info.server_address() );
						m_mapServers.RemoveAt( idx );
						idx = m_mapServers.InsertOrReplace( adr, temp );
					}
				}

				if ( m_mapServers.IsValidIndex( idx ) )
				{
					m_mapServers[ idx ].m_eStatus = TF_Gamestats_QuickPlay_t::k_Server_Scored;
					m_mapServers[ idx ].m_fTotalScoreFromGC = info.total_score();
					m_mapServers[ idx ].m_nOptionsScoreFromGC = info.options_score();
				}
				else
				{
					Warning( "Received score from GC for %s @ %s, but I didn't ask for that server!\n", steamID.Render(), adr.ToString() );
					Assert( m_mapServers.IsValidIndex( idx ) );
				}
			}
		} else {
			TF_MATCHMAKING_SPEW(1, "Using client-only scoring (no reputation data)\n" );

			// Just add all of the servers that we thought were good enough to send to the GC
			// to even request a score into the list of possible ones to join
			FOR_EACH_MAP( m_mapServers, i )
			{
				sortable_gameserveritem_t &s = m_mapServers[i];
				if ( s.m_eStatus >= TF_Gamestats_QuickPlay_t::k_Server_RequestedScore )
				{
					gameserver_ping_queue_entry_t item;
					item.m_adr.SetIPAndPort( s.server.m_NetAdr.GetIP(), s.server.m_NetAdr.GetConnectionPort() );
					item.m_fTotalScore = s.TotalScore(); // client-side score only, no reputation data
					m_vecServerJoinQueue.InsertNoSort( item );
				}
			}
		}
		m_vecServerJoinQueue.RedoSort();

		if ( msg.servers_size() > 0 )
		{
			TF_MATCHMAKING_SPEW(1, "Best GC score was %.2f\n", m_vecServerJoinQueue[0].m_fTotalScore );
		}

		// Show some UI
		if ( !m_bFeelingLucky )
		{
			ShowSelectServerUI();
			return;
		}

		// OK, we have some candidates, but let's only try the first few.
		// If they all fail, we probably have some bigger problem.
		// !GAH! CUtlVector SetCount will not truncate an existing list,
		// it will always fill the list with N dummy entries.  I have
		// no idea why.
		while ( m_vecServerJoinQueue.Count() > 5 )
		{
			m_vecServerJoinQueue.Remove( m_vecServerJoinQueue.Count()-1 );
		}

		// OK, we have a server.
		C_CTFGameStats::ImmediateWriteInterfaceEvent( "response(quickplay)", "gc_server_found" );

//		// !FIXME! Server pinging not working for some reason.
//		// Just try to connect to the best one.
//		ConnectToServer( m_vecServerJoinQueue[0].m_adr.GetIP(), m_vecServerJoinQueue[0].m_adr.GetPort() );
//		return;

		// Allright, begin loop pinging servers in order
		// until we find one that is still OK to join.
		// Hopefully and usually, the first one will
		// still be available.  But we sometimes waited
		// too long and something happened on that server
		// during the search and we need to go with our
		// 2nd choice
		m_eCurrentStep = k_EStep_PingCheckNotFull;
		PingNextBestServer();
	}

	void ShowSelectServerUI()
	{
		m_eCurrentStep = k_EStep_SelectServerUI;

		if ( m_pBusyContainer )
			m_pBusyContainer->SetVisible( false );
		if ( m_pResultsContainer )
			m_pResultsContainer->SetVisible( true );
		m_adrCurrentPing.Clear();

		if ( m_pServerListPanel )
		{
			FOR_EACH_VEC( m_vecServerJoinQueue, idxServer )
			{
				int idxMap = m_mapServers.Find( m_vecServerJoinQueue[ idxServer ].m_adr );
				if ( idxMap < 0 )
					continue;
				sortable_gameserveritem_t &s = m_mapServers[ idxMap ];
				CQuickListPanel *pServerPanel = new CQuickListPanel( m_pServerListPanel, "QuickListPanel" );

				char szFriendlyName[MAX_MAP_NAME];
				const char *pszFriendlyGameTypeName = GetServerBrowser()->GetMapFriendlyNameAndGameType( s.server.m_szMap, szFriendlyName, sizeof(szFriendlyName) );

				pServerPanel->SetName( s.server.m_szMap );
				pServerPanel->SetMapName( GetMapDisplayName( s.server.m_szMap ) );
				pServerPanel->SetImage( s.server.m_szMap );
				pServerPanel->SetGameType( pszFriendlyGameTypeName );

				KeyValues *pKV = new KeyValues( "ServerInfo" );

				pKV->SetInt( "ping", s.server.m_nPing );
				pKV->SetString( "name", s.server.GetName() );
				pKV->SetString( "players", CFmtStr( "%d/%d", s.server.m_nPlayers, s.server.m_nMaxPlayers ) );
				pServerPanel->SetServerInfo( pKV, idxMap, 0 );
				pKV->deleteThis();
				pServerPanel->InvalidateLayout();
				pServerPanel->SetVisible( true );
				m_pServerListPanel->AddItem( NULL, pServerPanel );

				// We need to reset the proportional flag again because the AddItem() call does a SetParent()
				// call  which copies the parent's proportional setting to the panel being added
				pServerPanel->SetProportional( false );
			}
			m_pServerListPanel->InvalidateLayout( false, true );
		}
	}

	void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		CGenericWaitingDialog::ApplySchemeSettings( pScheme );

		m_pBusyContainer = dynamic_cast< vgui::EditablePanel* >( FindChildByName( "BusyContainer" ) );
		m_pResultsContainer = dynamic_cast< vgui::EditablePanel* >( FindChildByName( "ResultsContainer" ) );
		if ( m_pBusyContainer == NULL || m_pResultsContainer == NULL )
		{
			Assert( m_pBusyContainer );
			Assert( m_pResultsContainer );
			return;
		}
		m_pBusyContainer->SetVisible( true );
		m_pResultsContainer->SetVisible( false );
		m_pServerListPanel = dynamic_cast< vgui::PanelListPanel* >( m_pResultsContainer->FindChildByName( "ServerList" ) ); Assert( m_pServerListPanel );
		if ( m_pServerListPanel )
			m_pServerListPanel->SetFirstColumnWidth( 0 );
		m_pProgressBar = dynamic_cast< vgui::ProgressBar* >( m_pBusyContainer->FindChildByName( "Progress" ) ); Assert( m_pProgressBar );

		vgui::Label *pTitle = dynamic_cast< vgui::Label* >( m_pBusyContainer->FindChildByName( "TitleLabel" ) ); Assert( pTitle );
		if ( pTitle ) pTitle->SetText( m_bFeelingLucky ? "#TF_MM_WaitDialog_Title_FeelingLucky" : "#TF_MM_WaitDialog_Title_ShowServers" );

		vgui::Panel *pPanel;
		pPanel = m_pBusyContainer->FindChildByName( "CloseButton" ); Assert( pPanel );
		if ( pPanel ) pPanel->AddActionSignalTarget( this );
		pPanel = m_pResultsContainer->FindChildByName( "ConnectButton" ); Assert( pPanel );
		if ( pPanel ) pPanel->AddActionSignalTarget( this );
		pPanel = m_pResultsContainer->FindChildByName( "CancelButton" ); Assert( pPanel );
		if ( pPanel ) pPanel->AddActionSignalTarget( this );
	}

	void ConnectToServer( uint32 iAddress, uint16 iPort )
	{

		SendStats( TF_Gamestats_QuickPlay_t::k_Result_TriedToConnect );

		netadr_t address( iAddress, iPort );
		Msg("Quickplay connecting to %s\n", address.ToString() );

		// Close the window (and queue us for deletion), set our status
		CloseMe();

		// Find/add an entry in the list of recent servers
		int iRecentIndex = FindRecentlyMatchedServer( iAddress, iPort );
		if ( iRecentIndex < 0 )
		{
			iRecentIndex = s_vecRecentlyMatchedServers.AddToTail();
			s_vecRecentlyMatchedServers[ iRecentIndex ].m_ip = iAddress;
			s_vecRecentlyMatchedServers[ iRecentIndex ].m_port = iPort;
		}

		// Set time when we were matched to this server
		s_vecRecentlyMatchedServers[ iRecentIndex ].m_timeMatched = CRTime::RTime32TimeCur();

		const char *pszConnectCode = m_bFeelingLucky ? "quickplay" : "quickpick";
		char command[256];
		Q_snprintf( command, sizeof(command), "connect %s %s_%d\n", address.ToString(), pszConnectCode, g_iQuickplaySessionIndex );
		engine->ClientCmd_Unrestricted( command );
	}

	void OnCommand( const char *pCommand )
	{
		if ( FStrEq( pCommand, "ConnectToServer" ) )
		{
			UserConnectToServer();
			return;
		}
		BaseClass::OnCommand( pCommand );
	}

	virtual void OnUserClose()
	{
		SendStats( TF_Gamestats_QuickPlay_t::k_Result_UserCancel );
		BaseClass::OnUserClose();
	}

protected:

	enum EStep
	{
		k_EStep_GMSQuery, // We're waiting on the GMS to give us back some servers
		k_EStep_GCScore, // We're waiting on the GC to send us back scores
		k_EStep_SelectServerUI, // User wasn't feeling lucky, wanted to see his options first
		k_EStep_PingCheckNotFull, // We're looking for the highest scores server that isn't full
		k_EStep_Terminated, // We've been terminated and are waiting to die
	};
	EStep m_eCurrentStep;
	bool m_bFeelingLucky;

	CUtlMap< netadr_t, sortable_gameserveritem_t > m_mapServers;
	HServerListRequest m_hServerListRequest;
	HServerQuery m_hServerQueryRequest;
	uint32 m_nAppID;
	CBlacklistedServerManager m_blackList;
	QuickplaySearchOptions m_options;
	CUtlStringList m_vecStrRequiredTags;
	CUtlStringList m_vecStrRejectTags;
	float m_fHoursPlayed;
	double m_timeGCScoreTimeout;
	double m_timeGMSSearchStarted;
	double m_timePingServerTimeout;
	netadr_t m_adrCurrentPing;
	vgui::EditablePanel *m_pBusyContainer;
	vgui::EditablePanel *m_pResultsContainer;
	vgui::PanelListPanel *m_pServerListPanel;
	vgui::ProgressBar *m_pProgressBar;

	// Piecewise linear data points for ping->score function
	float m_fPingA, m_fPingAScore;
	float m_fPingB, m_fPingBScore;
	float m_fPingC, m_fPingCScore;
	float m_fValveBonus;
	float m_fNoobMapBonus;

	// List of scores servers we will try to join (after confirming they are still
	// OK), in order
	CUtlSortVector<gameserver_ping_queue_entry_t, CGameServerItemSort> m_vecServerJoinQueue;

	TF2ScoringNumbers_t m_ScoringNumbers;

	TF_Gamestats_QuickPlay_t m_Stats;

	void SendStats( TF_Gamestats_QuickPlay_t::eResult result )
	{

		// Set final result code
		m_Stats.m_eResultCode = result;

		// Duration of total process
		m_Stats.m_fSearchTime = Plat_FloatTime() - m_timeGMSSearchStarted;

		// Populate the server list
		FOR_EACH_MAP( m_mapServers, i )
		{
			sortable_gameserveritem_t &item = m_mapServers[i];
			TF_Gamestats_QuickPlay_t::Server_t &s = m_Stats.m_vecServers[m_Stats.m_vecServers.AddToTail()];
			Assert( item.m_eStatus != TF_Gamestats_QuickPlay_t::k_Server_Invalid );
			s.m_eStatus = item.m_eStatus;
			s.m_ip = item.server.m_NetAdr.GetIP();
			s.m_port = item.server.m_NetAdr.GetConnectionPort();
			s.m_bSecure = item.server.m_bSecure;
			s.m_bRegistered = item.m_bRegistered;
			s.m_bValve = item.m_bValve;
			s.m_nPlayers = item.server.m_nPlayers;
			s.m_nMaxPlayers = item.server.m_nMaxPlayers;
			s.m_sMapName = item.server.m_szMap;
			s.m_sTags = item.server.m_szGameTags;
			s.m_iPing = item.server.m_nPing;
			s.m_bMapIsQuickPlayOK = s.m_bMapIsQuickPlayOK;
			s.m_bMapIsNewUserFriendly = item.m_bNewUserFriendly;
			s.m_fScoreClient = item.userScore;
			s.m_fScoreServer = item.serverScore;
			s.m_fScoreGC = item.m_fTotalScoreFromGC;
		}

		// Send em
		C_CTF_GameStats.QuickplayResults( m_Stats );
	}

//
// ISteamMatchmakingServerListResponse overrides
//

	virtual void ServerResponded( HServerListRequest hRequest, int iServer )
	{
		Assert( m_eCurrentStep == k_EStep_GMSQuery && m_hServerListRequest );
		gameserveritem_t *server = steamapicontext->SteamMatchmakingServers()->GetServerDetails( hRequest, iServer );
		if ( server )
		{
			ServerResponded( *server );
		}
	}

	virtual void ServerFailedToRespond( HServerListRequest hRequest, int iServer )
	{
		// Should only happen during server list process, in which case we don't
		// care.  (We just won't add it to our list.)
		Assert( m_eCurrentStep == k_EStep_GMSQuery && m_hServerListRequest );
	}

	virtual void RefreshComplete( HServerListRequest hRequest, EMatchMakingServerResponse response )
	{
		// Destroy our request.  We'll pick up on this fact next time in Think()
		// and send the best servers to the GC to be scored
		Assert( m_eCurrentStep == k_EStep_GMSQuery && m_hServerListRequest );
		DestroyServerListRequest();
	}

//
// ISteamMatchmakingPingResponse overrides
//

	virtual void ServerResponded( gameserveritem_t &server )
	{

		// Ignore servers with bogus addres.  Is this possible?
		if ( !server.m_NetAdr.GetIP() || !server.m_NetAdr.GetConnectionPort() )
		{
			Assert( server.m_NetAdr.GetIP() && server.m_NetAdr.GetConnectionPort() );
			return;
		}

		// Bogus steam ID?
		if ( !server.m_steamID.IsValid() || !server.m_steamID.BGameServerAccount() || server.m_steamID.GetAccountID() == 0 )
		{
			TF_MATCHMAKING_SPEW( 2, "Quickplay ignoring gameserver at %s with invalid Steam ID %s", server.m_NetAdr.GetQueryAddressString(), server.m_steamID.Render() );
			return;
		}

		//
		// Filter the server
		//
		sortable_gameserveritem_t item;
		item.server = server;
		item.m_bNewUserFriendly = false;
		// XXX(JohnS): This function is no longer available to clients with the matchmaking re-work. If we start using
		//             this UI for valve servers again we will need to fix this somehow.
		// BIsIPInValveAddressSpace( item.server.m_NetAdr.GetIP() );
		item.m_bValve = false;

		// Parse out tags
		CUtlStringList TagList; // auto-deletes strings on scope exit
		if ( item.server.m_szGameTags && item.server.m_szGameTags[0] )
		{
			V_SplitString( item.server.m_szGameTags, ",", TagList );
		}

		// Check if the server is registered
		item.m_bRegistered = BHasTag( TagList, "_registered" );

		const SchemaMap_t *pMapInfo = GetItemSchema()->GetMapForName( item.server.m_szMap );
		if ( pMapInfo != NULL )
		{
			item.m_bMapIsQuickPlayOK = true;
			item.m_bNewUserFriendly = pMapInfo->eQuickplayType == kQuickplay_AllUsers;
		}
		else
		{
			item.m_bMapIsQuickPlayOK = false;
			item.m_bNewUserFriendly = false;
		}

		// Joined recently?
		CalculateRecentMatchPenalty( item );

		// Do some basic filtering.  We put an extremely low score in there if the server fails
		// to match the criteria.  The actual value isn't important here, but it's handy to have
		// different values for different failure criteria for stats.
		int failureCodes = 0;
		if ( !PassesFilter( item.server ) ) failureCodes |= (1<<15);
		if ( !HasAppropriateTags(TagList) ) failureCodes |= (1<<14);
		if ( pMapInfo == NULL )
			failureCodes |= (1<<13); // unknown map
		else
		{

			// Map is known to us, make sure it matches search criteria
			switch ( m_options.m_eSelectedGameType )
			{
				case kGameCategory_EventMix:
					if ( pMapInfo->eGameCategory != kGameCategory_EventMix && pMapInfo->eGameCategory != kGameCategory_Event247 )
						failureCodes |= (1<<12);
					break;

				case kGameCategory_Quickplay:
					// Any map that's in the list will do
					break;

				default:
					// Must match requested game mode
					if ( pMapInfo->eGameCategory != m_options.m_eSelectedGameType )
						failureCodes |= (1<<12);
			}
		}
		if ( server.m_nPlayers >= server.m_nMaxPlayers ) failureCodes |= (1<<11);
		if ( m_blackList.IsServerBlacklisted( server ) ) failureCodes |= (1<<10);

		if ( failureCodes != 0 )
		{
			item.serverScore = item.userScore = -(float)failureCodes;
			item.m_eStatus = TF_Gamestats_QuickPlay_t::k_Server_Ineligible;

			// Check for certain failures that a valve server should never have
			if ( item.m_bValve && ( failureCodes & ( (1<<15) | (1<<14) | (1<<13) ) ) )
			{
				Warning( "Valve-hosted server '%s' does not meet quickplay criteria?\n", server.GetName() );
			}
		}
		else
		{
			// Server pases basic filters and should be scored
			item.m_eStatus = TF_Gamestats_QuickPlay_t::k_Server_Eligible;

			// Start a score based only on the server, not on us
			int nHumans = item.server.m_nPlayers - item.server.m_nBotPlayers;
			int nNumInSearchParty = 1; // we are a party of 1
			item.serverScore = QuickplayCalculateServerScore( nHumans, item.server.m_nBotPlayers, item.server.m_nMaxPlayers, nNumInSearchParty );
			item.serverScore += 6.0f; // !KLUDGE! Add offset to keep score ranges comparable with historical norms.  (We used to give bonuses for some things we now simply require.)

			// Start calculating score based on our own data
			item.userScore = 0.0f;

			// Evaluate piecewise linear ping->score function
			float fPing = (float)item.server.m_nPing;
			float fPingScore = 0.0f;
			if ( fPing < m_fPingA )
			{
				fPingScore = lerp( 0.0f, 1.0f, m_fPingA, m_fPingAScore, fPing );
			}
			else if ( fPing < m_fPingB )
			{
				fPingScore = lerp( m_fPingA, m_fPingAScore, m_fPingB, m_fPingCScore, fPing );
			}
			else
			{
				// note: This continues to get worse and worse if the ping > pingC
				fPingScore = lerp( m_fPingB, m_fPingBScore, m_fPingC, m_fPingCScore, fPing );
			}
			item.userScore += fPingScore;

			// Give a bonus to valve servers
			if ( item.m_bValve )
			{
				item.userScore += m_fValveBonus;
			}

			// weight "newbie" servers a higher
			if ( item.m_bNewUserFriendly )
			{
				item.userScore += m_fNoobMapBonus;
			}

			// Add in recently-joined cooldown factor
			item.userScore -= item.m_fRecentMatchPenalty;

			// Penalty for rule changes
			//item.userScore -= QuickplayCalculateRuleChangePenalty( TagList );

			//
			// Experiment
			//
			switch ( m_ScoringNumbers.m_eExperimentGroup )
			{
				default:
					Assert( !"Bogus experiment group" );
				case TF2ScoringNumbers_t::k_ExperimentGroup_Experiment1_Control:
				case TF2ScoringNumbers_t::k_ExperimentGroup_Experiment1_ValveBiasInactive:
				case TF2ScoringNumbers_t::k_ExperimentGroup_Experiment1_CommunityBiasInactive:
					// No modifications
					break;

				case TF2ScoringNumbers_t::k_ExperimentGroup_Experiment1_ValveBias:

					// Give valve servers with a good ping a significant bonus
					item.userScore -= 1.0f;
					if ( item.m_bValve )
					{
						item.userScore += fPingScore;
					}
					break;

				case TF2ScoringNumbers_t::k_ExperimentGroup_Experiment1_CommunityBias:

					// Give community servers with a good ping a significant bonus
					item.userScore -= 1.0f;
					if ( !item.m_bValve )
					{
						item.userScore += fPingScore;
					}
					break;
			}
		}

		netadr_t netAdr( server.m_NetAdr.GetIP(), server.m_NetAdr.GetConnectionPort() );

		// Are we still in the initial pass, or are we pinging servers?
		if ( m_eCurrentStep == k_EStep_GMSQuery )
		{

			// Insert/update our server map
			m_mapServers.InsertOrReplace( netAdr, item );
		}
		else if ( m_eCurrentStep == k_EStep_PingCheckNotFull || m_eCurrentStep == k_EStep_SelectServerUI )
		{

			// It had better be from who we expect
			if ( netAdr.GetIPNetworkByteOrder() != m_adrCurrentPing.GetIPNetworkByteOrder() || netAdr.GetPort() != m_adrCurrentPing.GetPort() )
			{
				// Of course, we could receive a packet on the network at any
				// time.  But we expect the steam API's to deal with that.
				// But still, just in case....
				Warning( "Received unexpected server ping from %s\n", netAdr.ToString() );
			}
			else
			{

				// !TEST! Simulate server filling up
				//failureCodes |= (1<<11);

				// Find it in the server list; update the result
				int iServerIndex = m_mapServers.Find( m_adrCurrentPing );
				Assert( m_mapServers.IsValidIndex(iServerIndex) );

				if ( failureCodes == 0 )
				{
					if ( m_mapServers.IsValidIndex(iServerIndex) )
					{
						Assert( m_mapServers[iServerIndex].m_eStatus == TF_Gamestats_QuickPlay_t::k_Server_Pinged );
						m_mapServers[iServerIndex].m_eStatus = TF_Gamestats_QuickPlay_t::k_Server_Connected;
					}

					// Still joinable.  JOIN IT!
					TF_MATCHMAKING_SPEW(2, "Reply ping from %s.  Client score is now %.02f.\n", netAdr.ToString(), item.TotalScore() );
					ConnectToServer( netAdr.GetIPHostByteOrder(), netAdr.GetPort() );
				}
				else
				{
					if ( m_mapServers.IsValidIndex(iServerIndex) )
					{
						Assert( m_mapServers[iServerIndex].m_eStatus == TF_Gamestats_QuickPlay_t::k_Server_Pinged );
						m_mapServers[iServerIndex].m_eStatus = TF_Gamestats_QuickPlay_t::k_Server_PingIneligible;
					}

					// We missed the window of opportunity.
					// Move on to our next best choice
					TF_MATCHMAKING_SPEW(2, "Reply ping from %s, but no longer joinable!\n", netAdr.ToString() );
					if ( m_eCurrentStep == k_EStep_SelectServerUI )
					{
						if ( failureCodes & (1<<11) )
							QuickpickPingFailed( "#TF_Quickplay_SelectedServer_Full" );
						else
							QuickpickPingFailed( "#TF_Quickplay_SelectedServer_NoLongerMeetsCriteria" );
					}
					else
					{
						PingNextBestServer();
					}
				}
			}
		}
		else
		{
			// Eh?
			Assert( false );
		}
	}

	virtual void ServerFailedToRespond()
	{
		if ( m_eCurrentStep == k_EStep_PingCheckNotFull || m_eCurrentStep == k_EStep_SelectServerUI )
		{
			TF_MATCHMAKING_SPEW(2, "Timeout waiting on ping reply from %s.\n", m_adrCurrentPing.ToString() );

			// Find it in the server list; record the failure result
			int iServerIndex = m_mapServers.Find( m_adrCurrentPing );
			if ( m_mapServers.IsValidIndex(iServerIndex) )
			{
				Assert( m_mapServers[iServerIndex].m_eStatus == TF_Gamestats_QuickPlay_t::k_Server_Pinged );
				m_mapServers[iServerIndex].m_eStatus = TF_Gamestats_QuickPlay_t::k_Server_PingTimedOut;
			}
			else
			{
				// Wat.
				Assert( m_mapServers.IsValidIndex(iServerIndex) );
			}

			if ( m_eCurrentStep == k_EStep_SelectServerUI )
			{
				QuickpickPingFailed( "#TF_Quickplay_SelectedServer_Timeout" );
			}
			else
			{
				PingNextBestServer();
			}
		}
		else
		{
			// We can get some timeouts after we've finished, during cleanup.
			// Just ignore it
			Assert( m_eCurrentStep == k_EStep_Terminated );
		}
	}

//
// Internal members
//
	bool HasAppropriateTags( const CUtlStringList &TagList )
	{
		FOR_EACH_VEC( m_vecStrRequiredTags, idx )
		{
			if ( !BHasTag( TagList, m_vecStrRequiredTags[idx] ) )
				return false;
		}
		FOR_EACH_VEC( m_vecStrRejectTags, idx )
		{
			if ( BHasTag( TagList, m_vecStrRejectTags[idx] ) )
				return false;
		}
		return true;
	}

	void DestroyServerListRequest()
	{
		if ( m_hServerListRequest )
		{
			steamapicontext->SteamMatchmakingServers()->ReleaseRequest( m_hServerListRequest );
			m_hServerListRequest = NULL;
		}
	}

	void DestroyServerQueryRequest()
	{
		if ( m_hServerQueryRequest != HSERVERQUERY_INVALID )
		{
			steamapicontext->SteamMatchmakingServers()->CancelServerQuery( m_hServerQueryRequest );
			m_hServerQueryRequest = HSERVERQUERY_INVALID;
		}
	}

	static void AddFilter( CUtlVector<MatchMakingKeyValuePair_t> &vecServerFilters, const char *pchKey, const char *pchValue )
	{
		// @note Tom Bui: this is to get around the linker error where we don't like strncpy in MatchMakingKeyValuePair_t's constructor!
		int idx = vecServerFilters.AddToTail();
		MatchMakingKeyValuePair_t &keyvalue = vecServerFilters[idx];	
		Q_strncpy( keyvalue.m_szKey, pchKey, sizeof( keyvalue.m_szKey ) );
		Q_strncpy( keyvalue.m_szValue, pchValue, sizeof( keyvalue.m_szValue ) );
	}

	static void AddMapsFilter( CUtlVector<MatchMakingKeyValuePair_t> &vecServerFilters, EGameCategory t )
	{
		CUtlString sMapList;
		for ( int i = 0 ; i < GetItemSchema()->GetMapCount() ; ++i )
		{
			const SchemaMap_t& map = GetItemSchema()->GetMapForIndex( i );
			int mapType = map.eGameCategory;
			if ( ( mapType == t )  || ( ( mapType == kGameCategory_Event247 ) && ( t == kGameCategory_EventMix ) ) )
			{
				if ( !sMapList.IsEmpty() )
				{
					sMapList.Append( "," );
				}
				sMapList.Append( map.pszMapName );
			}
		}
		MatchMakingKeyValuePair_t kludge;
		if ( sMapList.Length() < sizeof( kludge.m_szValue ) )
		{
			AddFilter( vecServerFilters, "map", sMapList );
		}
		else
		{
			Warning( "List of map names too long for this game mode, cannot filter on server side!\n" );
			Assert( false );
		}
	}

	bool PassesFilter( const gameserveritem_t &server )
	{
		if ( server.m_nAppID != m_nAppID )
		{
			// This should be filtered by the steam matchmaking servers API
			AssertMsg( server.m_nAppID == 0, "Got back server with wrong APP ID?" );
			return false;
		}
		
		if ( server.m_bPassword )
			return false;
		
		if ( GetUniverse() == k_EUniversePublic )
		{
			if ( server.m_bSecure == false )
				return false;
		}

		if ( server.m_nMaxPlayers > kTFQuickPlayMaxPlayers )
			return false;
		if ( server.m_nMaxPlayers < kTFQuickPlayMinMaxNumberOfPlayers )
			return false;
		switch ( m_options.m_eMaxPlayers )
		{
			case QuickplaySearchOptions::eMaxPlayers24:
				if ( server.m_nMaxPlayers != 24 )
					return false;
				break;

			case QuickplaySearchOptions::eMaxPlayers30Plus:
				if ( server.m_nMaxPlayers < 30 )
					return false;
				break;

			default:
				Assert( false );
			case QuickplaySearchOptions::eMaxPlayersDontCare:
				break;
		}

		if ( server.m_steamID.BGameServerAccount() == false )
			return false;		

		return true;
	}

	void CalculateRecentMatchPenalty( sortable_gameserveritem_t &item )
	{
		// Hysteresis
		int iRecentIndex = FindRecentlyMatchedServer( item.server.m_NetAdr.GetIP(), item.server.m_NetAdr.GetConnectionPort() );
		item.m_fRecentMatchPenalty = 0.0f;
		if ( iRecentIndex >= 0 )
		{
			float fAge = (float)(CRTime::RTime32TimeCur() - s_vecRecentlyMatchedServers[iRecentIndex].m_timeMatched);
			if ( fAge < 0.0f )
			{
				// Time running in reverse?!
				Assert( fAge >= 0.0f );
			}
			else
			{
				float fCooldownTime = tf_matchmaking_retry_cooldown_seconds.GetFloat();
				Assert( fCooldownTime > 0.0f );
				if ( fCooldownTime > 0.0f ) // 0 can be used to turn this off, but then all the entries should immediately expire and we shouldn't be here...
				{
					float fAgePct = MIN( fAge / fCooldownTime, 1.0f );
					item.m_fRecentMatchPenalty = (1.0f - fAgePct) * tf_matchmaking_retry_max_penalty.GetFloat();
				}
			}
		}
	}

	virtual void OnThink()
	{
		BaseClass::OnThink();

		double currentTime = Plat_FloatTime();

		// Do work, depending on the current step
		switch ( m_eCurrentStep )
		{
			case k_EStep_GMSQuery:
				CheckSendScoresToGC();
				break;

			case k_EStep_GCScore:
				// Check for timeout
				Assert( m_timeGCScoreTimeout > 0.0 );
				if ( currentTime > m_timeGCScoreTimeout )
				{

					// Well, we never heard back from the GC.  But we needn't let that stop us.
					// We have quite a bit of data, let's just go with what we have, rather
					// than totally failing
					C_CTFGameStats::ImmediateWriteInterfaceEvent( "response(quickplay)", "gc_score_timeout" );
					Warning( "Timed out waiting to get scores back from GC, proceding without reputation data.\n" );

					CMsgTFQuickplay_ScoreServersResponse emptyMsg;
					OnReceivedGCScores( emptyMsg );
				}
				break;

			case k_EStep_SelectServerUI:
				// !FIXME! Ping servers every now and then and remove the full ones?
				break;

			case k_EStep_PingCheckNotFull:

				// Do our own timeout processing
				Assert( m_hServerQueryRequest != HSERVERQUERY_INVALID );
				if ( currentTime > m_timePingServerTimeout || m_hServerQueryRequest == HSERVERQUERY_INVALID )
				{
					ServerFailedToRespond();
				}
				break;

			case k_EStep_Terminated:
				// Just wait to die
				break;

			default:
				Assert( false );
				SendStats( TF_Gamestats_QuickPlay_t::k_Result_InternalError );
				GenericFailure();
				break;
		}
	}

	// Called when we're done, just before we go on to wherever we're going next
	void CloseMe()
	{
		m_eCurrentStep = k_EStep_Terminated;
		DestroyServerListRequest();
		DestroyServerQueryRequest();
		CloseWaitingDialog();
	}

	virtual void GenericFailure()
	{
		CloseMe();
		ShowMessageBox( "#TF_MM_GenericFailure_Title", "#TF_MM_GenericFailure", "#GameUI_OK", &OpenQuickplayDialogCallback );
	}

	virtual void NoServersFoundFailure()
	{
		CloseMe();
		ShowMessageBox( "#TF_MM_ResultsDialog_Title", "#TF_MM_ResultsDialog_ServerNotFound", "#GameUI_OK", &OpenQuickplayDialogCallback );
	}

	void CheckSendScoresToGC()
	{

		// How long have we been searching?
		Assert( m_timeGMSSearchStarted > 0.0 );
		float fSearchDuration = Plat_FloatTime() - m_timeGMSSearchStarted;

		// Get a list of all the servers worth joining, and sort them
		float fScoreThreshold = 1.0f; // !FIXME! Put into convar
		CUtlSortVector< sortable_gameserveritem_t *, CGameServerItemSort > vecGameServers;
		FOR_EACH_MAP_FAST( m_mapServers, idx )
		{
			sortable_gameserveritem_t *pItem = &m_mapServers[idx];
			if ( pItem->TotalScore() > fScoreThreshold )
			{
				vecGameServers.InsertNoSort( pItem );
			}
		}
		vecGameServers.RedoSort();

		// See how long we've been searching, relative to the max time
		Assert( tf_matchmaking_max_search_time.GetFloat() > 0.0 ); // don't be dumb
		float fSearchTimePct = fSearchDuration / tf_matchmaking_max_search_time.GetFloat();

		// Give up based on sheer timeout?
		if ( fSearchTimePct > 1.0 )
		{
			// Time to give up
			TF_MATCHMAKING_SPEW(1, "Quickplay GMS search timed out.  We'll go with what we've found so far.\n" );
			DestroyServerListRequest();
		}

		// Check if we should keep searching
		if ( m_hServerListRequest )
		{

			// Determine "good enough" tolerances
			float fScoreLo = Lerp( fSearchTimePct, tf_matchmaking_goodenough_score_start.GetFloat(), tf_matchmaking_goodenough_score_end.GetFloat() );
			float fCountLo = Lerp( fSearchTimePct, tf_matchmaking_goodenough_count_start.GetFloat(), tf_matchmaking_goodenough_count_end.GetFloat() );
			//float fScoreHi = Lerp( fSearchTimePct, tf_matchmaking_goodenough_hi_score_start.GetFloat(), tf_matchmaking_goodenough_hi_score_end.GetFloat() );
			//float fCountHi = Lerp( fSearchTimePct, tf_matchmaking_goodenough_hi_count_start.GetFloat(), tf_matchmaking_goodenough_hi_count_end.GetFloat() );

			// !TEST! Hack this to force us to just show some servers
			//fScoreLo = 3.0f;
			//fCountLo = 10.0f;

			// Now scan list to see if we have enough results at this point
			int iGoodEnoughCount = 0;
			for ( iGoodEnoughCount = 0 ; iGoodEnoughCount < vecGameServers.Count() ; ++iGoodEnoughCount)
			{
				sortable_gameserveritem_t *pItem = vecGameServers[ iGoodEnoughCount ];
				if ( pItem->TotalScore() < fScoreLo )
				{
					// This guy and all subsequent ones don't meet the criteria.
					break;
				}
				Assert( pItem->m_eStatus == TF_Gamestats_QuickPlay_t::k_Server_Eligible );
			}

			// See how many more are eligible, just not meeting our criteria to terminate the search
			int iEligibleCount = iGoodEnoughCount;
			while ( iEligibleCount < vecGameServers.Count() && vecGameServers[iEligibleCount]->m_eStatus > TF_Gamestats_QuickPlay_t::k_Server_Ineligible )
			{
				Assert( vecGameServers[iEligibleCount]->m_eStatus == TF_Gamestats_QuickPlay_t::k_Server_Eligible );
				++iEligibleCount;
			}

			// Update the dialog
			if ( m_pBusyContainer )
			{
				locchar_t wszValue[64];
				locchar_t wszText[256];
				loc_sprintf_safe( wszValue, LOCCHAR( "%u" ), iEligibleCount );
				g_pVGuiLocalize->ConstructString_safe( wszText, g_pVGuiLocalize->Find( "#TF_Quickplay_NumGames" ), 1, wszValue );
				m_pBusyContainer->SetDialogVariable( "numservers", wszText );
			}

			// Found enough good servers to bail now?
			if ( iGoodEnoughCount >= (int)(fCountLo + 0.5f) )
			{
				TF_MATCHMAKING_SPEW(1, "Quickplay initial search: %d servers with scores %.2f or better.  Good enough...proceeding to backend scoring.\n", iGoodEnoughCount, fScoreLo );
			}
			else
			{
				// We haven't found enough good servers.  Update progress indicator
				// to show that we're doing something
				if ( m_pProgressBar )
				{
					// Determine percentage completion based on "good enough" criteria.
					float fGoodEnoughPct = (float)iGoodEnoughCount / (float)fCountLo;

					// Use max of this indicator, or basic timeout.  That way,
					// there is always some motion on the progress bar
					float fProgressPct = MAX( fGoodEnoughPct, fSearchTimePct );

					// Set it
					m_pProgressBar->SetProgress( fProgressPct );
				}

				// Keep searching
				return;
			}

			// OK, we think we've got enough decent servers to see what the GC thinks.
			// No need to keep talking to the GMS
			DestroyServerListRequest();
		}

		// Set progress bar to 100%
		if ( m_pProgressBar )
		{
			m_pProgressBar->SetProgress( 1.0f );
		}

		// Decide how many to score.
		const int iNumServersToScore = MIN( vecGameServers.Count(), kTFMaxQuickPlayServersToScore );

		//
		// Dump some debug stuff to the console
		//
		TF_MATCHMAKING_SPEW( 1, "Quickplay client scoring\n" );
		TF_MATCHMAKING_SPEW( 1, "Selected game mode: %d\n", m_options.m_eSelectedGameType );
		TF_MATCHMAKING_SPEW( 1, "Hours played: %.1f\n", m_fHoursPlayed );
		TF_MATCHMAKING_SPEW( 1, "Valve server bonus: %.2f, noob map bonus: %.2f\n", m_fValveBonus, m_fNoobMapBonus );
		TF_MATCHMAKING_SPEW( 1, "Search duration: %.1f\n", fSearchDuration );
		TF_MATCHMAKING_SPEW( 1, "Found %d total servers, %d met minimum score threshold, %d will be sent to GC for scoring\n", m_mapServers.Count(), vecGameServers.Count(), iNumServersToScore );
		CUtlSortVector< sortable_gameserveritem_t *, CGameServerItemSort > vecGameServersToPrint;
		if ( tf_matchmaking_debug.GetInt() >= 4 )
		{
			FOR_EACH_MAP_FAST( m_mapServers, idx )
			{
				vecGameServersToPrint.InsertNoSort( &m_mapServers[idx] );
			}
		}
		else if ( tf_matchmaking_debug.GetInt() >= 3 )
		{
			for ( int i = 0 ; i < vecGameServers.Count() ; ++i)
			{
				vecGameServersToPrint.InsertNoSort( vecGameServers[i] );
			}
		}
		else if ( tf_matchmaking_debug.GetInt() >= 2 )
		{
			for ( int i = 0 ; i < iNumServersToScore ; ++i)
			{
				vecGameServersToPrint.InsertNoSort( vecGameServers[i] );
			}
		}
		if ( vecGameServersToPrint.Count() > 0 )
		{
			vecGameServersToPrint.RedoSort();
			Msg( "Name, address, numplayers, maxplayers, ping, new user friendly, registered, valve, recent match penalty, user score, server score, total score, steamID, map, tags\n" );

			// Print in reverse order --- the bottom is the most interesting part and the log usually scrolls.
			// Anybody looking at a file can pull it into excel and sort however they want
			for ( int i = vecGameServersToPrint.Count()-1 ; i >=0 ; --i )
			{
				sortable_gameserveritem_t &item = *vecGameServersToPrint[i];
				Msg( "\"%s\",\"%s\",%d,%d,%d,%d,%d,%d,%.2f,%7.4f,%7.4f,%7.4f,\"%s\",\"%s\",\"%s\"\n",
					item.server.GetName(),
					item.server.m_NetAdr.GetConnectionAddressString(),
					item.server.m_nPlayers - item.server.m_nBotPlayers, item.server.m_nMaxPlayers, item.server.m_nPing,
					item.m_bNewUserFriendly ? 1 : 0,
					item.m_bRegistered ? 1 : 0,
					item.m_bValve ? 1 : 0,
					item.m_fRecentMatchPenalty,
					item.userScore, item.serverScore, item.serverScore+item.userScore,
					item.server.m_steamID.Render(),
					item.server.m_szMap,
					item.server.m_szGameTags
				);
			}
		}

		// *NOW* Check for failure.  (We waited until now, so we can properly log it.)
		if ( iNumServersToScore <= 0 )
		{
			C_CTFGameStats::ImmediateWriteInterfaceEvent( "response(quickplay)", "servers_not_found" );
			SendStats( m_mapServers.Count() > 0 ?
				TF_Gamestats_QuickPlay_t::k_Result_NoServersMetCrtieria :
				TF_Gamestats_QuickPlay_t::k_Result_NoServersFound );
			NoServersFoundFailure();
			return;
		}

		// send message to GC
		GCSDK::CProtoBufMsg< CMsgTFQuickplay_ScoreServers > msg( k_EMsgGC_QP_ScoreServers );
		for ( int i = 0; i < iNumServersToScore; ++i )
		{
			sortable_gameserveritem_t *pItem = vecGameServers[i];
			gameserveritem_t &server = pItem->server;
			Assert( pItem->m_eStatus == TF_Gamestats_QuickPlay_t::k_Server_Eligible );
			pItem->m_eStatus = TF_Gamestats_QuickPlay_t::k_Server_RequestedScore;
			CMsgTFQuickplay_ScoreServers_ServerInfo *pServerInfo = msg.Body().add_servers();
			pServerInfo->set_server_address( server.m_NetAdr.GetIP() );
			pServerInfo->set_server_port( server.m_NetAdr.GetConnectionPort() );
			pServerInfo->set_num_users( server.m_nPlayers - server.m_nBotPlayers );
			pServerInfo->set_max_users( server.m_nMaxPlayers );
			pServerInfo->set_user_score( pItem->userScore );
			pServerInfo->set_steam_id( server.m_steamID.ConvertToUint64() );
		}

		m_eCurrentStep = k_EStep_GCScore;

		//// !TEST! Skip scoring
		//CMsgTFQuickplay_ScoreServersResponse emptyMsg;
		//OnReceivedGCScores( emptyMsg );
		//return;

		GCClientSystem()->BSendMessage( msg );

		// Give the GC some time to respond.  It usually wil respond very quickly
		m_timeGCScoreTimeout = Plat_FloatTime() + 10.0f;
	}

	void UserConnectToServer()
	{
		Assert( m_eCurrentStep == k_EStep_SelectServerUI );
		if ( m_adrCurrentPing.IsValid() )
			return;
		CQuickListPanel *pPanel = dynamic_cast<CQuickListPanel*>( m_pServerListPanel->GetSelectedPanel() );
		if ( pPanel == NULL )
			return;
		int idx = pPanel->GetListID();
		m_adrCurrentPing = m_mapServers.Key( idx );
		PingServer();
	}

	void QuickpickPingFailed( const char *pszLocTok )
	{
		ShowMessageBox( "#TF_Quickplay_Error", pszLocTok, "#GameUI_OK" );

		// Remove this guy
		for ( int i = 0 ; i < m_vecServerJoinQueue.Count() ; ++i )
		{
			if ( m_vecServerJoinQueue[i].m_adr == m_adrCurrentPing )
			{
				m_vecServerJoinQueue.Remove(i);
				if ( m_pServerListPanel )
					m_pServerListPanel->RemoveItem(i);
				break;
			}
		}

		m_adrCurrentPing.Clear();

		// !KLUDGE! If the list goes empty, then just act like they hit ESC.
		if ( m_pServerListPanel && m_pServerListPanel->GetItemCount() == 0 )
			OnCommand( "user_close" );
	}

	void PingNextBestServer()
	{
		Assert( m_eCurrentStep == k_EStep_PingCheckNotFull );

		// If we already have any ping activity going, cancel it.  (We shouldn't)
		DestroyServerQueryRequest();

		// Any more options to try?
		if ( m_vecServerJoinQueue.Count() < 1 )
		{
			TF_MATCHMAKING_SPEW( 1, "No more scored servers left to ping.  We failed!\n" );
			SendStats( TF_Gamestats_QuickPlay_t::k_Result_FinalPingFailed );
			NoServersFoundFailure();
			return;
		}

		// Remove the next address from the queue
		m_adrCurrentPing = m_vecServerJoinQueue[0].m_adr;
		m_vecServerJoinQueue.Remove( 0 );

		// Do it
		PingServer();
	}

	void PingServer()
	{
		// If we already have any ping activity going, cancel it.  (We shouldn't)
		DestroyServerQueryRequest();

		// Find it in the server list; mark it as being pinged
		int iServerIndex = m_mapServers.Find( m_adrCurrentPing );
		if ( m_mapServers.IsValidIndex(iServerIndex) )
		{
			Assert( m_mapServers[iServerIndex].m_eStatus >= TF_Gamestats_QuickPlay_t::k_Server_Eligible );
			m_mapServers[iServerIndex].m_eStatus = TF_Gamestats_QuickPlay_t::k_Server_Pinged;
		}
		else
		{
			// Wat.
			Assert( m_mapServers.IsValidIndex(iServerIndex) );
		}

		// Send the ping
		TF_MATCHMAKING_SPEW( 2, "Pinging %s\n", m_adrCurrentPing.ToString() );
		m_hServerQueryRequest = steamapicontext->SteamMatchmakingServers()->PingServer( m_adrCurrentPing.GetIPHostByteOrder(), m_adrCurrentPing.GetPort(), this );
		Assert( m_hServerQueryRequest != HSERVERQUERY_INVALID );

		// Set timeout.  Since we've already pinged them once
		// fairly recently, and we are considering joining this
		// server, we don't want to wait around forever.  They
		// should reply to the ping pretty quickly or let's
		// not join them.
		m_timePingServerTimeout = Plat_FloatTime() + 1.0;
	}

	virtual void OnKeyCodeTyped( vgui::KeyCode code )
	{
		if( code == KEY_ESCAPE )
		{
			OnCommand( "user_close" );
		}
		else
		{
			BaseClass::OnKeyCodeTyped( code );
		}
	}

};

class CStandaloneQuickplayMenu : public CQuickplayWaitDialog
{
	DECLARE_CLASS_SIMPLE( CStandaloneQuickplayMenu, CQuickplayWaitDialog );
public:	
	CStandaloneQuickplayMenu( bool bFeelingLucky, const QuickplaySearchOptions &opt )
		: BaseClass( bFeelingLucky, opt )
	{}

	virtual void GenericFailure() OVERRIDE
	{
		CloseMe();
		ShowMessageBox( "#TF_MM_GenericFailure_Title", "#TF_MM_GenericFailure", "#GameUI_OK", NULL );
	}

	virtual void NoServersFoundFailure() OVERRIDE
	{
		CloseMe();
		ShowMessageBox( "#TF_MM_ResultsDialog_Title", "#TF_MM_ResultsDialog_ServerNotFound", "#GameUI_OK", NULL );
	}
};

//-----------------------------------------------------------------------------
// Purpose: Quickplay Dialog
//-----------------------------------------------------------------------------
ConVar tf_quickplay_lastviewedmode( "tf_quickplay_lastviewedmode", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );

CQuickplayPanelBase::CQuickplayPanelBase( vgui::Panel *parent, const char *name )
: vgui::EditablePanel( parent, name )
, m_iCurrentItem( 0 )
, m_pContainer( NULL )
, m_bSetInitialSelection( false )
, m_pPrevPageButton( NULL )
, m_pNextPageButton( NULL )
, m_bShowRandomOption( true )
, m_pSimplifiedOptionsContainer( NULL )
, m_pAdvOptionsContainer( NULL )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme(scheme);
	SetProportional( true );
}
CQuickplayPanelBase::~CQuickplayPanelBase()
{
	vgui::ivgui()->RemoveTickSignal( GetVPanel() );
}

void CQuickplayPanelBase::ShowItemByGameType( EGameCategory type )
{
	for ( int i = 0 ; i < m_vecItems.Count() ; ++i )
	{
		if ( m_vecItems[i].gameType == type )
		{
			ShowItemByIndex( i );
			return;
		}
	}

	AssertMsg1( false, "Bogus quickplay type %d", type );
}

void CQuickplayPanelBase::ShowItemByIndex( int iItem )
{
	Assert( iItem >= 0 );
	Assert( iItem < m_vecItems.Size() );
	m_iCurrentItem = iItem;

	QuickplayItem &item = m_vecItems[ m_iCurrentItem ];
	
	if ( m_pGameModeInfoContainer )
	{
		m_pGameModeInfoContainer->SetDialogVariable( "gametype", g_pVGuiLocalize->Find( item.pTitle ) );
		m_pGameModeInfoContainer->SetDialogVariable( "description", g_pVGuiLocalize->Find( item.pDescription ) );
		m_pGameModeInfoContainer->SetDialogVariable( "complexity", g_pVGuiLocalize->Find( item.pComplexity ) );

		if ( m_pMoreInfoContainer )
			m_pMoreInfoContainer->SetDialogVariable( "more_info", g_pVGuiLocalize->Find( item.pMoreInfo ) );

		vgui::ImagePanel *pImage = dynamic_cast< vgui::ImagePanel* >( m_pGameModeInfoContainer->FindChildByName( "ModeImage" ) );
		if ( pImage )
		{
			pImage->SetImage( GetItemImage( item ) );
		}
	}

	char szTmp[16];
	Q_snprintf(szTmp, 16, "%d/%d", m_iCurrentItem + 1, m_vecItems.Size() );
	if ( m_pSimplifiedOptionsContainer )
		m_pSimplifiedOptionsContainer->SetDialogVariable( "page", szTmp );

	if ( m_pGameModeCombo )
		m_pGameModeCombo->SilentActivateItemByRow( iItem );
		
	WriteOptionCombosAndSummary();
}

const char *CQuickplayPanelBase::GetItemImage( const QuickplayItem& item ) const
{
	return item.pImage;
}

void CQuickplayPanelBase::SetupActionTarget( const char *pPanelName )
{
	vgui::Panel *pPanel = m_pContainer->FindChildByName( pPanelName, true );
	if ( pPanel )
	{
		pPanel->AddActionSignalTarget( this );
	}
}

void CQuickplayPanelBase::AddItem( EGameCategory gameType, const char *pTitle, const char *pDescription, const char *pMoreInfo, const char *pComplexity, const char *pImage, const char *pBetaImage )
{
	int idx = m_vecAllItems.AddToTail();
	QuickplayItem &item = m_vecAllItems[idx];
	item.gameType = gameType;
	item.pTitle = pTitle;
	item.pDescription = pDescription;
	item.pMoreInfo = pMoreInfo;
	item.pComplexity = pComplexity;
	item.pImage = pImage;
	item.pBetaImage = pBetaImage;

	if ( m_pGameModeCombo )
		m_pGameModeCombo->AddItem( pTitle, NULL );
}

void CQuickplayPanelBase::UpdateSelectableItems()
{
	m_vecItems = m_vecAllItems;
}

void CQuickplayPanelBase::ApplySettings( KeyValues *pInResourceData )
{
	BaseClass::ApplySettings( pInResourceData );

	V_strcpy_safe( m_szEvent247Image, pInResourceData->GetString( "event247_image", "illustrations/gamemode_halloween" ) );
	V_strcpy_safe( m_szCommunityUpdateImage, pInResourceData->GetString( "community_update_image", "illustrations/gamemode_cp" ) );
}

void CQuickplayPanelBase::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pSimplifiedOptionsContainer = dynamic_cast< vgui::EditablePanel *>( FindChildByName( "SimplifiedOptionsContainer", true ) ); Assert( m_pSimplifiedOptionsContainer );
	if ( m_pSimplifiedOptionsContainer )
	{
		m_pGameModeInfoContainer = dynamic_cast< vgui::EditablePanel *>( m_pSimplifiedOptionsContainer->FindChildByName( "ModeInfoContainer", true ) ); Assert( m_pGameModeInfoContainer );
	}
	if ( m_pGameModeInfoContainer )
	{
		m_pMoreInfoContainer = dynamic_cast< vgui::EditablePanel *>( m_pGameModeInfoContainer->FindChildByName( "MoreInfoContainer", true ) ); Assert( m_pMoreInfoContainer );
	}
	m_pPrevPageButton = dynamic_cast< vgui::Button *>( FindChildByName( "PrevPageButton", true ) ); Assert( m_pPrevPageButton );
	m_pNextPageButton = dynamic_cast< vgui::Button *>( FindChildByName( "NextPageButton", true ) ); Assert( m_pNextPageButton );
	m_pMoreOptionsButton = dynamic_cast< vgui::Button *>( FindChildByName( "OptionsButton", true ) );
	m_pAdvOptionsContainer = dynamic_cast< vgui::EditablePanel *>( FindChildByName( "AdvOptionsContainer", true ) );
	m_pGameModeCombo = NULL;
	m_pOptionsSummaryLabel = NULL;
	if ( m_pAdvOptionsContainer )
	{
		Panel *pGameModeOptionContainer = m_pAdvOptionsContainer->FindChildByName( "GameModeOptionContainer", true ); Assert( pGameModeOptionContainer );

		if ( pGameModeOptionContainer )
		{
			m_pGameModeCombo = dynamic_cast< vgui::ComboBox *>( pGameModeOptionContainer->FindChildByName( "OptionCombo", true ) );
			Assert( m_pGameModeCombo );
		}
		if ( m_pGameModeCombo )
			m_pGameModeCombo->AddActionSignalTarget( this );
		if ( m_pContainer )
		{
			m_pOptionsSummaryLabel = dynamic_cast< vgui::Label *>( m_pContainer->FindChildByName( "OptionsSummaryLabel", true ) );
			Assert( m_pOptionsSummaryLabel );
		}
	}

	//m_pFavoritesCheckButton = dynamic_cast< vgui::CheckButton *>( m_pContainer->FindChildByName( "FavoritesCheckButton" ) );
	//m_pFavoritesCheckButton->AddActionSignalTarget( this );
	//
	//m_pRefreshButton = dynamic_cast< vgui::Button *>( m_pContainer->FindChildByName( "RefreshButton" ) );
	//m_pRefreshButton->AddActionSignalTarget( this );

	m_vecItems.RemoveAll();
	m_vecAllItems.RemoveAll();

	// listed in the order we want to show them
	extern bool TF_IsHolidayActive( int eHoliday );
	bool bHalloween = TF_IsHolidayActive( kHoliday_Halloween );
	if ( bHalloween )
	{
		AddItem( kGameCategory_Event247,	"#Gametype_Halloween247",		"#TF_GameModeDesc_Halloween247",		"#TF_GameModeDetail_Halloween247",		"#TF_Quickplay_Complexity1",	m_szEvent247Image,								NULL );
		AddItem( kGameCategory_EventMix, 	"#Gametype_HalloweenMix", 		"#TF_GameModeDesc_HalloweenMix",		"#TF_GameModeDetail_HalloweenMix", 		"#TF_Quickplay_Complexity1",	"illustrations/gamemode_halloween",				NULL );
	}
//	AddItem( kGameType_Community_Update,"#GameType_Community_Update",	"#TF_GameModeDesc_Community_Update",	"#TF_GameModeDetail_Community_Update",	"#TF_Quickplay_Complexity1",	m_szCommunityUpdateImage,						NULL );
//	AddItem( kGameType_Featured,		"#GameType_Featured",			"#TF_GameModeDesc_Featured",			"#TF_GameModeDetail_Featured",			"#TF_Quickplay_Complexity1",	"illustrations/gamemode_operation_tough_break",	NULL );
	AddItem( kGameCategory_Escort, 			"#Gametype_Escort", 			"#TF_GameModeDesc_Escort",				"#TF_GameModeDetail_Escort", 			"#TF_Quickplay_Complexity1",	"illustrations/gamemode_payload",				"illustrations/gamemode_payload_beta" );
	AddItem( kGameCategory_Koth, 			"#Gametype_Koth", 				"#TF_GameModeDesc_Koth", 				"#TF_GameModeDetail_Koth", 				"#TF_Quickplay_Complexity1",	"illustrations/gamemode_koth",					NULL );
	AddItem( kGameCategory_AttackDefense, 	"#Gametype_AttackDefense",		"#TF_GameModeDesc_AttackDefense", 		"#TF_GameModeDetail_AttackDefense",		"#TF_Quickplay_Complexity1",	"illustrations/gamemode_attackdefend",			NULL );
	AddItem( kGameCategory_EscortRace,		"#Gametype_EscortRace", 		"#TF_GameModeDesc_EscortRace", 			"#TF_GameModeDetail_EscortRace", 		"#TF_Quickplay_Complexity2",	"illustrations/gamemode_payloadrace",			NULL );
	AddItem( kGameCategory_CP, 				"#Gametype_CP", 				"#TF_GameModeDesc_CP", 					"#TF_GameModeDetail_CP", 				"#TF_Quickplay_Complexity2",	"illustrations/gamemode_cp",					NULL );
	AddItem( kGameCategory_CTF, 			"#Gametype_CTF", 				"#TF_GameModeDesc_CTF", 				"#TF_GameModeDetail_CTF", 				"#TF_Quickplay_Complexity2",	"illustrations/gamemode_ctf",					NULL );
	AddItem( kGameCategory_Misc, 			"#Gametype_Misc", 				"#TF_GameModeDesc_Misc", 				"#TF_GameModeDetail_Misc", 				"#TF_Quickplay_Complexity2",	"illustrations/gamemode_sd",					NULL );
	AddItem( kGameCategory_Powerup,			"#GameType_Powerup",			"#TF_GameModeDesc_Powerup",				"#TF_GameModeDetail_Powerup",			"#TF_Quickplay_Complexity3",	"illustrations/gamemode_powerup",				"illustrations/gamemode_powerup_beta" ); // Fix beta image once Heather has the image
	AddItem( kGameCategory_Passtime,		"#GameType_Passtime",			"#TF_GameModeDesc_Passtime",			"#TF_GameModeDetail_Passtime",			"#TF_Quickplay_Complexity2",	"illustrations/gamemode_passtime",				"illustrations/gamemode_passtime_beta" );
	AddItem( kGameCategory_RobotDestruction,"#Gametype_RobotDestruction",	"#TF_GameModeDesc_RobotDestruction",	"#TF_GameModeDetail_RobotDestruction",	"#TF_Quickplay_Complexity2",	"illustrations/gamemode_sd",					"illustrations/gamemode_robotdestruction_beta" );
	if ( m_bShowRandomOption )
		AddItem( kGameCategory_Quickplay, 		"#Gametype_Quickplay", 		"#TF_GameModeDesc_Quickplay",		"#TF_GameModeDetail_Quickplay", 	"#TF_Quickplay_Complexity2", "illustrations/quickplay", "illustrations/quickplay_beta" );

	// AddItem( kQuickplayGameType_Arena, 		"#Gametype_Arena", 			"#TF_GameModeDesc_Arena",			"#TF_GameModeDetail_Arena", "#TF_Quickplay_Complexity3", "maps/menu_photos_cp_granary" );
	// AddItem( kQuickplayGameType_Specialty, 	"#Gametype_Specialty", 		"#TF_GameModeDesc_Specialty",		"#TF_GameModeDetail_Specialty", "#TF_Quickplay_Complexity3", "maps/menu_photos_cp_granary" );

	UpdateSelectableItems();
	SetupMoreOptions();

	// set current, if we didn't already
	if ( !m_bSetInitialSelection )
	{
		m_iCurrentItem = tf_quickplay_lastviewedmode.GetInt();
		static bool bForcedOnce = false;
		//tagES need to remove this when Operation Gun Mettle is finished
		if ( /* bHalloween && */ !bForcedOnce )
		{
			m_iCurrentItem = 0;
			bForcedOnce = true;
		}
		m_bSetInitialSelection = true;
	}
	m_iCurrentItem = Min( m_iCurrentItem, m_vecItems.Count()-1 );
	m_iCurrentItem = Max( m_iCurrentItem, 0 );
	ShowItemByIndex( m_iCurrentItem );

	SetupActionTarget( "MoreInfoButton" );
	SetupActionTarget( "PrevPageButton" );
	SetupActionTarget( "NextPageButton" );
	SetupActionTarget( "CancelButton" );

	ShowSimplifiedOrAdvancedOptions();
}

void CQuickplayPanelBase::SetupMoreOptions()
{
	SetupActionTarget( "OptionsButton" );

//	Panel *pOptionTemplate = m_pMoreOptionsContainer->FindChildByName( "OptionContainerTemplate" );
//	Assert( pOptionTemplate );
//	KeyValues *pTemplateSettings = new KeyValues( "Template" );
//	pOptionTemplate->GetSettings( pTemplateSettings );
//	pOptionTemplate->SetVisible( false );
//
//	new QuickplayOptionPanel( m_pMoreOptionsContainer, 0, pTemplateSettings );
//	new QuickplayOptionPanel( m_pMoreOptionsContainer, 1, pTemplateSettings );
//	new QuickplayOptionPanel( m_pMoreOptionsContainer, 2, pTemplateSettings );
//
//	pTemplateSettings->deleteThis();

	AdvOption *pOpt;
	COMPILE_TIME_ASSERT( kEAdvOption_ServerHost == 0 );
	pOpt = &m_vecAdvOptions[ m_vecAdvOptions.AddToTail() ];
	pOpt->m_pszContainerName = "ValveServerOption";
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_ServerHost_Official" ); pOpt->m_vecOptionSummaryNames.AddToTail( "#TF_Quickplay_ServerHost_Official_Summary" );
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_ServerHost_Community" ); pOpt->m_vecOptionSummaryNames.AddToTail( "#TF_Quickplay_ServerHost_Community_Summary" );
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_ServerHost_DontCare" ); pOpt->m_vecOptionSummaryNames.AddToTail( "#TF_Quickplay_ServerHost_DontCare_Summary" );
	pOpt->m_pConvar = &tf_quickplay_pref_community_servers;

	COMPILE_TIME_ASSERT( kEAdvOption_MaxPlayers == 1 );
	pOpt = &m_vecAdvOptions[ m_vecAdvOptions.AddToTail() ];
	pOpt->m_pszContainerName = "IncreasedPlayerCountOption";
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_MaxPlayers_Default" ); pOpt->m_vecOptionSummaryNames.AddToTail( "" );
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_MaxPlayers_Increased" ); pOpt->m_vecOptionSummaryNames.AddToTail( "#TF_Quickplay_MaxPlayers_Increased_Summary" );
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_MaxPlayers_DontCare" ); pOpt->m_vecOptionSummaryNames.AddToTail( "#TF_Quickplay_MaxPlayers_DontCare_Summary" );
	pOpt->m_pConvar = &tf_quickplay_pref_increased_maxplayers;

	COMPILE_TIME_ASSERT( kEAdvOption_Respawn == 2 );
	pOpt = &m_vecAdvOptions[ m_vecAdvOptions.AddToTail() ];
	pOpt->m_pszContainerName = "RespawnTimesOption";
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_RespawnTimes_Default" ); pOpt->m_vecOptionSummaryNames.AddToTail( "" );
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_RespawnTimes_Instant" ); pOpt->m_vecOptionSummaryNames.AddToTail( "#TF_Quickplay_RespawnTimes_Instant_Summary" );
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_RespawnTimes_DontCare" ); pOpt->m_vecOptionSummaryNames.AddToTail( "#TF_Quickplay_RespawnTimes_DontCare_Summary" );
	pOpt->m_pConvar = &tf_quickplay_pref_respawn_times;

	COMPILE_TIME_ASSERT( kEAdvOption_RandomCrits == 3 );
	pOpt = &m_vecAdvOptions[ m_vecAdvOptions.AddToTail() ];
	pOpt->m_pszContainerName = "RandomCritsOption";
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_RandomCrits_Default" ); pOpt->m_vecOptionSummaryNames.AddToTail( "" );
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_RandomCrits_Disabled" ); pOpt->m_vecOptionSummaryNames.AddToTail( "#TF_Quickplay_RandomCrits_Disabled_Summary" );
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_RandomCrits_DontCare" ); pOpt->m_vecOptionSummaryNames.AddToTail( "#TF_Quickplay_RandomCrits_DontCare_Summary" );
	pOpt->m_pConvar = &tf_quickplay_pref_disable_random_crits;

	COMPILE_TIME_ASSERT( kEAdvOption_DamageSpread == 4 );
	pOpt = &m_vecAdvOptions[ m_vecAdvOptions.AddToTail() ];
	pOpt->m_pszContainerName = "DamageSpreadOption";
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_DamageSpread_Default" ); pOpt->m_vecOptionSummaryNames.AddToTail( "" );
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_DamageSpread_Enabled" ); pOpt->m_vecOptionSummaryNames.AddToTail( "#TF_Quickplay_DamageSpread_Enabled_Summary" );
	pOpt->m_vecOptionNames.AddToTail( "#TF_Quickplay_DamageSpread_DontCare" ); pOpt->m_vecOptionSummaryNames.AddToTail( "#TF_Quickplay_DamageSpread_DontCare_Summary" );
	pOpt->m_pConvar = &tf_quickplay_pref_enable_damage_spread;

	FOR_EACH_VEC( m_vecAdvOptions, idxAdvOpt )
	{
		pOpt = &m_vecAdvOptions[ idxAdvOpt ];
		EditablePanel *pContainer = dynamic_cast<EditablePanel*>( FindChildByName( pOpt->m_pszContainerName, true ) );
		FOR_EACH_VEC( pOpt->m_vecOptionNames, idx )
		{
			vgui::RadioButton *pRadioButton = NULL;
			if ( pContainer )
			{
				pRadioButton = dynamic_cast<vgui::RadioButton*>( pContainer->FindChildByName( VarArgs("RadioButton%d", idx ) ) );
				Assert( pRadioButton );
			}
			if ( pRadioButton )
			{
				pRadioButton->AddActionSignalTarget( this );
				pRadioButton->SetText( pOpt->m_vecOptionNames[idx] );
			}
			pOpt->m_vecRadioButtons.AddToTail( pRadioButton );
		}
	}

	// Populate choice from the convars
	FOR_EACH_VEC( m_vecAdvOptions, idxAdvOpt )
	{
		AdvOption *pOpt = &m_vecAdvOptions[ idxAdvOpt ];
		pOpt->m_nChoice = pOpt->m_pConvar->GetInt();
	}
}

void CQuickplayPanelBase::ShowSimplifiedOrAdvancedOptions()
{
	if ( m_pSimplifiedOptionsContainer )
		m_pSimplifiedOptionsContainer->SetVisible( !tf_quickplay_pref_advanced_view.GetBool() );
	if ( m_pAdvOptionsContainer )
		m_pAdvOptionsContainer->SetVisible( tf_quickplay_pref_advanced_view.GetBool() );
}

const size_t kQuickplayOptionsSummaryLen = 1024;
static void AppendOptionInfo( wchar_t *wszText, const char *pszLocToken )
{
	if ( pszLocToken == NULL || *pszLocToken == '\0' )
		return;
	const wchar_t *pwszLocalized = NULL;
	if ( *pszLocToken == '#' )
		pwszLocalized = g_pVGuiLocalize->Find( pszLocToken );
	wchar_t wszTemp[ 1024 ];
	if ( pwszLocalized == NULL )
	{
		V_UTF8ToUnicode( pszLocToken, wszTemp, sizeof(wszTemp) );
		pwszLocalized = wszTemp;
	}
	if ( *wszText != '\0' )
		V_wcsncat( wszText, L"; ", kQuickplayOptionsSummaryLen );
	V_wcsncat( wszText, pwszLocalized, kQuickplayOptionsSummaryLen );
}

void CQuickplayPanelBase::WriteOptionCombosAndSummary()
{
	wchar_t wszSmmary[ kQuickplayOptionsSummaryLen ] = {};
	
	GetOptionsAndSummaryText( wszSmmary );

	if ( m_pOptionsSummaryLabel )
		m_pOptionsSummaryLabel->SetText( wszSmmary );
}

void CQuickplayPanelBase::GetOptionsAndSummaryText( wchar_t *pwszSummary )
{
	if ( m_vecItems[m_iCurrentItem].gameType == kGameCategory_Quickplay )
		AppendOptionInfo( pwszSummary, "#Gametype_AnyGameMode" ); // "random" looks weird here, use custom text
	else
		AppendOptionInfo( pwszSummary, m_vecItems[m_iCurrentItem].pTitle );

	// Check if we need to force "community only" or "don't care" server host due to their options
	FOR_EACH_VEC( m_vecAdvOptions, idxVecOpt )
	{
		if ( idxVecOpt == kEAdvOption_ServerHost )
			continue;
		if ( m_vecAdvOptions[idxVecOpt].m_nChoice == 1 )
		{
			m_vecAdvOptions[kEAdvOption_ServerHost].m_nChoice = QuickplaySearchOptions::eServersCommunity;
			break;
		}
	}

	FOR_EACH_VEC( m_vecAdvOptions, idxAdvOpt )
	{
		AdvOption *pOpt = &m_vecAdvOptions[ idxAdvOpt ];

		FOR_EACH_VEC( pOpt->m_vecRadioButtons, idxRadButton )
		{
			if ( pOpt->m_vecRadioButtons[idxRadButton] )
			{
				//pOpt->m_vecRadioButtons[idxRadButton]->SetEnabled( idxRadButton == 0 || !bForceVanilla );
				pOpt->m_vecRadioButtons[idxRadButton]->SilentSetSelected( pOpt->m_nChoice == idxRadButton );
			}
		}
		
		int nChoice = ( tf_quickplay_pref_beta_content.GetBool() ? 0 : pOpt->m_nChoice );
		AppendOptionInfo( pwszSummary, pOpt->m_vecOptionSummaryNames[nChoice] );
	}
}

void CQuickplayPanelBase::OnTextChanged( vgui::Panel *panel )
{
	if ( panel == m_pGameModeCombo )
	{
		UserSelectItemByIndex( m_pGameModeCombo->GetActiveItem() );
		return;
	}

}

void CQuickplayPanelBase::OnRadioButtonChecked( vgui::Panel *panel )
{
	FOR_EACH_VEC( m_vecAdvOptions, idxAdvOpt )
	{
		AdvOption *pOpt = &m_vecAdvOptions[ idxAdvOpt ];
		int idxBtn = pOpt->m_vecRadioButtons.Find( (vgui::RadioButton*)panel );
		if ( idxBtn >= 0 )
		{
			pOpt->m_nChoice = idxBtn;

			// Check if they change the server hosting to valve servers,
			// then slam their choices to vanilla
			if ( idxAdvOpt == kEAdvOption_ServerHost && idxBtn == QuickplaySearchOptions::eServersOfficial )
			{
				FOR_EACH_VEC( m_vecAdvOptions, idxSlam )
				{
					if ( idxSlam != kEAdvOption_ServerHost )
						m_vecAdvOptions[idxSlam].m_nChoice = 0;
				}
			}
		}
	}
	WriteOptionCombosAndSummary();
}

void CQuickplayPanelBase::SetPageScrollButtonsVisible( bool bFlag )
{
	if ( m_pPrevPageButton )
		m_pPrevPageButton->SetVisible( bFlag );
	if ( m_pNextPageButton )
		m_pNextPageButton->SetVisible( bFlag );
}

void CQuickplayPanelBase::SaveSettings()
{
	tf_quickplay_lastviewedmode.SetValue( m_iCurrentItem );
	FOR_EACH_VEC( m_vecAdvOptions, idxAdvOpt )
	{
		AdvOption *pOpt = &m_vecAdvOptions[ idxAdvOpt ];
		pOpt->m_pConvar->SetValue( pOpt->m_nChoice );
	}
}

void CQuickplayPanelBase::OnCommand( const char *pCommand )
{
	if ( FStrEq( pCommand, "prevpage" ) )
	{
		UserSelectItemByIndex( ( m_iCurrentItem > 0 ) ? ( m_iCurrentItem - 1 ) : ( m_vecItems.Count() - 1 ) );
		if ( m_pPrevPageButton )
			m_pPrevPageButton->RequestFocus();
	}
	else if ( FStrEq( pCommand, "nextpage" ) )
	{
		UserSelectItemByIndex( ( m_iCurrentItem+1 < m_vecItems.Count() ) ? ( m_iCurrentItem+1 ) : 0 );
		if ( m_pNextPageButton )
			m_pNextPageButton->RequestFocus();
	}
	else if ( FStrEq( pCommand, "more_info" ) )
	{
		if ( m_pMoreInfoContainer && m_pGameModeInfoContainer )
		{
			m_pMoreInfoContainer->SetVisible( !m_pMoreInfoContainer->IsVisible() );
			vgui::ImagePanel *pImage = dynamic_cast< vgui::ImagePanel* >( m_pGameModeInfoContainer->FindChildByName( "ModeImage" ) );
			if ( pImage )
			{
				pImage->SetVisible( !m_pMoreInfoContainer->IsVisible() );
			}
		}
	}
	else if ( FStrEq( pCommand, "ToggleShowOptions" ) )
	{
		tf_quickplay_pref_advanced_view.SetValue( !tf_quickplay_pref_advanced_view.GetBool() );
		ShowSimplifiedOrAdvancedOptions();
	}
	else 
	{
		BaseClass::OnCommand( pCommand );
	}
}

void CQuickplayPanelBase::UserSelectItemByIndex( int iNewItem )
{
	ShowItemByIndex( iNewItem );
}



class CQuickplayDialog : public CQuickplayPanelBase
{
	DECLARE_CLASS_SIMPLE( CQuickplayDialog, CQuickplayPanelBase );
public:
	CQuickplayDialog( vgui::Panel *parent ) 
		: CQuickplayPanelBase( parent, "QuickPlayDialog" )
	{
		m_pContainer = new vgui::EditablePanel( this, "Container" );
		m_pBetaCheckButton = new vgui::CheckButton( m_pContainer, "BetaCheckButton", "BetaToggle" );
		m_pBetaCheckButton->AddActionSignalTarget( this );
		m_pBetaCheckButton->SetSelected( tf_quickplay_pref_beta_content.GetInt() == 1 );
		m_pTauntsExplanationPopup = new CExplanationPopup( m_pContainer, "BetaExplanation" );

		C_CTFGameStats::ImmediateWriteInterfaceEvent( "interface_open", "quickplay" );

		LoadControlSettings( "Resource/ui/QuickplayDialog.res" );
	}

	virtual ~CQuickplayDialog()
	{

		C_CTFGameStats::ImmediateWriteInterfaceEvent( "interface_close", "quickplay" );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings( pScheme );

		Panel *pPanel;
		pPanel = FindChildByName( "PlayNowButton", true ); Assert( pPanel );
		if ( pPanel ) pPanel->AddActionSignalTarget( this );
		pPanel = FindChildByName( "ShowServersButton", true ); Assert( pPanel );
		if ( pPanel ) pPanel->AddActionSignalTarget( this );
		pPanel = FindChildByName( "ExplainBetaButton", true ); Assert( pPanel );
		if ( pPanel ) pPanel->AddActionSignalTarget( this );
	}

	virtual void PerformLayout() OVERRIDE
	{
		m_pBetaCheckButton->SizeToContents();

		BaseClass::PerformLayout();

		// Center it, keeping requested size
		int x, y, ww, wt, wide, tall;
		vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
		GetSize(wide, tall);
		SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
		
		m_pMoreOptionsButton->SetVisible( !m_pBetaCheckButton->IsSelected() );
		tf_quickplay_pref_beta_content.SetValue( m_pBetaCheckButton->IsSelected() ? 1 : 0 );
		// @todo setup 
	}

	virtual void Show()
	{
		TFModalStack()->PushModal( this );

		// Make sure we're signed on
		if ( !CheckSteamSignOn() )
		{
			Close();
			return;
		}

		SetVisible( true );
		MakePopup();
		MoveToFront();
		SetKeyBoardInputEnabled( true );
		SetMouseInputEnabled( true );

		vgui::Panel *pPlayNowButton = FindChildByName( "PlayNowButton" );
		if ( pPlayNowButton )
		{
			pPlayNowButton->RequestFocus();
		}
	}

	bool CheckSteamSignOn()
	{
		// Make sure we are connected to steam, or they are going to be disappointed
		if ( steamapicontext == NULL
			|| steamapicontext->SteamUtils() == NULL
			|| steamapicontext->SteamMatchmakingServers() == NULL
			|| steamapicontext->SteamUser() == NULL
			|| !steamapicontext->SteamUser()->BLoggedOn()
		) {
			Warning( "Steam not properly initialized or connected.\n" );
			ShowMessageBox( "#TF_MM_GenericFailure_Title", "#TF_MM_GenericFailure", "#GameUI_OK" );
			return false;
		}
		return true;
	}

	virtual void OnCommand( const char *pCommand )
	{
		C_CTFGameStats::ImmediateWriteInterfaceEvent( "on_command(quickplay)", pCommand );

		if ( FStrEq( pCommand, "playnow" ) || FStrEq( pCommand, "show_servers" ) )
		{
			SaveSettings();
			Close();

			if ( !CheckSteamSignOn() )
			{
				return;
			}

			bool bBetaContent = tf_quickplay_pref_beta_content.GetBool();

			QuickplaySearchOptions opt;
			opt.m_eSelectedGameType = m_vecItems[m_iCurrentItem].gameType;
			opt.m_eServers = (QuickplaySearchOptions::EServers)( bBetaContent ? 
				0 :
				tf_quickplay_pref_community_servers.GetInt() );
			opt.m_eRandomCrits = (QuickplaySearchOptions::ERandomCrits)( bBetaContent ? 2 : tf_quickplay_pref_disable_random_crits.GetInt() );
			opt.m_eDamageSpread = (QuickplaySearchOptions::EDamageSpread)( bBetaContent ? 2 : tf_quickplay_pref_enable_damage_spread.GetInt() );
			opt.m_eRespawnTimes = (QuickplaySearchOptions::ERespawnTimes)( bBetaContent ? 2 : tf_quickplay_pref_respawn_times.GetInt() );
			opt.m_eMaxPlayers = (QuickplaySearchOptions::EMaxPlayers)( bBetaContent ? 2 : tf_quickplay_pref_increased_maxplayers.GetInt() );
			opt.m_eBetaContent = (QuickplaySearchOptions::EBetaContent)tf_quickplay_pref_beta_content.GetInt();
			ShowWaitingDialog( new CQuickplayWaitDialog( FStrEq( pCommand, "playnow" ), opt ), NULL, true, true, 0.0f );
		}
		else if ( FStrEq( pCommand, "cancel" ) )
		{
			Close();
			SaveSettings();
		}
		else if ( FStrEq( pCommand, "beta_toggle" ) )
		{
			UpdateSelectableItems();

			if ( m_pMoreOptionsButton )
			{
				// Disable the advanced filtering if beta box is checked
				m_pMoreOptionsButton->SetVisible( !m_pBetaCheckButton->IsSelected() );
				tf_quickplay_pref_beta_content.SetValue( m_pBetaCheckButton->IsSelected() ? 1 : 0 );

				WriteOptionCombosAndSummary();

				tf_quickplay_pref_advanced_view.SetValue( 0 );
				ShowSimplifiedOrAdvancedOptions();
			}
		}
		else if ( FStrEq( pCommand, "explain_beta" ) )
		{
			m_pTauntsExplanationPopup->Popup();
		}
		else 
		{
			BaseClass::OnCommand( pCommand );
		}
	}

	virtual void OnKeyCodePressed( vgui::KeyCode code )
	{
		vgui::KeyCode nButtonCode = GetBaseButtonCode( code );
		if( nButtonCode == KEY_XBUTTON_B || nButtonCode == STEAMCONTROLLER_B || nButtonCode == STEAMCONTROLLER_START )
		{
			OnCommand( "cancel" );
		}
		else if ( nButtonCode == KEY_XBUTTON_A || nButtonCode == STEAMCONTROLLER_A )
		{
			OnCommand( "playnow" );
		}
		else if ( nButtonCode == KEY_XBUTTON_X || nButtonCode == STEAMCONTROLLER_X )
		{
			OnCommand( "more_info" );
		}
		else if ( nButtonCode == KEY_XBUTTON_LEFT || 
				  nButtonCode == KEY_XSTICK1_LEFT ||
				  nButtonCode == KEY_XSTICK2_LEFT ||
				  nButtonCode == STEAMCONTROLLER_DPAD_LEFT ||
				  code == KEY_LEFT )
		{
			OnCommand( "prevpage" );
		}
		else if ( nButtonCode == KEY_XBUTTON_RIGHT || 
				  nButtonCode == KEY_XSTICK1_RIGHT ||
				  nButtonCode == KEY_XSTICK2_RIGHT ||
				  nButtonCode == STEAMCONTROLLER_DPAD_RIGHT ||
				  code == KEY_RIGHT )
		{
			OnCommand( "nextpage" );
		}
		else
		{
			BaseClass::OnKeyCodeTyped( code );
		}
	}

	virtual void OnKeyCodeTyped( vgui::KeyCode code )
	{
		if( code == KEY_ESCAPE )
		{
			OnCommand( "cancel" );
		}
		else if ( code == KEY_ENTER || code == KEY_SPACE )
		{
			OnCommand( "playnow" );
		}
		else
		{
			BaseClass::OnKeyCodeTyped( code );
		}
	}

protected:

	virtual const char *GetItemImage( const QuickplayItem& item ) const OVERRIDE
	{
		if ( m_pBetaCheckButton->IsSelected() && item.pBetaImage )
		{
			return item.pBetaImage;
		}
		
		return item.pImage;
	}

	virtual void GetOptionsAndSummaryText( wchar_t *pwszSummary ) OVERRIDE
	{
		BaseClass::GetOptionsAndSummaryText( pwszSummary );

		if ( m_pBetaCheckButton->IsSelected() )
		{
			AppendOptionInfo( pwszSummary, "#TF_Quickplay_Beta" );
		}
	}

	// called when the Cancel button is pressed
	void Close()
	{
		SetVisible( false );
		TFModalStack()->PopModal( this );
		MarkForDeletion();
	}

	void UpdateSelectableItems() OVERRIDE
	{
		m_vecItems.Purge();

		bool bBetaActive = m_pBetaCheckButton->IsSelected();

		// Go through each of the modes
		FOR_EACH_VEC( m_vecAllItems, i )
		{
			int nNumWithBetaContent = 0;
			int nNumForThisMode = 0;

			// Go through each of the modes
			for ( int j = 0 ; j < GetItemSchema()->GetMapCount(); ++j )
			{
				const SchemaMap_t& map = GetItemSchema()->GetMapForIndex( j );

				// Tally up maps for this mode
				if ( map.eGameCategory == m_vecAllItems[i].gameType )
				{
					nNumForThisMode++;

					// Check if any of the tags has "beta" as a tag, and tally that if so
					for( int k = 0; k < map.vecTags.Count(); ++k )
					{
						if ( map.vecTags.HasElement( GetItemSchema()->GetHandleForTag( "beta" ) ) )
						{
							nNumWithBetaContent++;
						}
					}
				}
			}

			// Only add the visible items if we're filtering for beta and this category has at least 1 beta map
			// OR if we're not filtering for beta and we have at least 1 map that's not beta
			const bool bBetaFilteringAndHasBetaContent = ( bBetaActive && nNumWithBetaContent > 0 );
			const bool bNotBetaFilteringHasNonBetaContent = ( !bBetaActive && nNumForThisMode > nNumWithBetaContent );
			const bool bIsRandom = m_vecAllItems[i].gameType == kGameCategory_Quickplay;
			if ( ( bBetaFilteringAndHasBetaContent ) || ( bNotBetaFilteringHasNonBetaContent ) || bIsRandom )
			{
				m_vecItems.AddToTail( m_vecAllItems[i] );
			}
		}

		// Go back to the first page
		ShowItemByIndex( 0 );
	}

private:

	vgui::CheckButton	*m_pBetaCheckButton;
	CExplanationPopup	*m_pTauntsExplanationPopup;
};
static vgui::DHANDLE<CQuickplayDialog> g_pQuickplayDialog;

//-----------------------------------------------------------------------------

class CGCTFQuickplay_ScoreServers_Response : public GCSDK::CGCClientJob
{
public:
	CGCTFQuickplay_ScoreServers_Response( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgTFQuickplay_ScoreServersResponse> msg( pNetPacket );

		if ( s_pQuickPlayWaitingDialog )
		{
			// !TEST! Forced failure
			//Warning( "CMsgTFQuickplay_ScoreServersResponse received, but discarded to simulate failure!!\n" );
			//return true;

			s_pQuickPlayWaitingDialog->OnReceivedGCScores( msg.Body() );
		}
		else
		{
			Warning(" Received CGCTFQuickplay_ScoreServers_Response, but no quick play query in progress dialog to receive them?\n" );
		}

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCTFQuickplay_ScoreServers_Response, "CGCTFQuickplay_ScoreServers_Response", k_EMsgGC_QP_ScoreServersResponse, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------
// Purpose: Callback to open the game menus
//-----------------------------------------------------------------------------

#ifdef ENABLE_GC_MATCHMAKING
ConVar tf_quickplay_beta_preference( "tf_quickplay_beta_preference", "-1", FCVAR_NONE, "Preference to participate in beta quickplay: -1 = no preference, 0 = opt out, 1 = opt in" );
ConVar tf_quickplay_beta_ask_percentage( "tf_quickplay_beta_ask_percentage", "0", FCVAR_NONE, "Percentage of people who will be prompted to participate in beta quickplay." );


void QuickplayBetaConfirmCallback( bool bConfirmed, void *pContext )
{
	tf_quickplay_beta_preference.SetValue( bConfirmed ? 1 : 0 );
	if ( bConfirmed )
	{
		engine->ClientCmd_Unrestricted( "OpenMatchmakingLobby quickplay" );
	}
	else
	{
		engine->ClientCmd_Unrestricted( "OpenQuickplayDialog nobeta" );
	}
}

#endif // #ifdef ENABLE_GC_MATCHMAKING

static void CL_OpenQuickplayDialog( const CCommand &args )
{

	// Check for opting into beta quickplay
	#ifdef ENABLE_GC_MATCHMAKING
		if ( args.ArgC() < 2 && tf_quickplay_beta_preference.GetInt() != 0 )
		{
			if ( tf_quickplay_beta_preference.GetInt() > 0 )
			{
				engine->ClientCmd_Unrestricted( "OpenMatchmakingLobby quickplay" );
				return;
			}
			if ( steamapicontext && steamapicontext->SteamUser() && ( steamapicontext->SteamUser()->GetSteamID().GetAccountID() % 100U ) < (uint32)tf_quickplay_beta_ask_percentage.GetInt() )
			{
				ShowConfirmDialog( "#TF_QuickplayBeta_OptIn_Title", "#TF_QuickplayBeta_OptIn_Message", "#TF_QuickplayBeta_OptIn_YesButton", "#TF_QuickplayBeta_OptIn_NoButton", QuickplayBetaConfirmCallback );
				return;
			}
		}
	#endif

	if ( g_pQuickplayDialog.Get() == NULL )
	{
		IViewPortPanel *pMMOverride = ( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
		g_pQuickplayDialog = vgui::SETUP_PANEL( new CQuickplayDialog( (CHudMainMenuOverride*)pMMOverride ) );
	}
	g_pQuickplayDialog->Show();
}

// the console commands
static ConCommand quickplaydialog( "OpenQuickplayDialog", &CL_OpenQuickplayDialog, "Displays the quickplay dialog." );


static void CL_OpenQuickplayDialogForMap( const CCommand &args )
{
	if ( args.ArgC() != 2 )
		return;

	QuickplaySearchOptions opt;
	opt.m_eServers = QuickplaySearchOptions::eServersOfficial; // Quests only work on official.
	// Default settings
	opt.m_eRandomCrits = QuickplaySearchOptions::ERandomCrits::eRandomCritsYes;
	opt.m_eDamageSpread = QuickplaySearchOptions::EDamageSpread::eDamageSpreadNo;
	opt.m_eRespawnTimes = QuickplaySearchOptions::ERespawnTimes::eRespawnTimesDefault;
	opt.m_eMaxPlayers = QuickplaySearchOptions::EMaxPlayers::eMaxPlayers24;
	opt.m_eBetaContent = QuickplaySearchOptions::EBetaContent::eBetaNo;
	opt.m_strMapName = args.Arg(1); // Use the map name passed in
	opt.m_eSelectedGameType =  kGameCategory_Quickplay;
			
	ShowWaitingDialog( new CStandaloneQuickplayMenu( false, opt ), NULL, true, true, 0.0f );
}

static ConCommand OpenQuickplayWaitDialogForMap( "OpenQuickplayWaitDialogForMap", &CL_OpenQuickplayDialogForMap, "Instantly starts a quickplay query for a map." );
