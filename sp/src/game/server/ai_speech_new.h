//========= Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_SPEECH_H
#define AI_SPEECH_H

#include "utlmap.h"

#include "soundflags.h"
#include "AI_Criteria.h"
#include "AI_ResponseSystem.h"
#include "utldict.h"
#include "ai_speechconcept.h"

#if defined( _WIN32 )
#pragma once
#endif

class KeyValues;

using ResponseRules::ResponseType_t;
using ResponseRules::AI_ResponseFollowup;


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

#if AI_CONCEPTS_ARE_STRINGS
typedef const char *AIConcept_t;
inline bool CompareConcepts( AIConcept_t c1, AIConcept_t c2 ) 
{
	return ( (void *)c1 == (void *)c2 || ( c1 && c2 && Q_stricmp( c1, c2 ) == 0 ) );
}
#else
typedef CAI_Concept AIConcept_t;
inline bool CompareConcepts( AIConcept_t c1, AIConcept_t c2 ) 
{
	return c1.m_iConcept == c2.m_iConcept;
}
#endif


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

#ifdef MAPBASE
	// Works around issues with CAI_ExpresserHost<> class hierarchy
	virtual CAI_Expresser *GetSinkExpresser() { return NULL; }
	virtual bool IsAllowedToSpeakFollowup( AIConcept_t concept, CBaseEntity *pIssuer, bool bSpecific ) { return true; }
	virtual bool Speak( AIConcept_t concept, AI_CriteriaSet *pCriteria, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL ) { return false; }
#endif
};

struct ConceptHistory_t
{
	DECLARE_SIMPLE_DATADESC();

	ConceptHistory_t(float timeSpoken = -1 )
	 : timeSpoken( timeSpoken ), m_response( )
	{
	}

	ConceptHistory_t( const ConceptHistory_t& src );
	ConceptHistory_t& operator = ( const ConceptHistory_t& src );

	~ConceptHistory_t();
	
	float		timeSpoken;
	AI_Response m_response;
};
//-------------------------------------

class CAI_Expresser : public ResponseRules::IResponseFilter
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
	bool Speak( const AIConcept_t &concept, AI_CriteriaSet *criteria, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );

	// Given modifiers (which are colon-delimited strings), fill out a criteria set including this 
	// character's contexts and the ones in the modifier. This lets us hang on to them after a call
	// to SpeakFindResponse.
	void GatherCriteria( AI_CriteriaSet *outputCritera, const AIConcept_t &concept, const char *modifiers );
	// These two methods allow looking up a response and dispatching it to be two different steps
	// AI_Response *SpeakFindResponse( AIConcept_t concept, const char *modifiers = NULL );
	// AI_Response *SpeakFindResponse( AIConcept_t &concept, AI_CriteriaSet *criteria );
	// Find the appropriate response for the given concept. Return false if none found.
	// Fills out the response object that you provide.
	bool FindResponse( AI_Response &outResponse, const AIConcept_t &concept, AI_CriteriaSet *modifiers = NULL );
	virtual bool SpeakDispatchResponse( AIConcept_t concept, AI_Response *response, AI_CriteriaSet *criteria, IRecipientFilter *filter = NULL );
	float GetResponseDuration( AI_Response *response );

#ifdef MAPBASE
	void SetUsingProspectiveResponses( bool bToggle );
	void MarkResponseAsUsed( AI_Response *response );
#endif

	virtual int SpeakRawSentence( const char *pszSentence, float delay, float volume = VOL_NORM, soundlevel_t soundlevel = SNDLVL_TALKING, CBaseEntity *pListener = NULL );
	
	bool SemaphoreIsAvailable( CBaseEntity *pTalker );
	float GetSemaphoreAvailableTime( CBaseEntity *pTalker );

	virtual void OnSpeechFinished() {};

	// This function can be overriden by games to suppress speech altogether during glue screens, etc
	static bool IsSpeechGloballySuppressed();

	// --------------------------------
	
	virtual bool IsSpeaking();
	bool CanSpeak();
	bool CanSpeakAfterMyself();
	float GetTimeSpeechComplete() const 	{ return m_flStopTalkTime; }
#ifdef MAPBASE
	float GetTimeSpeechCompleteWithoutDelay() const	{ return m_flStopTalkTimeWithoutDelay; }
#endif
	void  BlockSpeechUntil( float time );

	// --------------------------------
	
	bool CanSpeakConcept( const AIConcept_t &concept );
	bool SpokeConcept( const AIConcept_t &concept );
	float GetTimeSpokeConcept( const AIConcept_t &concept ); // returns -1 if never
	void SetSpokeConcept( const AIConcept_t &concept, AI_Response *response, bool bCallback = true );
	void ClearSpokeConcept( const AIConcept_t &concept );

#ifdef MAPBASE
	AIConcept_t GetLastSpokeConcept( AIConcept_t excludeConcept = NULL );
