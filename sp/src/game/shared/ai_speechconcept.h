//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Class data for an AI Concept, an atom of response-driven dialog.
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_SPEECHCONCEPT_H
#define AI_SPEECHCONCEPT_H

#if defined( _WIN32 )
#pragma once
#endif

#include "../../public/responserules/response_types.h"

class CAI_Concept : public ResponseRules::CRR_Concept
{
public:
	CAI_Concept() {};
	// construct concept from a string.
	CAI_Concept(const char *fromString) : CRR_Concept(fromString) {} ;

	// get/set BS
	inline EHANDLE GetSpeaker() const { return m_hSpeaker; }
	inline void SetSpeaker(EHANDLE val) { m_hSpeaker = val; }

	/*
	inline EHANDLE GetTarget() const { return m_hTarget; }
	inline void SetTarget(EHANDLE val) { m_hTarget = val; }
	inline EHANDLE GetTopic() const { return m_hTopic; }
	inline void SetTopic(EHANDLE val) { m_hTopic = val; }
	*/

protected:
	EHANDLE m_hSpeaker;

	/*
	EHANDLE m_hTarget;
	EHANDLE m_hTopic;
	*/
};


#endif
