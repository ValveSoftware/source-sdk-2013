//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GLOBALS_H
#define GLOBALS_H
#ifdef _WIN32
#pragma once
#endif


extern Vector g_vecAttackDir;
extern int g_iSkillLevel;
extern bool g_fGameOver;
extern ConVar g_Language;

#ifdef SecobMod__SAVERESTORE
extern bool Transitioned;
#endif //SecobMod__SAVERESTORE



#endif // GLOBALS_H
