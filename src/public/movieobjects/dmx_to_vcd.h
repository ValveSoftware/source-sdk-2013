//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMX_TO_VCD_H
#define DMX_TO_VCD_H
#ifdef _WIN32
#pragma once
#endif

class CDmeFilmClip;
class CChoreoScene;

bool ConvertSceneToDmx( CChoreoScene *scene, CDmeFilmClip *dmx );
bool ConvertDmxToScene( CDmeFilmClip *dmx, CChoreoScene *scene );

#define VCD_SCENE_RAMP_TRACK_GROUP_NAME			"scene_ramp"
#define VCD_GLOBAL_EVENTS_TRACK_GROUP_NAME		"global_events"

#endif // DMX_TO_VCD_H
