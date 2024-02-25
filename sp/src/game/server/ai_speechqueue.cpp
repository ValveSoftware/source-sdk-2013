//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "basemultiplayerplayer.h"
#include "ai_baseactor.h"
#include "ai_speech.h"
//#include "flex_expresser.h"
#ifdef MAPBASE
#include "sceneentity.h"
#endif
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern ConVar ai_debug_speech;
#define DebuggingSpeech() ai_debug_speech.GetBool()
extern ConVar rr_debugresponses;

ConVar rr_followup_maxdist( "rr_followup_maxdist", "1800", FCVAR_CHEAT, "'then ANY' or 'then ALL' response followups will be dispatched only to characters within this distance." );

///////////////////////////////////////////////////////////////////////////////
//   RESPONSE QUEUE DATA STRUCTURE
///////////////////////////////////////////////////////////////////////////////

CResponseQueue::CResponseQueue( int queueSize ) : m_Queue(queueSize), m_ExpresserTargets(8,8)
{};

/// Add a deferred response. 
void CResponseQueue::Add( const AIConcept_t &concept,  ///< concept to dispatch
		 const AI_CriteriaSet * RESTRICT contexts, 
		 float time,					 ///< when to dispatch it. You can specify a time of zero to mean "immediately."
		 const CFollowupTargetSpec_t &targetspec,
		 CBaseEntity *pIssuer
		 )
{
	// Add a response.
	AssertMsg( m_Queue.Count() < AI_RESPONSE_QUEUE_SIZE, "AI Response queue overfilled." );
	QueueType_t::IndexLocalType_t idx = m_Queue.AddToTail();
	m_Queue[idx].Init( concept, contexts, time, targetspec, pIssuer );
}


/// Remove a deferred response matching the concept and issuer. 
void CResponseQueue::Remove( const AIConcept_t &concept,  ///< concept to dispatch
			CBaseEntity * const RESTRICT pIssuer		  ///< the entity issuing the response, if one exists.
			) RESTRICT
{
	// walk through the queue until we find a response matching the concept and issuer, then strike it.
	QueueType_t::IndexLocalType_t idx = m_Queue.Head();
	while (idx != m_Queue.InvalidIndex())
	{
		CDeferredResponse &response = m_Queue[idx];
		QueueType_t::IndexLocalType_t previdx = idx; // advance the index immediately because we may be deleting the "current" element
		idx = m_Queue.Next(idx); // is now the next index
		if ( CompareConcepts( response.m_concept, concept ) && // if concepts match and 
			( !pIssuer || ( response.m_hIssuer.Get() == pIssuer ) ) // issuer is null, or matches the one in the response
			)
		{
			m_Queue.Remove(previdx);
		}
	}
}


void CResponseQueue::RemoveSpeechQueuedFor( const CBaseEntity *pSpeaker )
{
	// walk through the queue until we find a response matching the speaker, then strike it.
	// because responses are dispatched from inside a loop that is already walking through the
	// queue, it's not safe to actually remove the elements. Instead, quash it by replacing it
	// with a null event.
	
	for ( QueueType_t::IndexLocalType_t idx = m_Queue.Head() ;
		  idx != m_Queue.InvalidIndex() ;
		  idx = m_Queue.Next(idx) ) // is now the next index
	{
		CDeferredResponse &response = m_Queue[idx];
		if ( response.m_Target.m_hHandle.Get() == pSpeaker )
		{
			response.Quash();
		}
	}
}

// TODO: use a more compact representation.
void CResponseQueue::DeferContextsFromCriteriaSet( DeferredContexts_t &contextsOut, const AI_CriteriaSet * RESTRICT criteriaIn )
{
	contextsOut.Reset();
	if (criteriaIn)
	{
		contextsOut.Merge(criteriaIn);
	}
}

