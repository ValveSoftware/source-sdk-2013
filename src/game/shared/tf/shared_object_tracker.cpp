//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "shared_object_tracker.h"
#include "gcsdk/gcclient.h"
#include "gc_clientsystem.h"

#ifdef CLIENT_DLL
	#include "econ_notifications.h"
	#include "clientmode_tf.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

short g_nQuestSpewFlags = 0;

void SOTrackerSpew( const char* pszBuff, int nType )
{
	if ( ( g_nQuestSpewFlags & nType ) == 0 )
		return;

	Color questDebugColor = 
#ifdef GAME_DLL
	Color( 255, 100, 0, 255 );
	ConColorMsg( questDebugColor, "[SVTrackers]: %s", pszBuff );
#else
	Color( 255, 200, 0, 255 );
	ConColorMsg( questDebugColor, "[CLTrackers]: %s", pszBuff );
#endif
}

void SOTrackerSpewTypeToggle( const CCommand &args )
{
	if ( args.ArgC() != 2 )
	{
		Warning( "Incorrect parameters. Format: command_toggle_SO_TRACKER_SPEW_type <type>\n" );
		return;
	}

	CUtlString strType( args[1] );
	strType.ToLower();
	int nBitMask = 0;

	if ( FStrEq( strType, "objectives" ) )
	{
		nBitMask = SO_TRACKER_SPEW_OBJECTIVES;
	}
	else if ( FStrEq( strType, "itemtrackers" ) )
	{
		nBitMask = SO_TRACKER_SPEW_ITEM_TRACKER_MANAGEMENT;
	}
	else if ( FStrEq( strType, "objectivetrackers" ) )
	{
		nBitMask = SO_TRACKER_SPEW_OBJECTIVE_TRACKER_MANAGEMENT;
	}
	else if ( FStrEq( strType, "commits" ) )
	{
		nBitMask = SO_TRACKER_SPEW_GC_COMMITS;
	}
	else if ( FStrEq( strType, "socache" ) )
	{
		nBitMask = SO_TRACKER_SPEW_SOCACHE_ACTIVITY;
	}
	else if ( FStrEq( strType, "all" ) )
	{
		nBitMask = 0xFFFFFFFF;
	}

	if ( nBitMask == 0 )
	{
		Warning( "Invalid type.  Valid types are: objectives, itemtrackers, objectivetrackers, commits, or all for everything\n" );
		return;
	}

	g_nQuestSpewFlags ^=  nBitMask;

	DevMsg( "%s %s\n", strType.Get(), g_nQuestSpewFlags & nBitMask ? "ENABLED" : "DISABLED" );
}

ConCommand tf_so_tracker_spew_type_toggle( "tf_so_tracker_spew_type_toggle", SOTrackerSpewTypeToggle, NULL
#ifdef CLIENT_DLL
	, FCVAR_CHEAT
#endif 
	);

CBaseSOTracker::CBaseSOTracker( const CSharedObject* pSObject, CSteamID steamIDOwner, CSOTrackerManager* pManager )
	: m_pSObject( pSObject )
	, m_steamIDOwner( steamIDOwner )
	, m_pManager( pManager )
{
	Assert( m_pSObject );
	Assert( m_pManager );
}

CBaseSOTracker::~CBaseSOTracker()
{}

void CBaseSOTracker::Spew() const
{
	DevMsg( "Tracker for object type %d\n", m_pSObject->GetTypeID() );
	m_pSObject->Dump();
}

void CBaseSOTracker::OnRemove()
{
	CommitChangesToDB();
}

CSOTrackerManager::CSOTrackerManager()
	: m_mapItemTrackers( DefLessFunc( SOTrackerMap_t::KeyType_t ) )
	, m_mapUnacknowledgedCommits( DefLessFunc( CommitsMap_t::KeyType_t ) )
#ifdef GAME_DLL
	, CAutoGameSystemPerFrame( "CSOTrackerManager" )
#endif
{}


CSOTrackerManager::~CSOTrackerManager()
{
	SO_TRACKER_SPEW( "Destroying CQuestObjectiveManager\n", SO_TRACKER_SPEW_ITEM_TRACKER_MANAGEMENT );
	Shutdown();
}


void CSOTrackerManager::Initialize()
{
	ListenForGameEvent( "schema_updated" );

#ifdef GAME_DLL
	ListenForGameEvent( "player_spawn" );
	ListenForGameEvent( "player_initial_spawn" );
	ListenForGameEvent( "server_spawn" );
	ListenForGameEvent( "server_shutdown" );
	ListenForGameEvent( "player_disconnect" );

#endif
}


