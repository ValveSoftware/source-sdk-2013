//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains an interface to log some events to the Steam Share 
//			timeline via ISteamTimeline 
//
//===========================================================================//

#include "cbase.h"
#include "shareddefs.h"
#include "view.h"
#include "steamshare.h"
#include <vgui/ILocalize.h>
#include <tier1/fmtstr.h>
#include "basecombatweapon_shared.h"
#include "ammodef.h"
#include "c_ai_basenpc.h"

#if !defined( _X360 ) && !defined( NO_STEAM )
#include "steam/isteamtimeline.h"
#include "steam/steam_api.h"
#endif
#if defined ( TF_CLIENT_DLL )
#include "tf_gamerules.h"
#include "passtime_game_events.h"
#include "c_playerresource.h"
#endif // TF_CLIENT_DLL

#define TIMELINE_NO_PRIORITY	50
#define TIMELINE_LOW_PRIORITY	75
#define TIMELINE_MED_PRIORITY	100
#define TIMELINE_HIGH_PRIORITY	125

bool ShouldHaveLocalPlayerPickupTimelineEvents();

extern vgui::ILocalize *g_pVGuiLocalize;

static const char *GetLocalizedTitleString( const char *pszToken, const char *pszArg1 = NULL )
{
	wchar_t wszLocalizedArg1[256];

	const wchar_t *pwszLocalizedArg1 = nullptr;
	if ( pszArg1 )
	{
		pwszLocalizedArg1 = g_pVGuiLocalize->Find( pszArg1 );

		if ( !pwszLocalizedArg1 )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pszArg1, wszLocalizedArg1, sizeof( wszLocalizedArg1 ) );
			pwszLocalizedArg1 = wszLocalizedArg1;
		}
	}

	wchar_t wszLocalizedString[ 256 ];
	const wchar_t *pwszLocalizedString = nullptr;
	if ( pwszLocalizedArg1 )
	{	
		const wchar_t *pwszLocalizedToken = nullptr;
		pwszLocalizedToken = g_pVGuiLocalize->Find( pszToken );
		if ( !pwszLocalizedToken )
			return pszToken;

		g_pVGuiLocalize->ConstructString( wszLocalizedString, sizeof( wszLocalizedString ), pwszLocalizedToken, 1, pwszLocalizedArg1 );
		pwszLocalizedString = wszLocalizedString;
	}
	else
	{
		pwszLocalizedString = g_pVGuiLocalize->Find( pszToken );
		if ( !pwszLocalizedString )
			return pszToken;
	}

	static char szTitleString[ 256 ];
	g_pVGuiLocalize->ConvertUnicodeToANSI( pwszLocalizedString, szTitleString, sizeof( szTitleString ) );
	return szTitleString;
}

#if defined ( TF_CLIENT_DLL )
static const char *GetTimelineClassImage( int nTeam, int nClass )
{
	const char *pszReturnImage = nullptr;

	switch( nClass )
	{
	case TF_CLASS_SCOUT:
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? "scout_blu" : "scout_red";
		break;
	case TF_CLASS_SNIPER:
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? "sniper_blu" : "sniper_red";
		break;
	case TF_CLASS_SOLDIER:
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? "soldier_blu" : "soldier_red";
		break;
	case TF_CLASS_DEMOMAN:
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? "demoman_blu" : "demoman_red";
		break;
	case TF_CLASS_MEDIC:
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? "medic_blu" : "medic_red";
		break;
	case TF_CLASS_HEAVYWEAPONS:
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? "heavy_blu" : "heavy_red";
		break;
	case TF_CLASS_PYRO:
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? "pyro_blu" : "pyro_red";
		break;
	case TF_CLASS_SPY:
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? "spy_blu" : "spy_red";
		break;
	case TF_CLASS_ENGINEER:
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? "engineer_blu" : "engineer_red";
		break;
	default:
		break;
	}

	return pszReturnImage;
}

