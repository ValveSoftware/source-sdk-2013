//========= Copyright Valve Corporation, All rights reserved. ============//
#if !defined( CLIENTSTEAMCONTEXT_H )
#define CLIENTSTEAMCONTEXT_H
#ifdef _WIN32
#pragma once
#endif

#include "steam/steam_api.h"
#include "utldelegate.h"

struct SteamLoggedOnChange_t
{
	bool bPreviousLoggedOn;
	bool bLoggedOn;
};

class CClientSteamContext : public CSteamAPIContext
{
public:
	CClientSteamContext();
	~CClientSteamContext();

	void Activate();
	void Shutdown();

#if !defined(NO_STEAM)
	STEAM_CALLBACK( CClientSteamContext, OnSteamServersDisconnected, SteamServersDisconnected_t, m_CallbackSteamServersDisconnected );
	STEAM_CALLBACK( CClientSteamContext, OnSteamServerConnectFailure, SteamServerConnectFailure_t, m_CallbackSteamServerConnectFailure );
	STEAM_CALLBACK( CClientSteamContext, OnSteamServersConnected, SteamServersConnected_t, m_CallbackSteamServersConnected );
#endif

	bool BLoggedOn() { return m_bLoggedOn; }
	EUniverse GetConnectedUniverse() { return m_nUniverse; }
	uint32 GetAppID() { return m_nAppID; }
	const CSteamID & GetLocalPlayerSteamID() { return m_SteamIDLocalPlayer; }

	// Allow others to register for a callback when the Steam logged on status changes	
	void InstallCallback( CUtlDelegate< void ( const SteamLoggedOnChange_t & ) > delegate );
	void RemoveCallback( CUtlDelegate< void ( const SteamLoggedOnChange_t & ) > delegate );
	
private:
	void UpdateLoggedOnState();
	void InvokeCallbacks( const SteamLoggedOnChange_t &loggedOnStatus );
	
	bool m_bActive;
	bool m_bLoggedOn;
	CSteamID m_SteamIDLocalPlayer;
	EUniverse m_nUniverse;
	uint32 m_nAppID;

	CUtlVector< CUtlDelegate< void ( const SteamLoggedOnChange_t & ) > > m_LoggedOnCallbacks;
};

CClientSteamContext &ClientSteamContext();			// singleton accessor

#endif // CLIENTSTEAMCONTEXT_H