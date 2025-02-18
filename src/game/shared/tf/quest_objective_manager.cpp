//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "quest_objective_manager.h"
#include "gcsdk/gcclient.h"
#include "gc_clientsystem.h"
#include "econ_quests.h"
#include "steamworks_gamestats.h"
#include "tf_gamerules.h"
#include "entity_halloween_pickup.h"
#ifdef CLIENT_DLL
	#include "econ_notifications.h"
	#include "tf_item_inventory.h"
	#include "clientmode_tf.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_mm_trusted;

CQuestObjectiveManager::CQuestObjectiveManager()
{}

CQuestObjectiveManager::~CQuestObjectiveManager()
{
	SO_TRACKER_SPEW( "Destroying CQuestObjectiveManager\n", SO_TRACKER_SPEW_ITEM_TRACKER_MANAGEMENT );
	Shutdown();

	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		GCClientSystem()->GetGCClient()->RemoveSOCacheListener( steamID, this );
	}
}


CSOTrackerManager::SOTrackerMap_t::KeyType_t CQuestObjectiveManager::GetKeyForObjectTracker( const CSharedObject* pItem, CSteamID steamIDOwner )
{
	return assert_cast< const CQuest* >( pItem )->GetID();
}

bool CQuestObjectiveManager::ShouldTrackObject( const CSteamID & steamIDOwner, const CSharedObject *pObject ) const
{
	// We only care about quests!
	if( pObject->GetTypeID() != CQuest::k_nTypeID )
		return false;

	CQuest* pQuest = (CQuest*)( pObject );
	if ( !pQuest->Obj().active() || !pQuest->GetDefinition()->BActive() )
		return false;

	SO_TRACKER_SPEW( CFmtStr( "Accepting quest %llu with defindex %d.\n", pQuest->GetID(), pQuest->GetDefinition()->GetDefIndex() ), SO_TRACKER_SPEW_TRACKER_ACCEPTANCE );
	return true;
}

int CQuestObjectiveManager::CompareRecords( const ::google::protobuf::Message* pNewProtoMsg, const ::google::protobuf::Message* pExistingProtoMsg ) const
{
	const CMsgGCQuestObjective_PointsChange* pNew = assert_cast< const CMsgGCQuestObjective_PointsChange* >( pNewProtoMsg );
	const CMsgGCQuestObjective_PointsChange* pExisting = assert_cast< const CMsgGCQuestObjective_PointsChange* >( pExistingProtoMsg );


	int nNewPoints =		( pNew->points_2() << 16 )		+ ( pNew->points_1() << 8 )		 + pNew->points_0();
	int nExistingPoints =	( pExisting->points_2() << 16 ) + ( pExisting->points_1() << 8 ) + pExisting->points_0();

	return nNewPoints - nExistingPoints;
}

#ifdef CLIENT_DLL
void CQuestObjectiveManager::UpdateFromServer( itemid_t nID, uint32 nPoints0, uint32 nPoints1, uint32 nPoints2 )
{
	CQuestItemTracker* pTracker = assert_cast< CQuestItemTracker* >( GetTracker( nID ) );
	if ( pTracker )
	{
		pTracker->UpdateFromServer( nPoints0, nPoints1, nPoints2 );
	}
	else
	{
		SO_TRACKER_SPEW( CFmtStr( "Got update from server, but itemID: %llu doesn't exist!", nID ), SO_TRACKER_SPEW_OBJECTIVES );
	}
}
#endif // CLIENT_DLL

#ifdef GAME_DLL
void CQuestObjectiveManager::SendMessageForCommit( const ::google::protobuf::Message* pProtoMessage ) const
{
	GCSDK::CProtoBufMsg< CMsgGCQuestObjective_PointsChange > msg( k_EMsgGCQuestObjective_PointsChange );
	msg.Body() = *assert_cast< const CMsgGCQuestObjective_PointsChange* >( pProtoMessage );
	GCClientSystem()->BSendMessage( msg );
}
#endif

