#include "cbase.h"
#include "hl2mp_serveradmin.h"
#include "filesystem.h"
#include <KeyValues.h>
#include "hl2mp_player.h"
#include "convar.h"
#include "tier0/icommandline.h"
#include <time.h>
#include "fmtstr.h"

// always comes last
#include "tier0/memdbgon.h"

#ifndef Q_max
#define Q_max(a, b) ((a) > (b) ? (a) : (b))
#endif

extern bool bAdminMapChange;

CHL2MP_Admin *g_pHL2MPAdmin = NULL;
bool g_bAdminSystem = false;

// global list of admins
CUtlVector<CHL2MP_Admin *> g_AdminList;
FileHandle_t g_AdminLogFile = FILESYSTEM_INVALID_HANDLE;

CUtlMap<const char *, SpecialTarget> g_SpecialTargets( DefLessFunc( const char * ) );
static CUtlMap<CUtlString, AdminData_t> g_AdminMap( DefLessFunc( CUtlString ) );

CUtlMap<CUtlString, AdminData_t> &CHL2MP_Admin::GetAdminMap()
{
	return g_AdminMap;
}

bool CHL2MP_Admin::bIsListenServerMsg = false;

ConVar sv_showadminpermissions( "sv_showadminpermissions", "1", 0, "If non-zero, a non-root admin will only see the commands they have access to" );


// Was the command typed from the chat or from the console?
AdminReplySource GetCmdReplySource( CBasePlayer *pPlayer )
{
	if ( !pPlayer && UTIL_IsCommandIssuedByServerAdmin() )
	{
		return ADMIN_REPLY_SERVER_CONSOLE;
	}

	// If player and flag was set, it was chat-triggered
	if ( pPlayer && pPlayer->WasCommandUsedFromChat() )
	{
		pPlayer->SetChatCommandResetThink();  // Reset for next command
		return ADMIN_REPLY_CHAT;
	}

	return ADMIN_REPLY_CONSOLE;  // Player console (not chat)
}

void AdminReply( AdminReplySource source, CBasePlayer *pPlayer, const char *fmt, ... )
{
	char msg[ 512 ];
	va_list argptr;
	va_start( argptr, fmt );
	Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );

	switch ( source )
	{
	case ADMIN_REPLY_SERVER_CONSOLE:
		Msg( "%s\n", msg );
		break;

	case ADMIN_REPLY_CONSOLE:
		if ( pPlayer )
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs( "%s\n", msg ) );
		}
		break;

	case ADMIN_REPLY_CHAT:
		if ( pPlayer )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, msg );
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor/destructor
//-----------------------------------------------------------------------------
CHL2MP_Admin::CHL2MP_Admin()
{
	Assert( !g_pHL2MPAdmin );
	g_pHL2MPAdmin = this;

	bAll = bBlue = bRed = bAllButMe = bMe = bAlive = bDead = bBots = bHumans = false;
	bIsListenServerMsg = false;
	m_steamID = NULL;
	m_permissions = NULL;
}


