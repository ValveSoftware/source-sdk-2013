//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef TF_WEARABLE_CAMPAIGN_ITEM_H
#define TF_WEARABLE_CAMPAIGN_ITEM_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_item_wearable.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "shared_object_tracker.h"
#endif

#ifdef CLIENT_DLL
#define CTFWearableCampaignItem C_TFWearableCampaignItem
#endif // CLIENT_DLL

enum wearable_skin_offset_t
{
	WEARABLE_SKIN_STATIC = 0,
	WEARABLE_SKIN_OFF,
	WEARABLE_SKIN_GRID,
	WEARABLE_SKIN_FLASH,

	WEARABLE_NUM_SKINS
};

enum wearable_state_t
{
	WEARABLE_STATE_STATIC = 0,
	WEARABLE_STATE_OFF,
	WEARABLE_STATE_IDLE,
	WEARABLE_STATE_SCORE_NOVICE,
	WEARABLE_STATE_SCORE_ADVANCED,
	WEARABLE_STATE_SCORE_EXPERT,
	WEARABLE_STATE_SCORE_ASSIST_NOVICE,
	WEARABLE_STATE_SCORE_ASSIST_ADVANCED,
	WEARABLE_STATE_SCORE_ASSIST_EXPERT,
	WEARABLE_STATE_COMPLETED_NOVICE,
	WEARABLE_STATE_COMPLETED_ADVANCED,
	WEARABLE_STATE_COMPLETED_EXPERT,

	WEARABLE_NUM_STATES
};

#if defined( CLIENT_DLL )
class CTFWearableCampaignItem : public CTFWearable
#else
class CTFWearableCampaignItem : public CTFWearable, public GCSDK::ISharedObjectListener, public CGameEventListener
#endif
{
	DECLARE_CLASS( CTFWearableCampaignItem, CTFWearable );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFWearableCampaignItem();
	~CTFWearableCampaignItem();

	virtual void Precache() OVERRIDE;
	virtual int GetSkin() OVERRIDE;
	virtual void Equip( CBasePlayer* pOwner ) OVERRIDE;
	virtual void UnEquip( CBasePlayer* pOwner ) OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

#ifdef GAME_DLL
	virtual int UpdateTransmitState() OVERRIDE;

	virtual void SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE { SOEventInternal( steamIDOwner, pObject, eEvent ); }
	virtual void PreSOUpdate( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {}
	virtual void SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE { SOEventInternal( steamIDOwner, pObject, eEvent ); }
	virtual void PostSOUpdate( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {}
	virtual void SODestroyed( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE { SOEventInternal( steamIDOwner, pObject, eEvent ); }
	virtual void SOCacheSubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {}
	virtual void SOCacheUnsubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {}

	void SOEventInternal( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent );
	void EvaluateState( void );
#else
	virtual void ReceiveMessage( int classID, bf_read &msg ) OVERRIDE;
#endif // GAME_DLL

private:

#ifdef CLIENT_DLL
	void HandleStateEffects( int nState );

#else
	void UpdateListenerStatus( bool bShouldListen );

	CSteamID m_steamIDOwner;
	CHandle< CTFPlayer > m_hOwner;
#endif // CLIENT_DLL

	CNetworkVar( int, m_nState );
};

#endif // TF_WEARABLE_CAMPAIGN_ITEM_H
