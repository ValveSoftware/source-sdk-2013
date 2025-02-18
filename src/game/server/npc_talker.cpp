//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"

#include "npc_talker.h"
#include "npcevent.h"
#include "scriptevent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


	
BEGIN_SIMPLE_DATADESC( CNPCSimpleTalkerExpresser )
	//									m_pSink		(reconnected on load)
	DEFINE_AUTO_ARRAY(	m_szMonologSentence,	FIELD_CHARACTER	),
	DEFINE_FIELD(		m_iMonologIndex,		FIELD_INTEGER	),
	DEFINE_FIELD(		m_fMonologSuspended,	FIELD_BOOLEAN	),
	DEFINE_FIELD(		m_hMonologTalkTarget,	FIELD_EHANDLE	),
END_DATADESC()

BEGIN_DATADESC( CNPCSimpleTalker )
	DEFINE_FIELD( m_useTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextIdleSpeechTime, FIELD_TIME ),
	DEFINE_FIELD( m_nSpeak, FIELD_INTEGER ),
	DEFINE_FIELD( m_iszUse, FIELD_STRING ),
	DEFINE_FIELD( m_iszUnUse, FIELD_STRING ),
	// 							m_FollowBehavior (auto saved by AI)
	// Function Pointers
	DEFINE_USEFUNC( FollowerUse ),

END_DATADESC()

// array of friend names
char *CNPCSimpleTalker::m_szFriends[TLK_CFRIENDS] = 
{
	"NPC_barney",
	"NPC_scientist",
	"NPC_sitting_scientist",
	NULL,
};

bool CNPCSimpleTalker::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "UseSentence"))
	{
		m_iszUse = AllocPooledString(szValue);
	}
	else if (FStrEq(szKeyName, "UnUseSentence"))
	{
		m_iszUnUse = AllocPooledString(szValue);
	}
	else 
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

void CNPCSimpleTalker::Precache( void )
{
	/*
	// FIXME:  Need to figure out how to hook these...
	if ( m_iszUse != NULL_STRING )
		GetExpresser()->ModifyConcept( TLK_STARTFOLLOW, STRING( m_iszUse ) );
	if ( m_iszUnUse != NULL_STRING )
		GetExpresser()->ModifyConcept( TLK_STOPFOLLOW, STRING( m_iszUnUse ) );

	*/
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPCSimpleTalker::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	// Assume that if I move from the player, I can respond to a question
	if ( ConditionInterruptsCurSchedule( COND_PLAYER_PUSHING ) || ConditionInterruptsCurSchedule( COND_PROVOKED ) )
	{
		SetCustomInterruptCondition( COND_TALKER_RESPOND_TO_QUESTION );
	}
}

void CNPCSimpleTalker::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	(assert_cast<CNPCSimpleTalkerExpresser *>(GetExpresser()))->SpeakMonolog();
}

bool CNPCSimpleTalker::ShouldSuspendMonolog( void )
{
	float flDist;

	flDist = ((assert_cast<CNPCSimpleTalkerExpresser *>(GetExpresser()))->GetMonologueTarget()->GetAbsOrigin() - GetAbsOrigin()).Length();
	
	if( flDist >= 384 )
	{
		return true;
	}

	return false;
}

bool CNPCSimpleTalker::ShouldResumeMonolog( void )
{
	float flDist;

	if( HasCondition( COND_SEE_PLAYER ) )
	{
		flDist = ((assert_cast<CNPCSimpleTalkerExpresser *>(GetExpresser()))->GetMonologueTarget()->GetAbsOrigin() - GetAbsOrigin()).Length();
		
		if( flDist <= 256 )
		{
			return true;
		}
	}

	return false;
}

int CNPCSimpleTalker::SelectSchedule( void )
{
	if ( !HasCondition(COND_RECEIVED_ORDERS) )
	{
		if ( GetState() == NPC_STATE_IDLE )
		{
			// if never seen player, try to greet him
			// Filter might be preventing us from ever greeting the player
			if ( HasCondition( COND_SEE_PLAYER ) && CanSayHello())
			{
				return SCHED_TALKER_IDLE_HELLO;
			}
		}
	}

	return BaseClass::SelectSchedule();
}

