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

#ifdef Seco7_SAVERESTORE
extern bool Transitioned;
#endif //Seco7_SAVERESTORE



#endif // GLOBALS_H
