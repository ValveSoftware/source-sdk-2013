//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_match_description_shared.h"
#include "tf_ladder_data.h"
#include "tf_rating_data.h"


#if defined CLIENT_DLL || defined GAME_DLL
#include "tf_gamerules.h"
#endif

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

extern ConVar tf_mm_match_size_mvm;
extern ConVar tf_mm_match_size_ladder_6v6;
extern ConVar tf_mm_match_size_ladder_9v9;
extern ConVar tf_mm_match_size_ladder_12v12;
extern ConVar tf_mm_match_size_ladder_12v12_minimum;
extern ConVar servercfgfile;
extern ConVar lservercfgfile;
extern ConVar mp_tournament_stopwatch;
extern ConVar tf_gamemode_payload;
extern ConVar tf_gamemode_ctf;

#ifdef GAME_DLL
extern ConVar tf_mvm_allow_abandon_after_seconds;
extern ConVar tf_mvm_allow_abandon_below_players;
#endif

