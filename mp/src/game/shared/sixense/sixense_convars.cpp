//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef SIXENSE

//
// general sixense convars
//

ConVar sixense_enabled( "sixense_enabled", "0", FCVAR_ARCHIVE );
ConVar sixense_filter_level( "sixense_filter_level", "0.5", FCVAR_ARCHIVE );
ConVar sixense_features_enabled( "sixense_features_enabled", "0", FCVAR_REPLICATED );
ConVar sixense_mode( "sixense_mode", "0", FCVAR_ARCHIVE );

#ifdef PORTAL2
ConVar sixense_ratchet_lesson_angle_threshold( "sixense_ratchet_lesson_angle_threshold", "0.6", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_ratchet_lesson_angle_end_threshold( "sixense_ratchet_lesson_angle_end_threshold", "0.8", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_ratchet_lesson_angle_hold_time( "sixense_ratchet_lesson_angle_hold_time", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_disable_ratchet_lesson( "sixense_disable_ratchet_lesson", "0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_enable_tutorial_ratchet_lesson( "sixense_enable_tutorial_ratchet_lesson", "0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_disable_scale_reset_lesson( "sixense_disable_scale_reset_lesson", "0", FCVAR_REPLICATED | FCVAR_CHEAT );

//
// grab convars
//

ConVar sixense_throw_multiplier ("sixense_throw_multiplier", "1.0", FCVAR_REPLICATED | FCVAR_ARCHIVE );
ConVar sixense_hold_multiplier ("sixense_hold_multiplier", "7.0", FCVAR_REPLICATED | FCVAR_ARCHIVE );
ConVar sixense_hold_z_scale ("sixense_hold_z_scale", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_z_min( "sixense_hold_z_min", "-128.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_z_max ("sixense_hold_z_max", "256.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_z_min_no_portalgun( "sixense_hold_z_min_no_portalgun", "0.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_z_max_no_portalgun( "sixense_hold_z_max_no_portalgun", "48.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_throw_max_vel ("sixense_throw_max_vel", "750.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_blend ("sixense_hold_blend", "0.925", FCVAR_REPLICATED | FCVAR_ARCHIVE );
ConVar sixense_hold_offset_x( "sixense_hold_offset_x", "0.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_offset_y( "sixense_hold_offset_y", "0.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_offset_z( "sixense_hold_offset_z", "-20.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_velocity_blend_val( "sixense_velocity_blend_val", "0.5", FCVAR_REPLICATED | FCVAR_ARCHIVE );
ConVar sixense_hold_error_max_noise( "sixense_hold_error_max_noise", "3.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_error_min( "sixense_hold_error_min", "10.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_error_max( "sixense_hold_error_max", "100.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_sound_pitch( "sixense_hold_sound_pitch", "160.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_sound_scaling_pitch( "sixense_hold_sound_scaling_pitch", "190.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_sound_error_pitch( "sixense_hold_sound_error_pitch", "95.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_move_sound_pitch( "sixense_hold_move_sound_pitch", "10.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_beam_portal_delay_time( "sixense_hold_beam_portal_delay_time", "0.035", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_length_error( "sixense_hold_length_error", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_length_error_fraction( "sixense_hold_length_error_fraction", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_drop_cube_button_funnel_velocity_scale( "sixense_drop_cube_button_funnel_velocity_scale", "2.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_color_error_min( "sixense_hold_color_error_min", "0.2", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_color_error_max( "sixense_hold_color_error_max", "0.7", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_turret_pickup_time( "sixense_hold_turret_pickup_time", "0.8", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_hold_turret_distance( "sixense_hold_turret_distance", "70.0", FCVAR_REPLICATED | FCVAR_CHEAT );

//
// portal tweaking convars
//

ConVar sixense_portal_tweaking_enabled("sixense_portal_tweaking_enabled", "1", FCVAR_REPLICATED | FCVAR_ARCHIVE );
ConVar sixense_portal_tweaking_roll_scale( "sixense_portal_tweaking_roll_scale", "2.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Scale portal tweaking controller rotation by this amount." );
ConVar sixense_portal_tweaking_grab_radius( "sixense_portal_tweaking_grab_radius", "25.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Max grab radius from aimed trace world intersection and portal." );
ConVar sixense_portal_tweaking_fire_hold_time( "sixense_portal_tweaking_fire_hold_time", "0.375", FCVAR_REPLICATED | FCVAR_CHEAT, "Minimum time button held after firing portal to enter tweaking." );
ConVar sixense_portal_tweaking_break_cos_angle( "sixense_portal_tweaking_break_cos_angle", "0.906", FCVAR_REPLICATED | FCVAR_CHEAT, "Maximum cos(angle) between fire direction and portal until portal tweaking is lost." );
ConVar sixense_portal_tweaking_break_distance( "sixense_portal_tweaking_break_distance", "120", FCVAR_REPLICATED | FCVAR_CHEAT, "Maximum sqr(distance) between current portal location and new one until portal tweaking is lost." );
ConVar sixense_portal_tweaking_failed_break_distance( "sixense_portal_tweaking_failed_break_distance", "32.0", FCVAR_REPLICATED | FCVAR_CHEAT, "Maximum distance between current portal location and failed one until portal tweaking is lost." );
ConVar sixense_portal_tweaking_was_delay( "sixense_portal_tweaking_was_delay", "0.2", FCVAR_REPLICATED | FCVAR_CHEAT, "Delay after portal tweaking where WasPortalTweaking() returns true." );
ConVar sixense_portal_tweaking_align_blend_time( "sixense_portal_tweaking_align_blend_time", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_strength_noise( "sixense_portal_tweaking_strength_noise", "10.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_pitch_1( "sixense_portal_tweaking_pitch_1", "100.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_pitch_2( "sixense_portal_tweaking_pitch_2", "125.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_strength_pitch( "sixense_portal_tweaking_strength_pitch", "150.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_velocity_pitch( "sixense_portal_tweaking_velocity_pitch", "50.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_roll_velocity_pitch( "sixense_portal_tweaking_roll_velocity_pitch", "50.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_velocity_min( "sixense_portal_tweaking_velocity_min", "2.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_velocity_max( "sixense_portal_tweaking_velocity_max", "500.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_roll_velocity_min( "sixense_portal_tweaking_roll_velocity_min", "10.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_roll_velocity_max( "sixense_portal_tweaking_roll_velocity_max", "180.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_velocity_time( "sixense_portal_tweaking_velocity_time", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_volume( "sixense_portal_tweaking_volume", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_move_volume( "sixense_portal_tweaking_move_volume", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_whoosh_pitch( "sixense_portal_tweaking_whoosh_pitch", "25", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_disabled_with_scaled_cube( "sixense_portal_tweaking_disabled_with_scaled_cube", "1", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_portal_tweaking_moved_time( "sixense_portal_tweaking_moved_time", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT );

//
// scaling convars
//

ConVar sixense_scaling_hold_radius_mode( "sixense_scaling_hold_radius_mode", "4", FCVAR_REPLICATED | FCVAR_CHEAT, "0 = bounding radius, 1 = use MAX( min, bounding radius )" );
ConVar sixense_scaling_hold_radius( "sixense_scaling_hold_radius", "32.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_scaling_hold_radius_blend_time( "sixense_scaling_hold_radius_blend_time", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_scaling_error_blend_time( "sixense_scaling_error_blend_time", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_scaling_error_noise( "sixense_scaling_error_noise", "2.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_scaling_increment_controller_distance( "sixense_scaling_increment_controller_distance", "15.0", FCVAR_REPLICATED | FCVAR_CHEAT, "Distance in mm controller must be moved to scale object one increment." );
ConVar sixense_scaling_reset_time( "sixense_scaling_reset_time", "0.35", FCVAR_REPLICATED | FCVAR_CHEAT, "Time in seconds button must be hit twice in to perform reset of scale in all 3 dimensions." );
ConVar sixense_scaling_increment( "sixense_scaling_increment", "0.25", FCVAR_REPLICATED | FCVAR_CHEAT, "Scale increment." );
ConVar sixense_scaling_min( "sixense_scaling_min", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT, "Min scale." );
ConVar sixense_scaling_max( "sixense_scaling_max", "9.0", FCVAR_REPLICATED | FCVAR_CHEAT, "Max scale." );
ConVar sixense_scaling_volume( "sixense_scaling_volume", "0.25", FCVAR_REPLICATED | FCVAR_CHEAT, "Scaling sound volume." );
ConVar sixense_scaling_volume_clamped( "sixense_scaling_volume_clamped", "0.35", FCVAR_REPLICATED | FCVAR_CHEAT, "Scaling clamped sound volume." );
ConVar sixense_scaling_volume_initial( "sixense_scaling_volume_initial", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT, "Scaling initiated sound volume." );
ConVar sixense_scaling_pitch_clamped_min( "sixense_scaling_pitch_clamped_min", "140", FCVAR_REPLICATED | FCVAR_CHEAT, "Scaling clamped min sound pitch." );
ConVar sixense_scaling_pitch_clamped_max( "sixense_scaling_pitch_clamped_max", "255", FCVAR_REPLICATED | FCVAR_CHEAT, "Scaling clamped max sound pitch." );
ConVar sixense_scaling_pitch_initial( "sixense_scaling_pitch_initial", "85", FCVAR_REPLICATED | FCVAR_CHEAT, "Scaling initiated sound pitch." );
ConVar sixense_scaling_turret_controller_distance( "sixense_scaling_turret_controller_distance", "20.0", FCVAR_REPLICATED | FCVAR_CHEAT, "Distance in mm controller must be moved to scale turret." );
ConVar sixense_scaling_turret_min( "sixense_scaling_turret_min", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT, "Min turret scale." );
ConVar sixense_scaling_turret_max( "sixense_scaling_turret_max", "1.5", FCVAR_REPLICATED | FCVAR_CHEAT, "Max turret scale." );
ConVar sixense_scaling_turret_increment( "sixense_scaling_turret_increment", "0.05", FCVAR_REPLICATED | FCVAR_CHEAT, "Turret scale increment." );
ConVar sixense_scaling_turret_beam_min( "sixense_scaling_turret_beam_min", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT, "Min turret beam scale." );
ConVar sixense_scaling_turret_beam_max( "sixense_scaling_turret_beam_max", "2.0", FCVAR_REPLICATED | FCVAR_CHEAT, "Max turret beam scale." );
ConVar sixense_scaling_turret_time( "sixense_scaling_turret_time", "1.2", FCVAR_REPLICATED | FCVAR_CHEAT, "Time turret is scaled before exploding." );
ConVar sixense_scaling_turret_model_time( "sixense_scaling_turret_model_time", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT, "Time turret model scales." );
ConVar sixense_scaling_turret_volume( "sixense_scaling_turret_volume", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT, "Volume of sound when turret is scaled." );
ConVar sixense_scaling_turret_pitch_larger( "sixense_scaling_turret_pitch_larger", "140", FCVAR_REPLICATED | FCVAR_CHEAT, "Pitch of sound when turret is scaled larger." );
ConVar sixense_scaling_turret_pitch_smaller( "sixense_scaling_turret_pitch_smaller", "240", FCVAR_REPLICATED | FCVAR_CHEAT, "Pitch of sound when turret is scaled smaller." );
ConVar sixense_scaling_blend( "sixense_scaling_blend", "1", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_scaling_blend_time( "sixense_scaling_blend_time", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar sixense_scaling_controller_pos_offset_z( "sixense_scaling_controller_pos_offset_z", "-100", FCVAR_REPLICATED | FCVAR_CHEAT );

#endif // PORTAL2

#endif // SIXENSE
