//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot.cpp
// Team Fortress NextBot
// Michael Booth, February 2009

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_obj_sentrygun.h"
#include "team_control_point_master.h"
#include "tf_weapon_pipebomblauncher.h"
#include "team_train_watcher.h"
#include "tf_bot.h"
#include "tf_bot_manager.h"
#include "tf_bot_vision.h"
#include "tf_team.h"
#include "bot/map_entities/tf_bot_generator.h"
#include "trigger_area_capture.h"
#include "GameEventListener.h"
#include "NextBotUtil.h"
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"
#include "econ_item_system.h"
#include "bot/behavior/tf_bot_use_item.h"
#include "tf_wearable_weapons.h"
#include "tf_weapon_buff_item.h"
#include "tf_weapon_lunchbox.h"
#include "tf_weapon_medigun.h"
#include "func_respawnroom.h"
#include "soundenvelope.h"

#include "econ_entity_creation.h"

#include "player_vs_environment/tf_population_manager.h"

#include "bot/behavior/tf_bot_behavior.h"
#include "bot/map_entities/tf_bot_generator.h"
#include "bot/map_entities/tf_bot_hint_entity.h"

ConVar tf_bot_force_class( "tf_bot_force_class", "", FCVAR_GAMEDLL, "If set to a class name, all TFBots will respawn as that class" );

ConVar tf_bot_notice_gunfire_range( "tf_bot_notice_gunfire_range", "3000", FCVAR_GAMEDLL );
ConVar tf_bot_notice_quiet_gunfire_range( "tf_bot_notice_quiet_gunfire_range", "500", FCVAR_GAMEDLL );
ConVar tf_bot_sniper_personal_space_range( "tf_bot_sniper_personal_space_range", "1000", FCVAR_CHEAT, "Enemies beyond this range don't worry the Sniper" );
ConVar tf_bot_pyro_deflect_tolerance( "tf_bot_pyro_deflect_tolerance", "0.5", FCVAR_CHEAT );
ConVar tf_bot_keep_class_after_death( "tf_bot_keep_class_after_death", "0", FCVAR_GAMEDLL );
ConVar tf_bot_prefix_name_with_difficulty( "tf_bot_prefix_name_with_difficulty", "0", FCVAR_GAMEDLL, "Append the skill level of the bot to the bot's name" );
ConVar tf_bot_near_point_travel_distance( "tf_bot_near_point_travel_distance", "750", FCVAR_CHEAT, "If within this travel distance to the current point, bot is 'near' it" );
ConVar tf_bot_pyro_shove_away_range( "tf_bot_pyro_shove_away_range", "250", FCVAR_CHEAT, "If a Pyro bot's target is closer than this, compression blast them away" );
ConVar tf_bot_pyro_always_reflect( "tf_bot_pyro_always_reflect", "0", FCVAR_CHEAT, "Pyro bots will always reflect projectiles fired at them. For tesing/debugging purposes." );

ConVar tf_bot_sniper_spot_min_range( "tf_bot_sniper_spot_min_range", "1000", FCVAR_CHEAT );
ConVar tf_bot_sniper_spot_max_count( "tf_bot_sniper_spot_max_count", "10", FCVAR_CHEAT, "Stop searching for sniper spots when each side has found this many" );
ConVar tf_bot_sniper_spot_search_count( "tf_bot_sniper_spot_search_count", "10", FCVAR_CHEAT, "Search this many times per behavior update frame" );
ConVar tf_bot_sniper_spot_point_tolerance( "tf_bot_sniper_spot_point_tolerance", "750", FCVAR_CHEAT );
ConVar tf_bot_sniper_spot_epsilon( "tf_bot_sniper_spot_epsilon", "100", FCVAR_CHEAT );

ConVar tf_bot_sniper_goal_entity_move_tolerance( "tf_bot_sniper_goal_entity_move_tolerance", "500", FCVAR_CHEAT );

ConVar tf_bot_suspect_spy_touch_interval( "tf_bot_suspect_spy_touch_interval", "5", FCVAR_CHEAT, "How many seconds back to look for touches against suspicious spies" );
ConVar tf_bot_suspect_spy_forget_cooldown( "tf_bot_suspect_spy_forget_cooldown", "5", FCVAR_CHEAT, "How long to consider a suspicious spy as suspicious" );

ConVar tf_bot_debug_tags( "tf_bot_debug_tags", "0", FCVAR_CHEAT, "ent_text will only show tags on bots" );

ConVar tf_bot_spawn_use_preset_roster( "tf_bot_spawn_use_preset_roster", "1", FCVAR_CHEAT, "Bot will choose class from a preset class table." );

extern ConVar tf_bot_sniper_spot_max_count;
extern ConVar tf_bot_fire_weapon_min_time;
extern ConVar tf_bot_sniper_misfire_chance;
extern ConVar tf_bot_difficulty;
extern ConVar tf_bot_farthest_visible_theater_sample_count;
extern ConVar tf_bot_sniper_spot_min_range;
extern ConVar tf_bot_sniper_spot_epsilon;
extern ConVar tf_mvm_miniboss_min_health;
extern ConVar tf_bot_path_lookahead_range;

extern ConVar tf_mvm_miniboss_scale;


//-----------------------------------------------------------------------------------------------------
bool IsPlayerClassname( const char *string )
{
	for ( int i = TF_CLASS_SCOUT; i < TF_CLASS_COUNT_ALL; ++i )
	{
		if ( !stricmp( string, GetPlayerClassData( i )->m_szClassName ) )
		{
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
bool IsTeamName( const char *string )
{
	if ( !stricmp( string, "red" ) )
		return true;

	if ( !stricmp( string, "blue" ) )
		return true;

	return false;
}


//-----------------------------------------------------------------------------------------------------
CTFBot::DifficultyType StringToDifficultyLevel( const char *string )
{
	if ( !stricmp( string, "easy" ) )
		return CTFBot::EASY;

	if ( !stricmp( string, "normal" ) )
		return CTFBot::NORMAL;

	if ( !stricmp( string, "hard" ) )
		return CTFBot::HARD;

	if ( !stricmp( string, "expert" ) )
		return CTFBot::EXPERT;

	return CTFBot::UNDEFINED;
}


//-----------------------------------------------------------------------------------------------------
const char *DifficultyLevelToString( CTFBot::DifficultyType skill )
{
	switch( skill )
	{
	case CTFBot::EASY:		return "Easy ";
	case CTFBot::NORMAL:	return "Normal ";
	case CTFBot::HARD:		return "Hard ";
	case CTFBot::EXPERT:	return "Expert ";
	}

	return "Undefined ";
}


//-----------------------------------------------------------------------------------------------------
const char *GetRandomBotName( void )
{
	static const char *nameList[] =
	{
		"Chucklenuts",
		"CryBaby",
		"WITCH",
		"ThatGuy",
		"Still Alive",
		"Hat-Wearing MAN",
		"Me",
		"Numnutz",
		"H@XX0RZ",
		"The G-Man",
		"Chell",
		"The Combine",
		"Totally Not A Bot",
		"Pow!",
		"Zepheniah Mann",
		"THEM",
		"LOS LOS LOS",
		"10001011101",
		"DeadHead",
		"ZAWMBEEZ",
		"MindlessElectrons",
		"TAAAAANK!",
		"The Freeman",
		"Black Mesa",
		"Soulless",
		"CEDA",
		"BeepBeepBoop",
		"NotMe",
		"CreditToTeam",
		"BoomerBile",
		"Someone Else",
		"Mann Co.",
		"Dog",
		"Kaboom!",
		"AmNot",
		"0xDEADBEEF",
		"HI THERE",
		"SomeDude",
		"GLaDOS",
		"Hostage",
		"Headful of Eyeballs",
		"CrySomeMore",
		"Aperture Science Prototype XR7",
		"Humans Are Weak",
		"AimBot",
		"C++",
		"GutsAndGlory!",
		"Nobody",
		"Saxton Hale",
		"RageQuit",
		"Screamin' Eagles",

		"Ze Ubermensch",
		"Maggot",
		"CRITRAWKETS",
		"Herr Doktor",
		"Gentlemanne of Leisure",
		"Companion Cube",
		"Target Practice",
		"One-Man Cheeseburger Apocalypse",
		"Crowbar",
		"Delicious Cake",
		"IvanTheSpaceBiker",
		"I LIVE!",
		"Cannon Fodder",

		"trigger_hurt",
		"Nom Nom Nom",
		"Divide by Zero",
		"GENTLE MANNE of LEISURE",
		"MoreGun",
		"Tiny Baby Man",
		"Big Mean Muther Hubbard",
		"Force of Nature",

		"Crazed Gunman",
		"Grim Bloody Fable",
		"Poopy Joe",
		"A Professional With Standards",
		"Freakin' Unbelievable",
		"SMELLY UNFORTUNATE",
		"The Administrator",
		"Mentlegen",

		"Archimedes!",
		"Ribs Grow Back",
		"It's Filthy in There!",
		"Mega Baboon",
		"Kill Me",
		"Glorified Toaster with Legs",

		NULL
	};
	static int nameCount = 0;
	static int nameIndex = 0;

	if ( nameCount == 0 )
	{
		for( ; nameList[ nameCount ]; ++nameCount );

		// randomize the initial index
		nameIndex = RandomInt( 0, nameCount-1 );
	}

	const char *name = nameList[ nameIndex++ ];

	if ( nameIndex >= nameCount )
		nameIndex = 0;

	return name;
}


//-----------------------------------------------------------------------------------------------------
void CreateBotName( int iTeam, int iClassIndex, CTFBot::DifficultyType skill, char* pBuffer, int iBufferSize )
{
	char szBotNameBuffer[256];
	char szEnemyOrFriendlyString[256];

	const char *pBotName = "";
	const char *pFriendlyOrEnemyTitle = "";

	// @note (Tom Bui): it is okay to get localized name in training, since we should be on a listen server
	if ( TFGameRules()->IsInTraining() )
	{
		// get the friendly/enemy title
		const char *pBotTitle = NULL;
		if ( iTeam != TEAM_UNASSIGNED )
		{
			int iHumanTeam = TFGameRules()->GetAssignedHumanTeam();
			if ( iHumanTeam != TEAM_ANY )
			{
				if ( iHumanTeam == iTeam )
				{
					pBotTitle = "#TF_Bot_Title_Friendly";
				}
				else
				{
					pBotTitle = "#TF_Bot_Title_Enemy";
				}
			}
		}
		wchar_t *pLocalizedTitle = pBotTitle ? g_pVGuiLocalize->Find( pBotTitle ) : NULL;
		if ( pLocalizedTitle )
		{
			g_pVGuiLocalize->ConvertUnicodeToANSI( pLocalizedTitle, szEnemyOrFriendlyString, sizeof( szEnemyOrFriendlyString ) );
			pFriendlyOrEnemyTitle = szEnemyOrFriendlyString;
		}

		// get the class name
		wchar_t *pLocalizedName = NULL;
		if ( iClassIndex >= TF_FIRST_NORMAL_CLASS && iClassIndex < TF_LAST_NORMAL_CLASS )
		{
			pLocalizedName = g_pVGuiLocalize->Find( g_aPlayerClassNames[ iClassIndex ] );
		}
		else
		{
			pLocalizedName = g_pVGuiLocalize->Find( "#TF_Bot_Generic_ClassName" );
		}
		g_pVGuiLocalize->ConvertUnicodeToANSI( pLocalizedName, szBotNameBuffer, sizeof( szBotNameBuffer ) );
		pBotName = szBotNameBuffer;
	}
	else
	{
		pBotName = GetRandomBotName();
	}
	
	const char *pDifficultyString = tf_bot_prefix_name_with_difficulty.GetBool() ? DifficultyLevelToString( skill ) : "";

	// we use this as our formatting, because we don't know the language of the downstream clients
	CFmtStr name( "%s%s%s", 
				  pDifficultyString, pFriendlyOrEnemyTitle, pBotName );
	Q_strncpy( pBuffer, name.Access(), iBufferSize );
}


//-----------------------------------------------------------------------------------------------------
CON_COMMAND_F( tf_bot_add, "Add a bot.", FCVAR_GAMEDLL )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	bool bQuotaManaged = true;
	int botCount = 1;
	const char *classname = NULL;
	const char *teamname = "auto";
	const char *pszBotNameViaArg = NULL;
	CTFBot::DifficultyType skill = clamp( (CTFBot::DifficultyType)tf_bot_difficulty.GetInt(), CTFBot::EASY, CTFBot::EXPERT );

	int i;
	for( i=1; i<args.ArgC(); ++i )
	{
		CTFBot::DifficultyType trySkill = StringToDifficultyLevel( args.Arg(i) );
		int nArgAsInteger = atoi( args.Arg(i) );

		// each argument could be a classname, a team, a difficulty level, a count, or a name
		if ( IsPlayerClassname( args.Arg(i) ) )
		{
			classname = args.Arg(i);
		}
		else if ( IsTeamName( args.Arg(i) ) )
		{
			teamname = args.Arg(i);
		}
		else if ( !stricmp( args.Arg( i ), "noquota" ) )
		{
			bQuotaManaged = false;
		}
		else if ( trySkill != CTFBot::UNDEFINED )
		{
			skill = trySkill;
		}
		else if ( nArgAsInteger > 0 )
		{
			botCount = nArgAsInteger;
			pszBotNameViaArg = NULL; // can't have a custom name if spawning multiple bots
		}
		else if ( botCount == 1 )
		{
			pszBotNameViaArg = args.Arg( i );
		}
		else
		{
			Warning( "Invalid argument '%s'\n", args.Arg(i) );
		}
	}

	// cvar can override classname
	classname = FStrEq( tf_bot_force_class.GetString(), "" ) ? classname : tf_bot_force_class.GetString();
	int iClassIndex = classname ? GetClassIndexFromString( classname ) : TF_CLASS_UNDEFINED;

	int iTeam = TEAM_UNASSIGNED;
	if ( FStrEq( teamname, "red" ) )
	{
		iTeam = TF_TEAM_RED;
	}
	else if ( FStrEq( teamname, "blue" ) )
	{
		iTeam = TF_TEAM_BLUE;
	}

	if ( TFGameRules()->IsInTraining() )
	{
		skill = CTFBot::EASY;
	}
	
	char name[256];
	int iNumAdded = 0;
	for( i=0; i<botCount; ++i )
	{
		CTFBot *pBot = NULL;
		const char *pszBotName = NULL;

		if ( !pszBotNameViaArg )
		{
			CreateBotName( iTeam, iClassIndex, skill, name, sizeof(name) );
			pszBotName = name;
		}
		else
		{
			pszBotName = pszBotNameViaArg;
		}

		pBot = NextBotCreatePlayerBot< CTFBot >( pszBotName );

		if ( pBot ) 
		{
			if ( bQuotaManaged )
			{
				pBot->SetAttribute( CTFBot::QUOTA_MANANGED );
			}

			pBot->HandleCommand_JoinTeam( teamname );

			pBot->SetDifficulty( skill );

			// if no class is set, auto-select one
			const char *thisClassname = classname ? classname : pBot->GetNextSpawnClassname();
			pBot->HandleCommand_JoinClass( thisClassname );

			// set up a proper name now that we are in training
			if ( TFGameRules()->IsInTraining() )
			{
				CreateBotName( pBot->GetTeamNumber(), pBot->GetPlayerClass()->GetClassIndex(), skill, name, sizeof(name) );
				engine->SetFakeClientConVarValue( pBot->edict(), "name", name );
			}

			++iNumAdded;
		}
	}

	if ( bQuotaManaged )
	{
		TheTFBots().OnForceAddedBots( iNumAdded );
	}
}


//-----------------------------------------------------------------------------------------------------
CON_COMMAND_F( tf_bot_kick, "Remove a TFBot by name, or all bots (\"all\").", FCVAR_GAMEDLL )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() < 2 )
	{
		DevMsg( "%s <bot name>, \"red\", \"blue\", or \"all\"> <optional: \"moveToSpectatorTeam\"> \n", args.Arg(0) );
		return;
	}

	bool bMoveToSpectatorTeam = false;
	int iTeam = TEAM_UNASSIGNED;
	int i;
	const char *pPlayerName = "";
	for( i=1; i<args.ArgC(); ++i )
	{
		// each argument could be a classname, a team, or a count
		if ( FStrEq( args.Arg(i), "red" ) )
		{
			iTeam = TF_TEAM_RED;
		}
		else if ( FStrEq( args.Arg(i), "blue" ) )
		{
			iTeam = TF_TEAM_BLUE;
		}
		else if ( FStrEq( args.Arg(i), "all" ) )
		{
			iTeam = TEAM_ANY;
		}
		else if ( FStrEq( args.Arg(i), "moveToSpectatorTeam" ) )
		{
			bMoveToSpectatorTeam = true;
		}
		else 
		{
			pPlayerName = args.Arg(i);
		}
	}

	int iNumKicked = 0;
	for( int i=1; i<=gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( UTIL_PlayerByIndex( i ) );

		if ( !player )
			continue;

		if ( FNullEnt( player->edict() ) )
			continue;

		if ( player->MyNextBotPointer() )
		{
			if ( iTeam == TEAM_ANY ||
				 FStrEq( pPlayerName, player->GetPlayerName() ) ||
				 ( player->GetTeamNumber() == iTeam ) ||
				 ( player->GetTeamNumber() == iTeam ) )
			{
				if ( bMoveToSpectatorTeam )
				{
					player->ChangeTeam( TEAM_SPECTATOR, false, true );
				}
				else
				{
					engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", player->GetUserID() ) );
				}
				CTFBot* pBot = dynamic_cast< CTFBot* >( player );
				if ( pBot && pBot->HasAttribute( CTFBot::QUOTA_MANANGED ) )
				{
					++iNumKicked;
				}				
			}
		}
	}
	TheTFBots().OnForceKickedBots( iNumKicked );
}


