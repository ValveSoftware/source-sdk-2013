//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_sentence.h"
#include "ai_squad.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar npc_sentences( "npc_sentences", "0" );


//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
BEGIN_SIMPLE_DATADESC(CAI_SentenceBase)
	DEFINE_FIELD( m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( m_nQueuedSentenceIndex, FIELD_INTEGER ),
	DEFINE_FIELD( m_flQueueTimeout, FIELD_TIME ),
	DEFINE_FIELD( m_nQueueSoundPriority, FIELD_INTEGER ),
END_DATADESC();


//-----------------------------------------------------------------------------
// Speech
//-----------------------------------------------------------------------------
CAI_SentenceBase::CAI_SentenceBase()
{
	ClearQueue();
}


//-----------------------------------------------------------------------------
// Debug output
//-----------------------------------------------------------------------------
void CAI_SentenceBase::SentenceMsg( const char *pStatus, const char *pSentence )
{
	int nMode = npc_sentences.GetInt();
	switch( nMode )
	{
	case 0:
		return;

	case 1:
		DevMsg( "SENTENCE [%d %.2f] %s: %s\n", GetOuter()->entindex(), gpGlobals->curtime, pStatus, pSentence );
		break;

	case 2:
		DevMsg( GetOuter(), "SENTENCE [%d %.2f] %s: %s\n", GetOuter()->entindex(), gpGlobals->curtime, pStatus, pSentence );
		break;
	}
}


//-----------------------------------------------------------------------------
// Check for queued-up-sentences + speak them
//-----------------------------------------------------------------------------
void CAI_SentenceBase::ClearQueue()
{
	m_nQueuedSentenceIndex = -1;
}


//-----------------------------------------------------------------------------
// Check for queued-up-sentences + speak them
//-----------------------------------------------------------------------------
void CAI_SentenceBase::UpdateSentenceQueue()
{
	if ( m_nQueuedSentenceIndex == -1 )
		return;

	// Check for timeout
	if ( m_flQueueTimeout < gpGlobals->curtime )
	{
		ClearQueue();
		return;
	}

	if ( GetOuter()->FOkToMakeSound( m_nQueueSoundPriority ) )
	{
		SENTENCEG_PlaySentenceIndex( GetOuter()->edict(), m_nQueuedSentenceIndex, GetVolume(), GetSoundLevel(), 0, GetVoicePitch() );

		const char *pSentenceName = engine->SentenceNameFromIndex( m_nQueuedSentenceIndex ); 
		SentenceMsg( "Speaking [from QUEUE]", pSentenceName );

		GetOuter()->JustMadeSound( m_nQueueSoundPriority );
		ClearQueue();
	}
}


//-----------------------------------------------------------------------------
// Speech criteria
//-----------------------------------------------------------------------------
bool CAI_SentenceBase::MatchesCriteria( SentenceCriteria_t nCriteria )
{
	switch(	nCriteria )
	{
	case SENTENCE_CRITERIA_ALWAYS:
		return true;

	case SENTENCE_CRITERIA_NORMAL:
		return (GetOuter()->GetState() == NPC_STATE_COMBAT) || (GetOuter()->HasSpawnFlags( SF_NPC_GAG ) == 0);

	case SENTENCE_CRITERIA_IN_SQUAD:
		if ( (GetOuter()->GetState() != NPC_STATE_COMBAT) && GetOuter()->HasSpawnFlags( SF_NPC_GAG ) )
			return false;
		return GetOuter()->GetSquad() && (GetOuter()->GetSquad()->NumMembers() > 1);

	case SENTENCE_CRITERIA_SQUAD_LEADER:
		{
			if ( (GetOuter()->GetState() != NPC_STATE_COMBAT) && GetOuter()->HasSpawnFlags( SF_NPC_GAG ) )
				return false;

			CAI_Squad *pSquad = GetOuter()->GetSquad();
			return pSquad && (pSquad->NumMembers() > 1) && pSquad->IsLeader( GetOuter() );
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Play the actual sentence
//-----------------------------------------------------------------------------
int CAI_SentenceBase::PlaySentence( const char *pSentence )
{
	int nSentenceIndex = SENTENCEG_PlayRndSz( GetOuter()->edict(), pSentence, GetVolume(), GetSoundLevel(), 0, GetVoicePitch());
	if ( nSentenceIndex < 0 )
	{
		SentenceMsg( "BOGUS", pSentence );
		return -1;
	}

	const char *pSentenceName = engine->SentenceNameFromIndex( nSentenceIndex ); 
	SentenceMsg( "Speaking", pSentenceName );
	return nSentenceIndex;
}


//-----------------------------------------------------------------------------
// Speech
//-----------------------------------------------------------------------------
int CAI_SentenceBase::Speak( const char *pSentence, SentencePriority_t nSoundPriority, SentenceCriteria_t nCriteria )
{
	if ( !MatchesCriteria(nCriteria) )
		return -1;

	// Speaking clears the queue
	ClearQueue();

	if ( nSoundPriority == SENTENCE_PRIORITY_INVALID )
	{
		return PlaySentence( pSentence );
	}

	int nSentenceIndex = -1;
	if ( GetOuter()->FOkToMakeSound( nSoundPriority ) )
	{
		nSentenceIndex = PlaySentence( pSentence );

		// Make sure sentence length utility works
//		float flSentenceTime = enginesound->GetSoundDuration( nSentenceIndex );
		GetOuter()->JustMadeSound( nSoundPriority, 2.0f /*flSentenceTime*/ );
	}
	else
	{
		SentenceMsg( "CULL", pSentence );
	}

	return nSentenceIndex;
}


//-----------------------------------------------------------------------------
// Speech w/ queue
//-----------------------------------------------------------------------------
int CAI_SentenceBase::SpeakQueued( const char *pSentence, SentencePriority_t nSoundPriority, SentenceCriteria_t nCriteria )
{
	if ( !MatchesCriteria(nCriteria) )
		return -1;

	// Speaking clears the queue
	ClearQueue();

	int nSentenceIndex = Speak( pSentence, nSoundPriority, nCriteria );
	if ( nSentenceIndex >= 0 )
		return nSentenceIndex;
	
	// Queue up the sentence for later playing 
	int nQueuedSentenceIndex = SENTENCEG_PickRndSz( pSentence );
	if ( nQueuedSentenceIndex == -1 )
		return -1;

	int nSquadCount = GetOuter()->GetSquad() ? GetOuter()->GetSquad()->NumMembers() : 1;
	m_flQueueTimeout = gpGlobals->curtime + nSquadCount * 2.0f;
	m_nQueueSoundPriority = nSoundPriority;
	m_nQueuedSentenceIndex = nQueuedSentenceIndex;
	return -1;
}


	

	