void CSOTrackerManager::Shutdown()
{
	CommitAllChanges();

	m_mapItemTrackers.PurgeAndDeleteElements();
}


void CSOTrackerManager::FireGameEvent( IGameEvent *pEvent )
{
	const char* pszName = pEvent->GetName();
	
	if ( FStrEq( pszName, "schema_updated" ) )
	{
		// Recreate all existing trackers
		m_mapItemTrackers.PurgeAndDeleteElements();
	
		CUtlVector< CSteamID > vecIDsToUpdate;
#ifdef GAME_DLL
		// On the server, we need need new trackers for everyone
		for ( int i = 1; i<= gpGlobals->maxClients; i++)
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer )
			{
				CSteamID& steamID = vecIDsToUpdate[ vecIDsToUpdate.AddToTail() ];
				pPlayer->GetSteamID( &steamID );
			}
		}
#else
		// On the client we just need new trackers for us
		vecIDsToUpdate.AddToTail( steamapicontext->SteamUser()->GetSteamID() );
#endif

		FOR_EACH_VEC( vecIDsToUpdate, i )
		{
			EnsureTrackersForPlayer( vecIDsToUpdate[ i ] );
		}
	}
	else if ( FStrEq( pszName, "server_spawn" ) ) 
	{
		CommitAllChanges();
	}
	else if ( FStrEq( pszName, "server_shutdown" ) )
	{
		Shutdown();
	}
#ifdef GAME_DLL
	else if ( FStrEq( pszName, "player_disconnect" ) )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByUserId( pEvent->GetInt("userid") ) );
		if ( pPlayer )
		{
			CSteamID steamID;
			pPlayer->GetSteamID( &steamID );
			SO_TRACKER_SPEW( CFmtStr( "Unsubscribing from SOCache for user %s\n", steamID.Render() ), SO_TRACKER_SPEW_SOCACHE_ACTIVITY );
			GCClientSystem()->GetGCClient()->RemoveSOCacheListener( steamID, this );
		}
	}
	else if ( FStrEq( pszName, "player_spawn" ) )
	{
		const int nUserID = pEvent->GetInt( "userid" );
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByUserId( nUserID ) );
		EnsureTrackersForPlayer( pPlayer );
	}
	else if ( FStrEq( pszName, "player_initial_spawn" ) )
	{

		CTFPlayer *pNewPlayer = ToTFPlayer( UTIL_PlayerByIndex( pEvent->GetInt( "index" ) ) );
		Assert( pNewPlayer );
		// We want to listen for SO caches
		if ( pNewPlayer && !pNewPlayer->IsBot() )
		{
			CSteamID steamID;
			pNewPlayer->GetSteamID( &steamID );
			if( steamID.IsValid() )
			{
				SO_TRACKER_SPEW( CFmtStr( "Subscribing to SOCache for user %s\n", steamID.Render() ), SO_TRACKER_SPEW_SOCACHE_ACTIVITY );
				GCClientSystem()->GetGCClient()->AddSOCacheListener( steamID, this );

				EnsureTrackersForPlayer( steamID );
			}
		}
	}
#endif
}


void CSOTrackerManager::SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent )
{
	HandleSOEvent( steamIDOwner, pObject, TRACKER_CREATE_OR_UPDATE );
}


void CSOTrackerManager::SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent )
{
	HandleSOEvent( steamIDOwner, pObject, TRACKER_CREATE_OR_UPDATE );
}


void CSOTrackerManager::SODestroyed( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent) 
{
	HandleSOEvent( steamIDOwner, pObject, TRACKER_REMOVE );
}


void CSOTrackerManager::SOCacheSubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent )
{
	SO_TRACKER_SPEW( CFmtStr( "SOCacheSubscribed recieved for user %s\n", steamIDOwner.Render() ), SO_TRACKER_SPEW_SOCACHE_ACTIVITY );
	// Clear out trackers that are all now invalid
	RemoveTrackersForSteamID( steamIDOwner );
	EnsureTrackersForPlayer( steamIDOwner );
}


void CSOTrackerManager::SOCacheUnsubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent )
{
	SO_TRACKER_SPEW( CFmtStr( "SOCacheUnsubscribed recieved for user %s\n", steamIDOwner.Render() ), SO_TRACKER_SPEW_SOCACHE_ACTIVITY );
	RemoveTrackersForSteamID( steamIDOwner );
}