//-----------------------------------------------------------------------------------------------------
CON_COMMAND_F( tf_bot_kill, "Kill a TFBot by name, or all bots (\"all\").", FCVAR_GAMEDLL )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() < 2 )
	{
		DevMsg( "%s <bot name>, \"red\", \"blue\", or \"all\"> <optional: \"moveToSpectatorTeam\"> \n", args.Arg(0) );
		return;
	}

	int iTeam = TEAM_UNASSIGNED;
	int i;
	const char *pPlayerName = "";
	for( i=1; i<args.ArgC(); ++i )
	{
		// each argument could be a classname, a team, or a count
		if ( FStrEq( args.Arg(i), "red" ) )
		{
			iTeam = TF_TEAM_RED;
		}
		else if ( FStrEq( args.Arg(i), "blue" ) )
		{
			iTeam = TF_TEAM_BLUE;
		}
		else if ( FStrEq( args.Arg(i), "all" ) )
		{
			iTeam = TEAM_ANY;
		}
		else if ( FStrEq( args.Arg(i), "moveToSpectatorTeam" ) )
		{
			// bMoveToSpectatorTeam = true;
		}
		else 
		{
			pPlayerName = args.Arg(i);
		}
	}

	for( int i=1; i<=gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( UTIL_PlayerByIndex( i ) );

		if ( !player )
			continue;

		if ( FNullEnt( player->edict() ) )
			continue;

		if ( player->MyNextBotPointer() )
		{
			if ( iTeam == TEAM_ANY ||
				FStrEq( pPlayerName, player->GetPlayerName() ) ||
				( player->GetTeamNumber() == iTeam ) ||
				( player->GetTeamNumber() == iTeam ) )
			{
				CTakeDamageInfo info( player, player, 9999999.9f, DMG_ENERGYBEAM, TF_DMG_CUSTOM_NONE );
				player->TakeDamage( info );
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------
void CMD_BotWarpTeamToMe( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( !player )
		return;

	CTeam *myTeam = player->GetTeam();
	for( int i=0; i<myTeam->GetNumPlayers(); ++i )
	{
		if ( !myTeam->GetPlayer(i)->IsAlive() )
			continue;

		myTeam->GetPlayer(i)->SetAbsOrigin( player->GetAbsOrigin() );
	}
}
static ConCommand tf_bot_warp_team_to_me( "tf_bot_warp_team_to_me", CMD_BotWarpTeamToMe, "", FCVAR_GAMEDLL | FCVAR_CHEAT );


//-----------------------------------------------------------------------------------------------------
IMPLEMENT_INTENTION_INTERFACE( CTFBot, CTFBotMainAction );


//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_bot, CTFBot );

//-----------------------------------------------------------------------------------------------------
BEGIN_ENT_SCRIPTDESC( CTFBot, CTFPlayer, "Beep boop beep boop :3" )
DEFINE_SCRIPTFUNC_NAMED( SetAttribute, "AddBotAttribute", "Sets attribute flags on this TFBot" )
DEFINE_SCRIPTFUNC_NAMED( ClearAttribute, "RemoveBotAttribute", "Removes attribute flags on this TFBot" )
DEFINE_SCRIPTFUNC_NAMED( ClearAllAttributes, "ClearAllBotAttributes", "Clears all attribute flags on this TFBot" )
DEFINE_SCRIPTFUNC_NAMED( HasAttribute, "HasBotAttribute", "Checks if this TFBot has the given attributes" )

DEFINE_SCRIPTFUNC_NAMED( AddTag, "AddBotTag", "Adds a bot tag" )
DEFINE_SCRIPTFUNC_NAMED( RemoveTag, "RemoveBotTag", "Removes a bot tag" )
DEFINE_SCRIPTFUNC_NAMED( ClearTags, "ClearAllBotTags", "Clears bot tags" )
DEFINE_SCRIPTFUNC_NAMED( HasTag, "HasBotTag", "Checks if this TFBot has the given bot tag" )

DEFINE_SCRIPTFUNC_NAMED( SetWeaponRestriction, "AddWeaponRestriction", "Adds weapon restriction flags" )
DEFINE_SCRIPTFUNC_NAMED( RemoveWeaponRestriction, "RemoveWeaponRestriction", "Removes weapon restriction flags" )
DEFINE_SCRIPTFUNC_NAMED( ClearWeaponRestrictions, "ClearAllWeaponRestrictions", "Removes all weapon restriction flags" )
DEFINE_SCRIPTFUNC_NAMED( HasWeaponRestriction, "HasWeaponRestriction", "Checks if this TFBot has the given weapon restriction flags" )

DEFINE_SCRIPTFUNC_WRAPPED( IsWeaponRestricted, "Checks if the given weapon is restricted for use on the bot" )

DEFINE_SCRIPTFUNC_WRAPPED( GetDifficulty, "Returns the bot's difficulty level" )
DEFINE_SCRIPTFUNC_WRAPPED( SetDifficulty, "Sets the bots difficulty level" )
DEFINE_SCRIPTFUNC_WRAPPED( IsDifficulty, "Returns true/false if the bot's difficulty level matches." )

DEFINE_SCRIPTFUNC_WRAPPED( GetHomeArea, "Sets the home nav area of the bot" )
DEFINE_SCRIPTFUNC_WRAPPED( SetHomeArea, "Returns the home nav area of the bot -- may be nil." )

DEFINE_SCRIPTFUNC_WRAPPED( DelayedThreatNotice, "" )
DEFINE_SCRIPTFUNC( UpdateDelayedThreatNotices, "" )

DEFINE_SCRIPTFUNC( SetMaxVisionRangeOverride, "Sets max vision range override for the bot" )
DEFINE_SCRIPTFUNC( GetMaxVisionRangeOverride, "Gets the max vision range override for the bot" )

DEFINE_SCRIPTFUNC( SetScaleOverride, "Sets the scale override for the bot" )

DEFINE_SCRIPTFUNC( SetAutoJump, "Sets if the bot should automatically jump" )
DEFINE_SCRIPTFUNC( ShouldAutoJump, "Returns if the bot should automatically jump" )

DEFINE_SCRIPTFUNC( ShouldQuickBuild, "Returns if the bot should build instantly" )
DEFINE_SCRIPTFUNC( SetShouldQuickBuild, "Sets if the bot should build instantly" )

DEFINE_SCRIPTFUNC_WRAPPED( GetNearestKnownSappableTarget, "Gets the nearest known sappable target" )
DEFINE_SCRIPTFUNC_WRAPPED( GenerateAndWearItem, "Give me an item!" )

DEFINE_SCRIPTFUNC( IsInASquad, "Checks if we are in a squad" )
DEFINE_SCRIPTFUNC( LeaveSquad, "Makes us leave the current squad (if any)" )
DEFINE_SCRIPTFUNC( GetSquadFormationError, "Gets our formation error coefficient." )
DEFINE_SCRIPTFUNC( SetSquadFormationError, "Sets our formation error coefficient." )
DEFINE_SCRIPTFUNC_WRAPPED( DisbandCurrentSquad, "Forces the current squad to be entirely disbanded by everyone" )
DEFINE_SCRIPTFUNC_WRAPPED( FindVantagePoint, "Get the nav area of the closest vantage point (within distance)" )

DEFINE_SCRIPTFUNC_WRAPPED( SetAttentionFocus, "Sets our current attention focus to this entity" )
DEFINE_SCRIPTFUNC_WRAPPED( IsAttentionFocusedOn, "Is our attention focused on this entity" )
DEFINE_SCRIPTFUNC( ClearAttentionFocus, "Clear current focus" )
DEFINE_SCRIPTFUNC( IsAttentionFocused, "Is our attention focused right now?" )

DEFINE_SCRIPTFUNC( IsAmmoLow, "" )
DEFINE_SCRIPTFUNC( IsAmmoFull, "" )

DEFINE_SCRIPTFUNC_WRAPPED( GetSpawnArea, "Return the nav area of where we spawned" )

DEFINE_SCRIPTFUNC( PressFireButton, "" )
DEFINE_SCRIPTFUNC( PressAltFireButton, "" )
DEFINE_SCRIPTFUNC( PressSpecialFireButton, "" )

DEFINE_SCRIPTFUNC( GetBotId, "Get this bot's id" )
DEFINE_SCRIPTFUNC( FlagForUpdate, "Flag this bot for update" )
DEFINE_SCRIPTFUNC( IsFlaggedForUpdate, "Is this bot flagged for update" )
DEFINE_SCRIPTFUNC( GetTickLastUpdate, "Get last update tick" )
DEFINE_SCRIPTFUNC_WRAPPED( GetLocomotionInterface, "Get this bot's locomotion interface" )
DEFINE_SCRIPTFUNC_WRAPPED( GetBodyInterface, "Get this bot's body interface" )
DEFINE_SCRIPTFUNC_WRAPPED( GetIntentionInterface, "Get this bot's intention interface" )
DEFINE_SCRIPTFUNC_WRAPPED( GetVisionInterface, "Get this bot's vision interface" )
DEFINE_SCRIPTFUNC_WRAPPED( IsEnemy, "Return true if given entity is our enemy" )
DEFINE_SCRIPTFUNC_WRAPPED( IsFriend, "Return true if given entity is our friend" )
DEFINE_SCRIPTFUNC( IsImmobile, "Return true if we haven't moved in awhile" )
DEFINE_SCRIPTFUNC( GetImmobileDuration, "How long have we been immobile" )
DEFINE_SCRIPTFUNC( ClearImmobileStatus, "Clear immobile status" )
DEFINE_SCRIPTFUNC( GetImmobileSpeedThreshold, "Return units/second below which this actor is considered immobile" )

DEFINE_SCRIPTFUNC_NAMED( ScriptGetAllTags, "GetAllBotTags", "Get all bot tags" )

DEFINE_SCRIPTFUNC_WRAPPED( SetMission, "Set this bot's current mission to the given mission" )
DEFINE_SCRIPTFUNC_WRAPPED( SetPrevMission, "Set this bot's previous mission to the given mission" )
DEFINE_SCRIPTFUNC_WRAPPED( GetMission, "Get this bot's current mission" )
DEFINE_SCRIPTFUNC_WRAPPED( GetPrevMission, "Get this bot's previous mission" )
DEFINE_SCRIPTFUNC_WRAPPED( HasMission, "Return true if the given mission is this bot's current mission" )
DEFINE_SCRIPTFUNC( IsOnAnyMission, "Return true if this bot has a current mission" )
DEFINE_SCRIPTFUNC_WRAPPED( SetMissionTarget, "Set this bot's mission target to the given entity" )
DEFINE_SCRIPTFUNC_WRAPPED( GetMissionTarget, "Get this bot's current mission target" )

DEFINE_SCRIPTFUNC( SetBehaviorFlag, "Set the given behavior flag(s) for this bot" )
DEFINE_SCRIPTFUNC( ClearBehaviorFlag, "Clear the given behavior flag(s) for this bot" )
DEFINE_SCRIPTFUNC( IsBehaviorFlagSet, "Return true if the given behavior flag(s) are set for this bot" )

DEFINE_SCRIPTFUNC_WRAPPED( SetActionPoint, "Set the given action point for this bot" )
DEFINE_SCRIPTFUNC_WRAPPED( GetActionPoint, "Get the given action point for this bot" )

END_SCRIPTDESC();

//-----------------------------------------------------------------------------------------------------
/**
 * Allocate a bot and bind it to the edict
 */
CBasePlayer *CTFBot::AllocatePlayerEntity( edict_t *edict, const char *playerName )
{
	CBasePlayer::s_PlayerEdict = edict;
	return static_cast< CBasePlayer * >( CreateEntityByName( "tf_bot" ) );
}


//-----------------------------------------------------------------------------------------------------
void CTFBot::PressFireButton( float duration )
{
	// can't fire if stunned
	// @todo Tom Bui: Eventually, we'll probably want to check the actual weapon for supress fire
	if ( m_Shared.IsControlStunned() || m_Shared.IsLoserStateStunned() || HasAttribute( CTFBot::SUPPRESS_FIRE ) )
	{
		ReleaseFireButton();
		return;
	}

	BaseClass::PressFireButton( duration );
}


//-----------------------------------------------------------------------------------------------------
void CTFBot::PressAltFireButton( float duration )
{
	// can't fire if stunned
	// @todo Tom Bui: Eventually, we'll probably want to check the actual weapon for supress fire
	if ( m_Shared.IsControlStunned() || m_Shared.IsLoserStateStunned() || HasAttribute( CTFBot::SUPPRESS_FIRE ) )
	{
		ReleaseAltFireButton();
		return;
	}

	BaseClass::PressAltFireButton( duration );
}


//-----------------------------------------------------------------------------------------------------
void CTFBot::PressSpecialFireButton( float duration )
{
	// can't fire if stunned
	// @todo Tom Bui: Eventually, we'll probably want to check the actual weapon for supress fire
	if ( m_Shared.IsControlStunned() || m_Shared.IsLoserStateStunned() || HasAttribute( CTFBot::SUPPRESS_FIRE ) )
	{
		ReleaseAltFireButton();
		return;
	}

	BaseClass::PressSpecialFireButton( duration );
}


//-----------------------------------------------------------------------------------------------------
class CCountClassMembers
{
public:
	CCountClassMembers( const CTFBot *me, int teamID )
	{
		m_me = me;
		m_myTeam = teamID;
		m_teamSize = 0;
		
		for( int i=0; i<TF_LAST_NORMAL_CLASS; ++i )
			m_count[i] = 0;
	}

	bool operator() ( CBasePlayer *basePlayer )
	{
		CTFPlayer *player = (CTFPlayer *)basePlayer;

		if ( player->GetTeamNumber() != m_myTeam )
			return true;

		++m_teamSize;

		if ( m_me->IsSelf( player ) )
			return true;

		++m_count[ player->GetDesiredPlayerClassIndex() ];

		return true;
	}

	const CTFBot *m_me;
	int m_myTeam;
	int m_count[ TF_LAST_NORMAL_CLASS+1 ];
	int m_teamSize;
};


//-----------------------------------------------------------------------------------------------------
bool CTFBot::GetWeightDesiredClassToSpawn( CUtlVector< ETFClass > &vecClassToSpawn ) const
{
	// make sure there's nothing in the output
	vecClassToSpawn.RemoveAll();

	if ( !CanChangeClass() )
	{
		vecClassToSpawn.AddToTail( (ETFClass)GetPlayerClass()->GetClassIndex() );
		return false;
	}

	struct ClassSelectionInfo
	{
		ETFClass m_class;
		int m_minTeamSizeToSelect;					// team must have this many members to choose this class
		int m_countPerTeamSize;						// must have 1 Medic for each 4 team members, for example
		int m_minLimit;								// minimum that must be present (once other constraints are met)
		int m_maxLimit[NUM_DIFFICULTY_LEVELS];	// maximum that can be present (-1 for infinite)
	};

	const int NoLimit = -1;

	static ClassSelectionInfo defenseRoster[] =
	{
		{ TF_CLASS_ENGINEER,		0, 4, 1, { 1, 2, 3, 3 } },
		{ TF_CLASS_SOLDIER,			0, 0, 0, { NoLimit, NoLimit, NoLimit, NoLimit } },
		{ TF_CLASS_DEMOMAN,			0, 0, 0, { 2, 3, 3, 3 } },
		{ TF_CLASS_PYRO,			3, 0, 0, { NoLimit, NoLimit, NoLimit, NoLimit } },
		{ TF_CLASS_HEAVYWEAPONS,	3, 0, 0, { 1, 1, 2, 2 } },
		{ TF_CLASS_MEDIC,			4, 4, 1, { 1, 1, 2, 2 } },
		{ TF_CLASS_SNIPER,			5, 0, 0, { 0, 1, 1, 1 } },
		{ TF_CLASS_SPY,				5, 0, 0, { 0, 1, 2, 2 } },

		{ TF_CLASS_UNDEFINED,		0, -1 },
	};

	static ClassSelectionInfo offenseRoster[] =
	{
		{ TF_CLASS_SCOUT,			0, 0, 1, { 3, 3, 3, 3 } },
		{ TF_CLASS_SOLDIER,			0, 0, 0, { NoLimit, NoLimit, NoLimit, NoLimit } },
		{ TF_CLASS_DEMOMAN,			0, 0, 0, { 2, 3, 3, 3 } },							// must limit demomen, or the whole team will go demo to take out tough sentryguns
		{ TF_CLASS_PYRO,			3, 0, 0, { NoLimit, NoLimit, NoLimit, NoLimit } },
		{ TF_CLASS_HEAVYWEAPONS,	3, 0, 0, { 1, 1, 2, 2 } },
		{ TF_CLASS_MEDIC,			4, 4, 1, { 1, 1, 2, 2 } },
		{ TF_CLASS_SNIPER,			5, 0, 0, { 0, 1, 1, 1 } },
		{ TF_CLASS_SPY,				5, 0, 0, { 0, 1, 2, 2 } },
		{ TF_CLASS_ENGINEER,		5, 0, 0, { 1, 1, 1, 1 } },

		{ TF_CLASS_UNDEFINED,		0, -1 },
	};

	static ClassSelectionInfo compRoster[] =
	{
		{ TF_CLASS_SCOUT,			0, 0, 0, { 0, 0, 2, 2 } },
		{ TF_CLASS_SOLDIER,			0, 0, 0, { 0, 0, NoLimit, NoLimit } },
		{ TF_CLASS_DEMOMAN,			0, 0, 0, { 0, 0, 2, 2 } },							// must limit demomen, or the whole team will go demo to take out tough sentryguns
		{ TF_CLASS_PYRO,			0, -1 },
		{ TF_CLASS_HEAVYWEAPONS,	3, 0, 0, { 0, 0, 2, 2 } },
		{ TF_CLASS_MEDIC,			1, 0, 1, { 0, 0, 1, 1 } },
		{ TF_CLASS_SNIPER,			0, -1 },
		{ TF_CLASS_SPY,				0, -1 },
		{ TF_CLASS_ENGINEER,		0, -1 },

		{ TF_CLASS_UNDEFINED,		0, -1 },
	};

	// count classes in use by my team, not including me
	CCountClassMembers currentRoster( this, GetTeamNumber() );
	ForEachPlayer( currentRoster );

	// assume offense
	ClassSelectionInfo *desiredRoster = offenseRoster;

	if ( TFGameRules()->IsMatchTypeCompetitive() )
	{
		desiredRoster = compRoster;
	}
	else if ( TFGameRules()->IsInKothMode() )
	{
		CTeamControlPoint *point = GetMyControlPoint();
		if ( point )
		{
			if ( GetTeamNumber() == ObjectiveResource()->GetOwningTeam( point->GetPointIndex() ) )
			{
				// defend our point
				desiredRoster = defenseRoster;
			}
		}
	}
	else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
	{
		CUtlVector< CTeamControlPoint * > captureVector;
		TFGameRules()->CollectCapturePoints( const_cast< CTFBot * >( this ), &captureVector );

		CUtlVector< CTeamControlPoint * > defendVector;
		TFGameRules()->CollectDefendPoints( const_cast< CTFBot * >( this ), &defendVector );

		// if we have any points we can capture, try to do so
		if ( captureVector.Count() > 0 || defendVector.Count() == 0 )
		{
			desiredRoster = offenseRoster;
		}
		else
		{
			desiredRoster = defenseRoster;
		}
	}
	else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		if ( GetTeamNumber() == TF_TEAM_RED )
		{
			desiredRoster = defenseRoster;
		}
	}

	// build vector of classes we can pick from
	CUtlVector< ETFClass > desiredClassVector;
	CUtlVector< ETFClass > allowedClassForBotRosterVector;

	bool bHasRequiredClass = false;
	int nCurrentMinRequiredClass = INT_MAX;
	for ( int i = 0; desiredRoster[i].m_class != TF_CLASS_UNDEFINED; ++i )
	{
		ClassSelectionInfo *desiredClassInfo = &desiredRoster[ i ];

		if ( TFGameRules()->CanBotChooseClass( const_cast< CTFBot * >( this ), desiredClassInfo->m_class ) == false )
		{
			// not allowed to use this class
			continue;
		}
		// just in case we hit the class limits, we want to make sure we select a class that is allowed
		allowedClassForBotRosterVector.AddToTail( desiredClassInfo->m_class );

		if ( currentRoster.m_teamSize < desiredClassInfo->m_minTeamSizeToSelect )
		{
			// team is too small to choose this class
			continue;
		}

		if ( bHasRequiredClass && currentRoster.m_count[ desiredClassInfo->m_class ] > nCurrentMinRequiredClass )
		{
			// looking for required class, anything over the min count should be ignored
			continue;
		}

		// check limits
		if ( currentRoster.m_count[ desiredClassInfo->m_class ] < desiredClassInfo->m_minLimit )
		{
			// below required limit - choose only this class
			if ( currentRoster.m_count[ desiredClassInfo->m_class ] < nCurrentMinRequiredClass )
			{
				nCurrentMinRequiredClass = currentRoster.m_count[ desiredClassInfo->m_class ];
				desiredClassVector.RemoveAll();
			}
			desiredClassVector.AddToTail( desiredClassInfo->m_class );

			bHasRequiredClass = true;
			continue;
		}

		int maxLimit = desiredClassInfo->m_maxLimit[ (int)clamp( GetDifficulty(), CTFBot::EASY, CTFBot::EXPERT ) ];
		if ( maxLimit > NoLimit && currentRoster.m_count[ desiredClassInfo->m_class ] >= maxLimit )
		{
			// at or above limit for this class
			continue;
		}

		if ( desiredClassInfo->m_countPerTeamSize > 0 )
		{
			// how many of this class should there be at the given "per" count
			int maxCountPer = currentRoster.m_teamSize / desiredClassInfo->m_countPerTeamSize;
			if ( currentRoster.m_count[ desiredClassInfo->m_class ] - desiredClassInfo->m_minTeamSizeToSelect < maxCountPer )
			{
				// below required limit - choose only this class
				if ( currentRoster.m_count[ desiredClassInfo->m_class ] < nCurrentMinRequiredClass )
				{
					nCurrentMinRequiredClass = currentRoster.m_count[ desiredClassInfo->m_class ];
					desiredClassVector.RemoveAll();
				}
				desiredClassVector.AddToTail( desiredClassInfo->m_class );
				
				bHasRequiredClass = true;
				continue;
			}
		}

		// valid class to choose
		if ( !bHasRequiredClass )
		{
			desiredClassVector.AddToTail( desiredClassInfo->m_class );
		}
	}

	// copy to output
	if ( desiredClassVector.Count() == 0 )
	{
		vecClassToSpawn = allowedClassForBotRosterVector;
	}
	else
	{
		vecClassToSpawn = desiredClassVector;
	}

	return bHasRequiredClass;
}


//-----------------------------------------------------------------------------------------------------
ETFClass CTFBot::GetPresetClassToSpawn() const
{
	if ( !CanChangeClass() )
	{
		return (ETFClass)GetPlayerClass()->GetClassIndex();
	}

	static ETFClass offenseRoster[] =
	{
		TF_CLASS_MEDIC,
		TF_CLASS_ENGINEER,
		TF_CLASS_SOLDIER,
		TF_CLASS_HEAVYWEAPONS,
		TF_CLASS_DEMOMAN,
		TF_CLASS_SCOUT,

		TF_CLASS_PYRO,
		TF_CLASS_SOLDIER,
		TF_CLASS_DEMOMAN,
		TF_CLASS_SNIPER,
		TF_CLASS_MEDIC,
		TF_CLASS_SPY,
	};

	static ETFClass defenseRoster[] =
	{
		TF_CLASS_MEDIC,
		TF_CLASS_ENGINEER,
		TF_CLASS_SOLDIER,
		TF_CLASS_DEMOMAN,
		TF_CLASS_SCOUT,
		TF_CLASS_HEAVYWEAPONS,

		TF_CLASS_SNIPER,
		TF_CLASS_ENGINEER,
		TF_CLASS_SOLDIER,
		TF_CLASS_MEDIC,
		TF_CLASS_PYRO,
		TF_CLASS_SPY,
	};

	static ETFClass compRoster[] =
	{
		TF_CLASS_MEDIC,
		TF_CLASS_SCOUT,
		TF_CLASS_SOLDIER,
		TF_CLASS_DEMOMAN,
		TF_CLASS_SCOUT,
		TF_CLASS_SOLDIER,

		TF_CLASS_HEAVYWEAPONS,
		TF_CLASS_PYRO,
		TF_CLASS_MEDIC,
		TF_CLASS_ENGINEER,
		TF_CLASS_SNIPER,
		TF_CLASS_SPY,
	};

	// make sure we have completed list of rolls per team
	COMPILE_TIME_ASSERT( ARRAYSIZE( offenseRoster ) == 12 );
	COMPILE_TIME_ASSERT( ARRAYSIZE( defenseRoster ) == 12 );
	COMPILE_TIME_ASSERT( ARRAYSIZE( compRoster ) == 12 );

	// assume offense
	ETFClass *desiredRoster = offenseRoster;

	if ( TFGameRules()->IsMatchTypeCompetitive() )
	{
		desiredRoster = compRoster;
	}
	else if ( TFGameRules()->IsInKothMode() )
	{
		CTeamControlPoint *point = GetMyControlPoint();
		if ( point )
		{
			if ( GetTeamNumber() == ObjectiveResource()->GetOwningTeam( point->GetPointIndex() ) )
			{
				// defend our point
				desiredRoster = defenseRoster;
			}
		}
	}
	else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
	{
		CUtlVector< CTeamControlPoint * > captureVector;
		TFGameRules()->CollectCapturePoints( const_cast< CTFBot * >( this ), &captureVector );

		CUtlVector< CTeamControlPoint * > defendVector;
		TFGameRules()->CollectDefendPoints( const_cast< CTFBot * >( this ), &defendVector );

		// if we have any points we can capture, try to do so
		if ( captureVector.Count() > 0 || defendVector.Count() == 0 )
		{
			desiredRoster = offenseRoster;
		}
		else
		{
			desiredRoster = defenseRoster;
		}
	}
	else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		if ( GetTeamNumber() == TF_TEAM_RED )
		{
			desiredRoster = defenseRoster;
		}
	}

	// count classes in use by my team, not including me
	CCountClassMembers currentRoster( this, GetTeamNumber() );
	ForEachPlayer( currentRoster );

	int classCount[TF_LAST_NORMAL_CLASS];
	V_memset( classCount, 0, sizeof( classCount ) );
	for ( int i=0; i<12; ++i )
	{
		ETFClass iClass = desiredRoster[i];

		if ( currentRoster.m_count[ iClass ] > classCount[ iClass ] )
		{
			// if we have enough of this class, skip it
			classCount[ iClass ]++;
		}
		else
		{
			return iClass;
		}
	}

	AssertMsg( 0, "This return shouldn't happen." );
	return TF_CLASS_UNDEFINED;
}


bool CTFBot::CanChangeClass() const
{
	// if we are an engineer with an active sentry or teleporters, don't switch
	if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		if ( const_cast< CTFBot * >( this )->GetObjectOfType( OBJ_SENTRYGUN ) ||
			const_cast< CTFBot * >( this )->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT ) )
		{
			return false;
		}
	}
	// if a medic has 25% uber charge or more, don't allow to change class
	else if ( IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		CWeaponMedigun *medigun = dynamic_cast< CWeaponMedigun * >( m_Shared.GetActiveTFWeapon() );
		if ( medigun && medigun->GetChargeLevel() > 0.25f )
		{
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------------------------------
/**
 * NOTE: Assumes bot's difficulty has been set, and the bot is on a team.
 */
const char *CTFBot::GetNextSpawnClassname( void ) const
{
	ETFClass iNextClass = TF_CLASS_UNDEFINED;

	const char *pszForceClass = tf_bot_force_class.GetString();
	if ( !FStrEq( pszForceClass, "" ) )
	{
		iNextClass = (ETFClass)GetClassIndexFromString( pszForceClass );
	}

	if ( iNextClass == TF_CLASS_UNDEFINED && tf_bot_spawn_use_preset_roster.GetBool() && ( !TFGameRules() || !TFGameRules()->IsInTraining() ))
	{
		iNextClass = GetPresetClassToSpawn();
	}
	else if ( iNextClass == TF_CLASS_UNDEFINED )
	{
		CUtlVector< ETFClass > desiredClassVector;
		GetWeightDesiredClassToSpawn( desiredClassVector );
		if ( desiredClassVector.Count() == 0 )
		{
			// nothing available
			Warning( "TFBot unable to choose a class, defaulting to 'auto'\n" );
			return "auto";
		}

		int which = RandomInt( 0, desiredClassVector.Count() - 1 );

		// if we need to destroy a sentry, pick a class that can do so
		if ( GetEnemySentry() )
		{
			// best sentry demolitions
			int demoman = desiredClassVector.Find( TF_CLASS_DEMOMAN );
			if ( demoman >= 0 )
			{
				which = demoman;
			}
			else
			{
				// next best sentry demolitions
				int spy = desiredClassVector.Find( TF_CLASS_SPY );
				if ( spy >= 0 )
				{
					which = spy;
				}
				else
				{
					// good sentry demolitions
					int soldier = desiredClassVector.Find( TF_CLASS_SOLDIER );
					if ( soldier >= 0 )
					{
						which = soldier;
					}
				}
			}
		}

		iNextClass = desiredClassVector[ which ];
	}

	Assert( iNextClass != TF_CLASS_UNDEFINED );
	TFPlayerClassData_t *classData = GetPlayerClassData( iNextClass );
	if ( classData )
	{
		return classData->m_szClassName;
	}

	Warning( "TFBot unable to get data for desired class, defaulting to 'auto'\n" );
	return "auto";
}


//-----------------------------------------------------------------------------------------------------
CTFBot::CTFBot()
{
	m_body = new CTFBotBody( this );
	m_locomotor = new CTFBotLocomotion( this );
	m_vision = new CTFBotVision( this );
	ALLOCATE_INTENTION_INTERFACE( CTFBot );

	m_spawnArea = NULL;
	m_weaponRestrictionFlags = 0;
	m_attributeFlags = 0;
	m_homeArea = NULL;
	m_squad = NULL;
	m_didReselectClass = false;
	m_enemySentry = NULL;
	m_spotWhereEnemySentryLastInjuredMe = vec3_origin;
	m_isLookingAroundForEnemies = true;
	m_behaviorFlags = 0;
	m_attentionFocusEntity = NULL;
	m_noisyTimer.Invalidate();

	if ( TFGameRules()->IsInTraining() )
	{
		m_difficulty = CTFBot::EASY;
	}
	else
	{
		m_difficulty = clamp( (CTFBot::DifficultyType)tf_bot_difficulty.GetInt(), CTFBot::EASY, CTFBot::EXPERT );
	}

	m_actionPoint = NULL;
	m_proxy = NULL;
	m_spawner = NULL;

	m_myControlPoint = NULL;

	SetMission( NO_MISSION, MISSION_DOESNT_RESET_BEHAVIOR_SYSTEM );
	SetMissionTarget( NULL );
	m_missionString.Clear();

	m_fModelScaleOverride = -1.0f;
	m_maxVisionRangeOverride = -1.0f;
	m_squadFormationError = 0.0f;

	m_hFollowingFlagTarget = NULL;

	SetShouldQuickBuild( false );
	SetAutoJump( 0.f, 0.f );

	ClearSniperSpots();

	ListenForGameEvent( "teamplay_point_startcapture" );
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_flag_event" );
}


//-----------------------------------------------------------------------------------------------------
CTFBot::~CTFBot()
{
	// delete Intention first, since destruction of Actions may access other components
	DEALLOCATE_INTENTION_INTERFACE;

	if ( m_body )
		delete m_body;

	if ( m_locomotor )
		delete m_locomotor;

	if ( m_vision )
		delete m_vision;

	m_suspectedSpyVector.PurgeAndDeleteElements();
}


//-----------------------------------------------------------------------------------------------------
void CTFBot::Spawn()
{
	BaseClass::Spawn();

	m_spawnArea = NULL;
	m_justLostPointTimer.Invalidate();
	m_squad = NULL;
	m_didReselectClass = false;
	m_isLookingAroundForEnemies = true;
	m_attentionFocusEntity = NULL;

	m_suspectedSpyVector.PurgeAndDeleteElements();
	m_knownSpyVector.RemoveAll();
	m_delayedNoticeVector.RemoveAll();

	m_myControlPoint = NULL;
	ClearSniperSpots();
	ClearTags();

	m_hFollowingFlagTarget = NULL;

	m_requiredWeaponStack.Clear();
	SetShouldQuickBuild( false );

	SetSquadFormationError( 0.0f );
	SetBrokenFormation( false );

	GetVisionInterface()->ForgetAllKnownEntities();
}


//-----------------------------------------------------------------------------------------------------
void CTFBot::SetMission( MissionType mission, bool resetBehaviorSystem )
{
	SetPrevMission( m_mission );
	m_mission = mission;

	if ( resetBehaviorSystem )
	{
		// reset the behavior system to start the given mission
		GetIntentionInterface()->Reset();
	}

	// Temp hack - some missions play an idle loop
	if ( m_mission > NO_MISSION )
	{
		StartIdleSound();
	}
}

//-----------------------------------------------------------------------------------------------------
bool CTFBot::ShouldReEvaluateCurrentClass( void ) const
{
	ETFClass iCurrentClass = ( ETFClass )GetPlayerClass()->GetClassIndex();
	Assert( iCurrentClass != TF_CLASS_UNDEFINED );
	TFPlayerClassData_t *classData = GetPlayerClassData( iCurrentClass );
	Assert( classData );
	return classData && !FStrEq( classData->m_szClassName, GetNextSpawnClassname() );
}

// UGLY HACK TO FIX BOTS NOT RE-EVALUATING THEIR CLASS WHEN THEY SWITCH TEAM
// NEED TO REVISIT THIS AND UNDERSTAND THE COMMENT BELOW ABOUT DOING IT OUTSIDE THE BEHAVIOR SYSTEM
//-----------------------------------------------------------------------------------------------------
void CTFBot::ReEvaluateCurrentClass( void )
{
	// having the bot die will trigger them to
	// re-evaluate their class in PhysicsSimulate() below
	CommitSuicide( false, true );
}

//-----------------------------------------------------------------------------------------------------
void CTFBot::PhysicsSimulate( void )
{
	BaseClass::PhysicsSimulate();

	if ( m_spawnArea == NULL )
	{
		m_spawnArea = GetLastKnownArea();
	}

	if ( HasAttribute( CTFBot::ALWAYS_CRIT ) && !m_Shared.InCond( TF_COND_CRITBOOSTED_USER_BUFF ) )
	{
		m_Shared.AddCond( TF_COND_CRITBOOSTED_USER_BUFF );
	}

	// force my speed to be recalculated to keep squad together and restore speed afterwards
	TeamFortress_SetSpeed();

	if ( IsInASquad() )
	{
		if ( GetSquad()->GetMemberCount() <= 1 || GetSquad()->GetLeader() == NULL )
		{
			// squad has collapsed - disband it
			LeaveSquad();
		}
	}


	// If we're dead, choose a new class.
	// We need to do this outside of the behavior system, since changing class can
	// sometimes force an immediate respawn, which will destroy the bot's existing actions out from under it.
	if ( !IsAlive() && !m_didReselectClass && tf_bot_keep_class_after_death.GetBool() == false && TFGameRules()->CanBotChangeClass( this ) )
	{
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
			return;

		HandleCommand_JoinClass( GetNextSpawnClassname() );

		m_didReselectClass = true;
	}
}


//-----------------------------------------------------------------------------------------------------
void CTFBot::Touch( CBaseEntity *pOther )
{
	BaseClass::Touch( pOther );

	CTFPlayer *them = ToTFPlayer( pOther );
	if ( them && IsEnemy( them ) )
	{
		if ( them->m_Shared.IsStealthed() || them->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			// bumped a spy - they are discovered!
			if ( TFGameRules()->IsMannVsMachineMode() )	// we have to build up to knowing that they are a spy in MvM
			{
				SuspectSpy( them );
			}
			else
			{
				RealizeSpy( them );
			}
		}

		// always notice if we bump an enemy
		TheNextBots().OnWeaponFired( them, them->GetActiveTFWeapon() );
	}
}


//-----------------------------------------------------------------------------------------------------
// Avoid penetrating teammates
void CTFBot::AvoidPlayers( CUserCmd *pCmd )
{
	// Turn off the avoid player code.
	if ( !tf_avoidteammates.GetBool() || !tf_avoidteammates_pushaway.GetBool() )
		return;

	Vector forward, right;
	EyeVectors( &forward, &right );

	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS );

	Vector avoidVector = vec3_origin;

	float tooClose = 50.0f;
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		// bots stay farther apart in MvM mode
		tooClose = 150.0f;
	}

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *them = playerVector[i];

		if ( IsSelf( them ) )
		{
			continue;
		}

		if ( HasTheFlag() )
		{
			// Don't push around the flag (bomb) carrier.
			// We need this for MvM mode so friendly bots don't
			// move the bomb jumper and cause him to restart.
			continue;
		}

		if ( IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			if ( !them->IsPlayerClass( TF_CLASS_MEDIC ) )
			{
				// medics only avoid other medics, so they stay with their patient
				continue;
			}
		}
		else if ( IsInASquad() )
		{
			// if I'm a non-Medic in a Squad, I'm part of a formation
			continue;
		}

		Vector between = GetAbsOrigin() - them->GetAbsOrigin();
		if ( between.IsLengthLessThan( tooClose ) )
		{
			float range = between.NormalizeInPlace();

			avoidVector += ( 1.0f - ( range / tooClose ) ) * between;
		}
	}

	if ( avoidVector.IsZero() )
	{
		m_Shared.SetSeparation( false );
		m_Shared.SetSeparationVelocity( vec3_origin );
		return;
	}

	avoidVector.NormalizeInPlace();

	m_Shared.SetSeparation( true );

	const float maxSpeed = 50.0f;
	m_Shared.SetSeparationVelocity( avoidVector * maxSpeed );

	float ahead = maxSpeed * DotProduct( forward, avoidVector );
	float side = maxSpeed * DotProduct( right, avoidVector );

	pCmd->forwardmove	+= ahead;
	pCmd->sidemove		+= side;
}