CHL2MP_Admin::~CHL2MP_Admin()
{
	if ( m_steamID )
	{
		free( ( void * ) m_steamID );   // free the copied steamID string
	}

	if ( m_permissions )
	{
		free( ( void * ) m_permissions );  // free the copied permissions string
	}

	Assert( g_pHL2MPAdmin == this );
	g_pHL2MPAdmin = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Set up the admins
//-----------------------------------------------------------------------------
void CHL2MP_Admin::Initialize( const char *steamID, const char *permissions )
{
	if ( m_steamID )
	{
		free( ( void * ) m_steamID );
		m_steamID = NULL;
	}
	if ( m_permissions )
	{
		free( ( void * ) m_permissions );
		m_permissions = NULL;
	}

	m_steamID = steamID ? V_strdup( steamID ) : NULL;
	m_permissions = permissions ? V_strdup( permissions ) : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Reload the admins.txt file
//-----------------------------------------------------------------------------
bool CHL2MP_Admin::ParseAdminFile( const char *filename, CUtlMap<CUtlString, AdminData_t> &outAdminMap )
{
	outAdminMap.RemoveAll();

	KeyValues *kv = new KeyValues( "Admins" );
	if ( !kv->LoadFromFile( filesystem, filename ) )
	{
		kv->deleteThis();
		return false;
	}

	for ( KeyValues *pAdmin = kv->GetFirstSubKey(); pAdmin; pAdmin = pAdmin->GetNextKey() )
	{
		const char *steamID = pAdmin->GetName();
		const char *flags = pAdmin->GetString( "flags", "" );

		outAdminMap.Insert( steamID, AdminData_t( flags ) );
	}

	kv->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Cache the admins
//			This allows adding and removing admins without resorting to
//			restarting the entire admin system by calling the initializer
//-----------------------------------------------------------------------------
void CHL2MP_Admin::SaveAdminCache()
{
	KeyValues *kv = new KeyValues( "Admins" );

	for ( int i = 0; i < g_AdminList.Count(); ++i )
	{
		CHL2MP_Admin *pAdmin = g_AdminList[ i ];
		KeyValues *pAdminKV = new KeyValues( pAdmin->GetSteamID() );
		pAdminKV->SetString( "flags", pAdmin->m_permissions );
		kv->AddSubKey( pAdminKV );
	}

	kv->SaveToFile( filesystem, "cfg/admin/admins.txt" );
	kv->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: Admin permissions
//-----------------------------------------------------------------------------
bool CHL2MP_Admin::HasPermission( char flag ) const
{
	if ( !m_permissions )
		return false;

	DevMsg( "Checking permission flag %c against permissions %s\n", flag, m_permissions );

	bool hasPermission = ( strchr( m_permissions, flag ) != NULL ) || ( strchr( m_permissions, ADMIN_ROOT ) != NULL );

	// quick dev perms check
	if ( hasPermission )
		DevMsg( "Admin has the required permission.\n" );
	else
		DevMsg( "Admin does NOT have the required permission.\n" );

	return hasPermission;
}


//-----------------------------------------------------------------------------
// Purpose: Add a new admin with SteamID and permissions to the global admin list
//-----------------------------------------------------------------------------
void CHL2MP_Admin::AddAdmin( const char *steamID, const char *permissions )
{
	// Check if admin already exists
	CHL2MP_Admin *existingAdmin = GetAdmin( steamID );
	if ( existingAdmin )
	{
		Msg( "Admin with SteamID %s already exists.\n", steamID );
		return;
	}

	// steamID and permissions must be valid
	if ( steamID == NULL || permissions == NULL || *steamID == '\0' || *permissions == '\0' )
	{
		Msg( "Invalid admin data: SteamID or permissions are null.\n" );
		return;
	}

	CHL2MP_Admin *pNewAdmin = new CHL2MP_Admin();

	// Set the steamID and permissions after creation
	pNewAdmin->Initialize( steamID, permissions );

	// Add to the global list
	g_AdminList.AddToTail( pNewAdmin );

	Msg( "Added admin with SteamID %s and permissions %s.\n", steamID, permissions );
}


//-----------------------------------------------------------------------------
// Purpose: Do we have an admin?
//-----------------------------------------------------------------------------
CHL2MP_Admin *CHL2MP_Admin::GetAdmin( const char *steamID )
{
	for ( int i = 0; i < g_AdminList.Count(); i++ )
	{
		DevMsg( "Comparing against: %s\n", g_AdminList[ i ]->GetSteamID() );
		if ( Q_stricmp( g_AdminList[ i ]->GetSteamID(), steamID ) == 0 )
		{
			return g_AdminList[ i ];
		}
	}
	return NULL;  // No admin found
}

//-----------------------------------------------------------------------------
// Purpose: Check if a player's SteamID has admin permissions
//			Different from GetAdmin() just above since we do not
//			directly get the SteamID of an active player on the
//			the server using engine->GetPlayerNetworkIDString( edict )
//-----------------------------------------------------------------------------
static bool IsSteamIDAdmin( const char *steamID )
{
	KeyValues *kv = new KeyValues( "Admins" );

	if ( !kv->LoadFromFile( filesystem, "cfg/admin/admins.txt", "MOD" ) )
	{
		Msg( "Failed to open cfg/admin/admins.txt for reading.\n" );
		kv->deleteThis();
		return false;
	}

	// checking all the admin flags
	const char *adminFlags = kv->GetString( steamID, NULL );
	if ( adminFlags && *adminFlags != '\0' )
	{
		if ( Q_stristr( adminFlags, "b" ) || Q_stristr( adminFlags, "c" ) || Q_stristr( adminFlags, "d" ) ||
			Q_stristr( adminFlags, "e" ) || Q_stristr( adminFlags, "f" ) || Q_stristr( adminFlags, "g" ) ||
			Q_stristr( adminFlags, "h" ) || Q_stristr( adminFlags, "i" ) || Q_stristr( adminFlags, "j" ) ||
			Q_stristr( adminFlags, "k" ) || Q_stristr( adminFlags, "l" ) || Q_stristr( adminFlags, "m" ) ||
			Q_stristr( adminFlags, "n" ) || Q_stristr( adminFlags, "z" ) )
		{
			kv->deleteThis();
			return true;
		}
	}

	kv->deleteThis();
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Check if a player has admin permissions
//-----------------------------------------------------------------------------
bool CHL2MP_Admin::IsPlayerAdmin( CBasePlayer *pPlayer, const char *requiredFlags )
{
	if ( !pPlayer )
		return false;

	if ( !engine->IsDedicatedServer() && pPlayer == UTIL_GetListenServerHost() )
	{
		// We only need to see this once
		if ( !bIsListenServerMsg )
		{
			Msg( "Not a dedicated server, local server host is %s; granting all permissions\n", pPlayer->GetPlayerName() );
			bIsListenServerMsg = true;
		}
		return true;
	}

	const char *steamID = engine->GetPlayerNetworkIDString( pPlayer->edict() );

	CHL2MP_Admin *pAdmin = GetAdmin( steamID );

	if ( pAdmin )
	{
		if ( pAdmin->HasPermission( ADMIN_ROOT ) )  // root is z, all permissions
		{
			DevMsg( "Admin has root ('z') flag, granting all permissions.\n" );
			return true;
		}

		// else does this player have at least one flag?
		for ( int i = 0; requiredFlags[ i ] != '\0'; i++ )
		{
			if ( pAdmin->HasPermission( requiredFlags[ i ] ) )
			{
				// they do
				return true;
			}
		}
	}

	// or they don't
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Clear all admins (e.g., when the server resets or map changes)
//-----------------------------------------------------------------------------
void CHL2MP_Admin::ClearAllAdmins()
{
	g_AdminList.PurgeAndDeleteElements();
	DevMsg( "All admins have been cleared from the server.\n" );
}

//-----------------------------------------------------------------------------
// Purpose: Centralized function for sending messages and logging admin actions
//			when using a special group target like @all, @alive, @humans, etc.
//-----------------------------------------------------------------------------
void BuildGroupTargetMessage(
	const char *partialName,
	CBasePlayer *pPlayer,
	const char *action,
	const char *duration,
	CUtlString &logDetails,
	CUtlString &chatMessage,
	bool hasReason,
	const char *reason = NULL
)
{
	const char *adminName = pPlayer ? pPlayer->GetPlayerName() : "Console";

	auto AppendDuration = [ & ]() {
		if ( duration && duration[ 0 ] != '\0' )
		{
			if ( Q_stricmp( action, "banned" ) == 0 && Q_stricmp( duration, "permanently" ) == 0 )
			{
				chatMessage.Append( " permanently" );
			}
			else
			{
				chatMessage.Append( UTIL_VarArgs( " for %s", duration ) );
			}
		}
		};

	auto AppendLogReason = [ & ]() {
		if ( hasReason )
		{
			logDetails.Append( UTIL_VarArgs( " (Reason: %s)", ( reason && reason[ 0 ] != '\0' ) ? reason : "No reason provided" ) );
		}
		};

	if ( !Q_stricmp( partialName, "@me" ) )
	{
		logDetails.Format( "themself%s%s", duration ? " " : "", duration ? duration : "" );
		chatMessage = UTIL_VarArgs( "%s %s themself", adminName, action );
		AppendDuration();
		AppendLogReason();
	}
	else if ( !Q_stricmp( partialName, "@!me" ) )
	{
		logDetails.Format( "all players except themself%s%s", duration ? " " : "", duration ? duration : "" );
		chatMessage = UTIL_VarArgs( "%s %s everyone but themself", adminName, action );
		AppendDuration();
		AppendLogReason();
	}
	else if ( !Q_stricmp( partialName, "@all" ) )
	{
		logDetails.Format( "all players%s%s", duration ? " " : "", duration ? duration : "" );
		chatMessage = UTIL_VarArgs( "%s %s everyone", adminName, action );
		AppendDuration();
		AppendLogReason();
	}
	else if ( !Q_stricmp( partialName, "@bots" ) )
	{
		logDetails.Format( "all bots%s%s", duration ? " " : "", duration ? duration : "" );
		chatMessage = UTIL_VarArgs( "%s %s all bots", adminName, action );
		AppendDuration();
		AppendLogReason();
	}
	else if ( !Q_stricmp( partialName, "@humans" ) )
	{
		logDetails.Format( "all human players%s%s", duration ? " " : "", duration ? duration : "" );
		chatMessage = UTIL_VarArgs( "%s %s all human players", adminName, action );
		AppendDuration();
		AppendLogReason();
	}
	else if ( !Q_stricmp( partialName, "@alive" ) )
	{
		logDetails.Format( "all alive players%s%s", duration ? " " : "", duration ? duration : "" );
		chatMessage = UTIL_VarArgs( "%s %s all alive players", adminName, action );
		AppendDuration();
		AppendLogReason();
	}
	else if ( !Q_stricmp( partialName, "@dead" ) )
	{
		logDetails.Format( "all dead players%s%s", duration ? " " : "", duration ? duration : "" );
		chatMessage = UTIL_VarArgs( "%s %s all dead players", adminName, action );
		AppendDuration();
		AppendLogReason();
	}
#if defined(HL2MP) // If not HL2MP, then it will resort to the last else statement down there
	else if ( !Q_stricmp( partialName, "@red" ) )
	{
		logDetails.Format( "all Rebels players%s%s", duration ? " " : "", duration ? duration : "" );
		chatMessage = UTIL_VarArgs( "%s %s all Rebels players", adminName, action );
		AppendDuration();
		AppendLogReason();
	}
	else if ( !Q_stricmp( partialName, "@blue" ) )
	{
		logDetails.Format( "all Combine players%s%s", duration ? " " : "", duration ? duration : "" );
		chatMessage = UTIL_VarArgs( "%s %s all Combine players", adminName, action );
		AppendDuration();
		AppendLogReason();
	}
#endif
	else
	{
		logDetails.Format( "players in group %s%s%s", partialName + 1, duration ? " " : "", duration ? duration : "" );
		chatMessage = UTIL_VarArgs( "%s %s all players in group %s", adminName, action, partialName + 1 );
		AppendDuration();
		AppendLogReason();
	}

	if ( hasReason && reason && reason[ 0 ] != '\0' )
	{
		chatMessage.Append( UTIL_VarArgs( ". Reason: %s", reason ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find players or target groups
//-----------------------------------------------------------------------------
// We're going to be calling the functions below a lot
bool ParsePlayerTargets(
	CBasePlayer *pAdmin,
	AdminReplySource replySource,
	const char *partialName,
	CUtlVector<CBasePlayer *> &targetPlayers,
	CBasePlayer *&pSingleTarget,
	bool excludeDeadPlayers = false
)
{
	pSingleTarget = NULL;
	targetPlayers.RemoveAll();

	if ( partialName[ 0 ] == '@' )
	{
		int index = g_SpecialTargets.Find( partialName );
		if ( index == g_SpecialTargets.InvalidIndex() )
		{
			AdminReply( replySource, pAdmin, "Invalid special target specifier." );
			return false;
		}

		if ( !pAdmin && ( !Q_stricmp( partialName, "@me" ) || !Q_stricmp( partialName, "@!me" ) ) )
		{
			AdminReply( replySource, pAdmin, "The console cannot use special target %s.", partialName );
			return false;
		}

		if ( HL2MPAdmin()->FindSpecialTargetGroup( partialName ) )
		{
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
				if ( !pPlayer || !pPlayer->IsPlayer() )
					continue;

				if ( excludeDeadPlayers && !pPlayer->IsAlive() )
					continue;

				if ( ( HL2MPAdmin()->IsAllPlayers() ) ||
					( HL2MPAdmin()->IsAllBluePlayers() && pPlayer->GetTeamNumber() == TEAM_COMBINE ) ||
					( HL2MPAdmin()->IsAllRedPlayers() && pPlayer->GetTeamNumber() == TEAM_REBELS ) ||
					( HL2MPAdmin()->IsAllButMePlayers() && pPlayer != pAdmin ) ||
					( HL2MPAdmin()->IsMe() && pPlayer == pAdmin ) ||
					( HL2MPAdmin()->IsAllAlivePlayers() && pPlayer->IsAlive() ) ||
					( HL2MPAdmin()->IsAllDeadPlayers() && !pPlayer->IsAlive() ) ||
					( HL2MPAdmin()->IsAllBotsPlayers() && pPlayer->IsPlayerBot() ) ||
					( HL2MPAdmin()->IsAllHumanPlayers() && !pPlayer->IsPlayerBot() ) )
				{
					targetPlayers.AddToTail( pPlayer );
				}
			}
			HL2MPAdmin()->ResetSpecialTargetGroup();
		}

		if ( targetPlayers.Count() == 0 )
		{
			if ( !Q_stricmp( partialName, "@alive" ) )
			{
				AdminReply( replySource, pAdmin, "All players in the target group are dead." );
			}
			else
			{
				AdminReply( replySource, pAdmin, "No players found matching the target group." );
			}			
			return false;
		}

		return true;
	}

	if ( partialName[ 0 ] == '#' )
	{
		int userID = atoi( &partialName[ 1 ] );
		if ( userID > 0 )
		{
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
				if ( pPlayer && pPlayer->GetUserID() == userID )
				{
					if ( excludeDeadPlayers && !pPlayer->IsAlive() )
					{
						AdminReply( replySource, pAdmin, "Player is currently dead." );
						return false;
					}

					pSingleTarget = pPlayer;
					return true;
				}
			}
		}

		AdminReply( replySource, pAdmin, "No player found with that UserID." );
		return false;
	}

	// Partial name search
	CUtlVector<CBasePlayer *> matchingPlayers;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer && Q_stristr( pPlayer->GetPlayerName(), partialName ) )
		{
			matchingPlayers.AddToTail( pPlayer );
		}
	}

	if ( matchingPlayers.Count() == 0 )
	{
		AdminReply( replySource, pAdmin, "No players found matching that name." );
		return false;
	}
	else if ( matchingPlayers.Count() > 1 )
	{
		AdminReply( replySource, pAdmin, "Multiple players match that name:" );
		for ( int i = 0; i < matchingPlayers.Count(); i++ )
		{
			AdminReply( replySource, pAdmin, "%s", matchingPlayers[ i ]->GetPlayerName() );
		}
		return false;
	}

	pSingleTarget = matchingPlayers[ 0 ];

	if ( excludeDeadPlayers && pSingleTarget && !pSingleTarget->IsAlive() )
	{
		AdminReply( replySource, pAdmin, "Player is currently dead." );
		pSingleTarget = NULL;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Print messages when action occurs
//-----------------------------------------------------------------------------
void PrintActionMessage(
	CBasePlayer *pPlayer,
	bool isServerConsole,
	const char *action,
	const char *targetName,
	const char *duration,
	const char *reason
)
{
	CUtlString message;
	const char *adminName = pPlayer ? pPlayer->GetPlayerName() : "Console";

	message.Format( "%s %s %s", adminName, action, targetName );

	// Special case for bans
	if ( duration && duration[ 0 ] != '\0' )
	{
		if ( Q_stricmp( action, "banned" ) == 0 )
		{
			if ( Q_stricmp( duration, "permanently" ) == 0 )
			{
				message.Format( "%s permanently banned %s", adminName, targetName );
			}
			else
			{
				message.Format( "%s banned %s for %s", adminName, targetName, duration );
			}
		}
		else
		{
			message.Append( UTIL_VarArgs( " (%s)", duration ) );
		}
	}

	if ( reason && reason[ 0 ] != '\0' )
	{
		message.Append( UTIL_VarArgs( ". Reason: %s", reason ) );
	}

	UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.\n", message.Get() ) );
}

//-----------------------------------------------------------------------------
// Purpose: Admin say - Makes admin messages stand out for everyone
//-----------------------------------------------------------------------------
static void AdminSay( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	if ( !pPlayer && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "j" ) )
	{
		AdminReply( replySource, pPlayer, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pPlayer, "Usage: sa say <message>" );
		return;
	}

	// Assemble message
	CUtlString messageText;
	for ( int i = 2; i < args.ArgC(); ++i )
	{
		messageText.Append( args[ i ] );
		if ( i < args.ArgC() - 1 )
		{
			messageText.Append( " " );
		}
	}

	CHL2MP_Admin::LogAction( pPlayer, NULL, "sent admin message", messageText.Get() );

	if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "\x04(ADMIN) Console: \x01%s\n", messageText.Get() ) );
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "\x04(ADMIN) %s: \x01%s\n", pPlayer->GetPlayerName(), messageText.Get() ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Admin center say - Posts a centered message like the 
//			"Node Graph Out of Date - Rebuilding" message
//-----------------------------------------------------------------------------
static void AdminCSay( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	if ( !pPlayer && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "j" ) )
	{
		AdminReply( replySource, pPlayer, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pPlayer, "Usage: sa csay <message>" );
		return;
	}

	// Assemble message
	CUtlString messageText;
	for ( int i = 2; i < args.ArgC(); ++i )
	{
		messageText.Append( args[ i ] );
		if ( i < args.ArgC() - 1 )
		{
			messageText.Append( " " );
		}
	}

	CHL2MP_Admin::LogAction( pPlayer, NULL, "sent centered admin message", messageText.Get() );

	if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
	{
		UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "CONSOLE: %s\n", messageText.Get() ) );
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "%s: %s\n", pPlayer->GetPlayerName(), messageText.Get() ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Admin chat (only other admins can see those messages)
//-----------------------------------------------------------------------------
static void AdminChat( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	bool isServerConsole = !pPlayer && UTIL_IsCommandIssuedByServerAdmin();

	if ( !pPlayer && !isServerConsole )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "j" ) )
	{
		AdminReply( replySource, pPlayer, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pPlayer, "Usage: sa chat <message>" );
		return;
	}

	// Construct the message
	CUtlString messageText;
	for ( int i = 2; i < args.ArgC(); ++i )
	{
		messageText.Append( args[ i ] );
		if ( i < args.ArgC() - 1 )
		{
			messageText.Append( " " );
		}
	}

	// Format the message
	CUtlString formattedMessage;
	if ( isServerConsole )
	{
		formattedMessage = UTIL_VarArgs( "\x04(Admin Chat) Console: \x01%s\n", messageText.Get() );
	}
	else
	{
		formattedMessage = UTIL_VarArgs( "\x04(Admin Chat) %s: \x01%s\n", pPlayer->GetPlayerName(), messageText.Get() );
	}

	// Log action
	CHL2MP_Admin::LogAction(
		pPlayer,
		NULL,
		"sent message in admin chat:",
		messageText.Get()
	);

	// Send the message to all admins with at least "b" flag
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pLoopPlayer = UTIL_PlayerByIndex( i );
		if ( pLoopPlayer && CHL2MP_Admin::IsPlayerAdmin( pLoopPlayer, "b" ) )
		{
			ClientPrint( pLoopPlayer, HUD_PRINTTALK, formattedMessage.Get() );
		}
	}

	if ( isServerConsole )
	{
		Msg( "(Admin Chat) Console: %s\n", messageText.Get() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Private messages
//-----------------------------------------------------------------------------
static void AdminPSay( const CCommand &args )
{
	CBasePlayer *pSender = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pSender );
	bool isServerConsole = ( replySource == ADMIN_REPLY_SERVER_CONSOLE );

	if ( !pSender && !isServerConsole )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pSender && !CHL2MP_Admin::IsPlayerAdmin( pSender, "j" ) )
	{
		AdminReply( replySource, pSender, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 4 )
	{
		AdminReply( replySource, pSender, "Usage: sa psay <name|#userID> <message>" );
		return;
	}

	const char *partialName = args.Arg( 2 );

	// Prepare target variables
	CUtlVector<CBasePlayer *> targetPlayers;
	CBasePlayer *pTarget = NULL;

	if ( !ParsePlayerTargets( pSender, replySource, partialName, targetPlayers, pTarget, false ) )
		return;

	// Assemble the message from the remaining arguments
	CUtlString messageText;
	for ( int i = 3; i < args.ArgC(); ++i )
	{
		messageText.Append( args[ i ] );
		if ( i < args.ArgC() - 1 )
		{
			messageText.Append( " " );
		}
	}

	CUtlString formattedMessage;
	if ( isServerConsole )
	{
		formattedMessage = UTIL_VarArgs( "\x04[PRIVATE] Console: \x01%s\n", messageText.Get() );
	}
	else
	{
		formattedMessage = UTIL_VarArgs( "\x04[PRIVATE] %s: \x01%s\n", pSender->GetPlayerName(), messageText.Get() );
	}

	// Send the private message to the target
	ClientPrint( pTarget, HUD_PRINTTALK, formattedMessage.Get() );

	// Show the sender the message they sent (unless the console sent it)
	if ( pTarget != pSender && !isServerConsole )
	{
		ClientPrint( pSender, HUD_PRINTTALK,
			UTIL_VarArgs( "\x04[PRIVATE] To %s: \x01%s\n", pTarget->GetPlayerName(), messageText.Get() ) );
	}
	else if ( isServerConsole )
	{
		Msg( "Private message sent to %s: %s\n", pTarget->GetPlayerName(), messageText.Get() );
	}

	// Log the action
	CHL2MP_Admin::LogAction(
		pSender,
		pTarget,
		"sent private message to",
		messageText.Get()
	);
}

//-----------------------------------------------------------------------------
// Purpose: Reloads the admins list
//-----------------------------------------------------------------------------
static void ReloadAdminsCommand( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );
	bool isServerConsole = ( replySource == ADMIN_REPLY_SERVER_CONSOLE );

	if ( !pPlayer && !isServerConsole )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "i" ) )
	{
		AdminReply( replySource, pPlayer, "You do not have permission to use this command." );
		return;
	}

	CUtlMap<CUtlString, AdminData_t> newAdminMap( DefLessFunc( CUtlString ) );

	if ( !CHL2MP_Admin::ParseAdminFile( "cfg/admin/admins.txt", newAdminMap ) )
	{
		AdminReply( replySource, pPlayer, "Failed to reload admins: Could not parse admins.txt." );
		return;
	}

	// Clear out the entire live list and start fresh
	CHL2MP_Admin::ClearAllAdmins();

	// Rebuild g_AdminList and g_AdminMap from the parsed file
	g_AdminMap.RemoveAll();

	for ( int i = newAdminMap.FirstInorder(); i != newAdminMap.InvalidIndex(); i = newAdminMap.NextInorder( i ) )
	{
		const CUtlString &steamID = newAdminMap.Key( i );
		AdminData_t &adminData = newAdminMap.Element( i );

		CHL2MP_Admin *newAdmin = new CHL2MP_Admin();
		newAdmin->Initialize( steamID.Get(), adminData.flags.Get() );
		g_AdminList.AddToTail( newAdmin );

		g_AdminMap.Insert( steamID, adminData );  // Sync new map to global map
	}

	for ( int i = 0; i < g_AdminList.Count(); ++i )
	{
		CHL2MP_Admin *pAdmin = g_AdminList[ i ];
		CHL2MP_Admin::LogAction( pPlayer, NULL, "loaded admin", pAdmin->GetSteamID() );
	}

	CHL2MP_Admin::SaveAdminCache();

	AdminReply( replySource, pPlayer, "Admins list has been reloaded." );
}

//-----------------------------------------------------------------------------
// Purpose: Help commands
//-----------------------------------------------------------------------------
static void PrintAdminHelp( CBasePlayer *pPlayer = NULL, bool isServerConsole = false )
{
	if ( isServerConsole )
	{
		Msg( "[Server Admin] Usage: sa <command> [arguments]\n" );
		Msg( "===== Root Commands =====\n" );
		Msg( "    say <message> -> Sends an admin formatted message to all players in the chat\n" );
		Msg( "    csay <message> -> Sends a centered message to all players\n" );
		Msg( "    psay <name|#userID> <message> -> Sends a private message to a player\n" );
		Msg( "    chat <message> -> Sends a chat message to connected admins only\n" );
		Msg( "    kick <name|#userID> [reason] -> Kick a player\n" );
		Msg( "    ban <name|#userID> <time> [reason] -> Ban a player\n" );
		Msg( "    addban <time> <SteamID3> [reason] -> Add a manual ban to banned_user.cfg\n" );
		Msg( "    unban <SteamID3> -> Remove a banned SteamID from banned_user.cfg\n" );
		Msg( "    slap <name|#userID> [amount] -> Slap a player with damage if defined\n" );
		Msg( "    slay <name|#userID> -> Slay a player\n" );
		Msg( "    noclip <name|#userID> -> Toggle noclip mode for a player\n" );
		Msg( "    team <name|#userID> <team index> -> Move a player to another team\n" );
		Msg( "    gag <name|#userID> -> Gag a player\n" );
		Msg( "    ungag <name|#userID> -> Ungag a player\n" );
		Msg( "    mute <name|#userID> -> Mute a player\n" );
		Msg( "    unmute <name|#userID> -> Unmute a player\n" );
		Msg( "    bring <name|#userID> -> Teleport a player to where an admin is aiming\n" );
		Msg( "    goto <name|#userID> -> Teleport yourself to a player\n" );
		Msg( "    map <map name> -> Change the map\n" );
		Msg( "    cvar <cvar name> [new value|reset] -> Modify or reset any cvar's value\n" );
		Msg( "    exec <filename> -> Executes a configuration file\n" );
		Msg( "    rcon <command> [value] -> Send a command as if it was written in the server console\n" );
		Msg( "    reloadadmins -> Refresh the admin cache\n" );
		Msg( "    help -> Provide instructions on how to use the admin interface\n" );
		Msg( "    version -> Display version\n\n" );
		return;
	}

	if ( !pPlayer ) 
		return;

	if ( !sv_showadminpermissions.GetBool() || CHL2MP_Admin::IsPlayerAdmin( pPlayer, "z" ) )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "[Server Admin] Usage: sa <command> [arguments]\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "===== Admin Commands =====\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  say <message> -> Sends an admin formatted message to all players in the chat\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  csay <message> -> Sends a centered message to all players\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  psay <name|#userID> <message> -> Sends a private message to a player\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  chat <message> -> Sends a chat message to connected admins only\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  kick <name|#userID> [reason] -> Kick a player\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  ban <name|#userID> <time> [reason] -> Ban a player\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  addban <time> <SteamID3> [reason] -> Add a manual ban to banned_user.cfg\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  unban <SteamID3> -> Remove a banned SteamID from banned_user.cfg\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  slap <name|#userID> [amount] -> Slap a player with damage if defined\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  slay <name|#userID> -> Slay a player\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  noclip <name|#userID> -> Toggle noclip mode for a player\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  team <name|#userID> <team index> -> Move a player to another team\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  gag <name|#userID> -> Gag a player\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  ungag <name|#userID> -> Ungag a player\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  mute <name|#userID> -> Mute a player\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  unmute <name|#userID> -> Unmute a player\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  bring <name|#userID> -> Teleport a player to where an admin is aiming\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  goto <name|#userID> -> Teleport yourself to a player\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  map <map name> -> Change the map\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "   cvar <cvar name> [new value|reset] -> Modify or reset any cvar's value\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  exec <filename> -> Executes a configuration file\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  rcon <command> [value] -> Send a command as if it was written in the server console\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  reloadadmins -> Refresh the admin cache\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  help -> Provide instructions on how to use the admin interface\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  version -> Display version\n\n" );
	}
	else
	{
		// print what an admin has access to based on an admin's level
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "[Server Admin] Usage: sa <command> [arguments]\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "===== Admin Commands =====\n" );
		if ( !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "z" ) )
		{
			if ( CHL2MP_Admin::IsPlayerAdmin( pPlayer, "b" ) )
			{
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  help -> Provide instructions on how to use the admin interface\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  version -> Display version\n" );
			}
			if ( CHL2MP_Admin::IsPlayerAdmin( pPlayer, "c" ) )
			{
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  kick <name|#userID> [reason] -> Kick a player\n" );
			}
			if ( CHL2MP_Admin::IsPlayerAdmin( pPlayer, "d" ) )
			{
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  ban <name|#userID> <time> [reason] -> Ban a player\n" );
			}
			if ( CHL2MP_Admin::IsPlayerAdmin( pPlayer, "e" ) )
			{
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  unban <SteamID3> -> Remove a banned SteamID from banned_user.cfg\n" );
			}
			if ( CHL2MP_Admin::IsPlayerAdmin( pPlayer, "f" ) )
			{
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  slap <name|#userID> [amount] -> Slap a player with damage if defined\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  slay <name|#userID> -> Slay a player\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  noclip <name|#userID> -> Toggle noclip mode for a player\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  team <name|#userID> <team index> -> Move a player to another team\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  bring <name|#userID> -> Teleport a player to where an admin is aiming\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  goto <name|#userID> -> Teleport yourself to a player\n" );
			}
			if ( CHL2MP_Admin::IsPlayerAdmin( pPlayer, "g" ) )
			{
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  map <map name> -> Change the map\n" );
			}
			if ( CHL2MP_Admin::IsPlayerAdmin( pPlayer, "h" ) )
			{
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  cvar <cvar name> [new value] -> Modify any cvar's value\n" );
			}
			if ( CHL2MP_Admin::IsPlayerAdmin( pPlayer, "i" ) )
			{
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  exec <filename> -> Executes a configuration file\n" );
			}
			if ( CHL2MP_Admin::IsPlayerAdmin( pPlayer, "j" ) )
			{
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  gag <name|#userID> -> Gag a player\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  ungag <name|#userID> -> Ungag a player\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  mute <name|#userID> -> Mute a player\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  unmute <name|#userID> -> Unmute a player\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  say <message> -> Sends an admin formatted message to all players in the chat\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  csay <message> -> Sends a centered message to all players\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  psay <name|#userID> <message> -> Sends a private message to a player\n" );
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  chat <message> -> Sends a chat message to connected admins only\n" );
			}

			if ( CHL2MP_Admin::IsPlayerAdmin( pPlayer, "m" ) )
			{
				ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  rcon <command> [value] -> Send a command as if it was written in the server console\n" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Display version
//-----------------------------------------------------------------------------
// 10/17/24 - First version
#define BASE_YEAR 2024
#define BASE_MONTH 10
#define BASE_DAY 17

int GetBuildNumber()
{
	struct tm baseDate = {};
	baseDate.tm_year = BASE_YEAR - 1900;
	baseDate.tm_mon = BASE_MONTH - 1;
	baseDate.tm_mday = BASE_DAY;

	// Current date (parsed from __DATE__)
	struct tm currentDate = {};
	const char *compileDate = __DATE__;
	char monthStr[ 4 ] = {};
	int day, year;

	sscanf( compileDate, "%3s %d %d", monthStr, &day, &year );

	// Convert month string to index
	const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
	const char *pos = strstr( months, monthStr );
	if ( pos )
	{
		currentDate.tm_mon = ( pos - months ) / 3;
	}
	currentDate.tm_year = year - 1900;
	currentDate.tm_mday = day;

	// Convert to time_t
	time_t baseTime = mktime( &baseDate );
	time_t currentTime = mktime( &currentDate );

	if ( baseTime == -1 || currentTime == -1 )
	{
		return 0;
	}

	// Difference in days = build number
	return static_cast< int >( difftime( currentTime, baseTime ) / ( 60 * 60 * 24 ) );
}

static void VersionCommand( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	if ( !pPlayer && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	const char *versionString = UTIL_VarArgs( "Server Admin version %s.%d", VERSION, GetBuildNumber() );

	if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "===== SERVER ADMIN VERSION INFO =====\n" );
		Msg( "%s\n", versionString );
	}
	else
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "===== SERVER ADMIN VERSION INFO =====\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, versionString );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Help instructions for the admin interface
//-----------------------------------------------------------------------------
static void HelpPlayerCommand( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	bool isServerConsole = !pPlayer && UTIL_IsCommandIssuedByServerAdmin();

	if ( !pPlayer && !isServerConsole )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "b" ) )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "You do not have permission to use this command.\n" );
		return;
	}

	if ( isServerConsole )
	{
		Msg( "===== SERVER ADMIN USAGE =====\n"
			"\n"
			"The \"sa\" command provides access to administrative functions for managing the server.\n"
			"\n"
			"To view the list of available commands, type \"sa\" into the console. Different commands are available depending on whether you are using the server console or the client console.\n"
			"\n"
			"Please note that all commands used in the console must start with \"sa\", while commands used in the chat do not.\n"
			"\n"
			"You can target players using either part of their name or their UserID. To find this information, type \"status\" into the console to see a list of connected players and their UserIDs. Example:\n"
			"  sa ban #2 0\n"
			"The number sign (#) is required when targeting players by UserID.\n"
			"\n"
			"Most admin commands do not require quotes around player names. However, if the player's name contains spaces or if you want to match it exactly, quotes can be used. Examples:\n"
			"  sa ban Pet 0 Reason here\n"
			"  sa ban \"Peter Brev\" 0 Reason here\n"
			"  sa ban #2 0 Reason here\n"
			"\n" );
		Msg( "Note: Special group targets always take priority. For example, \"sa ban @all 0\" will ban all players, even if a player is named \"@all\". To target such players, use their UserID instead.\n"
			"\n"
			"You can also type a command with no arguments to see its proper usage. For example:\n"
			"  sa ban\n"
			"will print:\n"
			"  Usage: sa ban <name|#userid> <time> [reason]\n"
			"Angle brackets (< >) indicate required arguments, while square brackets ([ ]) indicate optional ones.\n"
			"\n"
			"Special group targets available:\n"
			"  @all     - All players\n"
			"  @me      - Yourself\n"
			"  @blue    - Combine team\n"
			"  @red     - Rebels team\n"
			"  @!me     - Everyone except yourself\n"
			"  @alive   - All alive players\n"
			"  @dead    - All dead players\n"
			"  @bots    - All bots\n"
			"  @humans  - All human players\n"
			"\n"
			"Reply Behavior:\n"
			"If you type commands directly into the console, responses will appear in the console.\n"
			"If you type commands in chat (using ! or /), responses will appear in the chat box visible to you.\n"
			"\n"
		);

	}
	else
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "===== SERVER ADMIN USAGE =====\n\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "The \"sa\" command provides access to administrative functions for managing the server.\n\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "To view the list of available commands, type \"sa\" into the console.\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "The list of commands available to you depends on whether you're using the server console or the client console.\n\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Please note that all commands used in the console must start with \"sa\", while commands used in the chat do not.\n\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "You can target players by name or UserID. Use \"status\" to list players and their UserIDs.\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Example:\n  sa ban #2 0 Reason here\nThe # is required when using UserIDs.\n\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Most commands don't require quotes around names, but they are allowed if the name has spaces or for exact matching.\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Examples:\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  sa ban Pet 0 Reason here\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  sa ban \"Peter Brev\" 0 Reason here\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  sa ban #2 0 Reason here\n\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Note: Special group targets always have priority.\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  Example: sa ban @all 0\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "This will ban all players, even if one player is named \"@all\". Use their UserID to target them directly.\n\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Type a command with no arguments to see its correct syntax. Example:\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  sa ban\nThis prints:\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  Usage: sa ban <name|#userid> <time> [reason]\n\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Special group targets:\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  @all     - All players\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  @me      - Yourself\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  @blue    - Combine team\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  @red     - Rebels team\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  @!me     - Everyone except you\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  @alive   - All alive players\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  @dead    - All dead players\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  @bots    - All bots\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  @humans  - All human players\n\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Reply Behavior:\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  Commands typed in the console will reply in the console.\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "  Commands typed in chat (with ! or /) will reply in the chat box.\n\n" );

	}
	return;
}

