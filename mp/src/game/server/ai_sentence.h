//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_SENTENCE_H
#define AI_SENTENCE_H

#include "ai_component.h"
#include "ai_basenpc.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"


//-----------------------------------------------------------------------------
// Sentence helper class used by humanoids sometimes. To use:
// 1) Include it into the leaf class
// 2) Use DEFINE_EMBEDDED to save/load its state
// 3) Call Init in the CreateComponents method
// 4) Use Speak() when it's time to speak
// 5) Add m_Sentences.UpdateSentenceQueue(); to the PrescheduleThink method of an NPC
//		 to get queued sentence support working
//-----------------------------------------------------------------------------
enum SentenceCriteria_t
{
	SENTENCE_CRITERIA_ALWAYS = 0,
	SENTENCE_CRITERIA_NORMAL,		// Obeys gag rules
	SENTENCE_CRITERIA_IN_SQUAD,
	SENTENCE_CRITERIA_SQUAD_LEADER,
};

enum SentencePriority_t
{
	SENTENCE_PRIORITY_INVALID = -1,
	SENTENCE_PRIORITY_NORMAL = 0,
	SENTENCE_PRIORITY_MEDIUM = 1,
	SENTENCE_PRIORITY_HIGH = 2,
};


//-----------------------------------------------------------------------------
// This is the met of the class
//-----------------------------------------------------------------------------
abstract_class CAI_SentenceBase : public CAI_Component 
{
	DECLARE_CLASS_NOBASE( CAI_SentenceBase );
	DECLARE_SIMPLE_DATADESC();

public:
	CAI_SentenceBase();

	void SetVoicePitch( int voicePitch );
	int GetVoicePitch() const;

	// Check for queued-up-sentences + speak them
	void UpdateSentenceQueue();

	// Returns the sentence index played, which can be used to determine
	// the sentence length of time using engine->SentenceLength
	int Speak( const char *pSentence, SentencePriority_t nSoundPriority = SENTENCE_PRIORITY_NORMAL, SentenceCriteria_t nCriteria = SENTENCE_CRITERIA_IN_SQUAD );

	// Returns the sentence index played, which can be used to determine
	// the sentence length of time using engine->SentenceLength. If the sentence
	// was queued, then -1 is returned, which is the same result as if the sound wasn't played
	int SpeakQueued( const char *pSentence, SentencePriority_t nSoundPriority = SENTENCE_PRIORITY_NORMAL, SentenceCriteria_t nCriteria = SENTENCE_CRITERIA_IN_SQUAD );

	// Clears the sentence queue
	void ClearQueue();

protected:
	virtual float GetVolume() = 0;
	virtual soundlevel_t GetSoundLevel() = 0;

private:
	// Speech criteria
	bool MatchesCriteria( SentenceCriteria_t nCriteria );

	// Play the actual sentence
	int PlaySentence( const char *pSentence );

	// Debug output
	void SentenceMsg( const char *pStatus, const char *pSentence );

	int		m_voicePitch;
	int		m_nQueuedSentenceIndex;
	float	m_flQueueTimeout;
	int		m_nQueueSoundPriority;
};


//-----------------------------------------------------------------------------
// NOTE: This is a template class so each user has a different set of globals
//-----------------------------------------------------------------------------
template< class NPC_CLASS >
class CAI_Sentence : public CAI_SentenceBase 
{
	DECLARE_CLASS_NOFRIEND( CAI_Sentence, CAI_SentenceBase );

public:
	void Init( NPC_CLASS *pOuter, const char *pGameSound );

protected:
	virtual float GetVolume() { return m_sentenceVolume; }
	virtual soundlevel_t GetSoundLevel() { return m_sentenceSoundlevel; }

private:
	static float m_sentenceVolume;
	static soundlevel_t m_sentenceSoundlevel;
	static int m_voicePitchMin;
	static int m_voicePitchMax;
};


//-----------------------------------------------------------------------------
// Voice pitch
//-----------------------------------------------------------------------------
inline void CAI_SentenceBase::SetVoicePitch( int voicePitch )
{ 
	m_voicePitch = voicePitch; 
}

inline int CAI_SentenceBase::GetVoicePitch() const
{ 
	return m_voicePitch; 
}


//-----------------------------------------------------------------------------
// Set up the class's sentence information
//-----------------------------------------------------------------------------
template< class NPC_CLASS >
void CAI_Sentence< NPC_CLASS >::Init( NPC_CLASS *pOuter, const char *pGameSound )
{
	SetOuter( pOuter );

	if ( m_voicePitchMin <= 0 || m_voicePitchMax <= 0 )
	{
		// init the sentence parameters using a dummy gamesounds entry
		CSoundParameters params;
		if ( GetOuter()->GetParametersForSound( pGameSound, params, NULL ) )
		{
			m_sentenceVolume = params.volume;
			m_sentenceSoundlevel = params.soundlevel;
			m_voicePitchMin = params.pitchlow;
			m_voicePitchMax = params.pitchhigh;
		}
	}
	
	// Select a voice pitch
	if ( random->RandomInt(0,1) )
	{
		SetVoicePitch( random->RandomInt( m_voicePitchMin, m_voicePitchMax ) );
	}
	else
	{
		SetVoicePitch( 100 );
	}
}


//-----------------------------------------------------------------------------
// Global instantiation
//-----------------------------------------------------------------------------
template< class NPC_CLASS > float CAI_Sentence< NPC_CLASS >::m_sentenceVolume = 1.0f;
template< class NPC_CLASS > soundlevel_t CAI_Sentence< NPC_CLASS >::m_sentenceSoundlevel = SNDLVL_NORM;
template< class NPC_CLASS > int CAI_Sentence< NPC_CLASS >::m_voicePitchMin = 0;
template< class NPC_CLASS > int CAI_Sentence< NPC_CLASS >::m_voicePitchMax = 0;



#endif	// AI_SENTENCE_H