//-----------------------------------------------------------------------------------------------------
void CTFBot::UpdateOnRemove( void )
{
	StopIdleSound();

	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------------------------------
int CTFBot::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	if ( HasAttribute( USE_BOSS_HEALTH_BAR ) )
	{
		return FL_EDICT_ALWAYS;
	}

	return BaseClass::ShouldTransmit( pInfo );
}


//-----------------------------------------------------------------------------------------------------
void CTFBot::ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent, bool bAutoBalance /*= false*/  )
{
	BaseClass::ChangeTeam( iTeamNum, bAutoTeam, bSilent, bAutoBalance );
	
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		SetPrevMission( CTFBot::NO_MISSION );
		ClearAllAttributes();
		// Clear Sound
		StopIdleSound();
	}
}


//-----------------------------------------------------------------------------------------------------
bool CTFBot::ShouldGib( const CTakeDamageInfo &info )
{
	// only gib giant/miniboss
	if ( TFGameRules()->IsMannVsMachineMode() && ( IsMiniBoss() || GetModelScale() > 1.f ) )
	{
		return true;
	}

	return BaseClass::ShouldGib( info );
}


//-----------------------------------------------------------------------------------------------------
bool CTFBot::IsAllowedToPickUpFlag( void ) const
{
	if ( !BaseClass::IsAllowedToPickUpFlag() )
	{
		return false;
	}

	// only the leader of a squad can pick up the flag
	if ( IsInASquad() && !GetSquad()->IsLeader( const_cast< CTFBot * >( this ) ) )
		return false;

	// mission bots can't pick up the flag
	return !IsOnAnyMission();
}


//-----------------------------------------------------------------------------------------------------
void CTFBot::InitClass( void )
{
	BaseClass::InitClass();
}

void CTFBot::ModifyMaxHealth( int nNewMaxHealth, bool bSetCurrentHealth /*= true*/, bool bAllowModelScaling /*= true*/ )
{
	if ( GetMaxHealth() != nNewMaxHealth )
	{
		static CSchemaAttributeDefHandle pAttrDef_HiddenMaxHealthNonBuffed( "hidden maxhealth non buffed" );
		if ( !pAttrDef_HiddenMaxHealthNonBuffed )
		{
			Warning( "TFBotSpawner: Invalid attribute 'hidden maxhealth non buffed'\n" );
		}
		else
		{
			CAttributeList *pAttrList = GetAttributeList();
			if ( pAttrList )
			{
				pAttrList->SetRuntimeAttributeValue( pAttrDef_HiddenMaxHealthNonBuffed, nNewMaxHealth - GetMaxHealth() );
			}
		}
	}

	if ( bSetCurrentHealth )
	{
		SetHealth( nNewMaxHealth );
	}

	if ( bAllowModelScaling && IsMiniBoss() )
	{
		SetModelScale( m_fModelScaleOverride > 0.0f ? m_fModelScaleOverride : tf_mvm_miniboss_scale.GetFloat() );
		SetViewOffset( GetClassEyeHeight() );
	}
}

//-----------------------------------------------------------------------------------------------------
/**
 * Invoked when a game event occurs
 */
