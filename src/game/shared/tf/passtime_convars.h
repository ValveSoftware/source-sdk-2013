//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PASSTIME_CONVARS_H
#define PASSTIME_CONVARS_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"

extern ConVar
	tf_passtime_scores_per_round,
	tf_passtime_ball_damping_scale,
	tf_passtime_ball_drag_coefficient,
	tf_passtime_ball_inertia_scale,
	tf_passtime_ball_mass,
	tf_passtime_ball_model,
	tf_passtime_ball_reset_time,
	tf_passtime_ball_rotdamping_scale,
	tf_passtime_ball_seek_range,
	tf_passtime_ball_seek_speed_factor,
	tf_passtime_ball_sphere_collision,
	tf_passtime_ball_sphere_radius,
	tf_passtime_ball_takedamage,
	tf_passtime_ball_takedamage_force,
	tf_passtime_flinch_boost,
	tf_passtime_mode_homing_lock_sec,
	tf_passtime_mode_homing_speed,
	tf_passtime_overtime_idle_sec,
	tf_passtime_player_reticles_enemies,
	tf_passtime_player_reticles_friends,
	tf_passtime_score_crit_sec,
	tf_passtime_speedboost_on_get_ball_time,
	tf_passtime_steal_on_melee,
	tf_passtime_teammate_steal_time,
	tf_passtime_throwarc_scout,
	tf_passtime_throwarc_sniper,
	tf_passtime_throwarc_soldier,
	tf_passtime_throwarc_demoman,
	tf_passtime_throwarc_medic,
	tf_passtime_throwarc_heavy,
	tf_passtime_throwarc_pyro,
	tf_passtime_throwarc_spy,
	tf_passtime_throwarc_engineer,
	tf_passtime_throwspeed_scout,
	tf_passtime_throwspeed_sniper,
	tf_passtime_throwspeed_soldier,
	tf_passtime_throwspeed_demoman,
	tf_passtime_throwspeed_medic,
	tf_passtime_throwspeed_heavy,
	tf_passtime_throwspeed_pyro,
	tf_passtime_throwspeed_spy,
	tf_passtime_throwspeed_engineer,
	tf_passtime_throwspeed_velocity_scale,
	tf_passtime_save_stats,

	tf_passtime_experiment_telepass,
	tf_passtime_experiment_autopass,
	tf_passtime_experiment_instapass_charge,
	tf_passtime_experiment_instapass,

	tf_passtime_powerball_decayamount,
	tf_passtime_powerball_decaysec,
	tf_passtime_powerball_decaysec_neutral,
	tf_passtime_powerball_passpoints,
	tf_passtime_powerball_threshold,
	tf_passtime_powerball_airtimebonus,
	tf_passtime_powerball_maxairtimebonus,
	tf_passtime_powerball_decay_delay,
	tf_passtime_pack_range,
	tf_passtime_pack_speed,
	tf_passtime_pack_hp_per_sec;

enum class EPasstimeExperiment_Telepass { 
	None,
	TeleportToCatcher,
	SwapWithCatcher,
	TeleportToCatcherMaintainPossession,
};

#endif // PASSTIME_CONVARS_H  
