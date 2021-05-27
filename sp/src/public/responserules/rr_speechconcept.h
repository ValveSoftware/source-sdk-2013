//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Class data for an AI Concept, an atom of response-driven dialog.
//
// $NoKeywords: $
//=============================================================================//

#ifndef RR_SPEECHCONCEPT_H
#define RR_SPEECHCONCEPT_H

#if defined( _WIN32 )
#pragma once
#endif

#include "utlsymbol.h"

#define RR_CONCEPTS_ARE_STRINGS 0


typedef CUtlSymbolTable CRR_ConceptSymbolTable;

namespace ResponseRules
{
class CRR_Concept
{
public: // local typedefs
	typedef CUtlSymbol tGenericId; // an int-like type that can be used to refer to all concepts of this type
	tGenericId m_iConcept;

public:
	CRR_Concept() {};
	// construct concept from a string.
	CRR_Concept(const char *fromString);

	// Return as a string
	const char *GetStringConcept() const;
	static const char *GetStringForGenericId(tGenericId genericId);

	operator tGenericId() const { return m_iConcept; }
	operator const char *() const { return GetStringConcept(); }
	inline bool operator==(const CRR_Concept &other) // default is compare by concept ids
	{
		return m_iConcept == other.m_iConcept;
	}
	bool operator==(const char *pszConcept);

protected:

private:
	// dupe a concept
	// CRR_Concept& operator=(CRR_Concept &other);
	CRR_Concept& operator=(const char *fromString);
};
};


#endif
