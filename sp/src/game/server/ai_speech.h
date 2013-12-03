//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_SPEECH_H
#define AI_SPEECH_H

#include "utlmap.h"

#include "soundflags.h"
#include "AI_ResponseSystem.h"
#include "utldict.h"

#if defined( _WIN32 )
#pragma once
#endif

class KeyValues;
class AI_CriteriaSet;

//-----------------------------------------------------------------------------
// Purpose: Used to share a global resource or prevent a system stepping on
//			own toes.
//-----------------------------------------------------------------------------

class CAI_TimedSemaphore
{
public:
	CAI_TimedSemaphore()
	 :	m_ReleaseTime( 0 )
	{
		m_hCurrentTalker = NULL;
	}
	
	void Acquire( float time, CBaseEntity *pTalker )		{ m_ReleaseTime = gpGlobals->curtime + time; m_hCurrentTalker = pTalker; }
	void Release()					{ m_ReleaseTime = 0; m_hCurrentTalker = NULL; }
	
	// Current owner of the semaphore is always allowed to talk
	bool IsAvailable( CBaseEntity *pTalker ) const		{ return ((gpGlobals->curtime > m_ReleaseTime) || (m_hCurrentTalker == pTalker)); }
	float GetReleaseTime() const 	{ return m_ReleaseTime; }

	CBaseEntity *GetOwner()	{ return m_hCurrentTalker; }

private:
	float		m_ReleaseTime;
	EHANDLE		m_hCurrentTalker;
};

//-----------------------------------------------------------------------------

extern CAI_TimedSemaphore g_AIFriendliesTalkSemaphore;
extern CAI_TimedSemaphore g_AIFoesTalkSemaphore;

#define GetSpeechSemaphore( pNpc ) (((pNpc)->IsPlayerAlly()) ? &g_AIFriendliesTalkSemaphore : &g_AIFoesTalkSemaphore )
//-----------------------------------------------------------------------------
// Basic speech system types
//-----------------------------------------------------------------------------

//-------------------------------------
// Constants


const float AIS_DEF_MIN_DELAY 	= 2.8; // Minimum amount of time an NPCs will wait after someone has spoken before considering speaking again
const float AIS_DEF_MAX_DELAY 	= 3.2; // Maximum amount of time an NPCs will wait after someone has spoken before considering speaking again
const float AIS_NO_DELAY  		= 0;
const soundlevel_t AIS_DEF_SNDLVL 	 	= SNDLVL_TALKING;
#define AI_NULL_CONCEPT NULL

#define AI_NULL_SENTENCE NULL

// Sentence prefix constants
#define AI_SP_SPECIFIC_SENTENCE	'!'
#define AI_SP_WAVFILE			'^'
#define AI_SP_SCENE_GROUP		'='
#define AI_SP_SPECIFIC_SCENE	'?'

#define AI_SPECIFIC_SENTENCE(str_constant)	"!" str_constant
#define AI_WAVFILE(str_constant)			"^" str_constant
// @Note (toml 09-12-02): as scene groups are not currently implemented, the string is a semi-colon delimited list
#define AI_SCENE_GROUP(str_constant)		"=" str_constant
#define AI_SPECIFIC_SCENE(str_constant)		"?" str_constant

// Designer overriding modifiers
#define AI_SPECIFIC_SCENE_MODIFIER "scene:"

//-------------------------------------

//-------------------------------------
// An id that represents the core meaning of a spoken phrase, 
// eventually to be mapped to a sentence group or scene

typedef const char *AIConcept_t;

inline bool CompareConcepts( AIConcept_t c1, AIConcept_t c2 ) 
{
	return ( (void *)c1 == (void *)c2 || ( c1 && c2 && Q_stricmp( c1, c2 ) == 0 ) );
}

//-------------------------------------
// Specifies and stores the base timing and attentuation values for concepts
//
class AI_Response;

//-----------------------------------------------------------------------------
// CAI_Expresser
//
// Purpose: Provides the functionality of going from abstract concept ("hello")
//			to specific sentence/scene/wave
//

//-------------------------------------
// Sink supports behavior control and receives notifications of internal events

class CAI_ExpresserSink
{
public:
	virtual void OnSpokeConcept( AIConcept_t concept, AI_Response *response )	{};
	virtual void OnStartSpeaking()						{}
	virtual bool UseSemaphore()							{ return true; }
};

struct ConceptHistory_t
{
	DECLARE_SIMPLE_DATADESC();

	ConceptHistory_t(float timeSpoken = -1 )
	 : timeSpoken( timeSpoken ), response( NULL )
	{
	}

	ConceptHistory_t( const ConceptHistory_t& src );
	ConceptHistory_t& operator = ( const ConceptHistory_t& src );

