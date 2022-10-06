//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: An event queue of AI concepts that dispatches them to appropriate characters.
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_SPEECHQUEUE_H
#define AI_SPEECHQUEUE_H

#if defined( _WIN32 )
#pragma once
#endif

#include "ai_speech.h"

#define AI_RESPONSE_QUEUE_SIZE 64

enum DeferredResponseTarget_t // possible targets for a deferred response
{
	kDRT_ANY, // best matching respondent within range -- except for the one in the m_hTarget handle
	kDRT_ALL, // send to everyone in range -- except for the one in the m_hTarget handle
	kDRT_SPECIFIC, // a specific entity is targeted

	kDRT_MAX, // high water mark
};

// Allows you to postpone AI speech concepts to a later time, or to direct them to 
// a specific character, or all of them.
class CResponseQueue
{
	//////////////////// Local types ////////////////////
public: 

	// We pack up contexts to send along with the concept.
	// For now I'll just copy criteria sets, but it will be better to do something
	// more efficient in the future.
	typedef AI_CriteriaSet DeferredContexts_t; 

	struct CFollowupTargetSpec_t ///< to whom a followup is directed. Can be a specific entity or something more exotic.
	{
		DeferredResponseTarget_t m_iTargetType; ///< ANY, ALL, or SPECIFIC. If specific, pass through a handle to:
		EHANDLE	m_hHandle; ///< a specific target for the message, or a specific character to OMIT. 
		inline bool IsValid( void ) const;

		// constructors/destructors
		explicit CFollowupTargetSpec_t(const DeferredResponseTarget_t &targetType, const EHANDLE &handle)
			: m_iTargetType(targetType), m_hHandle(handle) 
		{};
		explicit CFollowupTargetSpec_t(const EHANDLE &handle)
			: m_iTargetType(kDRT_SPECIFIC), m_hHandle(handle) 
		{};
		CFollowupTargetSpec_t(DeferredResponseTarget_t target) // eg, ANY, ALL, etc. 
			: m_iTargetType(target)  
		{	
			AssertMsg(m_iTargetType != kDRT_SPECIFIC, "Response rule followup tried to specify an entity target, but didn't provide the target.\n" ); 
		}
		CFollowupTargetSpec_t(void) // default: invalid
			: m_iTargetType(kDRT_MAX)
		{};
	};

	/// A single deferred response.
	struct CDeferredResponse
	{
		AIConcept_t m_concept;
		DeferredContexts_t m_contexts; ///< contexts to send along with the concept
		float m_fDispatchTime;
		EHANDLE m_hIssuer; ///< an entity, if issued by an entity
		/*
		DeferredResponseTarget_t m_iTargetType;
		EHANDLE m_hTarget; // May be invalid.
		*/
		CFollowupTargetSpec_t m_Target;

		inline void Init( const AIConcept_t &concept, const AI_CriteriaSet * RESTRICT contexts, float dtime, const CFollowupTargetSpec_t &target, CBaseEntity *pIssuer );
		inline bool IsQuashed() { return !m_Target.IsValid(); }
		void Quash(); ///< make this response invalid.
	};
	/// write
	static void DeferContextsFromCriteriaSet( DeferredContexts_t &contextsOut, const AI_CriteriaSet *criteriaIn );

	//////////////////// Methods ////////////////////
public:
	CResponseQueue( int queueSize );

	/// Add a deferred response. 
	void Add( const AIConcept_t &concept,  ///< concept to dispatch
			  const AI_CriteriaSet * RESTRICT contexts, ///< the contexts that come with it (may be NULL)
			  float time,					 ///< when to dispatch it. You can specify a time of zero to mean "immediately."
			  const CFollowupTargetSpec_t &targetspec, /// All information necessary to target this response
			  CBaseEntity *pIssuer = NULL ///< the entity who should not respond if this is a ANY or ALL rule. (eg, don't let people talk to themselves.)
			  );

	/// Remove all deferred responses matching the concept and issuer.
	void Remove( const AIConcept_t &concept,  ///< concept to dispatch
		CBaseEntity * const pIssuer = NULL ///< the entity issuing the response, if one exists.
		);

	/// Remove all deferred responses queued to be spoken by given character
	void RemoveSpeechQueuedFor( const CBaseEntity *pSpeaker	);
	
    /// Empty out all pending events
	void Evacuate();

	/// Go through and dispatch any deferred responses.
	void PerFrameDispatch();

	/// Add an expressor owner to this queue.
	void AddExpresserHost(CBaseEntity *host);

	/// Remove an expresser host from this queue.
	void RemoveExpresserHost(CBaseEntity *host);

