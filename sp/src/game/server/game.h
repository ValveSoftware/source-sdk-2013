//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef GAME_H
#define GAME_H


#include "globals.h"

extern void GameDLLInit( void );

extern ConVar	displaysoundlist;
extern ConVar	mapcyclefile;
extern ConVar	servercfgfile;
extern ConVar	lservercfgfile;

// multiplayer server rules
extern ConVar	teamplay;
extern ConVar	fraglimit;
extern ConVar	falldamage;
extern ConVar	weaponstay;
extern ConVar	forcerespawn;
extern ConVar	footsteps;
extern ConVar	flashlight;
extern ConVar	aimcrosshair;
extern ConVar	decalfrequency;
extern ConVar	teamlist;
extern ConVar	teamoverride;
extern ConVar	defaultteam;
extern ConVar	allowNPCs;

extern ConVar	suitvolume;

// Engine Cvars
extern const ConVar *g_pDeveloper;
#endif		// GAME_H
