//========= Copyright Valve Corporation, All rights reserved. ============//

class ConVar;

#include "cbase.h"
#include "convar.h"

#ifdef SIXENSE

//
// general sixense convars
//

extern ConVar sixense_enabled;
extern ConVar sixense_filter_level;
extern ConVar sixense_features_enabled;
extern ConVar sixense_mode;
extern ConVar sixense_ratchet_lesson_angle_threshold;
extern ConVar sixense_ratchet_lesson_angle_end_threshold;
extern ConVar sixense_ratchet_lesson_angle_hold_time;
extern ConVar sixense_disable_ratchet_lesson;
extern ConVar sixense_enable_tutorial_ratchet_lesson;
extern ConVar sixense_disable_scale_reset_lesson;

//
// grab convars
//

extern ConVar sixense_throw_multiplier;
extern ConVar sixense_hold_multiplier;
extern ConVar sixense_hold_z_scale;
extern ConVar sixense_hold_z_min;
extern ConVar sixense_hold_z_max;
extern ConVar sixense_hold_z_min_no_portalgun;
extern ConVar sixense_hold_z_max_no_portalgun;
extern ConVar sixense_throw_max_vel;
extern ConVar sixense_hold_blend;
extern ConVar sixense_hold_offset_x;
extern ConVar sixense_hold_offset_y;
extern ConVar sixense_hold_offset_z;
extern ConVar sixense_velocity_blend_val;
extern ConVar sixense_hold_error_max_noise;
extern ConVar sixense_hold_error_min;
extern ConVar sixense_hold_error_max;
extern ConVar sixense_hold_sound_pitch;
extern ConVar sixense_hold_sound_scaling_pitch;
extern ConVar sixense_hold_sound_error_pitch;
extern ConVar sixense_hold_move_sound_pitch;
extern ConVar sixense_hold_beam_portal_delay_time;
extern ConVar sixense_hold_length_error;
extern ConVar sixense_hold_length_error_fraction;
extern ConVar sixense_drop_cube_button_funnel_velocity_scale;
extern ConVar sixense_hold_color_error_min;
extern ConVar sixense_hold_color_error_max;
extern ConVar sixense_hold_turret_pickup_time;
extern ConVar sixense_hold_turret_distance;

//
// portal tweaking convars
//

extern ConVar sixense_portal_tweaking_enabled;
extern ConVar sixense_portal_tweaking_roll_scale;
extern ConVar sixense_portal_tweaking_grab_radius;
extern ConVar sixense_portal_tweaking_fire_hold_time;
extern ConVar sixense_portal_tweaking_break_cos_angle;
extern ConVar sixense_portal_tweaking_break_distance;
extern ConVar sixense_portal_tweaking_failed_break_distance;
extern ConVar sixense_portal_tweaking_was_delay;
extern ConVar sixense_portal_tweaking_align_blend_time;
extern ConVar sixense_portal_tweaking_strength_noise;
extern ConVar sixense_portal_tweaking_whoosh_pitch;
extern ConVar sixense_portal_tweaking_pitch_1;
extern ConVar sixense_portal_tweaking_pitch_2;
extern ConVar sixense_portal_tweaking_strength_pitch;
extern ConVar sixense_portal_tweaking_velocity_pitch;
extern ConVar sixense_portal_tweaking_roll_velocity_pitch;
extern ConVar sixense_portal_tweaking_velocity_min;
extern ConVar sixense_portal_tweaking_velocity_max;
extern ConVar sixense_portal_tweaking_roll_velocity_min;
extern ConVar sixense_portal_tweaking_roll_velocity_max;
extern ConVar sixense_portal_tweaking_velocity_time;
extern ConVar sixense_portal_tweaking_volume;
extern ConVar sixense_portal_tweaking_move_volume;
extern ConVar sixense_portal_tweaking_disabled_with_scaled_cube;
extern ConVar sixense_portal_tweaking_moved_time;

//
// scaling convars
//

extern ConVar sixense_scaling_hold_radius_mode;
extern ConVar sixense_scaling_hold_radius;
extern ConVar sixense_scaling_hold_radius_base;
extern ConVar sixense_scaling_hold_radius_blend_time;
extern ConVar sixense_scaling_error_blend_time;
extern ConVar sixense_scaling_error_noise;
extern ConVar sixense_scaling_increment_controller_distance;
extern ConVar sixense_scaling_reset_time;
extern ConVar sixense_scaling_increment;
extern ConVar sixense_scaling_min;
extern ConVar sixense_scaling_max;
extern ConVar sixense_scaling_volume;
extern ConVar sixense_scaling_volume_clamped;
extern ConVar sixense_scaling_volume_initial;
extern ConVar sixense_scaling_pitch_clamped_min;
extern ConVar sixense_scaling_pitch_clamped_max;
extern ConVar sixense_scaling_pitch_initial;
extern ConVar sixense_scaling_turret_controller_distance;
extern ConVar sixense_scaling_turret_min;
extern ConVar sixense_scaling_turret_max;
extern ConVar sixense_scaling_turret_increment;
extern ConVar sixense_scaling_turret_beam_min;
extern ConVar sixense_scaling_turret_beam_max;
extern ConVar sixense_scaling_turret_time;
extern ConVar sixense_scaling_turret_model_time;
extern ConVar sixense_scaling_turret_volume;
extern ConVar sixense_scaling_turret_pitch_larger;
extern ConVar sixense_scaling_turret_pitch_smaller;
extern ConVar sixense_scaling_blend;
extern ConVar sixense_scaling_blend_time;
extern ConVar sixense_scaling_controller_pos_offset_z;

#endif // SIXENSE