void InitializeSpecialTargets()
{
	g_SpecialTargets.Insert( "@all", TARGET_ALL );
	g_SpecialTargets.Insert( "@blue", TARGET_BLUE );
	g_SpecialTargets.Insert( "@red", TARGET_RED );
	g_SpecialTargets.Insert( "@!me", TARGET_ALL_BUT_ME );
	g_SpecialTargets.Insert( "@me", TARGET_ME );
	g_SpecialTargets.Insert( "@alive", TARGET_ALIVE );
	g_SpecialTargets.Insert( "@dead", TARGET_DEAD );
	g_SpecialTargets.Insert( "@bots", TARGET_BOTS );
	g_SpecialTargets.Insert( "@humans", TARGET_HUMANS );
}

void CHL2MP_Admin::ResetSpecialTargetGroup()
{
	bAll = bBlue = bRed = bAllButMe = bMe = bAlive = bDead = bBots = bHumans = false;
}

bool CHL2MP_Admin::FindSpecialTargetGroup( const char *targetSpecifier )
{
	int index = g_SpecialTargets.Find( targetSpecifier );

	if ( index == g_SpecialTargets.InvalidIndex() )
		return false;

	switch ( g_SpecialTargets[ index ] )
	{
	case TARGET_ALL: bAll = true; break;
	case TARGET_BLUE: bBlue = true; break;
	case TARGET_RED: bRed = true; break;
	case TARGET_ALL_BUT_ME: bAllButMe = true; break;
	case TARGET_ME: bMe = true; break;
	case TARGET_ALIVE: bAlive = true; break;
	case TARGET_DEAD: bDead = true; break;
	case TARGET_BOTS: bBots = true; break;
	case TARGET_HUMANS: bHumans = true; break;
	default: return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Check for any human players
//-----------------------------------------------------------------------------
static bool ArePlayersInGame()
{
	// mainly for the change level admin command
	// if no player connected to the server,
	// the time remains frozen, therefore the level
	// never changes; this makes it so that if no
	// players are connected, it changes the level
	// immediately rather than after 5 seconds
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->IsConnected() && !pPlayer->IsBot() )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Execute files
//-----------------------------------------------------------------------------
static void ExecFileCommand( const CCommand &args )
{
	CBasePlayer *pAdmin = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pAdmin );

	if ( !pAdmin && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pAdmin && !CHL2MP_Admin::IsPlayerAdmin( pAdmin, "i" ) )
	{
		AdminReply( replySource, pAdmin, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pAdmin, "Usage: sa exec <filename>" );
		return;
	}

	const char *inputFilename = args.Arg( 2 );

	// Make the .cfg extension optional but if server ops add it, 
	// then take it into account to avoid a double extension
	char filename[ MAX_PATH ];
	if ( !Q_stristr( inputFilename, ".cfg" ) )
	{
		Q_snprintf( filename, sizeof( filename ), "%s.cfg", inputFilename );
	}
	else
	{
		Q_strncpy( filename, inputFilename, sizeof( filename ) );
	}

	char fullPath[ MAX_PATH ];
	Q_snprintf( fullPath, sizeof( fullPath ), "cfg/%s", filename );

	if ( !filesystem->FileExists( fullPath ) )
	{
		AdminReply( replySource, pAdmin, UTIL_VarArgs( "Config file '%s' not found in cfg folder.", filename ) );
		return;
	}

	engine->ServerCommand( UTIL_VarArgs( "exec %s\n", filename ) );

	CHL2MP_Admin::LogAction(
		pAdmin,
		NULL,
		"executed config file",
		filename
	);

	CUtlString message;
	if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Executing config file: %s\n", filename );
	}
	else
	{
		message.Format( "Admin %s executed config file: %s",
			pAdmin ? pAdmin->GetPlayerName() : "Console", filename );

		UTIL_ClientPrintAll( HUD_PRINTTALK, message.Get() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Toggle noclip
//-----------------------------------------------------------------------------
static void ToggleNoClipForPlayer( CBasePlayer *pTarget )
{
	if ( pTarget->GetMoveType() == MOVETYPE_NOCLIP )
	{
		pTarget->SetMoveType( MOVETYPE_WALK );
	}
	else
	{
		pTarget->SetMoveType( MOVETYPE_NOCLIP );
	}
}

static void NoClipPlayerCommand( const CCommand &args )
{
	CBasePlayer *pAdmin = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pAdmin );

	if ( !pAdmin && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pAdmin && !CHL2MP_Admin::IsPlayerAdmin( pAdmin, "f" ) )
	{
		AdminReply( replySource, pAdmin, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pAdmin, "Usage: sa noclip <name|#userID>" );
		return;
	}

	const char *partialName = args.Arg( 2 );
	CUtlVector<CBasePlayer *> targetPlayers;
	CBasePlayer *pTarget = NULL;

	if ( !ParsePlayerTargets( pAdmin, replySource, partialName, targetPlayers, pTarget, true ) )
		return;

	if ( pTarget )
	{
		ToggleNoClipForPlayer( pTarget );

		CHL2MP_Admin::LogAction( pAdmin, pTarget, "toggled noclip for", "" );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "Console toggled noclip for player %s.\n", pTarget->GetPlayerName() );
			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs(
				"Console toggled noclip for %s\n",
				pTarget->GetPlayerName()
			) );
		}
		else
		{
			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs(
				"Admin %s toggled noclip for %s\n",
				pAdmin ? pAdmin->GetPlayerName() : "Console", pTarget->GetPlayerName()
			) );
		}

		return;
	}

	for ( int i = 0; i < targetPlayers.Count(); i++ )
	{
		if ( targetPlayers[ i ]->IsAlive() )
		{
			ToggleNoClipForPlayer( targetPlayers[ i ] );
		}
	}

	CUtlString logDetails, chatMessage;
	BuildGroupTargetMessage( partialName, pAdmin, "toggled noclip for", NULL, logDetails, chatMessage, false, NULL );

	CHL2MP_Admin::LogAction( pAdmin, NULL, "toggled noclip for", logDetails.Get() );

	UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.\n", chatMessage.Get() ) );

	if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Toggled noclip for %d player%s\n", targetPlayers.Count(), targetPlayers.Count() == 1 ? "" : "s" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Teleport to a player
