//========= Copyright Valve Corporation, All rights reserved. ============//
//----------------------------------------------------------------------------------------------------------------
// tf_bot_manager.cpp
// Team Fortress NextBotManager
// Tom Bui, February 2010
//----------------------------------------------------------------------------------------------------------------

#include "cbase.h"
#include "tf_bot_manager.h"

#include "Player/NextBotPlayer.h"
#include "team.h"
#include "tf_bot.h"
#include "tf_gamerules.h"
#include "bot/map_entities/tf_bot_hint.h"
#include "bot/map_entities/tf_bot_hint_sentrygun.h"
#include "bot/map_entities/tf_bot_hint_teleporter_exit.h"


//----------------------------------------------------------------------------------------------------------------

// Creates and sets CTFBotManager as the NextBotManager singleton
static CTFBotManager sTFBotManager;

ConVar tf_bot_difficulty( "tf_bot_difficulty", "1", FCVAR_NONE, "Defines the skill of bots joining the game.  Values are: 0=easy, 1=normal, 2=hard, 3=expert." );
ConVar tf_bot_quota( "tf_bot_quota", "0", FCVAR_NONE, "Determines the total number of tf bots in the game." );
ConVar tf_bot_quota_mode( "tf_bot_quota_mode", "normal", FCVAR_NONE, "Determines the type of quota.\nAllowed values: 'normal', 'fill', and 'match'.\nIf 'fill', the server will adjust bots to keep N players in the game, where N is bot_quota.\nIf 'match', the server will maintain a 1:N ratio of humans to bots, where N is bot_quota." );
ConVar tf_bot_join_after_player( "tf_bot_join_after_player", "1", FCVAR_NONE, "If nonzero, bots wait until a player joins before entering the game." );
ConVar tf_bot_auto_vacate( "tf_bot_auto_vacate", "1", FCVAR_NONE, "If nonzero, bots will automatically leave to make room for human players." );
ConVar tf_bot_offline_practice( "tf_bot_offline_practice", "0", FCVAR_NONE, "Tells the server that it is in offline practice mode." );
ConVar tf_bot_melee_only( "tf_bot_melee_only", "0", FCVAR_GAMEDLL, "If nonzero, TFBots will only use melee weapons" );

extern const char *GetRandomBotName( void );
extern void CreateBotName( int iTeam, int iClassIndex, CTFBot::DifficultyType skill, char* pBuffer, int iBufferSize );

static bool UTIL_KickBotFromTeam( int kickTeam )
{
	int i;

	// try to kick a dead bot first
	for ( i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		CTFBot* pBot = dynamic_cast<CTFBot*>(pPlayer);

		if (pBot == NULL)
			continue;

		if ( pBot->HasAttribute( CTFBot::QUOTA_MANANGED ) == false )
			continue;

		if ( ( pPlayer->GetFlags() & FL_FAKECLIENT ) == 0 )
			continue;

		if ( !pPlayer->IsAlive() && pPlayer->GetTeamNumber() == kickTeam )
		{
			// its a bot on the right team - kick it
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pPlayer->GetUserID() ) );

			return true;
		}
	}

	// no dead bots, kick any bot on the given team
	for ( i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		CTFBot* pBot = dynamic_cast<CTFBot*>(pPlayer);

		if (pBot == NULL)
			continue;

		if ( pBot->HasAttribute( CTFBot::QUOTA_MANANGED ) == false )
			continue;

		if ( ( pPlayer->GetFlags() & FL_FAKECLIENT ) == 0 )
			continue;

		if (pPlayer->GetTeamNumber() == kickTeam)
		{
			// its a bot on the right team - kick it
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pPlayer->GetUserID() ) );

			return true;
		}
	}

	return false;
}

//----------------------------------------------------------------------------------------------------------------

CTFBotManager::CTFBotManager()
	: NextBotManager()
	, m_flNextPeriodicThink( 0 )
{
	NextBotManager::SetInstance( this );
}


