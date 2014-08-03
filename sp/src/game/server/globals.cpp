//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== globals.cpp ========================================================

  DLL-wide global variable definitions.
  They're all defined here, for convenient centralization.
  Source files that need them should "extern ..." declare each
  variable, to better document what globals they care about.

*/

#include "cbase.h"
#include "soundent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

Vector			g_vecAttackDir;
int				g_iSkillLevel;
bool			g_fGameOver;
