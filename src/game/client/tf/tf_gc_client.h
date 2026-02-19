//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef _INCLUDED_TF_GC_CLIENT_H
#define _INCLUDED_TF_GC_CLIENT_H
#ifdef _WIN32
#pragma once
#endif

#if !defined( _X360 ) && !defined( NO_STEAM )
#include "steam/steam_api.h"
#endif

//#include "dota_gc_common.h"
#include "gcsdk/gcclientsdk.h"
//#include "dota_gamerules.h"
#include "tf_gcmessages.pb.h"
#include "../clientsteamcontext.h"
#include "gc_clientsystem.h"
#include "GameEventListener.h"
#include "confirm_dialog.h"
#include "econ_game_account_client.h"
#include "netadr.h"
#include "tf_gc_shared.h"

class CTFGCClientSystem : public CGCClientSystem
{
	friend class CLoalPlayerSOCacheListener;
	DECLARE_CLASS_GAMEROOT( CTFGCClientSystem, CGCClientSystem );
public:
	CTFGCClientSystem( void );
	~CTFGCClientSystem( void );

	// CAutoGameSystemPerFrame
	virtual bool Init() OVERRIDE;
	virtual void Update( float frametime ) OVERRIDE;

	void ServerRequestEquipment();
	void LocalInventoryChanged();

private:
	/// Steam callbacks
#define DECL_STEAM_CALLBACK( callback )             \
	void OnSteam##callback ( callback##_t *pInfo ); \
	CCallback<ThisClass, callback##_t, false>       \
		m_callbackSteam##callback { this, &ThisClass::OnSteam##callback };

	// Creates:
	//  void OnSteamThing( Thing_t *pInfo );
	//  CCallback<ThisClass, Thing_t, false > m_callbackSteamThing{ this, &ThisClass::OnSteamThing };

	// DECL_STEAM_CALLBACK( LobbyChatMsg );
	// DECL_STEAM_CALLBACK( SomeOtherFunThing... );
	DECL_STEAM_CALLBACK( GetTicketForWebApiResponse );

#undef DECL_STEAM_CALLBACK

	//
	// SDK inventory
	//
	void WebapiInventoryThink();
	void OnWebapiInventoryReceived( HTTPRequestCompleted_t* pInfo, bool bIOFailure );
	void OnWebapiAuthTicketReceived( GetTicketForWebApiResponse_t* pInfo );

	enum EWebapiInventoryState {
		kWebapiInventoryState_Init,

		// Request inventory for the local client
		kWebapiInventoryState_RequestAuthToken,
		kWebapiInventoryState_WaitingForAuthToken,
		kWebapiInventoryState_AuthTokenReceived,
		kWebapiInventoryState_RequestInventory,
		kWebapiInventoryState_WaitingForInventory,
		kWebapiInventoryState_InventoryReceived,

		// Once we have the local client inventory, we will update it to match
		// our set of equipped items, and then we will build an auth ticket to
		// send to whatever server we are connected to.
		kWebapiInventoryState_BuildServerMessage,
		kWebapiInventoryState_RequestServerAuthToken,
		kWebapiInventoryState_WaitingForServerAuthToken,
		kWebapiInventoryState_ServerAuthTokenReceived,
		kWebapiInventoryState_SentToServer,
	};

	struct WebapiInventoryState_t
	{
		EWebapiInventoryState m_eState = kWebapiInventoryState_Init;

		// Authentication
		HAuthTicket m_hSteamAuthTicket = k_HAuthTicketInvalid;
		CUtlVector<uint8> m_bufAuthToken;

		// Inventory request
		HTTPRequestHandle m_hInventoryRequest = INVALID_HTTPREQUEST_HANDLE;
		CCallResult<CTFGCClientSystem, HTTPRequestCompleted_t> m_InventoryRequestCompleted;

		// Server inventory -- they get a subset of our items that we allow
		CMsgAuthorizeServerItemRetrieval m_msgItems;
		CUtlMemory<char> m_strMsgItems; // serialized and base64 encoded version of m_msgItems, so we can sign it
		HAuthTicket m_hServerAuthTicket = k_HAuthTicketInvalid;
		CUtlVector<uint8> m_bufServerAuthToken;
		CUtlString m_strServerIdentity; // hex-encoded SHA256 of m_bufMsgItems

		// Did we make any changes that we need to communicate to a server?
		bool m_bLocalChangesApplied = false;

		// Backoff
		RTime32 m_rtNextRequest = 0;
		int m_nBackoffSec = 0;
		void Backoff();
		void RequestSucceeded();	// resets backoff timers
		bool IsBackingOff();
	};
	WebapiInventoryState_t m_WebapiInventory;

	//
	// SDK Server inventory -- just get the auth ticket and send it to the server
	//
	enum EWebapiServerInventoryState {
		kWebapiServerInventoryState_Init,
		kWebapiServerInventoryState_RequestAuthToken,
		kWebapiServerInventoryState_WaitingForAuthToken,
		kWebapiServerInventoryState_AuthTokenReceived,
		kWebapiServerInventoryState_SendToServer
	};
	WebapiInventoryState_t m_WebapiServerInventory;
	void OnWebapiServerAuthTicketReceived( GetTicketForWebApiResponse_t* pInfo );

	// SDK expansion points
	void SDK_SelectItemsToSendToServer( CMsgAuthorizeServerItemRetrieval* /*out*/ pMsg, CGCClientSharedObjectCache* pSOCache );
	void SDK_AddServerInventoryInfo( KeyValues* /*out*/ pKV, CGCClientSharedObjectCache* pSOCache );
};

CTFGCClientSystem* GTFGCClientSystem();

#endif // _INCLUDED_TF_GC_CLIENT_H
