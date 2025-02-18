//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "../EventLog.h"
#include "team.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "team_control_point_master.h"
#include "team_control_point_round.h"
#include "tf_team.h"
#include "KeyValues.h"

extern ConVar tf_flag_caps_per_round;

class CTFEventLog : public CEventLog
{
private:
	typedef CEventLog BaseClass;

public:
	bool PrintEvent( IGameEvent *event )	// override virtual function
	{
		if ( !PrintTFEvent( event ) ) // allow TF to override logging
		{
			return BaseClass::PrintEvent( event );
		}
		else
		{
			return true;
		}
	}

	bool Init()
	{
		BaseClass::Init();

		ListenForGameEvent( "player_death" );
		ListenForGameEvent( "medic_death" );
		ListenForGameEvent( "player_hurt" );
		ListenForGameEvent( "player_changeclass" );
		ListenForGameEvent( "tf_game_over" );
		ListenForGameEvent( "player_chargedeployed" );

		ListenForGameEvent( "teamplay_flag_event" );
		ListenForGameEvent( "teamplay_capture_blocked" );
		ListenForGameEvent( "teamplay_point_captured" );
		ListenForGameEvent( "teamplay_round_stalemate" );
		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "teamplay_game_over" );

		ListenForGameEvent( "player_builtobject" );
		ListenForGameEvent( "player_carryobject" );
		ListenForGameEvent( "player_dropobject" );
		ListenForGameEvent( "object_removed" );
		ListenForGameEvent( "object_detonated" );
		ListenForGameEvent( "object_destroyed" );
		return true;
	}

