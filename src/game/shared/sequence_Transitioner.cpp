//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "sequence_Transitioner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -----------------------------------------------------------------------------
// CSequenceTransitioner implementation.
// -----------------------------------------------------------------------------

void CSequenceTransitioner::CheckForSequenceChange( 
	CStudioHdr *hdr,
	int nCurSequence, 
	bool bForceNewSequence,
	bool bInterpolate )
{
	// sequence may be set before model is initialized
	if ( hdr == NULL)
		return;

	// FIXME?: this should detect that what's been asked to be drawn isn't what was expected
	// due to not only sequence change, by frame index, rate, or whatever.  When that happens, 
	// it should insert the previous rules.

	if (m_animationQueue.Count() == 0)
	{
		m_animationQueue.AddToTail();
	}

	CAnimationLayer *currentblend = &m_animationQueue[m_animationQueue.Count()-1];

	if (currentblend->m_flLayerAnimtime && 
		(currentblend->m_nSequence != nCurSequence || bForceNewSequence ))
	{
		mstudioseqdesc_t &seqdesc = hdr->pSeqdesc( nCurSequence );
		// sequence changed
		if ((seqdesc.flags & STUDIO_SNAP) || !bInterpolate )
		{
			// remove all entries
			m_animationQueue.RemoveAll();
		}
		else
		{
			mstudioseqdesc_t &prevseqdesc = hdr->pSeqdesc( currentblend->m_nSequence );
			currentblend->m_flLayerFadeOuttime = MIN( prevseqdesc.fadeouttime, seqdesc.fadeintime );
			/*
			// clip blends to time remaining
			if ( !IsSequenceLooping(hdr, currentblend->m_nSequence) )
			{
				float length = Studio_Duration( hdr, currentblend->m_nSequence, flPoseParameter ) / currentblend->m_flPlaybackRate;
				float timeLeft = (1.0 - currentblend->m_flCycle) * length;
				if (timeLeft < currentblend->m_flLayerFadeOuttime)
					currentblend->m_flLayerFadeOuttime = timeLeft;
			}
			*/
		}
		// push previously set sequence
		m_animationQueue.AddToTail();
		currentblend = &m_animationQueue[m_animationQueue.Count()-1];

	}

	currentblend->m_nSequence = -1;
	currentblend->m_flLayerAnimtime = 0.0;
	currentblend->m_flLayerFadeOuttime = 0.0;
}


void CSequenceTransitioner::UpdateCurrent( 
	CStudioHdr *hdr,
	int nCurSequence, 
	float flCurCycle,
	float flCurPlaybackRate,
	float flCurTime )
{
	// sequence may be set before model is initialized
	if ( hdr == NULL)
		return;

	if (m_animationQueue.Count() == 0)
	{
		m_animationQueue.AddToTail();
	}

	CAnimationLayer *currentblend = &m_animationQueue[m_animationQueue.Count()-1];

	// keep track of current sequence
	currentblend->m_nSequence = nCurSequence;
	currentblend->m_flLayerAnimtime = flCurTime;
	currentblend->m_flCycle = flCurCycle;
	currentblend->m_flPlaybackRate = flCurPlaybackRate;

	// calc blending weights for previous sequences
	int i;
	for (i = 0; i < m_animationQueue.Count() - 1;)
	{
 		float s = m_animationQueue[i].GetFadeout( flCurTime );

		if (s > 0)
		{
			m_animationQueue[i].m_flWeight = s;
			i++;
		}
		else
		{
			m_animationQueue.Remove( i );
		}
	}
}
