//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "ai_speech.h"

#include "game.h"
#include "engine/IEngineSound.h"
#include "KeyValues.h"
#include "ai_basenpc.h"
#include "AI_Criteria.h"
#include "isaverestore.h"
#include "sceneentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define DEBUG_AISPEECH 1
#ifdef DEBUG_AISPEECH
ConVar ai_debug_speech( "ai_debug_speech", "0" );
#define DebuggingSpeech() ai_debug_speech.GetBool()
#else
inline void SpeechMsg( ... ) {}
#define DebuggingSpeech() (false)
#endif

extern ConVar rr_debugresponses;

//-----------------------------------------------------------------------------

CAI_TimedSemaphore g_AIFriendliesTalkSemaphore;
CAI_TimedSemaphore g_AIFoesTalkSemaphore;

ConceptHistory_t::~ConceptHistory_t()
{
	if ( response )
	{
		delete response;
	}
	response = NULL;
}

ConceptHistory_t::ConceptHistory_t( const ConceptHistory_t& src )
{
	timeSpoken = src.timeSpoken;
	response = NULL;
	if ( src.response )
	{
		response = new AI_Response( *src.response );
	}
}

ConceptHistory_t& ConceptHistory_t::operator =( const ConceptHistory_t& src )
{
	if ( this == &src )
		return *this;

	timeSpoken = src.timeSpoken;
	response = NULL;
	if ( src.response )
	{
		response = new AI_Response( *src.response );
	}

	return *this;
}

BEGIN_SIMPLE_DATADESC( ConceptHistory_t )
	DEFINE_FIELD( timeSpoken,	FIELD_TIME ),  // Relative to server time
	// DEFINE_EMBEDDED( response,	FIELD_??? ),	// This is manually saved/restored by the ConceptHistory saverestore ops below
END_DATADESC()

class CConceptHistoriesDataOps : public CDefSaveRestoreOps
{
public:
	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{
		CUtlDict< ConceptHistory_t, int > *ch = ((CUtlDict< ConceptHistory_t, int > *)fieldInfo.pField);
		int count = ch->Count();
		pSave->WriteInt( &count );
		for ( int i = 0 ; i < count; i++ )
		{
			ConceptHistory_t *pHistory = &(*ch)[ i ];

			pSave->StartBlock();
			{

				// Write element name
				pSave->WriteString( ch->GetElementName( i ) );

				// Write data
				pSave->WriteAll( pHistory );
				// Write response blob
				bool hasresponse = pHistory->response != NULL ? true : false;
				pSave->WriteBool( &hasresponse );
				if ( hasresponse )
				{
					pSave->WriteAll( pHistory->response );
				}
				// TODO: Could blat out pHistory->criteria pointer here, if it's needed
			}
			pSave->EndBlock();
		}
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		CUtlDict< ConceptHistory_t, int > *ch = ((CUtlDict< ConceptHistory_t, int > *)fieldInfo.pField);

		int count = pRestore->ReadInt();
		Assert( count >= 0 );
		for ( int i = 0 ; i < count; i++ )
		{
			char conceptname[ 512 ];
			conceptname[ 0 ] = 0;
			ConceptHistory_t history;

			pRestore->StartBlock();
			{
				pRestore->ReadString( conceptname, sizeof( conceptname ), 0 );

				pRestore->ReadAll( &history );

				bool hasresponse = false;

				pRestore->ReadBool( &hasresponse );
				if ( hasresponse )
				{
					history.response = new AI_Response();
					pRestore->ReadAll( history.response );
				}
			}

			pRestore->EndBlock();

			// TODO: Could restore pHistory->criteria pointer here, if it's needed

			// Add to utldict
			if ( conceptname[0] != 0 )
			{
				ch->Insert( conceptname, history );
			}
			else
			{
				Assert( !"Error restoring ConceptHistory_t, discarding!" );
			}
		}
	}
	
	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		CUtlDict< ConceptHistory_t, int > *ch = ((CUtlDict< ConceptHistory_t, int > *)fieldInfo.pField);
		return ch->Count() == 0 ? true : false;
	}
};

