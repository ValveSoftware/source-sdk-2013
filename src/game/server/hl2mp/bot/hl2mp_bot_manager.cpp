//========= Copyright Valve Corporation, All rights reserved. ============//
//----------------------------------------------------------------------------------------------------------------

#include "cbase.h"
#include "hl2mp_bot_manager.h"

#include "Player/NextBotPlayer.h"
#include "team.h"
#include "hl2mp_bot.h"
#include "hl2mp_gamerules.h"


//----------------------------------------------------------------------------------------------------------------

// Creates and sets CHL2MPBotManager as the NextBotManager singleton
static CHL2MPBotManager sHL2MPBotManager;

ConVar hl2mp_bot_difficulty( "hl2mp_bot_difficulty", "2", FCVAR_NONE, "Defines the skill of bots joining the game.  Values are: 0=easy, 1=normal, 2=hard, 3=expert." );
ConVar hl2mp_bot_quota( "hl2mp_bot_quota", "0", FCVAR_NONE, "Determines the total number of tf bots in the game." );
ConVar hl2mp_bot_quota_mode( "hl2mp_bot_quota_mode", "normal", FCVAR_NONE, "Determines the type of quota.\nAllowed values: 'normal', 'fill', and 'match'.\nIf 'fill', the server will adjust bots to keep N players in the game, where N is bot_quota.\nIf 'match', the server will maintain a 1:N ratio of humans to bots, where N is bot_quota." );
ConVar hl2mp_bot_join_after_player( "hl2mp_bot_join_after_player", "1", FCVAR_NONE, "If nonzero, bots wait until a player joins before entering the game." );
ConVar hl2mp_bot_auto_vacate( "hl2mp_bot_auto_vacate", "1", FCVAR_NONE, "If nonzero, bots will automatically leave to make room for human players." );
ConVar hl2mp_bot_offline_practice( "hl2mp_bot_offline_practice", "0", FCVAR_NONE, "Tells the server that it is in offline practice mode." );
ConVar hl2mp_bot_melee_only( "hl2mp_bot_melee_only", "0", FCVAR_GAMEDLL, "If nonzero, HL2MPBots will only use melee weapons" );
ConVar hl2mp_bot_gravgun_only( "hl2mp_bot_gravgun_only", "0", FCVAR_GAMEDLL, "If nonzero, HL2MPBots will only use gravity gun weapon" );

extern const char *GetRandomBotName( void );
extern void CreateBotName( int iTeam, CHL2MPBot::DifficultyType skill, char* pBuffer, int iBufferSize );

static bool UTIL_KickBotFromTeam( int kickTeam )
{
	int i;

	// try to kick a dead bot first
	for ( i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_PlayerByIndex( i ) );
		CHL2MPBot* pBot = dynamic_cast<CHL2MPBot*>(pPlayer);

		if (pBot == NULL)
			continue;

		if ( pBot->HasAttribute( CHL2MPBot::QUOTA_MANANGED ) == false )
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
		CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_PlayerByIndex( i ) );
		CHL2MPBot* pBot = dynamic_cast<CHL2MPBot*>(pPlayer);

		if (pBot == NULL)
			continue;

		if ( pBot->HasAttribute( CHL2MPBot::QUOTA_MANANGED ) == false )
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

CHL2MPBotManager::CHL2MPBotManager()
	: NextBotManager()
	, m_flNextPeriodicThink( 0 )
{
	NextBotManager::SetInstance( this );
}


//----------------------------------------------------------------------------------------------------------------
CHL2MPBotManager::~CHL2MPBotManager()
{
	NextBotManager::SetInstance( NULL );
}


//----------------------------------------------------------------------------------------------------------------
void CHL2MPBotManager::OnMapLoaded( void )
{
	NextBotManager::OnMapLoaded();

	ClearStuckBotData();
}


//----------------------------------------------------------------------------------------------------------------
void CHL2MPBotManager::Update()
{
	MaintainBotQuota();

	DrawStuckBotData();

	NextBotManager::Update();
}