//-----------------------------------------------------------------------------
static void GotoPlayerCommand( const CCommand &args )
{
	CBasePlayer *pAdmin = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pAdmin );

	if ( !pAdmin )
	{
		AdminReply( replySource, pAdmin, "Command must be issued by a player." );
		return;
	}

	if ( !CHL2MP_Admin::IsPlayerAdmin( pAdmin, "f" ) )
	{
		AdminReply( replySource, pAdmin, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pAdmin, "Usage: sa goto <name|#userID>" );
		return;
	}

	const char *targetPlayerInput = args.Arg( 2 );
	CBasePlayer *pTarget = NULL;

	if ( targetPlayerInput[ 0 ] == '#' )
	{
		int userID = atoi( &targetPlayerInput[ 1 ] );
		if ( userID <= 0 )
		{
			AdminReply( replySource, pAdmin, "Invalid UserID provided." );
			return;
		}

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pLoopPlayer = UTIL_PlayerByIndex( i );
			if ( pLoopPlayer && pLoopPlayer->GetUserID() == userID )
			{
				pTarget = pLoopPlayer;
				break;
			}
		}

		if ( !pTarget )
		{
			AdminReply( replySource, pAdmin, "No player found with that UserID." );
			return;
		}
	}
	else
	{
		CUtlVector<CBasePlayer *> matchingPlayers;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pLoopPlayer = UTIL_PlayerByIndex( i );
			if ( pLoopPlayer && Q_stristr( pLoopPlayer->GetPlayerName(), targetPlayerInput ) )
			{
				matchingPlayers.AddToTail( pLoopPlayer );
			}
		}

		if ( matchingPlayers.Count() == 0 )
		{
			AdminReply( replySource, pAdmin, "No players found matching that name." );
			return;
		}
		else if ( matchingPlayers.Count() > 1 )
		{
			AdminReply( replySource, pAdmin, "Multiple players match that partial name:" );
			for ( int i = 0; i < matchingPlayers.Count(); i++ )
			{
				AdminReply( replySource, pAdmin, "%s", matchingPlayers[ i ]->GetPlayerName() );
			}
			return;
		}

		pTarget = matchingPlayers[ 0 ];
	}

	if ( !pTarget )
	{
		AdminReply( replySource, pAdmin, "Player not found." );
		return;
	}

	if ( !pTarget->IsAlive() )
	{
		AdminReply( replySource, pAdmin, "This player is currently dead." );
		return;
	}

	if ( pAdmin->IsAlive() )
	{
		Vector targetPosition = pTarget->GetAbsOrigin();
		targetPosition.z += 80.0f;  // Slightly above player to avoid getting stuck

		pAdmin->SetAbsOrigin( targetPosition );

		CHL2MP_Admin::LogAction( pAdmin, pTarget, "teleported to", "" );

		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs(
			"Admin %s teleported to %s\n",
			pAdmin->GetPlayerName(), pTarget->GetPlayerName()
		) );
	}
	else
	{
		AdminReply( replySource, pAdmin, "You must be alive to teleport to a player." );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Teleport players to where an admin is aiming
//-----------------------------------------------------------------------------
static void BringPlayerCommand( const CCommand &args )
{
	CBasePlayer *pAdmin = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pAdmin );

	if ( !pAdmin )
	{
		AdminReply( replySource, pAdmin, "Command must be issued by a player." );
		return;
	}

	if ( !CHL2MP_Admin::IsPlayerAdmin( pAdmin, "f" ) )
	{
		AdminReply( replySource, pAdmin, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pAdmin, "Usage: sa bring <name|#userID>" );
		return;
	}

	const char *targetPlayerInput = args.Arg( 2 );
	CBasePlayer *pTarget = NULL;

	if ( Q_stricmp( targetPlayerInput, "@me" ) == 0 )
	{
		pTarget = pAdmin;  // Admin can bring themselves (useful for debugging)
	}
	else if ( targetPlayerInput[ 0 ] == '#' )
	{
		int userID = atoi( &targetPlayerInput[ 1 ] );
		if ( userID <= 0 )
		{
			AdminReply( replySource, pAdmin, "Invalid UserID provided." );
			return;
		}

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pLoopPlayer = UTIL_PlayerByIndex( i );
			if ( pLoopPlayer && pLoopPlayer->GetUserID() == userID )
			{
				pTarget = pLoopPlayer;
				break;
			}
		}

		if ( !pTarget )
		{
			AdminReply( replySource, pAdmin, "No player found with that UserID." );
			return;
		}
	}
	else
	{
		CUtlVector<CBasePlayer *> matchingPlayers;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pLoopPlayer = UTIL_PlayerByIndex( i );
			if ( pLoopPlayer && Q_stristr( pLoopPlayer->GetPlayerName(), targetPlayerInput ) )
			{
				matchingPlayers.AddToTail( pLoopPlayer );
			}
		}

		if ( matchingPlayers.Count() == 0 )
		{
			AdminReply( replySource, pAdmin, "No players found matching that name." );
			return;
		}
		else if ( matchingPlayers.Count() > 1 )
		{
			AdminReply( replySource, pAdmin, "Multiple players match that partial name:" );
			for ( int i = 0; i < matchingPlayers.Count(); i++ )
			{
				AdminReply( replySource, pAdmin, matchingPlayers[ i ]->GetPlayerName() );
			}
			return;
		}

		pTarget = matchingPlayers[ 0 ];
	}

	if ( pTarget )
	{
		if ( pAdmin->IsAlive() && pAdmin->GetTeamNumber() != TEAM_SPECTATOR )
		{
			Vector forward;
			trace_t tr;

			pAdmin->EyeVectors( &forward );
			UTIL_TraceLine( pAdmin->EyePosition(), pAdmin->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pAdmin, COLLISION_GROUP_NONE, &tr );

			Vector targetPosition = tr.endpos;
			pTarget->SetAbsOrigin( targetPosition );

			CHL2MP_Admin::LogAction( pAdmin, pTarget, "teleported", "" );

			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs(
				"Admin %s teleported player %s\n",
				pAdmin->GetPlayerName(), pTarget->GetPlayerName()
			) );
		}
		else
		{
			AdminReply( replySource, pAdmin, "You must be alive to teleport a player." );
		}
	}
	else
	{
		AdminReply( replySource, pAdmin, "Player not found." );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Change a player's team
//-----------------------------------------------------------------------------
static void TeamPlayerCommand( const CCommand &args )
{
	CBasePlayer *pAdmin = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pAdmin );

	if ( !pAdmin && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pAdmin && !CHL2MP_Admin::IsPlayerAdmin( pAdmin, "f" ) )
	{
		AdminReply( replySource, pAdmin, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 4 )
	{
		AdminReply( replySource, pAdmin, "Usage: sa team <name|#userID> <team index>" );
		return;
	}

	const char *partialName = args.Arg( 2 );
	int teamIndex = atoi( args.Arg( 3 ) );

	if ( teamIndex < 1 || teamIndex > 3 )
	{
		AdminReply( replySource, pAdmin, "Invalid team index. Team index must be between 1 and 3." );
		return;
	}

	CUtlVector<CBasePlayer *> targetPlayers;
	CBasePlayer *pTarget = NULL;

	if ( !ParsePlayerTargets( pAdmin, replySource, partialName, targetPlayers, pTarget ) )
		return;

	const char *teamName = "Unassigned"; // Default (index == 0)
	if ( teamIndex == 1 )
	{
		teamName = "Spectator";
	}
	else if ( HL2MPRules()->IsTeamplay() )
	{
		if ( teamIndex == 2 )
			teamName = "Combine";
		else if ( teamIndex == 3 )
			teamName = "Rebels";
	}

	auto MovePlayerToTeam = [ teamIndex ]( CBasePlayer *pPlayer )
		{
			pPlayer->ChangeTeam( teamIndex );
			pPlayer->Spawn();
		};

	CUtlString logDetails, chatMessage;

	if ( pTarget )
	{
		if ( ( pTarget->GetTeamNumber() == teamIndex ) || ( ( teamIndex == 2 || teamIndex == 3 ) && pTarget->GetTeamNumber() == TEAM_UNASSIGNED ) )
		{
			AdminReply( replySource, pAdmin, "Player %s is already on team %s.", pTarget->GetPlayerName(), teamName );
			return;
		}

		MovePlayerToTeam( pTarget );

		CHL2MP_Admin::LogAction( pAdmin, pTarget, "moved", UTIL_VarArgs( "to team %s", teamName ) );

		CUtlString teamMessage;

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			teamMessage.Format( "Console moved player %s to team %s.", pTarget->GetPlayerName(), teamName );
		}
		else
		{
			teamMessage.Format( "Admin %s moved player %s to team %s.", pAdmin->GetPlayerName(), pTarget->GetPlayerName(), teamName );
		}

		UTIL_ClientPrintAll( HUD_PRINTTALK, teamMessage.Get() );
	}
	else
	{
		if ( targetPlayers.Count() == 0 )
		{
			AdminReply( replySource, pAdmin, "No players found matching the criteria." );
			return;
		}

		int movedPlayersCount = 0;

		for ( int i = 0; i < targetPlayers.Count(); i++ )
		{
			CBasePlayer *pPlayer = targetPlayers[ i ];

			// Skip players already on the desired team
			if ( ( pPlayer->GetTeamNumber() == teamIndex ) || ( ( teamIndex == 2 || teamIndex == 3 ) && pPlayer->GetTeamNumber() == TEAM_UNASSIGNED ) )
			{
				continue;
			}

			MovePlayerToTeam( pPlayer );
			movedPlayersCount++;
		}

		if ( movedPlayersCount == 0 )
		{
			AdminReply( replySource, pAdmin, "All selected players are already on team %s.", teamName );
			return;
		}

		BuildGroupTargetMessage( partialName, pAdmin, "moved", NULL, logDetails, chatMessage, false );

		CHL2MP_Admin::LogAction( pAdmin, NULL, "moved", UTIL_VarArgs( "%s to team %s", logDetails.Get(), teamName ), partialName + 1 );

		chatMessage.Append( UTIL_VarArgs( " to team %s", teamName ) );

		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.", chatMessage.Get() ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Unmute a player
//-----------------------------------------------------------------------------
static void UnMutePlayerCommand( const CCommand &args )
{
	CBasePlayer *pAdmin = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pAdmin );

	if ( !pAdmin && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pAdmin && !CHL2MP_Admin::IsPlayerAdmin( pAdmin, "j" ) ) // 'j' flag for mute/unmute permission
	{
		AdminReply( replySource, pAdmin, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pAdmin, "Usage: sa unmute <name|#userID>" );
		return;
	}

	const char *partialName = args.Arg( 2 );
	CUtlVector<CBasePlayer *> targetPlayers;
	CBasePlayer *pTarget = NULL;

	if ( !ParsePlayerTargets( pAdmin, replySource, partialName, targetPlayers, pTarget ) )
		return;

	CUtlString logDetails, chatMessage;

	auto ExecuteUnmute = []( CBasePlayer *pTarget )
		{
			if ( pTarget->IsMuted() )
			{
				pTarget->SetMuted( false );
			}
		};

	if ( pTarget )
	{
		if ( !pTarget->IsMuted() )
		{
			AdminReply( replySource, pAdmin, "Player is not muted." );
			return;
		}

		ExecuteUnmute( pTarget );

		CHL2MP_Admin::LogAction( pAdmin, pTarget, "unmuted", "" );

		PrintActionMessage( pAdmin, replySource == ADMIN_REPLY_SERVER_CONSOLE, "unmuted", pTarget->GetPlayerName(), NULL, NULL );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "Unmuted player %s\n", pTarget->GetPlayerName() );
		}
	}
	else
	{
		int unmutedCount = 0;

		for ( int i = 0; i < targetPlayers.Count(); i++ )
		{
			if ( targetPlayers[ i ]->IsMuted() )
			{
				ExecuteUnmute( targetPlayers[ i ] );
				unmutedCount++;
			}
		}

		BuildGroupTargetMessage( partialName, pAdmin, "unmuted", NULL, logDetails, chatMessage, false );

		CHL2MP_Admin::LogAction( pAdmin, NULL, "unmuted", logDetails.Get(), partialName + 1 );

		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.", chatMessage.Get() ) );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "Unmuted %d player%s\n", unmutedCount, unmutedCount == 1 ? "" : "s" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Mute player
//-----------------------------------------------------------------------------
static void MutePlayerCommand( const CCommand &args )
{
	CBasePlayer *pAdmin = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pAdmin );

	if ( !pAdmin && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pAdmin && !CHL2MP_Admin::IsPlayerAdmin( pAdmin, "j" ) ) // 'j' flag for mute permission
	{
		AdminReply( replySource, pAdmin, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pAdmin, "Usage: sa mute <name|#userID>" );
		return;
	}

	const char *partialName = args.Arg( 2 );
	CUtlVector<CBasePlayer *> targetPlayers;
	CBasePlayer *pTarget = NULL;

	if ( !ParsePlayerTargets( pAdmin, replySource, partialName, targetPlayers, pTarget ) )
	{
		return;
	}

	CUtlString logDetails, chatMessage;

	auto ExecuteMute = []( CBasePlayer *pTarget )
		{
			if ( !pTarget->IsMuted() )
			{
				pTarget->SetMuted( true );
			}
		};

	if ( pTarget )
	{
		if ( pTarget->IsMuted() )
		{
			AdminReply( replySource, pAdmin, "Player is already muted." );
			return;
		}

		ExecuteMute( pTarget );

		CHL2MP_Admin::LogAction( pAdmin, pTarget, "muted", "" );

		PrintActionMessage( pAdmin, replySource == ADMIN_REPLY_SERVER_CONSOLE, "muted", pTarget->GetPlayerName(), NULL, NULL );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "Muted player %s\n", pTarget->GetPlayerName() );
		}
	}
	else
	{
		int mutedCount = 0;

		for ( int i = 0; i < targetPlayers.Count(); i++ )
		{
			if ( !targetPlayers[ i ]->IsMuted() )
			{
				ExecuteMute( targetPlayers[ i ] );
				mutedCount++;
			}
		}

		BuildGroupTargetMessage( partialName, pAdmin, "muted", NULL, logDetails, chatMessage, false );

		CHL2MP_Admin::LogAction( pAdmin, NULL, "muted", logDetails.Get(), partialName + 1 );

		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.", chatMessage.Get() ) );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "Muted %d player%s\n", mutedCount, mutedCount == 1 ? "" : "s" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Ungag player
//-----------------------------------------------------------------------------
static void UnGagPlayerCommand( const CCommand &args )
{
	CBasePlayer *pAdmin = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pAdmin );

	if ( !pAdmin && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pAdmin && !CHL2MP_Admin::IsPlayerAdmin( pAdmin, "j" ) ) // 'j' flag for gag/ungag permissions
	{
		AdminReply( replySource, pAdmin, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pAdmin, "Usage: sa ungag <name|#userID>" );
		return;
	}

	const char *partialName = args.Arg( 2 );
	CUtlVector<CBasePlayer *> targetPlayers;
	CBasePlayer *pTarget = NULL;

	if ( !ParsePlayerTargets( pAdmin, replySource, partialName, targetPlayers, pTarget ) )
		return;

	CUtlString logDetails, chatMessage;

	auto ExecuteUnGag = []( CBasePlayer *pTarget ) {
		if ( pTarget->IsGagged() )
		{
			pTarget->SetGagged( false );
		}
		};

	if ( pTarget )
	{
		if ( !pTarget->IsGagged() )
		{
			AdminReply( replySource, pAdmin, "Player is not gagged." );
			return;
		}

		ExecuteUnGag( pTarget );

		CHL2MP_Admin::LogAction( pAdmin, pTarget, "ungagged", "" );

		PrintActionMessage( pAdmin, replySource == ADMIN_REPLY_SERVER_CONSOLE, "ungagged", pTarget->GetPlayerName(), NULL, NULL );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "Ungagged player %s\n", pTarget->GetPlayerName() );
		}
	}
	else
	{
		int ungaggedCount = 0;
		for ( int i = 0; i < targetPlayers.Count(); i++ )
		{
			if ( targetPlayers[ i ]->IsGagged() )
			{
				ExecuteUnGag( targetPlayers[ i ] );
				ungaggedCount++;
			}
		}

		BuildGroupTargetMessage( partialName, pAdmin, "ungagged", NULL, logDetails, chatMessage, false );

		CHL2MP_Admin::LogAction( pAdmin, NULL, "ungagged", logDetails.Get(), partialName + 1 );

		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.", chatMessage.Get() ) );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "Ungagged %d player%s\n", ungaggedCount, ungaggedCount == 1 ? "" : "s" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gag player
//-----------------------------------------------------------------------------
static void GagPlayerCommand( const CCommand &args )
{
	CBasePlayer *pAdmin = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pAdmin );

	if ( !pAdmin && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pAdmin && !CHL2MP_Admin::IsPlayerAdmin( pAdmin, "j" ) ) // 'j' flag for gag permission
	{
		AdminReply( replySource, pAdmin, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pAdmin, "Usage: sa gag <name|#userID>" );
		return;
	}

	const char *partialName = args.Arg( 2 );
	CUtlVector<CBasePlayer *> targetPlayers;
	CBasePlayer *pTarget = NULL;

	if ( !ParsePlayerTargets( pAdmin, replySource, partialName, targetPlayers, pTarget ) )
		return;

	CUtlString logDetails, chatMessage;

	auto ExecuteGag = []( CBasePlayer *pTarget )
		{
			if ( !pTarget->IsGagged() )
			{
				pTarget->SetGagged( true );
			}
		};

	if ( pTarget )
	{
		if ( pTarget->IsGagged() )
		{
			AdminReply( replySource, pAdmin, "Player is already gagged." );
			return;
		}

		ExecuteGag( pTarget );

		CHL2MP_Admin::LogAction( pAdmin, pTarget, "gagged", "" );

		PrintActionMessage( pAdmin, replySource == ADMIN_REPLY_SERVER_CONSOLE, "gagged", pTarget->GetPlayerName(), NULL, NULL );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "Gagged player %s\n", pTarget->GetPlayerName() );
		}
	}
	else
	{
		int gaggedCount = 0;
		for ( int i = 0; i < targetPlayers.Count(); i++ )
		{
			if ( !targetPlayers[ i ]->IsGagged() )
			{
				ExecuteGag( targetPlayers[ i ] );
				gaggedCount++;
			}
		}

		BuildGroupTargetMessage( partialName, pAdmin, "gagged", NULL, logDetails, chatMessage, false );

		CHL2MP_Admin::LogAction( pAdmin, NULL, "gagged", logDetails.Get(), partialName + 1 );

		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.", chatMessage.Get() ) );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "Gagged %d player%s\n", gaggedCount, gaggedCount == 1 ? "" : "s" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Change map
//-----------------------------------------------------------------------------
static void MapCommand( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );
	bool isServerConsole = ( replySource == ADMIN_REPLY_SERVER_CONSOLE );

	if ( !pPlayer && !isServerConsole )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "g" ) )
	{
		AdminReply( replySource, pPlayer, "You do not have permission to use this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pPlayer, "Usage: sa map <mapname>" );
		return;
	}

	if ( HL2MPRules()->IsMapChangeOnGoing() ) // Stops admins from spamming map changes
	{
		AdminReply( replySource, pPlayer, "A map change is already in progress..." );
		return;
	}

	const char *partialMapName = args.Arg( 2 );
	CUtlVector<char *> matchingMaps;
	char *exactMatchMap = NULL;

	// Find maps
	FileFindHandle_t fileHandle;
	const char *mapPath = filesystem->FindFirst( "maps/*.bsp", &fileHandle );

	while ( mapPath )
	{
		char mapName[ 256 ];
		V_FileBase( mapPath, mapName, sizeof( mapName ) );

		if ( Q_stricmp( mapName, partialMapName ) == 0 )
		{
			exactMatchMap = strdup( mapName );
			break;
		}

		if ( Q_stristr( mapName, partialMapName ) )
		{
			matchingMaps.AddToTail( strdup( mapName ) );
		}

		mapPath = filesystem->FindNext( fileHandle );
	}
	filesystem->FindClose( fileHandle );

	if ( exactMatchMap )
	{
		if ( !ArePlayersInGame() )
		{
			engine->ServerCommand( UTIL_VarArgs( "changelevel %s\n", exactMatchMap ) );
		}
		else
		{
			HL2MPRules()->SetScheduledMapName( exactMatchMap );
			HL2MPRules()->SetMapChange( true );
			HL2MPRules()->SetMapChangeOnGoing( true );
			bAdminMapChange = true;

			const char *adminName = isServerConsole ? "Console" : pPlayer->GetPlayerName();
			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s is changing the map to %s in 5 seconds...\n", adminName, exactMatchMap ) );

			engine->ServerCommand( "mp_timelimit 0\n" );
			CHL2MP_Admin::LogAction( pPlayer, NULL, "changed map", UTIL_VarArgs( "to %s", exactMatchMap ) );
		}

		free( exactMatchMap );
		return;
	}

	if ( matchingMaps.Count() == 0 )
	{
		AdminReply( replySource, pPlayer, UTIL_VarArgs( "No maps found matching \"%s\".", partialMapName ) );
		return;
	}

	if ( matchingMaps.Count() == 1 )
	{
		const char *selectedMap = matchingMaps[ 0 ];

		if ( !ArePlayersInGame() )
		{
			engine->ServerCommand( UTIL_VarArgs( "changelevel %s\n", selectedMap ) );
		}
		else
		{
			HL2MPRules()->SetScheduledMapName( selectedMap );
			HL2MPRules()->SetMapChange( true );
			HL2MPRules()->SetMapChangeOnGoing( true );
			bAdminMapChange = true;

			const char *adminName = isServerConsole ? "Console" : pPlayer->GetPlayerName();
			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s is changing the map to %s in 5 seconds...\n", adminName, selectedMap ) );

			CHL2MP_Admin::LogAction( pPlayer, NULL, "changed map", UTIL_VarArgs( "to %s", selectedMap ) );
		}
	}
	else
	{
		AdminReply( replySource, pPlayer, "Multiple maps match the partial name:" );
		for ( int i = 0; i < matchingMaps.Count(); i++ )
		{
			AdminReply( replySource, pPlayer, "%s", matchingMaps[ i ] );
		}
	}

	for ( int i = 0; i < matchingMaps.Count(); i++ )
	{
		free( matchingMaps[ i ] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Rcon
//-----------------------------------------------------------------------------
static void RconCommand( const CCommand &args )
{
	// For rcon, we are only making this command available to players in-game
	// (meaning it will not do anything if used within the server console directly)
	// because it makes no sense to use rcon in the server console, but we are also
	// disabling the usage of all "sa" commands with "sa rcon" since if the admin has rcon,
	// they probably have root privileges already.

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	bool isServerConsole = ( replySource == ADMIN_REPLY_SERVER_CONSOLE );

	if ( !pPlayer && !isServerConsole )
	{
		Msg( "Command must be issued by a player\n" );
		return;
	}

	if ( isServerConsole )
	{
		Msg( "Command must be issued by a player\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "m" ) )
	{
		AdminReply( replySource, pPlayer, "You do not have permission to use this command." );
		return;
	}

	const char *usage = "sa rcon <command> [arguments]";
	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pPlayer, "Usage: %s", usage );
		return;
	}

	const char *commandName = args.Arg( 2 );

	if ( Q_stricmp( commandName, "sa" ) == 0 )
	{
		AdminReply( replySource, pPlayer, "No \"sa rcon\" needed with commands starting with \"%s\"", commandName );
		return;
	}

	ConVar *pConVar = g_pCVar->FindVar( commandName );

	if ( pConVar && args.ArgC() == 3 )
	{
		if ( !( pConVar->IsFlagSet( FCVAR_NEVER_AS_STRING ) ) )  // String-type cvars
		{
			AdminReply( replySource, pPlayer, "Cvar %s is set to \"%s\"",
				commandName, pConVar->GetString() );
		}
		else  // Numeric cvars
		{
			float value = pConVar->GetFloat();
			if ( fabs( value - roundf( value ) ) < 0.0001f )
			{
				AdminReply( replySource, pPlayer, "Cvar %s is set to %d",
					commandName, static_cast< int >( value ) );
			}
			else
			{
				AdminReply( replySource, pPlayer, "Cvar %s is set to %f",
					commandName, value );
			}
		}
		return;
	}

	// Assemble command (handles `sv_tags`, etc.)
	CUtlString rconCommand;
	for ( int i = 2; i < args.ArgC(); i++ )
	{
		rconCommand.Append( args.Arg( i ) );
		if ( i < args.ArgC() - 1 )
		{
			rconCommand.Append( " " );
		}
	}

	engine->ServerCommand( UTIL_VarArgs( "%s\n", rconCommand.Get() ) );

	CHL2MP_Admin::LogAction( pPlayer, NULL, "executed rcon", rconCommand.Get() );

	AdminReply( replySource, pPlayer, "Rcon command issued: %s", rconCommand.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: Cvar
//-----------------------------------------------------------------------------
static void CVarCommand( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	bool isServerConsole = ( replySource == ADMIN_REPLY_SERVER_CONSOLE );

	if ( !pPlayer && !isServerConsole )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "h" ) )
	{
		AdminReply( replySource, pPlayer, "You do not have permission to use this command." );
		return;
	}

	const char *usage = "Usage: sa cvar <cvarname> [newvalue]";
	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pPlayer, "%s", usage );
		return;
	}

	const char *cvarName = args.Arg( 2 );
	ConVar *pConVar = cvar->FindVar( cvarName );

	if ( !pConVar )
	{
		AdminReply( replySource, pPlayer, "Cvar %s not found.", cvarName );
		return;
	}

	bool requiresCheatFlag = pConVar->IsFlagSet( FCVAR_CHEAT );
	if ( requiresCheatFlag && pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "n" ) )
	{
		AdminReply( replySource, pPlayer, "You do not have permission to change cheat-protected cvars." );
		return;
	}

	// Show the admin the value if they're not changing it
	if ( args.ArgC() == 3 )
	{
		if ( pConVar->IsFlagSet( FCVAR_PROTECTED ) || Q_stricmp( cvarName, "sv_password" ) == 0 )
		{
			AdminReply( replySource, pPlayer, "Value is protected and cannot be displayed." );
			return;
		}

		if ( !( pConVar->IsFlagSet( FCVAR_NEVER_AS_STRING ) ) )  // String-type cvar
		{
			AdminReply( replySource, pPlayer, "Cvar %s is currently set to \"%s\"",
				cvarName, pConVar->GetString() );
		}
		else  // Numeric value
		{
			float value = pConVar->GetFloat();
			if ( fabs( value - roundf( value ) ) < 0.0001f )
			{
				AdminReply( replySource, pPlayer, "Cvar %s is currently set to %d",
					cvarName, static_cast< int >( value ) );
			}
			else
			{
				AdminReply( replySource, pPlayer, "Cvar %s is currently set to %f",
					cvarName, value );
			}
		}
		return;
	}

	// Assemble new value for cvars that allow spaces (e.g., sv_tags, hostname)
	CUtlString newValue;
	for ( int i = 3; i < args.ArgC(); i++ )
	{
		newValue.Append( args.Arg( i ) );
		if ( i < args.ArgC() - 1 )
		{
			newValue.Append( " " );
		}
	}

	if ( Q_stricmp( newValue.Get(), "reset" ) == 0 )
	{
		pConVar->Revert();

		const char *adminName = pPlayer ? pPlayer->GetPlayerName() : "Console";

		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs(
			"Admin %s reset cvar %s to default value.\n",
			adminName, cvarName
		) );

		CHL2MP_Admin::LogAction( pPlayer, NULL, "reset cvar", UTIL_VarArgs( "%s reset %s to default value", adminName, cvarName ) );
		return;
	}

	pConVar->SetValue( newValue.Get() );

	CHL2MP_Admin::LogAction( pPlayer, NULL, "changed cvar", UTIL_VarArgs(
		"%s to %s",
		cvarName,
		pConVar->IsFlagSet( FCVAR_PROTECTED ) ? "***PROTECTED***" : newValue.Get()
	) );

	if ( pConVar->IsFlagSet( FCVAR_PROTECTED ) )
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs(
			"Admin %s changed cvar value of %s.\n",
			pPlayer ? pPlayer->GetPlayerName() : "Console",
			cvarName
		) );
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs(
			"Admin %s changed cvar value %s to %s.\n",
			pPlayer ? pPlayer->GetPlayerName() : "Console",
			cvarName,
			newValue.Get()
		) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: SLap player (+ damage)