void CNPCSimpleTalker::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_TALKER_WAIT_FOR_SEMAPHORE:
		if ( GetExpresser()->SemaphoreIsAvailable( this ) )
			TaskComplete();
		break;

	case TASK_TALKER_SPEAK:
		// ask question or make statement
		FIdleSpeak();
		TaskComplete();
		break;

	case TASK_TALKER_RESPOND:
		// respond to question
		IdleRespond();
		TaskComplete();
		break;

	case TASK_TALKER_HELLO:
		// greet player
		FIdleHello();
		TaskComplete();
		break;
	
	case TASK_TALKER_STARE:
		// let the player know I know he's staring at me.
		FIdleStare();
		TaskComplete();
		break;

	case TASK_TALKER_LOOK_AT_CLIENT:
	case TASK_TALKER_CLIENT_STARE:
		// track head to the client for a while.
		SetWait( pTask->flTaskData );
		break;

	case TASK_TALKER_EYECONTACT:
		break;

	case TASK_TALKER_IDEALYAW:
		if (GetSpeechTarget() != NULL)
		{
			GetMotor()->SetIdealYawToTarget( GetSpeechTarget()->GetAbsOrigin() );
		}
		TaskComplete();
		break;

	case TASK_TALKER_HEADRESET:
		// reset head position after looking at something
		SetSpeechTarget( NULL );
		TaskComplete();
		break;

	case TASK_TALKER_BETRAYED:
		Speak( TLK_BETRAYED );
		TaskComplete();
		break;

	case TASK_TALKER_STOPSHOOTING:
		// tell player to stop shooting
		Speak( TLK_NOSHOOT );
		TaskComplete();
		break;
	default:
		BaseClass::StartTask( pTask );
	}
}

void CNPCSimpleTalker::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_TALKER_WAIT_FOR_SEMAPHORE:
		if ( GetExpresser()->SemaphoreIsAvailable( this ) )
			TaskComplete();
		break;

	case TASK_TALKER_CLIENT_STARE:
	case TASK_TALKER_LOOK_AT_CLIENT:

		if ( pTask->iTask == TASK_TALKER_CLIENT_STARE && AI_IsSinglePlayer() )
		{
			// Get edict for one player
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
			Assert( pPlayer );

			// fail out if the player looks away or moves away.
			if ( ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).Length2D() > TALKER_STARE_DIST )
			{
				// player moved away.
				TaskFail("Player moved away");
			}

			Vector forward;
			AngleVectors( pPlayer->GetLocalAngles(), &forward );
			if ( UTIL_DotPoints( pPlayer->GetAbsOrigin(), GetAbsOrigin(), forward ) < m_flFieldOfView )
			{
				// player looked away
				TaskFail("Player looked away");
			}
		}

		if ( IsWaitFinished() )
		{
			TaskComplete();
		}
		break;

	case TASK_TALKER_EYECONTACT:
		if (IsMoving() || !GetExpresser()->IsSpeaking() || GetSpeechTarget() == NULL)
		{
			TaskComplete();
		}
		break;

	case TASK_WAIT_FOR_MOVEMENT:
		FIdleSpeakWhileMoving();
		BaseClass::RunTask( pTask );
		break;

	default:
		BaseClass::RunTask( pTask );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------

Activity CNPCSimpleTalker::NPC_TranslateActivity( Activity eNewActivity )
{
	if ((eNewActivity == ACT_IDLE)										&& 
		(GetExpresser()->IsSpeaking())										&&
		(SelectWeightedSequence ( ACT_SIGNAL3 ) != ACTIVITY_NOT_AVAILABLE)	)
	{
		return ACT_SIGNAL3;
	}
	else if ((eNewActivity == ACT_SIGNAL3)									&& 
			 (SelectWeightedSequence ( ACT_SIGNAL3 ) == ACTIVITY_NOT_AVAILABLE)	)
	{
		return ACT_IDLE;
	}
	return BaseClass::NPC_TranslateActivity( eNewActivity );
}


