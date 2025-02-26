//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Uploads KeyValue stats to the new SteamWorks gamestats system.
//
//=============================================================================

#include "cbase.h"
#include "cdll_int.h"
#include "tier2/tier2.h"
#include <time.h>

#ifdef	GAME_DLL
#include "gameinterface.h"
#elif	CLIENT_DLL
#include "c_playerresource.h"
#endif

#include "steam/isteamutils.h"

#include "steamworks_gamestats.h"
#include "achievementmgr.h"
#include "icommandline.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

static CSteamWorksGameStatsUploader g_SteamWorksGameStats;

//-----------------------------------------------------------------------------
// Purpose: Returns the time since the epoch
//-----------------------------------------------------------------------------
time_t CSteamWorksGameStatsUploader::GetTimeSinceEpoch( void )
{
	time_t aclock;
	time( &aclock );
	return aclock;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a reference to the global object
//-----------------------------------------------------------------------------
CSteamWorksGameStatsUploader& GetSteamWorksSGameStatsUploader()
{
	return g_SteamWorksGameStats;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor. Sets up the steam callbacks accordingly depending on client/server dll 
//-----------------------------------------------------------------------------
CSteamWorksGameStatsUploader::CSteamWorksGameStatsUploader() : CAutoGameSystemPerFrame( "CSteamWorksGameStatsUploader" )
#if !defined(NO_STEAM) && defined(GAME_DLL) 
, m_CallbackSteamSessionInfoIssued(this, &CSteamWorksGameStatsUploader::Steam_OnSteamSessionInfoIssued)
, m_CallbackSteamSessionInfoClosed(this, &CSteamWorksGameStatsUploader::Steam_OnSteamSessionInfoClosed)
#endif
{
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if Cvar diff tracking is enabled and if so uploads the diffs
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::UploadCvars()
{
}

//-----------------------------------------------------------------------------
// Purpose: Reset uploader state.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::Reset()
{
}

//-----------------------------------------------------------------------------
// Purpose: Init function from CAutoGameSystemPerFrame and must return true.
//-----------------------------------------------------------------------------
bool CSteamWorksGameStatsUploader::Init()
{

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Event handler for gathering basic info as well as ending sessions.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::FireGameEvent( IGameEvent *event )
{

}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:	Sets the server session ID but ONLY if it's not 0. We are using this to avoid a race 
// 			condition where a server sends their session stats before a client does, thereby,
//			resetting the client's server session ID to 0.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::SetServerSessionID( uint64 serverSessionID )
{
}

//-----------------------------------------------------------------------------
// Purpose:	Writes the disconnect time to the current server session entry.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::ClientDisconnect()
{


}

#endif

//-----------------------------------------------------------------------------
// Purpose: Called when the level shuts down.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::LevelShutdown()
{
}

//-----------------------------------------------------------------------------
// Purpose: Requests a session ID from steam.
//-----------------------------------------------------------------------------
EResult	CSteamWorksGameStatsUploader::RequestSessionID()
{
	return k_EResultOK;
}

//-----------------------------------------------------------------------------
// Purpose: Clears our session id and session id convar.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::ClearSessionID()
{
}

#ifndef	NO_STEAM

//-----------------------------------------------------------------------------
// Purpose: The steam callback used to get our session IDs.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::Steam_OnSteamSessionInfoIssued( GameStatsSessionIssued_t *pGameStatsSessionInfo )
{

}

//-----------------------------------------------------------------------------
// Purpose: The steam callback to notify us that we've submitted stats.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::Steam_OnSteamSessionInfoClosed( GameStatsSessionClosed_t *pGameStatsSessionInfo )
{
}

//-----------------------------------------------------------------------------
// Purpose: Per frame think. Used to periodically check if we have queued operations.
// For example: we may request a session id before steam is ready.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::FrameUpdatePostEntityThink()
{
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Opens a session: requests the session id, etc.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::StartSession()
{
}

//-----------------------------------------------------------------------------
// Purpose: Completes a session for the given type.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::EndSession()
{

}

//-----------------------------------------------------------------------------
// Purpose: Flush any unsent rows.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::FlushStats()
{
}

//-----------------------------------------------------------------------------
// Purpose: Uploads any end of session rows.
//-----------------------------------------------------------------------------
void CSteamWorksGameStatsUploader::WriteSessionRow()
{

}

//-----------------------------------------------------------------------------
// DATA ACCESS UTILITIES
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Verifies that we have a valid interface and will attempt to obtain a new one if we don't.
//-----------------------------------------------------------------------------
bool CSteamWorksGameStatsUploader::VerifyInterface( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper function to write an int32 to a table given the row name
//-----------------------------------------------------------------------------
EResult CSteamWorksGameStatsUploader::WriteIntToTable( const int value, uint64 iTableID, const char *pzRow )
{
	return k_EResultNoConnection;
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper function to write an int64 to a table given the row name
//-----------------------------------------------------------------------------
EResult CSteamWorksGameStatsUploader::WriteInt64ToTable( const uint64 value, uint64 iTableID, const char *pzRow )
{
	return k_EResultNoConnection;
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper function to write an float to a table given the row name
//-----------------------------------------------------------------------------
EResult CSteamWorksGameStatsUploader::WriteFloatToTable( const float value, uint64 iTableID, const char *pzRow )
{
	return k_EResultNoConnection;
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper function to write an string to a table given the row name
//-----------------------------------------------------------------------------
EResult CSteamWorksGameStatsUploader::WriteStringToTable( const char *value, uint64 iTableID, const char *pzRow )
{
	return k_EResultNoConnection;
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper function to search a KeyValues for a value with the given keyName and add the result to the 
// row. If the key isn't present, return ResultNoMatch to indicate such. 
//-----------------------------------------------------------------------------
EResult	CSteamWorksGameStatsUploader::WriteOptionalFloatToTable( KeyValues *pKV, const char* keyName, uint64 iTableID, const char *pzRow )
{
	return k_EResultNoConnection;
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper function to search a KeyValues for a value with the given keyName and add the result to the 
// row. If the key isn't present, return ResultNoMatch to indicate such. 
//-----------------------------------------------------------------------------
EResult	CSteamWorksGameStatsUploader::WriteOptionalIntToTable( KeyValues *pKV, const char* keyName, uint64 iTableID, const char *pzRow )
{
	return k_EResultNoConnection;
}



//-----------------------------------------------------------------------------
// STEAM ACCESS UTILITIES
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Determines if the system can connect to steam
//-----------------------------------------------------------------------------
bool CSteamWorksGameStatsUploader::AccessToSteamAPI( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: There's no guarantee that your interface pointer will persist across level transitions,
//			so this function will update your interface.
//-----------------------------------------------------------------------------
ISteamGameStats* CSteamWorksGameStatsUploader::GetInterface( void )
{

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Creates a table from the KeyValue file. Do NOT send nested KeyValue objects into this function!
//-----------------------------------------------------------------------------
EResult CSteamWorksGameStatsUploader::AddStatsForUpload( KeyValues *pKV, bool bSendImmediately )
{


	return k_EResultFail;
}

//-----------------------------------------------------------------------------
// Purpose: Parses all the keyvalue files we've been sent and creates tables from them and uploads them
//-----------------------------------------------------------------------------
double g_rowCommitTime = 0.0f;
double g_rowWriteTime = 0.0f;
EResult CSteamWorksGameStatsUploader::ParseKeyValuesAndSendStats( KeyValues *pKV, bool bIncludeClientsServerSessionID )
{
	return k_EResultFail;
}

#ifdef	CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Reports client's perf data at the end of a client session.
//---------------------------------`--------------------------------------------
void CSteamWorksGameStatsUploader::AddClientPerfData( KeyValues *pKV )
{

}
#endif

//-------------------------------------------------------------------------------------------------
/**
*	Purpose:	Calculates the number of humans in the game
*/
int CSteamWorksGameStatsUploader::GetHumanCountInGame()
{
	int iHumansInGame = 0;
	// TODO: Need to add server/client code to count the number of connected humans.
	return iHumansInGame;
}

#ifdef	CLIENT_DLL
//-------------------------------------------------------------------------------------------------
/**
*	Purpose:	Calculates the number of friends in the game
*/
int CSteamWorksGameStatsUploader::GetFriendCountInGame()
{

	return 0;
}
#endif

void CSteamWorksGameStatsUploader::ServerAddressToInt()
{
}

