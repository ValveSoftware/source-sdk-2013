//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "passtime_convars.h"

#define FLAGS_DEFAULT (FCVAR_NOTIFY | FCVAR_REPLICATED)

#define PASSTIME_CONVAR(NAME, STR, DESC) ConVar NAME(#NAME, #STR, FLAGS_DEFAULT, DESC)

PASSTIME_CONVAR( tf_passtime_scores_per_round,            5, "Number of scores it takes to win a round. Similar to tf_flag_caps_per_round." );
PASSTIME_CONVAR( tf_passtime_ball_damping_scale,          0.0f, "Physics ball damping param. 0.01f in default tf2" );
PASSTIME_CONVAR( tf_passtime_ball_drag_coefficient,       0.01f, "" );
PASSTIME_CONVAR( tf_passtime_ball_inertia_scale,          1.0f, "" );
PASSTIME_CONVAR( tf_passtime_ball_mass,                   1.0f, "" );
PASSTIME_CONVAR( tf_passtime_ball_model,                  models/passtime/ball/passfortress_ball.mdl, "Needs a model with collision info. Map change required." ); // TODO allow override in map
PASSTIME_CONVAR( tf_passtime_ball_sphere_collision,       1, "Boolean value. If nonzero, override mdl collision with a perfect sphere collider." );
PASSTIME_CONVAR( tf_passtime_ball_sphere_radius,          7.2f, "" );
PASSTIME_CONVAR( tf_passtime_ball_reset_time,             15, "How long the ball can be neutral before being automatically reset" );
PASSTIME_CONVAR( tf_passtime_ball_rotdamping_scale,       1.0f, "Higher values will prevent the ball from rolling on the ground." );
PASSTIME_CONVAR( tf_passtime_ball_seek_range,             128, "How close players have to be for the ball to be drawn to them." );
PASSTIME_CONVAR( tf_passtime_ball_seek_speed_factor,      3f, "How fast the ball will move toward nearby players as a ratio of that player's max speed." );
PASSTIME_CONVAR( tf_passtime_ball_takedamage,             1, "Enables shooting the ball" );
PASSTIME_CONVAR( tf_passtime_ball_takedamage_force,       800.0f, "Controls how much the ball responds to being shot" );
PASSTIME_CONVAR( tf_passtime_flinch_boost,                0, "Intensity of flinch on taking damage while carrying the ball. 0 to use TF defaults." );
PASSTIME_CONVAR( tf_passtime_mode_homing_lock_sec,        1.5f, "Number of seconds the ball carrier will stay locked on to a teammate after line of sight is broken." );
PASSTIME_CONVAR( tf_passtime_mode_homing_speed,           1000.0f, "How fast the ball moves during a pass." );
PASSTIME_CONVAR( tf_passtime_overtime_idle_sec,           5, "How many seconds the ball can be idle in overtime before the round ends.");
PASSTIME_CONVAR( tf_passtime_player_reticles_enemies,     1, "Controls HUD reticles for enemies. 0 = never, 1 = when carrying ball, 2 = always." );
PASSTIME_CONVAR( tf_passtime_player_reticles_friends,     2, "Controls HUD reticles for teammates. 0 = never, 1 = when carrying ball, 2 = always." );
PASSTIME_CONVAR( tf_passtime_score_crit_sec,              0.0f, "How long a scoring team's crits last, in default tf2 this is 5.0f" ); 
PASSTIME_CONVAR( tf_passtime_speedboost_on_get_ball_time, 2.0f, "How many seconds of speed boost players get when they get the ball." );
PASSTIME_CONVAR( tf_passtime_steal_on_melee,              1, "Enables melee stealing." );
PASSTIME_CONVAR( tf_passtime_teammate_steal_time,         45, "How many seconds a player can hold the ball before teammates can steal it." );
PASSTIME_CONVAR( tf_passtime_throwarc_scout,              0.1f, "" );
PASSTIME_CONVAR( tf_passtime_throwarc_soldier,            0.1f, "" );
PASSTIME_CONVAR( tf_passtime_throwarc_pyro,               0.1f, "" );
PASSTIME_CONVAR( tf_passtime_throwarc_demoman,            0.1f, "In default TF2 this is 0.15f" );
PASSTIME_CONVAR( tf_passtime_throwarc_heavy,              0.175f, "" );
PASSTIME_CONVAR( tf_passtime_throwarc_engineer,           0.2f, "" );
PASSTIME_CONVAR( tf_passtime_throwarc_medic,              0.0f, "" );
PASSTIME_CONVAR( tf_passtime_throwarc_sniper,             0.0f, "" );
PASSTIME_CONVAR( tf_passtime_throwarc_spy,                0.0f, "" );
PASSTIME_CONVAR( tf_passtime_throwspeed_scout,            700.0f, "" );
PASSTIME_CONVAR( tf_passtime_throwspeed_soldier,          800.0f, "" );
PASSTIME_CONVAR( tf_passtime_throwspeed_pyro,             750.0f, "" );
PASSTIME_CONVAR( tf_passtime_throwspeed_demoman,          800.0f, "In default tf2 this is 850.0f" );
PASSTIME_CONVAR( tf_passtime_throwspeed_heavy,            850.0f, "" );
PASSTIME_CONVAR( tf_passtime_throwspeed_engineer,         850.0f, "" );
PASSTIME_CONVAR( tf_passtime_throwspeed_medic,            900.0f, "" );
PASSTIME_CONVAR( tf_passtime_throwspeed_sniper,           900.0f, "" );
PASSTIME_CONVAR( tf_passtime_throwspeed_spy,              900.0f, "" );
PASSTIME_CONVAR( tf_passtime_throwspeed_velocity_scale,   0.33f, "How much player velocity to add when tossing (0=none 1=100%)" );
PASSTIME_CONVAR( tf_passtime_save_stats,                  0, "" );