void CTFBot::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( FStrEq( eventName, "teamplay_point_captured" ) )
	{
		ClearMyControlPoint();

		int whoCapped = event->GetInt( "team" );
		int pointID = event->GetInt( "cp" );

		if ( whoCapped == GetTeamNumber() )
		{
			OnTerritoryCaptured( pointID );
		}
		else
		{
			OnTerritoryLost( pointID );

			m_justLostPointTimer.Start( RandomFloat( 10.0f, 20.0f ) );
		}
	}
	else if ( FStrEq( eventName, "teamplay_point_startcapture" ) )
	{
		int pointID = event->GetInt( "cp" );

		OnTerritoryContested( pointID );
	}
	else if ( FStrEq( eventName, "teamplay_flag_event" ) )
	{
		if ( event->GetInt( "eventtype" ) == TF_FLAGEVENT_PICKUP )
		{
			int iPlayer = event->GetInt( "player" );
			if ( iPlayer == entindex() )
			{
				// I just picked up the flag
				OnPickUp( NULL, NULL );
			}
		}
	}
}

	
//-----------------------------------------------------------------------------------------------------
void CTFBot::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	if ( HasProxy() )
	{
		GetProxy()->OnKilled();
	}

	// announce Spies
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		if ( IsPlayerClass( TF_CLASS_SPY ) )
		{
			CUtlVector< CTFPlayer * > playerVector;
			CollectPlayers( &playerVector, TF_TEAM_PVE_INVADERS, COLLECT_ONLY_LIVING_PLAYERS );

			int spyCount = 0;
			for( int i=0; i<playerVector.Count(); ++i )
			{
				if ( playerVector[i]->IsPlayerClass( TF_CLASS_SPY ) )
				{
					++spyCount;
				}
			}

			IGameEvent *event = gameeventmanager->CreateEvent( "mvm_mission_update" );
			if ( event )
			{
				event->SetInt( "class", TF_CLASS_SPY );
				event->SetInt( "count", spyCount );
				gameeventmanager->FireEvent( event );
			}
		}
		else if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			// in MVM, when an engineer dies, we need to decouple his objects so they stay alive when his bot slot gets recycled
			while ( GetObjectCount() > 0 )
			{
				// set to not have owner
				CBaseObject *pObject = GetObject( 0 );
				if ( pObject )
				{
					pObject->SetOwnerEntity( NULL );
					pObject->SetBuilder( NULL );
				}
				RemoveObject( pObject );
			}

			// unown engineer nest if owned any
			for ( int i=0; i<ITFBotHintEntityAutoList::AutoList().Count(); ++i )
			{
				CBaseTFBotHintEntity* pHint = static_cast< CBaseTFBotHintEntity* >( ITFBotHintEntityAutoList::AutoList()[i] );
				if ( pHint->GetOwnerEntity() == this )
				{
					pHint->SetOwnerEntity( NULL );
				}
			}

			CUtlVector< CTFPlayer* > playerVector;
			CollectPlayers( &playerVector, TF_TEAM_PVE_INVADERS, COLLECT_ONLY_LIVING_PLAYERS );
			bool bShouldAnnounceLastEngineerBotDeath = HasAttribute( CTFBot::TELEPORT_TO_HINT );
			if ( bShouldAnnounceLastEngineerBotDeath )
			{
				for ( int i=0; i<playerVector.Count(); ++i )
				{
					if ( playerVector[i] != this && playerVector[i]->IsPlayerClass( TF_CLASS_ENGINEER ) )
					{
						bShouldAnnounceLastEngineerBotDeath = false;
						break;
					}
				}
			}

			if ( bShouldAnnounceLastEngineerBotDeath )
			{
				bool bEngineerTeleporterInTheWorld = false;
				for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
				{
					CBaseObject* pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
					if ( pObj->GetType() == OBJ_TELEPORTER && pObj->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
					{
						bEngineerTeleporterInTheWorld = true;
					}
				}

				if ( bEngineerTeleporterInTheWorld )
				{
					TFGameRules()->BroadcastSound( 255, "Announcer.MVM_An_Engineer_Bot_Is_Dead_But_Not_Teleporter" );
				}
				else
				{
					TFGameRules()->BroadcastSound( 255, "Announcer.MVM_An_Engineer_Bot_Is_Dead" );
				}
			}
		}

		// remove this bot from following flag
		for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
			{
				CCaptureFlag *flag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
				flag->RemoveFollower( this );
			}
		}
	} // MvM

	if ( HasSpawner() )
	{
		GetSpawner()->OnBotKilled( this );
	}

	if ( IsInASquad() )
	{
		LeaveSquad();
	}

	CTFNavArea *lastArea = (CTFNavArea *)GetLastKnownArea();
	if ( lastArea )
	{
		// remove us from old visible set
		NavAreaCollector wasVisible;
		lastArea->ForAllPotentiallyVisibleAreas( wasVisible );

		int i;
		for( i=0; i<wasVisible.m_area.Count(); ++i )
		{
			CTFNavArea *area = (CTFNavArea *)wasVisible.m_area[i];
			area->RemovePotentiallyVisibleActor( this );
		}
	}


	if ( info.GetInflictor() && info.GetInflictor()->GetTeamNumber() != GetTeamNumber() )
	{
		CObjectSentrygun *sentrygun = dynamic_cast< CObjectSentrygun * >( info.GetInflictor() );

		if ( sentrygun )
		{
			// we were killed by an enemy sentry - remember it
			RememberEnemySentry( sentrygun, GetAbsOrigin() );
		}
	}

	StopIdleSound();
}


//-----------------------------------------------------------------------------------------------------
CTeamControlPoint *CTFBot::SelectPointToCapture( CUtlVector< CTeamControlPoint * > *captureVector ) const
{
	if ( !captureVector || captureVector->Count() == 0 )
	{
		return NULL;
	}

	if ( captureVector->Count() == 1 )
	{
		// only one choice
		return captureVector->Element(0);
	}

	// if we're capturing a point, stay on it
	if ( const_cast< CTFBot * >( this )->IsCapturingPoint() )
	{
		CTriggerAreaCapture *trigger = const_cast< CTFBot * >( this )->GetControlPointStandingOn();
		if ( trigger )
		{
			return trigger->GetControlPoint();
		}
	}

	// if we're near a point that is being captured, go help (in the event multiple points are being simultaneously captured)
	CTeamControlPoint *closestPoint = SelectClosestControlPointByTravelDistance( captureVector );
	if ( closestPoint )
	{
		bool alwaysUseClosest = false;


		if ( IsPointBeingCaptured( closestPoint ) || alwaysUseClosest )
		{
			return closestPoint;
		}
	}

	// if any point is being captured by our team, go help
	for( int i=0; i<captureVector->Count(); ++i )
	{
		CTeamControlPoint *point = captureVector->Element(i);

		if ( IsPointBeingCaptured( point ) )
		{
			return point;
		}
	}

	// no points are currently being captured - pick the point with the least combat
	CTeamControlPoint *safestPoint = NULL;
	float safestPointCombat = FLT_MAX;
	bool areAllPointsCombatFree = true;

	for( int i=0; i<captureVector->Count(); ++i )
	{
		CTeamControlPoint *point = captureVector->Element(i);
		CTFNavArea *pointArea = TheTFNavMesh()->GetControlPointCenterArea( point->GetPointIndex() );

		if ( !pointArea )
		{
			continue;
		}

		float combat = pointArea->GetCombatIntensity();

		const float minCombat = 0.1f;
		if ( combat > minCombat )
		{
			areAllPointsCombatFree = false;
		}

		if ( combat < safestPointCombat )
		{
			safestPoint = point;
			safestPointCombat = combat;
		}
	}

	// if no points are in combat, pick a random point
	if ( areAllPointsCombatFree )
	{
		const float decisionPeriod = 60.0f;
		int which = captureVector->Count() * TransientlyConsistentRandomValue( decisionPeriod );
		which = clamp( which, 0, captureVector->Count()-1 );

		return captureVector->Element( which );
	}

	// choose the point with the least combat
	return safestPoint;
}


//---------------------------------------------------------------------------------------------
CTeamControlPoint *CTFBot::SelectPointToDefend( CUtlVector< CTeamControlPoint * > *defendVector ) const
{
	if ( defendVector && defendVector->Count() > 0 )
	{
		if ( HasAttribute( CTFBot::PRIORITIZE_DEFENSE ) )
		{
			return SelectClosestControlPointByTravelDistance( defendVector );
		}

		return defendVector->Element( RandomInt( 0, defendVector->Count()-1 ) );
	}

	return NULL;
}


//-----------------------------------------------------------------------------------------------------
/**
 * Return the point we have decided to capture or defend
 */
CTeamControlPoint *CTFBot::GetMyControlPoint( void ) const
{
	if ( m_myControlPoint != NULL && !m_evaluateControlPointTimer.IsElapsed() )
	{
		return m_myControlPoint;
	}

	m_evaluateControlPointTimer.Start( RandomFloat( 1.0f, 2.0f ) );


	CUtlVector< CTeamControlPoint * > captureVector;
	TFGameRules()->CollectCapturePoints( const_cast< CTFBot * >( this ), &captureVector );

	CUtlVector< CTeamControlPoint * > defendVector;
	TFGameRules()->CollectDefendPoints( const_cast< CTFBot * >( this ), &defendVector );

	bool bOnOffense = ( TFGameRules()->IsAttackDefenseMode() && GetTeamNumber() == TF_TEAM_BLUE );
	
	// Some attributes and classes prioritize defense (unless we're on the attacking team)
	if ( ( ( IsPlayerClass( TF_CLASS_ENGINEER ) || IsPlayerClass( TF_CLASS_SNIPER ) ) && !bOnOffense ) || HasAttribute( CTFBot::PRIORITIZE_DEFENSE ) )
	{
		if ( defendVector.Count() > 0 )
		{
			m_myControlPoint = SelectPointToDefend( &defendVector );
			return m_myControlPoint;
		}
	}

	// if we have a point we can capture - do it
	m_myControlPoint = SelectPointToCapture( &captureVector );

	if ( m_myControlPoint == NULL )
	{
		// otherwise, defend our point(s) from capture
		m_myControlPoint = SelectPointToDefend( &defendVector );
	}

	return m_myControlPoint;
}


//-----------------------------------------------------------------------------------------------------
// Return flag we want to fetch
CCaptureFlag *CTFBot::GetFlagToFetch( void ) const
{
	CUtlVector<CCaptureFlag *> flagsVector;
	int nCarriedFlags = 0;

	// MvM Engineer bot never pick up a flag
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( GetTeamNumber() == TF_TEAM_PVE_INVADERS && IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			return NULL;
		}

		if( HasAttribute( CTFBot::IGNORE_FLAG ) )
		{
			return NULL;
		}

		if ( TFGameRules()->IsMannVsMachineMode() && HasFlagTaget() )
		{
			return GetFlagTarget();
		}
	}

	// Collect flags
	for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
	{
		CCaptureFlag *flag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );

		if ( flag->IsDisabled() )
			continue;

		// If I'm carrying a flag, look for mine and early-out
		if ( HasTheFlag() )
		{
			if ( flag->GetOwnerEntity() == this )
			{
				return flag;
			}
		}

		switch( flag->GetType() )
		{
		case TF_FLAGTYPE_CTF:
			if ( flag->GetTeamNumber() == GetEnemyTeam( GetTeamNumber() ) )
			{
				// we want to steal the other team's flag
				flagsVector.AddToTail( flag );
			}
			break;

		case TF_FLAGTYPE_ATTACK_DEFEND:
		case TF_FLAGTYPE_TERRITORY_CONTROL:
		case TF_FLAGTYPE_INVADE:
			if ( flag->GetTeamNumber() != GetEnemyTeam( GetTeamNumber() ) )
			{
				// we want to move our team's flag or a neutral flag
				flagsVector.AddToTail( flag );
			}
			break;
		}

		if ( flag->IsStolen() )
		{
			nCarriedFlags++;
		}
	}

	CCaptureFlag *pClosestFlag = NULL;
	float flClosestFlagDist = FLT_MAX;
	CCaptureFlag *pClosestUncarriedFlag = NULL;
	float flClosestUncarriedFlagDist = FLT_MAX;

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		int nMinFollower = INT_MAX;

		FOR_EACH_VEC( flagsVector, i )
		{
			CCaptureFlag *pFlag = flagsVector[i];
			if ( pFlag )
			{
				// find the one which needs the most love
				if ( pFlag->GetNumFollowers() < nMinFollower )
				{
					nMinFollower = pFlag->GetNumFollowers();

					pClosestFlag = NULL;
					flClosestFlagDist = FLT_MAX;
					pClosestUncarriedFlag = NULL;
					flClosestUncarriedFlagDist = FLT_MAX;
				}
				
				if ( pFlag->GetNumFollowers() == nMinFollower )
				{
					// Find the closest
					float flDist = ( pFlag->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
					if ( flDist < flClosestFlagDist )
					{
						pClosestFlag = pFlag;
						flClosestFlagDist = flDist;
					}

					// Find the closest uncarried
					if ( nCarriedFlags < flagsVector.Count() && !pFlag->IsStolen() )
					{
						if ( flDist < flClosestUncarriedFlagDist )
						{
							pClosestUncarriedFlag = flagsVector[i];
							flClosestUncarriedFlagDist = flDist;
						}
					}
				}
			}
		}
	}
	else
	{
		FOR_EACH_VEC( flagsVector, i )
		{
			if ( flagsVector[i] )
			{
				// Find the closest
				float flDist = ( flagsVector[i]->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
				if ( flDist < flClosestFlagDist )
				{
					pClosestFlag = flagsVector[i];
					flClosestFlagDist = flDist;
				}

				// Find the closest uncarried
				if ( nCarriedFlags < flagsVector.Count() && !flagsVector[i]->IsStolen() )
				{
					if ( flDist < flClosestUncarriedFlagDist )
					{
						pClosestUncarriedFlag = flagsVector[i];
						flClosestUncarriedFlagDist = flDist;
					}
				}
			}
		}
	}

	// If we have an uncarried flag, prioritize
	if ( pClosestUncarriedFlag )
		return pClosestUncarriedFlag;

	return pClosestFlag;
}


//-----------------------------------------------------------------------------------------------------
// Return capture zone for our flag(s)
CCaptureZone *CTFBot::GetFlagCaptureZone( void ) const
{
	for( int i=0; i<ICaptureZoneAutoList::AutoList().Count(); ++i )
	{
		CCaptureZone *zone = static_cast< CCaptureZone* >( ICaptureZoneAutoList::AutoList()[i] );
		if ( zone->GetTeamNumber() == GetTeamNumber() )
		{
			return zone;
		}
	}

	return NULL;
}



//-----------------------------------------------------------------------------------------------------
void CTFBot::ClearMyControlPoint( void )
{
	m_myControlPoint = NULL;
	m_evaluateControlPointTimer.Invalidate();
}


//-----------------------------------------------------------------------------------------------------
/**
 * Return true if no enemy has contested any point yet
 */
