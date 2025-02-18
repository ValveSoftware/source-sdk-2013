
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_steamstats.h"
#include "tf_hud_statpanel.h"
#include "achievementmgr.h"
#include "engine/imatchmaking.h"
#include "ipresence.h"
#include "../game/shared/tf/tf_shareddefs.h"
#include "../game/shared/tf/tf_gamestats_shared.h"

struct StatMap_t
{
	const char *pszName;
	int	iStat;
	int iLiveStat;
};

// subset of stats which we store in Steam
StatMap_t g_SteamStats[] = {
	{ "iNumberOfKills",			TFSTAT_KILLS,				PROPERTY_KILLS,					},
	{ "iDamageDealt",			TFSTAT_DAMAGE,				PROPERTY_DAMAGE_DEALT,			},
	{ "iPlayTime",				TFSTAT_PLAYTIME,			PROPERTY_PLAY_TIME,				},
	{ "iPointCaptures",			TFSTAT_CAPTURES,			PROPERTY_POINT_CAPTURES,		},
	{ "iPointDefenses",			TFSTAT_DEFENSES,			PROPERTY_POINT_DEFENSES,		},
	{ "iDominations",			TFSTAT_DOMINATIONS,			PROPERTY_DOMINATIONS,			},
	{ "iRevenge",				TFSTAT_REVENGE,				PROPERTY_REVENGE,				},
	{ "iPointsScored",			TFSTAT_POINTSSCORED,		PROPERTY_POINTS_SCORED,			},
	{ "iBuildingsDestroyed",	TFSTAT_BUILDINGSDESTROYED,	PROPERTY_BUILDINGS_DESTROYED,	},
	{ "iNumInvulnerable",		TFSTAT_INVULNS,				PROPERTY_INVULNS,				},
	{ "iKillAssists",			TFSTAT_KILLASSISTS,			PROPERTY_KILL_ASSISTS,			},
};		

// class specific stats
StatMap_t g_SteamStats_Pyro[] = {
	{ "iFireDamage",			TFSTAT_FIREDAMAGE,			-1,								}, // Added post-XBox, isn't saved in Live
	{ NULL,						0,							0,								},
};

StatMap_t g_SteamStats_Demoman[] = {
	{ "iBlastDamage",			TFSTAT_BLASTDAMAGE,			-1,								}, // Added post-XBox, isn't saved in Live
	{ NULL,						0,							0,								},
};

StatMap_t g_SteamStats_Engineer[] = {
	{ "iBuildingsBuilt",		TFSTAT_BUILDINGSBUILT,		PROPERTY_BUILDINGS_BUILT,		},
	{ "iSentryKills",			TFSTAT_MAXSENTRYKILLS,		PROPERTY_SENTRY_KILLS,			},
	{ "iNumTeleports",			TFSTAT_TELEPORTS,			PROPERTY_TELEPORTS,				},
	{ NULL,						0,							0,								},
};

StatMap_t g_SteamStats_Medic[] = {
	{ "iHealthPointsHealed",	TFSTAT_HEALING,				PROPERTY_HEALTH_POINTS_HEALED,	},
	{ NULL,						0,							0,								},
};

StatMap_t g_SteamStats_Sniper[] = {
	{ "iHeadshots",				TFSTAT_HEADSHOTS,			PROPERTY_HEADSHOTS,				},
	{ NULL,						0,							0,								},
};

StatMap_t g_SteamStats_Spy[] = {
	{ "iHeadshots",				TFSTAT_HEADSHOTS,			PROPERTY_HEADSHOTS,				},
	{ "iBackstabs",				TFSTAT_BACKSTABS,			PROPERTY_BACKSTABS,				},
	{ "iHealthPointsLeached",	TFSTAT_HEALTHLEACHED,		PROPERTY_HEALTH_POINTS_LEACHED,	},
	{ NULL,						0,							0,								},
};

StatMap_t* g_SteamStats_Class[] = {
	NULL,					// Undefined
	NULL,					// Scout
	g_SteamStats_Sniper,	// Sniper
	NULL,					// Soldier
	g_SteamStats_Demoman,	// Demoman
	g_SteamStats_Medic,		// Medic
	NULL,					// Heavy
	g_SteamStats_Pyro,		// Pyro
	g_SteamStats_Spy,		// Spy
	g_SteamStats_Engineer,	// Engineer
};

