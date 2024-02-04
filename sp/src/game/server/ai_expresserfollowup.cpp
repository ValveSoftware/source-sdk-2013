//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "ai_speech.h"

#include "game.h"
#include "eventqueue.h"
#include "ai_basenpc.h"
#include "basemultiplayerplayer.h"
#include "ai_baseactor.h"
#include "sceneentity.h"
//#include "flex_expresser.h"
/*
#include "engine/ienginesound.h"
#include "keyvalues.h"
#include "ai_criteria.h"
#include "isaverestore.h"
#include "sceneentity.h"
*/



// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

static const char *GetResponseName( CBaseEntity *pEnt )
{
	Assert( pEnt );
	if ( pEnt == NULL )
		return "";
	return STRING( pEnt->GetEntityName() );
}

// This is a tiny helper function for below -- what I'd use a lambda for, usually
static void DispatchComeback( CAI_ExpresserWithFollowup *pExpress, CBaseEntity *pSpeaker, CBaseEntity *pRespondent, AI_ResponseFollowup &followup )
{
	AssertMsg(pSpeaker != NULL, "Response expressor somehow got called with a NULL Outer.\n");
	if ( !pRespondent )
	{
		return;
	}

	float delay = followup.followup_delay;
	if (pSpeaker == pRespondent && delay < 0)
	{
		Warning("Response rule with a 'self' target specified negative delay, which isn't legal because that would make someone talk over himself.");
		delay = 0;
	}

	//	Msg( "%s: Dispatch comeback about %s to %s\n", pSpeaker->GetBotString(), g_pConceptManager->GetTopicName( handle ), pRespondent->GetBotString() );

	// build an input event that we will use to force the bot to talk through the IO system
	variant_t value;
	// Don't send along null contexts
	if (followup.followup_contexts && followup.followup_contexts[0] != '\0')
	{
		value.SetString( MAKE_STRING( followup.followup_contexts ) );
		g_EventQueue.AddEvent( pRespondent, "AddContext", value, delay - 0.01, pSpeaker, pSpeaker );
	}

	/*
	value.SetString(MAKE_STRING(followup.followup_concept));
	g_EventQueue.AddEvent( pRespondent, "SpeakResponseConcept", value, delay , pSpeaker, pSpeaker );
	*/

	AI_CriteriaSet criteria;

	// add in the FROM context so dispatchee knows was from me
	const char * RESTRICT pszSpeakerName = GetResponseName( pSpeaker );
	criteria.AppendCriteria( "From", pszSpeakerName );
#ifdef MAPBASE
	// See DispatchFollowupThroughQueue()
	criteria.AppendCriteria( "From_idx", CNumStr( pSpeaker->entindex() ) );
	criteria.AppendCriteria( "From_class", pSpeaker->GetClassname() );
#endif
	// if a SUBJECT criteria is missing, put it back in.
	if ( criteria.FindCriterionIndex( "Subject" ) == -1 )
	{
		criteria.AppendCriteria( "Subject", pszSpeakerName );
	}

	// add in any provided contexts from the parameters onto the ones stored in the followup
	criteria.Merge( followup.followup_contexts );

#ifdef MAPBASE
	if (CAI_ExpresserSink *pSink = dynamic_cast<CAI_ExpresserSink *>(pRespondent))
	{
		criteria.AppendCriteria( "dist_from_issuer",  UTIL_VarArgs( "%f", (pRespondent->GetAbsOrigin() - pSpeaker->GetAbsOrigin()).Length() ) );
		g_ResponseQueueManager.GetQueue()->AppendFollowupCriteria( followup.followup_concept, criteria, pSink->GetSinkExpresser(), pSink, pRespondent, pSpeaker, kDRT_SPECIFIC );

		pSink->Speak( followup.followup_concept, &criteria );
	}
#else
	// This is kludgy and needs to be fixed in class hierarchy, but for now, try to guess at the most likely
	// kinds of targets and dispatch to them.
	if (CBaseMultiplayerPlayer *pPlayer = dynamic_cast<CBaseMultiplayerPlayer *>(pRespondent))
	{
		pPlayer->Speak( followup.followup_concept, &criteria );
	}

	else if (CAI_BaseActor *pActor = dynamic_cast<CAI_BaseActor *>(pRespondent))
	{
		pActor->Speak( followup.followup_concept, &criteria );
	}
#endif
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: Placeholder for rules based response system
// Input  : concept - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_ExpresserWithFollowup::Speak( AIConcept_t &concept, const char *modifiers /*= NULL*/, char *pszOutResponseChosen /* = NULL*/, size_t bufsize /* = 0 */, IRecipientFilter *filter /* = NULL */ )
{
	AI_Response *result = SpeakFindResponse( concept, modifiers );
	if ( !result )
	{
		return false;
	}

	CNPC_CompanionBot *pBot = dynamic_cast<CNPC_CompanionBot *>(GetOuter());
	if ( pBot )
	{
		pBot->SetConversationTopic( g_pConceptManager->GetTopic( handle ) );
		pBot->SetLastSpeaker( g_pConceptManager->GetSpeaker( handle ) );
		//		Msg( "%s: Conversing about %s\n", pBot->GetBotString(), g_pConceptManager->GetTopicName( handle ) );
	}

	SpeechMsg( GetOuter(), "%s (%x) spoke %s (%f)\n", STRING(GetOuter()->GetEntityName()), GetOuter(), g_pConceptManager->GetConcept( handle ), gpGlobals->curtime );

	bool spoke = SpeakDispatchResponse( handle, result, filter );
	if ( pszOutResponseChosen )
	{
		result->GetResponse( pszOutResponseChosen, bufsize );
	}

	return spoke;
}
#endif


// Work out the character from the "subject" context.
// Right now, this is a simple find by entity name search.
// But you can define arbitrary subject names, like L4D does
// for "biker", "manager", etc.
static CBaseEntity *AscertainSpeechSubjectFromContext( AI_Response *response, AI_CriteriaSet &criteria, const char *pContextName )
{
	const char *subject = criteria.GetValue( criteria.FindCriterionIndex( pContextName ) );
	if (subject)
	{
		CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, subject );

#ifdef MAPBASE
		// Allow entity indices to be used (see DispatchFollowupThroughQueue() for one particular use case)
		if (!pEnt && atoi(subject))
		{
			pEnt = CBaseEntity::Instance( atoi( subject ) );
		}
#endif

		return pEnt;

	}
	else
	{
		return NULL;
	}
}