#endif
	
	// --------------------------------
	
	void SetVoicePitch( int voicePitch )	{ m_voicePitch = voicePitch; }
	int GetVoicePitch() const;

	void NoteSpeaking( float duration, float delay = 0 );

	// Force the NPC to release the semaphore & clear next speech time
	void ForceNotSpeaking( void );

#ifdef MAPBASE_VSCRIPT
	bool ScriptSpeakRawScene( char const *soundname, float delay ) { return SpeakRawScene( soundname, delay, NULL ); }
	bool ScriptSpeakAutoGeneratedScene( char const *soundname, float delay ) { return SpeakAutoGeneratedScene( soundname, delay ); }
	int ScriptSpeakRawSentence( char const *pszSentence, float delay ) { return SpeakRawSentence( pszSentence, delay ); }
	bool ScriptSpeak( char const *concept, const char *modifiers ) { return Speak( concept, modifiers[0] != '\0' ? modifiers : NULL ); }
#endif

	// helper used in dealing with RESPONSE_ENTITYIO
	// response is the output of AI_Response::GetName
	// note: the response string will get stomped on (by strtok)
	// returns false on failure (eg, couldn't match parse contents)
	static bool FireEntIOFromResponse( char *response, CBaseEntity *pInitiator ); 

#ifdef MAPBASE_VSCRIPT
	// Used for RESPONSE_VSCRIPT(_FILE)
	static bool RunScriptResponse( CBaseEntity *pTarget, const char *response, AI_CriteriaSet *criteria, bool file );
#endif

#ifdef MAPBASE
public:
#else
protected:
#endif
	CAI_TimedSemaphore *GetMySpeechSemaphore( CBaseEntity *pNpc );

protected:

	bool SpeakRawScene( const char *pszScene, float delay, AI_Response *response, IRecipientFilter *filter = NULL );
	// This will create a fake .vcd/CChoreoScene to wrap the sound to be played
#ifdef MAPBASE
	bool SpeakAutoGeneratedScene( char const *soundname, float delay, AI_Response *response = NULL, IRecipientFilter *filter = NULL );
#else
	bool SpeakAutoGeneratedScene( char const *soundname, float delay );
#endif

	void DumpHistories();

	void SpeechMsg( CBaseEntity *pFlex, PRINTF_FORMAT_STRING const char *pszFormat, ... ) FMTFUNCTION(3, 4);

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
	void SetOuter( CBaseFlex *pOuter );	

	CBaseFlex *		GetOuter() 			{ return m_pOuter; }
	const CBaseFlex *	GetOuter() const 	{ return m_pOuter; }

private:
	CHandle<CBaseFlex>	m_pOuter;
};

//-----------------------------------------------------------------------------
//
// An NPC base class to assist a branch of the inheritance graph
// in utilizing CAI_Expresser
//

template <class BASE_NPC>
class CAI_ExpresserHost : public BASE_NPC, public CAI_ExpresserSink
{
	DECLARE_CLASS_NOFRIEND( CAI_ExpresserHost, BASE_NPC );

public:
#ifdef MAPBASE
	CAI_Expresser *GetSinkExpresser() { return this->GetExpresser(); }
#endif

	virtual void	NoteSpeaking( float duration, float delay );

	virtual bool 	Speak( AIConcept_t concept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );
	virtual bool 	Speak( AIConcept_t concept, AI_CriteriaSet *pCriteria, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );
#ifdef MAPBASE
	virtual bool	Speak( AIConcept_t concept, AI_CriteriaSet& modifiers, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL ) { return Speak( concept, &modifiers, pszOutResponseChosen, bufsize, filter ); }
#endif


	void GatherCriteria( AI_CriteriaSet *outputCritera, const AIConcept_t &concept, const char *modifiers );
	// These two methods allow looking up a response and dispatching it to be two different steps
#ifdef MAPBASE
	//AI_Response *SpeakFindResponse( AIConcept_t concept, const AI_CriteriaSet& modifiers );
	inline bool	SpeakDispatchResponse( const AIConcept_t &concept, AI_Response &response, AI_CriteriaSet *criteria = NULL ) { return SpeakDispatchResponse( concept, &response, criteria ); }
#endif
	bool SpeakFindResponse( AI_Response& outResponse, const AIConcept_t &concept, const char *modifiers = NULL );
	// AI_Response *	SpeakFindResponse( AIConcept_t concept, const char *modifiers = NULL );
	// AI_Response *SpeakFindResponse( AIConcept_t concept, AI_CriteriaSet *criteria );
	// AI_Response *SpeakFindResponse( AIConcept_t concept );
	// Find the appropriate response for the given concept. Return false if none found.
	// Fills out the response object that you provide.
	bool FindResponse( AI_Response &outResponse, const AIConcept_t &concept, AI_CriteriaSet *criteria = NULL );

