//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "KeyValues.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"

#ifdef CLIENT_DLL
	#include "c_baseplayer.h"
	#include "c_team.h"
#else
	#include "player.h"
	#include "game.h"
	#include "gamevars_shared.h"
	#include "team.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
static char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
static int team_scores[MAX_TEAMS];
static int num_teams = 0;

extern bool		g_fGameOver;

REGISTER_GAMERULES_CLASS( CTeamplayRules );

CTeamplayRules::CTeamplayRules()
{
	m_DisableDeathMessages = false;
	m_DisableDeathPenalty = false;
	m_bSwitchTeams = false;
	m_bScrambleTeams = false;

	memset( team_names, 0, sizeof(team_names) );
	memset( team_scores, 0, sizeof(team_scores) );
	num_teams = 0;

	// Copy over the team from the server config
	m_szTeamList[0] = 0;

	RecountTeams();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRules::Precache( void )
{
	// Call the Team Manager's precaches
	for ( int i = 0; i < GetNumberOfTeams(); i++ )
	{
		CTeam *pTeam = GetGlobalTeam( i );
		pTeam->Precache();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRules::Think ( void )
{
	BaseClass::Think();

	///// Check game rules /////

	if ( g_fGameOver )   // someone else quit the game already
	{
		BaseClass::Think();
		return;
	}

	float flTimeLimit = mp_timelimit.GetFloat() * 60;
	
	if ( flTimeLimit != 0 && gpGlobals->curtime >= flTimeLimit )
	{
		ChangeLevel();
		return;
	}

	float flFragLimit = fraglimit.GetFloat();
	if ( flFragLimit )
	{
		// check if any team is over the frag limit
		for ( int i = 0; i < num_teams; i++ )
		{
			if ( team_scores[i] >= flFragLimit )
			{
				ChangeLevel();
				return;
			}
		}
	}
}

//=========================================================
// ClientCommand
// the user has typed a command which is unrecognized by everything else;
// this check to see if the gamerules knows anything about the command
//=========================================================
bool CTeamplayRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	if( BaseClass::ClientCommand( pEdict, args ) )
		return true;
	
	if ( FStrEq( args[0], "menuselect" ) )
	{
		if ( args.ArgC() < 2 )
			return true;

		//int slot = atoi( args[1] );

		// select the item from the current menu

		return true;
	}

	return false;
}

const char *CTeamplayRules::SetDefaultPlayerTeam( CBasePlayer *pPlayer )
{
	// copy out the team name from the model
	int clientIndex = pPlayer->entindex();
	const char *team = (!pPlayer->IsNetClient())?"default":engine->GetClientConVarValue( clientIndex, "cl_team" );

	/* TODO

	pPlayer->SetTeamName( team );

	RecountTeams();

	// update the current player of the team he is joining
	if ( (pPlayer->TeamName())[0] == '\0' || !IsValidTeam( pPlayer->TeamName() ) || defaultteam.GetFloat() )
	{
		const char *pTeamName = NULL;
		
		if ( defaultteam.GetFloat() )
		{
			pTeamName = team_names[0];
		}
		else
		{
			pTeamName = TeamWithFewestPlayers();
		}
		pPlayer->SetTeamName( pTeamName );
 	} */

	return team; //pPlayer->TeamName();
}


//=========================================================
// InitHUD
//=========================================================
void CTeamplayRules::InitHUD( CBasePlayer *pPlayer )
{
	SetDefaultPlayerTeam( pPlayer );
	BaseClass::InitHUD( pPlayer );

	RecountTeams();

	/* TODO this has to be rewritten, maybe add a new USERINFO cvar "team"
	const char *team = engine->GetClientConVarValue( pPlayer->entindex(), "cl_team" );

	// update the current player of the team he is joining
	char text[1024];
	if ( !strcmp( mdls, pPlayer->TeamName() ) )
	{
		Q_snprintf( text,sizeof(text), "You are on team \'%s\'\n", pPlayer->TeamName() );
	}
	else
	{
		Q_snprintf( text,sizeof(text), "You were assigned to team %s\n", pPlayer->TeamName() );
	}

	ChangePlayerTeam( pPlayer, pPlayer->TeamName(), false, false );
	if ( Q_strlen( pPlayer->TeamName() ) > 0 )
	{
		UTIL_SayText( text, pPlayer );
	}
	RecountTeams(); */
}


void CTeamplayRules::ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib )
{
	int damageFlags = DMG_GENERIC;
	// int clientIndex = pPlayer->entindex();

	if ( !bGib )
	{
		damageFlags |= DMG_NEVERGIB;
	}
	else
	{
		damageFlags |= DMG_ALWAYSGIB;
	}


	// copy out the team name from the model
	// pPlayer->SetTeamName( pTeamName );
}

//-----------------------------------------------------------------------------
// Purpose: Player has just left the game
//-----------------------------------------------------------------------------
void CTeamplayRules::ClientDisconnected( edict_t *pClient )
{
	// Msg( "CLIENT DISCONNECTED, REMOVING FROM TEAM.\n" );

	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
	if ( pPlayer )
	{
		pPlayer->SetConnected( PlayerDisconnecting );

		// Remove the player from his team
		if ( pPlayer->GetTeam() )
		{
			pPlayer->ChangeTeam( 0 );
		}
	}

	BaseClass::ClientDisconnected( pClient );
}

//=========================================================
// ClientUserInfoChanged
//=========================================================
void CTeamplayRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	/* TODO: handle skin, model & team changes 

  	char text[1024];

	// skin/color/model changes
	int iTeam = Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_team" ) );
	int iClass = Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_class" ) );

	if ( defaultteam.GetBool() )
	{
		// int clientIndex = pPlayer->entindex();

		// engine->SetClientKeyValue( clientIndex, "model", pPlayer->TeamName() );
		// engine->SetClientKeyValue( clientIndex, "team", pPlayer->TeamName() );
		UTIL_SayText( "Not allowed to change teams in this game!\n", pPlayer );
		return;
	}

	if ( defaultteam.GetFloat() || !IsValidTeam( mdls ) )
	{
		// int clientIndex = pPlayer->entindex();

		// engine->SetClientKeyValue( clientIndex, "model", pPlayer->TeamName() );
		Q_snprintf( text,sizeof(text), "Can't change team to \'%s\'\n", mdls );
		UTIL_SayText( text, pPlayer );
		Q_snprintf( text,sizeof(text), "Server limits teams to \'%s\'\n", m_szTeamList );
		UTIL_SayText( text, pPlayer );
		return;
	}

	ChangePlayerTeam( pPlayer, mdls, true, true );
	// recound stuff
	RecountTeams(); */

	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );

	const char *pszOldName = pPlayer->GetPlayerName();

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strcmp( pszOldName, pszName ) )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
		if ( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			event->SetString( "oldname", pszOldName );
			event->SetString( "newname", pszName );
			gameeventmanager->FireEvent( event );
		}
		
		pPlayer->SetPlayerName( pszName );
	}

	// NVNT see if this user is still or has began using a haptic device
	const char *pszHH = engine->GetClientConVarValue( pPlayer->entindex(), "hap_HasDevice" );
	if(pszHH)
	{
		int iHH = atoi(pszHH);
		pPlayer->SetHaptics(iHH!=0);
	}
}

