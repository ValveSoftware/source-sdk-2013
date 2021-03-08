//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_RESPONSESYSTEM_H
#define AI_RESPONSESYSTEM_H

#include "utlvector.h"

#ifdef _WIN32
#pragma once
#endif

#include "ai_criteria.h"
#include "../../public/responserules/response_types.h"

// using ResponseRules::IResponseFilter;
// using ResponseRules::IResponseSystem;

ResponseRules::IResponseSystem *PrecacheCustomResponseSystem( const char *scriptfile );
ResponseRules::IResponseSystem *BuildCustomResponseSystemGivenCriteria( const char *pszBaseFile, const char *pszCustomName, AI_CriteriaSet &criteriaSet, float flCriteriaScore );
void DestroyCustomResponseSystems();

class ISaveRestoreBlockHandler *GetDefaultResponseSystemSaveRestoreBlockHandler();
class ISaveRestoreOps *GetResponseSystemSaveRestoreOps();

#endif // AI_RESPONSESYSTEM_H
