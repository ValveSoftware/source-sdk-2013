//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Suggested place to build certain objects
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_FUNC_SUGGESTED_BUILD_H
#define TF_FUNC_SUGGESTED_BUILD_H

#ifdef _WIN32
#pragma once
#endif

class CBaseObject;

//-----------------------------------------------------------------------------
// Returns true if the baseObject was in a suggested area, false otherwise.
//-----------------------------------------------------------------------------
bool NotifyObjectBuiltInSuggestedArea( CBaseObject &baseObject );

//-----------------------------------------------------------------------------
// Returns true if the upgraded baseObject was in a suggested area, false otherwise.
//-----------------------------------------------------------------------------
bool NotifyObjectUpgradedInSuggestedArea( CBaseObject &baseObject );

#endif // TF_FUNC_SUGGESTED_BUILD_H
