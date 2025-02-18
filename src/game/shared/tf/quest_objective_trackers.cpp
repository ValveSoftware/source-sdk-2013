//========= Copyright Valve Corporation, All rights reserved. ============//
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
#include "tf_gamerules.h"
#include "schemainitutils.h"
#include "econ_item_system.h"
#include "econ_quests.h"
#include "tf_proto_script_obj_def.h"
#include "tf_quest_map_node.h"
#include "tf_quest_map_utils.h"
#ifdef CLIENT_DLL
	#include "hud_basechat.h"
	#include "tf_hud_chat.h"
	#include "quest_log_panel.h"
#endif

#ifdef GAME_DLL
	#include "tf_gc_server.h"
	#include "tf_party.h"
	#include "econ_game_account_server.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( DEBUG ) || defined( STAGING_ONLY )
ConVar tf_quests_commit_every_point( "tf_quests_commit_every_point", "0", FCVAR_REPLICATED );
ConVar tf_quests_progress_enabled( "tf_quests_progress_enabled", "1", FCVAR_REPLICATED );
ConVar tf_quests_points_scale( "tf_quests_points_scale", "1", FCVAR_REPLICATED );
#endif


CQuestObjectiveManager *QuestObjectiveManager( void )
{
	static CQuestObjectiveManager g_QuestObjectiveManager;
	return &g_QuestObjectiveManager;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseQuestObjectiveTracker::CBaseQuestObjectiveTracker( const QuestObjectiveInstance_t& objectiveInstance,
														CQuestItemTracker* pParent,
														const CSteamID& ownerSteamID )
	: m_objectiveInstance( objectiveInstance )
	, m_pParent( pParent )
	, m_pEvaluator( NULL )
	, m_steamIDOwner( ownerSteamID )
{
	KeyValues *pKVConditions = m_objectiveInstance.GetObjectiveDef()->GetConditionsKeyValues();
	
	AssertMsg( !m_pEvaluator, "%s", CFmtStr( "Too many input for operator '%s'.", GetConditionName() ).Get() );

	const char *pszType = pKVConditions->GetString( "type" );
	m_pEvaluator = CreateEvaluatorByName( pszType, this );
	AssertMsg( m_pEvaluator != NULL, "%s", CFmtStr( "Failed to create quest condition name '%s' for '%s'", pszType, GetConditionName() ).Get() );

	SO_TRACKER_SPEW( CFmtStr( "Creating objective tracker def %d for quest def %d on item %llu for user %s\n",
							  GetObjectiveDefIndex(),
							  pParent->GetItem()->GetDefinition()->GetDefIndex(),
							  pParent->GetItem()->GetID(),
							  pParent->GetOwnerSteamID().Render() ),
					 SO_TRACKER_SPEW_OBJECTIVE_TRACKER_MANAGEMENT );

	// Let the quest definition apply modifiers
	pParent->GetItem()->GetDefinition()->AddModifiers( m_pEvaluator );
	// Let the objective apply modifier
	m_objectiveInstance.GetObjectiveDef()->AddModifiers( m_pEvaluator );

	if ( !m_pEvaluator->BInitFromKV( pKVConditions, NULL ) )
	{
		AssertMsg( false, "Failed to init from KeyValues" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseQuestObjectiveTracker::~CBaseQuestObjectiveTracker()
{
	if ( m_pEvaluator )
	{
		delete m_pEvaluator;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseQuestObjectiveTracker::IsValidForPlayer( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
{
	return m_pEvaluator->IsValidForPlayer( pOwner, invalidReasons );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const CTFPlayer *CBaseQuestObjectiveTracker::GetQuestOwner() const
{
	return GetQuestOwningPlayer();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseQuestObjectiveTracker::EvaluateCondition( CTFQuestEvaluator *pSender, int nScore )
{
#ifdef GAME_DLL
	// tracker should be the root
	Assert( !GetParent() );
	IncrementCount( nScore );
	ResetCondition();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseQuestObjectiveTracker::ResetCondition()
{
	m_pEvaluator->ResetCondition();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseQuestObjectiveTracker::UpdateConditions()
{
	// clean up previous evaluator
	if ( m_pEvaluator )
	{
		delete m_pEvaluator;
		m_pEvaluator = NULL;
	}

	CUtlVector< CUtlString > vecErrors;
	return BInitFromKV( m_objectiveInstance.GetObjectiveDef()->GetConditionsKeyValues(), &vecErrors );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const CTFPlayer* CBaseQuestObjectiveTracker::GetQuestOwningPlayer() const
{
#ifdef CLIENT_DLL
	return ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
#else
	return ToTFPlayer( GetPlayerBySteamID( m_steamIDOwner ) );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseQuestObjectiveTracker::IncrementCount( int nIncrementValue )
{
	uint32 nPointsToAdd = nIncrementValue * m_objectiveInstance.GetPoints();
	m_pParent->IncrementCount( nPointsToAdd, m_objectiveInstance, m_steamIDOwner );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestItemTracker::CQuestItemTracker( const CSharedObject* pItem, CSteamID SteamIDOwner, CSOTrackerManager* pManager )
	: CBaseSOTracker( pItem, SteamIDOwner, pManager )
	, m_pQuest( NULL )
	, m_bObjectivesOnlyForOwner( true ) // True!  If we then detect their preference is false, we will create
										// trackers for everyone in their party.
#ifdef GAME_DLL
	, m_bHasUnneededObjectiveTrackers( false )
#endif
{
	memset( m_nPoints, 0, sizeof( m_nPoints ) );
#ifdef GAME_DLL
	memset( m_nStartingPoints, 0, sizeof( m_nStartingPoints ) );
	m_mapQuestAssisters.SetLessFunc( DefLessFunc( CSteamID ) );

	ListenForGameEvent( "show_match_summary" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "player_disconnect" );
	ListenForGameEvent( "player_team" );
	ListenForGameEvent( "player_spawn" );

#else
	ListenForGameEvent( "client_disconnect" );
	ListenForGameEvent( "quest_progress" );
#endif

	m_pQuest = assert_cast< const CQuest* >( pItem );
	// Retrieve starting numbers
	UpdatePointsFromSOItem();

	SO_TRACKER_SPEW( CFmtStr( "Creating tracker for quest %d on item %llu for user %s with %ds0 %ds1 %ds2\n",
							  GetItem()->GetDefinition()->GetDefIndex(),
							  GetItem()->GetID(),
							  GetOwnerSteamID().Render(),
							  GetEarnedPoints( 0 ),
							  GetEarnedPoints( 1 ),
							  GetEarnedPoints( 2 ) ),
					 SO_TRACKER_SPEW_ITEM_TRACKER_MANAGEMENT );

	// The owner always gets objective trackers
	EnsureObjectiveTrackersForPlayer( GetOwnerSteamID() );

	// The party may get some trackers
#ifdef GAME_DLL
	auto pSOCache = GCClientSystem()->GetGCClient()->FindSOCache( GetOwnerSteamID() );
	if ( pSOCache )
	{
		pSOCache->AddListener( this );
	}

	UpdatePartyProgressPreference();
#endif

	if ( m_vecObjectiveTrackers.IsEmpty() )
	{
		SO_TRACKER_SPEW( CFmtStr( "Did not create any objective trackers for quest %d on item %llu for user %s with %ds0 %ds1 %ds2\n",
								  GetItem()->GetDefinition()->GetDefIndex(),
								  GetItem()->GetID(),
								  GetOwnerSteamID().Render(),
								  GetEarnedPoints( 0 ),
								  GetEarnedPoints( 1 ),
								  GetEarnedPoints( 2 ) ),
						 SO_TRACKER_SPEW_OBJECTIVE_TRACKER_MANAGEMENT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestItemTracker::~CQuestItemTracker()
{
#ifdef CLIENT_DLL
	SO_TRACKER_SPEW( CFmtStr( "Deleting tracker for questdef %d on quest %llu with %dp0 %dp1 %dp2\n",
							   m_pQuest->GetDefinition()->GetDefIndex(),
							   m_pQuest->GetID(),
							   m_nPoints[ 0 ],
							   m_nPoints[ 1 ],
							   m_nPoints[ 2 ] ),
					 SO_TRACKER_SPEW_ITEM_TRACKER_MANAGEMENT );
#else
	SO_TRACKER_SPEW( CFmtStr( "Deleting tracker for questdef %d on quest %llu with %dp0 %dp1 %dp2 and %dsp0 %dsp1 %dsp2\n",
							   m_pQuest->GetDefinition()->GetDefIndex(),
							   m_pQuest->GetID(),
							   m_nPoints[ 0 ],
							   m_nPoints[ 1 ],
							   m_nPoints[ 2 ],
							   m_nStartingPoints[ 0 ],
							   m_nStartingPoints[ 1 ],
							   m_nStartingPoints[ 2 ] ),
					 SO_TRACKER_SPEW_ITEM_TRACKER_MANAGEMENT );
#endif

#ifdef GAME_DLL
	auto pSOCache = GCClientSystem()->GetGCClient()->FindSOCache( GetOwnerSteamID() );
	if ( pSOCache )
	{
		pSOCache->RemoveListener( this );
	}

	m_mapQuestAssisters.RemoveAll();
#endif

	m_vecObjectiveTrackers.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: Take a look at our item and update what we think our points are
//			based on the attributes on the item IF they are greater.  We NEVER
//			want to lose progress for any reason.
//-----------------------------------------------------------------------------
void CQuestItemTracker::UpdatePointsFromSOItem()
{
	for ( int i=0; i < EQuestPoints_ARRAYSIZE; ++i )
	{
		// A bit slower, but we rarely do this
		const google::protobuf::FieldDescriptor* pField = m_pQuest->Obj().descriptor()->FindFieldByName( CFmtStr( "points_%d", i ).Get() );
		Assert( pField );
		if( !pField )
			continue;

		uint32 nNewPoints = m_pQuest->Obj().GetReflection()->GetUInt32( m_pQuest->Obj(), pField );
#ifdef GAME_DLL															 
		m_nStartingPoints[ i ] = Max( nNewPoints, m_nStartingPoints[ i ] );	 
#else																	 
		m_nPoints[ i ] = Max( nNewPoints, m_nPoints[ i ] );					 
#endif	
	}					 
																 

#ifdef GAME_DLL
	SendUpdateToClient( NULL, CSteamID( 0ULL ) );

	SO_TRACKER_SPEW( CFmtStr( "Updated points from item.  %dp0 %dp1 %dp2 and %dsp0 %dsp1 %dsp2\n", 
							  m_nPoints[ 0 ], 
							  m_nPoints[ 1 ],
							  m_nPoints[ 2 ],
							  m_nStartingPoints[ 0 ], 
							  m_nStartingPoints[ 1 ],
							  m_nStartingPoints[ 2 ] ), 
		SO_TRACKER_SPEW_OBJECTIVES );
#else
	SO_TRACKER_SPEW( CFmtStr( "Updated points from item.  %dp0 %dp1 %dp2\n", 
							  m_nPoints[ 0 ], 
							  m_nPoints[ 1 ], 
							  m_nPoints[ 2 ] ), 
		SO_TRACKER_SPEW_OBJECTIVES );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const CBaseQuestObjectiveTracker* CQuestItemTracker::FindTrackerForDefIndex( uint32 nDefIndex ) const
{
	FOR_EACH_VEC( m_vecObjectiveTrackers, i )
	{
		if ( m_vecObjectiveTrackers[ i ]->GetObjectiveDefIndex() == nDefIndex )
		{
			return m_vecObjectiveTrackers[ i ];
		}
	}

	return NULL;
}

uint32 CQuestItemTracker::GetEarnedPoints( uint32 eType ) const
{
#ifdef GAME_DLL
	return m_nStartingPoints[ eType ] + m_nPoints[ eType ];
#else
	return m_nPoints[ eType ];
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemTracker::IncrementCount( uint32 nIncrementValue, const QuestObjectiveInstance_t& objective, const CSteamID& steamIDScorer )
{
#if defined( DEBUG ) || defined( STAGING_ONLY )
	if ( !tf_quests_progress_enabled.GetBool() )
		return;

	nIncrementValue = nIncrementValue * tf_quests_points_scale.GetFloat();
#endif


#ifdef GAME_DLL
	const CQuestObjectiveDefinition* pObjective = objective.GetObjectiveDef();

	Assert( pObjective );
	Assert( m_pQuest );
	if ( !pObjective || !m_pQuest )
		return;

	auto pQuestDef = m_pQuest->GetDefinition();
	Assert( pQuestDef );
	if ( !pQuestDef )
		return;

	EQuestPoints eType = objective.GetPointsType();

	if ( g_pVGuiLocalize && ( g_nQuestSpewFlags & SO_TRACKER_SPEW_OBJECTIVES ) )
	{
		locchar_t loc_IntermediateName[ MAX_ITEM_NAME_LENGTH ];
		locchar_t locValue[ MAX_ITEM_NAME_LENGTH ];
		loc_sprintf_safe( locValue, LOCCHAR( "%d" ), objective.GetPoints() );
		loc_scpy_safe( loc_IntermediateName, CConstructLocalizedString( g_pVGuiLocalize->Find( pObjective->GetDescriptionToken() ), locValue ) );
		char szTempObjectiveName[256];
		::ILocalize::ConvertUnicodeToANSI( loc_IntermediateName, szTempObjectiveName, sizeof( szTempObjectiveName ));

		SO_TRACKER_SPEW( CFmtStr( "Increment for quest: %llu Objective: \"%s\" Type: %d, %d->%d (+%d)\n"
			, m_pQuest->GetID()
			, szTempObjectiveName
			, eType
			, m_nPoints[ eType ]
			, m_nPoints[ eType ] + nIncrementValue
			, nIncrementValue ), SO_TRACKER_SPEW_OBJECTIVES );
	}

	bool bAnyChange = false;
	bool bAnyCompleted = false;

	auto lambdaAddToCategory = [&]( EQuestPoints eTypeToAddTo )
	{
		// Regardless of standard or bonus, we fill the standard gauge first
		uint32 nMaxToAdd = pQuestDef->GetMaxPoints( eTypeToAddTo ) - GetEarnedPoints( eTypeToAddTo );
		bAnyCompleted |= ( ( nMaxToAdd > 0 ) && ( nMaxToAdd <= nIncrementValue ) );

		int nAmountToAdd = Min( nMaxToAdd, nIncrementValue );
		m_nPoints[ eTypeToAddTo ] += nAmountToAdd;
		bAnyChange |= nAmountToAdd > 0;

		if ( GetOwnerSteamID() != steamIDScorer )
		{
			// track the assists for sending to the GC later
			int iAssisterIndex = m_mapQuestAssisters.Find( steamIDScorer );
			if ( iAssisterIndex != m_mapQuestAssisters.InvalidIndex() )
			{
				m_mapQuestAssisters[iAssisterIndex] += nAmountToAdd;
			}
		}
	};

	// Add to our own category
	lambdaAddToCategory( eType );

	// The "Advanced" and "Expert" bonus categories fill the "Novice" category as well
	if ( eType == QUEST_POINTS_EXPERT || eType == QUEST_POINTS_ADVANCED )
	{
		lambdaAddToCategory( QUEST_POINTS_NOVICE );
	}

#if defined( DEBUG ) || defined( STAGING_ONLY )
	if ( tf_quests_commit_every_point.GetBool() )
	{
		CommitChangesToDB();
	}
#endif

	// When we're in round-end, commit every point
	bool bInRoundEnd = TFGameRules() && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN;
	if ( bInRoundEnd )
	{
		CommitChangesToDB();
	}

	if ( bAnyChange )
	{
		SendUpdateToClient(pObjective, steamIDScorer);

		CTFPlayer *pTFOwner = ToTFPlayer(GetPlayerBySteamID(GetOwnerSteamID()));
		CTFPlayer *pTFScorer = ToTFPlayer(GetPlayerBySteamID(steamIDScorer));
		if ( pTFOwner && pTFScorer )
		{
			IGameEvent *event = gameeventmanager->CreateEvent("quest_progress");
			if ( event )
			{
				event->SetInt( "owner", pTFOwner->GetUserID());
				event->SetInt( "scorer", pTFScorer->GetUserID());
				event->SetInt( "type", eType);
				event->SetBool( "completed", bAnyCompleted);
				event->SetInt( "quest_defindex", pQuestDef->GetDefIndex() );
				gameeventmanager->FireEvent(event);
			}
		}
	}
	else
	{
		m_bHasUnneededObjectiveTrackers = true;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Remove and delete any objective trackers that are no longer needed.
//			One is considered not needed if it's a tracker for a "standard"
//			objective and we're done getting standard points, or if we're at
//			full bonus points, then there's no way for us to get points anymore
//-----------------------------------------------------------------------------
void CQuestItemTracker::OnUpdate()
{
	PruneUnneededObjectiveTrackers();

#ifdef CLIENT_DLL
	UpdatePointsFromSOItem();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemTracker::OnRemove()
{
#ifdef GAME_DLL
	CommitRecord_t* pRecord = m_pManager->GetCommitRecord( m_pQuest->GetID() );
	if ( pRecord )
	{
		CMsgGCQuestObjective_PointsChange* pProto = assert_cast< CMsgGCQuestObjective_PointsChange* >( pRecord->m_pProtoMsg );
		pProto->set_update_base_points( true );
	}
#endif
}

void CQuestItemTracker::Spew() const 
{
	CBaseSOTracker::Spew();

	FOR_EACH_VEC( m_vecObjectiveTrackers, i )
	{
		DevMsg( "Tracking objective: %d\n", m_vecObjectiveTrackers[ i ]->GetObjectiveDefIndex() );
	}
}

void CQuestItemTracker::FireGameEvent( IGameEvent *event )
{
#ifdef GAME_DLL
	if ( FStrEq( event->GetName(), "show_match_summary" ) ||
		 FStrEq( event->GetName(), "teamplay_round_win" ) )
	{
		// We only want to commit changes on round win and match summary.  The reason being we don't want
		// people to join a match, finish their contract (potentially very quickly), then leave before
		// the match is over.  By only committing on victory, players have to stick around until the end.
		CommitChangesToDB();
	}
	else if ( FStrEq( event->GetName(), "player_disconnect" ) )
	{
		// Make sure the disconnecting guy is the owner (and not a bot)
		if ( event->GetInt( "bot" ) )
			return;

		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByUserId( event->GetInt("userid") ) );
		CSteamID steamIDDisconnecter;

		Assert( pPlayer );
		if ( !pPlayer )
			return;

		pPlayer->GetSteamID( &steamIDDisconnecter );

		if ( steamIDDisconnecter != GetOwnerSteamID() )
			return;

		if ( !TFGameRules()->ShowMatchSummary() )
		{
			// Reset their points if they left outside of the match summary
			memset( m_nPoints, 0, sizeof( m_nPoints ) );

			// reset any assisters that we were tracking
			FOR_EACH_MAP( m_mapQuestAssisters, iAssisterIndex )
			{
				m_mapQuestAssisters[iAssisterIndex] = 0;
			}
		}

		// Remove this guy's objective trackers.  They might not be the owner
		FOR_EACH_VEC_BACK( m_vecObjectiveTrackers, i )
		{
			if ( m_vecObjectiveTrackers[ i ]->GetTrackingPlayer() == steamIDDisconnecter )
			{
				delete m_vecObjectiveTrackers[ i ];
				m_vecObjectiveTrackers.Remove( i );
			}
		}
	}
	else if ( FStrEq( event->GetName(), "player_team" ) )
	{
		//
		// If the guy disconnecting is a party member who is not the owner, we want to
		// remove his objective trackers if the new team is not the owner's team, or we
		// want to add trackers for him if his new team is the owner's team.
		//

		// Get the changing player
		CTFPlayer *pChangingPlayer = ToTFPlayer( UTIL_PlayerByUserId( event->GetInt("userid") ) );
		CSteamID steamIDTeamChanger;
		pChangingPlayer->GetSteamID( &steamIDTeamChanger );

		// Get the owner's party
		CTFParty* pParty = GTFGCClientSystem()->GetPartyForPlayer( GetOwnerSteamID() );
		if ( !pParty )
			return;

		// Check if the changer is in the owner's party (this includes the owner!)
		for( int i=0; i < pParty->GetNumMembers(); ++i )
		{
			if ( pParty->GetMember( i ) == steamIDTeamChanger )
			{
				// A party member changed teams!  Update the objective trackers
				EnsureObjectiveTrackersForParty();
				return;
			}
		}
	}
	else if ( FStrEq( event->GetName(), "player_spawn" ) ) 
	{
		// This is a hack.  The objective calls into us to increment, so we can't delete it then
		// and we don't have a think, but this happens all the time.  Ugh.
		if ( m_bHasUnneededObjectiveTrackers )
		{
			PruneUnneededObjectiveTrackers();
		}
	}
#else
	if ( FStrEq( event->GetName(), "client_disconnect" ) )
	{
		// We just disconnected.  We need to handle the case where we abandoned and still have
		// an inflated score that has our pending points in it.  Pending points are lost if you
		// abandon a match. Calling UpdatePointsFromSOItem() will update our score to what the 
		// SObject (the authority) says it is, but we need to wipe what we think our score is first.
		// If we abandoned, our pending points will not have been commited to the SObject on the GC,
		// so our score will just revert back to what it actually is.
		memset( m_nPoints, 0, sizeof( m_nPoints ) );
		UpdatePointsFromSOItem();
	}
	else if ( FStrEq( event->GetName(), "quest_progress" ) )
	{
		// Let's send a chat message if we've completed an objective
		bool bCompleted = event->GetBool( "completed" );


		if ( bCompleted )
		{
			int nType = event->GetInt( "type" );
			CTFPlayer *pCompletingPlayer = ToTFPlayer( UTIL_PlayerByUserId( event->GetInt( "owner" ) ) );

			CBaseHudChat *hudChat = ( CBaseHudChat * )GET_HUDELEMENT( CHudChat );

			int nQuestDefIndex = event->GetInt( "quest_defindex" );
			const CQuestDefinition *pQuestDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestDefinition >( nQuestDefIndex );

			if ( hudChat && pCompletingPlayer && g_pVGuiLocalize )
			{
				wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
				g_pVGuiLocalize->ConvertANSIToUnicode( pCompletingPlayer->GetPlayerName(), wszPlayerName, sizeof( wszPlayerName ) );

				// Primary objective
				if ( nType == QUEST_POINTS_NOVICE )
				{
					wchar_t wszLocalizedChatMesssage[256];
					g_pVGuiLocalize->ConstructString_safe( wszLocalizedChatMesssage, g_pVGuiLocalize->Find( "QuestReport_ChatNotification_ObjectiveCompleted_Primary" ), 2, wszPlayerName, g_pVGuiLocalize->Find( pQuestDef->GetLocName() ) );

					char szLocalized[256];
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedChatMesssage, szLocalized, sizeof( szLocalized ) );

					hudChat->Printf( CHAT_FILTER_ACHIEVEMENT, "%s", szLocalized );
				}
				// Bonus objective
				else
				{
					wchar_t wszLocalizedChatMesssage[256];
					g_pVGuiLocalize->ConstructString_safe( wszLocalizedChatMesssage, g_pVGuiLocalize->Find( "QuestReport_ChatNotification_ObjectiveCompleted_Bonus" ), 2, wszPlayerName, g_pVGuiLocalize->Find( pQuestDef->GetLocName() ) );

					char szLocalized[256];
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedChatMesssage, szLocalized, sizeof( szLocalized ) );

					hudChat->Printf( CHAT_FILTER_ACHIEVEMENT, "%s", szLocalized );
				}
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CQuestItemTracker::DoesObjectiveNeedToBeTracked( const QuestObjectiveInstance_t& objective ) const
{
	// If the quest isn't activated, then progress cannot be made on it.
	// Don't create any objective trackers in this case.
	if ( m_pQuest->Obj().active() == false )
		return false;

	auto pQuestDef = m_pQuest->GetDefinition();

	Assert( objective.GetObjectiveDef() );
	if ( objective.GetObjectiveDef() && pQuestDef )
	{
		// Check if any of the point types equal to or less than this objective's
		// specified point type still need points.  Meaning, Expert objectives need
		// to be tracked if Expert, Advanced, or Normal points are full
		for( int i = objective.GetPointsType(); i >= 0; --i )
		{
			EQuestPoints eIndex = (EQuestPoints)i;
			if ( pQuestDef->GetMaxPoints( eIndex ) > 0 && GetEarnedPoints( eIndex ) < pQuestDef->GetMaxPoints( eIndex ) )
				return true;
		}
	}

	return false;
}

void CQuestItemTracker::EnsureObjectiveTrackersForPlayer( const CSteamID& steamIDTrackingPlayer )
{
	// Create trackers for each objective
	const QuestObjectiveDefVec_t& vecObjectives = m_pQuest->GetDefinition()->GetObjectives();

	FOR_EACH_VEC( vecObjectives, i )
	{
		if ( !DoesObjectiveNeedToBeTracked( vecObjectives[i] ) )
			continue;

		bool bAlreadyHas = false;
		// Don't double insert!
		FOR_EACH_VEC( m_vecObjectiveTrackers, j )
		{
			const CBaseQuestObjectiveTracker* pExistingTracker = m_vecObjectiveTrackers[ j ];
			if( pExistingTracker->GetObjectiveDefIndex() == vecObjectives[i].GetObjectiveDef()->GetDefIndex() &&
					pExistingTracker->GetTrackingPlayer() == steamIDTrackingPlayer )
				bAlreadyHas = true;
		}

		if ( bAlreadyHas )
			continue;

		CBaseQuestObjectiveTracker* pNewTracker = new CBaseQuestObjectiveTracker( vecObjectives[i], this, steamIDTrackingPlayer );
		m_vecObjectiveTrackers.AddToTail( pNewTracker );

#ifdef GAME_DLL
		if ( GetOwnerSteamID() != steamIDTrackingPlayer )
		{
			// add them to the list of possible assisters if they're not already there
			if ( m_mapQuestAssisters.Find( steamIDTrackingPlayer ) == m_mapQuestAssisters.InvalidIndex() )
			{
				m_mapQuestAssisters.Insert( steamIDTrackingPlayer, 0 );
			}
		}
#endif // GAME_DLL
	}
}

#ifdef GAME_DLL
void CQuestItemTracker::SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent )
{
	// We only care about game server accounts changing.  When they change, we need to make sure
	// that the party Contract preference it has is what we're following
	if ( pObject->GetTypeID() != CEconGameAccountForGameServers::k_nTypeID )
		return;

	UpdatePartyProgressPreference();
}

void CQuestItemTracker::UpdatePartyProgressPreference()
{
	// By default, party members make progress on each others' quests. 
	bool bOnlyMakeTrackersForOwner = false;

	// Check if the owner has opted-out of party quest progress
	auto pOwnerSOCache = GCClientSystem()->GetGCClient()->FindSOCache( GetOwnerSteamID() );
	Assert( pOwnerSOCache );
	if ( pOwnerSOCache )
	{	
		CEconGameAccountForGameServers *pGameAccount = pOwnerSOCache->GetSingleton< CEconGameAccountForGameServers >();
		if ( pGameAccount->Obj().disable_party_quest_progress() )
		{
			bOnlyMakeTrackersForOwner = true;
		}
	}

	// No change in who should track.  Don't need to do anything.
	if ( m_bObjectivesOnlyForOwner == bOnlyMakeTrackersForOwner )
		return;

	m_bObjectivesOnlyForOwner = bOnlyMakeTrackersForOwner;

	EnsureObjectiveTrackersForParty();
}

void CQuestItemTracker::EnsureObjectiveTrackersForParty()
{
	const CMatchInfo::PlayerMatchData_t* pOwnerMatchPlayer = GTFGCClientSystem()->GetLiveMatchPlayer( GetOwnerSteamID() );

	// Users can opt-out of their party making progress on their quests.  This block
	// handles switching that preference mid-round and removes party objective trackers
	if ( m_bObjectivesOnlyForOwner || !pOwnerMatchPlayer )
	{
		//
		// Go through all of the objective trackers and remove any trackers that aren't for
		// the quest owning player
		//
		FOR_EACH_VEC_BACK( m_vecObjectiveTrackers, i )
		{
			if ( m_vecObjectiveTrackers[ i ]->GetTrackingPlayer() != m_steamIDOwner )
			{
				delete m_vecObjectiveTrackers[ i ];
				m_vecObjectiveTrackers.Remove( i );
			}
		}
	}
	else
	{
		// We want party members to be able to help each other with their Contracts.
		// To do this, we want to go through the owner's party and create objective
		// trackers for each party member.  The objective trackers ping this object
		// whenever they detect their conditions are fulfilled, so the Contract owner
		// will get credit.
		CTFParty* pParty = GTFGCClientSystem()->GetPartyForPlayer( GetOwnerSteamID() );
		if ( pParty )
		{
			const auto eOwnerTeam = pOwnerMatchPlayer->eGCTeam;

			for( int i=0; i < pParty->GetNumMembers(); ++i )
			{
				const CSteamID& steamIDPartyMember = pParty->GetMember( i );
				// We don't want to create/destroy the owner's tracker.  Theirs is
				// created in the constructor of this quest tracker.
				if ( GetOwnerSteamID() == steamIDPartyMember )
					continue;

				const CMatchInfo::PlayerMatchData_t* pPartyMemberMatchPlayer = GTFGCClientSystem()->GetLiveMatchPlayer( steamIDPartyMember );
				// They might not be here, and that's ok.  
				if ( pPartyMemberMatchPlayer && pPartyMemberMatchPlayer->eGCTeam == eOwnerTeam )
				{
					// Same team!  Make sure we have trackers for them
					EnsureObjectiveTrackersForPlayer( steamIDPartyMember );
				}
				else
				{
					// Different teams, or they've left!  Make sure we DONT have trackers for them
					FOR_EACH_VEC_BACK( m_vecObjectiveTrackers, j )
					{
						if ( m_vecObjectiveTrackers[ j ]->GetTrackingPlayer() == steamIDPartyMember )
						{
							delete m_vecObjectiveTrackers[ j ];
							m_vecObjectiveTrackers.Remove( j );
						}
					}
				}
			}
		}
	}
}
#endif

void CQuestItemTracker::PruneUnneededObjectiveTrackers()
{
#ifdef GAME_DLL
	m_bHasUnneededObjectiveTrackers = false;
#endif

	FOR_EACH_VEC_BACK( m_vecObjectiveTrackers, i )
	{
		if ( !DoesObjectiveNeedToBeTracked( m_vecObjectiveTrackers[ i ]->GetObjectiveInstance() ) )
		{
			delete m_vecObjectiveTrackers[ i ];
			m_vecObjectiveTrackers.Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestItemTracker::CommitChangesToDB()
{
#ifdef GAME_DLL

	bool bAnyProgress = false;
	for( int i=0; i < EQuestPoints_ARRAYSIZE; ++i )
	{
		bAnyProgress |= m_nPoints[ i ] != 0;
	}

	if ( !bAnyProgress )
		return;

	SO_TRACKER_SPEW( CFmtStr( "CommitChangesToDB: %llu %dp0 %dp1 %dp2\n"
			, m_pQuest->GetID()
			, GetEarnedPoints( (EQuestPoints)0 )
			, GetEarnedPoints( (EQuestPoints)1 )
			, GetEarnedPoints( (EQuestPoints)2 ) )
			, 0 );
	
	CSteamID ownerSteamID( m_pQuest->Obj().account_id(), GetUniverse(), k_EAccountTypeIndividual );

	CMsgGCQuestObjective_PointsChange record;

	// Cook up our message.  This is where we actually update progress on the GC
	record.set_owner_steamid( ownerSteamID.ConvertToUint64() );
	record.set_quest_id( m_pQuest->GetID() );
	record.set_points_0( GetEarnedPoints( 0 ) );
	record.set_points_1( GetEarnedPoints( 1 ) );
	record.set_points_2( GetEarnedPoints( 2 ) );

	m_pManager->AddCommitRecord( &record, record.quest_id(), true );

	// reward any assisters
	FOR_EACH_MAP( m_mapQuestAssisters, iAssisterIndex )
	{
		int nIncrementValue = m_mapQuestAssisters[iAssisterIndex];
		m_mapQuestAssisters[iAssisterIndex] = 0; // reset this but leave them in the list for the next round, map, etc.
		if ( nIncrementValue > 0 )
		{
			GCSDK::CProtoBufMsg< GCQuestStrangeEvent > msg( k_EMsgGCQuestStrangeEvent );
			msg.Body().set_owner_account_id( ownerSteamID.GetAccountID() );
			msg.Body().set_scorer_account_id( m_mapQuestAssisters.Key( iAssisterIndex ).GetAccountID() );
			msg.Body().set_quest_id( m_pQuest->GetID() );
			msg.Body().set_strange_event_id( kKillEaterEvent_ContractPointsContributedToFriends );
			msg.Body().set_score( nIncrementValue );
			GCClientSystem()->BSendMessage( msg );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CQuestItemTracker::GetNumInactiveObjectives( const CTFPlayer *pOwner, InvalidReasonsContainer_t& invalidReasons ) const
{
	int nNumInvalid = 0;
	FOR_EACH_VEC( m_vecObjectiveTrackers, i )
	{
		m_vecObjectiveTrackers[ i ]->IsValidForPlayer( pOwner, invalidReasons );

		if ( !invalidReasons.IsValid() )
		{
			++nNumInvalid;
		}
	}

	return nNumInvalid;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: The server has changed scores.  Apply those changes here
//-----------------------------------------------------------------------------
void CQuestItemTracker::UpdateFromServer( uint32 nPoints0, uint32 nPoints1, uint32 nPoints2 )
{
	SO_TRACKER_SPEW( CFmtStr( "Updating \"%s's\" %dp0->%dp0 %dp1->%dp1 %dp2->%dp2\n"
							  , m_pQuest->GetDefinition()->GetLocName()
							  , m_nPoints[ 0 ]
							  , nPoints0
							  , m_nPoints[ 1 ]
							  , nPoints1
							  , m_nPoints[ 2 ]
							  , nPoints2 )
					, SO_TRACKER_SPEW_OBJECTIVES );

	m_nPoints[ 0 ] = nPoints0;
	m_nPoints[ 1 ] = nPoints1;
	m_nPoints[ 2 ] = nPoints2;
}
#else
void CQuestItemTracker::SendUpdateToClient( const CQuestObjectiveDefinition* pObjective, const CSteamID& steamIDScorer )
{
	const CTFPlayer* pPlayer = GetTrackedPlayer();

	const CBasePlayer* pScorer = GetPlayerBySteamID( steamIDScorer );
	
	int nScorerUserID = 0;
	if ( pScorer )
	{
		nScorerUserID = pScorer->GetUserID();
	}

	// They might've disconnected, so let's check if they're still around
	if ( pPlayer )
	{
		// We're writing bytes below!
		Assert( GetEarnedPoints( (EQuestPoints)0 ) < ( 256 ) );
		Assert( GetEarnedPoints( (EQuestPoints)1 ) < ( 256 ) );
		Assert( GetEarnedPoints( (EQuestPoints)2 ) < ( 256 ) );
		Assert( nScorerUserID < 256 );

		// Update the user on their progress
		CSingleUserRecipientFilter filter( GetTrackedPlayer() );
		filter.MakeReliable();
		UserMessageBegin( filter, "QuestObjectiveCompleted" );
		itemid_t nID = m_pQuest->GetID();
		WRITE_BITS( &nID, 64 );
		WRITE_BYTE( GetEarnedPoints( (EQuestPoints)0 ) );
		WRITE_BYTE( GetEarnedPoints( (EQuestPoints)1 ) );
		WRITE_BYTE( GetEarnedPoints( (EQuestPoints)2 ) );
		WRITE_WORD( pObjective ? pObjective->GetDefIndex() : (uint32)-1 );
		WRITE_BYTE( nScorerUserID );
		MessageEnd();
	}
}
#endif
