//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2mp_cvars.h"
#include "igamesystem.h"

static void HL2MPHostnameChanged( IConVar *pConVar, const char *, float )
{
	if ( Q_strcmp( pConVar->GetName(), "hostname" ) )
	{
		return;
	}

	ConVarRef hostname( pConVar );
	IGameEvent *event = gameeventmanager->CreateEvent( "server_cvar" );
	if ( !event )
	{
		return;
	}

	event->SetString( "cvarname", "hostname" );
	event->SetString( "cvarvalue", hostname.GetString() );
	gameeventmanager->FireEvent( event );
}

class CHL2MPHostnameNotifier : public CAutoGameSystem
{
public:
	CHL2MPHostnameNotifier() : CAutoGameSystem( "CHL2MPHostnameNotifier" )
	{
	}

	bool Init() OVERRIDE
	{
		cvar->InstallGlobalChangeCallback( HL2MPHostnameChanged );
		return true;
	}

	void Shutdown() OVERRIDE
	{
		cvar->RemoveGlobalChangeCallback( HL2MPHostnameChanged );
	}
};

static CHL2MPHostnameNotifier g_HL2MPHostnameNotifier;

// Ready restart
ConVar mp_readyrestart(
							"mp_readyrestart", 
							"0", 
							FCVAR_GAMEDLL,
							"If non-zero, game will restart once each player gives the ready signal" );

// Ready signal
ConVar mp_ready_signal(
							"mp_ready_signal",
							"ready",
							FCVAR_GAMEDLL,
							"Text that each player must speak for the match to begin" );