//----------------------------------------------------------------------------------------------------------------
CTFBotManager::~CTFBotManager()
{
	NextBotManager::SetInstance( NULL );
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::OnMapLoaded( void )
{
	NextBotManager::OnMapLoaded();

	ClearStuckBotData();
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::OnRoundRestart( void )
{
	NextBotManager::OnRoundRestart();

	// clear all hint ownership
	CTFBotHint *hint = NULL;
	while( ( hint = (CTFBotHint *)( gEntList.FindEntityByClassname( hint, "func_tfbot_hint" ) ) ) != NULL )
	{
		hint->SetOwnerEntity( NULL );
	}

	CTFBotHintSentrygun *sentryHint = NULL;
	while( ( sentryHint = (CTFBotHintSentrygun *)( gEntList.FindEntityByClassname( sentryHint, "bot_hint_sentrygun" ) ) ) != NULL )
	{
		sentryHint->SetOwnerEntity( NULL );
	}

	CTFBotHintTeleporterExit *teleporterHint = NULL;
	while( ( teleporterHint = (CTFBotHintTeleporterExit *)( gEntList.FindEntityByClassname( teleporterHint, "bot_hint_teleporter_exit" ) ) ) != NULL )
	{
		teleporterHint->SetOwnerEntity( NULL );
	}


#ifdef TF_CREEP_MODE
	m_creepExperience[ TF_TEAM_RED ] = 0;
	m_creepExperience[ TF_TEAM_BLUE ] = 0;
#endif

	m_isMedeivalBossScenarioSetup = false;
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::Update()
{
	MaintainBotQuota();

	DrawStuckBotData();

#ifdef TF_CREEP_MODE
	UpdateCreepWaves();
#endif

	NextBotManager::Update();
}


#ifdef TF_CREEP_MODE
ConVar tf_creep_initial_delay( "tf_creep_initial_delay", "30" );
ConVar tf_creep_wave_interval( "tf_creep_wave_interval", "30" );
ConVar tf_creep_wave_count( "tf_creep_wave_count", "3" );
ConVar tf_creep_class( "tf_creep_class", "heavyweapons" );
ConVar tf_creep_level_up( "tf_creep_level_up", "6" );


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::UpdateCreepWaves()
{
	if ( !TFGameRules()->IsCreepWaveMode() )
		return;

	if ( TFGameRules()->RoundHasBeenWon() )
	{
		// no more creep waves - game is over
		return;
	}

	if ( TFGameRules()->InSetup() || TFGameRules()->State_Get() == GR_STATE_STARTGAME || TFGameRules()->State_Get() == GR_STATE_PREROUND )
	{
		// no creeps at start of round
		m_creepWaveTimer.Start( tf_creep_initial_delay.GetFloat() );

		// delete all creeps
		for( int i=1; i<=gpGlobals->maxClients; ++i )
		{
			CBasePlayer *player = static_cast< CBasePlayer * >( UTIL_PlayerByIndex( i ) );

			if ( !player )
				continue;

			if ( FNullEnt( player->edict() ) )
				continue;

			CTFBot *creep = ToTFBot( player );
			if ( !creep || !creep->HasAttribute( CTFBot::IS_NPC ) )
				continue;

			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", player->GetUserID() ) );
		}

		return;
	}	

	if ( m_creepWaveTimer.IsElapsed() )
	{
		m_creepWaveTimer.Start( tf_creep_wave_interval.GetFloat() );

		SpawnCreepWave( TF_TEAM_RED );
		SpawnCreepWave( TF_TEAM_BLUE );
	}
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::SpawnCreepWave( int team )
{
	CTFBotSquad *squad = new CTFBotSquad;

	for( int i=0; i<tf_creep_wave_count.GetInt(); ++i )
	{
		SpawnCreep( team, squad );
	}
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::SpawnCreep( int team, CTFBotSquad *squad )
{
	CTFBot *bot = NextBotCreatePlayerBot< CTFBot >( "Creep" );

	if ( !bot ) 
		return;

	bot->SetAttribute( CTFBot::IS_NPC );
	bot->HandleCommand_JoinTeam( team == TF_TEAM_RED ? "red" : "blue" );
	bot->SetDifficulty( CTFBot::NORMAL );
	bot->HandleCommand_JoinClass( tf_creep_class.GetString() );
	bot->JoinSquad( squad );
	bot->AddGlowEffect();
	//BotGenerateAndWearItem( bot, "Honest Halo" );
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::OnCreepKilled( CTFPlayer *killer )
{
	CTFBot *bot = ToTFBot( killer );
	if ( bot && bot->HasAttribute( CTFBot::IS_NPC ) )
		return;

	++m_creepExperience[ killer->GetTeamNumber() ];

/*
	int xp = m_creepExperience[ killer->GetTeamNumber() ];
	int level = xp / tf_creep_level_up.GetInt();
	int left = xp % tf_creep_level_up.GetInt();

	char text[256];
	Q_snprintf( text, sizeof(text), "%s killed a creep. %s team LVL = %d+%d/%d\n", 
				killer->GetPlayerName(), 
				killer->GetTeamNumber() == TF_TEAM_RED ? "Red" : "Blue", 
				level+1, left, tf_creep_level_up.GetInt() );

	UTIL_ClientPrintAll( HUD_PRINTTALK, text );
*/

	UTIL_ClientPrintAll( HUD_PRINTTALK, "%s killed a creep" );
}

#endif // TF_CREEP_MODE

//----------------------------------------------------------------------------------------------------------------
bool CTFBotManager::RemoveBotFromTeamAndKick( int nTeam )
{
	CUtlVector< CTFPlayer* > vecCandidates;

	// Gather potential candidates
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer == NULL )
			continue;

		if ( FNullEnt( pPlayer->edict() ) )
			continue;

		if ( !pPlayer->IsConnected() )
			continue;

		CTFBot* pBot = dynamic_cast<CTFBot*>( pPlayer );
		if ( pBot && pBot->HasAttribute( CTFBot::QUOTA_MANANGED ) )
		{
			if ( pBot->GetTeamNumber() == nTeam )
			{
				vecCandidates.AddToTail( pPlayer );
			}
		}
	}
	
	CTFPlayer *pVictim = NULL;
	if ( vecCandidates.Count() > 0 )
	{
		// first look for bots that are currently dead
		FOR_EACH_VEC( vecCandidates, i )
		{
			CTFPlayer *pPlayer = vecCandidates[i];
			if ( pPlayer && !pPlayer->IsAlive() )
			{
				pVictim = pPlayer;
				break;
			}
		}

		// if we didn't fine one, try to kick anyone on the team
		if ( !pVictim )
		{
			FOR_EACH_VEC( vecCandidates, i )
			{
				CTFPlayer *pPlayer = vecCandidates[i];
				if ( pPlayer )
				{
					pVictim = pPlayer;
					break;
				}
			}
		}
	}

	if ( pVictim )
	{
		if ( pVictim->IsAlive() )
		{
			pVictim->CommitSuicide();
		}
		pVictim->ForceChangeTeam( TEAM_UNASSIGNED ); // skipping TEAM_SPECTATOR because some servers don't allow spectators
		UTIL_KickBotFromTeam( TEAM_UNASSIGNED );
		return true;
	}

	return false;
}

//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::MaintainBotQuota()
{
	if ( TheNavMesh->IsGenerating() )
		return;

	if ( g_fGameOver )
		return;

	// new players can't spawn immediately after the round has been going for some time
	if ( !TFGameRules() )
		return;

	// training mode controls the bots
	if ( TFGameRules()->IsInTraining() )
		return;

	// if it is not time to do anything...
	if ( gpGlobals->curtime < m_flNextPeriodicThink )
		return;

	// think every quarter second
	m_flNextPeriodicThink = gpGlobals->curtime + 0.25f;

	// don't add bots until local player has been registered, to make sure he's player ID #1
	if ( !engine->IsDedicatedServer() )
	{
		CBasePlayer *pPlayer = UTIL_GetListenServerHost();
		if ( !pPlayer )
			return;
	}

	// We want to balance based on who's playing on game teams not necessary who's on team spectator, etc.
	int nConnectedClients = 0;
	int nTFBots = 0;
	int nTFBotsOnGameTeams = 0;
	int nNonTFBotsOnGameTeams = 0;
	int nSpectators = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer == NULL )
			continue;

		if ( FNullEnt( pPlayer->edict() ) )
			continue;

		if ( !pPlayer->IsConnected() )
			continue;

		CTFBot* pBot = dynamic_cast<CTFBot*>( pPlayer );
		if ( pBot && pBot->HasAttribute( CTFBot::QUOTA_MANANGED ) )
		{
			nTFBots++;
			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED || pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				nTFBotsOnGameTeams++;
			}
		}
		else
		{
			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED || pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				nNonTFBotsOnGameTeams++;
			}
			else if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
			{
				nSpectators++;
			}
		}

		nConnectedClients++;
	}

	int desiredBotCount = tf_bot_quota.GetInt();
	int nTotalNonTFBots = nConnectedClients - nTFBots;

	if ( FStrEq( tf_bot_quota_mode.GetString(), "fill" ) )
	{
		desiredBotCount = MAX( 0, desiredBotCount - nNonTFBotsOnGameTeams );
	}
	else if ( FStrEq( tf_bot_quota_mode.GetString(), "match" ) )
	{
		// If bot_quota_mode is 'match', we want the number of bots to be bot_quota * total humans
		desiredBotCount = (int)MAX( 0, tf_bot_quota.GetFloat() * nNonTFBotsOnGameTeams );
	}

	// wait for a player to join, if necessary
	if ( tf_bot_join_after_player.GetBool() )
	{
		if ( ( nNonTFBotsOnGameTeams == 0 ) && ( nSpectators == 0 ) )
		{
			desiredBotCount = 0;
		}
	}

	// if bots will auto-vacate, we need to keep one slot open to allow players to join
	if ( tf_bot_auto_vacate.GetBool() )
	{
		desiredBotCount = MIN( desiredBotCount, gpGlobals->maxClients - nTotalNonTFBots - 1 );
	}
	else
	{
		desiredBotCount = MIN( desiredBotCount, gpGlobals->maxClients - nTotalNonTFBots );
	}

	// add bots if necessary
	if ( desiredBotCount > nTFBotsOnGameTeams )
	{
		// don't try to add a bot if it would unbalance
		if ( !TFGameRules()->WouldChangeUnbalanceTeams( TF_TEAM_BLUE, TEAM_UNASSIGNED ) ||
			 !TFGameRules()->WouldChangeUnbalanceTeams( TF_TEAM_RED, TEAM_UNASSIGNED ) )
		{
			CTFBot *pBot = GetAvailableBotFromPool();
			if ( pBot == NULL )
			{
				pBot = NextBotCreatePlayerBot< CTFBot >( GetRandomBotName() );
			}
			if ( pBot )
			{
				pBot->SetAttribute( CTFBot::QUOTA_MANANGED );

				// join a team before we pick our class, since we use our teammates to decide what class to be
				pBot->HandleCommand_JoinTeam( "auto" );

				pBot->HandleCommand_JoinClass( pBot->GetNextSpawnClassname() );

				// give the bot a proper name
				char name[256];
				CTFBot::DifficultyType skill = pBot->GetDifficulty();
				CreateBotName( pBot->GetTeamNumber(), pBot->GetPlayerClass()->GetClassIndex(), skill, name, sizeof( name ) );
				engine->SetFakeClientConVarValue( pBot->edict(), "name", name );

				// Keep track of any bots we add during a match
				CMatchInfo *pMatchInfo = GTFGCClientSystem()->GetMatch();
				if ( pMatchInfo )
				{
					pMatchInfo->m_nBotsAdded++;
				}
			}
		}
	}
	else if ( desiredBotCount < nTFBotsOnGameTeams )
	{
		// kick a bot to maintain quota
		
		// first remove any unassigned bots
		if ( UTIL_KickBotFromTeam( TEAM_UNASSIGNED ) )
			return;

		int kickTeam;

		CTeam *pRed = GetGlobalTeam( TF_TEAM_RED );
		CTeam *pBlue = GetGlobalTeam( TF_TEAM_BLUE );

		// remove from the team that has more players
		if ( pBlue->GetNumPlayers() > pRed->GetNumPlayers() )
		{
			kickTeam = TF_TEAM_BLUE;
		}
		else if ( pBlue->GetNumPlayers() < pRed->GetNumPlayers() )
		{
			kickTeam = TF_TEAM_RED;
		}
		// remove from the team that's winning
		else if ( pBlue->GetScore() > pRed->GetScore() )
		{
			kickTeam = TF_TEAM_BLUE;
		}
		else if ( pBlue->GetScore() < pRed->GetScore() )
		{
			kickTeam = TF_TEAM_RED;
		}
		else
		{
			// teams and scores are equal, pick a team at random
			kickTeam = (RandomInt( 0, 1 ) == 0) ? TF_TEAM_BLUE : TF_TEAM_RED;
		}

		// attempt to kick a bot from the given team
		if ( UTIL_KickBotFromTeam( kickTeam ) )
			return;

		// if there were no bots on the team, kick a bot from the other team
		UTIL_KickBotFromTeam( kickTeam == TF_TEAM_BLUE ? TF_TEAM_RED : TF_TEAM_BLUE );
	}
}