void CSOTrackerManager::HandleSOEvent( const CSteamID & steamIDOwner, const CSharedObject *pObject, ETrackerHandling_t eHandling )
{
	if ( pObject->GetTypeID() != GetType() )
		return;

	// We might not want to track this thing anymore
	if ( eHandling == TRACKER_CREATE_OR_UPDATE && !ShouldTrackObject( steamIDOwner, pObject ) )
	{
		eHandling = TRACKER_REMOVE;
	}

	UpdateTrackerForItem( pObject, eHandling, steamIDOwner );
}

CBaseSOTracker* CSOTrackerManager::GetTracker( SOTrackerMap_t::KeyType_t nKey ) const
{
	auto idx = m_mapItemTrackers.Find( nKey );
	if ( idx != m_mapItemTrackers.InvalidIndex() )
	{
		return m_mapItemTrackers[ idx ];
	}

	return NULL;
}

CommitRecord_t* CSOTrackerManager::GetCommitRecord( CommitsMap_t::KeyType_t nKey )
{
	auto idx = m_mapUnacknowledgedCommits.Find( nKey );
	if ( idx != m_mapUnacknowledgedCommits.InvalidIndex() )
	{
		return m_mapUnacknowledgedCommits[ idx ];
	}

	return NULL;
}

void CSOTrackerManager::UpdateTrackerForItem( const CSharedObject* pItem, ETrackerHandling_t eHandling, CSteamID steamIDOwner )
{	
	// Do we want to make sure we have a tracker, or that we dont have a tracker
	const bool bWantsTracker = eHandling != TRACKER_REMOVE;
	auto idx = m_mapItemTrackers.Find( GetKeyForObjectTracker( pItem, steamIDOwner ) );

	// Wants a tracker and doesnt have one?
	if ( bWantsTracker && idx == m_mapItemTrackers.InvalidIndex() )
	{
		CreateAndAddTracker( pItem, steamIDOwner );
	}
	else if ( !bWantsTracker && idx != m_mapItemTrackers.InvalidIndex() )	// Doesnt want a tracker and has one?
	{
		RemoveAndDeleteTrackerAtIndex( idx );
	}
	else if ( idx != m_mapItemTrackers.InvalidIndex() )
	{
		m_mapItemTrackers[ idx ]->OnUpdate();
	}
}

void CSOTrackerManager::EnsureTrackersForPlayer( const CSteamID& steamIDPlayer )
{
	GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamIDPlayer );
	if ( !pSOCache )
		return;

	CGCClientSharedObjectTypeCache *pSOTypeCache = pSOCache->FindTypeCache( GetType() );

	if ( !pSOTypeCache )
	{
		SO_TRACKER_SPEW( CFmtStr( "No SOCache for %s in %s!\n", steamIDPlayer.Render(), __FUNCTION__ ), SO_TRACKER_SPEW_ITEM_TRACKER_MANAGEMENT );
		return;
	}

	// Go through existing trackers and remove orphaned ones
	FOR_EACH_MAP_FAST( m_mapItemTrackers, i )
	{
		// If we didn't find the object in our cache, remove the tracker
		if ( m_mapItemTrackers[ i ]->GetOwnerSteamID() == steamIDPlayer &&
		     pSOTypeCache->FindSharedObject( *m_mapItemTrackers[ i ]->GetSObject() ) == NULL )
		{
			RemoveAndDeleteTrackerAtIndex( i );
			i = -1;
		}
	}

	// Go through SOTypeCache and ensure we have trackers for every object
	for ( uint32 i=0; i < pSOTypeCache->GetCount(); ++i )
	{
		CSharedObject* pObject = pSOTypeCache->GetObject( i );
		if ( ShouldTrackObject( steamIDPlayer, pObject ) )
		{
			UpdateTrackerForItem( pObject, TRACKER_CREATE_OR_UPDATE, steamIDPlayer );
		}
	}
}

void CSOTrackerManager::EnsureTrackersForPlayer( CTFPlayer* pPlayer )
{
	if ( pPlayer && !pPlayer->IsBot() )
	{
		CSteamID steamID;
		pPlayer->GetSteamID( &steamID );
		if( steamID.IsValid() )
		{
			EnsureTrackersForPlayer( steamID );
		}
	}
}


