//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_wearable_campaign_item.h"

#ifdef GAME_DLL
#include "econ_quests.h"
#include "quest_objective_manager.h"
#include "tf_quest_map_utils.h"
#endif // GAME_DLL


LINK_ENTITY_TO_CLASS( tf_wearable_campaign_item, CTFWearableCampaignItem );
PRECACHE_REGISTER( tf_wearable_campaign_item );

IMPLEMENT_NETWORKCLASS_ALIASED( TFWearableCampaignItem, DT_TFWearableCampaignItem )

BEGIN_NETWORK_TABLE( CTFWearableCampaignItem, DT_TFWearableCampaignItem )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_nState ) ),
#else
	SendPropInt( SENDINFO( m_nState ) ),
#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFWearableCampaignItem )
END_DATADESC()


#define WEARABLE_PARTICLE_SCORE					"pyrovision_explosion"				
#define WEARABLE_PARTICLE_COMPLETED				"pyrovision_explosion"

struct wearable_campaign_item_state_info_t
{
	wearable_state_t m_nState;
	wearable_skin_offset_t m_nSkinOffset;
	const char *m_pszSoundEffect;
	const char *m_pszParticleEffect;
};

static wearable_campaign_item_state_info_t s_WearableStateInfos[] =
{
	{ WEARABLE_STATE_STATIC,				WEARABLE_SKIN_STATIC,		"",											"" },
	{ WEARABLE_STATE_OFF,					WEARABLE_SKIN_OFF,			"",											"" },
	{ WEARABLE_STATE_IDLE,					WEARABLE_SKIN_GRID,			"",											"" },
	{ WEARABLE_STATE_SCORE_NOVICE,			WEARABLE_SKIN_FLASH,		"Quest.StatusTickNovicePDA",				"contract_score_primary" },
	{ WEARABLE_STATE_SCORE_ADVANCED,		WEARABLE_SKIN_FLASH,		"Quest.StatusTickAdvancedPDA",				"contract_score_bonus" },
	{ WEARABLE_STATE_SCORE_EXPERT,			WEARABLE_SKIN_FLASH,		"Quest.StatusTickExpertPDA",				"contract_score_bonus" },
	{ WEARABLE_STATE_SCORE_ASSIST_NOVICE,	WEARABLE_SKIN_FLASH,		"Quest.StatusTickNovicePDA",				"contract_score_primary" },
	{ WEARABLE_STATE_SCORE_ASSIST_ADVANCED,	WEARABLE_SKIN_FLASH,		"Quest.StatusTickAdvancedPDA",				"contract_score_bonus" },
	{ WEARABLE_STATE_SCORE_ASSIST_EXPERT,	WEARABLE_SKIN_FLASH,		"Quest.StatusTickExpertPDA",				"contract_score_bonus" },
	{ WEARABLE_STATE_COMPLETED_NOVICE,		WEARABLE_SKIN_FLASH,		"Quest.StatusTickNoviceCompletePDA",		"contract_completed_primary" },
	{ WEARABLE_STATE_COMPLETED_ADVANCED,	WEARABLE_SKIN_FLASH,		"Quest.StatusTickAdvancedCompletePDA",		"contract_completed_bonus" },
	{ WEARABLE_STATE_COMPLETED_EXPERT,		WEARABLE_SKIN_FLASH,		"Quest.StatusTickExpertCompletePDA",		"contract_completed_bonus" },
};
COMPILE_TIME_ASSERT( ARRAYSIZE( s_WearableStateInfos ) == WEARABLE_NUM_STATES );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWearableCampaignItem::CTFWearableCampaignItem()
{
	m_nState = WEARABLE_STATE_OFF;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWearableCampaignItem::~CTFWearableCampaignItem()
{
#ifdef GAME_DLL
	UpdateListenerStatus( false );
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWearableCampaignItem::Precache()
{
	BaseClass::Precache();

	for ( int i = 0; i < ARRAYSIZE( s_WearableStateInfos ); ++i )
	{
		if ( s_WearableStateInfos[i].m_pszSoundEffect && s_WearableStateInfos[i].m_pszSoundEffect[0] )
		{
			PrecacheScriptSound( s_WearableStateInfos[i].m_pszSoundEffect );
		}

		if ( s_WearableStateInfos[i].m_pszParticleEffect && s_WearableStateInfos[i].m_pszParticleEffect[0] )
		{
			PrecacheParticleSystem( s_WearableStateInfos[i].m_pszParticleEffect );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWearableCampaignItem::GetSkin()
{
	int nRetVal = s_WearableStateInfos[m_nState].m_nSkinOffset;

	CEconItemView *pItem = ( GetAttributeContainer() ? GetAttributeContainer()->GetItem() : NULL );
	if ( pItem )
	{
		nRetVal += ( pItem->GetStyle() * WEARABLE_NUM_SKINS );
	}

	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableCampaignItem::Equip( CBasePlayer *pOwner )
{
	BaseClass::Equip( pOwner );

#ifdef GAME_DLL
	if ( pOwner )
	{
		pOwner->GetSteamID( &m_steamIDOwner );
		m_hOwner = ToTFPlayer( pOwner );

		UpdateListenerStatus( true );
		EvaluateState();
	}
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableCampaignItem::UnEquip( CBasePlayer *pOwner )
{
	BaseClass::UnEquip( pOwner );

#ifdef GAME_DLL
	UpdateListenerStatus( false );
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableCampaignItem::FireGameEvent( IGameEvent *event )
{
#ifdef GAME_DLL
	const char *pszEvent = event->GetName();

	if ( FStrEq( pszEvent, "quest_progress" ) )
	{
		// is this message about us?
		if ( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() )
		{
			int nOwnerUserID = event->GetInt( "owner" );
			int nType = event->GetInt( "type" );
			CBasePlayer *pOwner = dynamic_cast< CBasePlayer * >( GetOwnerEntity() );
			if ( pOwner && ( pOwner->GetUserID() == nOwnerUserID ) )
			{
				bool bAssist = ( nOwnerUserID != event->GetInt( "scorer" ) );
				bool bCompleted = event->GetBool( "completed" );

				switch ( nType )
				{
				default:
				case QUEST_POINTS_NOVICE:
					m_nState = ( bCompleted ? WEARABLE_STATE_COMPLETED_NOVICE : ( bAssist ? WEARABLE_STATE_SCORE_ASSIST_NOVICE : WEARABLE_STATE_SCORE_NOVICE ) );
					break;
				case QUEST_POINTS_ADVANCED:
					m_nState = ( bCompleted ? WEARABLE_STATE_COMPLETED_ADVANCED : ( bAssist ? WEARABLE_STATE_SCORE_ASSIST_ADVANCED : WEARABLE_STATE_SCORE_ADVANCED ) );
					break;
				case QUEST_POINTS_EXPERT:
					m_nState = ( bCompleted ? WEARABLE_STATE_COMPLETED_EXPERT : ( bAssist ? WEARABLE_STATE_SCORE_ASSIST_EXPERT : WEARABLE_STATE_SCORE_EXPERT ) );
					break;
				}

				// add some delay before we EvaluateState again so the "score" state will be visible and then turn off
				SetContextThink( &CTFWearableCampaignItem::EvaluateState, gpGlobals->curtime + 1.f, "WearableEvaluateState" );

				// tell client to add the effects
				EntityMessageBegin( this, true );
					WRITE_BYTE( m_nState );
				MessageEnd();
			}
		}
	}
	else if ( FStrEq( pszEvent, "player_spawn" )
		   || FStrEq( pszEvent, "inventory_updated" )
		   || FStrEq( pszEvent, "localplayer_changeclass" )
		   || FStrEq( pszEvent, "schema_updated" )
		   || FStrEq( pszEvent, "client_disconnect" ) )
	{
		if ( m_nState <= WEARABLE_STATE_IDLE )
		{
			EvaluateState();
		}
	}
#endif // GAME_DLL
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWearableCampaignItem::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableCampaignItem::SOEventInternal( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent )
{
	// if we're currently in state "score" don't do anything yet
	if ( m_nState > WEARABLE_STATE_IDLE )
		return;

	Assert( steamIDOwner == m_steamIDOwner );
	if ( steamIDOwner != m_steamIDOwner )
		return;

	if ( pObject->GetTypeID() == k_EEConTypeQuest )
	{
		EvaluateState();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableCampaignItem::EvaluateState( void )
{
	CQuestMapHelper questMapHelper( m_steamIDOwner );

	const CQuest* pQuest = questMapHelper.GetActiveQuest();
	bool bActiveQuest = false;

	if ( pQuest && m_hOwner.Get() )
	{
		const CQuestItemTracker* pItemTracker = QuestObjectiveManager()->GetTypedTracker< CQuestItemTracker* >( pQuest->GetID() );
		InvalidReasonsContainer_t invalidReasons;
		if ( pItemTracker )
		{
			if ( pItemTracker->GetNumInactiveObjectives( m_hOwner.Get(), invalidReasons ) != pItemTracker->GetObjectiveTrackers().Count() )
			{
				bActiveQuest = true;
			}
		}
	}

	m_nState = ( bActiveQuest ? WEARABLE_STATE_IDLE : WEARABLE_STATE_OFF );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableCampaignItem::UpdateListenerStatus( bool bShouldListen )
{
	// SOCache events
	if ( m_steamIDOwner.IsValid() && GCClientSystem() && GCClientSystem()->GetGCClient() )
	{
		auto pSOCache = GCClientSystem()->GetGCClient()->FindSOCache( m_steamIDOwner );
		if ( pSOCache )
		{
			if ( bShouldListen )
			{
				pSOCache->AddListener( this );
			}
			else
			{
				pSOCache->RemoveListener( this );
			}
		}
	}

	// game events
	if ( bShouldListen )
	{
		ListenForGameEvent( "quest_progress" );
		ListenForGameEvent( "player_spawn" );
		ListenForGameEvent( "inventory_updated" );
		ListenForGameEvent( "localplayer_changeclass" );
		ListenForGameEvent( "schema_updated" );
		ListenForGameEvent( "client_disconnect" );
	}
	else
	{
		StopListeningForAllEvents();
	}
}

#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableCampaignItem::HandleStateEffects( int nState )
{
	if ( ( nState >= 0 ) && ( nState < WEARABLE_NUM_STATES ) )
	{
		// the sound is already played through the HUD for the local player
		if ( GetOwnerEntity() != CBasePlayer::GetLocalPlayer() )
		{
			if ( s_WearableStateInfos[nState].m_pszSoundEffect && s_WearableStateInfos[nState].m_pszSoundEffect[0] )
			{
				EmitSound( s_WearableStateInfos[nState].m_pszSoundEffect );
			}
		}

		if ( s_WearableStateInfos[nState].m_pszParticleEffect && s_WearableStateInfos[nState].m_pszParticleEffect[0] )
		{
			ParticleProp()->Create( s_WearableStateInfos[nState].m_pszParticleEffect, PATTACH_POINT_FOLLOW, "particle" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearableCampaignItem::ReceiveMessage( int classID, bf_read &msg )
{
	if ( classID != GetClientClass()->m_ClassID )
	{
		// message is for subclass
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}

	HandleStateEffects( msg.ReadByte() );
}
#endif // GAME_DLL
