//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_BOT_TEMP_H
#define TF_BOT_TEMP_H
#ifdef _WIN32
#pragma once
#endif


// If iTeam or iClass is -1, then a team or class is randomly chosen.
CBasePlayer *BotPutInServer( bool bTargetDummy, bool bFrozen, int iTeam, int iClass, const char *pszCustomName );

void Bot_RunAll();


#endif // TF_BOT_TEMP_H