void CResponseQueue::PerFrameDispatch()
{
failsafe:
	// Walk through the list, find any messages whose time has come, and dispatch them. Then remove them.
	QueueType_t::IndexLocalType_t idx = m_Queue.Head();
	while (idx != m_Queue.InvalidIndex())
	{
		// do we need to dispatch this concept?
		CDeferredResponse &response = m_Queue[idx];
		QueueType_t::IndexLocalType_t previdx = idx; // advance the index immediately because we may be deleting the "current" element
		idx = m_Queue.Next(idx); // is now the next index

		if ( response.IsQuashed() )
		{
			// we can delete this entry now
			m_Queue.Remove(previdx);
		}
		else if ( response.m_fDispatchTime <= gpGlobals->curtime )
		{
			// dispatch. we've had bugs where dispatches removed things from inside the queue; 
			// so, as a failsafe, if the queue length changes as a result, start over.
			int oldLength = m_Queue.Count();
			DispatchOneResponse(response);
			if ( m_Queue.Count() < oldLength )
			{
				AssertMsg( false, "Response queue length changed in non-reentrant way! FAILSAFE TRIGGERED" );
				goto failsafe; // ick
			}

			// we can delete this entry now
			m_Queue.Remove(previdx);
		}
	}
}


/// Add an expressor owner to this queue.
void CResponseQueue::AddExpresserHost(CBaseEntity *host)
{
	EHANDLE ehost(host);
	// see if it's in there already
	if (m_ExpresserTargets.HasElement(ehost))
	{
		AssertMsg1(false, "Tried to add %s to response queue when it was already in there.", host->GetDebugName());
	}
	else
	{
		// zip through the queue front to back, first see if there's any invalid handles to replace
		int count = m_ExpresserTargets.Count();
		for (int i = 0 ; i < count ; ++i )
		{
			if ( !m_ExpresserTargets[i].Get() )
			{
				m_ExpresserTargets[i] = ehost;
				return;
			}
		}

		// if we're down here we didn't find one to replace, so append the host to the end.
		m_ExpresserTargets.AddToTail(ehost);
	}
}

/// Remove an expresser host from this queue.
void CResponseQueue::RemoveExpresserHost(CBaseEntity *host)
{
	int idx = m_ExpresserTargets.Find(host);
	if (idx == -1)
	{
		// AssertMsg1(false, "Tried to remove %s from response queue, but it's not in there to begin with!", host->GetDebugName() );
	}
	else
	{
		m_ExpresserTargets.FastRemove(idx);
	}
}

#ifdef MAPBASE
/// Get the expresser for a base entity.
static CAI_Expresser *InferExpresserFromBaseEntity(CBaseEntity * RESTRICT pEnt, CAI_ExpresserSink **ppSink = NULL)
{
	if ( CAI_ExpresserSink *pSink = dynamic_cast<CAI_ExpresserSink *>(pEnt) )
	{
		if (ppSink)
			*ppSink = pSink;
		return pSink->GetSinkExpresser();
	}

	return NULL;
}
#else
/// Get the expresser for a base entity.
/// TODO: Kind of an ugly hack until I get the class hierarchy straightened out.
static CAI_Expresser *InferExpresserFromBaseEntity(CBaseEntity * RESTRICT pEnt)
{
	if ( CBaseMultiplayerPlayer *pPlayer = dynamic_cast<CBaseMultiplayerPlayer *>(pEnt) )
	{
		return pPlayer->GetExpresser();
	}
	else if ( CAI_BaseActor *pActor = dynamic_cast<CAI_BaseActor *>(pEnt) )
	{
		return pActor->GetExpresser();
	}
	/*
	else if ( CFlexExpresser *pFlex = dynamic_cast<CFlexExpresser *>(pEnt) )
	{
		return pFlex->GetExpresser();
	}
	*/
	else
	{
		return NULL;
	}
}
#endif


void CResponseQueue::CDeferredResponse::Quash()
{
	m_Target = CFollowupTargetSpec_t();
	m_fDispatchTime = 0;
}