//=========================================================
// Deathnotice. 
//=========================================================
void CTeamplayRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	if ( m_DisableDeathMessages )
		return;

	CBaseEntity *pKiller = info.GetAttacker();
	if ( pVictim && pKiller && pKiller->IsPlayer() )
	{
		CBasePlayer *pk = (CBasePlayer*)pKiller;

		if ( pk )
		{
			if ( (pk != pVictim) && (PlayerRelationship( pVictim, pk ) == GR_TEAMMATE) )
			{
				IGameEvent * event = gameeventmanager->CreateEvent( "player_death" );
				if ( event )
				{
					event->SetInt("killer", pk->GetUserID() );
					event->SetInt("victim", pVictim->GetUserID() );
					event->SetInt("priority", 7 );	// HLTV event priority, not transmitted
					
					gameeventmanager->FireEvent( event );
				}
				return;
			}
		}
	}

	BaseClass::DeathNotice( pVictim, info );
}

//=========================================================
//=========================================================
void CTeamplayRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	if ( !m_DisableDeathPenalty )
	{
		BaseClass::PlayerKilled( pVictim, info );
		RecountTeams();
	}
}


//=========================================================
// IsTeamplay
//=========================================================
bool CTeamplayRules::IsTeamplay( void )
{
	return true;
}

bool CTeamplayRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info )
{
	if ( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE && !info.IsForceFriendlyFire() )
	{
		// my teammate hit me.
		if ( (friendlyfire.GetInt() == 0) && (pAttacker != pPlayer) )
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return false;
		}
	}

	return BaseClass::FPlayerCanTakeDamage( pPlayer, pAttacker, info );
}

