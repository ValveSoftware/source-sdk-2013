//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef TF_GC_SERVER_H
#define TF_GC_SERVER_H
#ifdef _WIN32
#pragma once
#endif

#if !defined( _X360 ) && !defined( NO_STEAM )
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"
#endif

//#include "tf_gc_common.h"
#include "gcsdk/gcclientsdk.h"
#include "playergroup.h"
//#include "dota_gamerules.h"
#include "gc_clientsystem.h"
#include "tf_gcmessages.h"
#include "GameEventListener.h"
#include "rtime.h"
#include "tf_shareddefs.h"
#include "tf_gc_shared.h"
#include "vote_controller.h"

class CReliableMessageQueue;

//enum EDOTA_Uploading_Match_Stats
//{
//	EDOTA_MATCH_STATS_IDLE,
//	EDOTA_MATCH_STATS_UPLOADING,
//	EDOTA_MATCH_STATS_UPLOAD_COMPLETE
//};

#ifdef ENABLE_GC_MATCHMAKING

class CTFGCServerSystem : public CGCClientSystem
{
	DECLARE_CLASS_GAMEROOT( CTFGCServerSystem, CGCClientSystem );

public:
	CTFGCServerSystem( void );
	~CTFGCServerSystem( void );

	// CAutoGameSystemPerFrame
	virtual bool Init() OVERRIDE;
	virtual void Shutdown() OVERRIDE;
	virtual void PreClientUpdate() OVERRIDE;

	virtual bool ShouldHibernate();

	void ClientActive( CSteamID steamIDClient );
	void ClientConnected( CSteamID steamIDPlayer, edict_t *pEntity );

	float GetTimeLastConnectedToGC( void ) { return m_timeLastConnectedToGC; }

	//
	// Reliable Messages
	//
	const CReliableMessageQueue &ReliableMsgQueue() const { return m_ReliableMsgQueue; }
	CReliableMessageQueue &ReliableMsgQueue() { return m_ReliableMsgQueue; }
	bool BPendingReliableMessages() const { return ReliableMsgQueue().NumPendingMessages() > 0; }
	bool BStalledReliableMessages() const { return ReliableMsgQueue().BStalled(); }

	void ProcessPlayerInventoryRequest( CSteamID steamID, KeyValues* pKVRequest );

protected:

	// CGCClientSystem
	virtual void PreInitGC() OVERRIDE;
	virtual void PostInitGC() OVERRIDE;

private:
	bool m_bSetupSchema;

	CSteamID m_ourSteamID;
	CSteamID m_relayedGameServerSteamID;

	int m_iSavedVisibleMaxPlayers;
	bool m_bOverridingVisibleMaxPlayers;

	float m_timeLastConnectedToGC;
//	DOTAGameVersion	m_GameVersion;

	enum EWebapiEquipmentState {
		kWebapiEquipmentState_Init,

		// Waiting for message from client listing equipped items
		kWebapiEquipmentState_WaitingForClientRequest,

		// Request inventory for the remote client
		kWebapiEquipmentState_RequestInventory,
		kWebapiEquipmentState_WaitingForInventory,
		kWebapiEquipmentState_InventoryReceived,

		// Failure that requires client intervention
		// (they need to request a new auth token and send us more info)
		kWebapiEquipmentState_NotifyClientOfFailure,
	};

	struct WebapiEquipmentState_t
	{
		CSteamID m_ownerID;

		EWebapiEquipmentState m_eState = kWebapiEquipmentState_Init;

		// Webapi equipment request
		HTTPRequestHandle m_hEquipmentRequest = INVALID_HTTPREQUEST_HANDLE;
		CCallResult<CTFGCServerSystem::WebapiEquipmentState_t, HTTPRequestCompleted_t> m_EquipmentRequestCompleted;

		// Current request that is in flight
		KeyValues* m_pKVCurrentRequest = nullptr;

		// If we get a request to change loadout while we are waiting to get the items in the user's
		// current inventory, we wait for the existing request to complete before asking for the new items
		//
		// Note that we will only request new items and assume that any existing items are up-to-date
		// and unchanged, since sdk games cannot mutate the user's actual inventory.
		KeyValues* m_pKVNextRequest = nullptr;

		// Backoff
		RTime32 m_rtNextRequest = 0;
		int m_nBackoffSec = 0;
		void Backoff();
		void RequestSucceeded();	// resets backoff timers
		bool IsBackingOff();

		// Steam callback
		void OnWebapiEquipmentReceived( HTTPRequestCompleted_t* pInfo, bool bIOFailure );
	};
	typedef CUtlMap< CSteamID, WebapiEquipmentState_t*, int, CDefLess<CSteamID> > TMapEquipmentRequests;
	TMapEquipmentRequests m_mapEquipmentRequests;
	WebapiEquipmentState_t& FindOrCreateWebapiEquipmentState( CSteamID steamID );

	void WebapiEquipmentThink();
	void WebapiEquipmentThinkRequest( CSteamID steamID, WebapiEquipmentState_t* pState );
	void OnWebapiEquipmentReceived( CSteamID steamID, HTTPRequestCompleted_t* pInfo, bool bIOFailure );

	void SDK_ApplyInventoryInfo( CGCClientSharedObjectCache* pCache, KeyValues* pKVRequest ); // mod expansion point
	void SDK_ApplyLocalLoadout(CGCClientSharedObjectCache* pCache, KeyValues* pKVRequest);


	//
	// Reliable Messages
	//

	CReliableMessageQueue m_ReliableMsgQueue;
};

CTFGCServerSystem *GTFGCClientSystem();

#endif // #ifdef ENABLE_GC_MATCHMAKING

#endif // TF_GC_SERVER_H