//----------------------------------------------------------------------------------------------------------------
bool CHL2MPBotManager::RemoveBotFromTeamAndKick( int nTeam )
{
	CUtlVector< CHL2MP_Player* > vecCandidates;

	// Gather potential candidates
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer == NULL )
			continue;

		if ( FNullEnt( pPlayer->edict() ) )
			continue;

		if ( !pPlayer->IsConnected() )
			continue;

		CHL2MPBot* pBot = dynamic_cast<CHL2MPBot*>( pPlayer );
		if ( pBot && pBot->HasAttribute( CHL2MPBot::QUOTA_MANANGED ) )
		{
			if ( pBot->GetTeamNumber() == nTeam )
			{
				vecCandidates.AddToTail( pPlayer );
			}
		}
	}
	
	CHL2MP_Player *pVictim = NULL;
	if ( vecCandidates.Count() > 0 )
	{
		// first look for bots that are currently dead
		FOR_EACH_VEC( vecCandidates, i )
		{
			CHL2MP_Player *pPlayer = vecCandidates[i];
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
				CHL2MP_Player *pPlayer = vecCandidates[i];
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
		UTIL_KickBotFromTeam( TEAM_UNASSIGNED );
		return true;
	}

	return false;
}

//----------------------------------------------------------------------------------------------------------------
void CHL2MPBotManager::MaintainBotQuota()
{
	if ( TheNavMesh->IsGenerating() )
		return;

	if ( g_fGameOver )
		return;

	// new players can't spawn immediately after the round has been going for some time
	if ( !GameRules() )
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
	int nHL2MPBots = 0;
	int nHL2MPBotsOnGameTeams = 0;
	int nNonHL2MPBotsOnGameTeams = 0;
	int nSpectators = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer == NULL )
			continue;

		if ( FNullEnt( pPlayer->edict() ) )
			continue;

		if ( !pPlayer->IsConnected() )
			continue;

		CHL2MPBot* pBot = dynamic_cast<CHL2MPBot*>( pPlayer );
		if ( pBot && pBot->HasAttribute( CHL2MPBot::QUOTA_MANANGED ) )
		{
			nHL2MPBots++;
			if ( pPlayer->GetTeamNumber() == TEAM_REBELS || pPlayer->GetTeamNumber() == TEAM_COMBINE )
			{
				nHL2MPBotsOnGameTeams++;
			}
		}
		else
		{
			if ( pPlayer->GetTeamNumber() == TEAM_REBELS || pPlayer->GetTeamNumber() == TEAM_COMBINE )
			{
				nNonHL2MPBotsOnGameTeams++;
			}
			else if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
			{
				nSpectators++;
			}
		}

		nConnectedClients++;
	}

	int desiredBotCount = hl2mp_bot_quota.GetInt();
	int nTotalNonHL2MPBots = nConnectedClients - nHL2MPBots;

	if ( FStrEq( hl2mp_bot_quota_mode.GetString(), "fill" ) )
	{
		desiredBotCount = MAX( 0, desiredBotCount - nNonHL2MPBotsOnGameTeams );
	}
	else if ( FStrEq( hl2mp_bot_quota_mode.GetString(), "match" ) )
	{
		// If bot_quota_mode is 'match', we want the number of bots to be bot_quota * total humans
		desiredBotCount = (int)MAX( 0, hl2mp_bot_quota.GetFloat() * nNonHL2MPBotsOnGameTeams );
	}

	// wait for a player to join, if necessary
	if ( hl2mp_bot_join_after_player.GetBool() )
	{
		if ( ( nNonHL2MPBotsOnGameTeams == 0 ) && ( nSpectators == 0 ) )
		{
			desiredBotCount = 0;
		}
	}

	// if bots will auto-vacate, we need to keep one slot open to allow players to join
	if ( hl2mp_bot_auto_vacate.GetBool() )
	{
		desiredBotCount = MIN( desiredBotCount, gpGlobals->maxClients - nTotalNonHL2MPBots - 1 );
	}
	else
	{
		desiredBotCount = MIN( desiredBotCount, gpGlobals->maxClients - nTotalNonHL2MPBots );
	}

	// add bots if necessary
	if ( desiredBotCount > nHL2MPBotsOnGameTeams )
	{
		CHL2MPBot *pBot = GetAvailableBotFromPool();
		if ( pBot == NULL )
		{
			pBot = NextBotCreatePlayerBot< CHL2MPBot >( GetRandomBotName() );
		}
		if ( pBot )
		{
			pBot->SetAttribute( CHL2MPBot::QUOTA_MANANGED );

			int iTeam = TEAM_UNASSIGNED;

			if ( HL2MPRules()->IsTeamplay() && iTeam == TEAM_UNASSIGNED )
			{
				CTeam* pRebels = GetGlobalTeam( TEAM_REBELS );
				CTeam* pCombine = GetGlobalTeam( TEAM_COMBINE );

				iTeam = pRebels->GetNumPlayers() < pCombine->GetNumPlayers() ? TEAM_REBELS : TEAM_COMBINE;
			}

			const char* pszModel = "";
			if ( iTeam == TEAM_UNASSIGNED )
			{
				pszModel = g_ppszRandomModels[RandomInt( 0, ARRAYSIZE( g_ppszRandomModels ) )];
			}
			else if ( iTeam == TEAM_COMBINE )
			{
				pszModel = g_ppszRandomCombineModels[RandomInt( 0, ARRAYSIZE( g_ppszRandomCombineModels ) )];
			}
			else
			{
				pszModel = g_ppszRandomCitizenModels[RandomInt( 0, ARRAYSIZE( g_ppszRandomCitizenModels ) )];
			}

			// give the bot a proper name
			char name[256];
			CHL2MPBot::DifficultyType skill = pBot->GetDifficulty();
			CreateBotName( pBot->GetTeamNumber(), skill, name, sizeof( name ) );
			engine->SetFakeClientConVarValue( pBot->edict(), "cl_playermodel", pszModel );
			engine->SetFakeClientConVarValue( pBot->edict(), "name", name );
			pBot->HandleCommand_JoinTeam( iTeam );
			pBot->ChangeTeam( iTeam );
		}
	}
	else if ( desiredBotCount < nHL2MPBotsOnGameTeams )
	{
		// kick a bot to maintain quota
		
		// first remove any unassigned bots
		if ( UTIL_KickBotFromTeam( TEAM_UNASSIGNED ) )
			return;

		int kickTeam;

		CTeam *pRebels = GetGlobalTeam( TEAM_REBELS );
		CTeam *pCombine = GetGlobalTeam( TEAM_COMBINE );

		// remove from the team that has more players
		if ( pCombine->GetNumPlayers() > pRebels->GetNumPlayers() )
		{
			kickTeam = TEAM_COMBINE;
		}
		else if ( pCombine->GetNumPlayers() < pRebels->GetNumPlayers() )
		{
			kickTeam = TEAM_REBELS;
		}
		// remove from the team that's winning
		else if ( pCombine->GetScore() > pRebels->GetScore() )
		{
			kickTeam = TEAM_COMBINE;
		}
		else if ( pCombine->GetScore() < pRebels->GetScore() )
		{
			kickTeam = TEAM_REBELS;
		}
		else
		{
			// teams and scores are equal, pick a team at random
			kickTeam = (RandomInt( 0, 1 ) == 0) ? TEAM_COMBINE : TEAM_REBELS;
		}

		// attempt to kick a bot from the given team
		if ( UTIL_KickBotFromTeam( kickTeam ) )
			return;

		// if there were no bots on the team, kick a bot from the other team
		UTIL_KickBotFromTeam( kickTeam == TEAM_COMBINE ? TEAM_REBELS : TEAM_COMBINE );
	}
}