	~ConceptHistory_t();
	
	float		timeSpoken;
	AI_Response *response;
};
//-------------------------------------

class CAI_Expresser : public IResponseFilter
{
public:
	CAI_Expresser( CBaseFlex *pOuter = NULL );
	~CAI_Expresser();

	// --------------------------------
	
	bool Connect( CAI_ExpresserSink *pSink )		{ m_pSink = pSink; return true; }
	bool Disconnect( CAI_ExpresserSink *pSink )	{ m_pSink = NULL; return true;}

	void TestAllResponses();

	// --------------------------------
	
	bool Speak( AIConcept_t concept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );

	// These two methods allow looking up a response and dispatching it to be two different steps
	AI_Response *SpeakFindResponse( AIConcept_t concept, const char *modifiers = NULL );
	bool SpeakDispatchResponse( AIConcept_t concept, AI_Response *response, IRecipientFilter *filter = NULL );
	float GetResponseDuration( AI_Response *response );

	virtual int SpeakRawSentence( const char *pszSentence, float delay, float volume = VOL_NORM, soundlevel_t soundlevel = SNDLVL_TALKING, CBaseEntity *pListener = NULL );
	
	bool SemaphoreIsAvailable( CBaseEntity *pTalker );
	float GetSemaphoreAvailableTime( CBaseEntity *pTalker );

	// --------------------------------
	
	virtual bool IsSpeaking();
	bool CanSpeak();
	bool CanSpeakAfterMyself();
	float GetTimeSpeechComplete() const 	{ return m_flStopTalkTime; }
	void  BlockSpeechUntil( float time );

	// --------------------------------
	
	bool CanSpeakConcept( AIConcept_t concept );
	bool SpokeConcept( AIConcept_t concept );
	float GetTimeSpokeConcept( AIConcept_t concept ); // returns -1 if never
	void SetSpokeConcept( AIConcept_t concept, AI_Response *response, bool bCallback = true );
	void ClearSpokeConcept( AIConcept_t concept );
	
	// --------------------------------
	
	void SetVoicePitch( int voicePitch )	{ m_voicePitch = voicePitch; }
	int GetVoicePitch() const;

	void NoteSpeaking( float duration, float delay = 0 );

	// Force the NPC to release the semaphore & clear next speech time
	void ForceNotSpeaking( void );

protected:
	CAI_TimedSemaphore *GetMySpeechSemaphore( CBaseEntity *pNpc );

	bool SpeakRawScene( const char *pszScene, float delay, AI_Response *response, IRecipientFilter *filter = NULL );
	// This will create a fake .vcd/CChoreoScene to wrap the sound to be played
	bool SpeakAutoGeneratedScene( char const *soundname, float delay );

	void DumpHistories();

	void SpeechMsg( CBaseEntity *pFlex, PRINTF_FORMAT_STRING const char *pszFormat, ... );

	// --------------------------------
	
	CAI_ExpresserSink *GetSink() { return m_pSink; }

private:
	// --------------------------------

	virtual bool IsValidResponse( ResponseType_t type, const char *pszValue );

	// --------------------------------
	
	CAI_ExpresserSink *m_pSink;
	
	// --------------------------------
	//
	// Speech concept data structures
	//

	CUtlDict< ConceptHistory_t, int > m_ConceptHistories;
	
	// --------------------------------
	//
	// Speaking states
	//

	float				m_flStopTalkTime;				// when in the future that I'll be done saying this sentence.
	float				m_flStopTalkTimeWithoutDelay;	// same as the above, but minus the delay before other people can speak
	float				m_flBlockedTalkTime;
	int					m_voicePitch;					// pitch of voice for this head
	float				m_flLastTimeAcceptedSpeak;		// because speech may not be blocked until NoteSpeaking called by scene ent, this handles in-think blocking
	
	DECLARE_SIMPLE_DATADESC();

	// --------------------------------
	//
public:
	virtual void SetOuter( CBaseFlex *pOuter )	{ m_pOuter = pOuter; }

	CBaseFlex *		GetOuter() 			{ return m_pOuter; }
	const CBaseFlex *	GetOuter() const 	{ return m_pOuter; }

private:
	CHandle<CBaseFlex>	m_pOuter;
};

class CMultiplayer_Expresser : public CAI_Expresser
{
public:
	CMultiplayer_Expresser( CBaseFlex *pOuter = NULL );
	//~CMultiplayer_Expresser();

	virtual bool IsSpeaking();

	void AllowMultipleScenes();
	void DisallowMultipleScenes();

private:
	bool m_bAllowMultipleScenes;

};

