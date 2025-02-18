//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GAMEEVENTDEFS_H
#define GAMEEVENTDEFS_H

#ifdef _WIN32
#pragma once
#endif

// Make sure your gameevents.res and this file is in sync
// Event names may be 32 characters long and are case sensitive
// 256 is the maximum number of game events

#define GAME_EVENT_PLAYER_DEATH		"player_death"
#define GAME_EVENT_SAY_TEXT			"say_text"

#endif // GAMEEVENTDEFS_H