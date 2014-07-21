//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include <stdio.h>

//#define GAME_DLL
#ifdef GAME_DLL
#include "cbase.h"
#endif

#include <stdio.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "tier2/tier2.h"
#include "game/server/pluginvariant.h"
#include "game/server/iplayerinfo.h"
#include "game/server/ientityinfo.h"
#include "game/server/igameinfo.h"

//#define SAMPLE_TF2_PLUGIN
#ifdef SAMPLE_TF2_PLUGIN
#include "tf/tf_shareddefs.h"
#endif
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Interfaces from the engine
IVEngineServer	*engine = NULL; // helper functions (messaging clients, loading content, making entities, running commands, etc)
IGameEventManager *gameeventmanager_ = NULL; // game events interface
#ifndef GAME_DLL
#define gameeventmanager gameeventmanager_
#endif
IPlayerInfoManager *playerinfomanager = NULL; // game dll interface to interact with players
IEntityInfoManager *entityinfomanager = NULL; // game dll interface to interact with all entities (like IPlayerInfo)
IGameInfoManager *gameinfomanager = NULL; // game dll interface to get data from game rules directly
IBotManager *botmanager = NULL; // game dll interface to interact with bots
IServerPluginHelpers *helpers = NULL; // special 3rd party plugin helpers from the engine
IUniformRandomStream *randomStr = NULL;
IEngineTrace *enginetrace = NULL;


CGlobalVars *gpGlobals = NULL;

// function to initialize any cvars/command in this plugin
void Bot_RunAll( void ); 

// useful helper func
#ifndef GAME_DLL
inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(Q_stricmp(sz1, sz2) == 0);
}
#endif
//---------------------------------------------------------------------------------
// Purpose: a sample 3rd party plugin class
//---------------------------------------------------------------------------------
class CEmptyServerPlugin: public IServerPluginCallbacks, public IGameEventListener
{
public:
	CEmptyServerPlugin();
	~CEmptyServerPlugin();

	// IServerPluginCallbacks methods
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void );
	virtual void			UnPause( void );
	virtual const char     *GetPluginDescription( void );      
	virtual void			LevelInit( char const *pMapName );
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	virtual void			GameFrame( bool simulating );
	virtual void			LevelShutdown( void );
	virtual void			ClientActive( edict_t *pEntity );
	virtual void			ClientDisconnect( edict_t *pEntity );
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername );
	virtual void			SetCommandClient( int index );
	virtual void			ClientSettingsChanged( edict_t *pEdict );
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args );
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );
	virtual void			OnEdictAllocated( edict_t *edict );
	virtual void			OnEdictFreed( const edict_t *edict  );	

	// IGameEventListener Interface
	virtual void FireGameEvent( KeyValues * event );

	virtual int GetCommandIndex() { return m_iClientCommandIndex; }
private:
	int m_iClientCommandIndex;
};


// 
// The plugin is a static singleton that is exported as an interface
//
CEmptyServerPlugin g_EmtpyServerPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CEmptyServerPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_EmtpyServerPlugin );

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CEmptyServerPlugin::CEmptyServerPlugin()
{
	m_iClientCommandIndex = 0;
}