	bool 			SpeakDispatchResponse( AIConcept_t concept, AI_Response *response,  AI_CriteriaSet *criteria = NULL );
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
inline bool CAI_ExpresserHost<BASE_NPC>::Speak( AIConcept_t concept, AI_CriteriaSet *pCriteria, char *pszOutResponseChosen /*=NULL*/, size_t bufsize /* = 0 */, IRecipientFilter *filter /* = NULL */ ) 
{
	AssertOnce( this->GetExpresser()->GetOuter() == this );
	CAI_Expresser * const RESTRICT pExpresser = this->GetExpresser();
	concept.SetSpeaker(this);
	// add in any local criteria to the one passed on the command line.
	pExpresser->GatherCriteria( pCriteria, concept, NULL );
	// call the "I have aleady gathered criteria" version of Expresser::Speak
	return pExpresser->Speak( concept, pCriteria, pszOutResponseChosen, bufsize, filter ); 
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
inline void CAI_ExpresserHost<BASE_NPC>::GatherCriteria( AI_CriteriaSet *outputCriteria, const AIConcept_t &concept, const char *modifiers )
{
	return this->GetExpresser()->GatherCriteria( outputCriteria, concept, modifiers );
}


#if 1
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::SpeakFindResponse(AI_Response& outResponse, const AIConcept_t &concept, const char *modifiers /*= NULL*/ )
{
	AI_CriteriaSet criteria;
	GatherCriteria(&criteria, concept, modifiers);
	return FindResponse( outResponse, concept, &criteria );
}
#else
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline AI_Response *CAI_ExpresserHost<BASE_NPC>::SpeakFindResponse( const AIConcept_t &concept, const char *modifiers /*= NULL*/ )
{
	return this->GetExpresser()->SpeakFindResponse( concept, modifiers );
}
#endif

#if 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline AI_Response *CAI_ExpresserHost<BASE_NPC>::SpeakFindResponse( const AIConcept_t &concept, AI_CriteriaSet *criteria /*= NULL*/ )
{
	return this->GetExpresser()->SpeakFindResponse( concept, criteria );
}


//-----------------------------------------------------------------------------
// In this case we clearly don't care to hang on to the criteria, so make a convenience
// class that generates a one off.
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline AI_Response * CAI_ExpresserHost<BASE_NPC>::SpeakFindResponse( const AIConcept_t &concept )
{
	AI_CriteriaSet criteria;
	GatherCriteria( &criteria, concept, NULL );
	return this->GetExpresser()->SpeakFindResponse( concept, &criteria );
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::FindResponse( AI_Response &outResponse, const AIConcept_t &concept, AI_CriteriaSet *criteria )
{
	return this->GetExpresser()->FindResponse( outResponse, concept, criteria );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class BASE_NPC>
inline bool CAI_ExpresserHost<BASE_NPC>::SpeakDispatchResponse( AIConcept_t concept, AI_Response *response,  AI_CriteriaSet *criteria )
{
	if ( this->GetExpresser()->SpeakDispatchResponse( concept, response, criteria ) )
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
	Speak( AIConcept_t( conceptName ) );
}

//-----------------------------------------------------------------------------

/// A shim under CAI_ExpresserHost you can use when deriving a new expresser
/// host type under CAI_BaseNPC. This does the extra step of declaring an m_pExpresser
/// member and initializing it from CreateComponents(). If your BASE_NPC class isn't
/// actually an NPC, then CreateComponents() never gets called and you won't have 
/// an expresser created.
/// Note: you still need to add m_pExpresser to the Datadesc for your derived type.
/// This is because I couldn't figure out how to make a templatized datadesc declaration
/// that works generically on the template type.
template <class BASE_NPC, class EXPRESSER_TYPE>
class CAI_ExpresserHostWithData : public CAI_ExpresserHost<BASE_NPC>
{
	DECLARE_CLASS_NOFRIEND( CAI_ExpresserHostWithData, CAI_ExpresserHost<BASE_NPC> );

public:
	CAI_ExpresserHostWithData( ) : m_pExpresser(NULL) {};

	virtual CAI_Expresser *GetExpresser() { return m_pExpresser; }
	const CAI_Expresser *GetExpresser() const { return m_pExpresser; }

	virtual bool 			CreateComponents() 
	{ 
		return BaseClass::CreateComponents() &&	( CreateExpresser() != NULL );
	}

protected:
	EXPRESSER_TYPE *CreateExpresser( void )
	{
		AssertMsg1( m_pExpresser == NULL, "Tried to double-initialize expresser in %s\n", this->GetDebugName()  );
		m_pExpresser = new EXPRESSER_TYPE(this);
		if ( !m_pExpresser)
		{
			AssertMsg1( false, "Creating an expresser failed in %s\n", this->GetDebugName() );
			return NULL;
		}

		m_pExpresser->Connect(this);
		return m_pExpresser;
	}

	virtual ~CAI_ExpresserHostWithData( void )
	{
		delete m_pExpresser; 
		m_pExpresser = NULL;
	}

	EXPRESSER_TYPE *m_pExpresser;
};

/// response rules
namespace RR
{
	/// some applycontext clauses have operators preceding them,
	/// like ++1 which means "take the current value and increment it
	/// by one". These classes detect these cases and do the appropriate
	/// thing.
	class CApplyContextOperator
	{
	public:
		inline CApplyContextOperator( int nSkipChars ) : m_nSkipChars(nSkipChars) {};

		/// perform whatever this operator does upon the given context value. 
		/// Default op is simply to copy old to new.
		/// pOldValue should be the currently set value of the context. May be NULL meaning no prior value.
		/// pOperator the value that applycontext says to set
		/// pNewValue a pointer to a buffer where the real new value will be writ.
		/// returns true on success; false on failure (eg, tried to increment a 
		/// non-numeric value). 
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );

		/// This is the function that should be called from outside, 
		/// fed the input string, it'll select the right operator 
		/// to apply.
		static CApplyContextOperator *FindOperator( const char *pContextString );
	
	protected:
		int m_nSkipChars; // how many chars to "skip" in the value string to get past the op specifier to the actual value
						  // eg, "++3" has a m_nSkipChars of 2, because the op string "++" is two characters.
	};

	class CIncrementOperator : public CApplyContextOperator
	{
	public:
		inline CIncrementOperator( int nSkipChars ) : CApplyContextOperator(nSkipChars) {};
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );
	};

	class CDecrementOperator : public CApplyContextOperator
	{
	public:
		inline CDecrementOperator( int nSkipChars ) : CApplyContextOperator(nSkipChars) {};
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );
	};

#ifdef MAPBASE
	class CMultiplyOperator : public CApplyContextOperator
	{
	public:
		inline CMultiplyOperator( int nSkipChars ) : CApplyContextOperator(nSkipChars) {};
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );
	};