//-----------------------------------------------------------------------------
static void SlapPlayerCommand( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	if ( !pPlayer && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "f" ) )
	{
		AdminReply( replySource, pPlayer, "You do not have access to this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pPlayer, "Usage: sa slap <name|#userid> [damage]" );
		return;
	}

	const char *partialName = args.Arg( 2 );
	int slapDamage = ( args.ArgC() >= 4 ) ? Q_max( atoi( args.Arg( 3 ) ), 0 ) : 0;

	CUtlVector<CBasePlayer *> targetPlayers;
	CBasePlayer *pTarget = NULL;

	if ( !ParsePlayerTargets( pPlayer, replySource, partialName, targetPlayers, pTarget, false ) )
		return;

	Vector slapForce( RandomFloat( -150, 150 ), RandomFloat( -150, 150 ), RandomFloat( 200, 400 ) );

	auto EmitSlapSound = []( CBasePlayer *pVictim )
		{
			int soundChoice = random->RandomInt( 1, 2 );
			const char *sound = ( soundChoice == 1 ) ? "Player.FallDamage" : "Player.SonicDamage";

			CPASAttenuationFilter filter( pVictim );
			filter.AddRecipient( pVictim );
			filter.MakeReliable();

			CBaseEntity::EmitSound( filter, pVictim->entindex(), sound );
		};

	CUtlString logDetails, chatMessage;

	if ( pTarget )
	{
		if ( !pTarget->IsAlive() )
		{
			AdminReply( replySource, pPlayer, "This player is already dead." );
			return;
		}

		pTarget->ApplyAbsVelocityImpulse( slapForce );

		if ( slapDamage > 0 )
		{
			CTakeDamageInfo dmg( pTarget, pTarget, slapDamage, DMG_FALL );
			pTarget->TakeDamage( dmg );

			char currentDamage[ 64 ];
			Q_snprintf( currentDamage, sizeof( currentDamage ), "(Damage: %d)", slapDamage );
			logDetails.Format( "%s", currentDamage );
		}

		EmitSlapSound( pTarget );

		chatMessage.Format( "%s slapped %s",
			pPlayer ? pPlayer->GetPlayerName() : "Console",
			pTarget->GetPlayerName() );

		if ( slapDamage > 0 )
		{
			chatMessage.Append( UTIL_VarArgs( " (Damage: %d)", slapDamage ) );
		}

		CHL2MP_Admin::LogAction( pPlayer, pTarget, "slapped", logDetails.Get() );
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.\n", chatMessage.Get() ) );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			if ( slapDamage > 0 )
			{
				Msg( "Slapped %s (Damage: %d)\n", pTarget->GetPlayerName(), slapDamage );
			}
			else
			{
				Msg( "Slapped %s\n", pTarget->GetPlayerName() );
			}
		}
	}
	else
	{
		int aliveCount = 0;
		for ( int i = 0; i < targetPlayers.Count(); i++ )
		{
			if ( targetPlayers[ i ]->IsAlive() )
				aliveCount++;
		}

		if ( aliveCount == 0 )
		{
			AdminReply( replySource, pPlayer, "All players in the target group are dead." );
			return;
		}

		for ( int i = 0; i < targetPlayers.Count(); i++ )
		{
			if ( targetPlayers[ i ]->IsAlive() )
			{
				targetPlayers[ i ]->ApplyAbsVelocityImpulse( slapForce );
				if ( slapDamage > 0 )
				{
					CTakeDamageInfo dmg( targetPlayers[ i ], targetPlayers[ i ], slapDamage, DMG_FALL );
					targetPlayers[ i ]->TakeDamage( dmg );
				}
				EmitSlapSound( targetPlayers[ i ] );
			}
		}

		BuildGroupTargetMessage( partialName, pPlayer, "slapped", NULL, logDetails, chatMessage, false, NULL );

		if ( slapDamage > 0 )
		{
			logDetails.Append( UTIL_VarArgs( " (Damage: %d)", slapDamage ) );
			chatMessage.Append( UTIL_VarArgs( " (Damage: %d)", slapDamage ) );
		}

		CHL2MP_Admin::LogAction( pPlayer, NULL, "slapped", logDetails.Get(), partialName + 1 );
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.\n", chatMessage.Get() ) );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			if ( targetPlayers.Count() > 1 )
			{
				Msg( "Slapped %d players.\n", aliveCount );
			}
			else if ( targetPlayers.Count() == 1 )
			{
				if ( slapDamage > 0 )
				{
					Msg( "Slapped %s (Damage: %d)\n", targetPlayers[ 0 ]->GetPlayerName(), slapDamage );
				}
				else
				{
					Msg( "Slapped %s\n", targetPlayers[ 0 ]->GetPlayerName() );
				}
			}
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Slay player
//-----------------------------------------------------------------------------
static void SlayPlayerCommand( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	if ( !pPlayer && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "f" ) )  // 'f' is the permission flag for slay
	{
		AdminReply( replySource, pPlayer, "You do not have access to this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pPlayer, "Usage: sa slay <name|#userid> [reason]" );
		return;
	}

	const char *partialName = args.Arg( 2 );

	CUtlString reason;
	for ( int i = 3; i < args.ArgC(); ++i )
	{
		reason.Append( args.Arg( i ) );
		if ( i < args.ArgC() - 1 )
		{
			reason.Append( " " );
		}
	}
	const char *slayReason = reason.Length() > 0 ? reason.Get() : "No reason provided";

	CUtlVector<CBasePlayer *> targetPlayers;
	CBasePlayer *pTarget = NULL;

	if ( !ParsePlayerTargets( pPlayer, replySource, partialName, targetPlayers, pTarget, false ) )
	{
		return;
	}

	int aliveCount = 0;
	for ( int i = 0; i < targetPlayers.Count(); i++ )
	{
		if ( targetPlayers[ i ]->IsAlive() )
		{
			aliveCount++;
		}
	}

	if ( pTarget && !pTarget->IsAlive() )
	{
		AdminReply( replySource, pPlayer, "This player is already dead." );
		return;
	}
	else if ( targetPlayers.Count() > 0 && aliveCount == 0 )
	{
		AdminReply( replySource, pPlayer, "All players in the target group are dead." );
		return;
	}

	auto ExecuteSlay = []( CBasePlayer *pVictim )
		{
			if ( pVictim->IsAlive() )
			{
				pVictim->CommitSuicide();
			}
		};

	CUtlString logDetails, chatMessage;

	if ( pTarget )
	{
		ExecuteSlay( pTarget );

		logDetails.Format( "(Reason: %s)", slayReason );
		CHL2MP_Admin::LogAction( pPlayer, pTarget, "slew", logDetails.Get() );

		PrintActionMessage( pPlayer, replySource, "slew", pTarget->GetPlayerName(), NULL, slayReason );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "Slew player %s (Reason: %s)\n", pTarget->GetPlayerName(), slayReason );
		}
	}
	else
	{
		int slainCount = 0;
		for ( int i = 0; i < targetPlayers.Count(); ++i )
		{
			if ( targetPlayers[ i ]->IsAlive() )
			{
				ExecuteSlay( targetPlayers[ i ] );
				slainCount++;
			}
		}

		BuildGroupTargetMessage( partialName, pPlayer, "slew", NULL, logDetails, chatMessage, true, slayReason );

		CHL2MP_Admin::LogAction( pPlayer, NULL, "slew", logDetails.Get(), partialName + 1 );
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.\n", chatMessage.Get() ) );

		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "Slew %d player%s (Reason: %s)\n",
				slainCount, slainCount == 1 ? "" : "s", slayReason );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Kick player