	/// Iterate over potential expressers for this queue
	inline int GetNumExpresserTargets() const;
	inline CBaseEntity *GetExpresserHost(int which) const;

protected:
	/// Actually send off one response to a consumer
	/// Return true if dispatch succeeded
	bool DispatchOneResponse( CDeferredResponse &response );

private:
	/// Helper function for one case in DispatchOneResponse
	/// (for better organization)
	bool DispatchOneResponse_ThenANY( CDeferredResponse &response, AI_CriteriaSet * RESTRICT pDeferredCriteria, CBaseEntity *  const RESTRICT pIssuer, float followupMaxDistSq );

	//////////////////// Data ////////////////////
protected:
	typedef CUtlFixedLinkedList< CDeferredResponse > QueueType_t;
	QueueType_t m_Queue; // the queue of deferred responses, will eventually be sorted
	/// Note  about the queue type: if you move to replace it with a sorted priority queue,
	/// make sure it is a type such that an iterator is not invalidated by inserts and deletes.
	/// CResponseQueue::PerFrameDispatch() iterates over the queue calling DispatchOneResponse
	/// on each in turn, and those responses may very easily add new events to the queue. 
	/// A crash will result if the iterator used in CResponseQueue::PerFrameDispatch()'s loop
	/// becomes invalid.

	CUtlVector<EHANDLE> m_ExpresserTargets; // a list of legitimate expresser targets
};

inline void CResponseQueue::CDeferredResponse::Init(const AIConcept_t &concept, const AI_CriteriaSet * RESTRICT contexts, float dtime, const CFollowupTargetSpec_t &target, CBaseEntity *pIssuer )
{
	m_concept = concept; 
	m_fDispatchTime = dtime;
	/*
	m_iTargetType = targetType;
	m_hTarget =  handle ;
	*/
	m_Target = target;
	m_hIssuer = pIssuer;
	DeferContextsFromCriteriaSet(m_contexts, contexts);
}

int CResponseQueue::GetNumExpresserTargets() const
{
	return m_ExpresserTargets.Count();
}

CBaseEntity *CResponseQueue::GetExpresserHost(int which) const
{
	return m_ExpresserTargets[which];
}


// The wrapper game system that contains a response queue, and ticks it each frame.

class CResponseQueueManager : public CAutoGameSystemPerFrame
{
public:
	CResponseQueueManager(char const *name) : CAutoGameSystemPerFrame( name )
	{
		m_pQueue = NULL;
	}
	virtual ~CResponseQueueManager(void);
	virtual void Shutdown();
	virtual void FrameUpdatePostEntityThink( void );
	virtual void LevelInitPreEntity( void );

	inline CResponseQueue *GetQueue(void) { Assert(m_pQueue); return m_pQueue; }

protected:
	CResponseQueue *m_pQueue;
};


// Valid if the target type enum is within bounds. Furthermore if it 
// specifies a specific entity, that handle must be valid.
bool CResponseQueue::CFollowupTargetSpec_t::IsValid( void ) const
{
	if (m_iTargetType >= kDRT_MAX) 
		return false;
	if (m_iTargetType < 0) 
		return false;
	if (m_iTargetType == kDRT_SPECIFIC && !m_hHandle.IsValid())
		return false;

	return true;
}

extern CResponseQueueManager g_ResponseQueueManager;


// Handy global helper funcs

/// Automatically queue up speech to happen immediately -- calls straight through to response rules add
inline void QueueSpeak( const AIConcept_t &concept,					///< concept name to say
					    const CResponseQueue::CFollowupTargetSpec_t& targetspec,	///< kDRT_ANY, kDRT_ALL, etc
						CBaseEntity *pIssuer = NULL					///< if specifying ANY or ALL, use this to specify the one you *don't* want to speak
						)
{
	return g_ResponseQueueManager.GetQueue()->Add( concept, NULL, 0.0f, targetspec, pIssuer );
}

/// Automatically queue up speech to happen immediately -- calls straight through to response rules add
inline void QueueSpeak( const AIConcept_t &concept,					///< concept name to say
					    const CResponseQueue::CFollowupTargetSpec_t& targetspec,	///< kDRT_ANY, kDRT_ALL, etc
						const AI_CriteriaSet &criteria,				///< criteria to pass in
					    CBaseEntity *pIssuer = NULL					///< if specifying ANY or ALL, use this to specify the one you *don't* want to speak
					   )
{
	return g_ResponseQueueManager.GetQueue()->Add( concept, &criteria, 0.0f, targetspec, pIssuer );
}

/// Automatically queue up speech to happen immediately -- calls straight through to response rules add
inline void QueueSpeak( const AIConcept_t &concept,					///< concept name to say
					   const EHANDLE &target,						///< which entity shall speak
					   float delay,									///< how far in the future to speak
					   const AI_CriteriaSet &criteria,				///< criteria to pass in
					   CBaseEntity *pIssuer = NULL )
{
	return g_ResponseQueueManager.GetQueue()->Add( concept, &criteria, gpGlobals->curtime + delay,
		CResponseQueue::CFollowupTargetSpec_t(target), pIssuer );
}



#endif // AI_SPEECHQUEUE_H