#ifdef MAPBASE
void CResponseQueue::AppendFollowupCriteria( AIConcept_t concept, AI_CriteriaSet &set, CAI_Expresser *pEx,
		CAI_ExpresserSink *pSink, CBaseEntity *pTarget, CBaseEntity *pIssuer, DeferredResponseTarget_t nTargetType )
{
	// Allows control over which followups interrupt speech routines
	set.AppendCriteria( "followup_allowed_to_speak", (pSink->IsAllowedToSpeakFollowup( concept, pIssuer, nTargetType == kDRT_SPECIFIC )) ? "1" : "0" );

	set.AppendCriteria( "followup_target_type", UTIL_VarArgs( "%i", (int)nTargetType ) );
	
	// NOTE: This assumes any expresser entity derived from CBaseFlex is also derived from CBaseCombatCharacter
	if (pTarget->IsCombatCharacter())
		set.AppendCriteria( "is_speaking", (pEx->IsSpeaking() || IsRunningScriptedSceneWithSpeechAndNotPaused( assert_cast<CBaseFlex*>(pTarget) )) ? "1" : "0" );
	else
		set.AppendCriteria( "is_speaking", "0" );
}
#endif

bool CResponseQueue::DispatchOneResponse(CDeferredResponse &response)
{
	// find the target.
	CBaseEntity * RESTRICT pTarget = NULL;
	AI_CriteriaSet &deferredCriteria = response.m_contexts;
	CAI_Expresser * RESTRICT pEx = NULL;
	CBaseEntity * RESTRICT pIssuer = response.m_hIssuer.Get(); // MAY BE NULL
	float followupMaxDistSq;
	{
		/*
		CFlexExpresser * RESTRICT pOrator = CFlexExpresser::AsFlexExpresser( pIssuer );
		if ( pOrator )
		{
			// max dist is overridden. "0" means infinite distance (for orators only), 
			// anything else is a finite distance.
			if ( pOrator->m_flThenAnyMaxDist > 0 )
			{
				followupMaxDistSq = pOrator->m_flThenAnyMaxDist * pOrator->m_flThenAnyMaxDist;
			}
			else
			{
				followupMaxDistSq = FLT_MAX;
			}
			
		}
		else
		*/
		{
			followupMaxDistSq = rr_followup_maxdist.GetFloat(); // square of max audibility distance
	        followupMaxDistSq *= followupMaxDistSq;
		}
	}

	switch (response.m_Target.m_iTargetType)
	{
	case kDRT_SPECIFIC:
		{
			pTarget = response.m_Target.m_hHandle.Get();
		}
		break;
	case kDRT_ANY:
		{
			return DispatchOneResponse_ThenANY( response, &deferredCriteria, pIssuer, followupMaxDistSq  );
		}
		break;
	case kDRT_ALL:
		{
			bool bSaidAnything = false;
			Vector issuerLocation;
			if ( pIssuer ) 
			{ 
					issuerLocation = pIssuer->GetAbsOrigin(); 
			}

			// find all characters
			int numExprs = GetNumExpresserTargets();
			for ( int i = 0 ; i < numExprs; ++i ) 
			{
				pTarget = GetExpresserHost(i);
				float distIssuerToTargetSq = 0.0f;
				if ( pIssuer ) 
				{
					distIssuerToTargetSq = (pTarget->GetAbsOrigin() - issuerLocation).LengthSqr();
					if ( distIssuerToTargetSq > followupMaxDistSq )
						continue; // too far
				}

#ifdef MAPBASE
				CAI_ExpresserSink *pSink = NULL;
				pEx = InferExpresserFromBaseEntity( pTarget, &pSink );
#else
				pEx = InferExpresserFromBaseEntity(pTarget);
#endif
				if ( !pEx || pTarget == pIssuer ) 
					continue;

				AI_CriteriaSet characterCriteria;
				pEx->GatherCriteria(&characterCriteria, response.m_concept, NULL);
				characterCriteria.Merge(&deferredCriteria);
				if ( pIssuer ) 
				{
					characterCriteria.AppendCriteria( "dist_from_issuer",  UTIL_VarArgs( "%f", sqrt(distIssuerToTargetSq) ) );
				}

#ifdef MAPBASE
				AppendFollowupCriteria( response.m_concept, characterCriteria, pEx, pSink, pTarget, pIssuer, kDRT_ALL );
#endif

				AI_Response prospectiveResponse;
				if ( pEx->FindResponse( prospectiveResponse, response.m_concept, &characterCriteria ) )
				{
					// dispatch it
					bSaidAnything = pEx->SpeakDispatchResponse(response.m_concept, &prospectiveResponse, &deferredCriteria) || bSaidAnything ;
				}
			}

			return bSaidAnything;

		}
		break;
	default:
		// WTF?
		AssertMsg1( false, "Unknown deferred response type %d\n", response.m_Target.m_iTargetType );
		return false;
	}

	if (!pTarget)
		return false; // we're done right here.

	// Get the expresser for the target. 
#ifdef MAPBASE
	CAI_ExpresserSink *pSink = NULL;
	pEx = InferExpresserFromBaseEntity( pTarget, &pSink );
#else
	pEx = InferExpresserFromBaseEntity(pTarget);
#endif
	if (!pEx)
		return false;

	AI_CriteriaSet characterCriteria;
	pEx->GatherCriteria(&characterCriteria, response.m_concept, NULL);
	characterCriteria.Merge(&deferredCriteria);
#ifdef MAPBASE
	if ( pIssuer )
	{
		characterCriteria.AppendCriteria( "dist_from_issuer",  UTIL_VarArgs( "%f", (pTarget->GetAbsOrigin() - pIssuer->GetAbsOrigin()).Length() ) );
	}

	AppendFollowupCriteria( response.m_concept, characterCriteria, pEx, pSink, pTarget, pIssuer, kDRT_SPECIFIC );
#endif
	pEx->Speak( response.m_concept, &characterCriteria );
	
	return true;
}