//----------------------------------------------------------------------------------------------------------------
bool CHL2MPBotManager::IsAllBotTeam( int iTeam )
{
	CTeam *pTeam = GetGlobalTeam( iTeam );
	if ( pTeam == NULL )
	{
		return false;
	}

	// check to see if any players on the team are humans
	for ( int i = 0, n = pTeam->GetNumPlayers(); i < n; ++i )
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer( pTeam->GetPlayer( i ) );
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

	return true;
}


//----------------------------------------------------------------------------------------------------------------
void CHL2MPBotManager::SetIsInOfflinePractice(bool bIsInOfflinePractice)
{
	hl2mp_bot_offline_practice.SetValue( bIsInOfflinePractice ? 1 : 0 );
}


//----------------------------------------------------------------------------------------------------------------
bool CHL2MPBotManager::IsInOfflinePractice() const
{
	return hl2mp_bot_offline_practice.GetInt() != 0;
}


//----------------------------------------------------------------------------------------------------------------
bool CHL2MPBotManager::IsMeleeOnly() const
{
	return hl2mp_bot_melee_only.GetBool();
}


//----------------------------------------------------------------------------------------------------------------
bool CHL2MPBotManager::IsGravGunOnly() const
{
	return hl2mp_bot_gravgun_only.GetBool();
}