// TODO: Currently uses awful stricmp. Use symbols! Once I know which ones we want, that is. 
static CResponseQueue::CFollowupTargetSpec_t ResolveFollowupTargetToEntity( AIConcept_t &concept, AI_CriteriaSet &criteria, const char * RESTRICT szTarget, AI_Response * RESTRICT response = NULL )
{



	if ( Q_stricmp(szTarget, "self") == 0 )
	{
		return CResponseQueue::CFollowupTargetSpec_t( kDRT_SPECIFIC, concept.GetSpeaker() );
	}
	else if ( Q_stricmp(szTarget, "subject") == 0 )
	{
		return CResponseQueue::CFollowupTargetSpec_t( AscertainSpeechSubjectFromContext( response, criteria, "Subject" ) );
	}
	else if ( Q_stricmp(szTarget, "from") == 0 )
	{
#ifdef MAPBASE
		// See DispatchFollowupThroughQueue()
		return CResponseQueue::CFollowupTargetSpec_t( AscertainSpeechSubjectFromContext( response, criteria, "From_idx" ) );
#else
		return CResponseQueue::CFollowupTargetSpec_t( AscertainSpeechSubjectFromContext( response, criteria, "From" ) );
#endif
	}
	else if ( Q_stricmp(szTarget, "any") == 0 )
	{
		return CResponseQueue::CFollowupTargetSpec_t( kDRT_ANY, concept.GetSpeaker() );
	}
	else if ( Q_stricmp(szTarget, "all") == 0 )
	{
		return CResponseQueue::CFollowupTargetSpec_t( kDRT_ALL );
	}

	// last resort, try a named lookup
#ifdef MAPBASE
	else if ( CBaseEntity *pSpecific = gEntList.FindEntityByName(NULL, szTarget, concept.GetSpeaker()) ) // it could be anything
#else
	else if ( CBaseEntity *pSpecific = gEntList.FindEntityByName(NULL, szTarget) ) // it could be anything
#endif
	{
		return CResponseQueue::CFollowupTargetSpec_t( pSpecific );
	}

	Warning("Couldn't resolve response target %s\n", szTarget );
	return CResponseQueue::CFollowupTargetSpec_t(); // couldn't resolve.
}