CConceptHistoriesDataOps g_ConceptHistoriesSaveDataOps;

//-----------------------------------------------------------------------------
//
// CLASS: CAI_Expresser
//

BEGIN_SIMPLE_DATADESC( CAI_Expresser )
	//									m_pSink		(reconnected on load)
//	DEFINE_FIELD( m_pOuter, CHandle < CBaseFlex > ),
	DEFINE_CUSTOM_FIELD( m_ConceptHistories,	&g_ConceptHistoriesSaveDataOps ),
	DEFINE_FIELD(		m_flStopTalkTime,		FIELD_TIME		),
	DEFINE_FIELD(		m_flStopTalkTimeWithoutDelay, FIELD_TIME		),
	DEFINE_FIELD(		m_flBlockedTalkTime, 	FIELD_TIME		),
	DEFINE_FIELD(		m_voicePitch,			FIELD_INTEGER	),
	DEFINE_FIELD(		m_flLastTimeAcceptedSpeak, 	FIELD_TIME		),
END_DATADESC()

//-------------------------------------

bool CAI_Expresser::SemaphoreIsAvailable( CBaseEntity *pTalker )
{
	if ( !GetSink()->UseSemaphore() )
		return true;

	CAI_TimedSemaphore *pSemaphore = GetMySpeechSemaphore( pTalker->MyNPCPointer() );
	return (pSemaphore ? pSemaphore->IsAvailable( pTalker ) : true);
}

//-------------------------------------

float CAI_Expresser::GetSemaphoreAvailableTime( CBaseEntity *pTalker )
{
	CAI_TimedSemaphore *pSemaphore = GetMySpeechSemaphore( pTalker->MyNPCPointer() );
	return (pSemaphore ? pSemaphore->GetReleaseTime() : 0);
}

//-------------------------------------

int CAI_Expresser::GetVoicePitch() const
{
	return m_voicePitch + random->RandomInt(0,3);
}

#ifdef DEBUG
static int g_nExpressers;
#endif

CAI_Expresser::CAI_Expresser( CBaseFlex *pOuter )
 :	m_pOuter( pOuter ),
	m_pSink( NULL ),
	m_flStopTalkTime( 0 ),
	m_flLastTimeAcceptedSpeak( 0 ),
	m_flBlockedTalkTime( 0 ),
	m_flStopTalkTimeWithoutDelay( 0 ),
	m_voicePitch( 100 )
{
#ifdef DEBUG
	g_nExpressers++;
#endif
}

