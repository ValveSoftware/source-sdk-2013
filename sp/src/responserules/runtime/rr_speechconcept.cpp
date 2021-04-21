//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "rrbase.h"


// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#if RR_CONCEPTS_ARE_STRINGS
#pragma error("RR_CONCEPTS_ARE_STRINGS no longer supported")
#else

using namespace ResponseRules;

// Used to turn ad-hoc concept from strings into numbers.
CRR_ConceptSymbolTable *g_pRRConceptTable = NULL; 

// Q&D hack to defer initialization of concept table until I can figure out where it
// really needs to come from.
static void InitializeRRConceptTable()
{
	if (g_pRRConceptTable == NULL)
	{
		g_pRRConceptTable = new CRR_ConceptSymbolTable( 64, 64, true );
	}
}

// construct from string
CRR_Concept::CRR_Concept(const char *fromString)
{
	InitializeRRConceptTable();
	m_iConcept = g_pRRConceptTable->AddString(fromString);
}

CRR_Concept &CRR_Concept::operator=(const char *fromString)
{
	InitializeRRConceptTable();
	m_iConcept = g_pRRConceptTable->AddString(fromString);
	return *this;
}

bool CRR_Concept::operator==(const char *pszConcept)
{
	int otherConcept = g_pRRConceptTable->Find(pszConcept);
	return ( otherConcept != UTL_INVAL_SYMBOL && otherConcept == m_iConcept );
}

const char *CRR_Concept::GetStringConcept() const
{
	InitializeRRConceptTable();
	AssertMsg( m_iConcept.IsValid(), "AI Concept has invalid string symbol.\n" );
	const char * retval = g_pRRConceptTable->String(m_iConcept);
	AssertMsg( retval, "An RR_Concept couldn't find its string in the symbol table!\n" );
	if (retval == NULL)
	{
		Warning( "An RR_Concept couldn't find its string in the symbol table!\n" );
		retval = "";
	}
	return retval;
}

const char *CRR_Concept::GetStringForGenericId(tGenericId genericId)
{
	InitializeRRConceptTable();
	return g_pRRConceptTable->String(genericId);
}

#endif