int CQuestObjectiveManager::GetType() const
{
	return CQuest::k_nTypeID; 
}

CFmtStr CQuestObjectiveManager::GetDebugObjectDescription( const CSharedObject* pSObject ) const
{
	const CQuest* pItem = assert_cast< const CQuest* >( pSObject );
	return CFmtStr( "%llu (%s)", pItem->GetID(), pItem->GetDefinition()->GetLocName() );
}

CBaseSOTracker* CQuestObjectiveManager::AllocateNewTracker( const CSharedObject* pItem, CSteamID steamIDOwner, CSOTrackerManager* pManager ) const
{
	return new CQuestItemTracker( pItem, steamIDOwner, pManager );
}

::google::protobuf::Message* CQuestObjectiveManager::AllocateNewProtoMessage() const
{
	return new CMsgGCQuestObjective_PointsChange();
}

//-----------------------------------------------------------------------------
// Purpose: Handle the GC responding to an earlier commit.  Remove any unacknowledged
//			commits records we have.
//-----------------------------------------------------------------------------
void CQuestObjectiveManager::OnCommitRecieved( const ::google::protobuf::Message* pProtoMsg )
{
	const CMsgGCQuestObjective_PointsChange* pPointsChangeMsg = assert_cast< const CMsgGCQuestObjective_PointsChange* >( pProtoMsg );
	// Check if we should update points.  This happens when the record comes from a server
	// where the player has disconnected from (this could be ourselves).
	if ( pPointsChangeMsg->update_base_points() )
	{
		CQuestItemTracker* pItemTracker = assert_cast<CQuestItemTracker*>( GetTracker( pPointsChangeMsg->quest_id() ) );
		if ( pItemTracker )
		{
			pItemTracker->UpdatePointsFromSOItem();
		}
	}
}

#ifdef GAME_DLL
CON_COMMAND( tf_quests_spew_trackers, "Spews all currently active quest trackers" )
{
	QuestObjectiveManager()->Spew();
}

CON_COMMAND( ensure_so_trackers_for_steamid, "Ensures a steamID has all the trackers it should have, with extra spew along the way" )
{
	if ( args.ArgC() != 2 )
	{
		Warning( "Need the 64bit representation of a steamID as well\n" );
		return;
	}

	 CSteamID steamID( (uint32)V_atoi( args[1] ), GetUniverse(), k_EAccountTypeIndividual );

	 if ( !steamID.IsValid() )
	 {
		 Warning( "SteamID is not valid!\n" );
		 return;
	 }

	 g_nQuestSpewFlags |= SO_TRACKER_SPEW_TRACKER_ACCEPTANCE;
	 QuestObjectiveManager()->EnsureTrackersForPlayer( steamID );
	 g_nQuestSpewFlags &= ~SO_TRACKER_SPEW_TRACKER_ACCEPTANCE;
}
#endif


#if ( defined( DEBUG ) || defined( STAGING_ONLY ) ) && defined( GAME_DLL )
CON_COMMAND( tf_quests_spew_unacknowledged_commits, "Spews info on all unacknowledged commits" )
{
	QuestObjectiveManager()->DBG_SpewPendingCommits();
}
#endif // ( defined( DEBUG ) || defined( STAGING_ONLY ) ) && defined( GAME_DLL )

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: GC Msg handler for points change response
//-----------------------------------------------------------------------------
class CGCQuestObjective_PointsChangeResponse : public GCSDK::CGCClientJob
{
public:
	CGCQuestObjective_PointsChangeResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgGCQuestObjective_PointsChange > msg( pNetPacket );

		QuestObjectiveManager()->AcknowledgeCommit( &msg.Body(), msg.Body().quest_id() );

		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCQuestObjective_PointsChangeResponse, "CGCQuestObjective_PointsChangeResponse", k_EMsgGCQuestObjective_PointsChange, GCSDK::k_EServerTypeGCClient );

#endif // GAME_DLL