CAI_Expresser::~CAI_Expresser()
{
	m_ConceptHistories.Purge();

	CAI_TimedSemaphore *pSemaphore = GetMySpeechSemaphore( GetOuter() );
	if ( pSemaphore )
	{
		if ( pSemaphore->GetOwner() == GetOuter() )
			pSemaphore->Release();

#ifdef DEBUG
		g_nExpressers--;
		if ( g_nExpressers == 0 && pSemaphore->GetOwner() )
			DevMsg( 2, "Speech semaphore being held by non-talker entity\n" );
#endif
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_Expresser::TestAllResponses()
{
	IResponseSystem *pResponseSystem = GetOuter()->GetResponseSystem();
	if ( pResponseSystem )
	{
		CUtlVector<AI_Response *> responses;
		pResponseSystem->GetAllResponses( &responses );
		for ( int i = 0; i < responses.Count(); i++ )
		{
			char response[ 256 ];
			responses[i]->GetResponse( response, sizeof( response ) );

			Msg( "Response: %s\n", response );
			SpeakDispatchResponse( "", responses[i] );
		}
	}
}

//-----------------------------------------------------------------------------

static const int LEN_SPECIFIC_SCENE_MODIFIER = strlen( AI_SPECIFIC_SCENE_MODIFIER );

//-----------------------------------------------------------------------------
// Purpose: Searches for a possible response
// Input  : concept - 
//			NULL - 
// Output : AI_Response
//-----------------------------------------------------------------------------
AI_Response *CAI_Expresser::SpeakFindResponse( AIConcept_t concept, const char *modifiers /*= NULL*/ )
{
	IResponseSystem *rs = GetOuter()->GetResponseSystem();
	if ( !rs )
	{
		Assert( !"No response system installed for CAI_Expresser::GetOuter()!!!" );
		return NULL;
	}

	AI_CriteriaSet set;
	// Always include the concept name
	set.AppendCriteria( "concept", concept, CONCEPT_WEIGHT );

	// Always include any optional modifiers
	if ( modifiers != NULL )
	{
		char copy_modifiers[ 255 ];
		const char *pCopy;
		char key[ 128 ] = { 0 };
		char value[ 128 ] = { 0 };

		Q_strncpy( copy_modifiers, modifiers, sizeof( copy_modifiers ) );
		pCopy = copy_modifiers;

		while( pCopy )
		{
			pCopy = SplitContext( pCopy, key, sizeof( key ), value, sizeof( value ), NULL );

			if( *key && *value )
			{
				set.AppendCriteria( key, value, CONCEPT_WEIGHT );
			}
		}
	}

	// Let our outer fill in most match criteria
	GetOuter()->ModifyOrAppendCriteria( set );

	// Append local player criteria to set, but not if this is a player doing the talking
	if ( !GetOuter()->IsPlayer() )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
		if( pPlayer )
			pPlayer->ModifyOrAppendPlayerCriteria( set );
	}

	// Now that we have a criteria set, ask for a suitable response
	AI_Response *result = new AI_Response;
	Assert( result && "new AI_Response: Returned a NULL AI_Response!" );
	bool found = rs->FindBestResponse( set, *result, this );

	if ( rr_debugresponses.GetInt() == 3 )
	{
		if ( ( GetOuter()->MyNPCPointer() && GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT ) || GetOuter()->IsPlayer() )
		{
			const char *pszName;
			if ( GetOuter()->IsPlayer() )
			{
				pszName = ((CBasePlayer*)GetOuter())->GetPlayerName();
			}
			else
			{
				pszName = GetOuter()->GetDebugName();
			}

			if ( found )
			{
				char response[ 256 ];
				result->GetResponse( response, sizeof( response ) );

				Warning( "RESPONSERULES: %s spoke '%s'. Found response '%s'.\n", pszName, concept, response );
			}
			else
			{
				Warning( "RESPONSERULES: %s spoke '%s'. Found no matching response.\n", pszName, concept );
			}
		}
	}

	if ( !found )
	{
		//Assert( !"rs->FindBestResponse: Returned a NULL AI_Response!" );
		delete result;
		return NULL;
	}

	char response[ 256 ];
	result->GetResponse( response, sizeof( response ) );

	if ( !response[0] )
	{
		delete result;
		return NULL;
	}

	if ( result->GetOdds() < 100 && random->RandomInt( 1, 100 ) <= result->GetOdds() )
	{
		delete result;
		return NULL;
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Dispatches the result
// Input  : *response - 
//-----------------------------------------------------------------------------
bool CAI_Expresser::SpeakDispatchResponse( AIConcept_t concept, AI_Response *result, IRecipientFilter *filter /* = NULL */ )
{
	char response[ 256 ];
	result->GetResponse( response, sizeof( response ) );

	float delay = result->GetDelay();
	
	bool spoke = false;

	soundlevel_t soundlevel = result->GetSoundLevel();

	if ( IsSpeaking() && concept[0] != 0 )
	{
		DevMsg( "SpeakDispatchResponse:  Entity ( %i/%s ) already speaking, forcing '%s'\n", GetOuter()->entindex(), STRING( GetOuter()->GetEntityName() ), concept );

		// Tracker 15911:  Can break the game if we stop an imported map placed lcs here, so only
		//  cancel actor out of instanced scripted scenes.  ywb
		RemoveActorFromScriptedScenes( GetOuter(), true /*instanced scenes only*/ );
		GetOuter()->SentenceStop();

		if ( IsRunningScriptedScene( GetOuter() ) )
		{
			DevMsg( "SpeakDispatchResponse:  Entity ( %i/%s ) refusing to speak due to scene entity, tossing '%s'\n", GetOuter()->entindex(), STRING( GetOuter()->GetEntityName() ), concept );
			delete result;
			return false;
		}
	}

	switch ( result->GetType() )
	{
	default:
	case RESPONSE_NONE:
		break;

	case RESPONSE_SPEAK:
		{
			if ( !result->ShouldntUseScene() )
			{
				// This generates a fake CChoreoScene wrapping the sound.txt name
				spoke = SpeakAutoGeneratedScene( response, delay );
			}
			else
			{
				float speakTime = GetResponseDuration( result );
				GetOuter()->EmitSound( response );

				DevMsg( "SpeakDispatchResponse:  Entity ( %i/%s ) playing sound '%s'\n", GetOuter()->entindex(), STRING( GetOuter()->GetEntityName() ), response );
				NoteSpeaking( speakTime, delay );
				spoke = true;
			}
		}
		break;

	case RESPONSE_SENTENCE:
		{
			spoke = ( -1 != SpeakRawSentence( response, delay, VOL_NORM, soundlevel ) ) ? true : false;
		}
		break;

	case RESPONSE_SCENE:
		{
			spoke = SpeakRawScene( response, delay, result, filter );
		}
		break;

	case RESPONSE_RESPONSE:
		{
			// This should have been recursively resolved already
			Assert( 0 );
		}
		break;
	case RESPONSE_PRINT:
		{
			if ( g_pDeveloper->GetInt() > 0 )
			{
				Vector vPrintPos;
				GetOuter()->CollisionProp()->NormalizedToWorldSpace( Vector(0.5,0.5,1.0f), &vPrintPos );
				NDebugOverlay::Text( vPrintPos, response, true, 1.5 );
				spoke = true;
			}
		}
		break;
	}

	if ( spoke )
	{
		m_flLastTimeAcceptedSpeak = gpGlobals->curtime;
		if ( DebuggingSpeech() && g_pDeveloper->GetInt() > 0 && response && result->GetType() != RESPONSE_PRINT )
		{
			Vector vPrintPos;
			GetOuter()->CollisionProp()->NormalizedToWorldSpace( Vector(0.5,0.5,1.0f), &vPrintPos );
			NDebugOverlay::Text( vPrintPos, CFmtStr( "%s: %s", concept, response ), true, 1.5 );
		}

		if ( result->IsApplyContextToWorld() )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( engine->PEntityOfEntIndex( 0 ) );
			if ( pEntity )
			{
				pEntity->AddContext( result->GetContext() );
			}
		}
		else
		{
			GetOuter()->AddContext( result->GetContext() );
		}
		SetSpokeConcept( concept, result );
	}
	else
	{
		delete result;
	}

	return spoke;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *response - 
// Output : float
//-----------------------------------------------------------------------------
float CAI_Expresser::GetResponseDuration( AI_Response *result )
{
	Assert( result );
	char response[ 256 ];
	result->GetResponse( response, sizeof( response ) );

	switch ( result->GetType() )
	{
	default:
	case RESPONSE_NONE:
		break;
	case RESPONSE_SPEAK:
		{
			return GetOuter()->GetSoundDuration( response, STRING( GetOuter()->GetModelName() ) );
		}
		break;
	case RESPONSE_SENTENCE:
		{
			Assert( 0 );
			return 999.0f;
		}
		break;
	case RESPONSE_SCENE:
		{
			return GetSceneDuration( response );
		}
		break;
	case RESPONSE_RESPONSE:
		{
			// This should have been recursively resolved already
			Assert( 0 );
		}
		break;
	case RESPONSE_PRINT:
		{
			return 1.0;
		}
		break;
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Placeholder for rules based response system
// Input  : concept - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_Expresser::Speak( AIConcept_t concept, const char *modifiers /*= NULL*/, char *pszOutResponseChosen /* = NULL*/, size_t bufsize /* = 0 */, IRecipientFilter *filter /* = NULL */ )
{
	AI_Response *result = SpeakFindResponse( concept, modifiers );
	if ( !result )
	{
		return false;
	}

	SpeechMsg( GetOuter(), "%s (%p) spoke %s (%f)\n", STRING(GetOuter()->GetEntityName()), GetOuter(), concept, gpGlobals->curtime );

	bool spoke = SpeakDispatchResponse( concept, result, filter );
	if ( pszOutResponseChosen )
	{
		result->GetResponse( pszOutResponseChosen, bufsize );
	}
	
	return spoke;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_Expresser::SpeakRawScene( const char *pszScene, float delay, AI_Response *response, IRecipientFilter *filter /* = NULL */ )
{
	float sceneLength = GetOuter()->PlayScene( pszScene, delay, response, filter );
	if ( sceneLength > 0 )
	{
		SpeechMsg( GetOuter(), "SpeakRawScene( %s, %f) %f\n", pszScene, delay, sceneLength );

#if defined( HL2_EPISODIC ) || defined( TF_DLL )
		char szInstanceFilename[256];
		GetOuter()->GenderExpandString( pszScene, szInstanceFilename, sizeof( szInstanceFilename ) );
		// Only mark ourselves as speaking if the scene has speech
		if ( GetSceneSpeechCount(szInstanceFilename) > 0 )
		{
			NoteSpeaking( sceneLength, delay );
		}
#else
		NoteSpeaking( sceneLength, delay );
#endif

		return true;
	}
	return false;
}

// This will create a fake .vcd/CChoreoScene to wrap the sound to be played
bool CAI_Expresser::SpeakAutoGeneratedScene( char const *soundname, float delay )
{
	float speakTime = GetOuter()->PlayAutoGeneratedSoundScene( soundname );
	if ( speakTime > 0 )
	{
		SpeechMsg( GetOuter(), "SpeakAutoGeneratedScene( %s, %f) %f\n", soundname, delay, speakTime );
		NoteSpeaking( speakTime, delay );
		return true;
	}
	return false;
}

//-------------------------------------

int CAI_Expresser::SpeakRawSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, CBaseEntity *pListener )
{
	int sentenceIndex = -1;

	if ( !pszSentence )
		return sentenceIndex;

	if ( pszSentence[0] == AI_SP_SPECIFIC_SENTENCE )
	{
		sentenceIndex = SENTENCEG_Lookup( pszSentence );

		if( sentenceIndex == -1 )
		{
			// sentence not found
			return -1;
		}

		CPASAttenuationFilter filter( GetOuter(), soundlevel );
		CBaseEntity::EmitSentenceByIndex( filter, GetOuter()->entindex(), CHAN_VOICE, sentenceIndex, volume, soundlevel, 0, GetVoicePitch());
	}
	else
	{
		sentenceIndex = SENTENCEG_PlayRndSz( GetOuter()->NetworkProp()->edict(), pszSentence, volume, soundlevel, 0, GetVoicePitch() );
	}

	SpeechMsg( GetOuter(), "SpeakRawSentence( %s, %f) %f\n", pszSentence, delay, engine->SentenceLength( sentenceIndex ) );
	NoteSpeaking( engine->SentenceLength( sentenceIndex ), delay );

	return sentenceIndex;
}

//-------------------------------------

void CAI_Expresser::BlockSpeechUntil( float time ) 	
{ 
	SpeechMsg( GetOuter(), "BlockSpeechUntil(%f) %f\n", time, time - gpGlobals->curtime );
	m_flBlockedTalkTime = time; 
}


//-------------------------------------

void CAI_Expresser::NoteSpeaking( float duration, float delay )
{
	duration += delay;
	
	GetSink()->OnStartSpeaking();

	if ( duration <= 0 )
	{
		// no duration :( 
		m_flStopTalkTime = gpGlobals->curtime + 3;
		duration = 0;
	}
	else
	{
		m_flStopTalkTime = gpGlobals->curtime + duration;
	}

	m_flStopTalkTimeWithoutDelay = m_flStopTalkTime - delay;

	SpeechMsg( GetOuter(), "NoteSpeaking( %f, %f ) (stop at %f)\n", duration, delay, m_flStopTalkTime );

	if ( GetSink()->UseSemaphore() )
	{
		CAI_TimedSemaphore *pSemaphore = GetMySpeechSemaphore( GetOuter() );
		if ( pSemaphore )
		{
			pSemaphore->Acquire( duration, GetOuter() );
		}
	}
}

//-------------------------------------

void CAI_Expresser::ForceNotSpeaking( void )
{
	if ( IsSpeaking() )
	{
		m_flStopTalkTime = gpGlobals->curtime;
		m_flStopTalkTimeWithoutDelay = gpGlobals->curtime;

		CAI_TimedSemaphore *pSemaphore = GetMySpeechSemaphore( GetOuter() );
		if ( pSemaphore )
		{
			if ( pSemaphore->GetOwner() == GetOuter() )
			{
				pSemaphore->Release();
			}
		}
	}
}

//-------------------------------------

bool CAI_Expresser::IsSpeaking( void )
{
	if ( m_flStopTalkTime > gpGlobals->curtime )
		SpeechMsg( GetOuter(), "IsSpeaking() %f\n", m_flStopTalkTime - gpGlobals->curtime );

	if ( m_flLastTimeAcceptedSpeak == gpGlobals->curtime ) // only one speak accepted per think
		return true;

	return ( m_flStopTalkTime > gpGlobals->curtime );
}

//-------------------------------------

bool CAI_Expresser::CanSpeak()
{
	if ( m_flLastTimeAcceptedSpeak == gpGlobals->curtime ) // only one speak accepted per think
		return false;

	float timeOk = MAX( m_flStopTalkTime, m_flBlockedTalkTime );
	return ( timeOk <= gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if it's ok for this entity to speak after himself.
//			The base CanSpeak() includes the default speech delay, and won't
//			return true until that delay time has passed after finishing the
//			speech. This returns true as soon as the speech finishes.
//-----------------------------------------------------------------------------
bool CAI_Expresser::CanSpeakAfterMyself()
{
	if ( m_flLastTimeAcceptedSpeak == gpGlobals->curtime ) // only one speak accepted per think
		return false;

	float timeOk = MAX( m_flStopTalkTimeWithoutDelay, m_flBlockedTalkTime );
	return ( timeOk <= gpGlobals->curtime );
}

//-------------------------------------
bool CAI_Expresser::CanSpeakConcept( AIConcept_t concept )
{
	// Not in history?
	int iter = m_ConceptHistories.Find( concept );
	if ( iter == m_ConceptHistories.InvalidIndex() )
	{
		return true;
	}

	ConceptHistory_t *history = &m_ConceptHistories[iter];
	Assert( history );

	AI_Response *response = history->response;
	if ( !response )
		return true;

	if ( response->GetSpeakOnce() ) 
		return false;

	float respeakDelay = response->GetRespeakDelay();

	if ( respeakDelay != 0.0f )
	{
		if ( history->timeSpoken != -1 && ( gpGlobals->curtime < history->timeSpoken + respeakDelay ) )
			return false;
	}

	return true;
}

//-------------------------------------

bool CAI_Expresser::SpokeConcept( AIConcept_t concept )
{
	return GetTimeSpokeConcept( concept ) != -1.f;
}

//-------------------------------------

float CAI_Expresser::GetTimeSpokeConcept( AIConcept_t concept )
{
	int iter = m_ConceptHistories.Find( concept );
	if ( iter == m_ConceptHistories.InvalidIndex() ) 
		return -1;
	
	ConceptHistory_t *h = &m_ConceptHistories[iter];
	return h->timeSpoken;
}

//-------------------------------------

void CAI_Expresser::SetSpokeConcept( AIConcept_t concept, AI_Response *response, bool bCallback )
{
	int idx = m_ConceptHistories.Find( concept );
	if ( idx == m_ConceptHistories.InvalidIndex() )
	{
		ConceptHistory_t h;
		h.timeSpoken = gpGlobals->curtime;
		idx = m_ConceptHistories.Insert( concept, h );
	}

	ConceptHistory_t *slot = &m_ConceptHistories[ idx ];

	slot->timeSpoken = gpGlobals->curtime;
	// Update response info
	if ( response )
	{
		AI_Response *r = slot->response;
		if ( r )
		{
			delete r;
		}

		// FIXME:  Are we leaking AI_Responses?
		slot->response = response;
	}

	if ( bCallback )
		GetSink()->OnSpokeConcept( concept, response );
}

//-------------------------------------

void CAI_Expresser::ClearSpokeConcept( AIConcept_t concept )
{
	m_ConceptHistories.Remove( concept );
}

//-------------------------------------

void CAI_Expresser::DumpHistories()
{
	int c = 1;
	for ( int i = m_ConceptHistories.First(); i != m_ConceptHistories.InvalidIndex(); i = m_ConceptHistories.Next(i ) )
	{
		ConceptHistory_t *h = &m_ConceptHistories[ i ];

		DevMsg( "%i: %s at %f\n", c++, m_ConceptHistories.GetElementName( i ), h->timeSpoken );
	}
}

//-------------------------------------

bool CAI_Expresser::IsValidResponse( ResponseType_t type, const char *pszValue )
{
	if ( type == RESPONSE_SCENE )
	{
		char szInstanceFilename[256];
		GetOuter()->GenderExpandString( pszValue, szInstanceFilename, sizeof( szInstanceFilename ) );
		return ( GetSceneDuration( szInstanceFilename ) > 0 );
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_TimedSemaphore *CAI_Expresser::GetMySpeechSemaphore( CBaseEntity *pNpc ) 
{
	if ( !pNpc->MyNPCPointer() )
		return NULL;

	return (pNpc->MyNPCPointer()->IsPlayerAlly() ? &g_AIFriendliesTalkSemaphore : &g_AIFoesTalkSemaphore );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_Expresser::SpeechMsg( CBaseEntity *pFlex, const char *pszFormat, ... )
{
	if ( !DebuggingSpeech() )
		return;

	char string[ 2048 ];
	va_list argptr;
	va_start( argptr, pszFormat );
	Q_vsnprintf( string, sizeof(string), pszFormat, argptr );
	va_end( argptr );

	if ( pFlex->MyNPCPointer() )
	{
		DevMsg( pFlex->MyNPCPointer(), string );
	}
	else 
	{
		DevMsg( "%s", string );
	}
	UTIL_LogPrintf( string );
}


//-----------------------------------------------------------------------------

void CAI_ExpresserHost_NPC_DoModifyOrAppendCriteria( CAI_BaseNPC *pSpeaker, AI_CriteriaSet& set )
{
	// Append current activity name
	const char *pActivityName = pSpeaker->GetActivityName( pSpeaker->GetActivity() );
	if ( pActivityName )
	{
  		set.AppendCriteria( "activity", pActivityName );
	}

	static const char *pStateNames[] = { "None", "Idle", "Alert", "Combat", "Scripted", "PlayDead", "Dead" };
	if ( (int)pSpeaker->m_NPCState < ARRAYSIZE(pStateNames) )
	{
		set.AppendCriteria( "npcstate", UTIL_VarArgs( "[NPCState::%s]", pStateNames[pSpeaker->m_NPCState] ) );
	}

	if ( pSpeaker->GetEnemy() )
	{
		set.AppendCriteria( "enemy", pSpeaker->GetEnemy()->GetClassname() );
		set.AppendCriteria( "timesincecombat", "-1" );
	}
	else
	{
		if ( pSpeaker->GetLastEnemyTime() == 0.0 )
			set.AppendCriteria( "timesincecombat", "999999.0" );
		else
			set.AppendCriteria( "timesincecombat", UTIL_VarArgs( "%f", gpGlobals->curtime - pSpeaker->GetLastEnemyTime() ) );
	}

	set.AppendCriteria( "speed", UTIL_VarArgs( "%.3f", pSpeaker->GetSmoothedVelocity().Length() ) );

	CBaseCombatWeapon *weapon = pSpeaker->GetActiveWeapon();
	if ( weapon )
	{
		set.AppendCriteria( "weapon", weapon->GetClassname() );
	}
	else
	{
		set.AppendCriteria( "weapon", "none" );
	}

	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer )
	{
		Vector distance = pPlayer->GetAbsOrigin() - pSpeaker->GetAbsOrigin();

		set.AppendCriteria( "distancetoplayer", UTIL_VarArgs( "%f", distance.Length() ) );

	}
	else
	{
		set.AppendCriteria( "distancetoplayer", UTIL_VarArgs( "%i", MAX_COORD_RANGE ) );
	}

	if ( pSpeaker->HasCondition( COND_SEE_PLAYER ) )
	{
		set.AppendCriteria( "seeplayer", "1" );
	}
	else
	{
		set.AppendCriteria( "seeplayer", "0" );
	}

	if ( pPlayer && pPlayer->FInViewCone( pSpeaker ) && pPlayer->FVisible( pSpeaker ) )
	{
		set.AppendCriteria( "seenbyplayer", "1" );
	}
	else
	{
		set.AppendCriteria( "seenbyplayer", "0" );
	}
}

//-----------------------------------------------------------------------------

//=============================================================================
// HPE_BEGIN:
// [Forrest] Remove npc_speakall from Counter-Strike.
//=============================================================================
#ifndef CSTRIKE_DLL
extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
CON_COMMAND( npc_speakall, "Force the npc to try and speak all their responses" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBaseEntity *pEntity;

	if ( args[1] && *args[1] )
	{
		pEntity = gEntList.FindEntityByName( NULL, args[1], NULL );
		if ( !pEntity )
		{
			pEntity = gEntList.FindEntityByClassname( NULL, args[1] );
		}
	}
	else
	{
		pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	}
		
	if ( pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pNPC)
		{
			if ( pNPC->GetExpresser() )
			{
				bool save = engine->LockNetworkStringTables( false );
				pNPC->GetExpresser()->TestAllResponses();
				engine->LockNetworkStringTables( save );
			}
		}
	}
}
#endif
//=============================================================================
// HPE_END
//=============================================================================

//-----------------------------------------------------------------------------

CMultiplayer_Expresser::CMultiplayer_Expresser( CBaseFlex *pOuter ) : CAI_Expresser( pOuter )
{
	m_bAllowMultipleScenes = false;
}

bool CMultiplayer_Expresser::IsSpeaking( void )
{
	if ( m_bAllowMultipleScenes )
	{
		return false;
	}

	return CAI_Expresser::IsSpeaking();
}


void CMultiplayer_Expresser::AllowMultipleScenes()
{
	m_bAllowMultipleScenes = true;
}

void CMultiplayer_Expresser::DisallowMultipleScenes()
{
	m_bAllowMultipleScenes = false;
}