void CSOTrackerManager::CreateAndAddTracker( const CSharedObject* pItem, CSteamID steamIDOwner )
{
	CBaseSOTracker* pItemTracker = AllocateNewTracker( pItem, steamIDOwner, this );
	auto nKey = GetKeyForObjectTracker( pItem, steamIDOwner );
	m_mapItemTrackers.Insert( nKey, pItemTracker );

	SO_TRACKER_SPEW( CFmtStr( "Created tracker for object: %s\n", GetDebugObjectDescription( pItem ).Get() ), SO_TRACKER_SPEW_ITEM_TRACKER_MANAGEMENT );
}


void CSOTrackerManager::RemoveAndDeleteTrackerAtIndex( SOTrackerMap_t::IndexType_t idx )
{
	SO_TRACKER_SPEW( CFmtStr( "Deleted tracker for object: %s\n", GetDebugObjectDescription( m_mapItemTrackers[ idx ]->GetSObject() ).Get() ), SO_TRACKER_SPEW_ITEM_TRACKER_MANAGEMENT );

	delete m_mapItemTrackers[ idx ];
	m_mapItemTrackers.RemoveAt( idx );
}


void CSOTrackerManager::RemoveTrackersForSteamID( const CSteamID & steamIDOwner )
{
	// We need to remove all trackers for the user
	FOR_EACH_MAP_FAST( m_mapItemTrackers, idx )
	{
		// Don't care about the itemIDs, just the steamID
		if ( m_mapItemTrackers[ idx ]->GetOwnerSteamID() == steamIDOwner )
		{
			m_mapItemTrackers[ idx ]->OnRemove();

			delete m_mapItemTrackers[ idx ];
			m_mapItemTrackers.RemoveAt( idx );
			idx = -1; // Reset to be safe
		}
	}
}


void CSOTrackerManager::CommitAllChanges()
{
	// Commit everything
	FOR_EACH_MAP_FAST( m_mapItemTrackers, idx )
	{
		m_mapItemTrackers[ idx ]->CommitChangesToDB();
	}
}

void CSOTrackerManager::Spew()
{
	DevMsg( "--- Spewing all trackers for %s ---\n", GetName() );
	
	FOR_EACH_MAP( m_mapItemTrackers, i )
	{
		const CBaseSOTracker* pTracker = m_mapItemTrackers[ i ];
		CSteamID steamID( m_mapItemTrackers.Key( i ) );
		DevMsg( "\tTrackers for %s:\n", steamID.Render() );
		pTracker->Spew();
		DevMsg( "\t---\n" );
	}
}


#ifdef GAME_DLL

void CSOTrackerManager::CommitRecord( CommitRecord_t* pRecord ) const
{
	SO_TRACKER_SPEW( CFmtStr( "Sending %fs old record to GC for SObject. %s\n", Plat_FloatTime() - pRecord->m_flReportedTime, pRecord->m_pProtoMsg->DebugString().c_str() ), SO_TRACKER_SPEW_GC_COMMITS );

	SendMessageForCommit( pRecord->m_pProtoMsg );

	pRecord->m_flLastCommitTime = Plat_FloatTime();
}