PASSTIME_CONVAR( tf_passtime_experiment_telepass,         0, "None,\
	TeleportToCatcher,\
	SwapWithCatcher,\
	TeleportToCatcherMaintainPossession,");
PASSTIME_CONVAR( tf_passtime_experiment_instapass_charge, 0, "" );
PASSTIME_CONVAR( tf_passtime_experiment_autopass,         0, "" );
PASSTIME_CONVAR( tf_passtime_experiment_instapass,        0, "" );

PASSTIME_CONVAR( tf_passtime_powerball_decayamount,       1, "How many points are removed are removed per decay. (must be integer)" );
PASSTIME_CONVAR( tf_passtime_powerball_decaysec,          4.5f, "How many seconds per decay when the ball is held." );
PASSTIME_CONVAR( tf_passtime_powerball_decaysec_neutral,  1.5f, "How many seconds per decay when the ball is neutral." );
PASSTIME_CONVAR( tf_passtime_powerball_passpoints,        0, "How many ball meter points are awarded for a complete pass." );
PASSTIME_CONVAR( tf_passtime_powerball_threshold,         80, "How many ball meter points it takes to unlock bonus goals." );
PASSTIME_CONVAR( tf_passtime_powerball_airtimebonus,      0, "Ball meter points added per second of time a pass is in the air." );
PASSTIME_CONVAR( tf_passtime_powerball_maxairtimebonus,   100, "Cap on extra points added by tf_passtime_powerball_airtimebonus." );
PASSTIME_CONVAR( tf_passtime_powerball_decay_delay,       10, "Number of seconds between ball reaching full charge and decay beginning." );
PASSTIME_CONVAR( tf_passtime_pack_range,                  512, "How close players must be to the ball carrier to be included in the pack." );
PASSTIME_CONVAR( tf_passtime_pack_speed,                  1, "When set to 1, all players near the ball carrier will move the same speed." );
PASSTIME_CONVAR( tf_passtime_pack_hp_per_sec,             2.0f, "How many HP per second pack members are healed." );

// med splashing
PASSTIME_CONVAR( p4ss_med_cansplash,              1, "Enables med splashing." );
PASSTIME_CONVAR( p4ss_med_canpushball,            1, "Enables med pushing ball with crossbow." );
PASSTIME_CONVAR( p4ss_med_canheadshot,            0, "Enables med headshotting with crossbow." );
PASSTIME_CONVAR( p4ss_med_crossbow_damagefalloff, -1, "Sets damage falloff for crossbow. [-1, 0, 1]" );
PASSTIME_CONVAR( p4ss_med_crossbow_heal_mult,     2.0f, "Sets heal multiplier for crossbow. " );

PASSTIME_CONVAR( p4ss_golden_goal,                1, "Enables golden goal state when stalemate would happen." );
PASSTIME_CONVAR( p4ss_lock_eye_to_eye_los,        1, "Check LOS eye-to-eye when trying to lock-on." );
PASSTIME_CONVAR( p4ss_whistle_more,               1, "Allows users to whistle in more use cases." );
PASSTIME_CONVAR( p4ss_heal_on_pass,               0, "How many HP you recieve when ball is passed to you." );
PASSTIME_CONVAR( p4ss_heal_on_pass_flight_time,   2, "How many seconds between passes for a pass to heal." );
PASSTIME_CONVAR( p4ss_minicrit_protection_time,   3, "How many seconds you are protected from minicrits after picking up the ball." );
PASSTIME_CONVAR( p4ss_lock_max_turn_angle,        90, "Maximum angle in degrees you can turn away from locked target before unlocking. 0 to disable." );

PASSTIME_CONVAR( tf_passtime_no_jack_armor, 1, "Prevents ball thrower from immediately grabbing the ball." );
PASSTIME_CONVAR( tf_passtime_no_jack_armor_time, 0.25f, "How many seconds the thrower must wait before picking up the ball again." );