//-----------------------------------------------------------------------------
//
// An NPC base class to assist a branch of the inheritance graph
// in utilizing CAI_Expresser
//

template <class BASE_NPC>
class CAI_ExpresserHost : public BASE_NPC, protected CAI_ExpresserSink
{
	DECLARE_CLASS_NOFRIEND( CAI_ExpresserHost, BASE_NPC );

public:
	virtual void	NoteSpeaking( float duration, float delay );

	virtual bool 	Speak( AIConcept_t concept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );

	// These two methods allow looking up a response and dispatching it to be two different steps
	AI_Response *	SpeakFindResponse( AIConcept_t concept, const char *modifiers = NULL );
	bool 			SpeakDispatchResponse( AIConcept_t concept, AI_Response *response );
	virtual void	PostSpeakDispatchResponse( AIConcept_t concept, AI_Response *response ) { return; }
	float 			GetResponseDuration( AI_Response *response );

	float GetTimeSpeechComplete() const 	{ return this->GetExpresser()->GetTimeSpeechComplete(); }

	bool IsSpeaking()				{ return this->GetExpresser()->IsSpeaking(); }
	bool CanSpeak()					{ return this->GetExpresser()->CanSpeak(); }
	bool CanSpeakAfterMyself()		{ return this->GetExpresser()->CanSpeakAfterMyself(); }

	void SetSpokeConcept( AIConcept_t concept, AI_Response *response, bool bCallback = true ) 		{ this->GetExpresser()->SetSpokeConcept( concept, response, bCallback ); }
	float GetTimeSpokeConcept( AIConcept_t concept )												{ return this->GetExpresser()->GetTimeSpokeConcept( concept ); }
	bool SpokeConcept( AIConcept_t concept )														{ return this->GetExpresser()->SpokeConcept( concept ); }

protected:
	int 			PlaySentence( const char *pszSentence, float delay, float volume = VOL_NORM, soundlevel_t soundlevel = SNDLVL_TALKING, CBaseEntity *pListener = NULL );
	virtual void	ModifyOrAppendCriteria( AI_CriteriaSet& set );

	virtual IResponseSystem *GetResponseSystem();
	// Override of base entity response input handler
	virtual void	DispatchResponse( const char *conceptName );
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline void CAI_ExpresserHost<BASE_NPC>::NoteSpeaking( float duration, float delay )
{ 
	this->GetExpresser()->NoteSpeaking( duration, delay ); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::Speak( AIConcept_t concept, const char *modifiers /*= NULL*/, char *pszOutResponseChosen /*=NULL*/, size_t bufsize /* = 0 */, IRecipientFilter *filter /* = NULL */ ) 
{
	AssertOnce( this->GetExpresser()->GetOuter() == this );
	return this->GetExpresser()->Speak( concept, modifiers, pszOutResponseChosen, bufsize, filter ); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline int CAI_ExpresserHost<BASE_NPC>::PlaySentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, CBaseEntity *pListener )
{
	return this->GetExpresser()->SpeakRawSentence( pszSentence, delay, volume, soundlevel, pListener );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
extern void CAI_ExpresserHost_NPC_DoModifyOrAppendCriteria( CAI_BaseNPC *pSpeaker, AI_CriteriaSet& criteriaSet );

template <class BASE_NPC>
inline void CAI_ExpresserHost<BASE_NPC>::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	if ( this->MyNPCPointer() )
	{
		CAI_ExpresserHost_NPC_DoModifyOrAppendCriteria( this->MyNPCPointer(), criteriaSet );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline IResponseSystem *CAI_ExpresserHost<BASE_NPC>::GetResponseSystem()
{
	extern IResponseSystem *g_pResponseSystem;
	// Expressive NPC's use the general response system
	return g_pResponseSystem;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline AI_Response *CAI_ExpresserHost<BASE_NPC>::SpeakFindResponse( AIConcept_t concept, const char *modifiers /*= NULL*/ )
{
	return this->GetExpresser()->SpeakFindResponse( concept, modifiers );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::SpeakDispatchResponse( AIConcept_t concept, AI_Response *response )
{
	if ( this->GetExpresser()->SpeakDispatchResponse( concept, response ) )
	{
		PostSpeakDispatchResponse( concept, response );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline float CAI_ExpresserHost<BASE_NPC>::GetResponseDuration( AI_Response *response )
{
	return this->GetExpresser()->GetResponseDuration( response );
}

//-----------------------------------------------------------------------------
// Override of base entity response input handler
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline void CAI_ExpresserHost<BASE_NPC>::DispatchResponse( const char *conceptName )
	{
		Speak( (AIConcept_t)conceptName );
	}

//-----------------------------------------------------------------------------

#endif // AI_SPEECH_H