//=========================================================
//=========================================================
int CTeamplayRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() )
		return GR_NOTTEAMMATE;

	if ( (*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp( GetTeamID(pPlayer), GetTeamID(pTarget) ) )
	{
		return GR_TEAMMATE;
	}

	return GR_NOTTEAMMATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pListener - 
//			*pSpeaker - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTeamplayRules::PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker )
{
	return ( PlayerRelationship( pListener, pSpeaker ) == GR_TEAMMATE );
}

//=========================================================
//=========================================================
bool CTeamplayRules::ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target )
{
	// always autoaim, unless target is a teammate
	CBaseEntity *pTgt = CBaseEntity::Instance( target );
	if ( pTgt && pTgt->IsPlayer() )
	{
		if ( PlayerRelationship( pPlayer, pTgt ) == GR_TEAMMATE )
			return false; // don't autoaim at teammates
	}

	return BaseClass::ShouldAutoAim( pPlayer, target );
}

//=========================================================
//=========================================================
int CTeamplayRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if ( !pKilled )
		return 0;

	if ( !pAttacker )
		return 1;

	if ( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return -1;

	return 1;
}

//=========================================================
//=========================================================
const char *CTeamplayRules::GetTeamID( CBaseEntity *pEntity )
{
	if ( pEntity == NULL || pEntity->edict() == NULL )
		return "";

	// return their team name
	return pEntity->TeamID();
}


int CTeamplayRules::GetTeamIndex( const char *pTeamName )
{
	if ( pTeamName && *pTeamName != 0 )
	{
		// try to find existing team
		for ( int tm = 0; tm < num_teams; tm++ )
		{
			if ( !stricmp( team_names[tm], pTeamName ) )
				return tm;
		}
	}
	
	return -1;	// No match
}


const char *CTeamplayRules::GetIndexedTeamName( int teamIndex )
{
	if ( teamIndex < 0 || teamIndex >= num_teams )
		return "";

	return team_names[ teamIndex ];
}


bool CTeamplayRules::IsValidTeam( const char *pTeamName ) 
{
	if ( !m_teamLimit )	// Any team is valid if the teamlist isn't set
		return true;

	return ( GetTeamIndex( pTeamName ) != -1 ) ? true : false;
}

const char *CTeamplayRules::TeamWithFewestPlayers( void )
{
	int i;
	int minPlayers = MAX_TEAMS;
	int teamCount[ MAX_TEAMS ];
	char *pTeamName = NULL;

	memset( teamCount, 0, MAX_TEAMS * sizeof(int) );
	
	// loop through all clients, count number of players on each team
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if ( plr )
		{
			int team = GetTeamIndex( plr->TeamID() );
			if ( team >= 0 )
				teamCount[team] ++;
		}
	}

	// Find team with least players
	for ( i = 0; i < num_teams; i++ )
	{
		if ( teamCount[i] < minPlayers )
		{
			minPlayers = teamCount[i];
			pTeamName = team_names[i];
		}
	}

	return pTeamName;
}


//=========================================================
//=========================================================
void CTeamplayRules::RecountTeams( void )
{
	char	*pName;
	char	teamlist[TEAMPLAY_TEAMLISTLENGTH];

	// loop through all teams, recounting everything
	num_teams = 0;

	// Copy all of the teams from the teamlist
	// make a copy because strtok is destructive
	Q_strncpy( teamlist, m_szTeamList, sizeof(teamlist) );
	pName = teamlist;
	pName = strtok( pName, ";" );
	while ( pName != NULL && *pName )
	{
		if ( GetTeamIndex( pName ) < 0 )
		{
			Q_strncpy( team_names[num_teams], pName, sizeof(team_names[num_teams]));
			num_teams++;
		}
		pName = strtok( NULL, ";" );
	}

	if ( num_teams < 2 )
	{
		num_teams = 0;
		m_teamLimit = false;
	}

	// Sanity check
	memset( team_scores, 0, sizeof(team_scores) );

	// loop through all clients
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = UTIL_PlayerByIndex( i );

		if ( plr )
		{
			const char *pTeamName = plr->TeamID();
			// try add to existing team
			int tm = GetTeamIndex( pTeamName );
			
			if ( tm < 0 ) // no team match found
			{ 
				if ( !m_teamLimit )
				{
					// add to new team
					tm = num_teams;
					num_teams++;
					team_scores[tm] = 0;
					Q_strncpy( team_names[tm], pTeamName, MAX_TEAMNAME_LENGTH );
				}
			}

			if ( tm >= 0 )
			{
				team_scores[tm] += plr->FragCount();
			}
		}
	}
}
#endif // GAME_DLL