// TODO: Currently uses awful stricmp. Use symbols! Once I know which ones we want, that is. 
static CResponseQueue::CFollowupTargetSpec_t ResolveFollowupTargetToEntity( AIConcept_t &concept, AI_CriteriaSet &criteria, AI_Response * RESTRICT response, AI_ResponseFollowup * RESTRICT followup )
{
	const char * RESTRICT szTarget = followup->followup_target;
	const CResponseQueue::CFollowupTargetSpec_t INVALID; // default: invalid result
	if ( szTarget == NULL )
		return INVALID; 
	else
		return ResolveFollowupTargetToEntity( concept, criteria, szTarget, response );
}


ConVar chet_debug_idle( "chet_debug_idle", "0", FCVAR_ARCHIVE, "If set one, many debug prints to help track down the TLK_IDLE issue. Set two for super verbose info" );
// extern ConVar chet_debug_idle;
bool CAI_ExpresserWithFollowup::Speak( AIConcept_t concept, const char *modifiers /*= NULL*/, char *pszOutResponseChosen /* = NULL*/, size_t bufsize /* = 0 */, IRecipientFilter *filter /* = NULL */ )
{
	VPROF("CAI_Expresser::Speak");
	if ( IsSpeechGloballySuppressed() )
	{
		return false;
	}

	concept.SetSpeaker(GetOuter());
	AI_CriteriaSet criteria;
	GatherCriteria(&criteria, concept, modifiers);
	GetOuter()->ModifyOrAppendDerivedCriteria(criteria);
	AI_Response result;
	if ( !FindResponse( result, concept, &criteria ) )
	{
		if (chet_debug_idle.GetBool())
		{

			const char *name = GetOuter()->GetDebugName();

			Msg( "TLK_IDLE: %s did not FindResponse\n", name );
		}	
		return false;
	}
	else
	{
		if (chet_debug_idle.GetBool())
		{


			const char *name = GetOuter()->GetDebugName();

			Msg( "TLK_IDLE: %s SUCCESSFUL FindResponse\n", name );
		}
	}

	SpeechMsg( GetOuter(), "%s (%p) spoke %s (%f)", STRING(GetOuter()->GetEntityName()), GetOuter(), (const char*)concept, gpGlobals->curtime );
	// Msg( "%s:%s to %s:%s\n", GetOuter()->GetDebugName(), concept.GetStringConcept(), criteria.GetValue(criteria.FindCriterionIndex("Subject")), pTarget ? pTarget->GetDebugName() : "none" );

	bool spoke = SpeakDispatchResponse( concept, &result, &criteria, filter );
	if ( pszOutResponseChosen )
	{
		result.GetResponse( pszOutResponseChosen, bufsize );
	}

	return spoke;
}

extern ISoundEmitterSystemBase* soundemitterbase;

