//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Uploads gamestats via the SteamWorks API. 
//
//=============================================================================//

#ifndef STEAMWORKS_GAMESTATS_H
#define STEAMWORKS_GAMESTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "GameEventListener.h"
#include "steam/steam_api.h"
#include "utlvector.h"

#ifndef	_X360
#include "steam/isteamgamestats.h"
#endif

// Container to hold all the KeyValue stats to send only if the convar "steamworks_immediate_upload" is set to 0.
// Otherwise, the stats are uploaded as they are received.
typedef CUtlVector< KeyValues* > KeyValueStatList;

struct ClientServerSession_t
{
	uint64				m_ServerSessionID;
	RTime32				m_ConnectTime;
	RTime32				m_DisconnectTime;

	void Reset()
	{
		m_ServerSessionID = 0;
		m_ConnectTime = 0;
		m_DisconnectTime = 0;
	}
};

//used to drive most of the game stat event handlers as well as track basic stats under the hood of CBaseGameStats
class CSteamWorksGameStatsUploader : public CAutoGameSystemPerFrame, public CGameEventListener
{
public:

#ifdef	CLIENT_DLL
	// Called before rendering
	virtual void PreRender() {}

	// Gets called each frame
	virtual void Update( float frametime ) {}

	// Called after rendering
	virtual void PostRender() {}

	void AddClientPerfData( KeyValues *pKV );
	void SetServerSessionID( uint64 serverID );
	int	GetFriendCountInGame();

#ifndef	NO_STEAM
	STEAM_CALLBACK_MANUAL( CSteamWorksGameStatsUploader, Steam_OnSteamSessionInfoIssued, GameStatsSessionIssued_t, m_CallbackSteamSessionInfoIssued );
	STEAM_CALLBACK_MANUAL( CSteamWorksGameStatsUploader, Steam_OnSteamSessionInfoClosed, GameStatsSessionClosed_t, m_CallbackSteamSessionInfoClosed );	
#endif

#endif

#ifdef GAME_DLL
#ifndef	NO_STEAM
	STEAM_GAMESERVER_CALLBACK( CSteamWorksGameStatsUploader, Steam_OnSteamSessionInfoIssued, GameStatsSessionIssued_t, m_CallbackSteamSessionInfoIssued );
	STEAM_GAMESERVER_CALLBACK( CSteamWorksGameStatsUploader, Steam_OnSteamSessionInfoClosed, GameStatsSessionClosed_t, m_CallbackSteamSessionInfoClosed );	
#endif
#endif

	// Called each frame before entities think
	virtual void FrameUpdatePreEntityThink() {}
	// called after entities think
	virtual void FrameUpdatePostEntityThink();
	virtual void PreClientUpdate() {}

	void StartSession();
	void EndSession();

	void WriteSessionRow();

#ifdef GAME_DLL
	void WriteHostsRow();
#endif

#ifdef CLIENT_DLL
	void ClientDisconnect();
	void ClearServerSessionID() { m_ServerSessionID = 0 ;}
	int  GetNumServerConnects() { return m_iServerConnectCount; }		
#endif

	bool IsCollectingDetails() { return m_bCollectingDetails; }
	bool IsCollectingAnyData() { return m_bCollectingAny; }

	CSteamWorksGameStatsUploader();

	// Init, shutdown
	// return true on success. false to abort DLL init!
	virtual bool Init();
	virtual void PostInit() {}
	virtual void Shutdown() {}

	// Level init, shutdown
	virtual void LevelInitPreEntity() {}
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdown();

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

	virtual bool IsPerFrame() { return true; }

	virtual void FireGameEvent( IGameEvent *event );
	EResult		AddStatsForUpload( KeyValues *pKV, bool bSendImmediately=true );
	time_t		GetTimeSinceEpoch();
	void		FlushStats();

	uint32 GetServerIP() { return m_iServerIP; }
	const char* GetHostName() { return m_pzHostName; }
	bool IsPassworded() { return m_bPassword; }
	RTime32 GetStartTime() { return m_StartTime; }
	RTime32 GetEndTime() { return m_EndTime; }

#ifdef	CLIENT_DLL
	uint64 GetServerSessionID() { return m_ServerSessionID; }
#endif

	uint64		GetSessionID() { return m_SessionID; }
	void		ClearSessionID();

private:
	void		UploadCvars();
	void		Reset();
	bool		VerifyInterface();
	int			GetHumanCountInGame();

	EResult		RequestSessionID();

	bool		AccessToSteamAPI();
	EResult		WriteIntToTable( const int value, uint64 iTableID, const char *pzRow );
	EResult		WriteInt64ToTable( const uint64 value, uint64 iTableID, const char *pzRow );
	EResult		WriteFloatToTable( const float value, uint64 iTableID, const char *pzRow );
	EResult		WriteStringToTable( const char *pzValue, uint64 iTableID, const char *pzRow );
	EResult		WriteOptionalFloatToTable( KeyValues *pKV, const char* keyName, uint64 iTableID, const char *pzRow);
	EResult		WriteOptionalIntToTable( KeyValues *pKV, const char* keyName, uint64 iTableID, const char *pzRow );

	ISteamGameStats* GetInterface( void );	
	EResult		ParseKeyValuesAndSendStats( KeyValues *pKV, bool bIncludeClientsServerSessionID = true );
	void		ServerAddressToInt();

	ISteamGameStats*	m_SteamWorksInterface;

	uint64				m_UserID;
	uint32				m_iAppID;
	uint32				m_iServerIP;
	int					m_nClientJoinMethod;
	char				m_pzServerIP[MAX_PATH];
	char				m_pzMapStart[MAX_PATH];
	char				m_pzHostName[MAX_PATH];
	RTime32				m_StartTime;
	RTime32				m_EndTime;
	int					m_HumanCntInGame;
	int					m_FriendCntInGame;
	bool				m_bPassword;

	// Session IDs
	uint64				m_SessionID;
	bool				m_SessionIDRequestUnsent;
	bool				m_SessionIDRequestPending;
	bool				m_bCollectingAny;
	bool				m_bCollectingDetails;

#ifdef	CLIENT_DLL
	uint64				m_ServerSessionID;
#endif

	bool				m_ServiceTicking;
	float				m_LastServiceTick;

	bool				m_UploadedStats;

	KeyValueStatList	m_StatsToSend;

	ClientServerSession_t m_ActiveSession;
	int					m_iServerConnectCount;
};

CSteamWorksGameStatsUploader& GetSteamWorksSGameStatsUploader();

#endif // STEAMWORKS_GAMESTATS_H