//----------------------------------------------------------------------------------------------------------------
void CHL2MPBotManager::RevertOfflinePracticeConvars()
{
	hl2mp_bot_quota.Revert();
	hl2mp_bot_quota_mode.Revert();
	hl2mp_bot_auto_vacate.Revert();
	hl2mp_bot_difficulty.Revert();
	hl2mp_bot_offline_practice.Revert();
}


//----------------------------------------------------------------------------------------------------------------
void CHL2MPBotManager::LevelShutdown()
{
	m_flNextPeriodicThink = 0.0f;
	if ( IsInOfflinePractice() )
	{
		RevertOfflinePracticeConvars();
		SetIsInOfflinePractice( false );
	}		
}


//----------------------------------------------------------------------------------------------------------------
CHL2MPBot* CHL2MPBotManager::GetAvailableBotFromPool()
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_PlayerByIndex( i ) );
		CHL2MPBot* pBot = dynamic_cast<CHL2MPBot*>(pPlayer);

		if (pBot == NULL)
			continue;

		if ( ( pBot->GetFlags() & FL_FAKECLIENT ) == 0 )
			continue;

		if ( pBot->GetTeamNumber() == TEAM_SPECTATOR || pBot->GetTeamNumber() == TEAM_UNASSIGNED )
		{
			pBot->ClearAttribute( CHL2MPBot::QUOTA_MANANGED );
			return pBot;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------------------------------------------
void CHL2MPBotManager::OnForceAddedBots( int iNumAdded )
{
	hl2mp_bot_quota.SetValue( hl2mp_bot_quota.GetInt() + iNumAdded );
	m_flNextPeriodicThink = gpGlobals->curtime + 1.0f;
}


//----------------------------------------------------------------------------------------------------------------
void CHL2MPBotManager::OnForceKickedBots( int iNumKicked )
{
	hl2mp_bot_quota.SetValue( MAX( hl2mp_bot_quota.GetInt() - iNumKicked, 0 ) );
	// allow time for the bots to be kicked
	m_flNextPeriodicThink = gpGlobals->curtime + 2.0f;
}


//----------------------------------------------------------------------------------------------------------------
CHL2MPBotManager &TheHL2MPBots( void )
{
	return static_cast<CHL2MPBotManager&>( TheNextBots() );
}



//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( hl2mp_bot_debug_stuck_log, "Given a server logfile, visually display bot stuck locations.", FCVAR_GAMEDLL | FCVAR_CHEAT )
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

	TheHL2MPBots().ClearStuckBotData();

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
					CStuckBot *stuckBot = TheHL2MPBots().FindOrCreateStuckBot( botID, playerClassname );

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

	//TheHL2MPBots().DrawStuckBotData();
}


//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( hl2mp_bot_debug_stuck_log_clear, "Clear currently loaded bot stuck data", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheHL2MPBots().ClearStuckBotData();
}


//----------------------------------------------------------------------------------------------------------------
// for parsing and debugging stuck bot server logs
void CHL2MPBotManager::ClearStuckBotData()
{
	m_stuckBotVector.PurgeAndDeleteElements();
}


//----------------------------------------------------------------------------------------------------------------
// for parsing and debugging stuck bot server logs
CStuckBot *CHL2MPBotManager::FindOrCreateStuckBot( int id, const char *playerClass )
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
void CHL2MPBotManager::DrawStuckBotData( float deltaT )
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