//-----------------------------------------------------------------------------
static void KickPlayerCommand( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	if ( !pPlayer && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "c" ) )  // 'c' is the permission flag for kick
	{
		AdminReply( replySource, pPlayer, "You do not have access to this command." );
		return;
	}

	if ( args.ArgC() < 3 )
	{
		AdminReply( replySource, pPlayer, "Usage: sa kick <name|#userid> [reason]" );
		return;
	}

	const char *partialName = args.Arg( 2 );

	CUtlString reason;
	for ( int i = 3; i < args.ArgC(); i++ )
	{
		reason.Append( args.Arg( i ) );
		if ( i < args.ArgC() - 1 )
		{
			reason.Append( " " );
		}
	}
	const char *kickReason = reason.Length() > 0 ? reason.Get() : "No reason provided";

	CUtlVector<CBasePlayer *> targetPlayers;
	CBasePlayer *pTarget = NULL;

	if ( !ParsePlayerTargets( pPlayer, replySource, partialName, targetPlayers, pTarget, false ) )
	{
		return;
	}

	auto ExecuteKick = [ & ]( CBasePlayer *pTarget )
		{
			if ( reason.Length() > 0 )
			{
				engine->ServerCommand( UTIL_VarArgs( "kickid %d %s\n", pTarget->GetUserID(), reason.Get() ) );
			}
			else
			{
				engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pTarget->GetUserID() ) );
			}
		};

	CUtlString logDetails, chatMessage;

	if ( pTarget )
	{
		const char *targetSteamID = engine->GetPlayerNetworkIDString( pTarget->edict() );

		// Root admin immunity check 
		// can't have random admins kick root admins
		CHL2MP_Admin *pAdmin = CHL2MP_Admin::GetAdmin( targetSteamID );
		if ( pAdmin && pAdmin->HasPermission( ADMIN_ROOT ) && ( pTarget != pPlayer ) && replySource != ADMIN_REPLY_SERVER_CONSOLE )
		{
			AdminReply( replySource, pPlayer, "Cannot target this player (root admin privileges)." );
			return;
		}

		ExecuteKick( pTarget );

		logDetails.Format( "Reason: %s", kickReason );
		CHL2MP_Admin::LogAction( pPlayer, pTarget, "kicked", logDetails.Get() );

		PrintActionMessage( pPlayer, replySource, "kicked", pTarget->GetPlayerName(), NULL, kickReason );
	}
	else
	{
		for ( int i = 0; i < targetPlayers.Count(); i++ )
		{
			ExecuteKick( targetPlayers[ i ] );
		}

		BuildGroupTargetMessage( partialName, pPlayer, "kicked", NULL, logDetails, chatMessage, true, kickReason );

		CHL2MP_Admin::LogAction( pPlayer, NULL, "kicked", logDetails.Get(), partialName + 1 );

		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.\n", chatMessage.Get() ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: bans a player
//-----------------------------------------------------------------------------
bool IsStringDigitsOnly( const char *str )
{
	// We want to make sure only digits are used
	for ( const char *p = str; *p; ++p )
	{
		if ( !isdigit( *p ) )
			return false;
	}
	return true;
}

static void BanPlayerCommand( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	if ( !pPlayer && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "d" ) )  // 'd' flag for ban
	{
		AdminReply( replySource, pPlayer, "You do not have access to this command." );
		return;
	}

	if ( args.ArgC() < 4 )
	{
		AdminReply( replySource, pPlayer, "Usage: sa ban <name|#userid> <time> [reason]" );
		return;
	}

	const char *partialName = args.Arg( 2 );
	const char *timeArg = args.Arg( 3 );

	if ( !IsStringDigitsOnly( timeArg ) )
	{
		AdminReply( replySource, pPlayer, "Invalid ban time provided." );
		return;
	}

	int banTime = atoi( timeArg );
	if ( banTime < 0 )
	{
		AdminReply( replySource, pPlayer, "Invalid ban time provided." );
		return;
	}

	CUtlString reason;
	for ( int i = 4; i < args.ArgC(); i++ )
	{
		reason.Append( args.Arg( i ) );
		if ( i < args.ArgC() - 1 )
		{
			reason.Append( " " );
		}
	}
	const char *kickReason = reason.Length() > 0 ? reason.Get() : "No reason provided";

	CUtlVector<CBasePlayer *> targetPlayers;
	CBasePlayer *pTarget = NULL;

	if ( !ParsePlayerTargets( pPlayer, replySource, partialName, targetPlayers, pTarget ) )
	{
		return;
	}

	// Default ban messages if none is used
	const static char defaultBanMsg[] = "You have been banned from this server";
	const static char defaultPermaBanMsg[] = "You have been permanently banned from this server";

	CUtlString banDuration = ( banTime == 0 ) ? "permanently" : UTIL_VarArgs( "%d minute%s", banTime, banTime == 1 ? "" : "s" );

	auto ExecuteBan = [ & ]( CBasePlayer *pTarget )
		{
			const char *steamID = engine->GetPlayerNetworkIDString( pTarget->edict() );
			const char *finalReason = reason.Length() > 0 ? reason.Get() : ( banTime == 0 ? defaultPermaBanMsg : defaultBanMsg );

			if ( banTime == 0 )
			{
				engine->ServerCommand( UTIL_VarArgs( "banid 0 %s; kickid %d %s\n", steamID, pTarget->GetUserID(), finalReason ) );
				engine->ServerCommand( "writeid\n" );
			}
			else
			{
				engine->ServerCommand( UTIL_VarArgs( "banid %d %s; kickid %d %s\n", banTime, steamID, pTarget->GetUserID(), finalReason ) );
			}
		};

	CUtlString logDetails, chatMessage;

	if ( pTarget )
	{
		CHL2MP_Admin *pAdmin = CHL2MP_Admin::GetAdmin( engine->GetPlayerNetworkIDString( pTarget->edict() ) );
		if ( pAdmin && pAdmin->HasPermission( ADMIN_ROOT ) && ( pTarget != pPlayer ) && replySource != ADMIN_REPLY_SERVER_CONSOLE )
		{
			AdminReply( replySource, pPlayer, "Cannot target this player (root admin privileges)." );
			return;
		}

		ExecuteBan( pTarget );

		logDetails.Format( "%s (Reason: %s)", banDuration.Get(), kickReason );
		CHL2MP_Admin::LogAction( pPlayer, pTarget, "banned", logDetails.Get() );

		PrintActionMessage( pPlayer, replySource, "banned", pTarget->GetPlayerName(), banDuration.Get(), kickReason );
	}
	else
	{
		for ( int i = 0; i < targetPlayers.Count(); i++ )
		{
			ExecuteBan( targetPlayers[ i ] );
		}

		BuildGroupTargetMessage( partialName, pPlayer, "banned", banDuration.Get(), logDetails, chatMessage, true, kickReason );

		CHL2MP_Admin::LogAction( pPlayer, NULL, "banned", logDetails.Get(), partialName + 1 );
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.\n", chatMessage.Get() ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds a SteamID3 to the banned list
//-----------------------------------------------------------------------------
static void AddBanCommand( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	if ( !pPlayer && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	// We are making this Rcon only because this prevents random admins from abusing this command and 
	// simply adding a random SteamID into the ban list just because they feel like it. Despite the chat 
	// print when a SteamID is added to the ban list, they could add a ban when nobody is connected, 
	// and even then, it is not always obvious when a SteamID was banned without checking the logs, 
	// hence why it is severely restricted to one of the highest permission levels.

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "m" ) )  // "m" flag for addban (RCON only)
	{
		AdminReply( replySource, pPlayer, "You do not have access to this command." );
		return;
	}

	if ( args.ArgC() < 4 )
	{
		AdminReply( replySource, pPlayer, "Usage: sa addban <time> <SteamID3> [reason]" );
		return;
	}

	int banTime = atoi( args.Arg( 2 ) );
	if ( banTime < 0 )
	{
		AdminReply( replySource, pPlayer, "Invalid ban time provided." );
		return;
	}

	// The SteamID format requires us to break it down into multiple arguments because of colons, 
	// which is why there are many arguments below. Admittedly, we could reduce the number of 
	// arguments by placing the colons directly since they are static, but this works anyway.

	// Reassemble SteamID3 from arguments
	char steamID[ 64 ] = { 0 };
	Q_snprintf( steamID, sizeof( steamID ), "%s%s%s%s%s", args.Arg( 3 ), args.Arg( 4 ), args.Arg( 5 ), args.Arg( 6 ), args.Arg( 7 ) );

	// Validate SteamID format
	if ( Q_strncmp( steamID, "[U:1:", 5 ) != 0 || Q_strlen( steamID ) < 7 )
	{
		AdminReply( replySource, pPlayer, "Invalid SteamID format. SteamID must start with [U:1: and be correctly formatted." );
		return;
	}

	const char *idPart = Q_strstr( steamID, ":" ) + 3;
	const char *closingBracket = Q_strstr( steamID, "]" );

	if ( !closingBracket || idPart >= closingBracket )
	{
		AdminReply( replySource, pPlayer, "Invalid SteamID format. Missing closing bracket." );
		return;
	}

	for ( const char *c = idPart; c < closingBracket; ++c )
	{
		if ( !isdigit( *c ) )
		{
			AdminReply( replySource, pPlayer, "Invalid SteamID format. The numeric portion must contain only numbers." );
			return;
		}
	}

	if ( IsSteamIDAdmin( steamID ) )
	{
		AdminReply( replySource, pPlayer, "This player is an admin and cannot be banned." );
		return;
	}

	// Check if banned_user.cfg exists; if not, create it
	if ( !filesystem->FileExists( "cfg/banned_user.cfg", "MOD" ) )
	{
		FileHandle_t createFile = filesystem->Open( "cfg/banned_user.cfg", "w", "MOD" );
		if ( createFile )
		{
			filesystem->Close( createFile );
		}
		else
		{
			AdminReply( replySource, pPlayer, "Failed to create 'cfg/banned_user.cfg'. Check file permissions." );
			return;
		}
	}

	// Checking if the SteamID is already banned
	FileHandle_t file = filesystem->Open( "cfg/banned_user.cfg", "r", "MOD" );
	if ( file )
	{
		char buffer[ 1024 ];
		while ( filesystem->ReadLine( buffer, sizeof( buffer ), file ) )
		{
			if ( Q_stristr( buffer, steamID ) )
			{
				AdminReply( replySource, pPlayer, "SteamID is already banned." );
				filesystem->Close( file );
				return;
			}
		}
		filesystem->Close( file );
	}
	else
	{
		AdminReply( replySource, pPlayer, "Failed to read the ban list." );
		return;
	}

	// It has little purpose too. Main purpose is for logging.
	CUtlString reason;
	for ( int i = 8; i < args.ArgC(); i++ )
	{
		reason.Append( args.Arg( i ) );
		if ( i < args.ArgC() - 1 )
		{
			reason.Append( " " );
		}
	}

	// This too has little purpose. If we use addban, it is probably to permanently ban an ID. 
	// If we wanted to temporarily ban an ID with ban or addban, we would likely use a database 
	// of some sort, like MySQL to avoid just storing bans in memory and running into issues 
	// with it long term. Best to use 0 if this command is ever used.
	if ( banTime == 0 )
	{
		engine->ServerCommand( UTIL_VarArgs( "banid 0 %s\n", steamID ) );
	}
	else
	{
		engine->ServerCommand( UTIL_VarArgs( "banid %d %s\n", banTime, steamID ) );
	}
	engine->ServerCommand( "writeid\n" );

	CUtlString banDuration = ( banTime == 0 ) ? "permanently" : UTIL_VarArgs( "for %d minute%s", banTime, banTime > 1 ? "s" : "" );
	CUtlString logDetails = UTIL_VarArgs( "SteamID %s %s", steamID, banDuration.Get() );

	if ( reason.Length() > 0 )
	{
		logDetails.Append( UTIL_VarArgs( " (Reason: %s)", reason.Get() ) );
	}

	CHL2MP_Admin::LogAction( pPlayer, NULL, "added ban", logDetails.Get() );

	CUtlString message;
	if ( banTime == 0 )
	{
		message.Format( "SteamID %s permanently banned by %s", steamID, pPlayer ? pPlayer->GetPlayerName() : "Console" );
	}
	else
	{
		message.Format( "SteamID %s banned by %s for %d minute%s",
			steamID, pPlayer ? pPlayer->GetPlayerName() : "Console", banTime, banTime > 1 ? "s" : "" );
	}

	if ( reason.Length() > 0 )
	{
		message.Append( UTIL_VarArgs( ". Reason: %s", reason.Get() ) );
	}

	UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s.\n", message.Get() ) );
}

//-----------------------------------------------------------------------------
// Purpose: removes a ban from the banned user list
//-----------------------------------------------------------------------------
static void UnbanPlayerCommand( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	if ( !pPlayer && replySource != ADMIN_REPLY_SERVER_CONSOLE )
	{
		Msg( "Command must be issued by a player or the server console.\n" );
		return;
	}

	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "e" ) )  // "e" flag for unban permission
	{
		AdminReply( replySource, pPlayer, "You do not have access to this command." );
		return;
	}

	if ( args.ArgC() < 7 )
	{
		AdminReply( replySource, pPlayer, "Usage: sa unban <SteamID3>" );
		return;
	}

	// Same thing as addban, we need to reconstruct the SteamID
	char steamID[ 64 ];
	Q_snprintf( steamID, sizeof( steamID ), "%s%s%s%s%s", args.Arg( 2 ), args.Arg( 3 ), args.Arg( 4 ), args.Arg( 5 ), args.Arg( 6 ) );

	if ( Q_strncmp( steamID, "[U:1:", 5 ) != 0 || Q_strlen( steamID ) < 7 )
	{
		AdminReply( replySource, pPlayer, "Invalid SteamID format. SteamID must start with [U:1: and be properly formatted." );
		return;
	}

	const char *idPart = Q_strstr( steamID, ":" ) + 3;  // Skip "[U:1:"
	const char *closingBracket = Q_strstr( steamID, "]" );

	if ( !closingBracket || idPart >= closingBracket )
	{
		AdminReply( replySource, pPlayer, "Invalid SteamID format. Missing closing bracket." );
		return;
	}

	for ( const char *c = idPart; c < closingBracket; ++c )
	{
		if ( !isdigit( *c ) )
		{
			AdminReply( replySource, pPlayer, "Invalid SteamID format. The numeric portion must only contain digits." );
			return;
		}
	}

	// Check if SteamID is in the ban list
	FileHandle_t file = filesystem->Open( "cfg/banned_user.cfg", "r", "MOD" );
	if ( !file )
	{
		AdminReply( replySource, pPlayer, "Failed to read the ban list. Check that the file exists and is placed the cfg folder." );
		return;
	}

	const int bufferSize = 1024;
	char buffer[ bufferSize ];
	bool steamIDFound = false;

	while ( filesystem->ReadLine( buffer, bufferSize, file ) )
	{
		if ( Q_stristr( buffer, steamID ) )
		{
			steamIDFound = true;
			break;
		}
	}

	filesystem->Close( file );

	if ( steamIDFound )
	{
		engine->ServerCommand( UTIL_VarArgs( "removeid %s\n", steamID ) );
		engine->ServerCommand( "writeid\n" );

		CUtlString logDetails;
		logDetails.Format( "SteamID %s", steamID );
		CHL2MP_Admin::LogAction( pPlayer, NULL, "unbanned", logDetails.Get() );

		CUtlString message;
		message.Format( "SteamID %s was unbanned by %s.", steamID, pPlayer ? pPlayer->GetPlayerName() : "Console" );

		UTIL_ClientPrintAll( HUD_PRINTTALK, message.Get() );
	}
	else
	{
		AdminReply( replySource, pPlayer, UTIL_VarArgs( "SteamID %s was not found in the ban list.", steamID ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Show all admin commands via "sa" main command
//-----------------------------------------------------------------------------
typedef void ( *AdminCommandFunction )( const CCommand &args );

struct CommandEntry
{
	const char *name;
	AdminCommandFunction function;
};

static const CommandEntry g_AdminCommands[] = {
	{ "say", AdminSay },
	{ "csay", AdminCSay },
	{ "chat", AdminChat },
	{ "psay", AdminPSay },
	{ "kick", KickPlayerCommand },
	{ "ban", BanPlayerCommand },
	{ "addban", AddBanCommand },
	{ "unban", UnbanPlayerCommand },
	{ "slay", SlayPlayerCommand },
	{ "slap", SlapPlayerCommand },
	{ "gag", GagPlayerCommand },
	{ "ungag", UnGagPlayerCommand },
	{ "mute", MutePlayerCommand },
	{ "unmute", UnMutePlayerCommand },
	{ "team", TeamPlayerCommand },
	{ "bring", BringPlayerCommand },
	{ "goto", GotoPlayerCommand },
	{ "map", MapCommand },
	{ "noclip", NoClipPlayerCommand },
	{ "cvar", CVarCommand },
	{ "exec", ExecFileCommand },
	{ "rcon", RconCommand },
	{ "reloadadmins", ReloadAdminsCommand },
	{ "help", HelpPlayerCommand },
	{ "version", VersionCommand }
};

static AdminCommandFunction FindAdminCommand( const char *cmd )
{
	for ( const auto &entry : g_AdminCommands )
	{
		if ( Q_stricmp( entry.name, cmd ) == 0 )
		{
			return entry.function;
		}
	}
	return NULL;
}

static void AdminCommand( const CCommand &args )
{
	if ( !g_bAdminSystem && engine->IsDedicatedServer() )
	{
		if ( UTIL_IsCommandIssuedByServerAdmin() )
		{
			Msg( "Admin system disabled by the -noadmin launch command\nRemove launch command and restart the server\n" );
		}
		else if ( CBasePlayer *pPlayer = UTIL_GetCommandClient() )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "Admin system disabled by the -noadmin launch command\n" );
		}
		return;
	}

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	AdminReplySource replySource = GetCmdReplySource( pPlayer );

	// Handle "sa" with no arguments (print help menu)
	if ( args.ArgC() < 2 )
	{
		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			PrintAdminHelp( NULL, true );
		}
		else if ( pPlayer )
		{
			PrintAdminHelp( pPlayer );
		}
		return;
	}

	// Extract the subcommand
	const char *subCommand = args.Arg( 1 );

	// Version is a public command i.e. no permissions required
	if ( Q_stricmp( subCommand, "version" ) == 0 )
	{
		VersionCommand( args );
		return;
	}

	AdminCommandFunction commandFunc = FindAdminCommand( subCommand );
	if ( !commandFunc )
	{
		if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
		{
			Msg( "[Server Admin] Unknown command: %s\n", subCommand );
			PrintAdminHelp( NULL, true );
		}
		else if ( pPlayer )
		{
			AdminReply( replySource, pPlayer, "[Server Admin] Unknown command: %s", subCommand );
			PrintAdminHelp( pPlayer );
		}
		return;
	}

	// Server console can run anything without permission checks
	if ( replySource == ADMIN_REPLY_SERVER_CONSOLE )
	{
		commandFunc( args );
		return;
	}

	// For in-game players, ensure they at least have the "b" flag
	if ( pPlayer && !CHL2MP_Admin::IsPlayerAdmin( pPlayer, "b" ) )
	{
		AdminReply( replySource, pPlayer, "You do not have access to this command." );
		return;
	}

	commandFunc( args );
}
ConCommand sa( "sa", AdminCommand, "Admin menu.", FCVAR_SERVER_CAN_EXECUTE | FCVAR_CLIENTCMD_CAN_EXECUTE );

