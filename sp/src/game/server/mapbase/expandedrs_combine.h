//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Response system properties (concepts, etc.) shared by npc_combine_s and npc_metropolice
//
//=============================================================================//

#ifndef EXPANDEDRS_COMBINE_H
#define EXPANDEDRS_COMBINE_H

#ifdef _WIN32
#pragma once
#endif

#if 0
#include "ai_component.h"
#include "ai_basenpc.h"
#include "ai_sentence.h"
#endif

// 
// Concepts
// 
// These should be consistent with player allies and each other.
// 

// 
// Combine Soldiers
// 
#define TLK_CMB_ANNOUNCE "TLK_ANNOUNCE"
#define TLK_CMB_THROWGRENADE "TLK_THROWGRENADE"
#define TLK_CMB_PLAYERHIT "TLK_PLAYERHIT"
#define TLK_CMB_ASSAULT "TLK_ASSAULT"
#define TLK_CMB_ENEMY "TLK_STARTCOMBAT"
#define TLK_CMB_KILLENEMY "TLK_ENEMY_DEAD"
#define TLK_CMB_DANGER "TLK_DANGER"
#define TLK_CMB_KICK "TLK_KICK"
#define TLK_CMB_FLANK "TLK_FLANK"
#define TLK_CMB_PAIN "TLK_WOUND"
#define TLK_CMB_LOSTENEMY "TLK_LOSTENEMY"
#define TLK_CMB_REFINDENEMY "TLK_REFINDENEMY"
#define TLK_CMB_GOALERT "TLK_GOALERT"
//#define TLK_CMB_LASTOFSQUAD "TLK_LASTOFSQUAD"
#define TLK_CMB_MANDOWN "TLK_ALLY_KILLED"
#define TLK_CMB_DIE "TLK_DEATH"
#define TLK_CMB_QUESTION "TLK_QUESTION"
#define TLK_CMB_ANSWER "TLK_ANSWER"

// 
// Metrocops
// 
#define TLK_COP_MANHACKKILLED "TLK_ALLY_KILLED"
#define TLK_COP_MANDOWN "TLK_ALLY_KILLED"
#define TLK_COP_GO_ALERT "TLK_GOALERT"
#define TLK_COP_FREEZE "TLK_FREEZE"
#define TLK_COP_OVER_HERE "TLK_OVER_HERE"
#define TLK_COP_HES_RUNNING "TLK_HES_RUNNING"
#define TLK_COP_TAKE_HIM_DOWN "TLK_TAKE_HIM_DOWN"
#define TLK_COP_ARREST_IN_POS "TLK_ARREST_IN_POS"
#define TLK_COP_DEPLOY_MANHACK "TLK_DEPLOY_MANHACK"
#define TLK_COP_PLAYERHIT "TLK_PLAYERHIT"
#define TLK_COP_FLANK "TLK_FLANK"
#define TLK_COP_HEARD_SOMETHING "TLK_DARKNESS_HEARDSOUND"
#define TLK_COP_ENEMY "TLK_STARTCOMBAT"
#define TLK_COP_KILLENEMY "TLK_ENEMY_DEAD"
#define TLK_COP_NOAMMO "TLK_NOAMMO"
#define TLK_COP_LOWAMMO "TLK_LOWAMMO"
#define TLK_COP_DANGER "TLK_DANGER"
#define TLK_COP_DIE "TLK_DEATH"
#define TLK_COP_LOSTENEMY "TLK_LOSTENEMY"
#define TLK_COP_REFINDENEMY "TLK_REFINDENEMY"
#define TLK_COP_HARASS "TLK_STARE"
#define TLK_COP_IDLE "TLK_IDLE"
#define TLK_COP_QUESTION "TLK_QUESTION"
#define TLK_COP_ANSWER "TLK_ANSWER"
#define TLK_COP_PAIN "TLK_WOUND"
#define TLK_COP_COVER_HEAVY_DAMAGE "TLK_HEAVYDAMAGE"
#define TLK_COP_SHOOTCOVER "TLK_SHOOTCOVER"
#define TLK_COP_BACK_UP "TLK_BACK_UP"
#define TLK_COP_ON_FIRE "TLK_WOUND"
#define TLK_COP_HIT_BY_PHYSOBJ "TLK_PLYR_PHYSATK"
#define TLK_COP_THROWGRENADE "TLK_THROWGRENADE"
#define TLK_COP_ACTIVATE_BATON "TLK_ACTIVATE_BATON"
#define TLK_COP_DEACTIVATE_BATON "TLK_DEACTIVATE_BATON"