static const char *GetTimelineObjectImage( int nObjType, int nObjMode, int nTeam, bool bDestroyed )
{
	const char *pszReturnImage = nullptr;

	switch ( nObjType )
	{
	case OBJ_DISPENSER:
	{
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? ( bDestroyed ? "dispenser_blu_destroyed" : "dispenser_blu" ) : ( bDestroyed ? "dispenser_red_destroyed" : "dispenser_red" );
		break;
	}
	case OBJ_TELEPORTER:
	{
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? ( bDestroyed ? "teleporter_blu_destroyed" : ( nObjMode == MODE_TELEPORTER_ENTRANCE ) ? "teleporter_blu_in" : "teleporter_blu_out" ) : ( bDestroyed ? "teleporter_red_destroyed" : ( nObjMode == MODE_TELEPORTER_ENTRANCE ) ? "teleporter_red_in" : "teleporter_red_out" );
		break;
	}
	case OBJ_SENTRYGUN:
	{
		pszReturnImage = ( nTeam == TF_TEAM_BLUE ) ? ( bDestroyed ? "sentry_blu_destroyed" : "sentry_blu" ) : ( bDestroyed ? "sentry_red_destroyed" : "sentry_red" );
		break;
	}
	default:
	{
		break;
	}
	}

	return pszReturnImage;
}
#endif // TF_CLIENT_DLL


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
static CSteamShareSystem g_SteamShareSystem;

IGameSystem *SteamShareSystem()
{
	return &g_SteamShareSystem;
}

#if defined( TF_CLIENT_DLL ) || defined( HL2MP ) || defined( CSTRIKE_DLL ) || defined( DOD_DLL ) || defined( HL1MP_DLL )
#define STEAMSHARE_MULTIPLAYER 1
#endif

bool CSteamShareSystem::Init()
{
#if !defined( NO_STEAM )
	// if we don't have the version of ISteamTimeline we need, don't bother listening for any events
	// and just stub out this whole system.
	if ( !steamapicontext || !SteamTimeline() )
		return true;

	ListenForGameEvent( "gameui_activate" );
	ListenForGameEvent( "gameui_hide" );
	ListenForGameEvent( "server_spawn" );

#if defined( TF_CLIENT_DLL )
	ListenForGameEvent( "localplayer_builtobject" );
	ListenForGameEvent( "object_destroyed" );
	ListenForGameEvent( "object_detonated" );
	ListenForGameEvent( "teamplay_flag_event" );
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( PasstimeGameEvents::Score::s_eventName );
	ListenForGameEvent( PasstimeGameEvents::BallGet::s_eventName );
	ListenForGameEvent( PasstimeGameEvents::PassCaught::s_eventName );
	ListenForGameEvent( PasstimeGameEvents::BallFree::s_eventName );
	ListenForGameEvent( "killed_ball_carrier" );
	ListenForGameEvent( "localplayer_changeteam" );
	ListenForGameEvent( "localplayer_changeclass" );
	ListenForGameEvent( "player_death" );
#elif defined ( STEAMSHARE_MULTIPLAYER )
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "weapon_equipped" );
#else
	ListenForGameEvent( "single_player_death" );
	ListenForGameEvent( "weapon_equipped" );
	ListenForGameEvent( "entity_killed" );
#endif // !TF_CLIENT_DLL
	SteamTimeline()->SetTimelineGameMode( k_ETimelineGameMode_Menus );
#endif // !NO_STEAM

	return true;
}