protected:

	bool PrintTFEvent( IGameEvent *event )	// print Mod specific logs
	{
		const char *eventName = event->GetName();
	
		if ( !Q_strncmp( eventName, "server_", strlen("server_")) )
		{
			return false; // ignore server_ messages
		}
		
 		if ( !Q_strncmp( eventName, "player_death", Q_strlen( "player_death" ) ) )
 		{
			const int userid = event->GetInt( "userid" );
			CBasePlayer *pPlayer = UTIL_PlayerByUserId( userid );
			if ( !pPlayer )
			{
				return false;
			}

			const int attackerid = event->GetInt( "attacker" );
			const char *weapon = event->GetString( "weapon_logclassname" );
			int iCustomDamage = event->GetInt( "customkill" );
			CBasePlayer *pAttacker = UTIL_PlayerByUserId( attackerid );

			if ( pPlayer == pAttacker )  
			{  
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\" (attacker_position \"%d %d %d\")\n",  
								pPlayer->GetPlayerName(),
								userid,
								pPlayer->GetNetworkIDString(),
								pPlayer->GetTeam()->GetName(),
								weapon,
								(int)pPlayer->GetAbsOrigin().x, 
								(int)pPlayer->GetAbsOrigin().y,
								(int)pPlayer->GetAbsOrigin().z );
			}
			else if ( pAttacker )
			{
 				const char *pszCustom = NULL;
 
 				switch( iCustomDamage )
 				{
				case TF_DMG_CUSTOM_HEADSHOT_DECAPITATION:
 				case TF_DMG_CUSTOM_HEADSHOT:
 					pszCustom = "headshot";
 					break;
 				case TF_DMG_CUSTOM_BACKSTAB:
 					pszCustom = "backstab";
 					break;
 
 				default:
 					break;
 				}

				// is the spy feigning death?
				if ( event->GetInt( "death_flags" ) & TF_DEATH_FEIGN_DEATH )
				{
					pszCustom = "feign_death";
				}
 
 				if ( pszCustom )
 				{
 					UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\" (customkill \"%s\") (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",  
								pAttacker->GetPlayerName(),
								attackerid,
								pAttacker->GetNetworkIDString(),
								pAttacker->GetTeam()->GetName(),
								pPlayer->GetPlayerName(),
								userid,
								pPlayer->GetNetworkIDString(),
								pPlayer->GetTeam()->GetName(),
								weapon,
 								pszCustom,
								(int)pAttacker->GetAbsOrigin().x, 
								(int)pAttacker->GetAbsOrigin().y,
								(int)pAttacker->GetAbsOrigin().z,
								(int)pPlayer->GetAbsOrigin().x, 
								(int)pPlayer->GetAbsOrigin().y,
								(int)pPlayer->GetAbsOrigin().z );
				}
				else
				{  
 					UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\" (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",  
 						pAttacker->GetPlayerName(),
 						attackerid,
 						pAttacker->GetNetworkIDString(),
 						pAttacker->GetTeam()->GetName(),
 						pPlayer->GetPlayerName(),
 						userid,
 						pPlayer->GetNetworkIDString(),
 						pPlayer->GetTeam()->GetName(),
 						weapon,
						(int)pAttacker->GetAbsOrigin().x, 
						(int)pAttacker->GetAbsOrigin().y,
						(int)pAttacker->GetAbsOrigin().z,
						(int)pPlayer->GetAbsOrigin().x, 
						(int)pPlayer->GetAbsOrigin().y,
						(int)pPlayer->GetAbsOrigin().z );
 				}							
			}
			else
			{
				int iDamageBits = event->GetInt( "damagebits" );
				if ( ( iDamageBits & DMG_VEHICLE ) || ( iDamageBits & DMG_NERVEGAS ) )
				{
					const char *pszCustomKill = "train";
					if ( iDamageBits & DMG_NERVEGAS )
					{
						pszCustomKill = "saw";
					}

					// killed by the world
					UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"world\" (customkill \"%s\") (attacker_position \"%d %d %d\")\n",
						pPlayer->GetPlayerName(),
						userid,
						pPlayer->GetNetworkIDString(),
						pPlayer->GetTeam()->GetName(),
						pszCustomKill,
						(int)pPlayer->GetAbsOrigin().x, 
						(int)pPlayer->GetAbsOrigin().y,
						(int)pPlayer->GetAbsOrigin().z );

				}
				else
				{
					// killed by the world
					UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"world\" (attacker_position \"%d %d %d\")\n",
									pPlayer->GetPlayerName(),
									userid,
									pPlayer->GetNetworkIDString(),
									pPlayer->GetTeam()->GetName(),
									(int)pPlayer->GetAbsOrigin().x, 
									(int)pPlayer->GetAbsOrigin().y,
									(int)pPlayer->GetAbsOrigin().z );
				}
			}
 
 			// Assist kill
 			int assistid = event->GetInt( "assister" );
 			CBasePlayer *pAssister = UTIL_PlayerByUserId( assistid );
 
 			if ( pAssister )
 			{
 				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"kill assist\" against \"%s<%i><%s><%s>\" (assister_position \"%d %d %d\") (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",    
 					pAssister->GetPlayerName(),
 					assistid,
 					pAssister->GetNetworkIDString(),
 					pAssister->GetTeam()->GetName(),
 					pPlayer->GetPlayerName(),
 					userid,
 					pPlayer->GetNetworkIDString(),
 					pPlayer->GetTeam()->GetName(),
					(int)pAssister->GetAbsOrigin().x,
					(int)pAssister->GetAbsOrigin().y,
					(int)pAssister->GetAbsOrigin().z,
					pAttacker ? (int)pAttacker->GetAbsOrigin().x : 0,
					pAttacker ? (int)pAttacker->GetAbsOrigin().y : 0,
					pAttacker ? (int)pAttacker->GetAbsOrigin().z : 0,
					(int)pPlayer->GetAbsOrigin().x,
					(int)pPlayer->GetAbsOrigin().y,
					(int)pPlayer->GetAbsOrigin().z );
 			}
 
 			// Domination and Revenge
 			// pAttacker //int attackerid = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
 			// pPlayer //int userid = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
 			// pAssister // assistid
 
 			if ( event->GetInt( "death_flags" ) & TF_DEATH_DOMINATION && pAttacker )
 			{
 				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"domination\" against \"%s<%i><%s><%s>\"\n",  
 					pAttacker->GetPlayerName(),
 					attackerid,
 					pAttacker->GetNetworkIDString(),
 					pAttacker->GetTeam()->GetName(),
 					pPlayer->GetPlayerName(),
 					userid,
 					pPlayer->GetNetworkIDString(),
 					pPlayer->GetTeam()->GetName()
 					);
 			}
 			if ( event->GetInt( "death_flags" ) & TF_DEATH_ASSISTER_DOMINATION  && pAssister )
 			{
 				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"domination\" against \"%s<%i><%s><%s>\" (assist \"1\")\n",  
 					pAssister->GetPlayerName(),
 					assistid,
 					pAssister->GetNetworkIDString(),
 					pAssister->GetTeam()->GetName(),
 					pPlayer->GetPlayerName(),
 					userid,
 					pPlayer->GetNetworkIDString(),
 					pPlayer->GetTeam()->GetName()
 					);
 			}
 			if ( event->GetInt( "death_flags" ) & TF_DEATH_REVENGE && pAttacker ) 
 			{
 				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"revenge\" against \"%s<%i><%s><%s>\"\n",  
 					pAttacker->GetPlayerName(),
 					attackerid,
 					pAttacker->GetNetworkIDString(),
 					pAttacker->GetTeam()->GetName(),
 					pPlayer->GetPlayerName(),
 					userid,
 					pPlayer->GetNetworkIDString(),
 					pPlayer->GetTeam()->GetName()
 					);
 			}
 			if ( event->GetInt( "death_flags" ) & TF_DEATH_ASSISTER_REVENGE && pAssister ) 
 			{
 				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"revenge\" against \"%s<%i><%s><%s>\" (assist \"1\")\n",  
 					pAssister->GetPlayerName(),
 					assistid,
 					pAssister->GetNetworkIDString(),
 					pAssister->GetTeam()->GetName(),
 					pPlayer->GetPlayerName(),
 					userid,
 					pPlayer->GetNetworkIDString(),
 					pPlayer->GetTeam()->GetName()
 					);
 			}
 
			return true;
		}
 		else if ( FStrEq( eventName, "player_changeclass" ) )
 		{
 			const int userid = event->GetInt( "userid" );
 			CBasePlayer *pPlayer = UTIL_PlayerByUserId( userid );
 			if ( !pPlayer )
 			{
 				return false;
 			}
 
 			int iClass = event->GetInt("class");
 
 			if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
 				return true;
 
 			if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_LAST_NORMAL_CLASS  )
 			{
 				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed role to \"%s\"\n",  
 					pPlayer->GetPlayerName(),
 					userid,
 					pPlayer->GetNetworkIDString(),
 					pPlayer->GetTeam()->GetName(),
 					GetPlayerClassData( iClass )->m_szClassName
 					);
 			}
 
 			return true;
 		}
		else if ( FStrEq( eventName, "tf_game_over" ) || FStrEq( eventName, "teamplay_game_over" ) )
		{
			UTIL_LogPrintf( "World triggered \"Game_Over\" reason \"%s\"\n", event->GetString( "reason" ) );
			UTIL_LogPrintf( "Team \"Red\" final score \"%d\" with \"%d\" players\n", GetGlobalTeam( TF_TEAM_RED )->GetScore(), GetGlobalTeam( TF_TEAM_RED )->GetNumPlayers() );
			UTIL_LogPrintf( "Team \"Blue\" final score \"%d\" with \"%d\" players\n", GetGlobalTeam( TF_TEAM_BLUE )->GetScore(), GetGlobalTeam( TF_TEAM_BLUE )->GetNumPlayers() );
 			return true;		
 		}
 		else if ( FStrEq( eventName, "player_chargedeployed" ) )
 		{
 			const int userid = event->GetInt( "userid" );
 			CBasePlayer *pPlayer = UTIL_PlayerByUserId( userid );
 			if ( !pPlayer )
 			{
 				return false;
 			}
 
 			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"chargedeployed\"\n",  
 				pPlayer->GetPlayerName(),
 				userid,
 				pPlayer->GetNetworkIDString(),
 				pPlayer->GetTeam()->GetName()
 				);
 
 			return true;		
 		}
		else if ( FStrEq( eventName, "player_builtobject" ) ||
				  FStrEq( eventName, "player_carryobject" ) ||
				  FStrEq( eventName, "player_dropobject" ) ||
				  FStrEq( eventName, "player_removed" ) ||
				  FStrEq( eventName, "object_detonated" ) )
		{
			const int userid = event->GetInt( "userid" );
			CBasePlayer *pPlayer = UTIL_PlayerByUserId( userid );
			if ( pPlayer )
			{
				// Some events have "object" and some have "objecttype". We can't change them as there are third-party
				// scripts that listen for these events.
				const int objectid = !event->IsEmpty( "objecttype" ) ? event->GetInt( "objecttype" ) : event->GetInt( "object" );
				const CObjectInfo *pInfo = ( objectid >= 0 && objectid < OBJ_LAST ) ? GetObjectInfo( objectid ) : NULL;
				if ( pInfo )
				{
					UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"%s\" (object \"%s\") (position \"%d %d %d\")\n",
						pPlayer->GetPlayerName(),
						userid,
						pPlayer->GetNetworkIDString(),
						pPlayer->GetTeam()->GetName(),
						eventName,
						pInfo->m_pObjectName,
						(int)pPlayer->GetAbsOrigin().x,
						(int)pPlayer->GetAbsOrigin().y,
						(int)pPlayer->GetAbsOrigin().z );
				}
				return true;
			}
			return false;
		}
		else if ( FStrEq( eventName, "object_destroyed" ) )
 		{
 			int objectid = event->GetInt( "objecttype" );
 			const CObjectInfo *pInfo = ( objectid >= 0 && objectid < OBJ_LAST ) ? GetObjectInfo( objectid ) : NULL;
 			if ( !pInfo )
 				return false;
 
 			const int userid = event->GetInt( "userid" );
 			CBasePlayer *pObjectOwner = UTIL_PlayerByUserId( userid );
 			if ( !pObjectOwner )
 				return false;
 
 			const int attackerid = event->GetInt( "attacker" );
 			CBasePlayer *pAttacker = UTIL_PlayerByUserId( attackerid );
 			if ( !pAttacker )
 				return false;
 
 			const char *weapon = event->GetString( "weapon" );
 
 			// log that the person killed an object
 			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"killedobject\" (object \"%s\") (weapon \"%s\") (objectowner \"%s<%i><%s><%s>\") (attacker_position \"%d %d %d\")\n",   
 				pAttacker->GetPlayerName(),
 				attackerid,
 				pAttacker->GetNetworkIDString(),
 				pAttacker->GetTeam()->GetName(),
 				pInfo->m_pObjectName,
 				weapon,
 				pObjectOwner->GetPlayerName(),
 				userid,
 				pObjectOwner->GetNetworkIDString(),
 				pObjectOwner->GetTeam()->GetName(),
				(int)pAttacker->GetAbsOrigin().x, 
				(int)pAttacker->GetAbsOrigin().y,
				(int)pAttacker->GetAbsOrigin().z );
 
 			const int assisterid = event->GetInt( "assister" );
 			CBasePlayer *pAssister = UTIL_PlayerByUserId( assisterid );
 			if ( pAssister )
 			{
 				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"killedobject\" (object \"%s\") (objectowner \"%s<%i><%s><%s>\") (assist \"1\") (assister_position \"%d %d %d\") (attacker_position \"%d %d %d\")\n",   
 					pAssister->GetPlayerName(),
 					assisterid,
 					pAssister->GetNetworkIDString(),
 					pAssister->GetTeam()->GetName(),
 					pInfo->m_pObjectName,
 					pObjectOwner->GetPlayerName(),
 					userid,
 					pObjectOwner->GetNetworkIDString(),
 					pObjectOwner->GetTeam()->GetName(),
					(int)pAssister->GetAbsOrigin().x, 
					(int)pAssister->GetAbsOrigin().y,
					(int)pAssister->GetAbsOrigin().z,
					(int)pAttacker->GetAbsOrigin().x, 
					(int)pAttacker->GetAbsOrigin().y,
					(int)pAttacker->GetAbsOrigin().z );
 			}			
 		}
 		else if ( FStrEq( eventName, "teamplay_flag_event" ) )
 		{	
 			int playerindex = event->GetInt( "player" );
 
 			CBasePlayer *pPlayer = UTIL_PlayerByIndex( playerindex );
 			if ( !pPlayer )
 			{
 				return false;
 			}
 
 			const char *pszEvent = "unknown";	// picked up, dropped, defended, captured
			int iEventType = event->GetInt( "eventtype" );
			bool bPlainLogEntry = true;
 
 			switch ( iEventType )
 			{
 			case TF_FLAGEVENT_PICKUP:
 				pszEvent = "picked up";
 				break;
 			case TF_FLAGEVENT_CAPTURE:
 				pszEvent = "captured";

				if ( tf_flag_caps_per_round.GetInt() > 0 )
				{
					bPlainLogEntry = false;
				}
 				break;
 			case TF_FLAGEVENT_DEFEND:
 				pszEvent = "defended";
 				break;
 			case TF_FLAGEVENT_DROPPED:
 				pszEvent = "dropped";
 				break;
 			}

			if ( bPlainLogEntry )
			{
  				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"flagevent\" (event \"%s\") (position \"%d %d %d\")\n",    
 					pPlayer->GetPlayerName(),
 					pPlayer->GetUserID(),
 					pPlayer->GetNetworkIDString(),
 					pPlayer->GetTeam()->GetName(),
 					pszEvent,
					(int)pPlayer->GetAbsOrigin().x, 
					(int)pPlayer->GetAbsOrigin().y,
					(int)pPlayer->GetAbsOrigin().z );
 			}
			else
			{
				CTFTeam *pTeam = GetGlobalTFTeam( pPlayer->GetTeamNumber() );

				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"flagevent\" (event \"%s\") (team_caps \"%d\") (caps_per_round \"%d\") (position \"%d %d %d\")\n", 
					pPlayer->GetPlayerName(),
					pPlayer->GetUserID(),
					pPlayer->GetNetworkIDString(),
					pPlayer->GetTeam()->GetName(),
					pszEvent,
					pTeam->GetFlagCaptures(),
					tf_flag_caps_per_round.GetInt(),
					(int)pPlayer->GetAbsOrigin().x, 
					(int)pPlayer->GetAbsOrigin().y,
					(int)pPlayer->GetAbsOrigin().z );
			}
	 
 			return true;
 		}
 		else if ( FStrEq( eventName, "teamplay_capture_blocked" ) )
 		{
 			int blockerindex = event->GetInt( "blocker" );
 
 			CBasePlayer *pBlocker = UTIL_PlayerByIndex( blockerindex );
 			if ( !pBlocker )
 			{
				return true;		
			}

 			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"captureblocked\" (cp \"%d\") (cpname \"%s\") (position \"%d %d %d\")\n",   
 				pBlocker->GetPlayerName(),
 				pBlocker->GetUserID(),
 				pBlocker->GetNetworkIDString(),
 				pBlocker->GetTeam()->GetName(),
 				event->GetInt( "cp" ),
 				event->GetString( "cpname" ),
				(int)pBlocker->GetAbsOrigin().x, 
				(int)pBlocker->GetAbsOrigin().y,
				(int)pBlocker->GetAbsOrigin().z );
 		}
 		else if ( FStrEq( eventName, "teamplay_point_captured" ) )
 		{
 			CTeam *pTeam = GetGlobalTeam( event->GetInt( "team" ) );
 
 			if ( !pTeam )
 				return true;
 
 			const char *szCappers = event->GetString( "cappers" );
 
 			int iNumCappers = Q_strlen( szCappers );
 
 			if ( iNumCappers <= 0 )
 				return true;
 
 			char buf[1024];
 
 			Q_snprintf( buf, sizeof(buf), "Team \"%s\" triggered \"pointcaptured\" (cp \"%d\") (cpname \"%s\") (numcappers \"%d\") ",
 				pTeam->GetName(),
 				event->GetInt( "cp" ),
 				event->GetString( "cpname" ),
 				iNumCappers );
 
 			for ( int i=0;i<iNumCappers;i++ )
 			{
 				int iPlayerIndex = szCappers[i];
 
 				Assert( iPlayerIndex != '\0' && iPlayerIndex > 0 && iPlayerIndex <= MAX_PLAYERS );
 
 				CBasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerIndex );
 
 				if ( !pPlayer )
 					continue;
 
 				char playerBuf[256];
 				Q_snprintf( playerBuf, sizeof(playerBuf), "(player%d \"%s<%i><%s><%s>\") (position%d \"%d %d %d\") ", 
					i + 1,
 					pPlayer->GetPlayerName(),
 					pPlayer->GetUserID(),
 					pPlayer->GetNetworkIDString(),
 					pPlayer->GetTeam()->GetName(),
					i + 1,
					(int)pPlayer->GetAbsOrigin().x,
					(int)pPlayer->GetAbsOrigin().y,
					(int)pPlayer->GetAbsOrigin().z );
 
 				Q_strncat( buf, playerBuf, sizeof(buf), COPY_ALL_CHARACTERS );
 			}
 
 			UTIL_LogPrintf( "%s\n", buf );
 		}
		else if ( FStrEq( eventName, "teamplay_round_stalemate" ) )
		{
			int iReason = event->GetInt( "reason" );
			if ( iReason == STALEMATE_TIMER )
			{
				UTIL_LogPrintf( "World triggered \"Round_SuddenDeath\" reason \"round timelimit reached\"\n" );
			}
			else if ( iReason == STALEMATE_SERVER_TIMELIMIT )
			{
				UTIL_LogPrintf( "World triggered \"Round_SuddenDeath\" reason \"server timelimit reached\"\n" );
			}
			else
			{
				UTIL_LogPrintf( "World triggered \"Round_SuddenDeath\"\n" );
			}
			
			return true;
		}
		else if ( FStrEq( eventName, "teamplay_round_win" ) )
		{
			bool bShowScores = true;
			int iTeam = event->GetInt( "team" );
			bool bFullRound = event->GetBool( "full_round" );
			if ( iTeam == TEAM_UNASSIGNED )
			{
				UTIL_LogPrintf( "World triggered \"Round_Stalemate\"\n" );
			}
			else
			{
				const char *pszWinner = "Red";

				if ( iTeam == TF_TEAM_BLUE )
				{
					pszWinner = "Blue";
				}

				CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
				if ( pMaster && pMaster->PlayingMiniRounds() )
				{
					UTIL_LogPrintf( "World triggered \"Mini_Round_Win\" (winner \"%s\") (round \"%s\")\n", pszWinner, pMaster->GetCurrentRound()->GetEntityName().ToCStr() );
					UTIL_LogPrintf( "World triggered \"Mini_Round_Length\" (seconds \"%0.2f\")\n", event->GetFloat( "round_time" ) );
					bShowScores = false;
				}

				if ( bFullRound )
				{
					UTIL_LogPrintf( "World triggered \"Round_Win\" (winner \"%s\")\n", pszWinner );

					if ( !pMaster || !pMaster->PlayingMiniRounds() )
					{
						UTIL_LogPrintf( "World triggered \"Round_Length\" (seconds \"%0.2f\")\n", event->GetFloat( "round_time" ) );
					}

					bShowScores = true;
				}
			}

			if ( bShowScores )
			{
				UTIL_LogPrintf( "Team \"Red\" current score \"%d\" with \"%d\" players\n", GetGlobalTeam( TF_TEAM_RED )->GetScore(), GetGlobalTeam( TF_TEAM_RED )->GetNumPlayers() );
				UTIL_LogPrintf( "Team \"Blue\" current score \"%d\" with \"%d\" players\n", GetGlobalTeam( TF_TEAM_BLUE )->GetScore(), GetGlobalTeam( TF_TEAM_BLUE )->GetNumPlayers() );
			}
		}
		else if ( FStrEq( eventName, "medic_death" ) )
		{
			const int userid = event->GetInt( "userid" );
			CBasePlayer *pPlayer = UTIL_PlayerByUserId( userid );
			if ( !pPlayer )
			{
				return false;
			}

			const int attackerid = event->GetInt( "attacker" );
			CBasePlayer *pAttacker = UTIL_PlayerByUserId( attackerid );

			bool bCharged = event->GetBool( "charged" );
			int iHealing = event->GetInt( "healing" );

			if ( !pAttacker )
			{
				pAttacker = pPlayer;
			}

			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"medic_death\" against \"%s<%i><%s><%s>\" (healing \"%d\") (ubercharge \"%s\")\n",
				pAttacker->GetPlayerName(),
				attackerid,
				pAttacker->GetNetworkIDString(),
				pAttacker->GetTeam()->GetName(),
				pPlayer->GetPlayerName(),
				userid,
				pPlayer->GetNetworkIDString(),
				pPlayer->GetTeam()->GetName(),
				iHealing,
				bCharged ? "1" : "0" );

			return true;
		}

		return false;
	}
};

CTFEventLog g_TFEventLog;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* GameLogSystem()
{
	return &g_TFEventLog;
}

