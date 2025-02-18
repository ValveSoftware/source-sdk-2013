//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Class to act on a client's quest map, and handle communicating with
//			the GC with quest map related things.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_quest_map_controller.h"
#include "gcsdk/msgprotobuf.h"
#include "gc_clientsystem.h"
#include "econ_quests.h"
#include "tf_hud_mainmenuoverride.h"
#include "econ_gcmessages.pb.h"

using namespace GCSDK;

template < typename MSG_TYPE, ETFGCMsg E_MSG_TYPE >
class CQuestRequestBase : public GCSDK::CGCClientJob
{
public:
	typedef CProtoBufMsg< MSG_TYPE >   Msg_t;
	typedef CProtoBufMsg< CMsgGCQuestResponse > Reply_t;

	CQuestRequestBase()
		: GCSDK::CGCClientJob( GCClientSystem()->GetGCClient() )
		, m_msg( E_MSG_TYPE )
		, m_msgReply()
	{
		this->StartJobDelayed( NULL );
	}

	Msg_t &Msg() { return m_msg; }

	virtual bool BYieldingRunJob( void *pvStartParam )
	{
		// The "We're about to do a thing" event
		IGameEvent * event = gameeventmanager->CreateEvent( "quest_request" );
		if ( event )
		{
			event->SetInt( "request", E_MSG_TYPE );
			event->SetString( "msg", m_msg.Body().SerializeAsString().c_str() );
			gameeventmanager->FireEventClientSide( event );
		}

		BYldSendMessageAndGetReply_t result = BYldSendMessageAndGetReplyEx( m_msg, 5, &m_msgReply, E_MSG_TYPE );

		// The "We just got a response for a thing" event
		bool bSuccess = result == BYLDREPLY_SUCCESS;
		event = gameeventmanager->CreateEvent( "quest_response" );
		if ( event )
		{
			event->SetInt( "request", E_MSG_TYPE );
			event->SetBool( "success", bSuccess );
			event->SetString( "msg", m_msg.Body().SerializeAsString().c_str() );
			gameeventmanager->FireEventClientSide( event );
		}

		return bSuccess;
	}

protected:

	Msg_t   m_msg;
	Reply_t m_msgReply;
};

CQuestMapController& GetQuestMapController()
{
	static CQuestMapController s_Controller;
	return s_Controller;
}

CQuestMapController::CQuestMapController()
	: m_flRequestTime( 0.f )
{}

void CQuestMapController::RedeemLootForNode( const CSOQuestMapNode& node )
{
	// We're already doing something!
	if ( BBusyWithARequest() )
		return;

	class CQuestRequestTurnInNode : public CQuestRequestBase < CMsgGCQuestNodeTurnIn,
		k_EMsgGCQuestNodeTurnIn >
	{};

	CQuestRequestTurnInNode* pRequest = new CQuestRequestTurnInNode;

	auto &msg = pRequest->Msg().Body();
	msg.set_node_defindex( node.defindex() );

	auto pEvent = gameeventmanager->CreateEvent( "quest_turn_in_state" );
	if ( pEvent  )
	{
		gameeventmanager->FireEventClientSide( pEvent  );
	}
}

void CQuestMapController::SelectNodeQuest( const CSOQuestMapNode& node, uint32 nQuestDefIndex )
{
	// We're already doing something!
	if ( BBusyWithARequest() )
		return;

	class CQuestRequestUnlockNode : public CQuestRequestBase < CMsgGCQuestMapUnlockNode,
															    k_EMsgGC_QuestMapUnlockNode >
	{};

	CQuestRequestUnlockNode* pRequest = new CQuestRequestUnlockNode;

	auto &msg = pRequest->Msg().Body();
	msg.set_node_defindex( node.defindex() );
	msg.set_quest_defindex( nQuestDefIndex );
}

void CQuestMapController::PurchaseReward( const CQuestMapStoreItem* pStoreItemDef )
{
	if ( BBusyWithARequest() )
		return;

	class CQuestPurchaseRewardItem : public CQuestRequestBase < CMsgGCQuestMapPurchaseReward,
		k_EMsgGC_QuestMapPurchaseReward >
	{};

	CQuestPurchaseRewardItem* pRequest = new CQuestPurchaseRewardItem;

	auto& msg = pRequest->Msg().Body();
	msg.set_store_item_defindex( pStoreItemDef->GetDefIndex() );
}

void CQuestMapController::SetPartyProgressDisableState( bool bDisable )
{
	if ( BBusyWithARequest() )
		return;

	class CQuestSetPartyProgressEnabled : public CQuestRequestBase < CMsgGCSetDisablePartyQuestProgress,
		k_EMsgGC_SetDisablePartyQuestProgress >
	{};

	CQuestSetPartyProgressEnabled* pRequest = new CQuestSetPartyProgressEnabled;

	auto& msg = pRequest->Msg().Body();
	msg.set_state( bDisable );
}

void CQuestMapController::NewSOObject( const GCSDK::CSharedObject *pObject )
{
	if ( pObject->GetTypeID() == CQuest::k_nTypeID )
	{
		if ( BBusyWithARequest() )
		{
			// Got it -- we're not requesting anymore
			m_flRequestTime = 0.f;
		}
	}
}

void CQuestMapController::DestroyedSOObject( const GCSDK::CSharedObject *pObject )
{
	if ( pObject->GetTypeID() == CQuest::k_nTypeID )
	{
		if ( BBusyWithARequest() )
		{
			// Got it -- we're not requesting anymore
			m_flRequestTime = 0.f;
		}
	}
}

class CGCClientQuestProgressReport: public GCSDK::CGCClientJob
{
public:
	CGCClientQuestProgressReport( GCSDK::CGCClient *pGCClient ) : GCSDK::CGCClientJob( pGCClient ) { }

	virtual bool BYieldingRunJobFromMsg( IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgQuestProgressReport> msg( pNetPacket );

		GetQuestMapController().SetMostRecentProgressReport( msg.Body() );

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCClientQuestProgressReport, "CGCClientQuestProgressReport", k_EMsgGCQuestProgressReport, k_EServerTypeGCClient );