void CNPCSimpleTalker::Event_Killed( const CTakeDamageInfo &info )
{
	AlertFriends( info.GetAttacker() );
	if ( info.GetAttacker()->GetFlags() & FL_CLIENT )
	{
		LimitFollowers( info.GetAttacker(), 0 );
	}
	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity	*CNPCSimpleTalker::EnumFriends( CBaseEntity *pPrevious, int listNumber, bool bTrace )
{
	CBaseEntity *pFriend = pPrevious;
	char *pszFriend;
	trace_t tr;
	Vector vecCheck;

	pszFriend = m_szFriends[ FriendNumber(listNumber) ];
	while ( pszFriend != NULL && ((pFriend = gEntList.FindEntityByClassname( pFriend, pszFriend )) != NULL) )
	{
		if (pFriend == this || !pFriend->IsAlive())
			// don't talk to self or dead people
			continue;

		if ( bTrace )
		{
			Vector vecCheck;
			pFriend->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 1.0f ), &vecCheck );
			UTIL_TraceLine( GetAbsOrigin(), vecCheck, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);
		}
		else
		{
			tr.fraction = 1.0;
		}

		if (tr.fraction == 1.0)
		{
			return pFriend;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pKiller - 
//-----------------------------------------------------------------------------
void CNPCSimpleTalker::AlertFriends( CBaseEntity *pKiller )
{
	CBaseEntity *pFriend = NULL;
	int i;

	// for each friend in this bsp...
	for ( i = 0; i < TLK_CFRIENDS; i++ )
	{
		while ((pFriend = EnumFriends( pFriend, i, true )) != NULL )
		{
			CAI_BaseNPC *pNPC = pFriend->MyNPCPointer();
			if ( pNPC->IsAlive() )
			{
				// If a client killed me, make everyone else mad/afraid of him
				if ( pKiller->GetFlags() & FL_CLIENT )
				{
					CNPCSimpleTalker*pTalkNPC = (CNPCSimpleTalker *)pFriend;

					if (pTalkNPC && pTalkNPC->IsOkToCombatSpeak())
					{
						// FIXME: need to check CanSpeakConcept?
						pTalkNPC->Speak( TLK_BETRAYED );
					}
				}
				else
				{
					if( IRelationType(pKiller) == D_HT)
					{
						// Killed by an enemy!!!
						CNPCSimpleTalker *pAlly = (CNPCSimpleTalker *)pNPC;
						
						if( pAlly && pAlly->GetExpresser()->CanSpeakConcept( TLK_ALLY_KILLED ) )
						{
							pAlly->Speak( TLK_ALLY_KILLED );
						}
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCSimpleTalker::ShutUpFriends( void )
{
	CBaseEntity *pFriend = NULL;
	int i;

	// for each friend in this bsp...
	for ( i = 0; i < TLK_CFRIENDS; i++ )
	{
		while ((pFriend = EnumFriends( pFriend, i, true )) != NULL)
		{
			CAI_BaseNPC *pNPC = pFriend->MyNPCPointer();
			if ( pNPC )
			{
				pNPC->SentenceStop();
			}
		}
	}
}


// UNDONE: Keep a follow time in each follower, make a list of followers in this function and do LRU
// UNDONE: Check this in Restore to keep restored NPCs from joining a full list of followers
void CNPCSimpleTalker::LimitFollowers( CBaseEntity *pPlayer, int maxFollowers )
{
	CBaseEntity *pFriend = NULL;
	int i, count;

	count = 0;
	// for each friend in this bsp...
	for ( i = 0; i < TLK_CFRIENDS; i++ )
	{
		while ((pFriend = EnumFriends( pFriend, i, false )) != NULL)
		{
			CAI_BaseNPC *pNPC = pFriend->MyNPCPointer();
			CNPCSimpleTalker *pTalker;
			if ( pNPC )
			{
				if ( pNPC->GetTarget() == pPlayer )
				{
					count++;
					if ( count > maxFollowers && (pTalker = dynamic_cast<CNPCSimpleTalker *>( pNPC ) ) != NULL )
						pTalker->StopFollowing();
				}
			}
		}
	}
}

//=========================================================
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPCSimpleTalker::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{		
	case SCRIPT_EVENT_SENTENCE_RND1:		// Play a named sentence group 25% of the time
		if (random->RandomInt(0,99) < 75)
			break;
		// fall through...
	case SCRIPT_EVENT_SENTENCE:				// Play a named sentence group
		ShutUpFriends();
		PlaySentence( pEvent->options, random->RandomFloat(2.8, 3.4) );
		//Msg( "script event speak\n");
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Scan for nearest, visible friend. If fPlayer is true, look for nearest player
//-----------------------------------------------------------------------------
bool CNPCSimpleTalker::IsValidSpeechTarget( int flags, CBaseEntity *pEntity )
{
	return BaseClass::IsValidSpeechTarget( flags, pEntity );
}

CBaseEntity *CNPCSimpleTalker::FindNearestFriend(bool fPlayer)
{
	return FindSpeechTarget( (fPlayer) ? AIST_PLAYERS : AIST_NPCS );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: Respond to a previous question
//-----------------------------------------------------------------------------
void CNPCSimpleTalker::IdleRespond( void )
{
	if (!IsOkToSpeak())
		return;

	// play response
	SpeakAnswerFriend( GetSpeechTarget() );

	DeferAllIdleSpeech( random->RandomFloat( TALKER_DEFER_IDLE_SPEAK_MIN, TALKER_DEFER_IDLE_SPEAK_MAX ) );
}

bool CNPCSimpleTalker::IsOkToSpeak( void )
{
	if ( m_flNextIdleSpeechTime > gpGlobals->curtime )
		return false;

	return BaseClass::IsOkToSpeak();
}


//-----------------------------------------------------------------------------
// Purpose: Find a nearby friend to stare at
//-----------------------------------------------------------------------------
int CNPCSimpleTalker::FIdleStare( void )
{
	// Don't idly speak if our speech filter is preventing us
	if ( GetSpeechFilter() && GetSpeechFilter()->GetIdleModifier() == 0 )
		return true;

	SpeakIfAllowed( TLK_STARE );

	SetSpeechTarget( FindNearestFriend( true ) );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Try to greet player first time he's seen
// Output : int
//-----------------------------------------------------------------------------
int CNPCSimpleTalker::FIdleHello( void )
{
	// Filter might be preventing us from ever greeting the player
	if ( !CanSayHello() )
		return false;

	// get a player
	CBaseEntity *pPlayer = FindNearestFriend(true);

	if (pPlayer)
	{
		if (FInViewCone(pPlayer) && FVisible(pPlayer))
		{
			SayHelloToPlayer( pPlayer );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Say hello to the specified player
//-----------------------------------------------------------------------------
void CNPCSimpleTalker::SayHelloToPlayer( CBaseEntity *pPlayer )
{
	Assert( !GetExpresser()->SpokeConcept(TLK_HELLO) );

	SetSpeechTarget( pPlayer );

	Speak( TLK_HELLO );
	DeferAllIdleSpeech( random->RandomFloat( 5, 10 ) );

	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	CAI_PlayerAlly *pTalker;
	for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
	{
		pTalker = dynamic_cast<CAI_PlayerAlly *>(ppAIs[i]);

		if( pTalker && FVisible( pTalker ) )
		{
			// Tell this guy he's already said hello to the player, too.
			pTalker->GetExpresser()->SetSpokeConcept( TLK_HELLO, NULL );
		}
	}
}


//---------------------------------------------------------
// Stop all allies from idle speech for a fixed amount
// of time. Mostly filthy hack to hold us over until
// acting comes online.
//---------------------------------------------------------
void CNPCSimpleTalker::DeferAllIdleSpeech( float flDelay, CAI_BaseNPC *pIgnore )
{
	// Brute force. Just plow through NPC list looking for talkers.
	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	CNPCSimpleTalker *pTalker;

	float flTime = gpGlobals->curtime + flDelay;

	for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
	{
		if( ppAIs[i] != pIgnore )
		{
			pTalker = dynamic_cast<CNPCSimpleTalker *>(ppAIs[i]);

			if( pTalker )
			{
				pTalker->m_flNextIdleSpeechTime = flTime;
			}
		}
	}

	BaseClass::DeferAllIdleSpeech( flDelay, pIgnore );
}

//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
int CNPCSimpleTalker::FIdleSpeak( void )
{ 
	// try to start a conversation, or make statement
	int pitch;

	if (!IsOkToSpeak())
		return false;

	Assert( GetExpresser()->SemaphoreIsAvailable( this ) );
	
	pitch = GetExpresser()->GetVoicePitch();
		
	// player using this entity is alive and wounded?
	CBaseEntity *pTarget = GetTarget();

	if ( pTarget != NULL )
	{
		if ( pTarget->IsPlayer() )
		{
			if ( pTarget->IsAlive() )
			{
				SetSpeechTarget( GetTarget() );
				if (GetExpresser()->CanSpeakConcept( TLK_PLHURT3) && 
					(GetTarget()->m_iHealth <= GetTarget()->m_iMaxHealth / 8))
				{
					Speak( TLK_PLHURT3 );
					return true;
				}
				else if (GetExpresser()->CanSpeakConcept( TLK_PLHURT2) && 
					(GetTarget()->m_iHealth <= GetTarget()->m_iMaxHealth / 4))
				{
					Speak( TLK_PLHURT2 );
					return true;
				}
				else if (GetExpresser()->CanSpeakConcept( TLK_PLHURT1) &&
					(GetTarget()->m_iHealth <= GetTarget()->m_iMaxHealth / 2))
				{
					Speak( TLK_PLHURT1 );
					return true;
				}
			}
			else
			{
				//!!!KELLY - here's a cool spot to have the talkNPC talk about the dead player if we want.
				// "Oh dear, Gordon Freeman is dead!" -Scientist
				// "Damn, I can't do this without you." -Barney
			}
		}
	}

	// ROBIN: Disabled idle question & answer for now
	/*
	// if there is a friend nearby to speak to, play sentence, set friend's response time, return
	CBaseEntity *pFriend = FindNearestFriend(false);

	// 75% chance of talking to another citizen if one is available.
	if (pFriend && !(pFriend->IsMoving()) && random->RandomInt( 0, 3 ) != 0 )
	{
		if ( SpeakQuestionFriend( pFriend ) )
		{
			// force friend to answer
			CAI_PlayerAlly *pTalkNPC = dynamic_cast<CAI_PlayerAlly *>(pFriend);
			if (pTalkNPC && !pTalkNPC->HasSpawnFlags(SF_NPC_GAG) && !pTalkNPC->IsInAScript() )
			{
				SetSpeechTarget( pFriend );
				pTalkNPC->SetAnswerQuestion( this );
				pTalkNPC->GetExpresser()->BlockSpeechUntil( GetExpresser()->GetTimeSpeechComplete() );

				m_nSpeak++;
			}

			// Don't let anyone else butt in.
			DeferAllIdleSpeech( random->RandomFloat( TALKER_DEFER_IDLE_SPEAK_MIN, TALKER_DEFER_IDLE_SPEAK_MAX ), pTalkNPC );
			return true;
		}
	}
	*/

	// Otherwise, play an idle statement, try to face client when making a statement.
	CBaseEntity *pFriend = FindNearestFriend(true);
	if ( pFriend )
	{
		SetSpeechTarget( pFriend );

		// If we're about to talk to the player, and we've never said hello, say hello first
		if ( !GetSpeechFilter() || !GetSpeechFilter()->NeverSayHello() )
		{
			if ( GetExpresser()->CanSpeakConcept( TLK_HELLO ) && !GetExpresser()->SpokeConcept( TLK_HELLO ) )
			{
				SayHelloToPlayer( pFriend );
				return true;
			}
		}

		if ( Speak( TLK_IDLE ) )
		{
			DeferAllIdleSpeech( random->RandomFloat( TALKER_DEFER_IDLE_SPEAK_MIN, TALKER_DEFER_IDLE_SPEAK_MAX ) );
			m_nSpeak++;
		}
		else
		{
			// We failed to speak. Don't try again for a bit.
			m_flNextIdleSpeechTime = gpGlobals->curtime + 3;
		}

		return true;
	}

	// didn't speak
	m_flNextIdleSpeechTime = gpGlobals->curtime + 3;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Speak the right question based upon who we're asking
//-----------------------------------------------------------------------------
bool CNPCSimpleTalker::SpeakQuestionFriend( CBaseEntity *pFriend )
{
	return Speak( TLK_QUESTION );
}

//-----------------------------------------------------------------------------
// Purpose: Speak the right answer based upon who we're answering
//-----------------------------------------------------------------------------
bool CNPCSimpleTalker::SpeakAnswerFriend( CBaseEntity *pFriend )
{
	return Speak( TLK_ANSWER );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCSimpleTalker::FIdleSpeakWhileMoving( void )
{
	if ( GetExpresser()->CanSpeak() )
	{
		if (!GetExpresser()->IsSpeaking() || GetSpeechTarget() == NULL)
		{
			// override so that during walk, a scientist may talk and greet player
			FIdleHello();

			if ( ShouldSpeakRandom( m_nSpeak * 20, GetSpeechFilter() ? GetSpeechFilter()->GetIdleModifier() : 1.0 ) )
			{
				FIdleSpeak();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPCSimpleTalker::PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener )
{
	if ( !bConcurrent )
		ShutUpFriends();

	int sentenceIndex = BaseClass::PlayScriptedSentence( pszSentence, delay, volume, soundlevel, bConcurrent, pListener );
	delay += engine->SentenceLength( sentenceIndex );
	if ( delay < 0 )
		delay = 0;
	m_useTime = gpGlobals->curtime + delay;

	// Stop all idle speech until after the sentence has completed
	DeferAllIdleSpeech( delay + random->RandomInt( 3.0f, 5.0f ) );

	return sentenceIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Tell this NPC to answer a question from another NPC
//-----------------------------------------------------------------------------
void CNPCSimpleTalker::SetAnswerQuestion( CNPCSimpleTalker *pSpeaker )
{
	if ( !m_hCine )
	{
		SetCondition( COND_TALKER_RESPOND_TO_QUESTION );
	}

	SetSpeechTarget( (CAI_BaseNPC *)pSpeaker );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPCSimpleTalker::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo subInfo = info;

	// if player damaged this entity, have other friends talk about it.
	if (subInfo.GetAttacker() && (subInfo.GetAttacker()->GetFlags() & FL_CLIENT) && subInfo.GetDamage() < GetHealth() )
	{
		CBaseEntity *pFriend = FindNearestFriend(false);

		if (pFriend && pFriend->IsAlive())
		{
			// only if not dead or dying!
			CNPCSimpleTalker *pTalkNPC = (CNPCSimpleTalker *)pFriend;

			if (pTalkNPC && pTalkNPC->IsOkToCombatSpeak())
			{
				pTalkNPC->Speak( TLK_NOSHOOT );
			}
		}
	}
	return BaseClass::OnTakeDamage_Alive( subInfo );
}

int CNPCSimpleTalker::SelectNonCombatSpeechSchedule()
{
	if ( !IsOkToSpeak() )
		return SCHED_NONE;
		
	// talk about world
	if ( ShouldSpeakRandom( m_nSpeak * 2, GetSpeechFilter() ? GetSpeechFilter()->GetIdleModifier() : 1.0 ) )
	{
		//Msg("standing idle speak\n" );
		return SCHED_TALKER_IDLE_SPEAK;
	}
	
	// failed to speak, so look at the player if he's around
	if ( AI_IsSinglePlayer() && GetExpresser()->CanSpeak() && HasCondition ( COND_SEE_PLAYER ) && random->RandomInt( 0, 6 ) == 0 )
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
		Assert( pPlayer );

		if ( pPlayer )
		{
			// watch the client.
			Vector forward;
			AngleVectors( pPlayer->GetLocalAngles(), &forward );
			if ( ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).Length2D() < TALKER_STARE_DIST	&& 
				 UTIL_DotPoints( pPlayer->GetAbsOrigin(), GetAbsOrigin(), forward ) >= m_flFieldOfView )
			{
				// go into the special STARE schedule if the player is close, and looking at me too.
				return SCHED_TALKER_IDLE_WATCH_CLIENT_STARE;
			}

			return SCHED_TALKER_IDLE_WATCH_CLIENT;
		}
	}
	else
	{
		// look at who we're talking to
		if ( GetSpeechTarget() && GetExpresser()->IsSpeaking() )
			return SCHED_TALKER_IDLE_EYE_CONTACT;
	}
	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPCSimpleTalker::CanSayHello( void )
{
#ifndef HL1_DLL
	if ( Classify() == CLASS_PLAYER_ALLY_VITAL )
		return false;
#endif
	
	if ( GetSpeechFilter() && GetSpeechFilter()->NeverSayHello() )
		return false;

	if ( !GetExpresser()->CanSpeakConcept(TLK_HELLO) || GetExpresser()->SpokeConcept(TLK_HELLO) )
		return false;

	if ( !IsOkToSpeak() )
		return false;

	return true;
}

void CNPCSimpleTalker::OnStartingFollow( CBaseEntity *pTarget )
{
	GetExpresser()->SetSpokeConcept( TLK_HELLO, NULL );	// Don't say hi after you've started following
	if ( IsOkToSpeak() ) // don't speak if idle talk is blocked. player commanded/use follow will always speak
		Speak( TLK_STARTFOLLOW );
	SetSpeechTarget( GetTarget() );
	ClearCondition( COND_PLAYER_PUSHING );
}

void CNPCSimpleTalker::OnStoppingFollow( CBaseEntity *pTarget )
{
	if ( !(m_afMemory & bits_MEMORY_PROVOKED) )
	{
		if ( IsOkToCombatSpeak() )
		{
			if ( pTarget == NULL )
				Speak( TLK_STOPFOLLOW );
			else
				Speak( TLK_STOP );
		}
		SetSpeechTarget( FindNearestFriend(true) );
	}
}

void CNPCSimpleTalker::FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Don't allow use during a scripted_sentence
	if ( m_useTime > gpGlobals->curtime )
		return;

	if ( pCaller != NULL && pCaller->IsPlayer() )
	{
		if ( !m_FollowBehavior.GetFollowTarget() && IsInterruptable() )
		{
#if TOML_TODO
			LimitFollowers( pCaller , 1 );
#endif

			if ( m_afMemory & bits_MEMORY_PROVOKED )
				Msg( "I'm not following you, you evil person!\n" );
			else
			{
				StartFollowing( pCaller );
			}
		}
		else
		{
			StopFollowing();
		}
	}
}

//-----------------------------------------------------------------------------
void CNPCSimpleTalker::InputIdleRespond( inputdata_t &inputdata )
{
	// We've been told to respond. Check combat speak, not isoktospeak, because
	// we don't want to check the idle speech time.
	if (!IsOkToCombatSpeak())
		return;

	IdleRespond();
}

int CNPCSimpleTalkerExpresser::SpeakRawSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, CBaseEntity *pListener )
{
	char szSpecificSentence[1024];
	int sentenceIndex = -1;

	if ( !pszSentence )
		return sentenceIndex;

	if ( pszSentence[0] == AI_SP_START_MONOLOG )
	{
		// this sentence command will start this NPC speaking 
		// lengthy monolog from smaller sentences. 
		BeginMonolog( (char *)pszSentence, pListener );
		return -1;
	}
	else if ( pszSentence[0] == AI_SP_MONOLOG_LINE )
	{
		Q_strncpy(szSpecificSentence, pszSentence, sizeof(szSpecificSentence) );
		szSpecificSentence[0] = AI_SP_SPECIFIC_SENTENCE;
		pszSentence = szSpecificSentence;
	}
	else
	{
		// this bit of speech is interrupting my monolog!
		SuspendMonolog( 0 );
	}

	return CAI_Expresser::SpeakRawSentence( pszSentence, delay, volume, soundlevel, pListener );
}

//-------------------------------------

void CNPCSimpleTalkerExpresser::BeginMonolog( char *pszSentenceName, CBaseEntity *pListener )
{
	if( pListener )
	{
		m_hMonologTalkTarget = pListener;
	}
	else
	{
		Warning( "NULL Listener in BeginMonolog()!\n" );
		Assert(0);
		EndMonolog();
		return;
	}

	Q_strncpy( m_szMonologSentence, pszSentenceName ,sizeof(m_szMonologSentence));

	// change the "AI_SP_START_MONOLOG" to an "AI_SP_MONOLOG_LINE". m_sMonologSentence is now the 
	// string we'll tack numbers onto to play sentences from this group in 
	// sequential order.
	m_szMonologSentence[0] = AI_SP_MONOLOG_LINE;

	m_fMonologSuspended = false;

	m_iMonologIndex = 0;
}

//-------------------------------------

void CNPCSimpleTalkerExpresser::EndMonolog( void )
{
	m_szMonologSentence[0] = 0;
	m_iMonologIndex = -1;
	m_fMonologSuspended = false;
	m_hMonologTalkTarget = NULL;
}

//-------------------------------------

void CNPCSimpleTalkerExpresser::SpeakMonolog( void )
{
	int i;
	char szSentence[ MONOLOGNAME_LEN ];

	if( !HasMonolog() )
	{
		return;
	}

	if( CanSpeak() )
	{
		if( m_fMonologSuspended )
		{
			if ( GetOuter()->ShouldResumeMonolog() )
			{
				ResumeMonolog();
			}

			return;
		}

		Q_snprintf( szSentence,sizeof(szSentence), "%s%d", m_szMonologSentence, m_iMonologIndex );
		m_iMonologIndex++;

		i = SpeakRawSentence( szSentence, 0, VOL_NORM );

		if ( i == -1 )
		{
			EndMonolog();
		}
	}
	else
	{
		if( GetOuter()->ShouldSuspendMonolog() )
		{
			SuspendMonolog( 0 );
		}
	}
}

//-------------------------------------

void CNPCSimpleTalkerExpresser::SuspendMonolog( float flInterval )
{
	if( HasMonolog() )
	{
		m_fMonologSuspended = true;
	}
	
	// free up other characters to speak.
	if ( GetSink()->UseSemaphore() )
	{
		GetSpeechSemaphore( GetOuter() )->Release();
	}
}

//-------------------------------------

void CNPCSimpleTalkerExpresser::ResumeMonolog( void )
{
	if( m_iMonologIndex > 0 )
	{
		// back up and repeat what I was saying
		// when interrupted.
		m_iMonologIndex--;
	}

	GetOuter()->OnResumeMonolog();
	m_fMonologSuspended = false;
}

// try to smell something
void CNPCSimpleTalker::TrySmellTalk( void )
{
	if ( !IsOkToSpeak() )
		return;

	if ( HasCondition( COND_SMELL ) && GetExpresser()->CanSpeakConcept( TLK_SMELL ) )
		Speak( TLK_SMELL );
}

void CNPCSimpleTalker::OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior )
{
	BaseClass::OnChangeRunningBehavior( pOldBehavior,  pNewBehavior );

	CAI_FollowBehavior *pFollowBehavior;
	if ( ( pFollowBehavior = dynamic_cast<CAI_FollowBehavior *>(pNewBehavior) ) != NULL  )
	{
		OnStartingFollow( pFollowBehavior->GetFollowTarget() );
	}
	else if ( ( pFollowBehavior = dynamic_cast<CAI_FollowBehavior *>(pOldBehavior) ) != NULL  )
	{
		OnStoppingFollow( pFollowBehavior->GetFollowTarget() );
	}
}


bool CNPCSimpleTalker::OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule )
{
	bool interrupt = BaseClass::OnBehaviorChangeStatus( pBehavior, fCanFinishSchedule );
	if ( !interrupt )
	{
		interrupt = ( dynamic_cast<CAI_FollowBehavior *>(pBehavior) != NULL && ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT ) );
	}
	return interrupt;

}
//-----------------------------------------------------------------------------
// Purpose: Return true if I should speak based on the chance & the speech filter's modifier
//-----------------------------------------------------------------------------
bool CNPCSimpleTalker::ShouldSpeakRandom( int iChance, float flModifier )
{
	if ( flModifier != 1.0 )
	{
		// Avoid divide by zero
		if ( !flModifier )
			return false;

		iChance = floor( (float)iChance / flModifier );
	}

	return (random->RandomInt(0,iChance) == 0);
}


AI_BEGIN_CUSTOM_NPC(talk_monster,CNPCSimpleTalker)
	DECLARE_USES_SCHEDULE_PROVIDER( CAI_FollowBehavior )

	DECLARE_TASK(TASK_TALKER_RESPOND)
	DECLARE_TASK(TASK_TALKER_SPEAK)
	DECLARE_TASK(TASK_TALKER_HELLO)
	DECLARE_TASK(TASK_TALKER_BETRAYED)
	DECLARE_TASK(TASK_TALKER_HEADRESET)
	DECLARE_TASK(TASK_TALKER_STOPSHOOTING)
	DECLARE_TASK(TASK_TALKER_STARE)
	DECLARE_TASK(TASK_TALKER_LOOK_AT_CLIENT)
	DECLARE_TASK(TASK_TALKER_CLIENT_STARE)
	DECLARE_TASK(TASK_TALKER_EYECONTACT)
	DECLARE_TASK(TASK_TALKER_IDEALYAW)
	DECLARE_TASK(TASK_TALKER_WAIT_FOR_SEMAPHORE)

	//=========================================================
	// > SCHED_TALKER_IDLE_RESPONSE
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_RESPONSE,

		"	Tasks"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"	// Stop and listen
		"		TASK_WAIT						0.5"				// Wait until sure it's me they are talking to
		"		TASK_TALKER_IDEALYAW			0"					// look at who I'm talking to
		"		TASK_FACE_IDEAL					0"
		"		TASK_TALKER_EYECONTACT			0"					// Wait until speaker is done
		"		TASK_TALKER_WAIT_FOR_SEMAPHORE	0"
		"		TASK_TALKER_EYECONTACT			0"					// Wait until speaker is done
		"		TASK_TALKER_RESPOND				0"					// Wait and then say my response
		"		TASK_TALKER_IDEALYAW			0"					// look at who I'm talking to
		"		TASK_FACE_IDEAL					0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_SIGNAL3"
		"		TASK_TALKER_EYECONTACT			0"					// Wait until speaker is done
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_PLAYER_PUSHING"
		"		COND_GIVE_WAY"
	)

	//=========================================================
	// > SCHED_TALKER_IDLE_SPEAK
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_SPEAK,

		"	Tasks"
		"		TASK_TALKER_SPEAK			0"			// question or remark
		"		TASK_TALKER_IDEALYAW		0"			// look at who I'm talking to
		"		TASK_FACE_IDEAL				0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_SIGNAL3"
		"		TASK_TALKER_EYECONTACT		0"
		"		TASK_WAIT_RANDOM			0.5"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_PLAYER_PUSHING"
		"		COND_GIVE_WAY"
	)

	//=========================================================
	// > SCHED_TALKER_IDLE_HELLO
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_HELLO,

		"	Tasks"
		"		 TASK_SET_ACTIVITY				ACTIVITY:ACT_SIGNAL3"	// Stop and talk
		"		 TASK_TALKER_HELLO				0"			// Try to say hello to player
		"		 TASK_TALKER_EYECONTACT			0"
		"		 TASK_WAIT						0.5"		// wait a bit
		"		 TASK_TALKER_HELLO				0"			// Try to say hello to player
		"		 TASK_TALKER_EYECONTACT			0"
		"		 TASK_WAIT						0.5"		// wait a bit
		"		 TASK_TALKER_HELLO				0"			// Try to say hello to player
		"		 TASK_TALKER_EYECONTACT			0"
		"		 TASK_WAIT						0.5"		// wait a bit
		"		 TASK_TALKER_HELLO				0"			// Try to say hello to player
		"		 TASK_TALKER_EYECONTACT			0"
		"		 TASK_WAIT						0.5	"		// wait a bit
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_DANGER"
		"		COND_PLAYER_PUSHING"
		"		COND_GIVE_WAY"
	)

	//=========================================================
	// > SCHED_TALKER_IDLE_STOP_SHOOTING
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_STOP_SHOOTING,

		"	Tasks"
		"		 TASK_TALKER_STOPSHOOTING	0"	// tell player to stop shooting friend
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// Scold the player before attacking.
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_BETRAYED,

		"	Tasks"
		"		TASK_TALKER_BETRAYED	0"
		"		TASK_WAIT				0.5"
		""
		"	Interrupts"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// > SCHED_TALKER_IDLE_WATCH_CLIENT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_WATCH_CLIENT,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		TASK_TALKER_LOOK_AT_CLIENT		6"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"		// sound flags - change these and you'll break the talking code.
		"		COND_HEAR_DANGER"
		"		COND_SMELL"
		"		COND_PLAYER_PUSHING"
		"		COND_TALKER_CLIENTUNSEEN"
		"		COND_GIVE_WAY"
		"		COND_IDLE_INTERRUPT"
	)
	 
	//=========================================================
	// > SCHED_TALKER_IDLE_WATCH_CLIENT_STARE
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_WATCH_CLIENT_STARE,

		"	Tasks"
		"		 TASK_STOP_MOVING				0"
		"		 TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		 TASK_TALKER_CLIENT_STARE		6"
		"		 TASK_TALKER_STARE				0"
		"		 TASK_TALKER_IDEALYAW			0"			// look at who I'm talking to
		"		 TASK_FACE_IDEAL				0			 "
		"		 TASK_SET_ACTIVITY				ACTIVITY:ACT_SIGNAL3"
		"		 TASK_TALKER_EYECONTACT			0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"		// sound flags - change these and you'll break the talking code.
		"		COND_HEAR_DANGER"
		"		COND_SMELL"
		"		COND_PLAYER_PUSHING"
		"		COND_TALKER_CLIENTUNSEEN"
		"		COND_GIVE_WAY"
		"		COND_IDLE_INTERRUPT"
	)

	//=========================================================
	// > SCHED_TALKER_IDLE_EYE_CONTACT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_EYE_CONTACT,

		"	Tasks"
		"		TASK_TALKER_IDEALYAW			0"			// look at who I'm talking to
		"		TASK_FACE_IDEAL					0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_SIGNAL3"
		"		TASK_TALKER_EYECONTACT			0"			// Wait until speaker is done
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_PLAYER_PUSHING"
		"		COND_GIVE_WAY"
		"		COND_IDLE_INTERRUPT"
	)

AI_END_CUSTOM_NPC()