void CSOTrackerManager::FrameUpdatePreEntityThink()
{
	// Rate limit to once a second
	double flNextCommitTime = m_flLastUnacknowledgeCommitTime + 1.f;
	double flNow = Plat_FloatTime();

	if ( flNow > flNextCommitTime )
	{
		m_flLastUnacknowledgeCommitTime = flNow;

		auto i = m_mapUnacknowledgedCommits.FirstInorder();
		while( i != m_mapUnacknowledgedCommits.InvalidIndex() )
		{
			auto currentIndex = i;
			i = m_mapUnacknowledgedCommits.NextInorder( i );

			// Give records 10 minutes to get themselves reported and acknowledged
			if ( flNow - m_mapUnacknowledgedCommits[ currentIndex ]->m_flReportedTime > 600.f )
			{
				SO_TRACKER_SPEW( CFmtStr( "Record is %fs old.  Abandoning. %s\n", m_mapUnacknowledgedCommits[ currentIndex ]->m_flReportedTime, m_mapUnacknowledgedCommits[ currentIndex ]->m_pProtoMsg->DebugString().c_str() ), SO_TRACKER_SPEW_GC_COMMITS );
				m_mapUnacknowledgedCommits.RemoveAt( currentIndex );
			}
			else if ( m_mapUnacknowledgedCommits[ currentIndex ]->m_flLastCommitTime + 30.f < flNow )
			{
				// Only try committing for a given contract once every 30 seconds
				CommitRecord( m_mapUnacknowledgedCommits[ currentIndex ] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add a record of a commit to the GC.  This is so we can listen for a
//			response from the GC (or lack thereof) and attempt to re-commit if needed
//-----------------------------------------------------------------------------
void CSOTrackerManager::AddCommitRecord( const ::google::protobuf::Message* pRecord, uint64 nKey, bool bRequireResponse )
{
	// If we don't require a response, don't create a commit record that we have to track.  Just commit right now
	if ( !bRequireResponse )
	{
		SendMessageForCommit( pRecord );
		return;
	}

	bool bShouldCommitNow = false;

	// Check if there's no record for this commit
	auto idx = m_mapUnacknowledgedCommits.Find( nKey );
	if ( idx == m_mapUnacknowledgedCommits.InvalidIndex() )
	{
		// Add it if nothing for this item
		::google::protobuf::Message* pCopy = AllocateNewProtoMessage();
		pCopy->CopyFrom( *pRecord );
		idx = m_mapUnacknowledgedCommits.Insert( nKey, new CommitRecord_t( pCopy ) );
		bShouldCommitNow = true;

		SO_TRACKER_SPEW( CFmtStr( "Creating new commit record for SObject: %s\n", pRecord->DebugString().c_str() ), SO_TRACKER_SPEW_GC_COMMITS );
	}
	else
	{
		::google::protobuf::Message* pExisting = m_mapUnacknowledgedCommits[ idx ]->m_pProtoMsg;
		// Check if this new record is more up to date than an existing commit record.  If so, update the existing one
		if ( CompareRecords( pRecord, pExisting ) > 0 )
		{
			pExisting->CopyFrom( *pRecord );
			bShouldCommitNow = true;

			SO_TRACKER_SPEW( CFmtStr( "Updating existing commit record for SObject: %s\n", pRecord->DebugString().c_str() ), SO_TRACKER_SPEW_GC_COMMITS );
		}
		else
		{
			SO_TRACKER_SPEW( CFmtStr( "Existing commit record for SObject is more up to date: %s\n", pExisting->DebugString().c_str() ), SO_TRACKER_SPEW_GC_COMMITS );
		}
	}

	if ( bShouldCommitNow )
	{
		CommitRecord( m_mapUnacknowledgedCommits[ idx ] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle the GC responding to an earlier commit.  Remove any unacknowledged
//			commits records we have.
//-----------------------------------------------------------------------------
void CSOTrackerManager::AcknowledgeCommit( const ::google::protobuf::Message* pRecord, uint64 nKey )
{
	OnCommitRecieved( pRecord );

	// Find the record
	auto idx = m_mapUnacknowledgedCommits.Find( nKey );
	if ( idx != m_mapUnacknowledgedCommits.InvalidIndex() )
	{
		::google::protobuf::Message* pCommitRecord = m_mapUnacknowledgedCommits[ idx ]->m_pProtoMsg;

		// See if we have a matching record.  If so, remove it
		if ( CompareRecords( pCommitRecord, pRecord ) == 0 )
		{
			SO_TRACKER_SPEW( CFmtStr( "Got matched response for with record: %s\n", pRecord->DebugString().c_str() ), SO_TRACKER_SPEW_GC_COMMITS );

			delete m_mapUnacknowledgedCommits[ idx ];
			m_mapUnacknowledgedCommits.RemoveAt( idx );
		}
		else
		{
			SO_TRACKER_SPEW( CFmtStr( "Ignoring stale response with record: %s\n", pRecord->DebugString().c_str() ), SO_TRACKER_SPEW_GC_COMMITS );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Force a spew of all unacknowledged commits
//-----------------------------------------------------------------------------
void CSOTrackerManager::DBG_SpewPendingCommits()
{
	SO_TRACKER_SPEW( CFmtStr( "Unacknowledged commits: %d\n", m_mapUnacknowledgedCommits.Count() ), SO_TRACKER_SPEW_GC_COMMITS );
	FOR_EACH_MAP( m_mapUnacknowledgedCommits, i )
	{
		SO_TRACKER_SPEW( CFmtStr( "%d: %s\n", i, m_mapUnacknowledgedCommits[ i ]->m_pProtoMsg->DebugString().c_str() ), SO_TRACKER_SPEW_GC_COMMITS );
	}
}

#if ( defined( DEBUG ) || defined( STAGING_ONLY ) ) && defined( GAME_DLL )
CON_COMMAND( tf_quests_spew_unacknowledged_commits, "Spews info on all unacknowledged commits" )
{
//	QuestObjectiveManager()->DBG_SpewPendingCommits();
}
#endif

#endif
