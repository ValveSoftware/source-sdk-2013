//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  
//			
//=============================================================================

#ifndef TF_MATCH_DESCRIPTION_SHARED_H
#define TF_MATCH_DESCRIPTION_SHARED_H

#include "tf_match_description.h"
#include <functional>

#ifdef CLIENT_DLL
	#include "tf_gc_client.h"
	#include "animation.h"
	#include "vgui/ISurface.h"
	#include "vgui_controls/Controls.h"
	#include "tf_lobby_server.h"
#endif

#ifdef GAME_DLL
	#include "tf_lobby_server.h"
	#include "tf_gc_server.h"
	#include "tf_objective_resource.h"
	#include "team_control_point_master.h"
#endif

#ifdef GAME_DLL
extern ConVar tf_gamemode_payload;
extern ConVar tf_gamemode_ctf;
#endif

	extern ConVar tf_mm_match_size_mvm;
	extern ConVar tf_mm_match_size_ladder_6v6;
	extern ConVar tf_mm_match_size_ladder_9v9;
	extern ConVar tf_mm_match_size_ladder_12v12;
	extern ConVar tf_mm_match_size_ladder_12v12_minimum;
	extern ConVar servercfgfile;
	extern ConVar lservercfgfile;
	extern ConVar mp_tournament_stopwatch;

#ifdef GAME_DLL
extern ConVar tf_mvm_allow_abandon_after_seconds;
extern ConVar tf_mvm_allow_abandon_below_players;
#endif

#define MVM_REQUIRED_SCORE (ConVar*)NULL
#define LADDER_REQUIRED_SCORE (ConVar*)NULL
#define CASUAL_REQUIRED_SCORE (ConVar*)NULL


#endif //TF_MATCH_DESCRIPTION_SHARED_H