static float GetSpeechDurationForResponse( const AI_Response * RESTRICT response, const char *szActorModel)
{
	switch (response->GetType())
	{
	case ResponseRules::RESPONSE_SCENE:
	{
		char szScene[MAX_PATH];
		soundemitterbase->GenderExpandString(szActorModel, response->GetResponsePtr(), szScene, MAX_PATH);
		return GetSceneSpeechDuration(szScene);
	}
		break;
	default:
		break;
	}

	return 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: Dispatches the result
// Input  : *response - 
//-----------------------------------------------------------------------------
bool CAI_ExpresserWithFollowup::SpeakDispatchResponse( AIConcept_t concept, AI_Response *response, AI_CriteriaSet *criteria, IRecipientFilter *filter )
{
	// This gives the chance for the other bot to respond.
	if ( !concept.GetSpeaker().IsValid() )
	{
		concept.SetSpeaker(GetOuter());
	}

	bool bInterrupted = IsSpeaking();
	bool bSuc = CAI_Expresser::SpeakDispatchResponse( concept, response, criteria, filter );
	if (!bSuc)
	{
		return false;
	}

	if ( bInterrupted )
	{
		g_ResponseQueueManager.GetQueue()->RemoveSpeechQueuedFor( GetOuter() );
	}

	// Record my followup details so that I may defer its use til end of the speech
	AI_ResponseFollowup * RESTRICT followup = response->GetParams()->m_pFollowup;
	if ( followup  )
	{
		if ( followup->followup_entityiotarget && followup->followup_entityioinput )
		{
#ifdef MAPBASE
			CBaseEntity * RESTRICT pTarget = ResolveFollowupTargetToEntity( concept, *criteria, followup->followup_entityiotarget, response ).m_hHandle;
#else
			CBaseEntity * RESTRICT pTarget = gEntList.FindEntityByName( NULL, followup->followup_entityiotarget );
#endif
			if ( pTarget )
			{
				g_EventQueue.AddEvent( pTarget, followup->followup_entityioinput, variant_t(), followup->followup_entityiodelay, GetOuter(), GetOuter() );
			}
		}
		if ( followup->IsValid() )
		{
			// 11th hour change: rather than trigger followups from the end of a VCD,
			// instead fire it from the end of the last speech event in the VCD, because
			// there's a multisecond facial relax delay built into the scene. 
			// The speech length is stored in the cache, so we can post the followup now.
			if ( response->GetType() == ResponseRules::RESPONSE_SCENE && 
				 followup->followup_delay >= 0 )
			{
				float fTimeToLastSpeech = GetSpeechDurationForResponse( response, STRING(GetOuter()->GetModelName()) );
				// failsafe
				if ( fTimeToLastSpeech > 0 )
				{	
					DispatchFollowupThroughQueue( followup->followup_concept, followup->followup_contexts, 
						ResolveFollowupTargetToEntity( concept, *criteria, response, followup ), 
						fTimeToLastSpeech + followup->followup_delay, GetOuter() );
				}
				else  // error
				{
					// old way, copied from "else" below
					m_pPostponedFollowup = followup;
					if ( criteria )
						m_followupTarget = ResolveFollowupTargetToEntity( concept, *criteria, response, m_pPostponedFollowup );
					else
					{
						AI_CriteriaSet tmpCriteria;
						m_followupTarget = ResolveFollowupTargetToEntity( concept, tmpCriteria, response, m_pPostponedFollowup );
					}
				}
			}
			else if ( followup->followup_delay < 0 )
			{
				// a negative delay has a special meaning. Usually the comeback dispatches after
				// the currently said line is finished; the delay is added to that, to provide a 
				// pause between when character A finishes speaking and B begins. 
				// A negative delay (-n) actually means "dispatch the comeback n seconds
				// after I start talking".
				// In this case we do not need to postpone the followup; we just throw it directly
				// into the queue.
				DispatchFollowupThroughQueue( followup->followup_concept, followup->followup_contexts, 
					ResolveFollowupTargetToEntity( concept, *criteria, response, followup ), 
					-followup->followup_delay, GetOuter() );
			}
#ifndef MAPBASE // RESPONSE_PRINT now notes speaking time
			else if ( response->GetType() == ResponseRules::RESPONSE_PRINT )
			{	// zero-duration responses dispatch immediately via the queue (must be the queue bec.
				// the m_pPostponedFollowup will never trigger)
				DispatchFollowupThroughQueue( followup->followup_concept, followup->followup_contexts, 
					ResolveFollowupTargetToEntity( concept, *criteria, response, followup ), 
					followup->followup_delay, GetOuter() );
			}
#endif
			else
			{
				// this is kind of a quick patch to immediately deal with the issue of null criteria 
				// (arose while branching to main) without replumbing a bunch of stuff --  to be fixed
				// 5.13.08 egr
				m_pPostponedFollowup = followup;
				if ( criteria )
					m_followupTarget = ResolveFollowupTargetToEntity( concept, *criteria, response, m_pPostponedFollowup );
				else
				{
					AI_CriteriaSet tmpCriteria;
					m_followupTarget = ResolveFollowupTargetToEntity( concept, tmpCriteria, response, m_pPostponedFollowup );
				}
			}
		}
	}


	return bSuc;
}

// This is a gimmick used when a negative delay is specified in a followup, which is a shorthand
// for "this many seconds after the beginning of the line" rather than "this may seconds after the end
// of the line", eg to create a THEN rule when two characters talk over each other.
// It's static to avoid accidental use of the postponed followup/target members.
void CAI_ExpresserWithFollowup::DispatchFollowupThroughQueue( const AIConcept_t &concept,
															  const char * RESTRICT criteriaStr,
															  const CResponseQueue::CFollowupTargetSpec_t &target,
															  float delay,
															  CBaseEntity * RESTRICT pOuter
															 )
{
	AI_CriteriaSet criteria;
	// Don't add my own criteria! GatherCriteria( &criteria, followup.followup_concept, followup.followup_contexts );

	criteria.AppendCriteria( "From", STRING( pOuter->GetEntityName() ) );
#ifdef MAPBASE
	// The index of the "From" entity.
	// In HL2 mods, many followup users would be generic NPCs (e.g. citizens) who might not have any particular significance.
	// Those generic NPCs are quite likely to have no name or have a name in common with other entities. As a result, Mapbase
	// changes internal operations of the "From" context to search for an entity index. This won't be 100% reliable if the source
	// talker dies and another entity is created immediately afterwards, but it's a lot more reliable than a simple entity name search.
	criteria.AppendCriteria( "From_idx", CNumStr( pOuter->entindex() ) );

	// Generic NPCs should also be attributable by classname
	criteria.AppendCriteria( "From_class", pOuter->GetClassname() );
#endif

	criteria.Merge( criteriaStr );
	g_ResponseQueueManager.GetQueue()->Add( concept, &criteria, gpGlobals->curtime + delay, target, pOuter );
}

//-----------------------------------------------------------------------------
// Purpose: Handles the new concept objects
//-----------------------------------------------------------------------------
void CAI_ExpresserWithFollowup::SpeakDispatchFollowup( AI_ResponseFollowup &followup )
{
	if ( !m_followupTarget.IsValid() )
		return;

	// If a specific entity target is given, use the old pathway for now
	if ( m_followupTarget.m_iTargetType == kDRT_SPECIFIC && followup.followup_delay == 0 )
	{
		CBaseEntity *pTarget = m_followupTarget.m_hHandle.Get();
		if (!pTarget)
		{
			return;
		}
		DispatchComeback( this, GetOuter(), pTarget, followup );
	}
	else
	{
		DispatchFollowupThroughQueue( followup.followup_concept, followup.followup_contexts, m_followupTarget, followup.followup_delay, GetOuter() );
	}
	// clear out the followup member just in case.
	m_pPostponedFollowup = NULL;
	m_followupTarget.m_iTargetType = kDRT_MAX;
}

void CAI_ExpresserWithFollowup::OnSpeechFinished()
{
	if (m_pPostponedFollowup && m_pPostponedFollowup->IsValid())
	{
#ifdef MAPBASE
		// HACKHACK: Non-scene speech (e.g. noscene speak/sentence) fire OnSpeechFinished() immediately,
		// so add the actual speech time to the followup delay
		if (GetTimeSpeechCompleteWithoutDelay() > gpGlobals->curtime)
			m_pPostponedFollowup->followup_delay += GetTimeSpeechCompleteWithoutDelay() - gpGlobals->curtime;
#endif
		return SpeakDispatchFollowup(*m_pPostponedFollowup);
	}
}




void CC_RR_ForceConcept_f( const CCommand &args )
{
	if ( args.ArgC() < 3 )
	{
		Msg("USAGE: rr_forceconcept <target> <concept> \"criteria1:value1,criteria2:value2,...\"\n");
		return;
	}

	AI_CriteriaSet criteria;
	if ( args.ArgC() >= 3 )
	{
		const char *criteriastring = args[3];
		criteria.Merge( criteriastring );
	}

	AIConcept_t concept( args[2] );
	QueueSpeak( concept, ResolveFollowupTargetToEntity( concept, criteria, args[1] ), criteria );
}


static ConCommand rr_forceconcept( "rr_forceconcept", CC_RR_ForceConcept_f, 
								  "fire a response concept directly at a given character.\n"
								  "USAGE: rr_forceconcept <target> <concept> \"criteria1:value1,criteria2:value2,...\"\n"
								  "criteria values are optional.\n"

								  , FCVAR_CHEAT );