//----------------------------------------------------------------------------------------------------------------
bool CTFBotManager::IsAllBotTeam( int iTeam )
{
	CTeam *pTeam = GetGlobalTeam( iTeam );
	if ( pTeam == NULL )
	{
		return false;
	}

	// check to see if any players on the team are humans
	for ( int i = 0, n = pTeam->GetNumPlayers(); i < n; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pTeam->GetPlayer( i ) );
		if ( pPlayer == NULL )
		{
			continue;
		}
		if ( pPlayer->IsBot() == false )
		{
			return false;
		}
	}

	// if we made it this far, then they must all be bots!
	if ( pTeam->GetNumPlayers() != 0 )
	{
		return true;
	}

	// okay, this is a bit trickier...
	// if there are no people on this team, then we need to check the "assigned" human team
	return TFGameRules()->GetAssignedHumanTeam() != iTeam;
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::SetIsInOfflinePractice(bool bIsInOfflinePractice)
{
	tf_bot_offline_practice.SetValue( bIsInOfflinePractice ? 1 : 0 );
}


//----------------------------------------------------------------------------------------------------------------
bool CTFBotManager::IsInOfflinePractice() const
{
	return tf_bot_offline_practice.GetInt() != 0;
}


//----------------------------------------------------------------------------------------------------------------
bool CTFBotManager::IsMeleeOnly() const
{
	return tf_bot_melee_only.GetBool();
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::RevertOfflinePracticeConvars()
{
	tf_bot_quota.Revert();
	tf_bot_quota_mode.Revert();
	tf_bot_auto_vacate.Revert();
	tf_bot_difficulty.Revert();
	tf_bot_offline_practice.Revert();
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::LevelShutdown()
{
	m_flNextPeriodicThink = 0.0f;
	if ( IsInOfflinePractice() )
	{
		RevertOfflinePracticeConvars();
		SetIsInOfflinePractice( false );
	}		
}


//----------------------------------------------------------------------------------------------------------------
CTFBot* CTFBotManager::GetAvailableBotFromPool()
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		CTFBot* pBot = dynamic_cast<CTFBot*>(pPlayer);

		if (pBot == NULL)
			continue;

		if ( ( pBot->GetFlags() & FL_FAKECLIENT ) == 0 )
			continue;

		if ( pBot->GetTeamNumber() == TEAM_SPECTATOR || pBot->GetTeamNumber() == TEAM_UNASSIGNED )
		{
			pBot->ClearAttribute( CTFBot::QUOTA_MANANGED );
			return pBot;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::OnForceAddedBots( int iNumAdded )
{
	tf_bot_quota.SetValue( tf_bot_quota.GetInt() + iNumAdded );
	m_flNextPeriodicThink = gpGlobals->curtime + 1.0f;
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::OnForceKickedBots( int iNumKicked )
{
	tf_bot_quota.SetValue( MAX( tf_bot_quota.GetInt() - iNumKicked, 0 ) );
	// allow time for the bots to be kicked
	m_flNextPeriodicThink = gpGlobals->curtime + 2.0f;
}


//----------------------------------------------------------------------------------------------------------------
CTFBotManager &TheTFBots( void )
{
	return static_cast<CTFBotManager&>( TheNextBots() );
}



//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( tf_bot_debug_stuck_log, "Given a server logfile, visually display bot stuck locations.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() < 2 )
	{
		DevMsg( "%s <logfilename>\n", args.Arg(0) );
		return;
	}

	FileHandle_t file = filesystem->Open( args.Arg(1), "r", "GAME" );

	const int maxBufferSize = 1024;
	char buffer[ maxBufferSize ];

	char logMapName[ maxBufferSize ];
	logMapName[0] = '\000';

	TheTFBots().ClearStuckBotData();

	if ( file )
	{
		int line = 0;
		while( !filesystem->EndOfFile( file ) )
		{
			filesystem->ReadLine( buffer, maxBufferSize, file );
			++line;

			strtok( buffer, ":" );
			strtok( NULL, ":" );
			strtok( NULL, ":" );
			char *first = strtok( NULL, " " );

			if ( !first )
				continue;

			if ( !strcmp( first, "Loading" ) )
			{
				// L 08/08/2012 - 15:10:47: Loading map "mvm_coaltown"
				strtok( NULL, " " );
				char *mapname = strtok( NULL, "\"" );

				if ( mapname )
				{
					strcpy( logMapName, mapname );
					Warning( "*** Log file from map '%s'\n", mapname );
				}
			}
			else if ( first[0] == '\"' )
			{
				// might be a player ID

				char *playerClassname = &first[1];

				char *nameEnd = playerClassname;
				while( *nameEnd != '\000' && *nameEnd != '<' )
					++nameEnd;
				*nameEnd = '\000';

				char *botIDString = ++nameEnd;
				char *IDEnd = botIDString;
				while( *IDEnd != '\000' && *IDEnd != '>' )
					++IDEnd;
				*IDEnd = '\000';

				int botID = atoi( botIDString );

				char *second = strtok( NULL, " " );
				if ( second && !strcmp( second, "stuck" ) )
				{
					CStuckBot *stuckBot = TheTFBots().FindOrCreateStuckBot( botID, playerClassname );

					CStuckBotEvent *stuckEvent = new CStuckBotEvent;


					// L 08/08/2012 - 15:15:05: "Scout<53><BOT><Blue>" stuck (position "-180.61 2471.29 216.04") (duration "2.52") L 08/08/2012 - 15:15:05:    path_goal ( "-180.61 2471.29 216.04" )
					strtok( NULL, " (\"" );	// (position

					stuckEvent->m_stuckSpot.x = (float)atof( strtok( NULL, " )\"" ) );
					stuckEvent->m_stuckSpot.y = (float)atof( strtok( NULL, " )\"" ) );
					stuckEvent->m_stuckSpot.z = (float)atof( strtok( NULL, " )\"" ) );

					strtok( NULL, ") (\"" );
					stuckEvent->m_stuckDuration = (float)atof( strtok( NULL, "\"" ) );

					strtok( NULL, ") (\"-L0123456789/:" );	// path_goal

					char *goal = strtok( NULL, ") (\"" );

					if ( goal && strcmp( goal, "NULL" ) )
					{
						stuckEvent->m_isGoalValid = true;

						stuckEvent->m_goalSpot.x = (float)atof( goal );
						stuckEvent->m_goalSpot.y = (float)atof( strtok( NULL, ") (\"" ) );
						stuckEvent->m_goalSpot.z = (float)atof( strtok( NULL, ") (\"" ) );
					}
					else
					{
						stuckEvent->m_isGoalValid = false;
					}

					stuckBot->m_stuckEventVector.AddToTail( stuckEvent );
				}
			}
		}

		filesystem->Close( file );
	}
	else
	{
		Warning( "Can't open file '%s'\n", args.Arg(1) );
	}

	//TheTFBots().DrawStuckBotData();
}


//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( tf_bot_debug_stuck_log_clear, "Clear currently loaded bot stuck data", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheTFBots().ClearStuckBotData();
}


//----------------------------------------------------------------------------------------------------------------
// for parsing and debugging stuck bot server logs
void CTFBotManager::ClearStuckBotData()
{
	m_stuckBotVector.PurgeAndDeleteElements();
}


//----------------------------------------------------------------------------------------------------------------
// for parsing and debugging stuck bot server logs
CStuckBot *CTFBotManager::FindOrCreateStuckBot( int id, const char *playerClass )
{
	for( int i=0; i<m_stuckBotVector.Count(); ++i )
	{
		CStuckBot *stuckBot = m_stuckBotVector[i];

		if ( stuckBot->IsMatch( id, playerClass ) )
		{
			return stuckBot;
		}
	}

	// new instance of a stuck bot
	CStuckBot *newStuckBot = new CStuckBot( id, playerClass );
	m_stuckBotVector.AddToHead( newStuckBot );

	return newStuckBot;
}


//----------------------------------------------------------------------------------------------------------------
void CTFBotManager::DrawStuckBotData( float deltaT )
{
	if ( engine->IsDedicatedServer() )
		return;

	if ( !m_stuckDisplayTimer.IsElapsed() )
		return;

	m_stuckDisplayTimer.Start( deltaT );

	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player == NULL )
		return;

// 	Vector forward;
// 	AngleVectors( player->EyeAngles(), &forward );

	for( int i=0; i<m_stuckBotVector.Count(); ++i )
	{
		for( int j=0; j<m_stuckBotVector[i]->m_stuckEventVector.Count(); ++j )
		{
			m_stuckBotVector[i]->m_stuckEventVector[j]->Draw( deltaT );
		}

		for( int j=0; j<m_stuckBotVector[i]->m_stuckEventVector.Count()-1; ++j )
		{
			NDebugOverlay::HorzArrow( m_stuckBotVector[i]->m_stuckEventVector[j]->m_stuckSpot, 
									  m_stuckBotVector[i]->m_stuckEventVector[j+1]->m_stuckSpot,
									  3, 100, 0, 255, 255, true, deltaT );
		}

		NDebugOverlay::Text( m_stuckBotVector[i]->m_stuckEventVector[0]->m_stuckSpot, CFmtStr( "%s(#%d)", m_stuckBotVector[i]->m_name, m_stuckBotVector[i]->m_id ), false, deltaT );
	}
}