void CSteamShareSystem::FireGameEvent( IGameEvent *event )
{
#if !defined( NO_STEAM )
	if ( !V_stricmp( event->GetName(), "gameui_activate" ) )
	{
		SteamTimeline()->SetTimelineGameMode( k_ETimelineGameMode_Menus );
	}
	else if ( !V_stricmp( event->GetName(), "gameui_hide" ) )
	{
		SteamTimeline()->SetTimelineGameMode( k_ETimelineGameMode_Playing );
	}
	else if ( !V_stricmp( event->GetName(), "server_spawn" ) )
	{
#if defined( STEAMSHARE_MULTIPLAYER )
		const char *pszMapName = event->GetString( "mapname" );
		if ( pszMapName )
		{
#ifdef TF_CLIENT_DLL
			pszMapName = GetMapDisplayName( pszMapName );
#endif
			SteamTimeline()->SetTimelineStateDescription( pszMapName, 0 );
		}
#else // defined( STEAMSHARE_MULTIPLAYER )
		char buf[ 1024 ];
		engine->GetChapterName( buf, sizeof( buf ) );
		TrimRight( buf ); // why are chapter names left padded?
		const char *pszText = nullptr;
		if ( !V_isempty( buf ) )
		{
			pszText = g_pVGuiLocalize->FindAsUTF8( buf );
		}
		else
		{
			pszText = event->GetString( "mapname" );
		}

		if ( !V_isempty( pszText ) )
		{
			char buf2[ 1024 ];
			if ( pszText[ 0 ] == '"' )
			{
				// If the map name is in quotes like "A RED LETTER DAY", lose the 
				// quotes before we had this off to Steam.
				int len = V_strlen( pszText );
				V_strncpy( buf2, pszText + 1, len - 2 );
			}
			else
			{
				V_strcpy_safe( buf2, pszText );
			}

			// TODO(joe): This is a terrible idea and will mess up UTF-8 text something fierce. 
			// But we only care about English for our Steam Share testing, and the
			// all uppercase localized names look terrible there, so...
			V_strtitlecase( buf2 );

			SteamTimeline()->SetTimelineStateDescription( buf2, 0 );
		}
#endif // !TF_CLIENT_DLL
	}
#if defined( TF_CLIENT_DLL )
	else if ( !V_stricmp( event->GetName(), "localplayer_changeteam" ) )
	{
		const char *pszTeam = "#TF_Spectators";
		const char *pszIcon = "spectator";
		if ( GetLocalPlayerTeam() == TF_TEAM_BLUE )
		{
			pszTeam = "#TF_BlueTeam_Name";
			pszIcon = "team_blu";
		}
		else if ( GetLocalPlayerTeam() == TF_TEAM_RED )
		{
			pszTeam = "#TF_RedTeam_Name";
			pszIcon = "team_red";
		}

		const char *pszTitle = GetLocalizedTitleString( "#TF_Timeline_ChangeTeam", pszTeam );
		SteamTimeline()->AddTimelineEvent( pszIcon, pszTitle, "", TIMELINE_HIGH_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
	}
	else if ( !V_stricmp( event->GetName(), "localplayer_changeclass" ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer && pLocalPlayer->GetPlayerClass() )
		{
			int iClass = pLocalPlayer->GetPlayerClass()->GetClassIndex();
			const char *pszIcon = GetTimelineClassImage( GetLocalPlayerTeam(), iClass );
			if ( pszIcon )
			{
				const char *pszTitle = GetLocalizedTitleString( "#TF_Timeline_ChangeClass", g_aPlayerClassNames[ iClass ] );
				SteamTimeline()->AddTimelineEvent( pszIcon, pszTitle, "", TIMELINE_HIGH_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
			}
		}
	}
	else if ( !V_stricmp( event->GetName(), "localplayer_builtobject" ) )
	{
		int nObjType = event->GetInt( "object" );
		int nObjMode = event->GetInt( "object_mode" ); // teleporter entrance vs. exit
		int nObjTeam = GetLocalPlayerTeam();
		const char *pszIcon = GetTimelineObjectImage( nObjType, nObjMode, nObjTeam, false );
		if ( pszIcon )
		{
			const char *pszTitle = GetLocalizedTitleString( "#TF_Timeline_ObjectBuiltGeneric" );
			C_BaseEntity *pEntity = ClientEntityList().GetBaseEntity( event->GetInt( "index" ) );
			if ( pEntity && pEntity->IsBaseObject() )
			{
				C_BaseObject *pObj = assert_cast<C_BaseObject *>( pEntity );
				pszTitle = GetLocalizedTitleString( "#TF_Timeline_ObjectBuilt", pObj->GetStatusName() );
			}
			SteamTimeline()->AddTimelineEvent( pszIcon, pszTitle, "", TIMELINE_NO_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
		}
	}
	else if ( !V_stricmp( event->GetName(), "object_destroyed" ) )
	{
		int nLocalPlayer = GetLocalPlayerIndex();
		int nKillerID = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		int nOwnerID = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		int nAssisterID = engine->GetPlayerForUserID( event->GetInt( "assister" ) );

		if ( nKillerID == nLocalPlayer || nOwnerID == nLocalPlayer || nAssisterID == nLocalPlayer )
		{
			int nObjType = event->GetInt( "objecttype" );
			int nObjTeam = event->GetInt( "team" );
			const char *pszIcon = GetTimelineObjectImage( nObjType, -1, nObjTeam, true );
			if ( pszIcon )
			{
				const char *pszTitle = "#TF_Timeline_ObjectDestroyedGeneric";
				C_BaseEntity *pObject = ClientEntityList().GetBaseEntity( event->GetInt( "index" ) );
				if ( pObject && pObject->IsBaseObject() )
				{
					C_BaseObject *pObj = assert_cast<C_BaseObject *>( pObject );
					const char *pszTempTitle = "#TF_Timeline_ObjectDestroyed";
					if ( nOwnerID == nLocalPlayer )
					{
						pszTempTitle = "#TF_Timeline_ObjectDestroyedYours";
					}
					pszTitle = GetLocalizedTitleString( pszTempTitle, pObj->GetStatusName() );
				}
				SteamTimeline()->AddTimelineEvent( pszIcon, pszTitle, "", TIMELINE_NO_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
			}
		}
	}
	else if ( !V_stricmp( event->GetName(), "object_detonated" ) )
	{
		int nLocalPlayer = GetLocalPlayerIndex();
		int nOwnerID = engine->GetPlayerForUserID( event->GetInt( "userid" ) );

		if ( nOwnerID == nLocalPlayer )
		{
			int nObjType = event->GetInt( "objecttype" );
			const char *pszIcon = GetTimelineObjectImage( nObjType, -1, GetLocalPlayerTeam(), true );
			if ( pszIcon )
			{
				const char *pszTitle = GetLocalizedTitleString( "#TF_Timeline_ObjectDectonatedGeneric" );
				C_BaseEntity *pKiller = ClientEntityList().GetBaseEntity( event->GetInt( "index" ) );
				if ( pKiller && pKiller->IsBaseObject() )
				{
					C_BaseObject *pObj = assert_cast<C_BaseObject *>( pKiller );
					pszTitle = GetLocalizedTitleString( "#TF_Timeline_ObjectDectonated", pObj->GetStatusName() );
				}
				SteamTimeline()->AddTimelineEvent( pszIcon, pszTitle, "", TIMELINE_NO_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
			}
		}
	}
	else if ( !V_stricmp( event->GetName(), "teamplay_flag_event" ) )
	{
		int nLocalPlayer = GetLocalPlayerIndex();
		int nPlayer = event->GetInt( "player" );
		int nEventType = event->GetInt( "eventtype" );
		int nFlagTeam = event->GetInt( "team" );

		if ( nPlayer == nLocalPlayer )
		{
			const char *pszIcon = nullptr;
			const char *pszTempTitle = nullptr;
			if ( nEventType == TF_FLAGEVENT_CAPTURE )
			{
				if ( nFlagTeam == TEAM_UNASSIGNED )
				{
					// kill notification says the enemy flag was captured
					nFlagTeam = ( GetLocalPlayerTeam() == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;
				}
				pszIcon = ( nFlagTeam == TF_TEAM_BLUE ) ? "briefcase_blu" : "briefcase_red";
				pszTempTitle = "#TF_Timeline_IntelCaptured";
			}
			else if ( nEventType == TF_FLAGEVENT_PICKUP )
			{
				pszIcon = ( nFlagTeam == TEAM_UNASSIGNED ) ? "briefcase" : ( nFlagTeam == TF_TEAM_BLUE ) ? "briefcase_blu" : "briefcase_red";
				pszTempTitle = "#TF_Timeline_IntelPickedUp";
			}
			else if ( nEventType == TF_FLAGEVENT_DEFEND )
			{
				if ( nFlagTeam == TEAM_UNASSIGNED )
				{
					// kill notification says the enemy flag was defended
					nFlagTeam = ( GetLocalPlayerTeam() == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;
				}
				pszIcon = ( nFlagTeam == TF_TEAM_BLUE ) ? "briefcase_blu_destroyed" : "briefcase_red_destroyed";
				pszTempTitle = "#TF_Timeline_IntelCarrier";
			}
			else if ( nEventType == TF_FLAGEVENT_DROPPED )
			{
				if ( nFlagTeam == TEAM_UNASSIGNED )
				{
					// kill notification says the enemy flag was dropped
					nFlagTeam = ( GetLocalPlayerTeam() == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;
				}
				pszIcon = ( nFlagTeam == TF_TEAM_BLUE ) ? "briefcase_blu_destroyed" : "briefcase_red_destroyed";
				pszTempTitle = "#TF_Timeline_IntelDropped";
			}

			if ( pszIcon && pszTempTitle )
			{
				const char *pszTitle = GetLocalizedTitleString( pszTempTitle );
				SteamTimeline()->AddTimelineEvent( pszIcon, pszTitle, "", TIMELINE_LOW_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
			}
		}
	}
	else if ( !V_stricmp( event->GetName(), "teamplay_point_captured" ) )
	{
		int nLocalPlayer = GetLocalPlayerIndex();
		const char *szCappers = event->GetString( "cappers" );
		int nTeam = event->GetInt( "team" );

		int nNumCappers = Q_strlen( szCappers );
		if ( nNumCappers > 0 )
		{
			for ( int i = 0; i < nNumCappers; i++ )
			{
				int nPlayerID = szCappers[ i ];

				Assert( nPlayerID != '\0' && nPlayerID > 0 && nPlayerID <= MAX_PLAYERS );

				if ( nPlayerID == nLocalPlayer )
				{
					if ( TFGameRules() )
					{
						const char *pszTitle = GetLocalizedTitleString( "#TF_Timeline_CPCaptured" );
						if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
						{
							SteamTimeline()->AddTimelineEvent( ( nTeam == TF_TEAM_BLUE ) ? "control_point_blu" : "control_point_red", pszTitle, "", TIMELINE_LOW_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
						}
						else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
						{
							SteamTimeline()->AddTimelineEvent( ( nTeam == TF_TEAM_BLUE ) ? "payload_blu" : "payload_red", pszTitle, "", TIMELINE_LOW_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
						}
						break;
					}
				}
			}
		}
	}
	else if ( FStrEq( PasstimeGameEvents::Score::s_eventName, event->GetName() ) )
	{
		PasstimeGameEvents::Score ev( event );

		int nLocalPlayer = GetLocalPlayerIndex();
		int nScorer = ev.scorerIndex;
		int nAssister = ev.assisterIndex;

		if ( nScorer == nLocalPlayer || nAssister == nLocalPlayer )
		{
			const char *pszTitle = GetLocalizedTitleString( "#TF_Timeline_JackScore" );
			SteamTimeline()->AddTimelineEvent( ( GetLocalPlayerTeam() == TF_TEAM_BLUE ) ? "passtime_blu" : "passtime_red", pszTitle, "", TIMELINE_LOW_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
		}
	}
	else if ( FStrEq( PasstimeGameEvents::BallGet::s_eventName, event->GetName() ) )
	{
		PasstimeGameEvents::BallGet ev( event );

		int nLocalPlayer = GetLocalPlayerIndex();
		int nOwner = ev.ownerIndex;
		int nTeam = ev.team;

		if ( nOwner == nLocalPlayer )
		{
			const char *pszTitle = GetLocalizedTitleString( "#TF_Timeline_JackPickedUp" );
			SteamTimeline()->AddTimelineEvent( ( nTeam == TEAM_UNASSIGNED ) ? "passtime" : ( nTeam == TF_TEAM_BLUE ) ? "passtime_blu" : "passtime_red", pszTitle, "", TIMELINE_LOW_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
		}
	}
	else if ( FStrEq( PasstimeGameEvents::PassCaught::s_eventName, event->GetName() ) )
	{
		PasstimeGameEvents::PassCaught ev( event );

		if ( g_PR )
		{
			int nLocalPlayer = GetLocalPlayerIndex();
			int nCatcher = ev.catcherIndex;
			int nCatcherTeam = g_PR->GetTeam( nCatcher );
			int nPasserTeam = g_PR->GetTeam( ev.passerIndex );

			if ( nCatcher == nLocalPlayer )
			{
				const char *pszTitle = nullptr;
				if ( nPasserTeam == nCatcherTeam )
				{
					pszTitle = GetLocalizedTitleString( "#TF_Timeline_JackPass" );
				}
				else
				{
					pszTitle = GetLocalizedTitleString( "#TF_Timeline_JackIntercepted" );
				}
				SteamTimeline()->AddTimelineEvent( ( nCatcherTeam == TF_TEAM_BLUE ) ? "passtime_blu" : "passtime_red", pszTitle, "", TIMELINE_LOW_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
			}
		}
	}
	else if ( FStrEq( PasstimeGameEvents::BallFree::s_eventName, event->GetName() ) )
	{
		PasstimeGameEvents::BallFree ev( event );

		int nLocalPlayer = GetLocalPlayerIndex();
		int nOwner = ev.ownerIndex;

		if ( nOwner == nLocalPlayer )
		{
			const char *pszTitle = GetLocalizedTitleString( "#TF_Timeline_JackLost" );
			SteamTimeline()->AddTimelineEvent( ( GetLocalPlayerTeam() == TF_TEAM_BLUE ) ? "passtime_blu_destroyed" : "passtime_red_destroyed", pszTitle, "", TIMELINE_LOW_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
		}
	}
	else if ( !V_stricmp( event->GetName(), "killed_ball_carrier" ) )
	{
		int nLocalPlayer = GetLocalPlayerIndex();
		int nAttacker = event->GetInt( "attacker" );
		int nAssister = event->GetInt( "assister" );

		if ( nAttacker == nLocalPlayer || nAssister == nLocalPlayer )
		{
			const char *pszTitle = GetLocalizedTitleString( "#TF_Timeline_JackCarrier" );
			SteamTimeline()->AddTimelineEvent( ( GetLocalPlayerTeam() == TF_TEAM_BLUE ) ? "passtime_red_destroyed" : "passtime_blu_destroyed", pszTitle, "", TIMELINE_LOW_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
		}
	}
#endif

#if defined( TF_CLIENT_DLL )
	else if ( !V_stricmp( event->GetName(), "player_death" ) )
	{
		int nLocalPlayer = GetLocalPlayerIndex();
		int nAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		int nAssister = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		int nVictim = event->GetInt( "victim_entindex" );

		if ( ( nAttacker == nLocalPlayer || nAssister == nLocalPlayer ) && ( nVictim != nLocalPlayer ) )
		{
			C_TFPlayer *pVictim = ToTFPlayer( ClientEntityList().GetEnt( nVictim ) );
			if ( pVictim && pVictim->GetPlayerClass() )
			{
				int iVictimClassIndex = pVictim->GetPlayerClass()->GetClassIndex();
				const char *pszIcon = GetTimelineClassImage( pVictim->GetTeamNumber(), iVictimClassIndex );
				if ( pszIcon )
				{
					const char *pszTitle = GetLocalizedTitleString( ( iVictimClassIndex == TF_CLASS_ENGINEER ) ? "#TF_Timeline_KilledEngy" : "#TF_Timeline_Killed", g_aPlayerClassNames[ iVictimClassIndex ] );
					SteamTimeline()->AddTimelineEvent( pszIcon, pszTitle, "", TIMELINE_NO_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
				}
			}
		}
		else if ( nVictim == nLocalPlayer )
		{
			const char *pszTitle = "";
			C_TFPlayer *pAttacker = ToTFPlayer( ClientEntityList().GetEnt( nAttacker ) );
			if ( pAttacker && pAttacker->GetPlayerClass() )
			{
				int iAttackerClassIndex = pAttacker->GetPlayerClass()->GetClassIndex();
				const char *pszTempTitle = ( iAttackerClassIndex == TF_CLASS_ENGINEER ) ? "#TF_Timeline_WereKilledEngy" : "#TF_Timeline_WereKilled";
				if ( nAttacker == nLocalPlayer )
				{
					pszTempTitle = "#TF_Timeline_Suicide";
				}
				pszTitle = GetLocalizedTitleString( pszTempTitle, g_aPlayerClassNames[ iAttackerClassIndex ] );
			}
			else if ( nAttacker == 0 ) // world
			{
				pszTitle = GetLocalizedTitleString( "#TF_Timeline_Suicide" );
			}
			SteamTimeline()->AddTimelineEvent( "death", pszTitle, "", TIMELINE_MED_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Standard );
		}
	}
#elif defined ( STEAMSHARE_MULTIPLAYER )
	else if ( !V_stricmp( event->GetName(), "player_death" ) )
	{
		int nLocalPlayer = GetLocalPlayerIndex();
		int nAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		int nAssister = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		int nVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );

		if ( ( nAttacker == nLocalPlayer || nAssister == nLocalPlayer ) && ( nVictim != nLocalPlayer ) )
		{
			C_BasePlayer *pVictim = ToBasePlayer( ClientEntityList().GetEnt( nVictim ) );
			if ( pVictim )
			{
				const char *pszTitle = GetLocalizedTitleString( "#Valve_Timeline_Killed", pVictim->GetPlayerName() );
				SteamTimeline()->AddTimelineEvent( "steam_attack", pszTitle, "", TIMELINE_LOW_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Featured );
			}
		}
		else if ( nVictim == nLocalPlayer )
		{
			const char *pszTitle = "";
			C_BasePlayer *pAttacker = ToBasePlayer( ClientEntityList().GetEnt( nAttacker ) );
			if ( pAttacker )
			{
				const char *pszTempTitle = "#Valve_Timeline_WereKilled";
				if ( nAttacker == nLocalPlayer )
				{
					pszTempTitle = "#Valve_Timeline_Suicide";
				}
				pszTitle = GetLocalizedTitleString( pszTempTitle, pAttacker->GetPlayerName() );
			}
			else if ( nAttacker == 0 ) // world
			{
				pszTitle = GetLocalizedTitleString( "#Valve_Timeline_Suicide" );
			}
			SteamTimeline()->AddTimelineEvent( "steam_death", pszTitle, "", TIMELINE_MED_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Featured );
		}
	}
	else if ( !V_stricmp( event->GetName(), "weapon_equipped" ) )
	{
		int nWeaponEntindex = event->GetInt( "entindex" );
		int nOwnerEntIndex = event->GetInt( "owner_entindex" );
		C_BaseCombatWeapon* pWeapon = dynamic_cast<C_BaseCombatWeapon *>( ClientEntityList().GetEnt( nWeaponEntindex ) );
		C_BasePlayer* pOwner = dynamic_cast<C_BasePlayer *>( ClientEntityList().GetEnt( nOwnerEntIndex ) );
		if ( pWeapon && pOwner && pOwner == C_BasePlayer::GetLocalPlayer() && ShouldHaveLocalPlayerPickupTimelineEvents() )
		{
			const char* pchWeaponName = "weapon";
			const char* pchIcon = "steam_combat";
			if ( pWeapon )
			{
				const char* pchActualName = g_pVGuiLocalize->FindAsUTF8( pWeapon->GetPrintName() );
				if ( pchActualName )
				{
					pchWeaponName = pchActualName;
				}

				const char* pszTitle = GetLocalizedTitleString( "#Valve_Timeline_Pickup", pchWeaponName );

				SteamTimeline()->AddTimelineEvent( pchIcon, pszTitle, nullptr, TIMELINE_NO_PRIORITY, 0, 0, k_ETimelineEventClipPriority_Standard );
			}
		}
	}
#else
	else if ( !V_stricmp( event->GetName(), "single_player_death" ) )
	{
		SteamTimeline()->AddTimelineEvent( "steam_death", "Player was killed", nullptr, 40, 0, 0, k_ETimelineEventClipPriority_None );
	}
	else if ( !V_stricmp( event->GetName(), "weapon_equipped" ) )
	{
		int nWeaponEntindex = event->GetInt( "entindex" );
		IClientEntity *pWeaponEntity = cl_entitylist->GetBaseEntity( nWeaponEntindex );
		if ( pWeaponEntity )
		{
			C_BaseCombatWeapon *pWeapon = dynamic_cast<C_BaseCombatWeapon *>( pWeaponEntity );
			const char *pchWeaponName = "weapon";
			const char *pchIcon = "steam_combat";
			if ( pWeapon )
			{
				const char *pchActualName = g_pVGuiLocalize->FindAsUTF8( pWeapon->GetPrintName() );
				if ( pchActualName )
				{
					pchWeaponName = pchActualName;
				}

				if ( !V_stricmp( "weapon_crowbar", pWeapon->GetName() ) )
				{
					pchIcon = "weapon_crowbar";
				}
				else if ( !V_stricmp( "weapon_pistol", pWeapon->GetName() ) )
				{
					pchIcon = "weapon_pistol";
				}
				else if ( !V_stricmp( "weapon_smg1", pWeapon->GetName() ) || !V_stricmp( "weapon_smg2", pWeapon->GetName() ) )
				{
					pchIcon = "weapon_smg";
				}
			}

			SteamTimeline()->AddTimelineEvent( pchIcon, CFmtStr( "Picked up %s", pchWeaponName ), nullptr, 50, 0, 0, k_ETimelineEventClipPriority_Standard );
		}
	}
	else if ( !V_stricmp( event->GetName(), "entity_killed" ) )
	{
		C_AI_BaseNPC *pKilledEntity = dynamic_cast<C_AI_BaseNPC *>( cl_entitylist->GetBaseEntity( event->GetInt( "entindex_killed", 0 ) ) );
		if ( pKilledEntity )
		{
			const char *pchClassName = pKilledEntity->GetClassname();
			if ( !V_stricmp( "class C_BaseHelicopter", pchClassName ) )
			{
				SteamTimeline()->AddTimelineEvent( "enemy_chopper", "Killed attack chopper", nullptr, 200, -30.f, 35.f, k_ETimelineEventClipPriority_Featured );
			}

			C_BaseEntity *pInflictor = cl_entitylist->GetBaseEntity( event->GetInt( "entindex_inflictor", 0 ) );

			const char *pchModelName = pInflictor ? modelinfo->GetModelName( modelinfo->GetModel( pInflictor->GetModelIndex() ) ) : "";
			if ( nullptr != V_stristr_fast( pchModelName, "sawblade" ) )
			{
				SteamTimeline()->AddTimelineEvent( "sawblade_kill", "Chopped something up with a sawblade", nullptr, 50, 0, 0, k_ETimelineEventClipPriority_None );
			}
		}
	}
#endif // !TF_CLIENT_DLL
#endif // !NO_STEAM
}