#define TLK_COP_MOVE_ALONG "TLK_MOVE_ALONG"

#define TLK_COP_FT_APPROACH "TLK_FT_APPROACH"
#define TLK_COP_FT_MOUNT "TLK_FT_MOUNT"
#define TLK_COP_FT_SCAN "TLK_FT_SCAN"
#define TLK_COP_FT_DISMOUNT "TLK_FT_DISMOUNT"
#define TLK_COP_SO_BEGIN "TLK_SO_BEGIN"
#define TLK_COP_SO_END "TLK_SO_END"
#define TLK_COP_SO_FORCE_COVER "TLK_SO_FORCE_COVER"
#define TLK_COP_SO_PEEK "TLK_SO_PEEK"
#define TLK_COP_AS_HIT_RALLY "TLK_AS_HIT_RALLY"
#define TLK_COP_AS_HIT_ASSAULT "TLK_AS_HIT_ASSAULT"
#define TLK_COP_AS_ADV_RALLY "TLK_AS_ADV_RALLY"
#define TLK_COP_AS_ADV_ASSAULT "TLK_AS_ADV_ASSAULT"

// 
// Snipers
// 
#define TLK_SNIPER_DIE "TLK_DEATH"
#define TLK_SNIPER_TARGETDESTROYED "TLK_ENEMY_DEAD"
#define TLK_SNIPER_DANGER "TLK_DANGER"


#if 0
static void FixupSentence(const char **ppSentence, const char **ppPrefix);

//-----------------------------------------------------------------------------
// This is the met of the class
//-----------------------------------------------------------------------------
template< class NPC_CLASS >
class CAI_SentenceTalker : public CAI_Component 
{
	DECLARE_CLASS_NOBASE( CAI_SentenceTalker );
	DECLARE_SIMPLE_DATADESC();

public:
	//CAI_SentenceTalker();

	void Init( NPC_CLASS *pOuter, const char *pGameSound = NULL );

	// Check for queued-up-sentences + speak them
	//void UpdateSentenceQueue();

	// Returns the sentence index played, which can be used to determine
	// the sentence length of time using engine->SentenceLength
	int Speak( const char *pSentence, SentencePriority_t nSoundPriority = SENTENCE_PRIORITY_NORMAL, SentenceCriteria_t nCriteria = SENTENCE_CRITERIA_IN_SQUAD );

	// Returns the sentence index played, which can be used to determine
	// the sentence length of time using engine->SentenceLength. If the sentence
	// was queued, then -1 is returned, which is the same result as if the sound wasn't played
	//int SpeakQueued( const char *pSentence, SentencePriority_t nSoundPriority = SENTENCE_PRIORITY_NORMAL, SentenceCriteria_t nCriteria = SENTENCE_CRITERIA_IN_SQUAD );

	// Clears the sentence queue
	//void ClearQueue();

protected:
	virtual float GetVolume() = 0;
	virtual soundlevel_t GetSoundLevel() = 0;

private:
	// Speech criteria
	bool ShouldSpeak( SentencePriority_t nSoundPriority, SentenceCriteria_t nCriteria );
	bool MatchesCriteria( SentenceCriteria_t nCriteria );

	// Play the actual sentence
	//int PlaySentence( const char *pSentence );

	// Debug output
	void SentenceMsg( const char *pStatus, const char *pSentence );

	int		m_voicePitch;
	int		m_nQueuedSentenceIndex;
	float	m_flQueueTimeout;
	int		m_nQueueSoundPriority;

	SentencePriority_t	m_LastPriority;

public:
	string_t	m_iRemovePrefix;
};

template< class NPC_CLASS >
void CAI_SentenceTalker< NPC_CLASS >::Init( NPC_CLASS *pOuter, const char *pGameSound )
{
	SetOuter( pOuter );
}
#endif

#endif