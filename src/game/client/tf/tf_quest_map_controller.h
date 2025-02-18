//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_QUEST_MAP_CONTROLLER_H
#define TF_QUEST_MAP_CONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_gcmessages.h"
#include "tf_proto_def_messages.h"
#include "local_steam_shared_object_listener.h"
#include "tf_quest_map.h"

enum EQuestTurnInState
{
	TURN_IN_BEGIN = 0,
	TURN_IN_SHOW_SUCCESS,
	TURN_IN_HIDE_SUCCESS,
	TURN_IN_SHOW_STARS_EARNED,
	TURN_IN_SHOW_BLOOD_MONEY_EARNED,
	TURN_IN_SHOW_ITEMS_EARNED_EARNED,
	TURN_IN_SHOW_ITEM_PICKUP_SCREEN,
	TURN_IN_HIDE_NODE_VIEW,
	TURN_IN_SHOW_NODE_UNLOCKS,
	TURN_IN_SHOW_GLOBAL_BLOOD_MONEY,
	TURN_IN_SHOW_GLOBAL_STARS,
	TURN_IN_SHOW_FAILURE,
	TURN_IN_HIDE_FAILURE,
	TURN_IN_COMPLETE
};

class CGCClientQuestProgressReport;

class CQuestMapController : public CLocalSteamSharedObjectListener
{
	friend CGCClientQuestProgressReport;
public:
	CQuestMapController();

	void RedeemLootForNode( const CSOQuestMapNode& node );
	void SelectNodeQuest( const CSOQuestMapNode& node, uint32 nQuestDefIndex );
	void PurchaseReward( const CQuestMapStoreItem* pStoreItemDef );
	void SetPartyProgressDisableState( bool bDisable );

	bool BBusyWithARequest() const { return ( Plat_FloatTime() - m_flRequestTime ) < 5.f; }
	const CMsgQuestProgressReport& GetMostRecentProgressReport() const { return m_msgMostRecentReport; }

private:

	void SetMostRecentProgressReport( CMsgQuestProgressReport& report ) { m_msgMostRecentReport.CopyFrom( report ); }

	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { NewSOObject( pObject ); }
	virtual void SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { NewSOObject( pObject ); }
	virtual void SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { DestroyedSOObject( pObject ); }

	void NewSOObject( const GCSDK::CSharedObject *pObject );
	void DestroyedSOObject( const GCSDK::CSharedObject *pObject );
	
	float m_flRequestTime;
	CMsgQuestProgressReport m_msgMostRecentReport;
};

CQuestMapController& GetQuestMapController();

#endif // #ifndef TF_QUEST_MAP_CONTROLLER_H