//-----------------------------------------------------------------------------
// Purpose: Initialize the admin system (parse the file, add admins, register commands)
//-----------------------------------------------------------------------------
void CHL2MP_Admin::InitAdminSystem()
{
	if ( CommandLine()->CheckParm( "-noadmin" ) )
		return;

	g_bAdminSystem = true;

	CHL2MP_Admin::ClearAllAdmins();
	InitializeSpecialTargets();

	new CHL2MP_Admin();

	if ( !filesystem->IsDirectory( "cfg/admin/logs", "GAME" ) )
	{
		filesystem->CreateDirHierarchy( "cfg/admin/logs", "GAME" );
	}

	CUtlMap<CUtlString, AdminData_t> newAdminMap( DefLessFunc( CUtlString ) );

	if ( !ParseAdminFile( "cfg/admin/admins.txt", newAdminMap ) )
	{
		Warning( "Error: Unable to load admins.txt\nDoes the file exist and is placed in the right location?\n" );
		return;
	}

	// Populate g_AdminList and g_AdminMap directly from parsed data
	g_AdminMap.RemoveAll();

	for ( int i = newAdminMap.FirstInorder(); i != newAdminMap.InvalidIndex(); i = newAdminMap.NextInorder( i ) )
	{
		const CUtlString &steamID = newAdminMap.Key( i );
		AdminData_t &adminData = newAdminMap.Element( i );

		CHL2MP_Admin *newAdmin = new CHL2MP_Admin();
		newAdmin->Initialize( steamID.Get(), adminData.flags.Get() );
		g_AdminList.AddToTail( newAdmin );

		g_AdminMap.Insert( steamID, adminData );
	}

	DevMsg( "Admin list loaded from admins.txt.\n" );

	// Initialize log file
	char date[ 9 ];
	time_t now = time( 0 );
	strftime( date, sizeof( date ), "%Y%m%d", localtime( &now ) );

	char logFileName[ 256 ];
	Q_snprintf( logFileName, sizeof( logFileName ), "cfg/admin/logs/ADMINLOG_%s.txt", date );

	g_AdminLogFile = filesystem->Open( logFileName, "a+", "GAME" );
	if ( !g_AdminLogFile )
	{
		Warning( "Unable to create admin log file, but it will be created on first admin command usage.\n" );
		g_bAdminSystem = false;
	}
	else
	{
		Msg( "Admin log initialized: %s\n", logFileName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks chat for certain strings (chat commands)
//-----------------------------------------------------------------------------
struct ChatCommandEntry
{
	const char *chatCommand;
	const char *consoleCommand;
	bool requiresArguments;
	const char *consoleMessage;
};

static const ChatCommandEntry g_ChatCommands[] = {
	{ "!say", "sa say", true, NULL },
	{ "/say", "sa say", true, NULL },
	{ "!csay", "sa csay", true, NULL },
	{ "/csay", "sa csay", true, NULL },
	{ "!psay", "sa psay", true, NULL },
	{ "/psay", "sa psay", true, NULL },
	{ "!chat", "sa chat", true, NULL },
	{ "/chat", "sa chat", true, NULL },
	{ "!ban", "sa ban", true, NULL },
	{ "/ban", "sa ban", true, NULL },
	{ "!kick", "sa kick", true, NULL },
	{ "/kick", "sa kick", true, NULL },
	{ "!addban", "sa addban", true, NULL },
	{ "/addban", "sa addban", true, NULL },
	{ "!unban", "sa unban", true, NULL },
	{ "/unban", "sa unban", true, NULL },
	{ "!slay", "sa slay", true, NULL },
	{ "/slay", "sa slay", true, NULL },
	{ "!slap", "sa slap", true, NULL },
	{ "/slap", "sa slap", true, NULL },
	{ "!cvar", "sa cvar", true, NULL },
	{ "/cvar", "sa cvar", true, NULL },
	{ "!rcon", "sa rcon", true, NULL },
	{ "/rcon", "sa rcon", true, NULL },
	{ "!map", "sa map", true, NULL },
	{ "/map", "sa map", true, NULL },
	{ "!gag", "sa gag", true, NULL },
	{ "/gag", "sa gag", true, NULL },
	{ "!ungag", "sa ungag", true, NULL },
	{ "/ungag", "sa ungag", true, NULL },
	{ "!mute", "sa mute", true, NULL },
	{ "/mute", "sa mute", true, NULL },
	{ "!unmute", "sa unmute", true, NULL },
	{ "/unmute", "sa unmute", true, NULL },
	{ "!team", "sa team", true, NULL },
	{ "/team", "sa team", true, NULL },
	{ "!bring", "sa bring", true, NULL },
	{ "/bring", "sa bring", true, NULL },
	{ "!goto", "sa goto", true, NULL },
	{ "/goto", "sa goto", true, NULL },
	{ "!noclip", "sa noclip", true, NULL },
	{ "/noclip", "sa noclip", true, NULL },
	{ "!exec", "sa exec", true, NULL },
	{ "/exec", "sa exec", true, NULL },
	{ "!reloadadmins", "sa reloadadmins", false, NULL },
	{ "/reloadadmins", "sa reloadadmins", false, NULL },
	{ "!help", "sa help", false, "Check your console for output.\n" },
	{ "/help", "sa help", false, "Check your console for output.\n" },
	{ "!version", "sa version", false, "Check your console for output.\n" },
	{ "/version", "sa version", false, "Check your console for output.\n" },
	{ "!sa", "sa", false, "Check your console for output.\n" },
	{ "/sa", "sa", false, "Check your console for output.\n" }
};

void CHL2MP_Admin::CheckChatText( char *p, int bufsize )
{
	if ( !g_bAdminSystem )
		return;

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !p || bufsize <= 0 )
		return;

	if ( p[ 0 ] != '!' && p[ 0 ] != '/' )
		return;

	for ( size_t i = 0; i < ARRAYSIZE( g_ChatCommands ); ++i )
	{
		const ChatCommandEntry &cmd = g_ChatCommands[ i ];

		size_t cmdLen = strlen( cmd.chatCommand );
		if ( Q_strncmp( p, cmd.chatCommand, cmdLen ) == 0 )
		{
			char consoleCmd[ 256 ];

			if ( cmd.requiresArguments )
			{
				const char *args = p + cmdLen;
				Q_snprintf( consoleCmd, sizeof( consoleCmd ), "%s%s", cmd.consoleCommand, args );
			}
			else
			{
				Q_snprintf( consoleCmd, sizeof( consoleCmd ), "%s", cmd.consoleCommand );
			}

			if ( pPlayer )
			{
				pPlayer->SetLastCommandWasFromChat( true );
				engine->ClientCommand( pPlayer->edict(), consoleCmd );
				if ( cmd.consoleMessage )
				{
					ClientPrint( pPlayer, HUD_PRINTTALK, cmd.consoleMessage );
				}
			}
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Action log
//-----------------------------------------------------------------------------
void CHL2MP_Admin::LogAction( CBasePlayer *pAdmin, CBasePlayer *pTarget, const char *action, const char *details, const char *groupTarget )
{
	if ( g_AdminLogFile == FILESYSTEM_INVALID_HANDLE )
	{
		// Try to reopen the log file if it's missing.
		char date[ 9 ];
		time_t now = time( 0 );
		strftime( date, sizeof( date ), "%Y%m%d", localtime( &now ) );

		char logFileName[ 256 ];
		Q_snprintf( logFileName, sizeof( logFileName ), "cfg/admin/logs/ADMINLOG_%s.txt", date );

		g_AdminLogFile = filesystem->Open( logFileName, "a+", "GAME" );

		if ( g_AdminLogFile == FILESYSTEM_INVALID_HANDLE )
		{
			Warning( "Failed to open admin log file: %s\n", logFileName );
			return;
		}
	}

	time_t now = time( 0 );
	struct tm *localTime = localtime( &now );
	char dateString[ 11 ];
	char timeString[ 9 ];
	strftime( dateString, sizeof( dateString ), "%Y/%m/%d", localTime );
	strftime( timeString, sizeof( timeString ), "%H:%M:%S", localTime );

	const char *mapName = STRING( gpGlobals->mapname );
	const char *adminName = pAdmin ? pAdmin->GetPlayerName() : "Console";
	const char *adminSteamID = pAdmin ? engine->GetPlayerNetworkIDString( pAdmin->edict() ) : "Console";
	const char *targetName = pTarget ? pTarget->GetPlayerName() : "";
	const char *targetSteamID = pTarget ? engine->GetPlayerNetworkIDString( pTarget->edict() ) : "";

	CUtlString logEntry;

	if ( pTarget )
	{
		if ( Q_strlen( details ) > 0 )
		{
			logEntry.Format( "[%s] %s @ %s => Admin %s <%s> %s %s <%s> %s\n",
				mapName, dateString, timeString, adminName, adminSteamID,
				action, targetName, targetSteamID, details );
		}
		else
		{
			logEntry.Format( "[%s] %s @ %s => Admin %s <%s> %s %s <%s>\n",
				mapName, dateString, timeString, adminName, adminSteamID,
				action, targetName, targetSteamID );
		}
	}
	else if ( groupTarget )
	{
		if ( Q_strlen( details ) > 0 )
		{
			logEntry.Format( "[%s] %s @ %s => Admin %s <%s> %s %s\n",
				mapName, dateString, timeString, adminName, adminSteamID,
				action, details );
		}
		else
		{
			logEntry.Format( "[%s] %s @ %s => Admin %s <%s> %s\n",
				mapName, dateString, timeString, adminName, adminSteamID,
				action );
		}
	}
	else
	{
		if ( Q_strlen( details ) > 0 )
		{
			logEntry.Format( "[%s] %s @ %s => Admin %s <%s> %s %s\n",
				mapName, dateString, timeString, adminName, adminSteamID,
				action, details );
		}
		else
		{
			logEntry.Format( "[%s] %s @ %s => Admin %s <%s> %s\n",
				mapName, dateString, timeString, adminName, adminSteamID,
				action );
		}
	}

	filesystem->FPrintf( g_AdminLogFile, "%s", logEntry.Get() );
	filesystem->Flush( g_AdminLogFile );
}