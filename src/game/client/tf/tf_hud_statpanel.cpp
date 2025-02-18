
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#ifdef WIN32
#include "winerror.h"
#endif
#include "tf_hud_statpanel.h"
#include "tf_hud_winpanel.h"
#include <vgui/IVGui.h>
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "c_tf_playerresource.h"
#include <vgui_controls/Label.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "tf/c_tf_player.h"
#include "tf/c_tf_team.h"
#include "tf/tf_steamstats.h"
#include "filesystem.h"
#include "dmxloader/dmxloader.h"
#include "fmtstr.h"
#include "tf_statsummary.h"
#include "usermessages.h"
#include "hud_macros.h"
#include "ixboxsystem.h"
#include "achievementmgr.h"
#include "tf_hud_freezepanel.h"
#include "tf_gamerules.h"
#include "tf_mapinfo.h"
#include <vgui_controls/MessageBox.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT_DEPTH( CTFStatPanel, 1 );
DECLARE_HUD_MESSAGE( CTFStatPanel, PlayerStatsUpdate );
DECLARE_HUD_MESSAGE( CTFStatPanel, MapStatsUpdate );

BEGIN_DMXELEMENT_UNPACK( RoundStats_t )
	DMXELEMENT_UNPACK_FIELD( "iNumShotsHit", "0", int, m_iStat[TFSTAT_SHOTS_HIT] )
	DMXELEMENT_UNPACK_FIELD( "iNumShotsFired", "0", int, m_iStat[TFSTAT_SHOTS_FIRED] )
	DMXELEMENT_UNPACK_FIELD( "iNumberOfKills", "0", int, m_iStat[TFSTAT_KILLS] )
	DMXELEMENT_UNPACK_FIELD( "iNumDeaths", "0", int, m_iStat[TFSTAT_DEATHS] )
	DMXELEMENT_UNPACK_FIELD( "iDamageDealt", "0", int, m_iStat[TFSTAT_DAMAGE] )
	DMXELEMENT_UNPACK_FIELD( "iPlayTime", "0", int, m_iStat[TFSTAT_PLAYTIME] )
	DMXELEMENT_UNPACK_FIELD( "iPointCaptures", "0", int, m_iStat[TFSTAT_CAPTURES] )
	DMXELEMENT_UNPACK_FIELD( "iPointDefenses", "0", int, m_iStat[TFSTAT_DEFENSES] )
	DMXELEMENT_UNPACK_FIELD( "iDominations", "0", int, m_iStat[TFSTAT_DOMINATIONS] )
	DMXELEMENT_UNPACK_FIELD( "iRevenge", "0", int, m_iStat[TFSTAT_REVENGE] )
	DMXELEMENT_UNPACK_FIELD( "iPointsScored", "0", int, m_iStat[TFSTAT_POINTSSCORED] )
	DMXELEMENT_UNPACK_FIELD( "iBuildingsDestroyed", "0", int, m_iStat[TFSTAT_BUILDINGSDESTROYED] )
	DMXELEMENT_UNPACK_FIELD( "iHeadshots", "0", int, m_iStat[TFSTAT_HEADSHOTS] )
	DMXELEMENT_UNPACK_FIELD( "iHealthPointsHealed", "0", int, m_iStat[TFSTAT_HEALING] )
	DMXELEMENT_UNPACK_FIELD( "iNumInvulnerable", "0", int, m_iStat[TFSTAT_INVULNS] )
	DMXELEMENT_UNPACK_FIELD( "iKillAssists", "0", int, m_iStat[TFSTAT_KILLASSISTS] )
	DMXELEMENT_UNPACK_FIELD( "iBackstabs", "0", int, m_iStat[TFSTAT_BACKSTABS] )
	DMXELEMENT_UNPACK_FIELD( "iHealthPointsLeached", "0", int, m_iStat[TFSTAT_HEALTHLEACHED] )
	DMXELEMENT_UNPACK_FIELD( "iBuildingsBuilt", "0", int, m_iStat[TFSTAT_BUILDINGSBUILT] )
	DMXELEMENT_UNPACK_FIELD( "iSentryKills", "0", int, m_iStat[TFSTAT_MAXSENTRYKILLS] )
	DMXELEMENT_UNPACK_FIELD( "iNumTeleports", "0", int, m_iStat[TFSTAT_TELEPORTS] )
	DMXELEMENT_UNPACK_FIELD( "iFireDamage", "0", int, m_iStat[TFSTAT_FIREDAMAGE] )
	DMXELEMENT_UNPACK_FIELD( "iBonusPoints", "0", int, m_iStat[TFSTAT_BONUS_POINTS] )
	DMXELEMENT_UNPACK_FIELD( "iBlastDamage", "0", int, m_iStat[TFSTAT_BLASTDAMAGE] )
END_DMXELEMENT_UNPACK( RoundStats_t, s_RoundStatsUnpack )

BEGIN_DMXELEMENT_UNPACK( RoundMapStats_t )
	DMXELEMENT_UNPACK_FIELD( "iPlayTime", "0", int, m_iStat[TFMAPSTAT_PLAYTIME] )
END_DMXELEMENT_UNPACK( RoundMapStats_t, s_RoundMapStatsUnpack )

BEGIN_DMXELEMENT_UNPACK( ClassStats_t )
	DMXELEMENT_UNPACK_FIELD( "iPlayerClass", "0", int, iPlayerClass )
	DMXELEMENT_UNPACK_FIELD( "iNumberOfRounds", "0", int, iNumberOfRounds )
	// RoundStats_t		accumulated;
	// RoundStats_t		max;
	// RoundStats_t		currentRound;
	// RoundStats_t		accumulatedMVM;
	// RoundStats_t		maxMVM;
