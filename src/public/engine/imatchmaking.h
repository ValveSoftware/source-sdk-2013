//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef IMATCHMAKING_H
#define IMATCHMAKING_H
#ifdef _WIN32
#pragma once
#endif

#include "const.h"
#include "vgui/VGUI.h"

#if !defined( _X360 )
#include "xbox/xboxstubs.h"
#endif

class KeyValues;

enum SESSION_NOTIFY
{
	SESSION_NOTIFY_FAIL_SEARCH,
	SESSION_NOTIFY_SEARCH_COMPLETED,
	SESSION_NOFIFY_MODIFYING_SESSION,
	SESSION_NOTIFY_MODIFYING_COMPLETED_HOST,
	SESSION_NOTIFY_MODIFYING_COMPLETED_CLIENT,
	SESSION_NOTIFY_MIGRATION_COMPLETED,
	SESSION_NOTIFY_CONNECT_SESSIONFULL,
	SESSION_NOTIFY_CONNECT_NOTAVAILABLE,
	SESSION_NOTIFY_CONNECTED_TOSESSION,
	SESSION_NOTIFY_CONNECTED_TOSERVER,
	SESSION_NOTIFY_CONNECT_FAILED,
	SESSION_NOTIFY_FAIL_CREATE,
	SESSION_NOTIFY_FAIL_MIGRATE,
	SESSION_NOTIFY_REGISTER_COMPLETED,
	SESSION_NOTIFY_FAIL_REGISTER,
	SESSION_NOTIFY_CLIENT_KICKED,
	SESSION_NOTIFY_CREATED_HOST,
	SESSION_NOTIFY_CREATED_CLIENT,
	SESSION_NOTIFY_LOST_HOST,
	SESSION_NOTIFY_LOST_SERVER,
	SESSION_NOTIFY_COUNTDOWN,
	SESSION_NOTIFY_ENDGAME_RANKED,	// Ranked
	SESSION_NOTIFY_ENDGAME_HOST,	// Unranked
	SESSION_NOTIFY_ENDGAME_CLIENT,	// Unranked
	SESSION_NOTIFY_DUMPSTATS,		// debugging
	SESSION_NOTIFY_WELCOME,			// Close all dialogs and show the welcome main menu
};

enum SESSION_PROPS
{
	SESSION_CONTEXT,
	SESSION_PROPERTY,
	SESSION_FLAG,
};

struct hostData_s
{
	char hostName[MAX_PLAYER_NAME_LENGTH];
	char scenario[MAX_MAP_NAME];
	int  gameState;
	int	 gameTime;
	XUID xuid; 
};

struct MM_QOS_t
{
	int nPingMsMin;		// Minimum round-trip time in ms
	int nPingMsMed;		// Median round-trip time in ms
	float flBwUpKbs;	// Bandwidth upstream in kilobytes/s
	float flBwDnKbs;	// Bandwidth downstream in kilobytes/s
	float flLoss;		// Average packet loss in percents
};

#define NO_TIME_LIMIT	65000

abstract_class IMatchmaking
{
public:
	virtual void SessionNotification( const SESSION_NOTIFY notification, const int param = 0 ) = 0;
	virtual void AddSessionProperty( const uint nType, const char *pID, const char *pValue, const char *pValueType ) = 0;
	virtual void SetSessionProperties( KeyValues *pPropertyKeys ) = 0;
	virtual void SelectSession( uint idx ) = 0;
	virtual void ModifySession() = 0;
	virtual void UpdateMuteList() = 0;
	virtual void StartHost( bool bSystemLink = false ) = 0;
	virtual void StartClient( bool bSystemLink = false ) = 0;
	virtual bool StartGame() = 0;
	virtual bool CancelStartGame() = 0;
	virtual void ChangeTeam( const char *pTeamName ) = 0;
	virtual void TellClientsToConnect() = 0;
	virtual void CancelCurrentOperation() = 0;
	virtual void KickPlayerFromSession( uint64 id ) = 0;
	virtual void JoinInviteSession( XSESSION_INFO *pHostInfo ) = 0;
	virtual void JoinInviteSessionByID( XNKID nSessionID ) = 0;
	virtual void EndStatsReporting() = 0;

	// For Gameui
	virtual KeyValues *GetSessionProperties() = 0;

	// For voice chat
	virtual uint64	PlayerIdToXuid( int playerId ) = 0;
	virtual bool	IsPlayerMuted( int iUserId, XUID id ) = 0;

	// To determine host Quality-of-Service
	virtual MM_QOS_t GetQosWithLIVE() = 0;

	// Used by non-'host' local machines which are starting a map to "prime" the caches.  Will sit at near completion indefinitely -- 
	//  the client is waiting for a TellClientsToConnect message
	virtual bool	PreventFullServerStartup() = 0;
};

#define VENGINE_MATCHMAKING_VERSION "VENGINE_MATCHMAKING_VERSION001"

#endif // IMATCHMAKING_H