bool CTFBot::AreAllPointsUncontestedSoFar( void ) const
{
	CTeamControlPointMaster *master = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( master )
	{
		for( int i=0; i<master->GetNumPoints(); ++i )
		{
			CTeamControlPoint *point = master->GetControlPoint( i );
			
			if ( point && point->HasBeenContested() )
				return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------------------------------
// Return true if the given point is being captured
bool CTFBot::IsPointBeingCaptured( CTeamControlPoint *point ) const
{
	if ( point == NULL )
		return false;

	if ( point->LastContestedAt() > 0.0f && ( gpGlobals->curtime - point->LastContestedAt() ) < 5.0f )
	{
		// the point is, or was very recently, contested
		return true;
	}

	return false;
}


//---------------------------------------------------------------------------------------------
// Return true if any point is being captured
bool CTFBot::IsAnyPointBeingCaptured( void ) const
{
	CTeamControlPointMaster *master = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( master )
	{
		for( int i=0; i<master->GetNumPoints(); ++i )
		{
			CTeamControlPoint *point = master->GetControlPoint( i );

			if ( IsPointBeingCaptured( point ) )
				return true;
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
// Return true if we are within a short travel distance of the current point
bool CTFBot::IsNearPoint( CTeamControlPoint *point ) const
{
	CTFNavArea *myArea = GetLastKnownArea();

	if ( !myArea || !point )
	{
		return false;
	}

	CTFNavArea *pointArea = TheTFNavMesh()->GetControlPointCenterArea( point->GetPointIndex() );

	if ( !pointArea )
	{
		return false;
	}

	float travelToPoint = fabs( myArea->GetIncursionDistance( GetTeamNumber() ) - pointArea->GetIncursionDistance( GetTeamNumber() ) );

	return travelToPoint < tf_bot_near_point_travel_distance.GetFloat();
}


//---------------------------------------------------------------------------------------------
// Return time left to capture the point before we lose the game
float CTFBot::GetTimeLeftToCapture( void ) const
{
	if ( TFGameRules()->IsInKothMode() )
	{
		if ( TFGameRules()->GetKothTeamTimer( GetEnemyTeam( GetTeamNumber() ) ) )
		{
			return TFGameRules()->GetKothTeamTimer( GetEnemyTeam( GetTeamNumber() ) )->GetTimeRemaining();
		}
	}
	else if ( TFGameRules()->GetActiveRoundTimer() )
	{
		return TFGameRules()->GetActiveRoundTimer()->GetTimeRemaining();
	}

	return 0.0f;
}


//-----------------------------------------------------------------------------------------------------
// Do internal setup when control point changes
void CTFBot::SetupSniperSpotAccumulation( void )
{
	VPROF_BUDGET( "CTFBot::SetupSniperSpotAccumulation", "NextBot" );

	CBaseEntity *goalEntity = NULL;

	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		// try to find a payload cart to guard
		CTeamTrainWatcher *trainWatcher = TFGameRules()->GetPayloadToPush( GetTeamNumber() );

		if ( !trainWatcher )
		{
			trainWatcher = TFGameRules()->GetPayloadToBlock( GetTeamNumber() );
		}

		if ( trainWatcher )
		{
			goalEntity = trainWatcher->GetTrainEntity();
		}
	}
	else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
	{
		goalEntity = GetMyControlPoint();
	}

	if ( !goalEntity )
	{
		ClearSniperSpots();
		return;
	}

	if ( goalEntity == m_snipingGoalEntity )
	{
		// if goal has moved too much (ie: payload cart), recompute our spots
		Vector toGoal = m_snipingGoalEntity->WorldSpaceCenter() - m_lastSnipingGoalEntityPosition;

		if ( toGoal.IsLengthLessThan( tf_bot_sniper_goal_entity_move_tolerance.GetFloat() ) )
		{
			// already set up
			return;
		}
	}

	ClearSniperSpots();

	int myTeam = GetTeamNumber();
	int enemyTeam = ( myTeam == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;

	bool isDefendingPoint = false;
	CTFNavArea *goalEntityArea = NULL;

	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		// the cart is owned by the invaders
		isDefendingPoint = ( goalEntity->GetTeamNumber() != myTeam );

		// Note(misyl): This GETNAVAREA_CHECK_GROUND (raw flag) was wrong, and is mapped to a bool of 'anyZ'.
		//   -> goalEntityArea = (CTFNavArea *)TheTFNavMesh()->GetNearestNavArea( goalEntity->WorldSpaceCenter(), GETNAVAREA_CHECK_GROUND, 500.0f );
		// Changed to fix, but maintaining the "anyZ" for compat.
		const bool bAnyZ = true; // compat.
		const bool bCheckLOS = false;
		const bool bCheckGround = true;
		goalEntityArea = (CTFNavArea *)TheTFNavMesh()->GetNearestNavArea( goalEntity->WorldSpaceCenter(), bAnyZ, 500.0f, bCheckLOS, bCheckGround );
	}
	else
	{
		isDefendingPoint = ( GetMyControlPoint()->GetOwner() == myTeam );
		goalEntityArea = TheTFNavMesh()->GetControlPointCenterArea( GetMyControlPoint()->GetPointIndex() );
	}

	// we are sniping a different control point - setup for new point accumulation
	m_sniperVantageAreaVector.RemoveAll();
	m_sniperTheaterAreaVector.RemoveAll();

	if ( !goalEntityArea )
	{
		return;
	}

	for( int i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)TheNavAreas[i];

		if ( !area->IsReachableByTeam( myTeam ) || !area->IsReachableByTeam( enemyTeam ) )
		{
			continue;
		}

		if ( area->GetIncursionDistance( enemyTeam ) <= goalEntityArea->GetIncursionDistance( enemyTeam ) )
		{
			m_sniperTheaterAreaVector.AddToTail( area );
		}

		// if this is my point, I can stand on it, or go a bit beyond it
		float myIncursionTolerance = tf_bot_sniper_spot_point_tolerance.GetFloat();

		if ( !isDefendingPoint )
		{
			// not my point, keep back from it a bit
			myIncursionTolerance *= -1.0f;
		}
		
		if ( area->GetIncursionDistance( myTeam ) <= goalEntityArea->GetIncursionDistance( myTeam ) + myIncursionTolerance )
		{
			m_sniperVantageAreaVector.AddToTail( area );
		}
	}

	m_snipingGoalEntity = goalEntity;
	m_lastSnipingGoalEntityPosition = goalEntity->WorldSpaceCenter();
}


//-----------------------------------------------------------------------------------------------------
// Randomly sample points within candidate areas to find good sniping positions
void CTFBot::AccumulateSniperSpots( void )
{
	VPROF_BUDGET( "CTFBot::AccumulateSniperSpots", "NextBot" );

	SetupSniperSpotAccumulation();

	if ( m_sniperVantageAreaVector.Count() == 0 || m_sniperTheaterAreaVector.Count() == 0 )
	{
		// retry every so often to catch cases where the incursion data is invalid during setup time
		// due to blocked/closed off areas, etc.
		if ( m_retrySniperSpotSetupTimer.IsElapsed() )
		{
			// retry
			ClearSniperSpots();
		}

		return;
	}

	SniperSpotInfo info;

	for( int count=0; count<tf_bot_sniper_spot_search_count.GetInt(); ++count )
	{
		// pick a random vantage area to sample
		int which = RandomInt( 0, m_sniperVantageAreaVector.Count()-1 );
		info.m_vantageArea = m_sniperVantageAreaVector[ which ];
		info.m_vantageSpot = info.m_vantageArea->GetRandomPoint();

		// pick a random theater area to sample
		which = RandomInt( 0, m_sniperTheaterAreaVector.Count()-1 );
		info.m_theaterArea = m_sniperTheaterAreaVector[ which ];
		info.m_theaterSpot = info.m_theaterArea->GetRandomPoint();

		info.m_range = ( info.m_vantageSpot - info.m_theaterSpot ).Length();
		if ( info.m_range < tf_bot_sniper_spot_min_range.GetFloat() )
		{
			// not long enough sightline
			continue;
		}

		for( int i=0; i<m_sniperSpotVector.Count(); ++i )
		{
			if ( ( info.m_vantageSpot - m_sniperSpotVector[i].m_vantageSpot ).IsLengthLessThan( tf_bot_sniper_spot_epsilon.GetFloat() ) )
			{
				// too close to existing spot
				continue;
			}
		}

		Vector eyeOffset( 0, 0, 60.0f );
		if ( IsLineOfFireClear( info.m_vantageSpot + eyeOffset, info.m_theaterSpot + eyeOffset ) )
		{
			// valid spot

			// maximize the time it takes the enemy to get to us
			info.m_advantage = info.m_vantageArea->GetIncursionDistance( GetEnemyTeam( GetTeamNumber() ) ) - info.m_theaterArea->GetIncursionDistance( GetEnemyTeam( GetTeamNumber() ) );

			// if we have already maxxed out our sniper spots, replace the worst one if this is better
			if ( m_sniperSpotVector.Count() >= tf_bot_sniper_spot_max_count.GetInt() )
			{
				int worst = -1;

				for( int i=0; i<m_sniperSpotVector.Count(); ++i )
				{
					if ( worst < 0 || m_sniperSpotVector[i].m_advantage < m_sniperSpotVector[ worst ].m_advantage )
					{
						worst = i;
					}
				}

				// if our new spot is better, replace it
				if ( info.m_advantage > m_sniperSpotVector[ worst ].m_advantage )
				{
					m_sniperSpotVector[ worst ] = info;
				}
			}
			else
			{
				m_sniperSpotVector.AddToTail( info );
			}
		}
	}

	if ( IsDebugging( NEXTBOT_BEHAVIOR ) )
	{
		for( int i=0; i<m_sniperSpotVector.Count(); ++i )
		{
			NDebugOverlay::Cross3D( m_sniperSpotVector[i].m_vantageSpot, 5.0f, 255, 0, 255, true, 0.1f );
			NDebugOverlay::Line( m_sniperSpotVector[i].m_vantageSpot, m_sniperSpotVector[i].m_theaterSpot, 0, 200, 0, true, 0.1f );
		}
	}
}



//-----------------------------------------------------------------------------------------------------
void CTFBot::ClearSniperSpots( void )
{
	m_sniperSpotVector.RemoveAll();
	m_sniperVantageAreaVector.RemoveAll();
	m_sniperTheaterAreaVector.RemoveAll();
	m_snipingGoalEntity = NULL;
	m_retrySniperSpotSetupTimer.Start( RandomFloat( 5.0f, 10.0f ) );
}



//---------------------------------------------------------------------------------------------
class CCollectReachableObjects : public ISearchSurroundingAreasFunctor
{
public:
	CCollectReachableObjects( const CTFBot *me, float maxRange, const CUtlVector< CHandle< CBaseEntity > > &potentialVector, CUtlVector< CHandle< CBaseEntity > > *collectionVector ) : m_potentialVector( potentialVector )
	{
		m_me = me;
		m_maxRange = maxRange;
		m_collectionVector = collectionVector;
	}

	virtual bool operator() ( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar )
	{
		// do any of the potential objects overlap this area?
		FOR_EACH_VEC( m_potentialVector, it )
		{
			CBaseEntity *obj = m_potentialVector[ it ];

			if ( obj && area->Contains( obj->WorldSpaceCenter() ) )
			{
				// reachable - keep it
				if ( !m_collectionVector->HasElement( obj ) )
				{
					m_collectionVector->AddToTail( obj );
				}
			}
		}
		return true;
	}

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar )
	{
		if ( adjArea->IsBlocked( m_me->GetTeamNumber() ) )
		{
			return false;
		}

		if ( travelDistanceSoFar > m_maxRange )
		{
			// too far away
			return false;
		}

		return currentArea->IsContiguous( adjArea );
	}

	const CTFBot *m_me;
	float m_maxRange;
	const CUtlVector< CHandle< CBaseEntity > > &m_potentialVector;
	CUtlVector< CHandle< CBaseEntity > > *m_collectionVector;
};


//
// Search outwards from startSearchArea and collect all reachable objects from the given list that pass the given filter
// Items in selectedObjectVector will be approximately sorted in nearest-to-farthest order (because of SearchSurroundingAreas)
//
void CTFBot::SelectReachableObjects( const CUtlVector< CHandle< CBaseEntity > > &candidateObjectVector, 
									 CUtlVector< CHandle< CBaseEntity > > *selectedObjectVector, 
									 const INextBotFilter &filter, 
									 CNavArea *startSearchArea, 
									 float maxRange ) const
{
	if ( startSearchArea == NULL || selectedObjectVector == NULL )
		return;

	selectedObjectVector->RemoveAll();

	// filter candidate objects
	CUtlVector< CHandle< CBaseEntity > > filteredObjectVector;
	for( int i=0; i<candidateObjectVector.Count(); ++i )
	{
		if ( filter.IsSelected( candidateObjectVector[i] ) )
		{
			filteredObjectVector.AddToTail( candidateObjectVector[i] );
		}
	}

	// only keep those that are reachable by us
	CCollectReachableObjects collector( this, maxRange, filteredObjectVector, selectedObjectVector );
	SearchSurroundingAreas( startSearchArea, collector );
}


//---------------------------------------------------------------------------------------------
bool CTFBot::IsAmmoLow( void ) const
{
	CTFWeaponBase *myWeapon = m_Shared.GetActiveTFWeapon();
	if ( myWeapon )
	{
		if ( myWeapon->GetWeaponID() == TF_WEAPON_WRENCH )
		{
			// wrench is special. it's a melee weapon that wants ammo - metal
			return ( GetAmmoCount( TF_AMMO_METAL ) <= 0 );
		}

		if ( myWeapon->IsMeleeWeapon() )
		{
			// we never run out of ammo with a melee weapon
			return false;
		}

		// no projectile, no ammo needed
		const char *weaponAlias = WeaponIdToAlias( myWeapon->GetWeaponID() );
		if ( weaponAlias )
		{
			WEAPON_FILE_INFO_HANDLE	weaponInfoHandle = LookupWeaponInfoSlot( weaponAlias );
			if ( weaponInfoHandle != GetInvalidWeaponInfoHandle() )
			{
				CTFWeaponInfo *weaponInfo = static_cast< CTFWeaponInfo * >( GetFileWeaponInfoFromHandle( weaponInfoHandle ) );
				if ( weaponInfo && weaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile == TF_PROJECTILE_NONE )
				{
					// we don't shoot anything, so we don't need ammo
					return false;
				}
			}
		}

		float ratio = (float)GetAmmoCount( TF_AMMO_PRIMARY ) / (float)( const_cast< CTFBot * >( this )->GetMaxAmmo( TF_AMMO_PRIMARY ) );

		if ( ratio < 0.2f )
		{
			return true;
		}
		//if ( !myWeapon->HasPrimaryAmmo() && myWeapon->GetWeaponID() != TF_WEAPON_BUILDER && myWeapon->GetWeaponID() != TF_WEAPON_MEDIGUN )
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
bool CTFBot::IsAmmoFull( void ) const
{
	bool isPrimaryFull = GetAmmoCount( TF_AMMO_PRIMARY ) >= const_cast< CTFBot * >( this )->GetMaxAmmo( TF_AMMO_PRIMARY );
	bool isSecondaryFull = GetAmmoCount( TF_AMMO_SECONDARY ) >= const_cast< CTFBot * >( this )->GetMaxAmmo( TF_AMMO_SECONDARY );

	if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		// wrench is special. it's a melee weapon that wants ammo - metal
		return ( GetAmmoCount( TF_AMMO_METAL ) >= 200 ) && isPrimaryFull && isSecondaryFull;
	}

	return isPrimaryFull && isSecondaryFull;

/*
	CTFWeaponBase *myWeapon = m_Shared.GetActiveTFWeapon();
	if ( myWeapon )
	{
		if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			// wrench is special. it's a melee weapon that wants ammo - metal
			return ( GetAmmoCount( TF_AMMO_METAL ) >= 200 );
		}

		if ( myWeapon->IsMeleeWeapon() )
		{
			// we never run out of ammo with a melee weapon
			return true;
		}

		// no projectile, no ammo needed
		const char *weaponAlias = WeaponIdToAlias( myWeapon->GetWeaponID() );
		if ( weaponAlias )
		{
			WEAPON_FILE_INFO_HANDLE	weaponInfoHandle = LookupWeaponInfoSlot( weaponAlias );
			if ( weaponInfoHandle != GetInvalidWeaponInfoHandle() )
			{
				CTFWeaponInfo *weaponInfo = static_cast< CTFWeaponInfo * >( GetFileWeaponInfoFromHandle( weaponInfoHandle ) );
				if ( weaponInfo && weaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile == TF_PROJECTILE_NONE )
				{
					// we don't shoot anything, so we don't need ammo
					return true;
				}
			}
		}

		bool isPrimaryFull = GetAmmoCount( TF_AMMO_PRIMARY ) >= const_cast< CTFBot * >( this )->GetMaxAmmo( TF_AMMO_PRIMARY );
		bool isSecondaryFull = GetAmmoCount( TF_AMMO_SECONDARY ) >= const_cast< CTFBot * >( this )->GetMaxAmmo( TF_AMMO_SECONDARY );

		return isPrimaryFull && isSecondaryFull;
	}

	return false;
*/
}


bool CTFBot::IsDormantWhenDead( void ) const
{
	return false;
}


//-----------------------------------------------------------------------------------------------------
/**
 * When someone fires their weapon
 */
void CTFBot::OnWeaponFired( CBaseCombatCharacter *whoFired, CBaseCombatWeapon *weapon )
{
	VPROF_BUDGET( "CTFBot::OnWeaponFired", "NextBot" );

	BaseClass::OnWeaponFired( whoFired, weapon );

	if ( !whoFired || !whoFired->IsAlive() )
		return;

	if ( IsRangeGreaterThan( whoFired, tf_bot_notice_gunfire_range.GetFloat() ) )
		return;

	int noticeChance = 100;

	if ( IsQuietWeapon( (CTFWeaponBase *)weapon ) )
	{
		if ( IsRangeGreaterThan( whoFired, tf_bot_notice_quiet_gunfire_range.GetFloat() ) )
		{
			// too far away to hear in any event
			return;
		}

		switch( GetDifficulty() )
		{
		case EASY:
			noticeChance = 10;
			break;

		case NORMAL:
			noticeChance = 30;
			break;

		case HARD:
			noticeChance = 60;
			break;

		default:
		case EXPERT:
			noticeChance = 90;
			break;
		}

		if ( IsEnvironmentNoisy() )
		{
			// less likely to notice with all the noise
			noticeChance /= 2;
		}
	}
	else if ( IsRangeLessThan( whoFired, 1000.0f ) )
	{
		// loud gunfire in our area - it's now "noisy" for a bit
		m_noisyTimer.Start( 3.0f );
	}

	if ( RandomInt( 1, 100 ) > noticeChance )
	{
		return;
	}

	// notice the gunfire
	GetVisionInterface()->AddKnownEntity( whoFired );
}


//-----------------------------------------------------------------------------------------------------
// Return true if we match the given debug symbol
bool CTFBot::IsDebugFilterMatch( const char *name ) const
{
	// player classname
	if ( !Q_strnicmp( name, const_cast< CTFBot * >( this )->GetPlayerClass()->GetName(), Q_strlen( name ) ) )
	{
		return true;
	}

	return BaseClass::IsDebugFilterMatch( name );
}


//-----------------------------------------------------------------------------------------------------
class CFindClosestPotentiallyVisibleAreaToPos
{
public:
	CFindClosestPotentiallyVisibleAreaToPos( const Vector &pos )
	{
		m_pos = pos;
		m_closeArea = NULL;
		m_closeRangeSq = FLT_MAX;
	}

	bool operator() ( CNavArea *baseArea )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;

		Vector close;
		area->GetClosestPointOnArea( m_pos, &close );

		float rangeSq = ( close - m_pos ).LengthSqr();
		if ( rangeSq < m_closeRangeSq )
		{
			m_closeArea = area;
			m_closeRangeSq = rangeSq;
		}

		return true;
	}

	Vector m_pos;
	CTFNavArea *m_closeArea;
	float m_closeRangeSq;
};


//-----------------------------------------------------------------------------------------------------
// Update our view to watch where members of the given team will be coming from
void CTFBot::UpdateLookingAroundForIncomingPlayers( bool lookForEnemies )
{
	if ( !m_lookAtEnemyInvasionAreasTimer.IsElapsed() )
		return;

	const float maxLookInterval = 1.0f;
	m_lookAtEnemyInvasionAreasTimer.Start( RandomFloat( 0.333f, maxLookInterval ) );

	float minGazeRange = m_Shared.InCond( TF_COND_ZOOMED ) ? 750.0f : 150.0f;

	CTFNavArea *myArea = GetLastKnownArea();
	if ( myArea )
	{
		int team = GetTeamNumber();

		// if we want to look where teammates come from, we need to pass in
		// the *enemy* team, since the method collects *enemy* invasion areas
		if ( !lookForEnemies )
		{
			team = GetEnemyTeam( team );
		}

		const CUtlVector< CTFNavArea * > &invasionAreaVector = myArea->GetEnemyInvasionAreaVector( team );

		if ( invasionAreaVector.Count() > 0 )
		{
			// try to not look directly at walls
			const int retryCount = 20.0f;
			for( int r=0; r<retryCount; ++r )
			{
				int which = RandomInt( 0, invasionAreaVector.Count()-1 );
				Vector gazeSpot = invasionAreaVector[ which ]->GetRandomPoint() + Vector( 0, 0, 0.75f * HumanHeight );

				if ( IsRangeGreaterThan( gazeSpot, minGazeRange ) && GetVisionInterface()->IsLineOfSightClear( gazeSpot ) )
				{
					// use maxLookInterval so these looks override body aiming from path following
					GetBodyInterface()->AimHeadTowards( gazeSpot, IBody::INTERESTING, maxLookInterval, NULL, "Looking toward enemy invasion areas" );
					break;
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------------------------------
/**
 * Update our view to keep an eye on areas where the enemy will be coming from
 */
void CTFBot::UpdateLookingAroundForEnemies( void )
{
	if ( !m_isLookingAroundForEnemies )
		return;

	if ( HasAttribute( CTFBot::IGNORE_ENEMIES ) )
		return;

	if ( m_Shared.IsControlStunned() )
		return;

	const float maxLookInterval = 1.0f;

	const CKnownEntity *known = GetVisionInterface()->GetPrimaryKnownThreat();

	if ( known )
	{
		if ( known->IsVisibleInFOVNow() )
		{
			if ( IsPlayerClass( TF_CLASS_SPY ) && 
				 GetDifficulty() >= CTFBot::HARD &&
				 m_Shared.InCond( TF_COND_DISGUISED ) &&
				 !m_Shared.IsStealthed() )
			{
				// smart Spies don't look at their victims until it's too late...
				// look around at where *teammates* will be coming from to fool the enemy
				UpdateLookingAroundForIncomingPlayers( LOOK_FOR_FRIENDS );
				return;
			}

			// I see you!
			GetBodyInterface()->AimHeadTowards( known->GetEntity(), IBody::CRITICAL, 1.0f, NULL, "Aiming at a visible threat" );
			return;
		}

/* apparently sounds update last known position...
		if ( known->WasEverVisible() && known->GetTimeSinceLastSeen() < 3.0f )
		{
			// I saw you just a moment ago...
			GetBodyInterface()->AimHeadTowards( known->GetLastKnownPosition() + GetClassEyeHeight(), IBody::IMPORTANT, 1.0f, NULL, "Aiming at a last known threat position" );
			return;
		}
*/

		// known but not currently visible (I know you're around here somewhere)

		// if there is unobstructed space between us, turn around
		if ( IsLineOfSightClear( known->GetEntity(), IGNORE_ACTORS ) )
		{
			Vector toThreat = known->GetEntity()->GetAbsOrigin() - GetAbsOrigin();
			float threatRange = toThreat.NormalizeInPlace();

			float aimError = M_PI/6.0f;

			float s, c;
			FastSinCos( aimError, &s, &c );

			float error = threatRange * s;
			Vector imperfectAimSpot = known->GetEntity()->WorldSpaceCenter();
			imperfectAimSpot.x += RandomFloat( -error, error );
			imperfectAimSpot.y += RandomFloat( -error, error );

			GetBodyInterface()->AimHeadTowards( imperfectAimSpot, IBody::IMPORTANT, 1.0f, NULL, "Turning around to find threat out of our FOV" );
			return;
		}
			
		if ( !IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			// look toward potentially visible area nearest the last known position
			CTFNavArea *myArea = GetLastKnownArea();
			if ( myArea )
			{
				const CTFNavArea *closeArea = NULL;
				CFindClosestPotentiallyVisibleAreaToPos find( known->GetLastKnownPosition() );
				myArea->ForAllPotentiallyVisibleAreas( find );

				closeArea = find.m_closeArea;

				if ( closeArea )
				{
					// try to not look directly at walls
					const int retryCount = 10.0f;
					for( int r=0; r<retryCount; ++r )
					{
						Vector gazeSpot = closeArea->GetRandomPoint() + Vector( 0, 0, 0.75f * HumanHeight );

						if ( GetVisionInterface()->IsLineOfSightClear( gazeSpot ) )
						{
							// use maxLookInterval so these looks override body aiming from path following
							GetBodyInterface()->AimHeadTowards( gazeSpot, IBody::IMPORTANT, maxLookInterval, NULL, "Looking toward potentially visible area near known but hidden threat" );
							return;
						}
					}					

					// can't find a clear line to look along
					if ( IsDebugging( NEXTBOT_VISION | NEXTBOT_ERRORS ) )
					{
						ConColorMsg( Color( 255, 255, 0, 255 ), "%3.2f: %s can't find clear line to look at potentially visible near known but hidden entity %s(#%d)\n", 
										gpGlobals->curtime,
										GetDebugIdentifier(),
										known->GetEntity()->GetClassname(),
										known->GetEntity()->entindex() );
					}
				}
				else if ( IsDebugging( NEXTBOT_VISION | NEXTBOT_ERRORS ) )
				{
					ConColorMsg( Color( 255, 255, 0, 255 ), "%3.2f: %s no potentially visible area to look toward known but hidden entity %s(#%d)\n", 
									gpGlobals->curtime,
									GetDebugIdentifier(),
									known->GetEntity()->GetClassname(),
									known->GetEntity()->entindex() );
				}
			}

			return;
		}
	}

	// no known threat - look toward where enemies will come from
	UpdateLookingAroundForIncomingPlayers( LOOK_FOR_ENEMIES );
}


//---------------------------------------------------------------------------------------------
class CFindVantagePoint : public ISearchSurroundingAreasFunctor
{
public:
	CFindVantagePoint( int enemyTeamIndex )
	{
		m_enemyTeamIndex = enemyTeamIndex;
		m_vantageArea = NULL;
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;

		CTeam *enemyTeam = GetGlobalTeam( m_enemyTeamIndex );
		for( int i=0; i<enemyTeam->GetNumPlayers(); ++i )
		{
			CTFPlayer *enemy = (CTFPlayer *)enemyTeam->GetPlayer(i);

			if ( !enemy->IsAlive() || !enemy->GetLastKnownArea() )
				continue;

			CTFNavArea *enemyArea = (CTFNavArea *)enemy->GetLastKnownArea();
			if ( enemyArea->IsCompletelyVisible( area ) )
			{
				// nearby area from which we can see the enemy team
				m_vantageArea = area;
				return false;
			}
		}

		return true;
	}

	int m_enemyTeamIndex;
	CTFNavArea *m_vantageArea;
};


//-----------------------------------------------------------------------------------------------------
// Return a nearby area where we can see a member of the enemy team
CTFNavArea *CTFBot::FindVantagePoint( float maxTravelDistance ) const
{
	CFindVantagePoint find( GetTeamNumber() == TF_TEAM_BLUE ? TF_TEAM_RED : TF_TEAM_BLUE );
	SearchSurroundingAreas( GetLastKnownArea(), find, maxTravelDistance );
	return find.m_vantageArea;
}


//-----------------------------------------------------------------------------------------------------
/**
 * Return perceived danger of threat (0=none, 1=immediate deadly danger)
 * @todo: Move this to contextual query
 * @todo: Differentiate between potential threats (that sentry up ahead along our route) and immediate threats (the sentry I'm in range of)
 */
float CTFBot::GetThreatDanger( CBaseCombatCharacter *who ) const
{
	if ( who == NULL )
		return 0.0f;

	if ( IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		if ( IsRangeGreaterThan( who, tf_bot_sniper_personal_space_range.GetFloat() ) )
		{
			// far away enemies are no threat to a Sniper
			return 0.0f;
		}
	}

	if ( who->IsPlayer() )
	{
		CTFPlayer *player = ToTFPlayer( who );

		// ubers are scary
		if ( player->m_Shared.IsInvulnerable() )
			return 1.0f;

		switch( player->GetPlayerClass()->GetClassIndex() )
		{
		case TF_CLASS_MEDIC:
			return 0.2f;		// 1/5

		case TF_CLASS_ENGINEER:
		case TF_CLASS_SNIPER:
			return 0.4f;		// 2/5

		case TF_CLASS_SCOUT:
		case TF_CLASS_SPY:
		case TF_CLASS_DEMOMAN:
			return 0.6f;		// 3/5

		case TF_CLASS_SOLDIER:
		case TF_CLASS_HEAVYWEAPONS:
			return 0.8f;		// 4/5

		case TF_CLASS_PYRO:
			return 1.0f;		// 5/5
		}

	}
	else
	{
		// sentry gun
		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( who );
		if ( sentry )
		{
			if ( !sentry->IsAlive() || sentry->IsPlacing() || sentry->HasSapper() || sentry->IsPlasmaDisabled() || sentry->IsUpgrading() || sentry->IsBuilding() )
				return 0.0f;

			switch( sentry->GetUpgradeLevel() )
			{
			case 3:		return 1.0f;
			case 2:		return 0.8f;
			default:	return 0.6f;
			}
		}
	}

	return 0.0f;
}


//-----------------------------------------------------------------------------------------------------
/**
 * Return the max range at which we can effectively attack
 */
float CTFBot::GetMaxAttackRange( void ) const
{
	CTFWeaponBase *myWeapon = m_Shared.GetActiveTFWeapon();
	if ( !myWeapon )
		return 0.0f;

	if ( myWeapon->IsMeleeWeapon() )
	{
		return 100.0f;
	}
	
	if ( myWeapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) )
	{
		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			const float flameRange = 350.0f;

			static CSchemaItemDefHandle pItemDef_GiantFlamethrower( "MVM Giant Flamethrower" );

			if ( IsActiveTFWeapon( pItemDef_GiantFlamethrower ) )
			{
				return 2.5f * flameRange;
			}

			return flameRange;
		}

		return 250.0f;
	}

	if ( WeaponID_IsSniperRifle( myWeapon->GetWeaponID() ) )
	{
		// infinite
		return FLT_MAX;
	}

	if ( myWeapon->IsWeapon( TF_WEAPON_ROCKETLAUNCHER ) )
	{
		return 3000.0f;
	}

	// bullet spray weapons, grenades, etc
	// for now, default to infinite so bot always returns fire and doesn't look dumb
	return FLT_MAX;
}


//-----------------------------------------------------------------------------------------------------
/**
 * Return the ideal range at which we can effectively attack
 */
float CTFBot::GetDesiredAttackRange( void ) const
{
	CTFWeaponBase *myWeapon = m_Shared.GetActiveTFWeapon();
	if ( !myWeapon )
		return 0.0f;

	if ( myWeapon->IsWeapon( TF_WEAPON_KNIFE ) )
	{
		// get very close and stab
		return 70.0f;	// 60
	}

	if ( myWeapon->IsMeleeWeapon() )
	{
		return 100.0f;
	}
	
	if ( myWeapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) )
	{
		return 100.0f;
	}

	if ( WeaponID_IsSniperRifle( myWeapon->GetWeaponID() ) )
	{
		// infinite
		return FLT_MAX;
	}

	if ( myWeapon->IsWeapon( TF_WEAPON_ROCKETLAUNCHER ) && !TFGameRules()->IsMannVsMachineMode() )
	{
		return 1250.0f;
	}

	// bullet spray weapons, grenades, etc
	return 500.0f;
}


//-----------------------------------------------------------------------------------------------------
// If we're required to equip a specific weapon, do it.
bool CTFBot::EquipRequiredWeapon( void )
{
	// if we have a required weapon on our stack, it takes precedence (items, etc)
	if ( m_requiredWeaponStack.Count() )
	{
		CBaseCombatWeapon *pWeapon = m_requiredWeaponStack.Top().Get();
		return Weapon_Switch( pWeapon );
	}

	if ( TheTFBots().IsMeleeOnly() || TFGameRules()->IsInMedievalMode() || HasWeaponRestriction( MELEE_ONLY ) )
	{
		// force use of melee weapons
		Weapon_Switch( Weapon_GetSlot( TF_WPN_TYPE_MELEE ) );
		return true;
	}

	if ( HasWeaponRestriction( PRIMARY_ONLY ) )
	{
		Weapon_Switch( Weapon_GetSlot( TF_WPN_TYPE_PRIMARY ) );
		return true;
	}

	if ( HasWeaponRestriction( SECONDARY_ONLY ) )
	{
		Weapon_Switch( Weapon_GetSlot( TF_WPN_TYPE_SECONDARY ) );
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
// Equip the best weapon we have to attack the given threat
void CTFBot::EquipBestWeaponForThreat( const CKnownEntity *threat )
{
	if ( EquipRequiredWeapon() )
		return;

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
	{
		if ( HasAttribute( CTFBot::AGGRESSIVE ) )
		{
			// mobs never equip other weapons
			return;
		}

		if ( GetPlayerClass()->GetClassIndex() == TF_CLASS_DEMOMAN && !IsInASquad() )
		{
			// wandering demomen use stickies only
			Weapon_Switch( Weapon_GetSlot( TF_WPN_TYPE_SECONDARY ) );
			return;
		}
	}
#endif // TF_RAID_MODE
	 
	CTFWeaponBase *primary = dynamic_cast< CTFWeaponBase *>( Weapon_GetSlot( TF_WPN_TYPE_PRIMARY ) );
	if ( !IsCombatWeapon( primary ) )
	{
		primary = NULL;
	}

	CTFWeaponBase *secondary = dynamic_cast< CTFWeaponBase *>( Weapon_GetSlot( TF_WPN_TYPE_SECONDARY ) );
	if ( !IsCombatWeapon( secondary ) )
	{
		secondary = NULL;
	}

	// no secondary weapons in MvM
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		if ( IsPlayerClass( TF_CLASS_MEDIC ) && IsInASquad() && GetSquad() && !GetSquad()->IsLeader( this ) )
		{
			// always try to heal leader
			Weapon_Switch( Weapon_GetSlot( TF_WPN_TYPE_SECONDARY ) );
			return;
		}

		secondary = NULL;
	}

	CTFWeaponBase *melee = dynamic_cast< CTFWeaponBase *>( Weapon_GetSlot( TF_WPN_TYPE_MELEE ) );
	if ( !IsCombatWeapon( melee ) )
	{
		melee = NULL;
	}

	CTFWeaponBase *gun = NULL;
	if ( primary )
	{
		gun = primary;
	}
	else if ( secondary )
	{
		gun = secondary;
	}
	else
	{
		gun = melee;
	}

	if ( IsDifficulty( CTFBot::EASY ) )
	{
		// easy bots always use their primary weapon if they have one
		if ( gun )
		{
			Weapon_Switch( gun );
		}

		return;
	}

	if ( !threat || !threat->WasEverVisible() || threat->GetTimeSinceLastSeen() > 5.0f )
	{
		// no threat - go back to primary weapon so it has a chance to reload
		if ( gun )
		{
			Weapon_Switch( gun );
		}

		return;
	}

	// now filter weapons by available ammo
	if ( GetAmmoCount( TF_AMMO_PRIMARY ) <= 0 )
	{
		primary = NULL;
	}

	if ( GetAmmoCount( TF_WPN_TYPE_SECONDARY ) <= 0 )
	{
		secondary = NULL;
	}

	// modify our gun choice based on threat situation (range, etc)
	switch( GetPlayerClass()->GetClassIndex() )
	{
	case TF_CLASS_DEMOMAN:
	case TF_CLASS_HEAVYWEAPONS:
	case TF_CLASS_SPY:
	case TF_CLASS_MEDIC:
	case TF_CLASS_ENGINEER:
		// primary
		break;

	case TF_CLASS_SCOUT:
		{
			if ( secondary )
			{
				if ( gun && !gun->Clip1() )
				{
					gun = secondary;
				}
			}
		}
		break;

	case TF_CLASS_SOLDIER:
		{
			// if we've emptied our rocket launcher clip and are fighting a nearby threat, switch to our secondary if it is ready to fire
			if ( gun && !gun->Clip1() )
			{
				if ( secondary && secondary->Clip1() )
				{
					const float closeSoldierRange = 500.0f;
					if ( IsRangeLessThan( threat->GetLastKnownPosition(), closeSoldierRange ) )
					{
						gun = secondary;
					}
				}
			}
		}
		break;

	case TF_CLASS_SNIPER:
		{
			const float closeSniperRange = 750.0f;
			if ( secondary && IsRangeLessThan( threat->GetLastKnownPosition(), closeSniperRange ) )
				gun = secondary;
		}
		break;

	case TF_CLASS_PYRO:
		{
			const float flameRange = 750.0f;
			if ( secondary && IsRangeGreaterThan( threat->GetLastKnownPosition(), flameRange ) )
			{
				gun = secondary;
			}

			// keep flamethrower out to reflect projectiles
			if ( threat->GetEntity() && threat->GetEntity()->IsPlayer() )
			{
				CTFPlayer *enemy = ToTFPlayer( threat->GetEntity() );

				if ( enemy->IsPlayerClass( TF_CLASS_SOLDIER ) || enemy->IsPlayerClass( TF_CLASS_DEMOMAN ) )
				{
					gun = primary;
				}
			}
		}
		break;
	}

	if ( gun )
	{
		Weapon_Switch( gun );
	}
}


//-----------------------------------------------------------------------------------------------------
// NOTE: This assumes default weapon loadouts
bool CTFBot::EquipLongRangeWeapon( void )
{
	// no secondary weapons in MvM
	if ( TFGameRules()->IsMannVsMachineMode() )
		return false;

	if ( IsPlayerClass( TF_CLASS_SOLDIER ) || 
		 IsPlayerClass( TF_CLASS_DEMOMAN ) ||
		 IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) ||
		 IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		CBaseCombatWeapon *primary = Weapon_GetSlot( TF_WPN_TYPE_PRIMARY );
		if ( primary )
		{
			if ( GetAmmoCount( TF_AMMO_PRIMARY ) > 0 )
			{
				Weapon_Switch( primary );
				return true;
			}
		}
	}

	// fall back to our secondary (or go right to it if its the only thing we have that has reach)
	CBaseCombatWeapon *secondary = Weapon_GetSlot( TF_WPN_TYPE_SECONDARY );
	if ( secondary )
	{
		if ( GetAmmoCount( TF_AMMO_SECONDARY ) > 0 )
		{
			Weapon_Switch( secondary );
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
// Force us to equip and use this weapon until popped off the required stack
void CTFBot::PushRequiredWeapon( CTFWeaponBase *weapon )
{
	m_requiredWeaponStack.Push( weapon );
}


//-----------------------------------------------------------------------------------------------------
// Pop top required weapon off of stack and discard
void CTFBot::PopRequiredWeapon( void )
{
	m_requiredWeaponStack.Pop();
}


//-----------------------------------------------------------------------------------------------------
// return true if given weapon can be used to attack
bool CTFBot::IsCombatWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == MY_CURRENT_GUN )		// MY_CURRENT_GUN == NULL
	{
		weapon = m_Shared.GetActiveTFWeapon();
	}

	if ( weapon )
	{
		switch ( weapon->GetWeaponID() )
		{
		case TF_WEAPON_MEDIGUN:
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_DISPENSER:
		case TF_WEAPON_INVIS:
		case TF_WEAPON_LUNCHBOX:
		case TF_WEAPON_BUFF_ITEM:
		case TF_WEAPON_PUMPKIN_BOMB:
			return false;
		};
	}

	return true;
}


//-----------------------------------------------------------------------------------------------------
// return true if given weapon is a "hitscan" weapon
bool CTFBot::IsHitScanWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == MY_CURRENT_GUN )		// MY_CURRENT_GUN == NULL
	{
		weapon = m_Shared.GetActiveTFWeapon();
	}

	if ( weapon )
	{
		switch ( weapon->GetWeaponID() )
		{
		case TF_WEAPON_SHOTGUN_PRIMARY:
		case TF_WEAPON_SHOTGUN_SOLDIER:
		case TF_WEAPON_SHOTGUN_HWG:
		case TF_WEAPON_SHOTGUN_PYRO:
		case TF_WEAPON_SCATTERGUN:
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_MINIGUN:
		case TF_WEAPON_SMG:
		case TF_WEAPON_CHARGED_SMG:
		case TF_WEAPON_PISTOL:
		case TF_WEAPON_PISTOL_SCOUT:
		case TF_WEAPON_REVOLVER:
		case TF_WEAPON_SENTRY_BULLET:
		case TF_WEAPON_SENTRY_ROCKET:
		case TF_WEAPON_SENTRY_REVENGE:
		case TF_WEAPON_HANDGUN_SCOUT_PRIMARY:
		case TF_WEAPON_HANDGUN_SCOUT_SECONDARY:
		case TF_WEAPON_SODA_POPPER:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
		case TF_WEAPON_PEP_BRAWLER_BLASTER:
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
			return true;
		};
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
// return true if given weapon "sprays" bullets/fire/etc continuously (ie: not individual rockets/etc)
bool CTFBot::IsContinuousFireWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == MY_CURRENT_GUN )
	{
		weapon = m_Shared.GetActiveTFWeapon();
	}

	if ( !IsCombatWeapon( weapon ) )
		return false;

	if ( weapon )
	{
		switch ( weapon->GetWeaponID() )
		{
		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
		case TF_WEAPON_GRENADELAUNCHER:
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_PISTOL:
		case TF_WEAPON_PISTOL_SCOUT:
		case TF_WEAPON_FLAREGUN:
		case TF_WEAPON_JAR:
		case TF_WEAPON_COMPOUND_BOW:
			return false;
		};
	}

	return true;

}


//-----------------------------------------------------------------------------------------------------
// return true if given weapon launches explosive projectiles with splash damage
bool CTFBot::IsExplosiveProjectileWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == MY_CURRENT_GUN )
	{
		weapon = m_Shared.GetActiveTFWeapon();
	}

	if ( weapon )
	{
		switch ( weapon->GetWeaponID() )
		{
		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
		case TF_WEAPON_GRENADELAUNCHER:
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_JAR:
			return true;
		};
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
// return true if given weapon has small clip and long reload cost (ie: rocket launcher, etc)
bool CTFBot::IsBarrageAndReloadWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == MY_CURRENT_GUN )
	{
		weapon = m_Shared.GetActiveTFWeapon();
	}

	if ( weapon ) 
	{
		switch ( weapon->GetWeaponID() )
		{
		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
		case TF_WEAPON_GRENADELAUNCHER:
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_SCATTERGUN:
			return true;
		};
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
// Return true if given weapon doesn't make much sound when used (ie: spy knife, etc)
bool CTFBot::IsQuietWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == MY_CURRENT_GUN )
	{
		weapon = m_Shared.GetActiveTFWeapon();
	}

	if ( weapon ) 
	{
		switch ( weapon->GetWeaponID() )
		{
		case TF_WEAPON_KNIFE:
		case TF_WEAPON_FISTS:
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_MEDIGUN:
		case TF_WEAPON_DISPENSER:
		case TF_WEAPON_INVIS:
		case TF_WEAPON_FLAREGUN:
		case TF_WEAPON_LUNCHBOX:
		case TF_WEAPON_JAR:
		case TF_WEAPON_COMPOUND_BOW:
		case TF_WEAPON_SWORD:
		case TF_WEAPON_CROSSBOW:
			return true;
		};
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
// Return true if a weapon has no obstructions along the line between the given points
bool CTFBot::IsLineOfFireClear( const Vector &from, const Vector &to ) const
{
	trace_t trace;
	NextBotTraceFilterIgnoreActors botFilter( NULL, COLLISION_GROUP_NONE );
	CTraceFilterIgnoreFriendlyCombatItems ignoreFriendlyCombatFilter( this, COLLISION_GROUP_NONE, GetTeamNumber() );
	CTraceFilterChain filter( &botFilter, &ignoreFriendlyCombatFilter );

	UTIL_TraceLine( from, to, MASK_SOLID_BRUSHONLY, &filter, &trace );

	return !trace.DidHit();
}


//-----------------------------------------------------------------------------------------------------
// Return true if a weapon has no obstructions along the line from our eye to the given position
bool CTFBot::IsLineOfFireClear( const Vector &where ) const
{
	return IsLineOfFireClear( const_cast< CTFBot * >( this )->EyePosition(), where );
}


//-----------------------------------------------------------------------------------------------------
// Return true if a weapon has no obstructions along the line between the given point and entity
bool CTFBot::IsLineOfFireClear( const Vector &from, CBaseEntity *who ) const
{
	trace_t trace;
	NextBotTraceFilterIgnoreActors botFilter( NULL, COLLISION_GROUP_NONE );
	CTraceFilterIgnoreFriendlyCombatItems ignoreFriendlyCombatFilter( this, COLLISION_GROUP_NONE, GetTeamNumber() );
	CTraceFilterChain filter( &botFilter, &ignoreFriendlyCombatFilter );

	UTIL_TraceLine( from, who->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, &filter, &trace );

	return !trace.DidHit() || trace.m_pEnt == who;
}


//-----------------------------------------------------------------------------------------------------
// Return true if a weapon has no obstructions along the line from our eye to the given entity
bool CTFBot::IsLineOfFireClear( CBaseEntity *who ) const
{
	return IsLineOfFireClear( const_cast< CTFBot * >( this )->EyePosition(), who );
}


//-----------------------------------------------------------------------------------------------------
bool CTFBot::IsEntityBetweenTargetAndSelf( CBaseEntity *other, CBaseEntity *target )
{
	Vector toTarget = target->GetAbsOrigin() - GetAbsOrigin();
	float rangeToTarget = toTarget.NormalizeInPlace();

	Vector toOther = other->GetAbsOrigin() - GetAbsOrigin();
	float rangeToOther = toOther.NormalizeInPlace();

	return rangeToOther < rangeToTarget && DotProduct( toTarget, toOther ) > 0.7071f;
}


//-----------------------------------------------------------------------------------------------------
// Return true if we are sure this player actually is an enemy spy
bool CTFBot::IsKnownSpy( CTFPlayer *player ) const
{
	for( int i=0; i<m_knownSpyVector.Count(); ++i )
	{
		CTFPlayer *spy = m_knownSpyVector[i];
		if ( spy && player->entindex() == spy->entindex() )
		{
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
// Return true if we suspect this player might be an enemy spy
CTFBot::SuspectedSpyInfo_t* CTFBot::IsSuspectedSpy( CTFPlayer *pPlayer )
{
	for( int i=0; i<m_suspectedSpyVector.Count(); ++i )
	{
		SuspectedSpyInfo_t* pSpyInfo = m_suspectedSpyVector[i];
		CTFPlayer* pSpy = pSpyInfo->m_suspectedSpy;
		if ( pSpy && pPlayer->entindex() == pSpy->entindex() )
		{
			return pSpyInfo;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------------------------------
// Note that this player might be a spy
void CTFBot::SuspectSpy( CTFPlayer *pPlayer )
{
	SuspectedSpyInfo_t* pSpyInfo = IsSuspectedSpy( pPlayer );

	// Start suspecting this spy if we're not aware of them until now
	if( pSpyInfo == NULL )
	{
		// add to head for LRU effect
		pSpyInfo = new SuspectedSpyInfo_t;
		pSpyInfo->m_suspectedSpy = pPlayer;
		m_suspectedSpyVector.AddToHead( pSpyInfo );
	}
	
	// Suspicious!
	pSpyInfo->Suspect();

	// Too suspicious?
	if( pSpyInfo->TestForRealizing() )
	{
		RealizeSpy( pPlayer );
	}
}

void CTFBot::SuspectedSpyInfo_t::Suspect()
{
	int nCurTime = floor(gpGlobals->curtime);

	// Add our new entry
	m_touchTimes.AddToHead( nCurTime );
}

bool CTFBot::SuspectedSpyInfo_t::TestForRealizing()
{
	// Remove any old entries
	int nCurTime = floor(gpGlobals->curtime);
	int nCutoffTime = nCurTime - tf_bot_suspect_spy_touch_interval.GetInt();

	FOR_EACH_VEC_BACK( m_touchTimes, i )
	{
		if( m_touchTimes[i] <= nCutoffTime )
			m_touchTimes.Remove( i );
	}

	// Add our new entry
	m_touchTimes.AddToHead( nCurTime );
	
	// Setup an array of bools representing the past few seconds that we want
	// to look for suspicious activity
	CUtlVector<bool> vecSeconds;
	vecSeconds.SetSize( tf_bot_suspect_spy_touch_interval.GetInt() );
	FOR_EACH_VEC( vecSeconds, i )
	{
		vecSeconds[i] = false;
	}

	// Go through each time chunk and mark if there was suspicious activity
	FOR_EACH_VEC( m_touchTimes, i )
	{
		int nTouchTime = m_touchTimes[i];
		int nTimeSlot = nCurTime - nTouchTime;

		if( nTimeSlot >= 0 && nTimeSlot < vecSeconds.Count() )
		{
			vecSeconds[nTimeSlot] = true;
		}
	}

	// If all are true, then the spy has been suspicious enough to warrant being realized
	FOR_EACH_VEC( vecSeconds, i )
	{
		if( vecSeconds[i] == false )
		{
			return false;
		}
	}

	return true;
}


bool CTFBot::SuspectedSpyInfo_t::IsCurrentlySuspected()
{
	float flCutoffTime = gpGlobals->curtime - tf_bot_suspect_spy_forget_cooldown.GetFloat();
	if( m_touchTimes.Count() && m_touchTimes.Head() > flCutoffTime )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------
// Note that this player *IS* a spy
void CTFBot::RealizeSpy( CTFPlayer *pPlayer )
{
	// We already know about this spy
	if ( IsKnownSpy( pPlayer ) )
		return;

	// add to head for LRU effect
	m_knownSpyVector.AddToHead( pPlayer );

	// inform my teammates
	SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_CLOAKEDSPY );

	// If I am suspicious of this spy, make everyone around me know that
	// they should be suspicious too
	SuspectedSpyInfo_t* pSuspectInfo = IsSuspectedSpy( pPlayer );
	if( pSuspectInfo && pSuspectInfo->IsCurrentlySuspected() )
	{
		// Tell others around us we've realized there's a spy
		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS );
		FOR_EACH_VEC( playerVector, i )
		{
			CTFPlayer* pOther = playerVector[i];

			if( !pOther->IsBot() )
				continue;

			//Make sure they're close by
			Vector vecBetween = EyePosition() - pOther->EyePosition();
			if( vecBetween.IsLengthLessThan( 512.f ) )
			{
				// If they dont know about this spy
				CTFBot* pOtherBot = static_cast<CTFBot*>( pOther );
				if( !pOtherBot->IsKnownSpy( pPlayer ) )
				{
					// I was suspicious that they were a spy, make my friend suspicious as well.
					// This will cause them to attack a disguised spy in MvM for a bit.
					pOtherBot->SuspectSpy( pPlayer );

					// Tell them about it
					pOtherBot->RealizeSpy( pPlayer );
				}
			}
		}
	}
	
}


//-----------------------------------------------------------------------------------------------------
// Remove player from spy suspect system
void CTFBot::ForgetSpy( CTFPlayer *pPlayer )
{
	StopSuspectingSpy( pPlayer );
	m_knownSpyVector.FindAndFastRemove( pPlayer );
}

void CTFBot::StopSuspectingSpy( CTFPlayer *pPlayer )
{
	// Find the entry matching this spy
	for( int i=0; i<m_suspectedSpyVector.Count(); ++i )
	{
		SuspectedSpyInfo_t* pSpyInfo = m_suspectedSpyVector[i];
		CTFPlayer* pSpy = pSpyInfo->m_suspectedSpy;
		if ( pSpy && pPlayer->entindex() == pSpy->entindex() )
		{
			delete pSpyInfo;
			m_suspectedSpyVector.Remove(i);
			break;
		}
	}
}


//-----------------------------------------------------------------------------------------------------
// Return the nearest human player on the given team who is looking directly at me
CTFPlayer *CTFBot::GetClosestHumanLookingAtMe( int team ) const
{
	CUtlVector< CTFPlayer * > otherVector;
	CollectPlayers( &otherVector, team, COLLECT_ONLY_LIVING_PLAYERS );

	float closeRange = FLT_MAX;
	CTFPlayer *close = NULL;

	for( int i=0; i<otherVector.Count(); ++i )
	{
		CTFPlayer *other = otherVector[i];

		if ( other->IsBot() )
			continue;

		Vector otherEye, otherForward;
		other->EyePositionAndVectors( &otherEye, &otherForward, NULL, NULL );

		Vector toMe = const_cast< CTFBot * >( this )->EyePosition() - otherEye;
		float range = toMe.NormalizeInPlace();

		if ( range < closeRange )
		{
			const float cosTolerance = 0.98f;
			if ( DotProduct( toMe, otherForward ) > cosTolerance )
			{
				// a human is looking toward me - check LOS
				if ( IsLineOfSightClear( otherEye, IGNORE_NOTHING, other ) )
				{
					close = other;
					closeRange = range;
				}
			}
		}
	}

	return close;
}


//-----------------------------------------------------------------------------------------------------
// become a member of the given squad
void CTFBot::JoinSquad( CTFBotSquad *squad )
{
	if ( squad )
	{
		squad->Join( this );
		m_squad = squad;
	}
}


//-----------------------------------------------------------------------------------------------------
// leave our current squad
void CTFBot::LeaveSquad( void )
{
	if ( m_squad )
	{
		m_squad->Leave( this );
		m_squad = NULL;
	}
}

//-----------------------------------------------------------------------------------------------------
// leave our current squad
void CTFBot::DeleteSquad( void )
{
	if ( m_squad )
	{
		m_squad = NULL;
	}
}

//---------------------------------------------------------------------------------------------
bool CTFBot::IsWeaponRestricted( CTFWeaponBase *weapon ) const
{
	if ( !weapon )
	{
		return false;
	}

	// Get the weapon's loadout slot
	CEconItemView *pEconItemView = weapon->GetAttributeContainer()->GetItem();
	if ( !pEconItemView )
		return false;
	CTFItemDefinition *pItemDef = pEconItemView->GetStaticData();
	if ( !pItemDef )
		return false;
	int iLoadoutSlot = pItemDef->GetLoadoutSlot( GetPlayerClass()->GetClassIndex() );

	if ( HasWeaponRestriction( MELEE_ONLY ) )
	{
		return (iLoadoutSlot != LOADOUT_POSITION_MELEE);
	}

	if ( HasWeaponRestriction( PRIMARY_ONLY ) )
	{
		return (iLoadoutSlot != LOADOUT_POSITION_PRIMARY);
	}

	if ( HasWeaponRestriction( SECONDARY_ONLY ) )
	{
		return (iLoadoutSlot != LOADOUT_POSITION_SECONDARY);
	}

	return false;
}

bool CTFBot::ScriptIsWeaponRestricted( HSCRIPT script ) const
{
	CBaseEntity *pEntity = ToEnt( script );
	if ( !pEntity )
		return true;

	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( pEntity );
	if ( !pWeapon )
		return true;

	return IsWeaponRestricted( pWeapon );
}

//---------------------------------------------------------------------------------------------
//
// Return true if there is something we want to reflect directly ahead of us
//
bool CTFBot::ShouldFireCompressionBlast( void )
{
	if ( TFGameRules()->IsInTraining() )
	{
		// no reflection in training mode
		return false;
	}

	if ( !tf_bot_pyro_always_reflect.GetBool() )
	{
		if ( IsDifficulty( CTFBot::EASY ) )
		{
			// easy bots can't reflect at all
			return false;
		}

		if ( IsDifficulty( CTFBot::NORMAL ) )
		{
			// normal bots reflect some of the time
			if ( TransientlyConsistentRandomValue( 1.0f ) < 0.5f )
			{
				return false;
			}
		}

		if ( IsDifficulty( CTFBot::HARD ) )
		{
			// hard bots reflect most of the time
			if ( TransientlyConsistentRandomValue( 1.0f ) < 0.1f )
			{
				return false;
			}
		}
	}

	bool shouldPushPlayers = !TFGameRules()->IsMannVsMachineMode();

	if ( shouldPushPlayers )
	{
		const CKnownEntity *threat = GetVisionInterface()->GetPrimaryKnownThreat( true );
		if ( threat && threat->GetEntity() && threat->GetEntity()->IsPlayer() )
		{
			CTFPlayer *pushVictim = ToTFPlayer( threat->GetEntity() );

			if ( IsRangeLessThan( pushVictim, tf_bot_pyro_shove_away_range.GetFloat() ) )
			{
				// our threat is very close - shove them!

				// always shove ubers
				if ( pushVictim && pushVictim->m_Shared.IsInvulnerable() )
				{
					return true;
				}

				if ( pushVictim->GetGroundEntity() == NULL )
				{
					// they are in the air - juggle them some of the time
					return ( TransientlyConsistentRandomValue( 0.5f ) < 0.5f );
				}

				if ( pushVictim->IsCapturingPoint() )
				{
					// push them off the point!
					return true;
				}

				// be pushy sometimes
				if ( TransientlyConsistentRandomValue( 3.0f ) < 0.5f )
				{
					return true;
				}
			}
		}
	}


	Vector vecEye = EyePosition();
	Vector vecForward, vecRight, vecUp;

	AngleVectors( EyeAngles(), &vecForward, &vecRight, &vecUp );

	Vector vecCenter = vecEye + vecForward * 128;
	Vector vecSize = Vector( 128, 128, 64 );

	const int maxCollectedEntities = 128;
	CBaseEntity	*pObjects[ maxCollectedEntities ];
	int count = UTIL_EntitiesInBox( pObjects, maxCollectedEntities, vecCenter - vecSize, vecCenter + vecSize, FL_CLIENT | FL_GRENADE );

	for ( int i = 0; i < count; i++ )
	{
		CBaseEntity *pObject = pObjects[i];
		if ( pObject == this )
			continue;

		if ( pObject->GetTeamNumber() == GetTeamNumber() )
			continue;

		// should air blast player logic is already done before this loop
		if ( pObject->IsPlayer() )
			continue;

		// is this something I want to deflect?
		if ( !pObject->IsDeflectable() )
			continue;

		if ( FClassnameIs( pObject, "tf_projectile_rocket" ) || FClassnameIs( pObject, "tf_projectile_energy_ball" ) )
		{
			// is it headed right for me?
			Vector vecThemUnitVel = pObject->GetAbsVelocity();
			vecThemUnitVel.z = 0.0f;
			vecThemUnitVel.NormalizeInPlace();

			Vector horzForward( vecForward.x, vecForward.y, 0.0f );
			horzForward.NormalizeInPlace();

			if ( DotProduct( horzForward, vecThemUnitVel ) > -tf_bot_pyro_deflect_tolerance.GetFloat() )
				continue;
		}

		// can I see it?
		if ( !GetVisionInterface()->IsLineOfSightClear( pObject->WorldSpaceCenter() ) )
			continue;

		// bounce it!
		return true;
	}

	return false;
}


//---------------------------------------------------------------------------------------------
// Compute a pseudo random value (0-1) that stays consistent for the 
// given period of time, but changes unpredictably each period.
float CTFBot::TransientlyConsistentRandomValue( float period, int seedValue ) const
{
	CNavArea *area = GetLastKnownArea();
	if ( !area )
	{
		return 0.0f;
	}

	// this term stays stable for 'period' seconds, then changes in an unpredictable way
	int timeMod = (int)( gpGlobals->curtime / period ) + 1;
	return fabs( FastCos( (float)( seedValue + ( entindex() * area->GetID() * timeMod ) ) ) );
}


//---------------------------------------------------------------------------------------------
// Given a target entity, find a target within 'maxSplashRadius' that has clear line of fire
// to both the target entity and to me.
bool CTFBot::FindSplashTarget( CBaseEntity *target, float maxSplashRadius, Vector *splashTarget ) const
{
	if ( !target || !splashTarget )
		return false;

	*splashTarget = target->WorldSpaceCenter();

	const int retryCount = 50;
	for( int i=0; i<retryCount; ++i )
	{
		Vector probe = target->WorldSpaceCenter() + RandomVector( -maxSplashRadius, maxSplashRadius );

		trace_t trace;
		NextBotTraceFilterIgnoreActors filter( NULL, COLLISION_GROUP_NONE );

		UTIL_TraceLine( target->WorldSpaceCenter(), probe, MASK_SOLID_BRUSHONLY, &filter, &trace );
		if ( trace.DidHitWorld() )
		{
			// can we shoot this spot?
			if ( IsLineOfFireClear( trace.endpos ) )
			{
				// yes, found a corner-sticky target
				*splashTarget = trace.endpos;

				NDebugOverlay::Line( target->WorldSpaceCenter(), trace.endpos, 255, 0, 0, true, 60.0f );
				NDebugOverlay::Cross3D( trace.endpos, 5.0f, 255, 255, 0, true, 60.0f );

				return true;
			}
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
// Restrict bot's attention to only this entity (or radius around this entity) to the exclusion of everything else
void CTFBot::SetAttentionFocus( CBaseEntity *focusOn )
{
	m_attentionFocusEntity = focusOn;
}


//---------------------------------------------------------------------------------------------
// Remove attention focus restrictions
void CTFBot::ClearAttentionFocus( void )
{
	m_attentionFocusEntity = NULL;
}


//---------------------------------------------------------------------------------------------
bool CTFBot::IsAttentionFocused( void ) const
{
	return m_attentionFocusEntity != NULL;
}


//---------------------------------------------------------------------------------------------
bool CTFBot::IsAttentionFocusedOn( CBaseEntity *who ) const
{
	if ( m_attentionFocusEntity == NULL || who == NULL )
	{
		return false;
	}

	if ( m_attentionFocusEntity->entindex() == who->entindex() )
	{
		// specifically focused on this entity
		return true;
	}

	CTFBotActionPoint *actionPoint = dynamic_cast< CTFBotActionPoint * >( m_attentionFocusEntity.Get() );
	if ( actionPoint )
	{
		// we attend to everything within the action point's radius
		return actionPoint->IsWithinRange( who );
	}

	return false;
}


//---------------------------------------------------------------------------------------------
// Notice the given threat after the given number of seconds have elapsed
void CTFBot::DelayedThreatNotice( CHandle< CBaseEntity > who, float noticeDelay )
{
	float when = gpGlobals->curtime + noticeDelay;

	// if we already have a delayed notice for this threat, ignore the new one unless the delay is less
	for( int i=0; i<m_delayedNoticeVector.Count(); ++i )
	{
		if ( m_delayedNoticeVector[i].m_who == who )
		{
			if ( m_delayedNoticeVector[i].m_when > when )
			{
				// update delay to shorter time
				m_delayedNoticeVector[i].m_when = when;
			}
			return;
		}
	}

	// new notice
	DelayedNoticeInfo delay;
	delay.m_who = who;
	delay.m_when = when;
	m_delayedNoticeVector.AddToTail( delay );
}


//---------------------------------------------------------------------------------------------
void CTFBot::UpdateDelayedThreatNotices( void )
{
	for( int i=0; i<m_delayedNoticeVector.Count(); ++i )
	{
		if ( m_delayedNoticeVector[i].m_when <= gpGlobals->curtime )
		{
			// delay is up - notice this threat
			CBaseEntity *who = m_delayedNoticeVector[i].m_who;

			if ( who )
			{
				if ( who->IsPlayer() )
				{
					CTFPlayer *player = ToTFPlayer( who );
					if ( player->IsPlayerClass( TF_CLASS_SPY ) )
					{
						RealizeSpy( player );
					}
				}

				GetVisionInterface()->AddKnownEntity( who );
			}

			m_delayedNoticeVector.Remove( i );
			--i;
		}
	}
}


//---------------------------------------------------------------------------------------------
void CTFBot::GiveRandomItem( loadout_positions_t loadoutPosition )
{
	CUtlVector< const CEconItemDefinition * > itemVector;

	const CEconItemSchema::ItemDefinitionMap_t& mapItemDefs = ItemSystem()->GetItemSchema()->GetItemDefinitionMap();
	FOR_EACH_MAP_FAST( mapItemDefs, i )
	{
		const CTFItemDefinition *pItemDef = dynamic_cast< const CTFItemDefinition * >( mapItemDefs[i] );

		if ( pItemDef && pItemDef->GetLoadoutSlot( GetPlayerClass()->GetClassIndex() ) == loadoutPosition )
		{
			itemVector.AddToTail( pItemDef );
		}
	}

	if ( itemVector.Count() > 0 )
	{
		int which = RandomInt( 0, itemVector.Count()-1 );

/*
		CBaseCombatWeapon *myMelee = me->Weapon_GetSlot( TF_WPN_TYPE_MELEE );
		me->Weapon_Detach( myMelee );
		UTIL_Remove( myMelee );
*/

		const char *itemName = itemVector[ which ]->GetDefinitionName();
		BotGenerateAndWearItem( this, itemName );
	}
}


//---------------------------------------------------------------------------------------------
bool CTFBot::IsSquadmate( CTFPlayer *who ) const
{
	if ( !m_squad || !who || !who->IsBotOfType( TF_BOT_TYPE ) )
		return false;

	return GetSquad() == ToTFBot( who )->GetSquad();
}


//---------------------------------------------------------------------------------------------
// Set Spy disguise to be a class that someone on the enemy team is actually using
void CTFBot::DisguiseAsMemberOfEnemyTeam( void )
{
	CUtlVector< CTFPlayer * > enemyVector;
	CollectPlayers( &enemyVector, GetEnemyTeam( GetTeamNumber() ) );

	int disguise = RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS-1 );

	if ( enemyVector.Count() > 0 )
	{
		disguise = enemyVector[ RandomInt( 0, enemyVector.Count()-1 ) ]->GetPlayerClass()->GetClassIndex();
	}

	m_Shared.Disguise( GetEnemyTeam( GetTeamNumber() ), disguise );
}


//---------------------------------------------------------------------------------------------
void CTFBot::ClearTags( void )
{
	m_tags.RemoveAll();
}


//---------------------------------------------------------------------------------------------
void CTFBot::AddTag( const char *tag )
{
	if ( !HasTag( tag ) )
	{
		m_tags.AddToTail( CFmtStr( "%s", tag ) );
	}
}


//---------------------------------------------------------------------------------------------
void CTFBot::RemoveTag( const char *tag )
{
	for ( int i=0; i<m_tags.Count(); ++i )
	{
		if ( FStrEq( tag, m_tags[i] ) )
		{
			m_tags.Remove(i);
			return;
		}
	}
}


//---------------------------------------------------------------------------------------------
// TODO: Make this an efficient lookup/match
bool CTFBot::HasTag( const char *tag )
{
	for( int i=0; i<m_tags.Count(); ++i )
	{
		if ( FStrEq( tag, m_tags[i] ) )
		{
			return true;
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
void CTFBot::ScriptGetAllTags( HSCRIPT hTable )
{
	for ( int i = 0; i < m_tags.Count(); i++ )
	{
		g_pScriptVM->SetValue( hTable, CFmtStr( "%d", i ), m_tags[ i ] );
	}
}


//---------------------------------------------------------------------------------------------
CBaseObject *CTFBot::GetNearestKnownSappableTarget( void )
{
	CUtlVector< CKnownEntity > knownVector;
	GetVisionInterface()->CollectKnownEntities( &knownVector );

	CBaseObject *closeObject = NULL;
	float closeObjectRangeSq = 500.0f * 500.0f;

	for( int i=0; i<knownVector.Count(); ++i )
	{
		CBaseObject *enemyObject = dynamic_cast< CBaseObject * >( knownVector[i].GetEntity() );
		if ( enemyObject && !enemyObject->HasSapper() && IsEnemy( enemyObject ) )
		{
			float rangeSq = GetRangeSquaredTo( enemyObject );
			if ( rangeSq < closeObjectRangeSq )
			{
				closeObjectRangeSq = rangeSq;
				closeObject = enemyObject;
			}		
		}
	}

	return closeObject;
}


//-----------------------------------------------------------------------------------------
Action< CTFBot > *CTFBot::OpportunisticallyUseWeaponAbilities( void )
{
	if ( !m_opportunisticTimer.IsElapsed() )
	{
		return NULL;
	}

	m_opportunisticTimer.Start( RandomFloat( 0.1f, 0.2f ) );


	// if I'm wearing a charge shield, use it!
	if ( IsPlayerClass( TF_CLASS_DEMOMAN ) && m_Shared.IsShieldEquipped() )
	{
		Vector forward;
		EyeVectors( &forward );
		bool bShouldCharge = GetLocomotionInterface()->IsPotentiallyTraversable( GetAbsOrigin(), GetAbsOrigin() + 100.0f * forward, ILocomotion::IMMEDIATELY );
		if ( HasAttribute( CTFBot::AIR_CHARGE_ONLY ) && ( GetGroundEntity() || GetAbsVelocity().z > 0 ) )
		{
			bShouldCharge = false;
		}

		if ( bShouldCharge )
		{
			PressAltFireButton();
		}
	}
	// if I'm wearing parachute, check if I should activate my parachute
	else if ( m_Shared.IsParachuteEquipped() )
	{
		bool bIsBurning = m_Shared.InCond( TF_COND_BURNING );
		float flHealthPercent = (float)GetHealth() / GetMaxHealth();
		const float flHealthThreshold = 0.5f;
		// should I activate parachute?
		if ( !m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) )
		{
			float flMinParachuteGroundDistance = 300.f;
			// check if I'm falling, high enough off the ground to deploy parachute, and not burning
			if ( flHealthPercent >= flHealthThreshold && !bIsBurning && GetAbsVelocity().z < 0 && GetLocomotionInterface()->IsPotentiallyTraversable( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, -flMinParachuteGroundDistance ), ILocomotion::IMMEDIATELY ) )
			{
				PressJumpButton();
			}
		}
		// should I deactivate parachute?
		else
		{
			float flCancelParachuteDistance = 150.f;
			// if I'm burning or close enough to landing, deactivate the parachute or health less than some threshold
			if ( flHealthPercent < flHealthThreshold || bIsBurning || !GetLocomotionInterface()->IsPotentiallyTraversable( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, -flCancelParachuteDistance ), ILocomotion::IMMEDIATELY ) )
			{
				PressJumpButton();
			}
		}
	}

	// don't use items if we have the flag, since most of them are unusable (unless we're a bomb carrier in MvM)
	if ( HasTheFlag() && !TFGameRules()->IsMannVsMachineMode() )
	{
		return NULL;
	}

	for ( int w=0; w<MAX_WEAPONS; ++w )
	{
		CTFWeaponBase *weapon = ( CTFWeaponBase * )GetWeapon( w );
		if ( !weapon )
			continue;

		// if I have some kind of buff banner - use it!
		if ( weapon->GetWeaponID() == TF_WEAPON_BUFF_ITEM )
		{
			CTFBuffItem *buff = (CTFBuffItem *)weapon;
			if ( buff->IsFull() )
			{
				return new CTFBotUseItem( buff );
			}
		}
		else if ( weapon->GetWeaponID() == TF_WEAPON_LUNCHBOX )
		{
			// if we have an eatable (drink, sandvich, etc) - eat it!
			CTFLunchBox *lunchbox = (CTFLunchBox *)weapon;
			if ( lunchbox->HasAmmo() )
			{
				// scout lunchboxes are also gated by their energy drink meter
				if ( !IsPlayerClass( TF_CLASS_SCOUT ) || m_Shared.GetScoutEnergyDrinkMeter() >= 100 )
				{
					return new CTFBotUseItem( lunchbox );
				}
			}
		}
		else if ( weapon->GetWeaponID() == TF_WEAPON_BAT_WOOD )
		{
			// sandman
			if ( GetAmmoCount( TF_AMMO_GRENADES1 ) > 0 )
			{
				const CKnownEntity *threat = GetVisionInterface()->GetPrimaryKnownThreat();
				if ( threat && threat->IsVisibleInFOVNow() )
				{
					// hit a stunball
					PressAltFireButton();			
				}
			}
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------------------
// mostly for MvM - pick a random enemy player that is not in their spawn room
CTFPlayer *CTFBot::SelectRandomReachableEnemy( void )
{
	CUtlVector< CTFPlayer * > livePlayerVector;
	CollectPlayers( &livePlayerVector, GetEnemyTeam( GetTeamNumber() ), COLLECT_ONLY_LIVING_PLAYERS );

	// only consider players who have left their spawn
	CUtlVector< CTFPlayer * > playerVector;
	for( int i=0; i<livePlayerVector.Count(); ++i )
	{
		CTFPlayer *player = livePlayerVector[i];
		if ( !PointInRespawnRoom( player, player->WorldSpaceCenter() ) )
		{
			playerVector.AddToTail( player );
		}
	}

	if ( playerVector.Count() > 0 )
	{
		return playerVector[ RandomInt( 0, playerVector.Count()-1 ) ];
	}

	return NULL;
}


//-----------------------------------------------------------------------------------------
// Different sized bots used different lookahead distances
float CTFBot::GetDesiredPathLookAheadRange( void ) const
{
	return tf_bot_path_lookahead_range.GetFloat() * GetModelScale();
}

//-----------------------------------------------------------------------------------------
// Hack to apply idle loop sounds in MvM
void CTFBot::StartIdleSound( void )
{
	StopIdleSound();

	if ( TFGameRules() && !TFGameRules()->IsMannVsMachineMode() )
		return;

	// SHIELD YOUR EYES MIKEB!!!
	if ( IsMiniBoss() )
	{
		const char *pszSoundName = NULL;

		int iClass = GetPlayerClass()->GetClassIndex();
		switch ( iClass )
		{
		case TF_CLASS_HEAVYWEAPONS:
			{
				pszSoundName = "MVM.GiantHeavyLoop";
				break;
			}
		case TF_CLASS_SOLDIER:
			{
				pszSoundName = "MVM.GiantSoldierLoop";
				break;
			}
		case TF_CLASS_DEMOMAN:
			{
				if ( m_mission == MISSION_DESTROY_SENTRIES )
				{
					pszSoundName = "MVM.SentryBusterLoop";
				}
				else
				{
					pszSoundName = "MVM.GiantDemomanLoop";
				}
				break;
			}
		case TF_CLASS_SCOUT:
			{
				pszSoundName = "MVM.GiantScoutLoop";
				break;
			}
		case TF_CLASS_PYRO:
			{
				pszSoundName = "MVM.GiantPyroLoop";
				break;
			}
		}

		if ( pszSoundName )
		{
			CReliableBroadcastRecipientFilter filter;
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			m_pIdleSound = controller.SoundCreate( filter, entindex(), pszSoundName );
			controller.Play( m_pIdleSound, 1.0, 100 );
		}
	}
}

//-----------------------------------------------------------------------------------------
void CTFBot::StopIdleSound( void )
{
	if ( m_pIdleSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pIdleSound );
		m_pIdleSound = NULL;
	}
}

bool CTFBot::ShouldAutoJump()
{
	if ( !HasAttribute( CTFBot::AUTO_JUMP ) )
		return false;

	if ( !m_autoJumpTimer.HasStarted() )
	{
		m_autoJumpTimer.Start( RandomFloat( m_flAutoJumpMin, m_flAutoJumpMax ) );
		return true;
	}
	else if ( m_autoJumpTimer.IsElapsed() )
	{
		m_autoJumpTimer.Start( RandomFloat( m_flAutoJumpMin, m_flAutoJumpMax ) );
		return true;
	}
	
	return false;
}


void CTFBot::SetFlagTarget( CCaptureFlag* pFlag )
{
	if ( m_hFollowingFlagTarget != pFlag )
	{
		if ( m_hFollowingFlagTarget )
		{
			m_hFollowingFlagTarget->RemoveFollower( this );
		}

		m_hFollowingFlagTarget = pFlag;
		if ( m_hFollowingFlagTarget )
		{
			m_hFollowingFlagTarget->AddFollower( this );
		}
	}
}


int CTFBot::DrawDebugTextOverlays(void)
{
	int offset = tf_bot_debug_tags.GetBool() ? 1 : BaseClass::DrawDebugTextOverlays();

	CUtlString strTags = "Tags : ";
	for( int i=0; i<m_tags.Count(); ++i )
	{
		strTags.Append( m_tags[i] );
		strTags.Append( " " );
	}

	EntityText( offset, strTags.Get(), 0 );
	offset++;

	return offset;
}


void CTFBot::AddEventChangeAttributes( const CTFBot::EventChangeAttributes_t* newEvent )
{
	m_eventChangeAttributes.AddToTail( newEvent );
}


const CTFBot::EventChangeAttributes_t* CTFBot::GetEventChangeAttributes( const char* pszEventName ) const
{
	for ( int i=0; i<m_eventChangeAttributes.Count(); ++i )
	{
		if ( FStrEq( m_eventChangeAttributes[i]->m_eventName, pszEventName ) )
		{
			return m_eventChangeAttributes[i];
		}
	}
	return NULL;
}


void CTFBot::OnEventChangeAttributes( const CTFBot::EventChangeAttributes_t* pEvent )
{
	if ( pEvent )
	{
		SetDifficulty( pEvent->m_skill );

		ClearWeaponRestrictions();
		SetWeaponRestriction( pEvent->m_weaponRestriction );

		SetMission( pEvent->m_mission );

		ClearAllAttributes();
		SetAttribute( pEvent->m_attributeFlags );

		SetMaxVisionRangeOverride( pEvent->m_maxVisionRange );

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			SetAttribute( CTFBot::BECOME_SPECTATOR_ON_DEATH );
			SetAttribute( CTFBot::RETAIN_BUILDINGS );
		}

		// cache off health value before we clear attribute because ModifyMaxHealth adds new attribute and reset the health
		int nHealth = GetHealth();
		int nMaxHealth = GetMaxHealth();

		// remove any player attributes
		RemovePlayerAttributes( false );
		// and add ones that we want specifically
		FOR_EACH_VEC( pEvent->m_characterAttributes, i )
		{
			const CEconItemAttributeDefinition *pDef = pEvent->m_characterAttributes[i].GetAttributeDefinition();
			if ( pDef )
			{
				Assert( GetAttributeList() );
				GetAttributeList()->SetRuntimeAttributeValue( pDef, pEvent->m_characterAttributes[i].m_value.asFloat );
			}
		}
		NetworkStateChanged();

		// set health back to what it was before we clear bot's attributes
		ModifyMaxHealth( nMaxHealth );
		SetHealth( nHealth );

		// give items to bot before apply attribute changes
		FOR_EACH_VEC( pEvent->m_items, i )
		{
			AddItem( pEvent->m_items[i] );
		}

		// add attributes to equipped items
		FOR_EACH_VEC( pEvent->m_itemsAttributes, i )
		{
			const CTFBot::EventChangeAttributes_t::item_attributes_t& itemAttributes = pEvent->m_itemsAttributes[i];
			CSchemaItemDefHandle itemDef( itemAttributes.m_itemName );
			if ( !itemDef )
			{
				Warning( "Unable to find item %s to update attribute.\n", itemAttributes.m_itemName.Get() ); 
			}

			for ( int iItemSlot = LOADOUT_POSITION_PRIMARY ; iItemSlot < CLASS_LOADOUT_POSITION_COUNT ; iItemSlot++ )
			{
				CEconEntity* pEntity = NULL;
				CEconItemView *pCurItemData = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( this, iItemSlot, &pEntity );
				if ( pCurItemData && itemDef && ( pCurItemData->GetItemDefIndex() == itemDef->GetDefinitionIndex() ) )
				{
					for ( int iAtt=0; iAtt<itemAttributes.m_attributes.Count(); ++iAtt )
					{
						const static_attrib_t& attrib = itemAttributes.m_attributes[iAtt];
						CAttributeList *pAttribList = pCurItemData->GetAttributeList();
						if ( pAttribList )
						{
							pAttribList->SetRuntimeAttributeValue( attrib.GetAttributeDefinition(), attrib.m_value.asFloat );
						}
					}

					if ( pEntity )
					{
						// update model incase we change style
						pEntity->UpdateModelToClass();
					}

					// move on to the next set of attributes
					break;
				}
			} // for each slot
		} // for each set of attributes

		// tags
		ClearTags();
		for( int g=0; g<pEvent->m_tags.Count(); ++g )
		{
			AddTag( pEvent->m_tags[g] );
		}
	}
}


void CTFBot::AddItem( const char* pszItemName )
{
	CItemSelectionCriteria criteria;
	criteria.SetQuality( AE_USE_SCRIPT_VALUE );
	criteria.BAddCondition( "name", k_EOperator_String_EQ, pszItemName, true );

	CBaseEntity *pItem = ItemGeneration()->GenerateRandomItem( &criteria, WorldSpaceCenter(), vec3_angle );
	if ( pItem )
	{
		CEconItemView *pScriptItem = static_cast< CBaseCombatWeapon * >( pItem )->GetAttributeContainer()->GetItem();

		// If we already have an item in that slot, remove it
		int iClass = GetPlayerClass()->GetClassIndex();
		int iSlot = pScriptItem->GetStaticData()->GetLoadoutSlot( iClass );
		equip_region_mask_t unNewItemRegionMask = pScriptItem->GetItemDefinition() ? pScriptItem->GetItemDefinition()->GetEquipRegionConflictMask() : 0;

		if ( IsWearableSlot( iSlot ) )
		{
			// Remove any wearable that has a conflicting equip_region
			for ( int wbl = 0; wbl < GetNumWearables(); wbl++ )
			{
				CEconWearable *pWearable = GetWearable( wbl );
				if ( !pWearable )
					continue;

				equip_region_mask_t unWearableRegionMask = 0;
				if ( pWearable->GetAttributeContainer()->GetItem() )
				{
					unWearableRegionMask = pWearable->GetAttributeContainer()->GetItem()->GetItemDefinition()->GetEquipRegionConflictMask();
				}

				if ( unWearableRegionMask & unNewItemRegionMask )
				{
					RemoveWearable( pWearable );
				}
			}
		}
		else
		{
			CBaseEntity	*pEntity = GetEntityForLoadoutSlot( iSlot );
			if ( pEntity )
			{
				CBaseCombatWeapon *pWpn = dynamic_cast< CBaseCombatWeapon * >( pEntity );
				Weapon_Detach( pWpn );
				UTIL_Remove( pEntity );
			}
		}

		// Fake global id
		pScriptItem->SetItemID( 1 );

		DispatchSpawn( pItem );

		CEconEntity *pNewItem = assert_cast<CEconEntity*>( pItem );
		if ( pNewItem )
		{
			pNewItem->GiveTo( this );
		}

		PostInventoryApplication();
	}
	else
	{
		if ( pszItemName && pszItemName[0] )
		{
			DevMsg( "CTFBotSpawner::AddItemToBot: Invalid item %s.\n", pszItemName );
		}
	}
}


int CTFBot::GetUberHealthThreshold()
{
	int iUberHealthThreshold = 0;
	CALL_ATTRIB_HOOK_INT( iUberHealthThreshold, bot_medic_uber_health_threshold );
	if ( iUberHealthThreshold > 0 )
	{
		return iUberHealthThreshold;
	}

	return 50;
}


float CTFBot::GetUberDeployDelayDuration()
{
	float flDelayUberDuration = 0;
	CALL_ATTRIB_HOOK_INT( flDelayUberDuration, bot_medic_uber_deploy_delay_duration );
	if ( flDelayUberDuration > 0 )
	{
		return flDelayUberDuration;
	}
	
	return -1.f;
}