END_DMXELEMENT_UNPACK( ClassStats_t, s_ClassStatsUnpack )

BEGIN_DMXELEMENT_UNPACK( MapStats_t )
	DMXELEMENT_UNPACK_FIELD( "iMapID", "0", map_identifier_t, iMapID )
	DMXELEMENT_UNPACK_FIELD( "iNumberOfRounds", "0", int, iNumberOfRounds )
	// RoundStats_t		accumulated;
	// RoundStats_t		currentRound;
END_DMXELEMENT_UNPACK( MapStats_t, s_MapStatsUnpack )

// priority order of stats to display record for; earlier position in list is highest
TFStatType_t g_statPriority[] = { TFSTAT_HEADSHOTS, TFSTAT_BACKSTABS, TFSTAT_MAXSENTRYKILLS, TFSTAT_HEALING, TFSTAT_KILLS, TFSTAT_KILLASSISTS,  
	TFSTAT_DAMAGE, TFSTAT_DOMINATIONS, TFSTAT_INVULNS, TFSTAT_BUILDINGSDESTROYED, TFSTAT_CAPTURES, TFSTAT_DEFENSES, TFSTAT_REVENGE, TFSTAT_TELEPORTS, TFSTAT_BUILDINGSBUILT, 
	TFSTAT_HEALTHLEACHED, TFSTAT_POINTSSCORED, TFSTAT_PLAYTIME, TFSTAT_BONUS_POINTS };
// stat types that we don't display records for, kept in this list just so we can assert all stats appear in one list or the other
TFStatType_t g_statUnused[] = { TFSTAT_DEATHS, TFSTAT_UNDEFINED, TFSTAT_SHOTS_FIRED, TFSTAT_SHOTS_HIT, TFSTAT_FIREDAMAGE, TFSTAT_BLASTDAMAGE,
	TFSTAT_DAMAGETAKEN, TFSTAT_HEALTHKITS, TFSTAT_AMMOKITS, TFSTAT_CLASSCHANGES, TFSTAT_CRITS, TFSTAT_SUICIDES, TFSTAT_CURRENCY_COLLECTED, TFSTAT_DAMAGE_ASSIST, TFSTAT_HEALING_ASSIST, 
	TFSTAT_DAMAGE_BOSS, TFSTAT_DAMAGE_BLOCKED, TFSTAT_DAMAGE_RANGED, TFSTAT_DAMAGE_RANGED_CRIT_RANDOM, TFSTAT_DAMAGE_RANGED_CRIT_BOOSTED, TFSTAT_REVIVED, TFSTAT_THROWABLEHIT, TFSTAT_THROWABLEKILL, TFSTAT_KILLS_RUNECARRIER, TFSTAT_FLAGRETURNS,
	TFSTAT_KILLSTREAK_MAX };

// priority order of stats to display record for; earlier position in list is highest
TFMapStatType_t g_mapStatPriority[] = { TFMAPSTAT_PLAYTIME };
// stat types that we don't display records for, kept in this list just so we can assert all stats appear in one list or the other
TFMapStatType_t g_mapStatUnused[] = { TFMAPSTAT_UNDEFINED };

// localization keys for stat panel text, must be in same order as TFStatType_t
const char *g_szLocalizedRecordText[] =
{
	"",
	"[shots hit]",
	"[shots fired]",
	"#StatPanel_Kills",	
	"[deaths]",
	"#StatPanel_DamageDealt",
	"#StatPanel_Captures",
	"#StatPanel_Defenses",
	"#StatPanel_Dominations",
	"#StatPanel_Revenge",
	"#StatPanel_PointsScored",
	"#StatPanel_BuildingsDestroyed",
	"#StatPanel_Headshots",
	"#StatPanel_PlayTime",
	"#StatPanel_Healing",
	"#StatPanel_Invulnerable",
	"#StatPanel_KillAssists",
	"#StatPanel_Backstabs",
	"#StatPanel_HealthLeached",
	"#StatPanel_BuildingsBuilt",
	"#StatPanel_SentryKills",		
	"#StatPanel_Teleports",
	"#StatPanel_BonusPoints"
};

const char *g_szLocalizedMVMRecordText[] =
{
	"",
	"[shots hit]",
	"[shots fired]",
	"#StatPanel_MVM_Kills",	
	"[deaths]",
	"#StatPanel_MVM_DamageDealt",
	"#StatPanel_MVM_Captures",
	"#StatPanel_MVM_Defenses",
	"#StatPanel_MVM_Dominations",
	"#StatPanel_MVM_Revenge",
	"#StatPanel_MVM_PointsScored",
	"#StatPanel_MVM_BuildingsDestroyed",
	"#StatPanel_MVM_Headshots",
	"#StatPanel_MVM_PlayTime",
	"#StatPanel_MVM_Healing",
	"#StatPanel_MVM_Invulnerable",
	"#StatPanel_MVM_KillAssists",
	"#StatPanel_MVM_Backstabs",
	"#StatPanel_MVM_HealthLeached",
	"#StatPanel_MVM_BuildingsBuilt",
	"#StatPanel_MVM_SentryKills",		
	"#StatPanel_MVM_Teleports",
	"#StatPanel_MVM_BonusPoints"
};


static CTFStatPanel *statPanel = NULL;
extern CAchievementMgr g_AchievementMgrTF;

