//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================


#include <tier0/dbg.h>
#include "fgdlib/InputOutput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


typedef struct
{
	InputOutputType_t eType;	// The enumeration of this type.
	char *pszName;				// The name of this type.
} TypeMap_t;


char *CClassInputOutputBase::g_pszEmpty = "";


//-----------------------------------------------------------------------------
// Maps type names to type enums for inputs and outputs.
//-----------------------------------------------------------------------------
static TypeMap_t TypeMap[] =
{
	{ iotVoid,		"void" },
	{ iotInt,		"integer" },
	{ iotBool,		"bool" },
	{ iotString,	"string" },
	{ iotFloat,		"float" },
	{ iotVector,	"vector" },
	{ iotEHandle,	"target_destination" },
	{ iotColor,		"color255" },
	{ iotEHandle,	"ehandle" }, // for backwards compatibility
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CClassInputOutputBase::CClassInputOutputBase(void)
{
	m_eType = iotInvalid;
	m_pszDescription = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszName - 
//			eType - 
//-----------------------------------------------------------------------------
CClassInputOutputBase::CClassInputOutputBase(const char *pszName, InputOutputType_t eType)
{
	m_pszDescription = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CClassInputOutputBase::~CClassInputOutputBase(void)
{
	delete m_pszDescription;
	m_pszDescription = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a string representing the type of this I/O, eg. "integer".
//-----------------------------------------------------------------------------
const char *CClassInputOutputBase::GetTypeText(void)
{
	for (int i = 0; i < sizeof(TypeMap) / sizeof(TypeMap[0]); i++)
	{
		if (TypeMap[i].eType == m_eType)
		{
			return(TypeMap[i].pszName);
		}
	}

	return("unknown");
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : szType - 
// Output : InputOutputType_t
//-----------------------------------------------------------------------------
InputOutputType_t CClassInputOutputBase::SetType(const char *szType)
{
	for (int i = 0; i < sizeof(TypeMap) / sizeof(TypeMap[0]); i++)
	{
		if (!stricmp(TypeMap[i].pszName, szType))
		{
			m_eType = TypeMap[i].eType;
			return(m_eType);
		}
	}

	return(iotInvalid);
}


//-----------------------------------------------------------------------------
// Purpose: Assignment operator.
//-----------------------------------------------------------------------------
CClassInputOutputBase &CClassInputOutputBase::operator =(CClassInputOutputBase &Other)
{
	strcpy(m_szName, Other.m_szName);
	m_eType = Other.m_eType;

	//
	// Copy the description.
	//
	delete m_pszDescription;
	if (Other.m_pszDescription != NULL)
	{
		m_pszDescription = new char[strlen(Other.m_pszDescription) + 1];
		strcpy(m_pszDescription, Other.m_pszDescription);
	}
	else
	{
		m_pszDescription = NULL;
	}

	return(*this);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CClassInput::CClassInput(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszName - 
//			eType - 
//-----------------------------------------------------------------------------
CClassInput::CClassInput(const char *pszName, InputOutputType_t eType)
	: CClassInputOutputBase(pszName, eType)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CClassOutput::CClassOutput(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszName - 
//			eType - 
//-----------------------------------------------------------------------------
CClassOutput::CClassOutput(const char *pszName, InputOutputType_t eType)
	: CClassInputOutputBase(pszName, eType)
{
}
