//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_GC_API_H
#define TF_GC_API_H
#ifdef _WIN32
#pragma once
#endif

void GameCoordinator_NotifyGameState();
void GameCoordinator_NotifyLevelShutdown();
const char *GameCoordinator_GetRegistrationString();

#endif // TF_GC_API_H