//-----------------------------------------------------------------------------
// Purpose: Returns the static stats panel
//-----------------------------------------------------------------------------
CTFStatPanel *GetStatPanel()
{
	return statPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFStatPanel::CTFStatPanel( const char *pElementName )
: EditablePanel( NULL, "StatPanel" ), CHudElement( pElementName )
{
	// Assert that all defined stats are in our prioritized list or explicitly unused
	Assert( ARRAYSIZE( g_statPriority ) + ARRAYSIZE( g_statUnused ) == TFSTAT_TOTAL );
	Assert( ARRAYSIZE( g_mapStatPriority ) + ARRAYSIZE( g_mapStatUnused ) == TFMAPSTAT_TOTAL );

	ResetDisplayedStat();
	m_bStatsChanged = false;
	m_bLocalFileTrusted = false;
	m_flTimeLastSpawn = 0;
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	m_bShouldBeVisible = false;
	SetScheme( "ClientScheme" );
	statPanel = this;

	m_pClassImage = new CTFClassImage( this, "StatPanelClassImage" );

	// Read stats from disk.  (Definitive stat store for X360; for PC, whatever we get from Steam is authoritative.)
	ReadStats();

	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFStatPanel::~CTFStatPanel()
{
	if ( statPanel == this )
		statPanel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: called when level is shutting down
//-----------------------------------------------------------------------------
void CTFStatPanel::LevelShutdown()
{
	// write out stats if they've changed
	WriteStats();
	UpdateStatSummaryPanel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::Reset()
{
	if ( gpGlobals->curtime > m_flTimeHide )
	{
		Hide();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resets which stat is being displayed
//-----------------------------------------------------------------------------
void CTFStatPanel::ResetDisplayedStat()
{
	m_iCurStatValue = 0;
	m_iCurStatTeam = TEAM_UNASSIGNED;
	m_statType = TFSTAT_UNDEFINED;
	m_recordBreakType = RECORDBREAK_NONE;
	m_iCurStatClass = TF_CLASS_UNDEFINED;
	m_bDisplayAfterSpawn = false;
	m_flTimeHide = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::Init()
{
	// listen for events
	HOOK_HUD_MESSAGE( CTFStatPanel, PlayerStatsUpdate );
	HOOK_HUD_MESSAGE( CTFStatPanel, MapStatsUpdate );
	ListenForGameEvent( "player_spawn" );

	Hide();

	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::UpdateStats( int iClass, const RoundStats_t &stats, bool bAlive )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	// don't count stats if cheats on, commentary mode, etc
	if ( !g_AchievementMgrTF.CheckAchievementsEnabled() )
		return;

	ClassStats_t &classStats = GetClassStats( iClass );

	bool bMVM = TFGameRules() && TFGameRules()->IsMannVsMachineMode();

	if ( bMVM )
	{
		classStats.AccumulateMVMRound( stats );
	}
	else
	{
		classStats.AccumulateRound( stats );
	}

	// sentry kills is a max value rather than a count, meaningless to accumulate
	classStats.accumulated.m_iStat[TFSTAT_MAXSENTRYKILLS] = 0;	
	classStats.accumulatedMVM.m_iStat[TFSTAT_MAXSENTRYKILLS] = 0;

	ResetDisplayedStat();
	m_iCurStatClass = iClass;

	// run through all stats we keep records for, update the max value, and if a record is set,
	// remember the highest priority record
	for ( int i= ARRAYSIZE( g_statPriority )-1; i >= 0; i-- )
	{
		TFStatType_t statType = g_statPriority[i];
		if ( statType == TFSTAT_BONUS_POINTS )
			continue;

		int iCur = stats.m_iStat[statType];
		int iMax = ( bMVM ? classStats.maxMVM.m_iStat[statType] : classStats.max.m_iStat[statType] );
		if ( iCur > iMax )
		{
			// Record was set, remember what stat set a record.
			if ( bMVM )
			{
				classStats.maxMVM.m_iStat[statType] = iCur;
			}
			else
			{
				classStats.max.m_iStat[statType] = iCur;
			}

			m_iCurStatValue = iCur;
			m_statType = statType;
			m_recordBreakType = RECORDBREAK_BEST;
		}
		else if ( ( iCur > 0 ) && ( m_recordBreakType <= RECORDBREAK_TIE ) && ( iCur == iMax ) )
		{
			// if we haven't broken a record and we tied this one, display it
			m_iCurStatValue = iCur;
			m_statType = statType;
			m_recordBreakType = RECORDBREAK_TIE;
		}
		else if ( ( iCur > 0 ) && ( m_recordBreakType <= RECORDBREAK_CLOSE ) && ( iCur >= (int) ( (float) iMax * 0.8f ) ) )
		{
			// if we haven't broken a record or tied a record but we came close to this one, display it
			m_iCurStatValue = iCur;
			m_statType = statType;
			m_recordBreakType = RECORDBREAK_CLOSE;
		}
	}

	m_bStatsChanged = true;

	if ( m_statType > TFSTAT_UNDEFINED )
	{
		m_iCurStatTeam = pPlayer->GetTeamNumber();
		if ( !bAlive || ( gpGlobals->curtime - m_flTimeLastSpawn < 3.0 ) )
		{
			// show the panel now if dead or very recently spawned
			vgui::ivgui()->AddTickSignal( GetVPanel(), 1000 );
			extern ConVar hud_freezecamhide;
			if ( !hud_freezecamhide.GetBool() && !IsTakingAFreezecamScreenshot() )
			{
				ShowStatPanel( m_iCurStatClass, m_iCurStatTeam, m_iCurStatValue, m_statType, m_recordBreakType, bAlive );
				m_flTimeHide = gpGlobals->curtime + 20.0f;
			}
		}
		else
		{
			// otherwise wait until next spawn to show panel
			m_bDisplayAfterSpawn = true;
		}
	}

	IGameEvent * event = gameeventmanager->CreateEvent( "player_stats_updated" );
	if ( event )
	{
		event->SetBool( "forceupload", false );
		gameeventmanager->FireEventClientSide( event );
	}

	UpdateStatSummaryPanel();
}

void CTFStatPanel::UpdateMapStats( map_identifier_t iMapID, const RoundMapStats_t &stats )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	// It's ok to count map stats while cheating and such
	//if ( !g_AchievementMgrTF.CheckAchievementsEnabled() )
	//	return;

	MapStats_t &mapStats = GetMapStats( iMapID );

	mapStats.AccumulateRound( stats );

	ResetDisplayedStat();
	m_iCurStatClass = iMapID;

	m_bStatsChanged = true;

	IGameEvent * event = gameeventmanager->CreateEvent( "player_stats_updated" );
	if ( event )
	{
		event->SetBool( "forceupload", false );
		gameeventmanager->FireEventClientSide( event );
	}

	UpdateStatSummaryPanel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::TestStatPanel( TFStatType_t statType, RecordBreakType_t recordType )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
	{
		Msg( "Please load a map first.\n" );
		return;
	}

	bool bMVM = TFGameRules() && TFGameRules()->IsMannVsMachineMode();

	m_iCurStatClass = pPlayer->GetPlayerClass()->GetClassIndex();
	ClassStats_t &classStats = GetClassStats( m_iCurStatClass );
	m_iCurStatValue = bMVM ? classStats.maxMVM.m_iStat[statType] : classStats.max.m_iStat[statType];
	m_iCurStatTeam = pPlayer->GetTeamNumber();

	ShowStatPanel( m_iCurStatClass, m_iCurStatTeam, m_iCurStatValue, statType, recordType, false );
}

//-----------------------------------------------------------------------------
// Purpose: Writes stat file.  Used as primary storage for X360.  For PC,
//			Steam is authoritative but we write stat file for debugging (although
//			we never read it).
//-----------------------------------------------------------------------------
void CTFStatPanel::WriteStats( void )
{
	if ( !m_bStatsChanged )
		return;

	MEM_ALLOC_CREDIT();

	DECLARE_DMX_CONTEXT();
	CDmxElement *pPlayerStats = CreateDmxElement( "PlayerStats" );
	CDmxElementModifyScope modify( pPlayerStats );

	// get Steam ID.  If not logged into Steam, use 0
	int iSteamID = 0;
	if ( steamapicontext->SteamUser() )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		iSteamID = steamID.GetAccountID();
	}	
	// Calc CRC of all data to make the local data file somewhat tamper-resistant
	int iCRC = CalcCRC( iSteamID );

	pPlayerStats->SetValue( "iVersion", static_cast<int>( PLAYERSTATS_FILE_VERSION ) );
	pPlayerStats->SetValue( "SteamID", iSteamID );	
	pPlayerStats->SetValue( "iTimestamp", iCRC );	// store the CRC with a non-obvious name

	CDmxAttribute *pClassStatsList = pPlayerStats->AddAttribute( "aClassStats" );
	CUtlVector< CDmxElement* >& classStats = pClassStatsList->GetArrayForEdit<CDmxElement*>();

	modify.Release();

	for( int i = 0; i < m_aClassStats.Count(); i++ )
	{
		const ClassStats_t &stat = m_aClassStats[ i ];

		// strip out any garbage class data
		if ( ( stat.iPlayerClass > (TF_LAST_NORMAL_CLASS-1) ) || ( stat.iPlayerClass < TF_FIRST_NORMAL_CLASS ) )
			continue;

		CDmxElement *pClass = CreateDmxElement( "ClassStats_t" );
		classStats.AddToTail( pClass );

		CDmxElementModifyScope modifyClass( pClass );

		pClass->SetValue( "comment: classname", g_aPlayerClassNames_NonLocalized[ stat.iPlayerClass ] );
		pClass->AddAttributesFromStructure( &stat, s_ClassStatsUnpack );

		CDmxElement *pAccumulated = CreateDmxElement( "RoundStats_t" );
		pAccumulated->AddAttributesFromStructure( &stat.accumulated, s_RoundStatsUnpack );
		pClass->SetValue( "accumulated", pAccumulated );

		CDmxElement *pMax = CreateDmxElement( "RoundStats_t" );
		pMax->AddAttributesFromStructure( &stat.max, s_RoundStatsUnpack );
		pClass->SetValue( "max", pMax );

		CDmxElement *pAccumulatedMVM = CreateDmxElement( "RoundStats_t" );
		pAccumulatedMVM->AddAttributesFromStructure( &stat.accumulatedMVM, s_RoundStatsUnpack );
		pClass->SetValue( "accumulatedmvm", pAccumulatedMVM );

		CDmxElement *pMaxMVM = CreateDmxElement( "RoundStats_t" );
		pMaxMVM->AddAttributesFromStructure( &stat.maxMVM, s_RoundStatsUnpack );
		pClass->SetValue( "maxmvm", pMaxMVM );
	}

	CDmxAttribute *pMapStatsList = pPlayerStats->AddAttribute( "aMapStats" );
	CUtlVector< CDmxElement* >& mapStats = pMapStatsList->GetArrayForEdit<CDmxElement*>();

	for( int i = 0; i < m_aMapStats.Count(); i++ )
	{
		const MapStats_t &stat = m_aMapStats[ i ];

		// strip out any garbage map data
		if ( !IsValidMapID( stat.iMapID ) )
			continue;

		CDmxElement *pMap = CreateDmxElement( "MapStats_t" );
		mapStats.AddToTail( pMap );

		CDmxElementModifyScope modifyClass( pMap );

		pMap->SetValue( "comment: mapname", GetMapNameFromID( stat.iMapID ) );
		pMap->AddAttributesFromStructure( &stat, s_MapStatsUnpack );

		CDmxElement *pAccumulated = CreateDmxElement( "RoundMapStats_t" );
		pAccumulated->AddAttributesFromStructure( &stat.accumulated, s_RoundMapStatsUnpack );
		pMap->SetValue( "accumulated", pAccumulated );
	}

	if ( IsX360() )
	{
#ifdef _X360
		if ( XBX_GetStorageDeviceId() == XBX_INVALID_STORAGE_ID || XBX_GetStorageDeviceId() == XBX_STORAGE_DECLINED )
			return;
#endif
	}

	char szFilename[_MAX_PATH];

	if ( IsX360() )
		Q_snprintf( szFilename, sizeof( szFilename ), "cfg:/tf2_playerstats.dmx" );
	else
		Q_snprintf( szFilename, sizeof( szFilename ), "tf2_playerstats.dmx" );

	{
		MEM_ALLOC_CREDIT();
		CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
		if ( SerializeDMX( buf, pPlayerStats, szFilename ) )
		{
			filesystem->WriteFile( szFilename, "MOD", buf );
		}
	}

	CleanupDMX( pPlayerStats );

	if ( IsX360() )
	{
		xboxsystem->FinishContainerWrites();
	}

	m_bStatsChanged = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFStatPanel::ReadStats( void )
{
	CDmxElement *pPlayerStats;

	DECLARE_DMX_CONTEXT();

	if ( IsX360() )
	{
#ifdef _X360
		if ( XBX_GetStorageDeviceId() == XBX_INVALID_STORAGE_ID || XBX_GetStorageDeviceId() == XBX_STORAGE_DECLINED )
			return false;
#endif
	}

	char	szFilename[_MAX_PATH];

	if ( IsX360() )
	{
		Q_snprintf( szFilename, sizeof( szFilename ), "cfg:/tf2_playerstats.dmx" );
	}
	else
	{
		Q_snprintf( szFilename, sizeof( szFilename ), "tf2_playerstats.dmx" );
	}

	MEM_ALLOC_CREDIT();

	bool bOk = UnserializeDMX( szFilename, "MOD", true, &pPlayerStats );

	if ( !bOk )
		return false;

	int iVersion = pPlayerStats->GetValue< int >( "iVersion" );
	if ( iVersion > PLAYERSTATS_FILE_VERSION )
	{
		// file is beyond our comprehension
		return false;
	}

	int iSteamID = pPlayerStats->GetValue<int>( "SteamID" );
	int iCRCFile = pPlayerStats->GetValue<int>( "iTimestamp" );	

	const CUtlVector< CDmxElement* > &aClassStatsList = pPlayerStats->GetArray< CDmxElement * >( "aClassStats" );
	int iCount = aClassStatsList.Count();
	m_aClassStats.SetCount( iCount );
	for( int i = 0; i < m_aClassStats.Count(); i++ )
	{
		CDmxElement *pClass = aClassStatsList[ i ];
		ClassStats_t &stat = m_aClassStats[ i ];

		pClass->UnpackIntoStructure( &stat, sizeof( stat ), s_ClassStatsUnpack );

		CDmxElement *pAccumulated = pClass->GetValue< CDmxElement * >( "accumulated" );
		if ( pAccumulated )
		{
			pAccumulated->UnpackIntoStructure( &stat.accumulated, sizeof( stat.accumulated ), s_RoundStatsUnpack );
		}

		CDmxElement *pMax = pClass->GetValue< CDmxElement * >( "max" );
		if ( pMax )
		{
			pMax->UnpackIntoStructure( &stat.max, sizeof( stat.max ), s_RoundStatsUnpack );
		}

		CDmxElement *pAccumulatedMVM = pClass->GetValue< CDmxElement * >( "accumulatedMVM" );
		if ( pAccumulatedMVM )
		{
			pAccumulatedMVM->UnpackIntoStructure( &stat.accumulatedMVM, sizeof( stat.accumulatedMVM ), s_RoundStatsUnpack );
		}

		CDmxElement *pMaxMVM = pClass->GetValue< CDmxElement * >( "maxMVM" );
		if ( pMaxMVM )
		{
			pMaxMVM->UnpackIntoStructure( &stat.maxMVM, sizeof( stat.maxMVM ), s_RoundStatsUnpack );
		}
	}

	const CUtlVector< CDmxElement* > &aMapStatsList = pPlayerStats->GetArray< CDmxElement * >( "aMapStats" );
	iCount = aMapStatsList.Count();
	m_aMapStats.SetCount( iCount );
	for( int i = 0; i < aMapStatsList.Count(); i++ )
	{
		CDmxElement *pMap = aMapStatsList[ i ];
		MapStats_t &stat = m_aMapStats[ i ];

		pMap->UnpackIntoStructure( &stat, sizeof( stat ), s_MapStatsUnpack );

		CDmxElement *pAccumulated = pMap->GetValue< CDmxElement * >( "accumulated" );
		if ( pAccumulated )
		{
			pAccumulated->UnpackIntoStructure( &stat.accumulated, sizeof( stat.accumulated ), s_RoundMapStatsUnpack );
		}
	}

	CleanupDMX( pPlayerStats );

	UpdateStatSummaryPanel();

	// check file CRC and steam ID to see if we think this file has not been tampered with
	int iCRC = CalcCRC( iSteamID );
	// does file CRC match CRC generated from file data, and is there a Steam ID in the file
	if ( ( iCRC == iCRCFile ) && ( iSteamID > 0 ) && steamapicontext->SteamUser() ) 
	{		
		// does the file Steam ID match current Steam ID (so you can't hand around files)
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		if ( steamID.GetAccountID() == (uint32) iSteamID )
		{
			m_bLocalFileTrusted = true;
		}
	}

	m_bStatsChanged = false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Calcs CRC of all stat data
//-----------------------------------------------------------------------------
int CTFStatPanel::CalcCRC( int iSteamID )
{
	CRC32_t crc;
	CRC32_Init( &crc );

	// make a CRC of stat data
	CRC32_ProcessBuffer( &crc, &iSteamID, sizeof( iSteamID ) );

	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		// add each class' data to the CRC
		ClassStats_t &classStats = GetClassStats( iClass );
		CRC32_ProcessBuffer( &crc, &classStats, sizeof( classStats ) );
		// since the class data structure is highly guessable from the file, add one other thing to make the CRC hard to hack w/o code disassembly
		int iObfuscate = iClass * iClass;
		CRC32_ProcessBuffer( &crc, &iObfuscate, sizeof( iObfuscate ) );	
	}

	CRC32_Final( &crc );

	return (int) ( crc & 0x7FFFFFFF );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::ShowStatPanel( int iClass, int iTeam, int iCurStatValue, TFStatType_t statType, RecordBreakType_t recordBreakType,
								 bool bAlive )
{
	bool bMVM = TFGameRules() && TFGameRules()->IsMannVsMachineMode();

	// If this is MvM mode and we're looking at the round end, dont show the stats
	// panel because the PVEWinPanel will be up
	if( bMVM && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		return;
	}

	ClassStats_t &classStats = GetClassStats( iClass );
	vgui::Label *pLabel = dynamic_cast<Label *>( FindChildByName( "summaryLabel" ) );
	if ( !pLabel )
		return;

	const char *pRecordTextSuffix[RECORDBREAK_MAX] = { "", "close", "tie", "best" };

	const char *pLocalizedTitle = bAlive ? "#StatPanel_Title_Alive" : "#StatPanel_Title_Dead";
	SetDialogVariable( "title", g_pVGuiLocalize->Find( pLocalizedTitle ) );
	SetDialogVariable( "stattextlarge", "" );
	SetDialogVariable( "stattextsmall", "" );
	if ( recordBreakType == RECORDBREAK_CLOSE )
	{		
		// if we are displaying that the player got close to a record, show current & best values
		char szCur[32],szBest[32];
		wchar_t wzCur[32],wzBest[32];
		GetStatValueAsString( iCurStatValue, statType, szCur, ARRAYSIZE( szCur ) );
		GetStatValueAsString( bMVM ? classStats.maxMVM.m_iStat[statType] : classStats.max.m_iStat[statType], statType, szBest, ARRAYSIZE( szBest ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( szCur, wzCur, sizeof( wzCur ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( szBest, wzBest, sizeof( wzBest ) );
		wchar_t *wzFormat = g_pVGuiLocalize->Find( "#StatPanel_Format_Close" );
		wchar_t wzText[256];
		g_pVGuiLocalize->ConstructString_safe( wzText, wzFormat, 2, wzCur, wzBest );
		SetDialogVariable( "stattextsmall", wzText );
	}
	else
	{
		// player broke or tied a record, just show current value
		char szValue[32];
		GetStatValueAsString( iCurStatValue, statType, szValue, ARRAYSIZE( szValue ) );
		SetDialogVariable( "stattextlarge", szValue );
	}

	SetDialogVariable( "statdesc", g_pVGuiLocalize->Find( CFmtStr( "%s_%s", bMVM ? g_szLocalizedMVMRecordText[statType] : g_szLocalizedRecordText[statType], 
		pRecordTextSuffix[recordBreakType] ) ) );

	// Set the class name. We can't use a dialog variable because it's a string that's already
	// been set using a dialog variable, and apparently we don't support nested dialog variables.
	wchar_t szOriginalSummary[ 256 ];
	wchar_t szSummary[ 256 ];

	// This is the field that "statdesc" completed for us
	pLabel->GetText( szOriginalSummary, sizeof( szOriginalSummary ) );
	const wchar_t *pszPlayerClass = L"undefined";

	if ( ( iClass >= TF_FIRST_NORMAL_CLASS ) && ( iClass <= TF_LAST_NORMAL_CLASS ) )
	{
		pszPlayerClass = g_pVGuiLocalize->Find( g_aPlayerClassNames[ iClass ] );
	}

	g_pVGuiLocalize->ConstructString_safe( szSummary, szOriginalSummary, 1, pszPlayerClass );

	pLabel->SetText( szSummary );

	if ( m_pClassImage )
	{
		m_pClassImage->SetClass( iTeam, iClass, 0 );
	}

	Show();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "player_spawn", pEventName ) == 0 )
	{
		int iUserID = event->GetInt( "userid" );
		if ( !C_TFPlayer::GetLocalTFPlayer() || ( C_TFPlayer::GetLocalTFPlayer()->GetUserID() != iUserID ) )
			return;

		if ( m_bDisplayAfterSpawn )
		{
			// if we have a panel to display after spawn, show it now
			vgui::ivgui()->AddTickSignal( GetVPanel(), 1000 );
			ShowStatPanel( m_iCurStatClass, m_iCurStatTeam, m_iCurStatValue, m_statType, m_recordBreakType, true );
			m_flTimeHide = gpGlobals->curtime + 12.0f;
			m_bDisplayAfterSpawn = false;
		}
		else
		{
			// hide panel if we're currently showing it
			Hide();
		}		
		m_flTimeLastSpawn = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/StatPanel_Base.res" );

	vgui::Panel *pStatBox = FindChildByName("StatBox");
	if ( pStatBox )
	{
		// Dirty hack: Make the statbox update now, and then change its bgColor.
		// When it then gets ApplySchemeSetting called shortly after this, it doesn't
		// reapply the scheme because its dirty-scheme flag has been removed.
		pStatBox->ApplySchemeSettings( pScheme );
		pStatBox->SetBgColor( GetSchemeColor("TransparentLightBlack", pScheme) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::OnTick()
{
	// see if it's time to hide the panel
	if ( m_flTimeHide > 0 && gpGlobals->curtime > m_flTimeHide )
	{
		Hide();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::Show()
{
	// Josh: See comment in tf_hud_freezepanel.cpp - CTFFreezePanel::Show()
	MakeReadyForUse();

	m_bShouldBeVisible = true;

	HideLowerPriorityHudElementsInGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::Hide()
{
	m_bShouldBeVisible = false;
	if ( m_flTimeHide > 0 )
	{
		m_flTimeHide = 0;
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}

	UnhideLowerPriorityHudElementsInGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFStatPanel::ShouldDraw( void )
{
	if ( !m_bShouldBeVisible )
		return false;

	if ( IsTakingAFreezecamScreenshot() )
		return false;
	
	if ( TFGameRules() && TFGameRules()->IsInArenaMode() == true )
	{
		m_flTimeHide = gpGlobals->curtime;
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::ClearStatsInMemory( void )
{
	m_aClassStats.RemoveAll();
	m_aMapStats.RemoveAll();
	m_bStatsChanged = true;
	ResetDisplayedStat();
	UpdateStatSummaryPanel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatPanel::ResetStats( void )
{
	// Only allow this action if the player is connected to steam and the achievement manager exists.
	// Otherwise, our stat panel stats, stat file, and local steam context stats will be out of sync because UploadStats will fail.
	CAchievementMgr *pAchievementMgr = dynamic_cast<CAchievementMgr *>( engine->GetAchievementMgr() );
	if ( !steamapicontext->SteamUserStats() || !pAchievementMgr )
	{
		MessageBox *msg = new MessageBox( "#TF_SteamRequired", "#TF_SteamRequiredResetStats" );
		if ( msg != NULL )
		{
			msg->AddActionSignalTarget(this);
			msg->MoveToFront();
			msg->DoModal();
		}
		return;
	}

	// The following operations are order dependent:

	// Nukes the panel's version of the stats. Note that after this is called the local steam stats are out of sync.
	ClearStatsInMemory();

	// WriteStats will write out our stat file (not really used on the PC, authoritative on the XBox 360).
	WriteStats();

	// At this point local and remote steam stats are out of sync.

	// Nuke the players stats on steam to seal the deal.
	steamapicontext->SteamUserStats()->ResetAllStats( false );

	// UploadStats will pull the players' class stats from CTFStatPanel and stomp the steam stats in memory.
	g_TFSteamStats.UploadStats();
}

//-----------------------------------------------------------------------------
// Purpose: returns class stat struct for specified class
//-----------------------------------------------------------------------------
ClassStats_t &CTFStatPanel::GetClassStats( int iClass )
{
	Assert( statPanel );
	Assert( iClass >= TF_FIRST_NORMAL_CLASS );
	Assert( iClass <= TF_LAST_NORMAL_CLASS );
	int i;
	for( i = 0; i < statPanel->m_aClassStats.Count(); i++ )
	{
		if ( statPanel->m_aClassStats[i].iPlayerClass == iClass )
			return statPanel->m_aClassStats[i];
	}

	ClassStats_t stats;
	stats.iPlayerClass = iClass;
	statPanel->m_aClassStats.AddToTail( stats );
	return statPanel->m_aClassStats[statPanel->m_aClassStats.Count()-1];
}

MapStats_t &CTFStatPanel::GetMapStats( map_identifier_t iMapID )
{
	Assert( statPanel );
	Assert( IsValidMapID( iMapID ) );
	int i;
	for( i = 0; i < statPanel->m_aMapStats.Count(); i++ )
	{
		if ( statPanel->m_aMapStats[i].iMapID == iMapID )
			return statPanel->m_aMapStats[i];
	}

	MapStats_t stats;
	stats.iMapID = iMapID;
	statPanel->m_aMapStats.AddToTail( stats );
	return statPanel->m_aMapStats[statPanel->m_aMapStats.Count()-1];
}

bool CTFStatPanel::IsValidMapID( map_identifier_t iMapID )
{
	for ( int i = 0; i < GetItemSchema()->GetMapCount(); i++ )
	{
		const MapDef_t* pMap = GetItemSchema()->GetMasterMapDefByIndex( i );
		if ( iMapID == pMap->GetStatsIdentifier() )
		{
			return true;
		}
	}

	return false;
}

const char* CTFStatPanel::GetMapNameFromID( map_identifier_t iMapID )
{
	for ( int i = 0; i < GetItemSchema()->GetMapCount(); i++ )
	{
		const MapDef_t* pMap = GetItemSchema()->GetMasterMapDefByIndex( i );
		if ( iMapID == pMap->GetStatsIdentifier() )
		{
			return pMap->pszMapName;
		}
	}

	return "";
}

//-----------------------------------------------------------------------------
// Purpose: Updates the stat summary panel w/current stats
//-----------------------------------------------------------------------------
void CTFStatPanel::UpdateStatSummaryPanel()
{
	UpdateStatSummaryPanels( m_aClassStats );
}


//-----------------------------------------------------------------------------
// Purpose: Return the total time this player has played the game, in hours.
//-----------------------------------------------------------------------------
float CTFStatPanel::GetTotalHoursPlayed( void )
{
	float totalTimePlayed = 0;

	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{		
		totalTimePlayed += GetClassStats( iClass ).accumulated.m_iStat[ TFSTAT_PLAYTIME ] + GetClassStats( iClass ).accumulatedMVM.m_iStat[ TFSTAT_PLAYTIME ];
	}
	
	return totalTimePlayed / ( 60.0f * 60.0f );
}


//-----------------------------------------------------------------------------
// Purpose: Renders stat value as string
//-----------------------------------------------------------------------------
void CTFStatPanel::GetStatValueAsString( int iValue, TFStatType_t statType, char *value, int valuelen )
{
	if ( TFSTAT_PLAYTIME == statType )
	{
		// Format time as a time string
		Q_strncpy( value, FormatSeconds( iValue ), valuelen );
	}
	else
	{
		// all other stats are just displayed as #'s
		Q_snprintf( value, valuelen, "%d", iValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when we get a stat update for the local player
//-----------------------------------------------------------------------------
void CTFStatPanel::MsgFunc_PlayerStatsUpdate( bf_read &msg )
{
	RoundStats_t stats;

	// get the fixed-size information
	int iClass = msg.ReadByte();
	bool bAlive = msg.ReadByte();
	int iSendBits = msg.ReadLong();

	Assert( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_LAST_NORMAL_CLASS );
	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass > TF_LAST_NORMAL_CLASS )
		return;

	// the bitfield indicates which stats are contained in the message.  Set the stats appropriately.
	int iStat = TFSTAT_FIRST;
	while ( iSendBits > 0 && iStat <= TFSTAT_LAST )
	{
		if ( iSendBits & 1 )
		{
			stats.m_iStat[iStat] = msg.ReadLong();
		}
		iSendBits >>= 1;
		iStat++;
	}

	// sanity check: the message should contain exactly the # of bytes we expect based on the bit field
	Assert( !msg.IsOverflowed() );
	Assert( 0 == msg.GetNumBytesLeft() );
	// if byte count isn't correct, bail out and don't use this data, rather than risk polluting player stats with garbage
	if ( msg.IsOverflowed() || ( 0 != msg.GetNumBytesLeft() ) )
		return;

	UpdateStats( iClass, stats, bAlive );
}

void CTFStatPanel::MsgFunc_MapStatsUpdate( bf_read &msg )
{
	RoundMapStats_t stats;

	// get the fixed-size information
	map_identifier_t iMapID = msg.ReadUBitLong( 32 );
	int iSendBits = msg.ReadLong();

	if ( !IsValidMapID( iMapID ) )
		return;

	// the bitfield indicates which stats are contained in the message.  Set the stats appropriately.
	int iStat = TFMAPSTAT_FIRST;
	while ( iSendBits > 0 && iStat <= TFMAPSTAT_LAST )
	{
		if ( iSendBits & 1 )
		{
			stats.m_iStat[iStat] = msg.ReadLong();
		}
		iSendBits >>= 1;
		iStat++;
	}

	// sanity check: the message should contain exactly the # of bytes we expect based on the bit field
	Assert( !msg.IsOverflowed() );
	Assert( 0 == msg.GetNumBytesLeft() );
	// if byte count isn't correct, bail out and don't use this data, rather than risk polluting player stats with garbage
	if ( msg.IsOverflowed() || ( 0 != msg.GetNumBytesLeft() ) )
		return;

	UpdateMapStats( iMapID, stats );
}

/**********************************************************************************/

void TestStatPanel( const CCommand &args )
{
	int iPanelType;

	if( args.ArgC() < 2 )
	{
		ConMsg( "Usage:  teststatpanel < panel type > < optional record type >\n" );
		ConMsg( "Usable panel types are %d to %d. Record types are %d to %d\n", TFSTAT_FIRST, TFSTAT_LAST, RECORDBREAK_NONE+1, RECORDBREAK_MAX-1 );
		return;
	}

	if ( statPanel )
	{
		iPanelType = atoi( args.Arg( 1 ) );
		int iRecordType = RECORDBREAK_BEST;

		if ( args.ArgC() >= 3 )
		{
			iRecordType = atoi( args.Arg( 2 ) );
		}

		if ( ( iPanelType <= TFSTAT_UNDEFINED ) || ( iPanelType >= TFSTAT_TOTAL ) || (iRecordType <= RECORDBREAK_NONE) || (iRecordType >= RECORDBREAK_MAX) )
		{
			ConMsg( "Usage:  teststatpanel < panel type > < optional record type >\n" );
			ConMsg( "Usable panel types are %d to %d. Record types are %d to %d\n", TFSTAT_FIRST, TFSTAT_LAST, RECORDBREAK_NONE+1, RECORDBREAK_MAX-1 );
			return;
		}

		statPanel->TestStatPanel( (TFStatType_t) iPanelType, (RecordBreakType_t)iRecordType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HideStatPanel()
{
	if ( statPanel )
	{
		statPanel->Hide();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ResetPlayerStats()
{
	if ( statPanel )
	{
		statPanel->ResetStats();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RefreshPlayerStats()
{
	if ( statPanel )
	{
		if ( !statPanel->ReadStats() )
		{
			// Read failed, need to clear everything
			statPanel->ClearStatsInMemory();
		}
	}
}

ConCommand teststatpanel( "teststatpanel", TestStatPanel, "", FCVAR_DEVELOPMENTONLY );
ConCommand hidestatpanel( "hidestatpanel", HideStatPanel, "", FCVAR_DEVELOPMENTONLY );
ConCommand refreshplayerstats( "refreshplayerstats", RefreshPlayerStats, "", FCVAR_DEVELOPMENTONLY );

ConCommand resetplayerstats( "resetplayerstats", ResetPlayerStats, "", FCVAR_CLIENTCMD_CAN_EXECUTE );