// 
ConVar rr_thenany_score_slop( "rr_thenany_score_slop", "0.0", FCVAR_CHEAT, "When computing respondents for a 'THEN ANY' rule, all rule-matching scores within this much of the best score will be considered." );
#define EXARRAYMAX 32 // maximum number of prospective expressers in the array (hardcoded for simplicity)
bool CResponseQueue::DispatchOneResponse_ThenANY( CDeferredResponse &response, AI_CriteriaSet * RESTRICT pDeferredCriteria, CBaseEntity *  const RESTRICT pIssuer, float followupMaxDistSq )
{
	CBaseEntity * RESTRICT pTarget = NULL;
	CAI_Expresser * RESTRICT pEx = NULL;
	float bestScore = 0;
	float slop = rr_thenany_score_slop.GetFloat();
	Vector issuerLocation;
	if ( pIssuer )
	{
		issuerLocation = pIssuer->GetAbsOrigin();
	}

	// this is an array of prospective respondents.
	CAI_Expresser * RESTRICT pBestEx[EXARRAYMAX];
	AI_Response responseToSay[EXARRAYMAX];
	int numExFound = 0; // and this is the high water mark for the array.

	// Here's the algorithm: we're going to walk through all the characters, finding the
	// highest scoring ones for this rule. Let the highest score be called k. 
	// Because there may be (n) many characters all scoring k, we store an array of
	// all characters with score k, then choose randomly from that array at return. 
	// We also define an allowable error for k in the global cvar
	// rr_thenany_score_slop , which may be zero.

	// find all characters (except the issuer)
	int numExprs = GetNumExpresserTargets();
	AssertMsg1( numExprs <= EXARRAYMAX, "Response queue has %d possible expresser targets, please increase EXARRAYMAX ", numExprs );
	for ( int i = 0 ; i < numExprs; ++i ) 
	{
		pTarget = GetExpresserHost(i);
		if ( pTarget == pIssuer )
			continue; // don't dispatch to myself

		if ( !pTarget->IsAlive() )
			continue; // dead men tell no tales

		float distIssuerToTargetSq = 0.0f;
		if ( pIssuer )
		{
			distIssuerToTargetSq = (pTarget->GetAbsOrigin() - issuerLocation).LengthSqr();
			if ( distIssuerToTargetSq > followupMaxDistSq )
				continue; // too far
		}

#ifdef MAPBASE
		CAI_ExpresserSink *pSink = NULL;
		pEx = InferExpresserFromBaseEntity( pTarget, &pSink );
#else
		pEx = InferExpresserFromBaseEntity(pTarget);
#endif
		if ( !pEx  ) 
			continue;

		AI_CriteriaSet characterCriteria;
		pEx->GatherCriteria(&characterCriteria, response.m_concept, NULL);
		characterCriteria.Merge( pDeferredCriteria );
		pTarget->ModifyOrAppendDerivedCriteria( characterCriteria );
		if ( pIssuer )
		{
			characterCriteria.AppendCriteria( "dist_from_issuer",  UTIL_VarArgs( "%f", sqrt(distIssuerToTargetSq) ) );
		}

#ifdef MAPBASE
		AppendFollowupCriteria( response.m_concept, characterCriteria, pEx, pSink, pTarget, pIssuer, kDRT_ANY );
#endif

		AI_Response prospectiveResponse;

#ifdef MAPBASE
		pEx->SetUsingProspectiveResponses( true );
#endif

		if ( pEx->FindResponse( prospectiveResponse, response.m_concept, &characterCriteria ) )
		{
			float score = prospectiveResponse.GetMatchScore();
			if ( score > 0 && !prospectiveResponse.IsEmpty() ) // ignore scores that are zero, regardless of slop
			{	
				// if this score is better than all we've seen (outside the slop), then replace the array with 
				// an entry just to this expresser
				if ( score > bestScore + slop )
				{
					responseToSay[0] = prospectiveResponse;
					pBestEx[0] = pEx;
					bestScore = score;
					numExFound = 1;
				}
				else if ( score >= bestScore - slop ) // if this score is at least as good as the best we've seen, but not better than all 
				{
					if ( numExFound >= EXARRAYMAX )
					{
#ifdef MAPBASE
						pEx->SetUsingProspectiveResponses( false );
#endif
						continue;  // SAFETY: don't overflow the array
					}

					responseToSay[numExFound] = prospectiveResponse;
					pBestEx[numExFound] = pEx;
					bestScore = fpmax( score, bestScore );
					numExFound += 1;
				}
			}
		}

#ifdef MAPBASE
		pEx->SetUsingProspectiveResponses( false );
#endif
	}

	// if I have a response, dispatch it.
	if ( numExFound > 0 )
	{
		// get a random number between 0 and the responses found
		int iSelect = numExFound > 1 ? RandomInt( 0, numExFound - 1 ) : 0;

		if ( pBestEx[iSelect] != NULL )
		{
#ifdef MAPBASE
			pBestEx[iSelect]->MarkResponseAsUsed( responseToSay + iSelect );
#endif
			return pBestEx[iSelect]->SpeakDispatchResponse( response.m_concept, responseToSay + iSelect, pDeferredCriteria );
		}
		else
		{
			AssertMsg( false, "Response queue somehow found a response, but no expresser for it.\n" );
			return false;
		}
	}	
	else
	{	// I did not find a response.
		return false;
	}

	return false; // just in case
}

void CResponseQueue::Evacuate()
{
	m_Queue.RemoveAll();
}

#undef EXARRAYMAX


///////////////////////////////////////////////////////////////////////////////
//   RESPONSE QUEUE MANAGER
///////////////////////////////////////////////////////////////////////////////


void CResponseQueueManager::LevelInitPreEntity( void )
{
	if (m_pQueue == NULL)
	{
		m_pQueue = new CResponseQueue(AI_RESPONSE_QUEUE_SIZE);
	}
}

CResponseQueueManager::~CResponseQueueManager()
{
	if (m_pQueue != NULL)
	{
		delete m_pQueue;
		m_pQueue = NULL;
	}
}

void CResponseQueueManager::Shutdown()
{
	if (m_pQueue != NULL)
	{
		delete m_pQueue;
		m_pQueue = NULL;
	}
}

void CResponseQueueManager::FrameUpdatePostEntityThink()
{
	Assert(m_pQueue);
	m_pQueue->PerFrameDispatch();
}

CResponseQueueManager g_ResponseQueueManager( "CResponseQueueManager" );

