//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Tracker for War Data on a player
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_gc_server.h"
#include "tf_wartracker.h"
#include "tf_gcmessages.pb.h"
#include "tf_player.h"
#include "gcsdk/gcconstants.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define WAR_ASSIST_POINTS 1
#define WAR_KILL_POINTS 2
#define WAR_WIN_POINTS 20



CTFWarTrackerManager* GetWarTrackerManager()
{
	static CTFWarTrackerManager trackerManager;
	return &trackerManager;
}


CTFWarTrackerManager::CTFWarTrackerManager()
{}

CSOTrackerManager::SOTrackerMap_t::KeyType_t CTFWarTrackerManager::GetKeyForObjectTracker( const CSharedObject* pItem, CSteamID steamIDOwner )
{
	return steamIDOwner.ConvertToUint64();
}

#ifdef GAME_DLL
void CTFWarTrackerManager::SendMessageForCommit( const ::google::protobuf::Message* pProtoMessage ) const
{
	GCSDK::CProtoBufMsg< CGCMsgGC_War_IndividualUpdate > msg( k_EMsgGC_War_IndividualUpdate );
	msg.Body() = *assert_cast< const CGCMsgGC_War_IndividualUpdate* >( pProtoMessage );
	GCClientSystem()->BSendMessage( msg );
}
#endif

CFmtStr CTFWarTrackerManager::GetDebugObjectDescription( const CSharedObject* pItem ) const
{
	const CWarData* pWarData = assert_cast< const CWarData* >( pItem );
	const CWarDefinition* pWarDef = GetItemSchema()->GetWarDefinitionByIndex( pWarData->Obj().war_id() );
	const CWarDefinition::CWarSideDefinition_t* pSide = pWarDef->GetSide( pWarData->Obj().affiliation() );
	
	return CFmtStr( "%s: Side: %s Points: %d", pWarDef->GetDefName(), pSide->m_pszLocalizedName, pWarData->Obj().points_scored() );
}

CBaseSOTracker* CTFWarTrackerManager::AllocateNewTracker( const CSharedObject* pItem, CSteamID steamIDOwner, CSOTrackerManager* pManager ) const
{
	return new CTFWarTracker( pItem, steamIDOwner, pManager );
}

::google::protobuf::Message* CTFWarTrackerManager::AllocateNewProtoMessage() const
{
	return new CGCMsgGC_War_IndividualUpdate();
}

void CTFWarTrackerManager::OnCommitRecieved( const ::google::protobuf::Message* pProtoMsg )
{
	// ...
}

bool CTFWarTrackerManager::ShouldTrackObject( const CSteamID & steamIDOwner, const CSharedObject *pObject ) const
{
	// We only want war data!
	if ( pObject->GetTypeID() != CWarData::k_nTypeID )
		return false;

#ifdef CLIENT_DLL
	if ( steamapicontext()->SteamUser()->GetSteamID() != steamIDOwner )
		return false;
#endif

	const CWarData* pWarData = assert_cast< const CWarData* >( pObject );
	const CWarDefinition* pWarDef = GetItemSchema()->GetWarDefinitionByIndex( pWarData->Obj().war_id() );
	// Needs to be for a real war!
	if ( !pWarDef )
		return false;

	if ( !pWarDef->IsActive() )
		return false;

	// Needs to be for a valid side!
	if ( !pWarDef->IsValidSide( pWarData->Obj().affiliation() ) )
		return false;

	return true;
}

int CTFWarTrackerManager::CompareRecords( const ::google::protobuf::Message* pNewProtoMsg, const ::google::protobuf::Message* pExistingProtoMsg ) const
{
	const CGCMsgGC_War_IndividualUpdate* pNew = assert_cast< const CGCMsgGC_War_IndividualUpdate* >( pNewProtoMsg );
	const CGCMsgGC_War_IndividualUpdate* pExisting = assert_cast< const CGCMsgGC_War_IndividualUpdate* >( pExistingProtoMsg );

	return pNew->score() - pExisting->score();
}

CTFWarTracker::CTFWarTracker( const CSharedObject* pItem, CSteamID SteamIDOwner, CSOTrackerManager* pManager )
	: CBaseSOTracker( pItem, SteamIDOwner, pManager )
{
	m_ProtoData.Clear();
	m_ProtoData.set_steam_id( SteamIDOwner.ConvertToUint64() );
	ListenForGameEvent( "player_score_changed" );
}



//-----------------------------------------------------------------------------
// Purpose: Send to the GC our points scored for Team Spy or Team Engy
//-----------------------------------------------------------------------------
void CTFWarTracker::CommitChangesToDB()
{
	// Nothing to update!
 	if ( m_ProtoData.score() == 0 )
		return;

	m_pManager->AddCommitRecord( &m_ProtoData, GetOwnerSteamID().ConvertToUint64(), false );

	// Reset!  We're adding to our score on the GC, not setting it
	m_ProtoData.set_score( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Listen for player deaths and give points if the player we're tracking
//			kills someone on the other team
//-----------------------------------------------------------------------------
void CTFWarTracker::FireGameEvent( IGameEvent *pEvent )
{
	// Only score points in competitive
	if ( !TFGameRules()->IsCompetitiveMode() )
		return;

	const char *pszName = pEvent->GetName();

	// You get a point when you kill, or assist in a kill, while in a comp game
	if( FStrEq( pszName, "player_score_changed" ) )
	{
		const CTFPlayer *pScorer = ToTFPlayer( UTIL_PlayerByIndex( pEvent->GetInt( "player" ) ) );
		const CTFPlayer *pOwner = GetTrackedPlayer();

		if ( pScorer != pOwner )
			return;

		int nDelta = pEvent->GetInt( "delta", 0 );

		// Only take positive changes
		if ( nDelta <= 0 )
			return;

		m_ProtoData.set_score( m_ProtoData.score() + nDelta );
	}

}