CEmptyServerPlugin::~CEmptyServerPlugin()
{
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CEmptyServerPlugin::Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	ConnectTier1Libraries( &interfaceFactory, 1 );
	ConnectTier2Libraries( &interfaceFactory, 1 );

	entityinfomanager = (IEntityInfoManager *)gameServerFactory(INTERFACEVERSION_ENTITYINFOMANAGER,NULL);
	if ( !entityinfomanager )
	{
		Warning( "Unable to load entityinfomanager, ignoring\n" ); // this isn't fatal, we just won't be able to access entity data
	}

	playerinfomanager = (IPlayerInfoManager *)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER,NULL);
	if ( !playerinfomanager )
	{
		Warning( "Unable to load playerinfomanager, ignoring\n" ); // this isn't fatal, we just won't be able to access specific player data
	}

	botmanager = (IBotManager *)gameServerFactory(INTERFACEVERSION_PLAYERBOTMANAGER, NULL);
	if ( !botmanager )
	{
		Warning( "Unable to load botcontroller, ignoring\n" ); // this isn't fatal, we just won't be able to access specific bot functions
	}
	gameinfomanager = (IGameInfoManager *)gameServerFactory(INTERFACEVERSION_GAMEINFOMANAGER, NULL);
	if (!gameinfomanager)
	{
		Warning( "Unable to load gameinfomanager, ignoring\n" );
	}

	engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	gameeventmanager = (IGameEventManager *)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER,NULL);
	helpers = (IServerPluginHelpers*)interfaceFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL);
	enginetrace = (IEngineTrace *)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER,NULL);
	randomStr = (IUniformRandomStream *)interfaceFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL);

	// get the interfaces we want to use
	if(	! ( engine && gameeventmanager && g_pFullFileSystem && helpers && enginetrace && randomStr ) )
	{
		return false; // we require all these interface to function
	}

	if ( playerinfomanager )
	{
		gpGlobals = playerinfomanager->GetGlobalVars();
	}

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
	ConVar_Register( 0 );
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::Unload( void )
{
	gameeventmanager->RemoveListener( this ); // make sure we are unloaded from the event system

	ConVar_Unregister( );
	DisconnectTier2Libraries( );
	DisconnectTier1Libraries( );
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::Pause( void )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::UnPause( void )
{
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CEmptyServerPlugin::GetPluginDescription( void )
{
	return "Emtpy-Plugin V2, Valve";
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::LevelInit( char const *pMapName )
{
	Msg( "Level \"%s\" has been loaded\n", pMapName );
	gameeventmanager->AddListener( this, true );
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::GameFrame( bool simulating )
{
	if ( simulating )
	{
		Bot_RunAll();
	}
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::LevelShutdown( void ) // !!!!this can get called multiple times per map change
{
	gameeventmanager->RemoveListener( this );
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientActive( edict_t *pEntity )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientDisconnect( edict_t *pEntity )
{
}

//---------------------------------------------------------------------------------
// Purpose: called on 
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientPutInServer( edict_t *pEntity, char const *playername )
{
	KeyValues *kv = new KeyValues( "msg" );
	kv->SetString( "title", "Hello" );
	kv->SetString( "msg", "Hello there" );
	kv->SetColor( "color", Color( 255, 0, 0, 255 ));
	kv->SetInt( "level", 5);
	kv->SetInt( "time", 10);
	helpers->CreateMessage( pEntity, DIALOG_MSG, kv, this );
	kv->deleteThis();
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::SetCommandClient( int index )
{
	m_iClientCommandIndex = index;
}

void ClientPrint( edict_t *pEdict, char *format, ... )
{
	va_list		argptr;
	static char		string[1024];
	
	va_start (argptr, format);
	Q_vsnprintf(string, sizeof(string), format,argptr);
	va_end (argptr);

	engine->ClientPrintf( pEdict, string );
}
//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientSettingsChanged( edict_t *pEdict )
{
	if ( playerinfomanager )
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEdict );

		const char * name = engine->GetClientConVarValue( engine->IndexOfEdict(pEdict), "name" );

		// CAN'T use Q_stricmp here, this dll is made by 3rd parties and may not link to tier0/vstdlib
		if ( playerinfo && name && playerinfo->GetName() && 
			 stricmp( name, playerinfo->GetName()) ) // playerinfo may be NULL if the MOD doesn't support access to player data 
													   // OR if you are accessing the player before they are fully connected
		{
			ClientPrint( pEdict, "Your name changed to \"%s\" (from \"%s\"\n", name, playerinfo->GetName() );
						// this is the bad way to check this, the better option it to listen for the "player_changename" event in FireGameEvent()
						// this is here to give a real example of how to use the playerinfo interface
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	return PLUGIN_CONTINUE;
}

CON_COMMAND( DoAskConnect, "Server plugin example of using the ask connect dialog" )
{
	if ( args.ArgC() < 2 )
	{
		Warning ( "DoAskConnect <server IP>\n" );
	}
	else
	{
		const char *pServerIP = args.Arg( 1 );

		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", pServerIP );	// The IP address of the server to connect to goes in the "title" field.
		kv->SetInt( "time", 3 );

		for ( int i=1; i < gpGlobals->maxClients; i++ )
		{
			edict_t *pEdict = engine->PEntityOfEntIndex( i );
			if ( pEdict )
			{
				helpers->CreateMessage( pEdict, DIALOG_ASKCONNECT, kv, &g_EmtpyServerPlugin );
			}
		}

		kv->deleteThis();
	}
}

#ifdef SAMPLE_TF2_PLUGIN
const char *classNames[] =
{
	"unknown",
	"scout",
	"sniper",
	"soldier",
	"demoman",
	"medic",
	"heavy weapons guy",
	"pyro",
	"spy",
	"engineer",
};

bool TFPlayerHasCondition( int inBits, int condition )
{
	Assert( condition >= 0 && condition < TF_COND_LAST );

	return ( ( inBits & (1<<condition) ) != 0 );
}

void SentryStatus( edict_t *pEntity )
{
	IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
	if (!playerinfo)
	{
		Msg("couldn't get playerinfo\n");
		return;
	}

	Msg("Sentry Status:\n");
	pluginvariant value;
	pluginvariant emptyVariant;
	edict_t *pSentry = NULL;
	if (playerinfo->GetCustomInfo(TFPLAYERINFO_ENTINDEX_SENTRY, value, emptyVariant))
	{
		pSentry = engine->PEntityOfEntIndex( value.Int() );
		if (!pSentry)
		{
			Warning("couldn't attain sentry gun entity\n");
			return;
		}
	}
	else
	{
		Msg("No Sentrygun built.\n");
		return;

	}
	IEntityInfo *entinfo = entityinfomanager->GetEntityInfo( pSentry );
	if (!entinfo)
	{
		Warning("couldn't get entinfo for sentry gun\n");
		return;
	}

	if (playerinfo->GetCustomInfo(TFPLAYERINFO_BUILDING_SENTRY, value, emptyVariant))
	{
		if (value.Bool())
			Msg("Sentry Under Construction...\n");
	}
	if (playerinfo->GetCustomInfo(TFPLAYERINFO_UPGRADING_SENTRY, value, emptyVariant))
	{
		if (value.Bool())
			Msg("Sentry Upgrading...\n");
	}

	int sentryLevel = 0;
	if (playerinfo->GetCustomInfo(TFPLAYERINFO_SENTRY_LEVEL, value, emptyVariant))
	{
		sentryLevel = value.Int();
		Msg("Sentry Level: %i\n", sentryLevel );
	}
	else
		Msg("Unable to retrive sentry level\n");

	if (playerinfo->GetCustomInfo(TFPLAYERINFO_SENTRY_PROGRESS, value, emptyVariant))
	{
		if (sentryLevel < 3)
		{
			int iMetal, iRequiredMetal;
			iRequiredMetal = value.Int() & 0xFF;
			iMetal = (value.Int()>>8) & 0xFF;
			Msg("%i / %i Metal Required for Sentry Level %i\n", iMetal, iRequiredMetal, sentryLevel+1);
		}
		else
			Msg("Sentry cannot be upgraded further.\n");
	}

	Msg("Health: %i\n", entinfo->GetHealth() );

	if (playerinfo->GetCustomInfo(TFPLAYERINFO_SENTRY_KILLS, value, emptyVariant))
		Msg("Kills: %i\n", value.Int() );
	else
		Msg("Unable to retrieve sentry kills\n");

	if (playerinfo->GetCustomInfo(TFPLAYERINFO_SENTRY_AMMO_SHELLS, value, emptyVariant))
	{
		int iShells, iMaxShells;
		iMaxShells = value.Int() & 0xFF;
		iShells = (value.Int()>>8) & 0xFF;
		Msg("Shells: %i / %i\n", iShells, iMaxShells);
	}
	if (sentryLevel > 2)
	{
		if (playerinfo->GetCustomInfo(TFPLAYERINFO_SENTRY_AMMO_ROCKETS, value, emptyVariant))
		{
			int iRockets, iMaxRockets;
			iMaxRockets = value.Int() & 0xFF;
			iRockets = (value.Int()>>8) & 0xFF;
			Msg("Rockets: %i / %i\n", iRockets, iMaxRockets);
		}
	}

}
void DispenserStatus( edict_t *pEntity )
{
	IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
	if (!playerinfo)
	{
		Msg("couldn't get playerinfo\n");
		return;
	}

	Msg("Dispenser Status:\n");
	pluginvariant value;
	pluginvariant emptyVariant;
	edict_t *pDispenser = NULL;
	if (playerinfo->GetCustomInfo(TFPLAYERINFO_ENTINDEX_DISPENSER, value, emptyVariant))
	{
		pDispenser = engine->PEntityOfEntIndex( value.Int() );
		if (!pDispenser)
		{
			Warning("couldn't attain dispenser entity\n");
			return;
		}
	}
	else
	{
		Msg("No dispenser built.\n");
		return;
	}
	IEntityInfo *entinfo = entityinfomanager->GetEntityInfo( pDispenser );
	if (!entinfo)
	{
		Warning("couldn't get entinfo for dispenser\n");
		return;
	}
	if (playerinfo->GetCustomInfo(TFPLAYERINFO_BUILDING_DISPENSER, value, emptyVariant))
	{
		if (value.Bool())
			Msg("Dispenser Under Construction...\n");
	}
	Msg("Health: %i\n", entinfo->GetHealth() );
	if (playerinfo->GetCustomInfo(TFPLAYERINFO_DISPENSER_METAL, value, emptyVariant))
		Msg("Metal: %i\n", value.Int() );
}
void TeleporterStatus( edict_t *pEntity )
{
	IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
	if (!playerinfo)
	{
		Msg("couldn't get playerinfo\n");
		return;
	}

	Msg("Teleporter Status:\n");

	pluginvariant value;
	pluginvariant emptyVariant;
	edict_t *pEntrance = NULL;
	edict_t *pExit = NULL;
	if (playerinfo->GetCustomInfo(TFPLAYERINFO_ENTINDEX_TELEPORTER_ENTRANCE, value, emptyVariant))
	{
		pEntrance = engine->PEntityOfEntIndex( value.Int() );
		if (!pEntrance)
		{
			Warning("couldn't attain entrance entity\n");
		}
	}
	else
	{
		Msg("No Teleporter Entrance built.\n");
	}
	if (playerinfo->GetCustomInfo(TFPLAYERINFO_ENTINDEX_TELEPORTER_EXIT, value, emptyVariant))
	{
		pExit = engine->PEntityOfEntIndex( value.Int() );
		if (!pExit)
		{
			Warning("couldn't attain exit entity\n");
		}
	}
	else
	{
		Msg("No Teleporter Entrance built.\n");
	}
	IEntityInfo *entranceInfo = entityinfomanager->GetEntityInfo( pEntrance );
	if (!entranceInfo)
	{
		Warning("couldn't get entinfo for teleporter entrance\n");
	}
	IEntityInfo *exitInfo = entityinfomanager->GetEntityInfo( pExit );
	if (!exitInfo)
	{
		Warning("couldn't get entinfo for teleporter exit\n");
	}

	if (pEntrance && entranceInfo)
	{
		if (playerinfo->GetCustomInfo(TFPLAYERINFO_BUILDING_TELEPORTER_ENTRANCE, value, emptyVariant))
		{
			if (value.Bool())
				Msg("Entrance Under Construction...\n");
		}
		Msg("Entrance Health: %i\n", entranceInfo->GetHealth() );
		if (playerinfo->GetCustomInfo(TFPLAYERINFO_TELEPORTER_USES, value, emptyVariant))
			Msg("Entrance Used %i Times.\n", value.Int() );

	}
	if (pExit && exitInfo)
	{
		if (playerinfo->GetCustomInfo(TFPLAYERINFO_BUILDING_TELEPORTER_EXIT, value, emptyVariant))
		{
			if (value.Bool())
				Msg("Exit Under Construction...\n");
		}
		Msg("Exit Health: %i\n", exitInfo->GetHealth() );
	}
}
void ClassStatus( edict_t *pEntity )
{
	IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
	if (!playerinfo)
	{
		Msg("couldn't get playerinfo\n");
		return;
	}
	int playerClassId = playerinfo->GetPlayerClassId();

	Msg("Player Class: %s\n", playerinfo->GetPlayerClassName());
	pluginvariant conditionValue;
	pluginvariant emptyVariant;
	if (!playerinfo->GetCustomInfo(TFPLAYERINFO_CONDITIONS, conditionValue, emptyVariant))
	{
		Warning("unable to retrieve conditions!\n");
	}
	if (TFPlayerHasCondition(conditionValue.Int(), TF_COND_INVULNERABLE ))
		Msg("You are Invulnerable!\n");
	if (TFPlayerHasCondition(conditionValue.Int(), TF_COND_SELECTED_TO_TELEPORT ))
		Msg("You are about to Teleport.\n");
	if (TFPlayerHasCondition(conditionValue.Int(), TF_COND_TELEPORTED ))
		Msg("You have recently been teleported.\n");

	switch(playerClassId)
	{
	default:
	case TF_CLASS_MEDIC:
		break;
	case TF_CLASS_ENGINEER:
		Msg("Building Information:\n");
		SentryStatus( pEntity );
		DispenserStatus( pEntity );
		TeleporterStatus( pEntity );
		break;
	case TF_CLASS_SPY:
		{
			int disguiseClass = 0;
			pluginvariant value;

			if (playerinfo->GetCustomInfo(TFPLAYERINFO_SPY_DISGUISEDAS, value, emptyVariant))
				disguiseClass = value.Int();

			if ( TFPlayerHasCondition(conditionValue.Int(), TF_COND_DISGUISING ) )
				Msg("Disguising..\n");
			else if (TFPlayerHasCondition(conditionValue.Int(), TF_COND_DISGUISED ) )
				Msg("Disguised as: %s\n", classNames[disguiseClass] );

			if (TFPlayerHasCondition(conditionValue.Int(), TF_COND_STEALTHED ))
				Msg("Cloaked!\n");
			if (playerinfo->GetCustomInfo(TFPLAYERINFO_SPY_CLOAKCHARGELEVEL, value, emptyVariant))
				Msg("Cloak Charge Percent: %d\n", value.Float() );

			break;
		}
	case TF_CLASS_DEMOMAN:
		break;
	}
}
const char *ctf_flagtype[] =
{
	"ctf",					//TF_FLAGTYPE_CTF = 0,
	"attack / defend",		//TF_FLAGTYPE_ATTACK_DEFEND,
	"territory control",	//TF_FLAGTYPE_TERRITORY_CONTROL,
	"invade",				//TF_FLAGTYPE_INVADE,
	"king of the hill",		//TF_FLAGTYPE_KINGOFTHEHILL,
};
const char *ctf_flagstatus[] =
{
	"unknown",
	"At Home",
	"Dropped",
	"Stolen",
};
void FlagStatus( edict_t *pPlayer )
{
	IPlayerInfo *pInfo = playerinfomanager->GetPlayerInfo( pPlayer );
	if (!pInfo)
	{
		Msg( "couldn't get playerinfo\n" );
		return;
	}
	IGameInfo *gameInfo = gameinfomanager->GetGameInfo();
	if (!gameInfo)
	{
		Msg( "couldn't get gameinfo\n" );
	}

	int gameType = gameInfo->GetInfo_GameType();

	if (gameType != 1)
	{
		Msg( "Game is not CTF.\n" );
		return;
	}
	Msg( "===============================\n" );
	Msg( "Capture The Flag -- Flag Status\n" );
	Msg( "===============================\n" );
	pluginvariant value, options;

	edict_t *pFlag = NULL;
	while ( (pFlag = entityinfomanager->FindEntityByClassname(pFlag, "item_teamflag")) != NULL )
	{
		IEntityInfo *pFlagInfo = entityinfomanager->GetEntityInfo( pFlag );
		if (!pFlagInfo)
			continue;

		Msg( "\nTeam %s's Flag\n",	gameInfo->GetInfo_GetTeamName( pFlagInfo->GetTeamIndex() ) );
		options.SetInt(engine->IndexOfEdict(pFlag));
		if ( gameInfo->GetInfo_Custom( TFGAMEINFO_CTF_FLAG_TYPE, value, options) )
			Msg( "Type: %s\n", ctf_flagtype[value.Int()] );
		if ( gameInfo->GetInfo_Custom( TFGAMEINFO_CTF_FLAG_STATUS, value, options) )
		{
			Msg( "Status: %s\n", ctf_flagstatus[value.Int()] );
			//Tony; if we're carried, find out who has us.
			if (value.Int() == 3)
			{
				edict_t *pPlayer = pFlagInfo->GetOwner();
				if (pPlayer)
				{
					IPlayerInfo *pPlayerInfo = playerinfomanager->GetPlayerInfo( pPlayer );
					if (pPlayerInfo)
						Msg( "Carried by: %s\n", pPlayerInfo->GetName() );
				}
			}
		}
	}


	Msg( "===============================\n" );
}
#endif

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::ClientCommand( edict_t *pEntity, const CCommand &args )
{
	const char *pcmd = args[0];

	if ( !pEntity || pEntity->IsFree() ) 
	{
		return PLUGIN_CONTINUE;
	}

	if ( FStrEq( pcmd, "menu" ) )
	{
		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", "You've got options, hit ESC" );
		kv->SetInt( "level", 1 );
		kv->SetColor( "color", Color( 255, 0, 0, 255 ));
		kv->SetInt( "time", 20 );
		kv->SetString( "msg", "Pick an option\nOr don't." );
		
		for( int i = 1; i < 9; i++ )
		{
			char num[10], msg[10], cmd[10];
			Q_snprintf( num, sizeof(num), "%i", i );
			Q_snprintf( msg, sizeof(msg), "Option %i", i );
			Q_snprintf( cmd, sizeof(cmd), "option%i", i );

			KeyValues *item1 = kv->FindKey( num, true );
			item1->SetString( "msg", msg );
			item1->SetString( "command", cmd );
		}

		helpers->CreateMessage( pEntity, DIALOG_MENU, kv, this );
		kv->deleteThis();
		return PLUGIN_STOP; // we handled this function
	}
	else if ( FStrEq( pcmd, "rich" ) )
	{
		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", "A rich message" );
		kv->SetInt( "level", 1 );
		kv->SetInt( "time", 20 );
		kv->SetString( "msg", "This is a long long long text string.\n\nIt also has line breaks." );
		
		helpers->CreateMessage( pEntity, DIALOG_TEXT, kv, this );
		kv->deleteThis();
		return PLUGIN_STOP; // we handled this function
	}
	else if ( FStrEq( pcmd, "msg" ) )
	{
		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", "Just a simple hello" );
		kv->SetInt( "level", 1 );
		kv->SetInt( "time", 20 );
		
		helpers->CreateMessage( pEntity, DIALOG_MSG, kv, this );
		kv->deleteThis();
		return PLUGIN_STOP; // we handled this function
	}
	else if ( FStrEq( pcmd, "entry" ) )
	{
		KeyValues *kv = new KeyValues( "entry" );
		kv->SetString( "title", "Stuff" );
		kv->SetString( "msg", "Enter something" );
		kv->SetString( "command", "say" ); // anything they enter into the dialog turns into a say command
		kv->SetInt( "level", 1 );
		kv->SetInt( "time", 20 );
		
		helpers->CreateMessage( pEntity, DIALOG_ENTRY, kv, this );
		kv->deleteThis();
		return PLUGIN_STOP; // we handled this function		
	}
#ifdef SAMPLE_TF2_PLUGIN
	else if ( FStrEq( pcmd, "gameinfo" ) )
	{
		IGameInfo *gameInfo = gameinfomanager->GetGameInfo();
		if (!gameInfo)
			return PLUGIN_STOP;

		Msg("=== Game Information ===\n");
		Msg("Game Type: %i / %s\n", gameInfo->GetInfo_GameType(), gameInfo->GetInfo_GameTypeName() );
		int teamCount = gameInfo->GetInfo_GetTeamCount();
		Msg("Num Teams: %i\n", teamCount );

		Msg("Player Counts:\n");
		for (int i = 0;i<teamCount;i++)
		{
			//If this failes, we can assume the rest is invalid too.
			if (!gameInfo->GetInfo_GetTeamName(i) )
				continue;
			Msg("Team: %s, Players: %i\n", gameInfo->GetInfo_GetTeamName(i), gameInfo->GetInfo_NumPlayersOnTeam(i) );
		}
		return PLUGIN_STOP;

	}
	// Sample to use the new CustomInfo added to TF2 for plugins
	else if ( FStrEq( pcmd, "tfcond" ) )
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
		if (!playerinfo)
			return PLUGIN_STOP;

		pluginvariant conditionValue;
		pluginvariant emptyVariant;
		if (!playerinfo->GetCustomInfo(TFPLAYERINFO_CONDITIONS, conditionValue, emptyVariant))
		{
			Msg("unable to retrieve conditions!\n");
			return PLUGIN_STOP;
		}

		Msg("Disguising?: %s\n", TFPlayerHasCondition(conditionValue.Int(), TF_COND_DISGUISING ) ? "yes" : "no" );
		Msg("Disguised?: %s\n", TFPlayerHasCondition(conditionValue.Int(), TF_COND_DISGUISED ) ? "yes" : "no" );
		Msg("Stealthed?: %s\n", TFPlayerHasCondition(conditionValue.Int(), TF_COND_STEALTHED ) ? "yes" : "no" );
		Msg("Invulnerable?: %s\n", TFPlayerHasCondition(conditionValue.Int(), TF_COND_INVULNERABLE ) ? "yes" : "no" );
		Msg("Teleported Recently?: %s\n", TFPlayerHasCondition(conditionValue.Int(), TF_COND_TELEPORTED ) ? "yes" : "no" );
		Msg("Selected for Teleportation?: %s\n", TFPlayerHasCondition(conditionValue.Int(), TF_COND_SELECTED_TO_TELEPORT ) ? "yes" : "no" );
		Msg("On Fire?: %s\n", TFPlayerHasCondition(conditionValue.Int(), TF_COND_BURNING ) ? "yes" : "no" );

		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "sentry_status" ) )
	{
		SentryStatus(pEntity);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "class_status" ) )
	{
		ClassStatus(pEntity);
		return PLUGIN_STOP;
	}
	else if ( FStrEq( pcmd, "flag_status" ) )
	{
		FlagStatus(pEntity);
		return PLUGIN_STOP;
	}
	#ifdef GAME_DLL
		else if ( FStrEq( pcmd, "cbe_test" ) )
		{
			IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
			if (!playerinfo)
				return PLUGIN_STOP;

			CBaseEntity *pEnt = static_cast< CBaseEntity* >(entityinfomanager->GetEntity( pEntity ));
			if (pEnt)
				Msg("got a pointer to CBaseEntity..\n");
			Msg("attempting to print this entities modelname directly..\n");

			Msg("ModelName: %s\n", STRING(pEnt->GetModelName()) );

			return PLUGIN_STOP;
		}
	#endif
#endif


	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a cvar value query is finished
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
	Msg( "Cvar query (cookie: %d, status: %d) - name: %s, value: %s\n", iCookie, eStatus, pCvarName, pCvarValue );
}
void CEmptyServerPlugin::OnEdictAllocated( edict_t *edict )
{
}
void CEmptyServerPlugin::OnEdictFreed( const edict_t *edict  )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::FireGameEvent( KeyValues * event )
{
	const char * name = event->GetName();
	Msg( "CEmptyServerPlugin::FireGameEvent: Got event \"%s\"\n", name );
}

//---------------------------------------------------------------------------------
// Purpose: an example of how to implement a new command
//---------------------------------------------------------------------------------
CON_COMMAND( empty_version, "prints the version of the empty plugin" )
{
	Msg( "Version:2.0.0.0\n" );
}

CON_COMMAND( empty_log, "logs the version of the empty plugin" )
{
	engine->LogPrint( "Version:2.0.0.0\n" );
}

//---------------------------------------------------------------------------------
// Purpose: an example cvar
//---------------------------------------------------------------------------------
static ConVar empty_cvar("plugin_empty", "0", FCVAR_NOTIFY, "Example plugin cvar");