// subset of map stats which we store in Steam
StatMap_t g_SteamMapStats[] = {
	{ "iPlayTime",				TFMAPSTAT_PLAYTIME,			PROPERTY_PLAY_TIME,				},
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFSteamStats::CTFSteamStats() 
{
	m_flTimeNextForceUpload = 0;
}

//-----------------------------------------------------------------------------
// Purpose: called at init time after all systems are init'd.  We have to
//			do this in PostInit because the Steam app ID is not available earlier
//-----------------------------------------------------------------------------
void CTFSteamStats::PostInit()
{
	SetNextForceUploadTime();
	ListenForGameEvent( "player_stats_updated" );
	ListenForGameEvent( "user_data_downloaded" );
}

//-----------------------------------------------------------------------------
// Purpose: called at level shutdown
//-----------------------------------------------------------------------------
void CTFSteamStats::LevelShutdownPreEntity()
{
	// upload user stats to Steam on every map change
	UploadStats();
}

//-----------------------------------------------------------------------------
// Purpose: called when the stats have changed in-game
//-----------------------------------------------------------------------------
void CTFSteamStats::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();
	if ( 0 == Q_strcmp( pEventName, "player_stats_updated" ) )
	{
		bool bForceUpload = event->GetBool( "forceupload" );

		// if we haven't uploaded stats in a long time, upload them 
		if ( ( gpGlobals->curtime >= m_flTimeNextForceUpload ) || bForceUpload )
		{
			UploadStats();
		}
	}
	else if ( 0 == Q_strcmp( pEventName, "user_data_downloaded" ) )
	{
		Assert( steamapicontext->SteamUserStats() );
		if ( !steamapicontext->SteamUserStats() )
			return; 
		CTFStatPanel *pStatPanel = GET_HUDELEMENT( CTFStatPanel );
		Assert( pStatPanel );

		for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
		{
			// Grab generic stats:
			ClassStats_t &classStats = CTFStatPanel::GetClassStats( iClass );
			for ( int iStat = 0; iStat < ARRAYSIZE( g_SteamStats ); iStat++ )
			{
				char szStatName[256];
				int iData;

				Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.accum.%s", g_aPlayerClassNames_NonLocalized[iClass], g_SteamStats[iStat].pszName );
				if ( steamapicontext->SteamUserStats()->GetStat( szStatName, &iData ) )
				{					
					classStats.accumulated.m_iStat[g_SteamStats[iStat].iStat] = iData;					
				}
				Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.max.%s", g_aPlayerClassNames_NonLocalized[iClass], g_SteamStats[iStat].pszName );
				if ( steamapicontext->SteamUserStats()->GetStat( szStatName, &iData ) )
				{		
					classStats.max.m_iStat[g_SteamStats[iStat].iStat] = iData;
				}

				// MVM Stats
				Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.mvm.accum.%s", g_aPlayerClassNames_NonLocalized[iClass], g_SteamStats[iStat].pszName );
				if ( steamapicontext->SteamUserStats()->GetStat( szStatName, &iData ) )
				{					
					classStats.accumulatedMVM.m_iStat[g_SteamStats[iStat].iStat] = iData;					
				}
				Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.mvm.max.%s", g_aPlayerClassNames_NonLocalized[iClass], g_SteamStats[iStat].pszName );
				if ( steamapicontext->SteamUserStats()->GetStat( szStatName, &iData ) )
				{		
					classStats.maxMVM.m_iStat[g_SteamStats[iStat].iStat] = iData;
				}	
			}

			// Grab class specific stats:
			StatMap_t* pClassStatMap = g_SteamStats_Class[iClass];
			if ( pClassStatMap )
			{
				int iStat = 0;
				do
				{
					char szStatName[256];
					int iData;

					Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.accum.%s", g_aPlayerClassNames_NonLocalized[iClass], pClassStatMap[iStat].pszName );
					if ( steamapicontext->SteamUserStats()->GetStat( szStatName, &iData ) )
					{					
						classStats.accumulated.m_iStat[pClassStatMap[iStat].iStat] = iData;					
					}
					Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.max.%s", g_aPlayerClassNames_NonLocalized[iClass], pClassStatMap[iStat].pszName );
					if ( steamapicontext->SteamUserStats()->GetStat( szStatName, &iData ) )
					{		
						classStats.max.m_iStat[pClassStatMap[iStat].iStat] = iData;
					}

					// MVM Stats
					Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.mvm.accum.%s", g_aPlayerClassNames_NonLocalized[iClass], pClassStatMap[iStat].pszName );
					if ( steamapicontext->SteamUserStats()->GetStat( szStatName, &iData ) )
					{					
						classStats.accumulatedMVM.m_iStat[pClassStatMap[iStat].iStat] = iData;					
					}
					Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.mvm.max.%s", g_aPlayerClassNames_NonLocalized[iClass], pClassStatMap[iStat].pszName );
					if ( steamapicontext->SteamUserStats()->GetStat( szStatName, &iData ) )
					{		
						classStats.maxMVM.m_iStat[pClassStatMap[iStat].iStat] = iData;
					}
					iStat++;
				}
				while ( pClassStatMap[iStat].pszName );
			}
		}

		for ( int i = 0; i < GetItemSchema()->GetMapCount(); i++ )
		{
			const MapDef_t* pMap = GetItemSchema()->GetMasterMapDefByIndex( i );

			// Grab generic stats:
			MapStats_t &mapStats = CTFStatPanel::GetMapStats( pMap->GetStatsIdentifier() );
			for ( int iStat = 0; iStat < ARRAYSIZE( g_SteamMapStats ); iStat++ )
			{
				char szStatName[256];
				int iData;

				Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.accum.%s",  pMap->pszMapName, g_SteamMapStats[iStat].pszName );
				if ( steamapicontext->SteamUserStats()->GetStat( szStatName, &iData ) )
				{					
					mapStats.accumulated.m_iStat[g_SteamMapStats[iStat].iStat] = iData;					
				}
			}
		}

		IGameEvent * pEvent = gameeventmanager->CreateEvent( "player_stats_updated" );
		if ( pEvent )
		{
			pEvent->SetBool( "forceupload", false );
			gameeventmanager->FireEventClientSide( pEvent );
		}

		pStatPanel->SetStatsChanged( true );
		pStatPanel->UpdateStatSummaryPanel();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Uploads stats for current Steam user to Steam
//-----------------------------------------------------------------------------
void CTFSteamStats::UploadStats()
{
	if ( IsX360() )
	{
		ReportLiveStats();
		return;
	}

	// Only upload if Steam is running & the achievement manager exists.
	if ( !steamapicontext->SteamUserStats() )
		return; 

	CAchievementMgr *pAchievementMgr = dynamic_cast<CAchievementMgr *>( engine->GetAchievementMgr() );
	if ( !pAchievementMgr )
		return;

	// Stomp local steam context stats with those in the stat panel.
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		// Set generic stats:
		ClassStats_t &classStats = CTFStatPanel::GetClassStats( iClass );
		for ( int iStat = 0; iStat < ARRAYSIZE( g_SteamStats ); iStat++ )
		{
			char szStatName[256];

			Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.accum.%s", g_aPlayerClassNames_NonLocalized[iClass], g_SteamStats[iStat].pszName );
			steamapicontext->SteamUserStats()->SetStat( szStatName, classStats.accumulated.m_iStat[g_SteamStats[iStat].iStat] );

			Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.max.%s", g_aPlayerClassNames_NonLocalized[iClass], g_SteamStats[iStat].pszName );
			steamapicontext->SteamUserStats()->SetStat( szStatName, classStats.max.m_iStat[g_SteamStats[iStat].iStat] );

			// MVM Stats
			Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.mvm.accum.%s", g_aPlayerClassNames_NonLocalized[iClass], g_SteamStats[iStat].pszName );
			steamapicontext->SteamUserStats()->SetStat( szStatName, classStats.accumulatedMVM.m_iStat[g_SteamStats[iStat].iStat] );

			Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.mvm.max.%s", g_aPlayerClassNames_NonLocalized[iClass], g_SteamStats[iStat].pszName );
			steamapicontext->SteamUserStats()->SetStat( szStatName, classStats.maxMVM.m_iStat[g_SteamStats[iStat].iStat] );
		}

		// Set class specific stats:
		StatMap_t* pClassStatMap = g_SteamStats_Class[iClass];
		if ( pClassStatMap )
		{
			int iStat = 0;
			do
			{
				char szStatName[256];

				Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.accum.%s", g_aPlayerClassNames_NonLocalized[iClass], pClassStatMap[iStat].pszName );
				steamapicontext->SteamUserStats()->SetStat( szStatName, classStats.accumulated.m_iStat[pClassStatMap[iStat].iStat] );

				Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.max.%s", g_aPlayerClassNames_NonLocalized[iClass], pClassStatMap[iStat].pszName );
				steamapicontext->SteamUserStats()->SetStat( szStatName, classStats.max.m_iStat[pClassStatMap[iStat].iStat] );

				// MVM Stats
				Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.mvm.accum.%s", g_aPlayerClassNames_NonLocalized[iClass], pClassStatMap[iStat].pszName );
				steamapicontext->SteamUserStats()->SetStat( szStatName, classStats.accumulatedMVM.m_iStat[pClassStatMap[iStat].iStat] );

				Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.mvm.max.%s", g_aPlayerClassNames_NonLocalized[iClass], pClassStatMap[iStat].pszName );
				steamapicontext->SteamUserStats()->SetStat( szStatName, classStats.maxMVM.m_iStat[pClassStatMap[iStat].iStat] );

				iStat++;
			}
			while ( pClassStatMap[iStat].pszName );
		}
	}

	// Stomp local steam context stats with those in the stat panel.
	for ( int i = 0; i < GetItemSchema()->GetMapCount(); i++ )
	{
		const MapDef_t* pMap = GetItemSchema()->GetMasterMapDefByIndex( i );

		// Set generic stats:
		MapStats_t &mapStats = CTFStatPanel::GetMapStats( pMap->GetStatsIdentifier() );
		for ( int iStat = 0; iStat < ARRAYSIZE( g_SteamMapStats ); iStat++ )
		{
			char szStatName[256];

			Q_snprintf( szStatName, ARRAYSIZE( szStatName ), "%s.accum.%s", pMap->pszMapName, g_SteamMapStats[iStat].pszName );
			steamapicontext->SteamUserStats()->SetStat( szStatName, mapStats.accumulated.m_iStat[g_SteamMapStats[iStat].iStat] );
		}
	}

	// Send our local steam context stats to the server.
	pAchievementMgr->UploadUserData();
	SetNextForceUploadTime();

	// Now everything should be sync'd up (stat panel, local steam context, remote steam depot).
}

//-----------------------------------------------------------------------------
// Purpose: Accumulate player stats and send them to matchmaking for reporting to Live
//-----------------------------------------------------------------------------
void CTFSteamStats::ReportLiveStats()
{
	int statsTotals[ARRAYSIZE( g_SteamStats )];
	Q_memset( &statsTotals, 0, sizeof( statsTotals ) );

	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		ClassStats_t &classStats = CTFStatPanel::GetClassStats( iClass );
		for ( int iStat = 0; iStat < ARRAYSIZE( g_SteamStats ); iStat++ )
		{
			statsTotals[iStat] = MAX( statsTotals[iStat], classStats.max.m_iStat[g_SteamStats[iStat].iStat] );
		}
	}

	// send the stats to matchmaking
	for ( int i = 0; i < ARRAYSIZE( g_SteamStats ); ++i )
	{
		// Points scored is looked up by the stats reporting function
		if ( g_SteamStats[i].iLiveStat == PROPERTY_POINTS_SCORED )
			continue;

		// If we hit this assert, we've added a new stat that Live won't know how to store
		Assert( g_SteamStats[i].iLiveStat != -1 );

		if ( g_SteamStats[i].iLiveStat != -1 )
		{
			presence->SetStat( g_SteamStats[i].iLiveStat, statsTotals[i], XUSER_DATA_TYPE_INT32 );
		}
	}

	presence->UploadStats();
}

//-----------------------------------------------------------------------------
// Purpose: sets the next time to force a stats upload at
//-----------------------------------------------------------------------------
void CTFSteamStats::SetNextForceUploadTime()
{
	// pick a time a while from now (an hour +/- 15 mins) to upload stats if we haven't gotten a map change by then
	m_flTimeNextForceUpload = gpGlobals->curtime + ( 60 * RandomInt( 45, 75 ) );
}

CTFSteamStats g_TFSteamStats;