	class CDivideOperator : public CApplyContextOperator
	{
	public:
		inline CDivideOperator( int nSkipChars ) : CApplyContextOperator(nSkipChars) {};
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );
	};
#endif

	class CToggleOperator : public CApplyContextOperator
	{
	public:
		inline CToggleOperator( int nSkipChars ) : CApplyContextOperator(nSkipChars) {};
		virtual bool Apply( const char *pOldValue, const char *pOperator, char *pNewValue, int pNewValBufSize );
	};

	// the singleton operators
	extern CApplyContextOperator sm_OpCopy;
	extern CIncrementOperator sm_OpIncrement;
	extern CDecrementOperator sm_OpDecrement;
#ifdef MAPBASE
	extern CMultiplyOperator sm_OpMultiply;
	extern CDivideOperator sm_OpDivide;
#endif
	extern CToggleOperator	  sm_OpToggle;

#ifdef MAPBASE
	// LEGACY - See CApplyContextOperator::FindOperator()
	extern CIncrementOperator sm_OpLegacyIncrement;
	extern CDecrementOperator sm_OpLegacyDecrement;
	extern CMultiplyOperator sm_OpLegacyMultiply;
	extern CDivideOperator sm_OpLegacyDivide;
#endif
};


//-----------------------------------------------------------------------------
#include "ai_speechqueue.h"

//-----------------------------------------------------------------------------
// A kind of AI Expresser that can dispatch a follow-up speech event when it
// finishes speaking.
//-----------------------------------------------------------------------------
class CAI_ExpresserWithFollowup : public CAI_Expresser
{
public:
	CAI_ExpresserWithFollowup( CBaseFlex *pOuter = NULL ) : CAI_Expresser(pOuter),
		m_pPostponedFollowup(NULL) 
		{};
	virtual bool Speak( AIConcept_t concept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );
	virtual bool SpeakDispatchResponse( AIConcept_t concept, AI_Response *response,  AI_CriteriaSet *criteria, IRecipientFilter *filter = NULL );
	virtual void SpeakDispatchFollowup( AI_ResponseFollowup &followup );

	virtual void OnSpeechFinished();

	typedef CAI_Expresser BaseClass;
protected:
	static void DispatchFollowupThroughQueue( const AIConcept_t &concept,
		const char *criteriaStr,
		const CResponseQueue::CFollowupTargetSpec_t &target,
		float delay,
		CBaseEntity * RESTRICT pOuter		);

	AI_ResponseFollowup *m_pPostponedFollowup; // TODO: save/restore
	CResponseQueue::CFollowupTargetSpec_t	m_followupTarget;
};

class CMultiplayer_Expresser : public CAI_ExpresserWithFollowup
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


#endif // AI_SPEECH_H
