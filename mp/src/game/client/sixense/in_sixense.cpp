//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef _XBOX
//#include <windows.h>
#endif
#include "cbase.h"
#include "convar.h"



#ifdef SIXENSE

#include "in_buttons.h"
#include "sixense/in_sixense.h"
#include "sixense/sixense_convars_extern.h"

#if !defined( HL2_CLIENT_DLL) && !defined( TF_CLIENT_DLL )
#include "weapon_csbase.h"
#endif

#if defined( HL2_CLIENT_DLL )
#include "c_basehlplayer.h"
#endif

#if defined( TF_CLIENT_DLL )
#include "c_tf_player.h"
#include "tf_weaponbase.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_gamerules.h"
#include "backpack_panel.h"
#include "baseviewport.h"
extern ConVar _cl_classmenuopen;
extern const char *COM_GetModDirectory();
#endif

#if defined( CSTRIKE15 ) || defined (CSTRIKE_DLL)
#include "c_cs_player.h"
#endif

#include <isixense.h>
#include <sixense_math.hpp>
#include <sixense_utils/interfaces.hpp>

using sixenseMath::Vector2;
using sixenseMath::Vector3;
using sixenseMath::Vector4;
using sixenseMath::Quat;
using sixenseMath::Line;

#if defined( WIN32 ) && !defined( _X360 )
#define _WIN32_WINNT 0x0502
#endif
#include <winlite.h>

#include "vgui/IVGui.h"
#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Label.h"
#include "ienginevgui.h"
#include "vgui_controls/ImagePanel.h"
#include "flashlighteffect.h"
#include "weapon_selection.h"
#include "iinput.h"
#include "game/client/iviewport.h"
#include "filesystem.h"
#include "sourcevr/isourcevirtualreality.h"

#ifdef TF_CLIENT_DLL
#include "tf_hud_menu_engy_build.h"
#include "tf_hud_menu_engy_destroy.h"
#include "tf_hud_menu_spy_disguise.h"
#endif 

#ifdef PORTAL2
#include "BasePanel.h"
#include "usermessages.h"
#include "cdll_int.h"
#include "iclientmode.h"
#include "c_gameinstructor.h"
#include "glow_outline_effect.h"
#include "portal2/vsixensevideohint.h"
#include "portal2/basemodframe.h"
#include "portal2/c_prop_weightedcube.h"
#include "portal/c_portal_player.h"
#include "c_weapon_portalgun.h"
#include "radialmenu.h"
#endif

#ifdef CSTRIKE15
#include "basepanel.h"
#endif

using sixenseMath::Plane;
using sixenseMath::Vector3;

matrix3x4_t ConvertMatrix( sixenseMath::Matrix4 ss_mat );

static void SixenseSensitivityLevelChanged( IConVar *var, const char *pOldValue, float flOldValue );
static void SixenseConvarChanged( IConVar *var, const char *pOldValue, float flOldValue );

extern ConVar cl_forwardspeed;
extern ConVar cl_sidespeed;

#ifdef PORTAL2
extern BaseModUI::SixenseVideoHint *_video_hint_panel;
static SixenseBaseWarning *base_warning = NULL;
#endif


// sixense variables
ConVar sixense_weapon_select_sensitivity( "sixense_weapon_select_sensitivity", "1.65", FCVAR_ARCHIVE );
ConVar sixense_crouch_sensitivity( "sixense_crouch_sensitivity", "1.5", FCVAR_ARCHIVE );
ConVar sixense_jump_sensitivity( "sixense_jump_sensitivity", "1.2", FCVAR_ARCHIVE );
ConVar sixense_reload_sensitivity( "sixense_reload_sensitivity", "1.5", FCVAR_ARCHIVE );

ConVar sixense_left_handed( "sixense_left_handed", "0", FCVAR_ARCHIVE );

#ifdef PORTAL2
ConVar sixense_scale_disables_jump_timer( "sixense_scale_disables_jump_timer", "1.0" );
ConVar sixense_auto_one_to_one_enabled( "sixense_auto_one_to_one_enabled", "0", FCVAR_ARCHIVE );
ConVar sixense_auto_one_to_one_center_z_bias( "sixense_auto_one_to_one_center_z_bias", "0", FCVAR_ARCHIVE );
ConVar sixense_large_objects_lock_one_to_one( "sixense_large_objects_lock_one_to_one", "0", FCVAR_ARCHIVE );
ConVar sixense_large_objects_lock_one_to_one_size( "sixense_large_objects_lock_one_to_one_size", "3.0", FCVAR_ARCHIVE );
ConVar sixense_dist_one_to_one_enabled( "sixense_dist_one_to_one_enabled", "1", FCVAR_ARCHIVE );
ConVar sixense_dist_one_to_one_dist( "sixense_dist_one_to_one_dist", "65", FCVAR_ARCHIVE );
ConVar sixense_dist_one_to_one_end_ratchet_delay( "sixense_dist_one_to_one_end_ratchet_delay", "0.05", FCVAR_ARCHIVE );
ConVar sixense_pickup_momentary_time( "sixense_pickup_momentary_time", "45", FCVAR_ARCHIVE );
ConVar sixense_auto_one_to_one_start_vel( "sixense_auto_one_to_one_start_vel", "550.0f", FCVAR_ARCHIVE );
ConVar sixense_auto_one_to_one_start_accel( "sixense_auto_one_to_one_start_accel", "2500.0f", FCVAR_ARCHIVE );
ConVar sixense_auto_one_to_one_start_accel_timer( "sixense_auto_one_to_one_start_accel_timer", "800", FCVAR_ARCHIVE );
ConVar sixense_auto_one_to_one_angle_thresh( "sixense_auto_one_to_one_angle_thresh", "0.5235f", FCVAR_ARCHIVE );
ConVar sixense_auto_one_to_one_stop_xy_dist( "sixense_auto_one_to_one_stop_xy_dist", "100", FCVAR_ARCHIVE );
ConVar sixense_auto_one_to_one_stop_z_dist( "sixense_auto_one_to_one_stop_z_dist", "10.0f", FCVAR_ARCHIVE );
ConVar sixense_hold_spin_start_screen_ratio( "sixense_hold_spin_start_screen_ratio", "0.5", FCVAR_ARCHIVE );
ConVar sixense_hold_spin_speed( "sixense_hold_spin_speed", "1.0", FCVAR_ARCHIVE );
ConVar sixense_hold_spin_speed_bias( "sixense_hold_spin_speed_bias", "0.2", FCVAR_ARCHIVE );
ConVar sixense_hold_spin_fade_min_dist( "sixense_hold_spin_fade_min_dist", "0.0",  FCVAR_ARCHIVE );
ConVar sixense_hold_spin_fade_max_dist( "sixense_hold_spin_fade_max_dist", "0.0",  FCVAR_ARCHIVE );
ConVar sixense_hold_spin_damp( "sixense_hold_spin_damp", "0.95",  FCVAR_ARCHIVE, "dampening for camera panning, 0 is no dampening" );
ConVar sixense_hold_spin_max_angle( "sixense_hold_spin_max_angle", "75",  FCVAR_ARCHIVE, "disallow camera tracking if the held object is greater than this angle relative to the players view" );
ConVar sixense_hold_slide_z_min_dist( "sixense_hold_slide_z_min_dist", "20.0f",  FCVAR_ARCHIVE );
ConVar sixense_hold_slide_xy_radius( "sixense_hold_slide_xy_radius", "80.0f",  FCVAR_ARCHIVE );
ConVar sixense_played_tutorial("sixense_played_tutorial", "0", FCVAR_ARCHIVE);
#endif

// 0 = low, 1 = med, 2 = high, 3 = custom
ConVar sixense_sensitivity_level( "sixense_sensitivity_level", "-1", FCVAR_ARCHIVE );
ConVar sixense_controller_angle_mode( "sixense_controller_angle_mode", "0.0f", FCVAR_ARCHIVE );
ConVar sixense_roll_correct_blend( "sixense_roll_correct_blend", "0.965f", FCVAR_ARCHIVE );


ConVar sixense_exit_one_to_one_dot( "sixense_exit_one_to_one_dot", "0.85", FCVAR_ARCHIVE );
ConVar sixense_exit_metroid_blend( "sixense_exit_metroid_blend", "0.95f", FCVAR_ARCHIVE );

ConVar sixense_max_charge_spin( "sixense_max_charge_spin", "3.0f", FCVAR_ARCHIVE );


ConVar sixense_zoom_momentary_time( "sixense_zoom_momentary_time", "500", FCVAR_ARCHIVE );

ConVar sixense_base_offset_x( "sixense_base_offset_x", "0.0", FCVAR_ARCHIVE );
ConVar sixense_base_offset_y( "sixense_base_offset_y", "0.0", FCVAR_ARCHIVE );
ConVar sixense_base_offset_z( "sixense_base_offset_z", "-20.0", FCVAR_ARCHIVE );

ConVar sixense_trigger_threshold( "sixense_trigger_threshold", "0.05",  FCVAR_ARCHIVE );

ConVar sixense_tilt_gesture_angle_threshold( "sixense_tilt_gesture_angle_threshold", "35.0",  FCVAR_ARCHIVE );
ConVar sixense_point_gesture_angle_threshold( "sixense_point_gesture_angle_threshold", "15.0",  FCVAR_ARCHIVE );

ConVar sixense_mouse_enabled( "sixense_mouse_enabled", "1.0",FCVAR_ARCHIVE );
ConVar sixense_mouse_sensitivity( "sixense_mouse_sensitivity", "1.0", FCVAR_ARCHIVE );

ConVar sixense_spring_view_enabled( "sixense_spring_view_enabled", "1.0f", FCVAR_ARCHIVE );
ConVar sixense_spring_view_min_spring( "sixense_spring_view_min_spring", "0.025f", FCVAR_ARCHIVE );
ConVar sixense_spring_view_max_spring( "sixense_spring_view_max_spring", "0.9999f", FCVAR_ARCHIVE );
ConVar sixense_spring_view_min_angle( "sixense_spring_view_min_angle", "1.0f", FCVAR_ARCHIVE );
ConVar sixense_spring_view_max_angle( "sixense_spring_view_max_angle", "45.0f", FCVAR_ARCHIVE );

// mode 0:
ConVar sixense_aim_freeaim_heading_multiplier( "sixense_aim_freeaim_heading_multiplier", "1.5", FCVAR_ARCHIVE );
ConVar sixense_aim_freeaim_pitch_multiplier( "sixense_aim_freeaim_pitch_multiplier", "1.5", FCVAR_ARCHIVE );
ConVar sixense_aim_freeaim_dead_zone_radius( "sixense_aim_freeaim_dead_zone_radius", "0.25", FCVAR_ARCHIVE );
ConVar sixense_aim_freeaim_accel_band_size( "sixense_aim_freeaim_accel_band_size", "20.0", FCVAR_ARCHIVE );
ConVar sixense_aim_freeaim_max_speed( "sixense_aim_freeaim_max_speed", "7.0", FCVAR_ARCHIVE );
ConVar sixense_aim_freeaim_auto_level_rate( "sixense_aim_freeaim_auto_level_rate", "1.0", FCVAR_ARCHIVE );
ConVar sixense_aim_freeaim_accel_band_exponent( "sixense_aim_freeaim_accel_band_exponent", "1.0", FCVAR_ARCHIVE );
ConVar sixense_aim_freeaim_switch_blend_time_enter( "sixense_aim_freeaim_switch_blend_time_enter", "0.5", FCVAR_CHEAT );
ConVar sixense_aim_freeaim_switch_blend_time_exit( "sixense_aim_freeaim_switch_blend_time_exit", "0.25", FCVAR_ARCHIVE );
ConVar sixense_teleport_metroid_blend_time( "sixense_teleport_metroid_blend_time", "3.0", FCVAR_CHEAT );
ConVar sixense_teleport_wait_to_blend_time( "sixense_teleport_wait_to_blend_time", "0.75", FCVAR_CHEAT );

// mode 1:
ConVar sixense_aim_1to1_ratchet_vertical( "sixense_aim_1to1_ratchet_vertical", "1.0", FCVAR_ARCHIVE );
ConVar sixense_aim_1to1_heading_multiplier( "sixense_aim_1to1_heading_multiplier", "3.0", FCVAR_ARCHIVE );
ConVar sixense_aim_1to1_pitch_multiplier( "sixense_aim_1to1_pitch_multiplier", "2.0", FCVAR_ARCHIVE );
ConVar sixense_feet_angles_offset_stick_spin_horiz_multiplier( "sixense_feet_angles_offset_stick_spin_horiz_multiplier", "4.0", FCVAR_ARCHIVE );
ConVar sixense_feet_angles_offset_stick_spin_vert_multiplier( "sixense_feet_angles_offset_stick_spin_vert_multiplier", "2.0", FCVAR_ARCHIVE );
ConVar sixense_feet_angles_offset_stick_spin_invert_pitch( "sixense_feet_angles_offset_stick_spin_invert_pitch", "1.0", FCVAR_ARCHIVE );
ConVar sixense_feet_angles_offset_stick_spin_exponent( "sixense_feet_angles_offset_stick_spin_exponent", "1.0", FCVAR_ARCHIVE );

ConVar sixense_aim_scope_heading_multiplier( "sixense_aim_scope_heading_multiplier", "0.6", FCVAR_ARCHIVE );
ConVar sixense_aim_scope_pitch_multiplier( "sixense_aim_scope_pitch_multiplier", "0.6", FCVAR_ARCHIVE );

ConVar sixense_melee_pitch_blend_val( "sixense_melee_pitch_blend_val", "0.99", FCVAR_ARCHIVE );

ConVar sixense_crosshair_horiz_multiplier( "sixense_crosshair_horiz_multiplier", "1.0", FCVAR_ARCHIVE );
ConVar sixense_crosshair_vert_multiplier( "sixense_crosshair_vert_multiplier", "1.0", FCVAR_ARCHIVE );
ConVar sixense_always_draw_crosshair( "sixense_always_draw_crosshair", "1",  FCVAR_ARCHIVE );

// walking
ConVar sixense_walking_dead_zone_percent( "sixense_walking_dead_zone_percent", "10.0",  FCVAR_ARCHIVE );
ConVar sixense_walking_exponent( "sixense_walking_exponent", "1.0",  FCVAR_ARCHIVE );

SixenseGUIFrame *SixenseInput::m_SixenseFrame = NULL;

SixenseInput *g_pSixenseInput = NULL;

CSysModule *g_pSixenseModule = NULL;
CSysModule *g_pSixenseUtilsModule = NULL;

// A bunch of our convars have their values pushed down into the sixense_utils lib, so install a handler on each
// to let us know if there are any changes
void SixenseInput::InstallConvarCallbacks() 
{

	sixense_mode.InstallChangeCallback( SixenseConvarChanged );

	sixense_weapon_select_sensitivity.InstallChangeCallback( SixenseConvarChanged );
	sixense_crouch_sensitivity.InstallChangeCallback( SixenseConvarChanged );
	sixense_jump_sensitivity.InstallChangeCallback( SixenseConvarChanged );
	sixense_reload_sensitivity.InstallChangeCallback( SixenseConvarChanged );
	sixense_controller_angle_mode.InstallChangeCallback( SixenseConvarChanged );

	sixense_aim_freeaim_heading_multiplier.InstallChangeCallback( SixenseConvarChanged );
	sixense_aim_freeaim_pitch_multiplier.InstallChangeCallback( SixenseConvarChanged );
	sixense_aim_freeaim_dead_zone_radius.InstallChangeCallback( SixenseConvarChanged );
	sixense_aim_freeaim_accel_band_size.InstallChangeCallback( SixenseConvarChanged );
	sixense_aim_freeaim_auto_level_rate.InstallChangeCallback( SixenseConvarChanged );
	sixense_aim_freeaim_accel_band_exponent.InstallChangeCallback( SixenseConvarChanged );

#ifdef PORTAL2
	sixense_auto_one_to_one_start_vel.InstallChangeCallback( SixenseConvarChanged );
	sixense_auto_one_to_one_start_accel.InstallChangeCallback( SixenseConvarChanged );
	sixense_auto_one_to_one_start_accel_timer.InstallChangeCallback( SixenseConvarChanged );
	sixense_auto_one_to_one_angle_thresh.InstallChangeCallback( SixenseConvarChanged );
	sixense_auto_one_to_one_stop_xy_dist.InstallChangeCallback( SixenseConvarChanged );
	sixense_auto_one_to_one_stop_z_dist.InstallChangeCallback( SixenseConvarChanged );
#endif


	sixense_aim_freeaim_switch_blend_time_exit.InstallChangeCallback( SixenseConvarChanged );
	sixense_roll_correct_blend.InstallChangeCallback( SixenseConvarChanged );
	sixense_exit_metroid_blend.InstallChangeCallback( SixenseConvarChanged );

	sixense_spring_view_min_spring.InstallChangeCallback( SixenseConvarChanged );
	sixense_spring_view_max_spring.InstallChangeCallback( SixenseConvarChanged );
	sixense_spring_view_min_angle.InstallChangeCallback( SixenseConvarChanged );
	sixense_spring_view_max_angle.InstallChangeCallback( SixenseConvarChanged );
	sixense_melee_pitch_blend_val.InstallChangeCallback( SixenseConvarChanged );

	sixense_aim_1to1_ratchet_vertical.InstallChangeCallback( SixenseConvarChanged );

	sixense_aim_1to1_heading_multiplier.InstallChangeCallback( SixenseConvarChanged );
	sixense_aim_1to1_pitch_multiplier.InstallChangeCallback( SixenseConvarChanged );
	sixense_spring_view_enabled.InstallChangeCallback( SixenseConvarChanged );

	sixense_feet_angles_offset_stick_spin_horiz_multiplier.InstallChangeCallback( SixenseConvarChanged );
	sixense_feet_angles_offset_stick_spin_vert_multiplier.InstallChangeCallback( SixenseConvarChanged );
	sixense_feet_angles_offset_stick_spin_invert_pitch.InstallChangeCallback( SixenseConvarChanged );
	sixense_feet_angles_offset_stick_spin_exponent.InstallChangeCallback( SixenseConvarChanged );

	sixense_walking_dead_zone_percent.InstallChangeCallback( SixenseConvarChanged );
	sixense_walking_exponent.InstallChangeCallback( SixenseConvarChanged );

	sixense_tilt_gesture_angle_threshold.InstallChangeCallback( SixenseConvarChanged );
	sixense_point_gesture_angle_threshold.InstallChangeCallback( SixenseConvarChanged );

	sixense_trigger_threshold.InstallChangeCallback( SixenseConvarChanged );

	sixense_aim_scope_heading_multiplier.InstallChangeCallback( SixenseConvarChanged );

}

// If any of the interesting convars changed this frame, this call will push all the values to where they need to be.
void SixenseInput::UpdateValuesFromConvars()
{

	if( !m_pFPSEvents )
	{
		return;
	}

	int mode = sixense_mode.GetInt();

	if ( mode < 0 || mode > 2 )
	{
		mode = 0;
	}

	SetMode( mode );

	// Set all the parameters from the cvars
	m_pFPSEvents->setParameter( sixenseUtils::IFPSEvents::WEAPON_SELECT_SENSITIVITY, sixense_weapon_select_sensitivity.GetFloat() );
	m_pFPSEvents->setParameter( sixenseUtils::IFPSEvents::CROUCH_SENSITIVITY, sixense_crouch_sensitivity.GetFloat() );
	m_pFPSEvents->setParameter( sixenseUtils::IFPSEvents::JUMP_SENSITIVITY, sixense_jump_sensitivity.GetFloat() );
	m_pFPSEvents->setParameter( sixenseUtils::IFPSEvents::RELOAD_SENSITIVITY, sixense_reload_sensitivity.GetFloat() );
	m_pFPSEvents->setParameter( sixenseUtils::IFPSEvents::CONTROLLER_ANGLE_MODE, sixense_controller_angle_mode.GetFloat() );

#ifdef PORTAL2
	m_pFPSEvents->setParameter( sixenseUtils::IFPSEvents::AUTO_ONE_TO_ONE_START_VEL, sixense_auto_one_to_one_start_vel.GetFloat() );
	m_pFPSEvents->setParameter( sixenseUtils::IFPSEvents::AUTO_ONE_TO_ONE_START_ACCEL, sixense_auto_one_to_one_start_accel.GetFloat() );
	m_pFPSEvents->setParameter( sixenseUtils::IFPSEvents::AUTO_ONE_TO_ONE_START_ACCEL_TIMER, sixense_auto_one_to_one_start_accel_timer.GetFloat() );
	m_pFPSEvents->setParameter( sixenseUtils::IFPSEvents::AUTO_ONE_TO_ONE_START_ANGLE_THRESH, sixense_auto_one_to_one_angle_thresh.GetFloat() );
	m_pFPSEvents->setParameter( sixenseUtils::IFPSEvents::AUTO_ONE_TO_ONE_STOP_XY_DIST, sixense_auto_one_to_one_stop_xy_dist.GetFloat() );
	m_pFPSEvents->setParameter( sixenseUtils::IFPSEvents::AUTO_ONE_TO_ONE_STOP_Z_DIST, sixense_auto_one_to_one_stop_z_dist.GetFloat() );
#endif

	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_HEADING_MULTIPLIER, sixense_aim_freeaim_heading_multiplier.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_PITCH_MULTIPLIER, sixense_aim_freeaim_pitch_multiplier.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_DEAD_ZONE_RADIUS, sixense_aim_freeaim_dead_zone_radius.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_ACCEL_BAND_SIZE, sixense_aim_freeaim_accel_band_size.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_AUTO_LEVEL_RATE, sixense_aim_freeaim_auto_level_rate.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_ACCEL_BAND_EXPONENT, sixense_aim_freeaim_accel_band_exponent.GetFloat() );

	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_SWITCH_BLEND_TIME_EXIT, sixense_aim_freeaim_switch_blend_time_exit.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::CONTROLLER_ANGLE_MODE, sixense_controller_angle_mode.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::ROLL_CORRECTION_BLEND, sixense_roll_correct_blend.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::EXIT_METROID_BLEND, sixense_exit_metroid_blend.GetFloat() );

	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::SPRING_VIEW_MIN_SPRING, sixense_spring_view_min_spring.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::SPRING_VIEW_MAX_SPRING, sixense_spring_view_max_spring.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::SPRING_VIEW_MIN_ANGLE, sixense_spring_view_min_angle.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::SPRING_VIEW_MAX_ANGLE, sixense_spring_view_max_angle.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::PITCH_CHANGE_BLEND_VAL, sixense_melee_pitch_blend_val.GetFloat() );

	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_1TO1_RATCHET_VERTICAL, sixense_aim_1to1_ratchet_vertical.GetFloat() );

	// In metroid mode, we switch into mouselook when looking down scope, so configure mouselook differently.
	if( sixense_mode.GetInt() == 1 ) 
	{
		m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_1TO1_HEADING_MULTIPLIER, sixense_aim_1to1_heading_multiplier.GetFloat() );
		m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_1TO1_PITCH_MULTIPLIER, sixense_aim_1to1_pitch_multiplier.GetFloat() );
		m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::SPRING_VIEW_ENABLED, sixense_spring_view_enabled.GetFloat() );
	} 
	else if( sixense_mode.GetInt() == 0 ) 
	{

		// Use scope sensitivites in metroid mode, so when we look down the scope it's set correctly
		m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_1TO1_HEADING_MULTIPLIER, sixense_aim_scope_heading_multiplier.GetFloat() );
		m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_1TO1_PITCH_MULTIPLIER, sixense_aim_scope_pitch_multiplier.GetFloat() );

		// No springview when looking down scope
		m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::SPRING_VIEW_ENABLED, 0 );
	}


	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::FEET_ANGLES_OFFSET_STICK_SPIN_HORIZ_MULTIPLIER, sixense_feet_angles_offset_stick_spin_horiz_multiplier.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::FEET_ANGLES_OFFSET_STICK_SPIN_VERT_MULTIPLIER, sixense_feet_angles_offset_stick_spin_vert_multiplier.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::FEET_ANGLES_OFFSET_STICK_SPIN_INVERT_PITCH, sixense_feet_angles_offset_stick_spin_invert_pitch.GetFloat() );
	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::FEET_ANGLES_OFFSET_STICK_SPIN_EXPONENT, sixense_feet_angles_offset_stick_spin_exponent.GetFloat() );

	m_pFPSPlayerMovement->setParameter( sixenseUtils::IFPSPlayerMovement::DEAD_ZONE_PERCENT, sixense_walking_dead_zone_percent.GetFloat() );
	m_pFPSPlayerMovement->setParameter( sixenseUtils::IFPSPlayerMovement::EXPONENTIAL, sixense_walking_exponent.GetFloat() );

	m_pLeftButtonStates->setAbsoluteTiltAngleThresholdInDeg( sixense_tilt_gesture_angle_threshold.GetFloat() );
	m_pLeftButtonStates->setRelativeTiltAngleThresholdInDeg( sixense_point_gesture_angle_threshold.GetFloat() );

	m_pLeftButtonStates->setTriggerThreshold( sixense_trigger_threshold.GetFloat() );
	m_pRightButtonStates->setTriggerThreshold( sixense_trigger_threshold.GetFloat() );

}

// Try to load sixense.dll and sixense_utils.dll
bool SixenseInput::LoadModules() 
{

	// If the modules are already loaded return success
	if( m_bModulesLoaded ) 
		return true;

	// Try to load the sixense DLLs

	g_pSixenseModule = Sys_LoadModule( "sixense" );

	if( !g_pSixenseModule )
	{
		Msg("Failed to load sixense.dll\n");
		return false;
	}

	g_pSixenseUtilsModule = Sys_LoadModule( "sixense_utils" );

	if( !g_pSixenseUtilsModule )
	{
		Msg("Failed to load sixense_utils.dll\n");
		return false;
	}

	Msg("Successfully loaded sixense modules.\n");

	bool found_objects = false;

	if(g_pSixenseModule)
	{
		CreateInterfaceFn factory = Sys_GetFactory( g_pSixenseModule );

		if( factory ) 
		{
			m_pSixenseAPI = reinterpret_cast< ISixenseAPI* >( factory( "SixenseAPI", NULL ) );

			if( m_pSixenseAPI )
			{
				found_objects = true;
			}
		}
	}

	if( !found_objects ) 
	{
		Msg("Failed to find factory in sixense.dll\n");
		return false;
	}


	// Now try to init from sixense_utils
	found_objects = false;

	if(g_pSixenseUtilsModule)
	{
		CreateInterfaceFn factory = Sys_GetFactory( g_pSixenseUtilsModule );

		if( factory ) 
		{

			m_pFPSViewAngles = reinterpret_cast< sixenseUtils::IFPSViewAngles* >( factory( "FPSViewAngles0", NULL ) );
			m_pFPSPlayerMovement = reinterpret_cast< sixenseUtils::IFPSPlayerMovement* >( factory( "FPSPlayerMovement0", NULL ) );
			m_pFPSEvents = reinterpret_cast< sixenseUtils::IFPSEvents* >( factory( "FPSEvents0", NULL ) );

			m_pLaserPointer = reinterpret_cast< sixenseUtils::ILaserPointer* >( factory( "LaserPointer0", NULL ) );

			m_pLeftDeriv = reinterpret_cast< sixenseUtils::IDerivatives* >( factory( "Derivatives0", NULL ) );
			m_pRightDeriv = reinterpret_cast< sixenseUtils::IDerivatives* >( factory( "Derivatives1", NULL ) );

			m_pLeftButtonStates = reinterpret_cast< sixenseUtils::IButtonStates* >( factory( "ButtonStates0", NULL ) );
			m_pRightButtonStates = reinterpret_cast< sixenseUtils::IButtonStates* >( factory( "ButtonStates1", NULL ) );

			m_pControllerManager = reinterpret_cast< sixenseUtils::IControllerManager* >( factory( "ControllerManager", NULL ) );

			if( m_pFPSViewAngles && m_pFPSPlayerMovement && m_pFPSEvents && m_pLaserPointer && m_pLeftDeriv && 
				m_pRightDeriv && m_pLeftButtonStates && m_pRightButtonStates && m_pControllerManager )
			{
				found_objects = true;
			}
		}

	}

	if( !found_objects ) 
	{
		Msg("Failed to find factory in sixense_utils.dll\n");
		return false;
	}

	m_bModulesLoaded = true;

	Init();

	// We can't set the mode until modules are loaded, so do it now
	SetMode( sixense_mode.GetInt() );

	return true;
}

bool SixenseInput::UnloadModules()
{
	if( g_pSixenseModule ) 
	{
		Sys_UnloadModule( g_pSixenseModule );
		g_pSixenseModule = NULL;
	}

	if( g_pSixenseUtilsModule ) 
	{
		Sys_UnloadModule( g_pSixenseUtilsModule );
		g_pSixenseUtilsModule = NULL;
	}

	m_bModulesLoaded = false;

	Shutdown();

	return true;
}

#if 0
static void update_controller_manager_visibility( sixenseAllControllerData *acd ) 
{

	if( !SixenseInput::m_SixenseFrame ) return;

	bool controllers_docked = acd->controllers[0].is_docked || acd->controllers[1].is_docked;


	bool power_up_screens_showing = false;
	// It's ok for there to be no device plugged in, don't show the controller manager in that case
	std::string current_step = m_pControllerManager->getTextureFileName();
	if( current_step == "1P2C/p1c2_power_up_0.tif" || current_step == "1P2C/p1c2_power_up_1.tif" ) 
	{
		power_up_screens_showing = true;
	}

	//Msg("current step %s\n", current_step.c_str() );

	// if sixense is enabled, and the cm is trying to say something, and it's not trying to show the power up screens, and the controllers arent docked show the frame
	if( g_pSixenseInput->IsEnabled() && 
		m_pControllerManager->isMenuVisible() &&
		!power_up_screens_showing && 
		!controllers_docked ) 
	{
		if( !SixenseInput::m_SixenseFrame->IsVisible() ) 
		{
			SixenseInput::m_SixenseFrame->SetVisible( true );
			SixenseInput::m_SixenseFrame->MoveToFront();

			// Pause the engine if we can...
			engine->ClientCmd_Unrestricted( "setpause nomsg" );
		}
	}
	else 
	{
		if( SixenseInput::m_SixenseFrame->IsVisible() ) 	// otherwise turn it off

		{
			SixenseInput::m_SixenseFrame->SetVisible( false );
			engine->ClientCmd_Unrestricted( "unpause nomsg" );

		}
	}

}
#endif

static void controller_manager_setup_callback( sixenseUtils::IControllerManager::setup_step SetupStep )
{

	g_pSixenseInput->controllerManagerCallback( (int)SetupStep );
}

void SixenseInput::controllerManagerCallback( int iSetupStep )
{
#if 0
	Msg( "controller_manager_setup_callback: %s\n", m_pControllerManager->getTextureFileName() );

	if ( m_pControllerManager->isMenuVisible() )
	{

		if ( SixenseInput::m_SixenseFrame )
		{

			sixenseUtils::sixense_utils_string tex_name = m_pControllerManager->getTextureFileName();

			std::string image_name( tex_name.begin(), tex_name.end() );

			image_name = image_name.substr( image_name.find_last_of( '/' ) );
			image_name = image_name.substr( 0, image_name.find_first_of( '.' ) );

			CUtlString cm_str( "../sixense_controller_manager" );
			cm_str.Append( image_name.c_str() );

			SixenseInput::m_SixenseFrame->setImage( cm_str );

		}
	}

	if ( m_pControllerManager->shouldPlaySound() == sixenseUtils::IControllerManager::SUCCESS_BEEP )
	{
		vgui::surface()->PlaySound( "UI/buttonclickrelease.wav" );
	}
#endif

}

#ifdef PORTAL2
ConCommand sixense_reset_view( "sixense_reset_view", reset_view );

static void reset_view( const CCommand &args )
{

	if ( args.ArgC() != 4 )
	{
		Warning( "Incorrect parameters. Format: sixense_reset_view <pitch> <yaw> <roll>\n" );
		return;
	}

	QAngle reset_angs;

	reset_angs[PITCH] = ( float )atof( args[1] );
	reset_angs[YAW] = ( float )atof( args[2] );
	reset_angs[ROLL] = ( float )atof( args[3] );

	g_pSixenseInput->ResetView( reset_angs );

}


static void SixenseAutosave( const CCommand &args )
{
	if ( sixense_enabled.GetInt() )
	{
		const char szSaveName[32] = "autosave_motionpack";
		char szFullSaveFileName[32];
		char szComment[32];

		engine->SaveGame( 
			szSaveName, 
			IsX360(), 
			szFullSaveFileName, 
			sizeof( szFullSaveFileName ),
			szComment,
			sizeof( szComment ) );
	}
}

ConCommand sixense_autosave( "sixense_autosave", SixenseAutosave );

#endif

////////////

#define INPUT_EVENTS
#ifdef INPUT_EVENTS
static void sendMouseClick( int click, int release )
{
#ifdef WIN32
	// Set up the input event struct
	INPUT input_ev[1]; 

	input_ev[0].type = INPUT_MOUSE;

	PMOUSEINPUT mouse_evp;
	mouse_evp = &input_ev[0].mi;

	mouse_evp->dx = 0;
	mouse_evp->dy = 0;
	mouse_evp->time=0;

	if( click ) 
	{
		if( click == 1 ) 
		{
			mouse_evp->dwFlags = MOUSEEVENTF_LEFTDOWN;
		}
		else if( click == 2 ) 
		{
			mouse_evp->dwFlags = MOUSEEVENTF_RIGHTDOWN;
		}
		else if( click == 3 ) 
		{
			mouse_evp->dwFlags = MOUSEEVENTF_MIDDLEDOWN;
		}

		SendInput( 1, input_ev, sizeof( INPUT ) );
	}

	if( release ) 
	{
		if( release == 1 ) 
		{
			mouse_evp->dwFlags = MOUSEEVENTF_LEFTUP;
		}
		else if( release == 2 ) 
		{
			mouse_evp->dwFlags = MOUSEEVENTF_RIGHTUP;
		}
		else if( release == 3 ) 
		{
			mouse_evp->dwFlags = MOUSEEVENTF_MIDDLEUP;
		}

		SendInput( 1, input_ev, sizeof( INPUT ) );
	}
#endif
}

static void sendKeyState( char key, int press, int release )
{
#ifdef WIN32
	// Set up the input event struct
	INPUT input_ev[1];

	input_ev[0].type = INPUT_KEYBOARD;

	PKEYBDINPUT key_evp;
	key_evp = &input_ev[0].ki;

	key_evp->wScan = key;
	key_evp->time = 0;

	unsigned short extended = 0;

	if( key & 0x80 ) {
		extended = KEYEVENTF_EXTENDEDKEY;
		key_evp->wScan =  key & ~0xff80;
	}

	if( press ) {
		key_evp->dwFlags = extended | KEYEVENTF_SCANCODE;

		SendInput( 1, input_ev, sizeof( INPUT ) );
	}

	if( release ) {
		key_evp->dwFlags = extended | KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

		SendInput( 1, input_ev, sizeof( INPUT ) );
	}
#endif
}

static void sendAbsoluteMouseMove( float x, float y ) 
{
#ifdef WIN32
	// Set up the input event struct
	INPUT input_ev[1];

	input_ev[0].type = INPUT_MOUSE;

	PMOUSEINPUT mouse_evp;
	mouse_evp = &input_ev[0].mi;

	mouse_evp->time=0;

	mouse_evp->dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | 0x2000;

	mouse_evp->dx = (int)(65535.0f*x);
	mouse_evp->dy = 65535-(int)(65535.0f*y);

	SendInput( 1, input_ev, sizeof( INPUT ) );
#endif
}
#endif

#if 0
#ifdef DEBUG
extern "C" int sixenseSetDebugParam( const char *param_name, float val );
extern "C" int sixenseGetDebugParam( const char *param_name, float *val );
static void inc_debug_val( const CCommand &args )
{

	if ( args.ArgC() != 3 )
	{
		Warning( "Incorrect parameters. Format: sixense_set_debug_val <var> <val>\n" );
		return;
	}

	float current;
	sixenseGetDebugParam( args[1], &current );

	float new_val = current + atof( args[2] );
	sixenseSetDebugParam( args[1], new_val );

	Msg( "set \"%s\" to %f\n", args[1], new_val );
}

ConCommand sixense_inc_debug_val( "sixense_inc_debug_val", inc_debug_val );
#endif
#endif

void SixenseConvarChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
	if( g_pSixenseInput )
	{
		g_pSixenseInput->ConvarChanged();
	}
}

void SixenseInput::ConvarChanged()
{
	m_bConvarChanged = true;
}

SixenseInput::SixenseInput()
{
	m_bModulesLoaded = false;

	m_pSixenseAPI = NULL;

	m_pFPSViewAngles = NULL;
	m_pFPSPlayerMovement = NULL;
	m_pFPSEvents = NULL;
	m_pLaserPointer = NULL;

	m_pLeftDeriv = NULL;
	m_pRightDeriv = NULL;
	
	m_pLeftButtonStates = NULL;
	m_pRightButtonStates = NULL;
	
	m_bWasInMenuMode = false;
	
	m_pLaserPointer = NULL;

	m_pControllerManager = NULL;

	m_pACD = NULL;

	m_bConvarChanged = true;

	m_bIsEnabled = false; // sixense.dll loaded
	m_bIsActive = false; // controllers not docked

	m_nFilterLevel = 1;
	m_bMoveMouseToCenter = false;

	m_bShouldSetBaseOffset = false;

	m_LastViewMode = sixenseUtils::IFPSViewAngles::FREE_AIM_TWO_CONTROLLER;

	sixense_sensitivity_level.InstallChangeCallback( SixenseSensitivityLevelChanged );

	// Don't listen to our convars until sixense is loaded
	InstallConvarCallbacks();

	m_nFreeaimSpinDisabled = 0;
	m_nGesturesDisabled = 0;
	m_fTeleportWaitToBlendTime = 0.0f;

	m_bPlayerValid = false;

	m_nShouldUnduck = false;

	m_nLeftIndex = -1;
	m_nRightIndex = -1;

	m_bJustSpawned = false;

	// For keeping track of the previous mode when looking down the scope changes it.
	m_bScopeSwitchedMode = false;
	m_nScopeSwitchedPrevSpringViewEnabled = 0;

	m_pGestureBindings = new SixenseGestureBindings;

#ifdef PORTAL2
	m_bJustPortalled = false;
	m_bIsLeftTriggerDown = false;
	m_bIsRightTriggerDown = false;
	m_fDisableJumpUntil = 0.0f;
	m_AnglesToRightHand.Init();
	m_AnglesToLeftHand.Init();

	m_bIsIn1to1Mode = false;
	m_bIs1to1ModeLocked = false;
	m_bIs1to1ModeScaling = false;
	m_bIs1to1ModeRatcheting = false;
	m_bExitOneWhenAimingForwards = false;

	m_bScalingLockedOneToOne = false;

	m_bIsTweaking = false;

	m_nGlowIndex = -1;

	m_fLastHorizSpeedMult = 0.0f;
	m_fLastVertSpeedMult = 0.0f;
#endif

}


SixenseInput::~SixenseInput()
{
}

bool SixenseInput::IsSixenseMap()
{
#ifdef PORTAL2
	if ( Q_strncmp( engine->GetLevelName(), "maps/sixense_", 13 ) == 0 )
	{
		return true;
	}
#endif

	return false;
}

SixenseGestureBindings *SixenseInput::GetGestureBindings()
{
	return m_pGestureBindings;
}

void SixenseInput::FireGameEvent( IGameEvent *pEvent )
{
	const char *eventName = pEvent->GetName();
	if ( !eventName )
		return;

	if( FStrEq(eventName, "player_spawn") )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

		C_BasePlayer *spawner = UTIL_PlayerByUserId( pEvent->GetInt( "userid" ) );

		if( pPlayer == spawner )
		{
			m_bJustSpawned = true;
		}
	}

#ifdef PORTAL2
	if ( Q_strcmp( type, "sixense_player_teleported" ) == 0 )
	{
		int	playerId = pEvent->GetInt( "userid", 0 );

		// Only set view on player that sent the event
		C_BasePlayer * pPlayer = C_BasePlayer::GetLocalPlayer();

		if ( pPlayer && pPlayer->GetUserID() == playerId )
		{
			float x = pEvent->GetFloat( "pitch", 0 );
			float y = pEvent->GetFloat( "yaw", 0 );
			float z = pEvent->GetFloat( "roll", 0 );
			float isInitialSpawn = pEvent->GetBool( "isInitialSpawn", false );

			if ( isInitialSpawn )
			{
				PlayerSpawn();
			}

			QAngle newAngle;
			newAngle.Init( x, y, z );
			ResetView( newAngle );

			m_fTeleportWaitToBlendTime = gpGlobals->curtime + sixense_teleport_wait_to_blend_time.GetFloat();
		}
	}

	else if ( Q_strcmp( type, "player_drop" ) == 0 )
	{
		int playerId = pEvent->GetInt( "userid", 0 );
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer && ( pPlayer->GetUserID() == playerId ) )
		{
			PlayerDroppedEntity( pEvent->GetInt( "entity", 0 ) );
		}
	}

	else if ( Q_strcmp( type, "player_use" ) == 0 )
	{
		int playerId = pEvent->GetInt( "userid", 0 );
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer && pPlayer->GetUserID() == playerId )
		{
			PlayerUsedEntity( pEvent->GetInt( "entity", 0 ) );
		}
	}
#endif
}

#ifdef PORTAL2
void SixenseInput::PlayerDroppedEntity( int entityID )
{
	if ( UTIL_IsScaledCube( cl_entitylist->GetBaseEntity( entityID ) ) &&
		!m_bIsIn1to1Mode &&
		m_pFPSViewAngles &&
		( m_pFPSViewAngles->getParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_MAX_SPEED ) <= sixense_aim_freeaim_max_speed.GetFloat() ) )
	{
		m_pFPSViewAngles->forceMetroidBlend( m_pFPSViewAngles->getParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_MAX_SPEED ) / sixense_aim_freeaim_max_speed.GetFloat() );
	}
}

void SixenseInput::PlayerUsedEntity( int entityID )
{
}

void SixenseInput::PlayerPortalled( const VMatrix &PortalMatrix )
{

	sixenseMath::Vector3 ss_view_angles = g_pSixenseInput->m_pFPSViewAngles->getViewAngles();
	QAngle new_viewangles;
	new_viewangles[YAW] = ss_view_angles[0];
	new_viewangles[PITCH] = ss_view_angles[1];
	new_viewangles[ROLL] = ss_view_angles[2];



	QAngle corrected_viewangles;
	UTIL_Portal_AngleTransform( PortalMatrix, new_viewangles, corrected_viewangles );

	//Msg("Fixed local view angles %f %f %f\n", corrected_viewangles[YAW], corrected_viewangles[PITCH], corrected_viewangles[ROLL] );

	g_pSixenseInput->ForceViewAngles( corrected_viewangles );

}

void SixenseInput::SetPortalTweakingParameters( bool bIsTweaking )
{
	// set params if state changed
	if ( m_bIsTweaking != bIsTweaking )
	{
		// set new state
		m_bIsTweaking = bIsTweaking;

		// set params
		if ( m_bIsTweaking )
		{
			// save previous values
			m_fTweakSixenseAimFreeaimAccelBandExponent = sixense_aim_freeaim_accel_band_exponent.GetFloat();
			m_fTweakSixenseAimFreeaimAutoLevelRate = sixense_aim_freeaim_auto_level_rate.GetFloat();
			m_fTweakSixenseAimFreeaimAccelBandSize = sixense_aim_freeaim_accel_band_size.GetFloat();
			m_fTweakSixenseAimFreeaimMaxSpeed = sixense_aim_freeaim_max_speed.GetFloat();
			m_fTweakSixenseAimFreeaimDeadZoneRadius = sixense_aim_freeaim_dead_zone_radius.GetFloat();
			m_fTweakSixenseAimFreeaimHeadingMultiplier = sixense_aim_freeaim_heading_multiplier.GetFloat();
			m_fTweakSixenseAimFreeaimPitchMultiplier = sixense_aim_freeaim_pitch_multiplier.GetFloat();
			m_fTweakSixenseAim1to1HeadingMultiplier = sixense_aim_1to1_heading_multiplier.GetFloat();
			m_fTweakSixenseAim1to1PitchMultiplier = sixense_aim_1to1_pitch_multiplier.GetFloat();

			// set tweak values
			sixense_aim_freeaim_accel_band_exponent.SetValue( 1.0f );
			sixense_aim_freeaim_auto_level_rate.SetValue( 1.0f );
			sixense_aim_freeaim_accel_band_size.SetValue( 25.0f );
			sixense_aim_freeaim_max_speed.SetValue( 3.0f );
			sixense_aim_freeaim_dead_zone_radius.SetValue( 2.0f );
			sixense_aim_freeaim_heading_multiplier.SetValue( 1.0f );
			sixense_aim_freeaim_pitch_multiplier.SetValue( 1.0f );
			sixense_aim_1to1_heading_multiplier.SetValue( 2.0f );
			sixense_aim_1to1_pitch_multiplier.SetValue( 1.5f );
		}
		else
		{
			// restore values
			sixense_aim_freeaim_accel_band_exponent.SetValue( m_fTweakSixenseAimFreeaimAccelBandExponent );
			sixense_aim_freeaim_auto_level_rate.SetValue( m_fTweakSixenseAimFreeaimAutoLevelRate );
			sixense_aim_freeaim_accel_band_size.SetValue( m_fTweakSixenseAimFreeaimAccelBandSize );
			sixense_aim_freeaim_max_speed.SetValue( m_fTweakSixenseAimFreeaimMaxSpeed );
			sixense_aim_freeaim_dead_zone_radius.SetValue( m_fTweakSixenseAimFreeaimDeadZoneRadius );
			sixense_aim_freeaim_heading_multiplier.SetValue( m_fTweakSixenseAimFreeaimHeadingMultiplier );
			sixense_aim_freeaim_pitch_multiplier.SetValue( m_fTweakSixenseAimFreeaimPitchMultiplier );
			sixense_aim_1to1_heading_multiplier.SetValue( m_fTweakSixenseAim1to1HeadingMultiplier );
			sixense_aim_1to1_pitch_multiplier.SetValue( m_fTweakSixenseAim1to1PitchMultiplier );

			// force blend back to original mode
			if ( m_pFPSViewAngles->getParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_MAX_SPEED ) <= sixense_aim_freeaim_max_speed.GetFloat() )
			{
				m_pFPSViewAngles->forceMetroidBlend( m_pFPSViewAngles->getParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_MAX_SPEED ) / sixense_aim_freeaim_max_speed.GetFloat() );
			}
		}
	}
}

void SixenseInput::SetOneToOneMode( bool bOnOrOff )
{

	// Dont do anything if we're already in that mode
	if ( bOnOrOff == m_bIsIn1to1Mode ) return;

	Vector glowColor;
	glowColor.x = 1.0f;
	glowColor.y = 1.0f;
	glowColor.z = 0.5f;

	if ( bOnOrOff )
	{

		m_LastViewMode = m_pFPSViewAngles->getMode();
		m_pFPSViewAngles->setMode( sixenseUtils::IFPSViewAngles::DUAL_ANALOG );
		m_bIsIn1to1Mode = true;
		m_bIs1to1ModeLocked = true;

		// Start glowing
		m_nGlowIndex = g_GlowObjectManager.RegisterGlowObject( GetHeldObject(), glowColor, 0.25f, GET_ACTIVE_SPLITSCREEN_SLOT() );

		if ( !sixense_dist_one_to_one_enabled.GetInt() )
		{
			// Auto calib
			SetBaseOffset();
		}

	}
	else
	{

		m_bScalingLockedOneToOne = false;

		m_pFPSViewAngles->setMode( m_LastViewMode );
		m_bIsIn1to1Mode = false;
		m_bIs1to1ModeLocked = false;

		// Stop glowing
		if ( m_nGlowIndex != -1 )
		{
			g_GlowObjectManager.UnregisterGlowObject( m_nGlowIndex );
			m_nGlowIndex = -1;
		}

	}

	m_pFPSViewAngles->update( &m_pACD->controllers[m_nLeftIndex], &m_pACD->controllers[m_nRightIndex] );

}

bool SixenseInput::IsHoldingObject()
{

	if( GetHeldObject() ) 
	{
		return true;
	}
	else
	{
		return false;
	}

}

C_BaseEntity *SixenseInput::GetHeldObject()
{
	C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	if ( pLocalPlayer )
	{
		return GetPlayerHeldEntity( pLocalPlayer );
	}
	return NULL;
}

bool SixenseInput::IsInOneToOneMode()
{
	// We're never in one to one if sixense features aren't turned on
	if ( sixense_features_enabled.GetInt() == 0 )
	{
		return false;
	}

	// We're in 1 to one mode if the object is being held and the view is locked.
	return m_bIsIn1to1Mode;

}

bool SixenseInput::IsInAlwaysOneToOneMode()
{
	// We're never in one to one if sixense features aren't turned on
	if ( sixense_features_enabled.GetInt() == 0 )
	{
		return false;
	}

	return ( C_BasePlayer::GetLocalPlayer()->GetSixenseFlags() & CBasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT_ALWAYS_ONE_TO_ONE ) ? true : false;
}
#endif

void SixenseInput::BlendView()
{
	// blend in view
	sixenseUtils::IFPSViewAngles::fps_mode cur_mode = m_pFPSViewAngles->getMode();
	m_pFPSViewAngles->setMode( sixenseUtils::IFPSViewAngles::DUAL_ANALOG );
	m_pFPSViewAngles->setMode( cur_mode );
}

// We don't seem to have an event that is fired on map load or level change
void SixenseInput::PlayerSpawn()
{
#ifdef PORTAL2
	// Hide any hints that were there from a previous level if left on
	hide_video_hint();
#endif

	// Reset the sensitiviy settings
	LoadDefaultSettings( sixense_sensitivity_level.GetInt() );

	m_nGesturesDisabled = 0;
	m_nFreeaimSpinDisabled = 0;

#ifdef PORTAL2
	sixense_disable_scale_reset_lesson.Revert();
	sixense_disable_ratchet_lesson.Revert();
	sixense_enable_tutorial_ratchet_lesson.Revert();
#endif

	m_bPlayerValid = true;
}

// Turns sixense support on and off. Note this is different than m_bIsActive, which gets
// set when the controllers are not in the dock.
void SixenseInput::SetEnabled( bool bEnabled )
{
	if ( !m_bIsEnabled && bEnabled )
	{
		// Just turned on...

		// Make sure the modules are either loaded or loaded previously
		if( !LoadModules() ) 
		{
			// Modules failed to load, disable
			sixense_enabled.SetValue( 0 );
			m_bIsEnabled = false;
			return;
		}

	}
	else if ( m_bIsEnabled && !bEnabled )
	{
		// Just turned off...

		if ( m_bPlayerValid )
		{
			C_BasePlayer * pPlayer = C_BasePlayer::GetLocalPlayer();

			if ( pPlayer )
			{
				// We are just	switching off...
				QAngle ident;
				ident.Init();
				pPlayer->SetEyeAngleOffset( ident ); // This is the way the player is looking, and the orientation of the weapon model
			}
		}

		// UnloadModules(); // fix crash on unload in sixense_utils
	}

#ifdef PORTAL2
	if ( m_bPlayerValid )
	{
		C_BasePlayer * pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer )
		{
			GetGameInstructor().ForceStopAllLessons();
		}
	}
#endif

	m_bIsEnabled = bEnabled;
}

void SixenseInput::ResetView( QAngle SpawnAngles )
{
	m_pFPSViewAngles->reset();

	Vector3 ss_spawn_angles( SpawnAngles[YAW], SpawnAngles[PITCH], SpawnAngles[ROLL] );

	m_pFPSViewAngles->forceViewAngles( m_pFPSViewAngles->getMode(), ss_spawn_angles );

}


void SixenseInput::Init()
{

	m_pSixenseAPI->sixenseInit();

	if( !m_pACD  )
	{
		m_pACD = new sixenseAllControllerData;
	}

	// init the sixense controller manager
	m_pControllerManager->setGameType( sixenseUtils::IControllerManager::ONE_PLAYER_TWO_CONTROLLER );
	m_pControllerManager->registerSetupCallback( controller_manager_setup_callback );

#ifdef PORTAL2
	m_pFPSViewAngles->setGame( "portal" );
	m_pFPSEvents->setGame( "portal" );
#else 
	m_pFPSViewAngles->setGame( "cstrike15" );
	m_pFPSEvents->setGame( "cstrike15" );
#endif

}

void SixenseInput::PostInit()
{
#ifdef PORTAL2
	ListenForGameEvent( "sixense_player_teleported" );
	ListenForGameEvent( "player_drop" );
	ListenForGameEvent( "player_use" );
#endif
	ListenForGameEvent( "player_spawn" );

	if( sixense_sensitivity_level.GetInt() == -1 ) 
	{
		LoadDefaultSettings( 2 );
	}

	engine->ExecuteClientCmd( "exec sixense_bindings.cfg" );

	if( m_pGestureBindings->GetNumBindings() == 0 )
	{
		// Try to create the default sixense bindings file if it doesn't already exist\n");
		m_pGestureBindings->CreateDefaultBindings();
		m_pGestureBindings->WriteBindings( "" );
	}

}

void SixenseInput::Shutdown()
{

	// Clear out pointers fetched from factories
	m_pFPSViewAngles = NULL;
	m_pFPSPlayerMovement = NULL;
	m_pFPSEvents = NULL;

	m_pLaserPointer = NULL;

	m_pLeftDeriv = NULL;
	m_pRightDeriv = NULL;

	m_pLeftButtonStates = NULL;
	m_pRightButtonStates = NULL;

	m_pControllerManager = NULL;

	if( m_pACD )
	{
		delete m_pACD;
	}


	if ( m_SixenseFrame )
	{
		if ( !m_SixenseFrame->IsAutoDeleteSet() )
		{
			m_SixenseFrame->SetParent( (vgui::Panel*)NULL );
			delete m_SixenseFrame;
			m_SixenseFrame = NULL;
		}

	}

	if( m_pGestureBindings )
	{
		delete m_pGestureBindings;
		m_pGestureBindings = NULL;
	}

	if( m_pSixenseAPI ) 
	{
		m_pSixenseAPI->sixenseExit();
	}

}

bool SixenseInput::IsEnabled()
{
	return m_bIsEnabled && m_bIsActive;
}

bool SixenseInput::IsLeftHanded()
{
	return sixense_left_handed.GetInt()==0?false:true;
}

void SixenseInput::SetBaseOffset()
{
	m_bShouldSetBaseOffset = true;
}

void SixenseInput::GetFOV( float *hfov, float *vfov )
{

#if ( defined( HL2_CLIENT_DLL ) || defined( TF_CLIENT_DLL ) || defined( CSTRIKE_DLL ) ) && !defined( CSTRIKE15 ) && !defined( TERROR )
	float engineAspectRatio = engine->GetScreenAspectRatio();
#else
	// avoid GetLocalPlayer() assert...
	if( !engine->IsLocalPlayerResolvable() ) {
		// defaults?
		*hfov = 90.0f;
		*vfov = 50.0f;
		return;
	}

	float engineAspectRatio = engine->GetScreenAspectRatio( ScreenWidth(), ScreenHeight() );
#endif

	C_BasePlayer * pPlayer = C_BasePlayer::GetLocalPlayer();
	if( pPlayer ) 
	{
		*hfov = pPlayer->GetFOV();
		*vfov = *hfov / engineAspectRatio;
	}
	else
	{
		// defaults?
		*hfov = 90.0f;
		*vfov = 50.0f;
	}

}


void SixenseInput::SetMode( int nNewMode )
{

	// The command id's don't match with the enum, so map it here.
	sixenseUtils::IFPSViewAngles::fps_mode mode = sixenseUtils::IFPSViewAngles::FREE_AIM_TWO_CONTROLLER;

	switch ( nNewMode )
	{
	case 0:
		mode = sixenseUtils::IFPSViewAngles::FREE_AIM_TWO_CONTROLLER;
		break;
	case 1:
		mode = sixenseUtils::IFPSViewAngles::MOUSELOOK;
		break;
	case 2:
		mode = sixenseUtils::IFPSViewAngles::DUAL_ANALOG;
		break;
	} 

	if ( m_pFPSViewAngles && (m_pFPSViewAngles->getMode() != mode) )
	{

		m_pFPSViewAngles->setMode( mode );

		m_LastViewMode = m_pFPSViewAngles->getMode();

	}

}

bool SixenseInput::InMenuMode()
{

#if defined( CSTRIKE_DLL ) && !defined( TERROR )

	bool cstrike_panel_visible = false;

#if defined( CSTRIKE15 ) // csgo
	const int num_panels = 16;
	char *panel_names[] = {
		PANEL_OVERVIEW,		
		PANEL_CLASS,		
		PANEL_TEAM,			
		PANEL_SPECMENU,		
		PANEL_INFO,		
		PANEL_BUY,			
		PANEL_BUY_CT,		
		PANEL_BUY_TER,		
		PANEL_BUY_EQUIP_CT,	
		PANEL_BUY_EQUIP_TER,	
		PANEL_NAV_PROGRESS,
		PANEL_BUYPRESET_MAIN,	
		PANEL_BUYPRESET_EDIT,	
		PANEL_INTRO,		
		PANEL_COMMENTARY_MODELVIEWER,	
		PANEL_SURVEY				
	};
#else // css
	const int num_panels = 15;
	char *panel_names[] = {
		PANEL_OVERVIEW,		
		PANEL_CLASS,		
		PANEL_TEAM,			
		PANEL_SPECMENU,		
		PANEL_INFO,		
		PANEL_BUY,			
		PANEL_BUY_CT,		
		PANEL_BUY_TER,		
		PANEL_BUY_EQUIP_CT,	
		PANEL_BUY_EQUIP_TER,	
		PANEL_NAV_PROGRESS,
		PANEL_BUYPRESET_MAIN,	
		PANEL_BUYPRESET_EDIT,	
		PANEL_INTRO,		
		PANEL_COMMENTARY_MODELVIEWER
	};
#endif

	for( int i=0; i<num_panels; i++ ) 
	{
#ifdef CSTRIKE15
		IViewPortPanel *panel =  GetViewPortInterface()->FindPanelByName( panel_names[i] );
#else
		IViewPortPanel *panel =  gViewPortInterface->FindPanelByName( panel_names[i] );
#endif
		if( panel ) 
		{
			if( panel->IsVisible() )
			{
				cstrike_panel_visible = true;
				//Msg("Menu visible %s\n", panel_names[i] );
			}
		}

	}

#endif

#if defined( TF_CLIENT_DLL )
	CTFPlayer *pTFPlayer = dynamic_cast<CTFPlayer *>(C_BasePlayer::GetLocalPlayer());
	if( pTFPlayer && pTFPlayer->m_Shared.GetState() == TF_STATE_DYING )
	{
		return true;
	}

#endif



	if( 
#ifdef PORTAL2
		( pPlayer && pPlayer->IsTaunting() ) || 
		( engine->IsLocalPlayerResolvable() && IsRadialMenuOpen() ) || 
#endif

#if defined( CSTRIKE_DLL ) && !defined( TERROR )
#ifdef CSTRIKE15
		BasePanel()->IsScaleformPauseMenuVisible() ||
#endif
		cstrike_panel_visible ||
#endif
		(SixenseInput::m_SixenseFrame && SixenseInput::m_SixenseFrame->IsVisible() ) || 
		engine->IsPaused() || 
		( enginevgui && enginevgui->IsGameUIVisible() ) || 
		vgui::surface()->IsCursorVisible() ||
		( m_pControllerManager && m_pControllerManager->isMenuVisible() ) )
	{
		return true;
	}

	return false;

}

bool SixenseInput::SixenseFrame( float flFrametime, CUserCmd *pCmd )
{

	if ( sixense_enabled.GetInt() && !m_bIsEnabled )
	{
		SetEnabled( true );
	}
	else if ( !sixense_enabled.GetInt() && m_bIsEnabled )
	{
		SetEnabled( false );
	}

#ifdef SIXENSE_PLAYER_DATA
	C_BasePlayer * pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	// If sixense is disabled, hide the controller manager screens and return
	if ( pLocalPlayer )
	{
		if ( !m_bIsEnabled )
		{

			pCmd->sixense_flags &= ~CBasePlayer::PLAYER_SIXENSE_ENABLED;

			pLocalPlayer->SetSixenseFlags( pLocalPlayer->GetSixenseFlags() & ~CBasePlayer::PLAYER_SIXENSE_ENABLED );
			return false;
		}
		else
		{

			pCmd->sixense_flags |= CBasePlayer::PLAYER_SIXENSE_ENABLED;
			pLocalPlayer->SetSixenseFlags( pLocalPlayer->GetSixenseFlags() | CBasePlayer::PLAYER_SIXENSE_ENABLED );
		}
	}
#endif

	// If sixense isn't enabled just return
	if( !m_bIsEnabled )
	{
		return false;
	}

	if( m_bConvarChanged )
	{
		m_bConvarChanged = false;

		UpdateValuesFromConvars();
	}
	
	m_pSixenseAPI->sixenseGetAllNewestData( m_pACD );

	if( m_pACD->controllers[0].enabled &&  m_pACD->controllers[1].enabled )
	{

		// Disable sixense when both controllers are docked.
		bool controllers_docked = m_pACD->controllers[0].is_docked || m_pACD->controllers[1].is_docked || !m_pSixenseAPI->sixenseIsBaseConnected(0);

		if( controllers_docked && m_bIsActive ) {
			m_bIsActive = false;

			QAngle engine_angles;
			engine->GetViewAngles( engine_angles );

			QAngle new_engine_angles = engine_angles + GetViewAngleOffset();

			engine->SetViewAngles( new_engine_angles );

		} else if( !controllers_docked && !m_bIsActive ) {
			m_bIsActive = true;

			// Reset the view next time through
			m_bJustSpawned = true;

			// If we're unpausing, flip the mode around so the metroid rotation is blended
			sixenseUtils::IFPSViewAngles::fps_mode cur_mode = m_pFPSViewAngles->getMode();
			m_pFPSViewAngles->setMode( sixenseUtils::IFPSViewAngles::DUAL_ANALOG );
			m_pFPSViewAngles->setMode( cur_mode );

		}
	}

	SixenseUpdateControllerManager();

	if( !sixense_left_handed.GetInt() ) {
		m_nLeftIndex = m_pControllerManager->getIndex( sixenseUtils::IControllerManager::P1L );
		m_nRightIndex = m_pControllerManager->getIndex( sixenseUtils::IControllerManager::P1R );
	} else {
		m_nLeftIndex = m_pControllerManager->getIndex( sixenseUtils::IControllerManager::P1R );
		m_nRightIndex = m_pControllerManager->getIndex( sixenseUtils::IControllerManager::P1L );
	}

	if( m_nLeftIndex < 0 || m_nRightIndex < 0 ) return false;

	// If the controllers are docked we are inactive, return until they are picked up
	if( !m_bIsActive )
		return false;

	m_pLeftButtonStates->update( &m_pACD->controllers[m_nLeftIndex] );
	m_pRightButtonStates->update( &m_pACD->controllers[m_nRightIndex] );

	m_pGestureBindings->UpdateBindings( m_pLeftButtonStates, m_pRightButtonStates, AreBindingsDisabled() );

	SixenseUpdateKeys( flFrametime, pCmd ); 

	SixenseUpdateMouseCursor();

#ifdef SIXENSE_PLAYER_DATA
	if ( !engine->IsPaused() && !( enginevgui && enginevgui->IsGameUIVisible() ) && sixense_features_enabled.GetInt() )
		SetPlayerHandPositions( pCmd, flFrametime );
#endif


	static unsigned char last_seq = 0;

	// Same data as last time, just return
	if ( m_pACD->controllers[0].sequence_number == last_seq )
	{
		return false;
	}

#ifdef PORTAL2
	C_Portal_Player *pPlayer = GetPortalPlayer();
#endif

	// If the controller manager is up or the game is paused, don't do anything.
	if ( InMenuMode() )
	{
		m_bWasInMenuMode = true;
		return false;
	}

	// If the menu just hid, blend the view back in
	if( m_bWasInMenuMode )
	{
		m_bWasInMenuMode = false;

		BlendView();
	}

#ifdef PORTAL2
	// Fail if the video hint is up. Trigger should skip, probably.
	if (_video_hint_panel && _video_hint_panel->IsVisible())
	{
		return false;
	}
#endif

	last_seq = m_pACD->controllers[0].sequence_number;

	float freeAimSpinSpeed = sixense_aim_freeaim_max_speed.GetFloat();

	if ( m_nFreeaimSpinDisabled > 0 )
	{
		float fCurrentSpeed = m_pFPSViewAngles->getParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_MAX_SPEED );
		freeAimSpinSpeed = fCurrentSpeed *= 0.95;
	}
	else
	{
#ifdef PORTAL2
		CPropWeightedCube* pScaledCube = UTIL_GetAsScaledCube( GetHeldObject() );
		if ( pScaledCube )
		{
			Vector vecScale = pScaledCube->GetScale();

			float fMaxScale = vecScale[0];
			if( vecScale[1] > fMaxScale ) fMaxScale = vecScale[1];
			if( vecScale[2] > fMaxScale ) fMaxScale = vecScale[2];

			float fSpinMult = 1.0 - sqrt( clamp( ( fMaxScale-1.0f ) / ( sixense_scaling_max.GetFloat() - 1.0f ), 0.0f, 1.0f ) );
			freeAimSpinSpeed = MAX( freeAimSpinSpeed * fSpinMult, sixense_hold_spin_speed.GetFloat() );
		}
#endif
	}

	static int last_mode = 0;

	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_MAX_SPEED, freeAimSpinSpeed );//sixense_aim_freeaim_max_speed.GetFloat() );

	if ( m_fTeleportWaitToBlendTime - gpGlobals->curtime <= 0.0f )
	{
		// teleport delay complete, blend in the view
		if ( m_fTeleportWaitToBlendTime > 0.0f )
		{
			BlendView();
			m_fTeleportWaitToBlendTime = 0.0f;
		} 
		else
		{
			float fBlendTime = clamp( freeAimSpinSpeed * 0.16f * sixense_aim_freeaim_switch_blend_time_enter.GetFloat(), 0.1f, 2.0f);
			m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_SWITCH_BLEND_TIME_ENTER, fBlendTime );
		}
	}
	else
	{
		m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_MAX_SPEED, 0.0f );
	}

	m_pFPSViewAngles->setParameter( sixenseUtils::IFPSViewAngles::AIM_METROID_MAX_SPEED, freeAimSpinSpeed );//sixense_aim_freeaim_max_speed.GetFloat() );

	// Keep the fov up to date
	float hfov, vfov;
	SixenseInput::GetFOV( &hfov, &vfov );
	m_pFPSViewAngles->setFov( hfov, vfov );


	m_pLeftDeriv->update( &m_pACD->controllers[m_nLeftIndex] );
	m_pRightDeriv->update( &m_pACD->controllers[m_nRightIndex] );

	// Update returns false if there aren't enough controllers to play, so disable all the sixense stuff
	if ( m_pFPSViewAngles->update( &m_pACD->controllers[m_nLeftIndex], &m_pACD->controllers[m_nRightIndex], flFrametime*1000.0f ) == SIXENSE_FAILURE ||
		m_pControllerManager->isMenuVisible() ||
		vgui::surface()->IsCursorVisible() )
	{

		return false;
	}


	static float filtered_frametime = 0.0f;
	const float frametime_filt_param = 0.99f;
	filtered_frametime = filtered_frametime * frametime_filt_param + flFrametime * 1000.0f * ( 1.0f - frametime_filt_param );


	m_pFPSPlayerMovement->update( &m_pACD->controllers[m_nLeftIndex], &m_pACD->controllers[m_nRightIndex], filtered_frametime );
	m_pFPSEvents->update( &m_pACD->controllers[m_nLeftIndex], &m_pACD->controllers[m_nRightIndex], filtered_frametime );


	CheckWeaponForScope();

	return true;
}

void SixenseInput::CheckWeaponForScope()
{

	bool zoomed = false;

#if defined( TERROR ) || defined (CSTRIKE15) || defined (CSTRIKE_DLL)
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && (pPlayer->GetFOV() != pPlayer->GetDefaultFOV()) )
	{
		zoomed = true;
	}
#endif

#if defined( HL2_CLIENT_DLL )
	C_BaseHLPlayer *hlPlayer = dynamic_cast<C_BaseHLPlayer *>(C_BasePlayer::GetLocalPlayer());

	if ( hlPlayer && hlPlayer->m_HL2Local.m_bZooming )
	{
		zoomed = true;
	}
#endif

#if defined( TF_CLIENT_DLL)
	CTFPlayer *ctfPlayer = dynamic_cast<CTFPlayer *>(C_BasePlayer::GetLocalPlayer());

	if ( ctfPlayer && ctfPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		zoomed = true;
	}
#endif

#if !defined( HL2_CLIENT_DLL ) && !defined( CSTRIKE_DLL ) && !defined( TF_CLIENT_DLL)
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if( pPlayer )
	{

		C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();

		if( pWeapon )
		{

			CWeaponCSBase *pCSWeapon = dynamic_cast< CWeaponCSBase * >( pWeapon );

			if( pCSWeapon )
			{
				const float min_fov = 45.0f;

				if ( pCSWeapon->HasScope() && (pPlayer->GetFOV() < min_fov) )
				{
					zoomed = true;
				}
			}
		}
	}
#endif

	if( zoomed && !m_bScopeSwitchedMode )
	{
		// we have a cs weapon that has a scope and we are zoomed.
		// Remember the state of some stuff we set
		m_nScopeSwitchedPrevMode = m_pFPSViewAngles->getMode();
		m_nScopeSwitchedPrevSpringViewEnabled = sixense_spring_view_enabled.GetInt();

		m_bScopeSwitchedMode = true;

		m_pFPSViewAngles->setMode( sixenseUtils::IFPSViewAngles::MOUSELOOK );

	}
	else if( !zoomed && m_bScopeSwitchedMode )
	{

#if defined( TF_CLIENT_DLL)

		// In TF2 wait until we're done reloading before switching back to metroid mode
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

		if( pPlayer )
		{
			CTFSniperRifle *tfSniperRifle = dynamic_cast<CTFSniperRifle *>(pPlayer->GetActiveWeapon());

			if( tfSniperRifle && (tfSniperRifle->GetRezoomTime() != -1.0f) )
			{
				return;
			}
		}
#endif

#if defined( CSTRIKE15 ) || defined (CSTRIKE_DLL)
		C_CSPlayer *csPlayer = dynamic_cast<C_CSPlayer *>(C_BasePlayer::GetLocalPlayer());

		if( csPlayer && csPlayer->m_bResumeZoom )
		{
			return;
		}
#endif


		// not zoomed anymore, put the old mode back
		if( m_bScopeSwitchedMode ) {

			m_bScopeSwitchedMode = false;

			sixense_spring_view_enabled.SetValue( m_nScopeSwitchedPrevSpringViewEnabled );

			m_pFPSViewAngles->setMode( m_nScopeSwitchedPrevMode );

		}

	}

}

void SixenseInput::SwitchViewModes( CUserCmd *pCmd )
{


#ifdef PORTAL2
	if( sixense_large_objects_lock_one_to_one.GetInt() ) 
	{
		C_BaseEntity *held = GetHeldObject();
		if( held ) 
		{
			C_PropWeightedCube *scaled_cube = UTIL_GetAsScaledCube( held );

			if( scaled_cube )
			{
				Vector scale = scaled_cube->GetScale();

				float max_scale = scale[0];
				if( scale[1] > max_scale ) max_scale = scale[1];
				if( scale[2] > max_scale ) max_scale = scale[2];

				const float scale_force_1to1 = sixense_large_objects_lock_one_to_one_size.GetFloat();

				if( max_scale > scale_force_1to1 ) 
				{
					// Lock one to one mode on
					m_bScalingLockedOneToOne = true;

					if( !m_bIsIn1to1Mode ) 
					{
						SetOneToOneMode( true );
					}
				} else {
					m_bScalingLockedOneToOne = false;

				}
			}
		}
	}



	unsigned char lock_button;

	lock_button = SIXENSE_BUTTON_1;
	//lock_button = SIXENSE_BUTTON_4;

	if ( sixense_auto_one_to_one_enabled.GetInt() )
	{
		lock_button = 0;
	}

	C_Portal_Player *pPortalPlayer = C_Portal_Player::GetLocalPortalPlayer();


	// Whenever we scale, lock into one to one mode until the next drop (block dist one to one from exiting)
	//if( pPortalPlayer->IsScalingUseItem() ) {
	//	m_bScalingLockedOneToOne = true;
	//}


	// This was old code for controlling 1to1 with a button, but this code was getting hit on drop.
	// Go into 1-to-1 when the button is pressed (and we're holding an object, and we're not scaling)
	//if ( !m_bIs1to1ModeLocked &&
	//        !m_bIs1to1ModeScaling &&
	//        IsHoldingObject() &&
	//        ( lock_button && m_pRightButtonStates->justPressed( lock_button ) ) &&
	//		( pPortalPlayer && !pPortalPlayer->IsScalingUseItem() && !pPortalPlayer->IsScalingUseItemTurret() ) )
	//{
	//	SetOneToOneMode( true );
	//}
	// exit 1-to-1 if we dropped the object we were holding, or if the lock button was pressed again
	if ( ( !IsHoldingObject() && m_bIsIn1to1Mode && !m_bExitOneWhenAimingForwards ) ||
		( m_bIs1to1ModeLocked &&
		!m_bIs1to1ModeScaling &&
		m_pRightButtonStates->justPressed( lock_button ) &&
		( pPortalPlayer && !pPortalPlayer->IsScalingUseItem() && !pPortalPlayer->IsScalingUseItemTurret() ) ) )
	{
		m_bExitOneWhenAimingForwards = true;
	}

	// Object was dropped while in 1-to-1, don't leave one-to-1 mode until the hand comes back a bit
	if( m_bExitOneWhenAimingForwards ) {

		if( IsAimingForwards() ) 
		{
			m_bIs1to1ModeScaling = false;
			m_bIs1to1ModeRatcheting = false;
			m_pFPSViewAngles->setRatcheting( false );
			SetOneToOneMode( false );

			m_bExitOneWhenAimingForwards = false;

		}

	}

	// if we just started scaling go into 1-to-1
	else if ( !m_bIs1to1ModeLocked &&
		!m_bIs1to1ModeScaling &&
		IsHoldingObject() &&
		( pPortalPlayer && ( pPortalPlayer->IsScalingUseItem() || pPortalPlayer->IsScalingUseItemTurret() ) ) )
	{
		m_bIs1to1ModeScaling = true;
		SetOneToOneMode( true );

	}

	// if we just stopped scaling exit 1-to-1
	else if (
		( !IsHoldingObject() && m_bIsIn1to1Mode ) ||
		( m_bIs1to1ModeLocked &&
		m_bIs1to1ModeScaling &&
		( pPortalPlayer && !pPortalPlayer->IsScalingUseItem() && !pPortalPlayer->IsScalingUseItemTurret() ) ) )
	{
		m_bIs1to1ModeScaling = false;
		m_bIs1to1ModeRatcheting = false;
		m_pFPSViewAngles->setRatcheting( false );

		if( !m_bScalingLockedOneToOne ) {
			SetOneToOneMode( false );
		}
	}
#endif
}

bool SixenseInput::IsAimingForwards()
{
	// Wait until controller is facing forwards again
	Quat rot_quat( m_pACD->controllers[m_nRightIndex].rot_quat );
	Vector3 forwards = rot_quat * Vector3( 0, 0, -1 );

	float dot = forwards * Vector3(0, 0, -1);

	if( dot > sixense_exit_one_to_one_dot.GetFloat() ) {
		return true;
	} 
	else 
	{
		return false;
	}
}

#ifdef SIXENSE_PLAYER_DATA
void SixenseInput::SetPlayerHandPositions( CUserCmd *pCmd, float flFrametime )
{
	if ( m_bShouldSetBaseOffset )
	{
		m_bShouldSetBaseOffset = false;

		float base_offset_z_bias = sixense_auto_one_to_one_center_z_bias.GetFloat();

		if ( sixense_dist_one_to_one_enabled.GetInt() )
		{
			base_offset_z_bias = 0.0f;
		}

		sixense_base_offset_x.SetValue( - m_pACD->controllers[m_nRightIndex].pos[0] );
		sixense_base_offset_y.SetValue( - m_pACD->controllers[m_nRightIndex].pos[1] );
		sixense_base_offset_z.SetValue( - m_pACD->controllers[m_nRightIndex].pos[2] - base_offset_z_bias );
	}

	sixenseMath::Vector3 pos;
	sixenseMath::Vector3 base_offset( sixense_base_offset_x.GetFloat(), sixense_base_offset_y.GetFloat(), sixense_base_offset_z.GetFloat() );


	// Tell the client player about the hand positions. This will get sent to the server player as part of the usercmd
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();


	// Maintain the player and command one-to-one flags
	if ( m_bIsIn1to1Mode && IsHoldingObject() )
	{
		pCmd->sixense_flags |= C_BasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT;
		player->SetSixenseFlags( player->GetSixenseFlags() | CBasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT );

		if ( m_bIs1to1ModeRatcheting )
		{
			pCmd->sixense_flags |= C_BasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT_RATCHETING;
			player->SetSixenseFlags( player->GetSixenseFlags() | CBasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT_RATCHETING );
		}
		else
		{
			pCmd->sixense_flags &= ~C_BasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT_RATCHETING;
			player->SetSixenseFlags( player->GetSixenseFlags() & ~CBasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT_RATCHETING );
		}

	}
	else
	{
		pCmd->sixense_flags &= ~C_BasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT;
		pCmd->sixense_flags &= ~C_BasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT_RATCHETING;
		player->SetSixenseFlags( player->GetSixenseFlags() & ~CBasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT );
		player->SetSixenseFlags( player->GetSixenseFlags() & ~CBasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT_RATCHETING );
	}

	if ( sixense_dist_one_to_one_enabled.GetInt() )
	{
		pCmd->sixense_flags |= C_BasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT_ALWAYS_ONE_TO_ONE;
		player->SetSixenseFlags( player->GetSixenseFlags() | CBasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT_ALWAYS_ONE_TO_ONE );
	}
	else
	{
		pCmd->sixense_flags &= ~C_BasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT_ALWAYS_ONE_TO_ONE;
		player->SetSixenseFlags( player->GetSixenseFlags() & ~CBasePlayer::PLAYER_SIXENSE_HOLDING_OBJECT_ALWAYS_ONE_TO_ONE );
	}





	// Get the angle mode from the fps angles so we can remove it from the hand orientations
	float angle_mode_angle = m_pFPSViewAngles->getParameter( sixenseUtils::IFPSViewAngles::CONTROLLER_ANGLE_MODE );


	unsigned char left_sequence, right_sequence;
	left_sequence = m_pACD->controllers[m_nLeftIndex].sequence_number;
	right_sequence = m_pACD->controllers[m_nRightIndex].sequence_number;

	sixenseMath::Matrix4 left_mat;

	// Set rotation first
	left_mat = sixenseMath::Matrix4::rotation( sixenseMath::Quat( m_pACD->controllers[m_nLeftIndex].rot_quat[0], m_pACD->controllers[m_nLeftIndex].rot_quat[1], m_pACD->controllers[m_nLeftIndex].rot_quat[2], m_pACD->controllers[m_nLeftIndex].rot_quat[3] ) );

	// Correct for controller angle mode
	left_mat = sixenseMath::Matrix4::rotation( -angle_mode_angle, sixenseMath::Vector3( 1.0f, 0.0f, 0.0f ) ) * left_mat;

	// Fill in translation
	Vector3 ss_left_pos = sixenseMath::Vector3( m_pACD->controllers[m_nLeftIndex].pos[0], m_pACD->controllers[m_nLeftIndex].pos[1], m_pACD->controllers[m_nLeftIndex].pos[2] ) + base_offset;



	left_mat.set_col( 3, sixenseMath::Vector4( ss_left_pos, 1.0f ) );

	// This converts between sixense and source coord sys
	matrix3x4_t left_mat_source = ConvertMatrix( left_mat );



	player->SetSixenseSequence( m_pACD->controllers[0].sequence_number );


	// Set the translations
	Vector left_pos;
	MatrixPosition( left_mat_source, left_pos );

	pCmd->hand_pos_left = left_pos;
	player->SetHandPosLeft( left_pos );



	// Rotations
	QAngle left_rot;

	MatrixAngles( left_mat_source, left_rot );

	pCmd->hand_rot_left = left_rot;
	player->SetHandRotLeft( left_rot );

	if ( left_sequence != m_nLastLeftSequence )
	{
		pCmd->left_hand_pos_new = 1;
		player->SetLeftHandDataNew( true );
	}
	else
	{
		pCmd->left_hand_pos_new = 0;
		player->SetLeftHandDataNew( false );
	}

	m_nLastLeftSequence = left_sequence;

	pCmd->sixense_seq = left_sequence;


	sixenseMath::Matrix4 right_mat;

	// Set rotation first
	right_mat = sixenseMath::Matrix4::rotation( sixenseMath::Quat( m_pACD->controllers[m_nRightIndex].rot_quat[0], m_pACD->controllers[m_nRightIndex].rot_quat[1], m_pACD->controllers[m_nRightIndex].rot_quat[2], m_pACD->controllers[m_nRightIndex].rot_quat[3] ) );

	// Correct for controller angle mode
	right_mat = sixenseMath::Matrix4::rotation( -angle_mode_angle, sixenseMath::Vector3( 1.0f, 0.0f, 0.0f ) ) * right_mat;

	// Fill in translation
	Vector3 ss_right_pos = sixenseMath::Vector3( m_pACD->controllers[m_nRightIndex].pos[0], m_pACD->controllers[m_nRightIndex].pos[1], m_pACD->controllers[m_nRightIndex].pos[2] ) + base_offset;


	// This 'slides' the hold origin if you pull the object back into the player
	float min_z_dist = sixense_hold_slide_z_min_dist.GetFloat();
	float xy_radius = sixense_hold_slide_xy_radius.GetFloat();

	if ( !m_bScalingLockedOneToOne && (Vector3( ss_right_pos[0], ss_right_pos[1], 0.0f ).length() < xy_radius) && (ss_right_pos[2] > min_z_dist) )
	{
		sixense_base_offset_z.SetValue( sixense_base_offset_z.GetFloat() - ( ss_right_pos[2] - min_z_dist ) );

		// Also adjust the position at which the object was grabbed so that the dist one to one slides also
		m_GrabPos[2] += ( ss_right_pos[2] - min_z_dist );

		ss_right_pos[2] = min_z_dist;
	}

	right_mat.set_col( 3, sixenseMath::Vector4( ss_right_pos, 1.0f ) );

	// This converts between sixense and source coord sys
	matrix3x4_t right_mat_source = ConvertMatrix( right_mat );



	// Set the translations
	Vector right_pos;
	MatrixPosition( right_mat_source, right_pos );

	pCmd->hand_pos_right = right_pos;
	player->SetHandPosRight( right_pos );


	// Rotations
	QAngle right_rot;

	MatrixAngles( right_mat_source, right_rot );

	pCmd->hand_rot_right = right_rot;
	player->SetHandRotRight( right_rot );

	if ( right_sequence != m_nLastRightSequence )
	{
		pCmd->right_hand_pos_new = 1;
		player->SetRightHandDataNew( true );
	}
	else
	{
		player->SetRightHandDataNew( false );
		pCmd->right_hand_pos_new = 0;
	}

	m_nLastRightSequence = right_sequence;

	// set controller positions for object scaling
	pCmd->controller_pos_left.Init( ss_left_pos[0], ss_left_pos[1], ss_left_pos[2] );
	pCmd->controller_pos_right.Init( ss_right_pos[0], ss_right_pos[1], ss_right_pos[2] );
	player->SetControllerPosLeft( pCmd->controller_pos_left );
	player->SetControllerPosRight( pCmd->controller_pos_right );

	// set controller rotations for portal tweaking, fix roll so that it goes from (-180 to 180) instead of (-90 to 90)
	sixenseMath::Vector3 left_contoller_angles = left_mat.getEulerAngles();
	sixenseMath::Vector3 right_contoller_angles = right_mat.getEulerAngles();
	QAngle controller_rot_left = QAngle( RAD2DEG( left_contoller_angles[2] ), RAD2DEG( left_contoller_angles[0] ), RAD2DEG( left_contoller_angles[1] ) );
	QAngle controller_rot_right = QAngle( RAD2DEG( right_contoller_angles[2] ), RAD2DEG( right_contoller_angles[0] ), RAD2DEG( right_contoller_angles[1] ) );
	//	QAngle controller_rot_left = left_rot;
	//	QAngle controller_rot_right = right_rot;
	//	if( left_mat_source[2][2] < 0.0f ) {
	//		controller_rot_left.x = (controller_rot_left.x > 0.0f) ? (180.0f - controller_rot_left.x) : (-180.0f + controller_rot_left.x);
	//	}
	//	if( right_mat_source[2][2] < 0.0f ) {
	//		controller_rot_right.x = (controller_rot_right.x > 0.0f) ? (180.0f - controller_rot_right.x) : (-180.0f + controller_rot_right.x);
	//	}
	pCmd->controller_rot_left.Init( controller_rot_left.x, controller_rot_left.y, controller_rot_left.z );
	pCmd->controller_rot_right.Init( controller_rot_right.x, controller_rot_right.y, controller_rot_right.z );
	player->SetControllerRotLeft( controller_rot_left );
	player->SetControllerRotRight( controller_rot_right );

	// Set some sixense flags...
	if ( m_bIsIn1to1Mode && !m_nFreeaimSpinDisabled )
	{

		// pan the camera to keep the held object on screen
		C_BaseEntity *pHeldObject = GetHeldObject();

		if ( pHeldObject )
		{
			float hold_spin_start_screen_ratio = sixense_hold_spin_start_screen_ratio.GetFloat();
			float hold_spin_speed = sixense_hold_spin_speed.GetFloat();
			float hold_spin_speed_bias = sixense_hold_spin_speed_bias.GetFloat();
			float hold_spin_fade_min_dist = sixense_hold_spin_fade_min_dist.GetFloat();
			float hold_spin_fade_max_dist = sixense_hold_spin_fade_max_dist.GetFloat();
			float hold_spin_max_angle = sixense_hold_spin_max_angle.GetFloat();
			C_Portal_Player *pPlayer = GetPortalPlayer();

			// where is the held object in world coordinates
			Vector objectLocation = pHeldObject->GetAbsOrigin();

			// where the player is
			Vector playerLocation = pPlayer->GetAbsOrigin();

			float dist_from_player = ( objectLocation - playerLocation ).Length();

			// check to see if object is on other side of portal
			if ( pPlayer->IsHeldObjectOnOppositeSideOfPortal() )
			{
				objectLocation = pPlayer->GetGrabLocation();
			}

			// Don't pan if player is reaching behind, he's probably trying to wind up for a throw
			QAngle eyeAngles = pPlayer->EyeAngles();
			Vector vForward;
			AngleVectors( eyeAngles, &vForward );

			float horiz_speed_mult = 0.0f;
			float vert_speed_mult = 0.0f;

			float angle =  acos( ( objectLocation - pPlayer->EyePosition() ).Normalized().Dot( vForward ) ) * 180 / M_PI;

			//engine->Con_NPrintf( 1, "angle: %f\n", angle );
			if ( angle < hold_spin_max_angle )
			{
				// world to screen
				Vector screen;
				ScreenTransform( objectLocation, screen );
				screen.x = clamp( screen.x, -1.0f, 1.0f );
				screen.y = clamp( screen.y, -1.0f, 1.0f );

				// horizontal
				if ( abs( screen.x ) > 1.0 - hold_spin_start_screen_ratio )
				{
					horiz_speed_mult = ( ( abs( screen.x ) - ( 1.0 - hold_spin_start_screen_ratio ) ) / hold_spin_start_screen_ratio );
					horiz_speed_mult = Bias( horiz_speed_mult, hold_spin_speed_bias );

					if ( screen.x > 0 ) horiz_speed_mult *= -1;
				}

				// vertical
				if ( abs( screen.y ) > 1.0 - hold_spin_start_screen_ratio )
				{
					vert_speed_mult = ( ( abs( screen.y ) - ( 1.0 - hold_spin_start_screen_ratio ) ) / hold_spin_start_screen_ratio );
					vert_speed_mult = Bias( vert_speed_mult, hold_spin_speed_bias );

					if ( screen.y > 0 ) vert_speed_mult *= -1;
				}

				//engine->Con_NPrintf( 1, "x: %f  y:%f\n", screen.x, screen.y );
				//engine->Con_NPrintf( 2, "horiz_speed: %f\n", horiz_speed);
				//engine->Con_NPrintf( 3, "vert_speed: %f\n", vert_speed);

				if ( hold_spin_fade_max_dist != 0.0f && hold_spin_fade_max_dist != 0.0f )
				{

					float spin_fade = ( dist_from_player - hold_spin_fade_min_dist ) / ( hold_spin_fade_max_dist - hold_spin_fade_min_dist );

					spin_fade = clamp( spin_fade, 0.0f, 1.0f );

					horiz_speed_mult *= spin_fade;
					vert_speed_mult *= spin_fade;
				}
			}

			// dampen the camera tracking
			float spin_damp = sixense_hold_spin_damp.GetFloat();
			horiz_speed_mult = horiz_speed_mult * ( 1.0f - spin_damp ) + m_fLastHorizSpeedMult * spin_damp;
			vert_speed_mult = vert_speed_mult * ( 1.0f - spin_damp ) + m_fLastVertSpeedMult * spin_damp;

			m_fLastHorizSpeedMult = horiz_speed_mult;
			m_fLastVertSpeedMult = vert_speed_mult;

			m_pFPSViewAngles->setHoldingTurnSpeed( horiz_speed_mult * hold_spin_speed, vert_speed_mult * hold_spin_speed );
		}
	}
	else
	{
		m_pFPSViewAngles->setHoldingTurnSpeed( 0.0f, 0.0f );
	}


	// Compute the angles that point from the player to the hand positions. This can be used to aim the weapon model at the object being moved.
	// The goofy offsets are because the gun doesn't rotate around the right position, so they fudge it so it looks like it's pointing correctly
	VectorAngles( right_pos + Vector( 250, 0, -25 ), m_AnglesToRightHand );
	VectorAngles( left_pos + Vector( 250, 0, -25 ), m_AnglesToLeftHand );
}
#endif

void SixenseInput::SixenseUpdateControllerManager()
{

	if( !m_bIsEnabled )
		return;

	m_pControllerManager->update( m_pACD );

#if 0
	update_controller_manager_visibility( &acd );
#endif

}


void SixenseInput::SetFilter( float f )
{
	if( f < 0.25f ) 
	{
		m_pSixenseAPI->sixenseSetFilterParams( 500.0f, 0.5f, 1600.0f, 0.75f );
		Msg("SixenseInput::SetFilter: low\n");
	}
	else if( f < 0.75f )
	{
		m_pSixenseAPI->sixenseSetFilterParams( 500.0f, 0.85f, 1600.0f, 0.95f );
		Msg("SixenseInput::SetFilter: med\n");
	}
	else 
	{
		m_pSixenseAPI->sixenseSetFilterParams( 500.0f, 0.96f, 1600.0f, 0.98f );
		Msg("SixenseInput::SetFilter: high\n");
	}

	sixense_filter_level.SetValue( f );
}


void SixenseInput::ForceViewAngles( QAngle angles )
{

	Vector3 ssangles( angles[YAW], angles[PITCH], angles[ROLL] );


	m_pFPSViewAngles->forceViewAngles( m_pFPSViewAngles->getMode(), ssangles );
}

static QAngle FixAngles( sixenseMath::Vector3 ss_angles )
{

	QAngle qa;
	qa[YAW] = ss_angles[0];
	qa[PITCH] = ss_angles[1];
	qa[ROLL] = ss_angles[2];

	return qa;

}

sixenseMath::Vector3 FixAngles( QAngle qa )
{

	sixenseMath::Vector3 ss_angles;
	ss_angles[0] = qa[YAW];
	ss_angles[1] = qa[PITCH];
	ss_angles[2] = qa[ROLL];

	return ss_angles;

}

#if defined( TF_CLIENT_DLL )
static float g_fDemoChargeViewOffsetScale = 1.0f;
#endif

void SixenseInput::SetView( float flInputSampleFrametime, CUserCmd *pCmd )
{

	if ( InMenuMode() )
	{
		return;
	}

	// Sample the axes, apply the input, and consume sample time.
	if ( m_fRemainingFrameTime > 0 )
	{
		flInputSampleFrametime = MIN( m_fRemainingFrameTime, flInputSampleFrametime );
		m_fRemainingFrameTime -= flInputSampleFrametime;
	}

	// Set the new engine angles. This is the direction the player is really aiming, and the only direction the
	// server really cares about.
	QAngle spin_speed = -1.0f * FixAngles( m_pFPSViewAngles->getSpinSpeed() );

	if( UseVR() )
	{
		spin_speed[PITCH] = 0.f;
		spin_speed[ROLL] = 0.f;
	}

#if defined( TF_CLIENT_DLL )

	static bool last_charge = false, charging = false;
	bool charge_started=false, charge_stopped=false;

	CTFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( pPlayer ) 
	{
		charging = pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE );
		if ( charging )
		{
			if( !last_charge )
			{
				charge_started = true;
			}
		}
		else
		{
			if( last_charge )
			{
				charge_stopped = true;
			}
		}
	}
	last_charge = charging;

	if( charging )
	{
		g_fDemoChargeViewOffsetScale *= 0.9f;
	}
	else
	{
		g_fDemoChargeViewOffsetScale = g_fDemoChargeViewOffsetScale * 0.9f + 0.1f;
	}

#endif

	// Keep track of the total accumulated spin angle. This accumulation is done outisde of sixenseUtils because we need to
	// keep a close look on the engine time to do proper time sync.
	QAngle accumulated_spin_angles;
	accumulated_spin_angles = FixAngles( m_pFPSViewAngles->getFeetAnglesMetroid() ) + ( spin_speed * flInputSampleFrametime * 30.0f );

	// Clamp it +/- 90
	if ( accumulated_spin_angles[PITCH] > 89.0f )
	{
		accumulated_spin_angles[PITCH] = 89.0f;
	}
	else if ( accumulated_spin_angles[PITCH] < -89.0f )
	{
		accumulated_spin_angles[PITCH] = -89.0f;
	}

	// Tell the view angle system about the total angle so GetViewAngleOffset can compute the player camera angle
	m_pFPSViewAngles->setFeetAnglesMetroid( FixAngles( accumulated_spin_angles ) );

	if( m_bJustSpawned ) 
	{
		m_bJustSpawned = false;

		QAngle engine_angles;
		engine->GetViewAngles( engine_angles );

		ResetView( engine_angles );

		return;
	}

	QAngle new_viewangles = FixAngles( m_pFPSViewAngles->getViewAngles() );


#if defined( TF_CLIENT_DLL )
	// Dont turn when charging
	if( !charging )
	{
		engine->SetViewAngles( new_viewangles );
	}

	if( charge_stopped )
	{

		QAngle engine_angles;
		engine->GetViewAngles( engine_angles );

		ForceViewAngles( engine_angles );
		Msg("charge stopped\n");
	}
#else
	// Set the engine's aim direction
	engine->SetViewAngles( new_viewangles );
#endif

	if ( pCmd )
	{
		pCmd->mousedx = spin_speed[YAW];
		pCmd->mousedy = spin_speed[PITCH];
	}

	// Also set the player direction, this is the view frustum direction, which in metroid mode counter-rotates
	// from the aim direction engine angles.
	C_BasePlayer::GetLocalPlayer()->SetEyeAngleOffset( GetViewAngleOffset() ); // This is the way the player is looking, and the orientation of the weapon model

	//C_BaseEntity *pEntOrg = C_BasePlayer::GetLocalPlayer()->GetViewEntity();

#ifdef SIXENSE_PLAYER_DATA
	// Set the eye offset angles in the ucmd so thay get sent to the server
	pCmd->view_angle_offset = GetViewAngleOffset();
#endif

}

void SixenseInput::SixenseUpdateKeys( float flFrametime, CUserCmd *pCmd )
{

	static int new_buttons = 0;

	// Sometimes we get called but Sixense isn't ready to produce any new data yet. We want to preseve whatever we were doing before,
	// ie walking, so keep track of the last state we used.
	static int last_buttons = 0;
	static float last_forwardmove = 0.0f;
	static float last_sidemove = 0.0f;
	static int last_sixense_flags = 0;

	// If the controller manager is up or the game is paused, don't do anything.
	if ( InMenuMode() ||
		(C_BasePlayer::GetLocalPlayer() && C_BasePlayer::GetLocalPlayer()->IsObserver()) ||
		( m_nLeftIndex == -1 ) ||
		( m_nRightIndex == -1 ) ||
		( m_nGesturesDisabled == 1 ) )
	{

		// No new data, set the keys to what they were last time we did have new data

#ifdef PORTAL2
		// Unduck here too so if gestures are disabled like in the tutorial we're not stuck ducking
		if ( m_pFPSEvents->eventStopped( sixenseUtils::IFPSEvents::CROUCH ) )
		{
			last_buttons &= ~IN_DUCK;
			new_buttons &= ~IN_DUCK;
			//engine->ClientCmd( "-duck" );
		}
#endif

		if( pCmd ) 
		{
			// Sometimes we get called but Sixense isn't ready to produce any new data yet. We want to preseve whatever we were doing before,
			// ie walking, so keep track of the last state we used.
			pCmd->buttons = last_buttons;
	#ifdef SIXENSE_PLAYER_DATA
			pCmd->sixense_flags = last_sixense_flags;
	#endif

			pCmd->mousedx = 0;
			pCmd->mousedy = 0;

			//pCmd->forwardmove = last_forwardmove;
			//pCmd->sidemove = last_sidemove;

			if( InMenuMode() )
			{
				// Force walking to stop when menus are up
				pCmd->forwardmove = 0.0f;
				pCmd->sidemove = 0.0f;
			}
		}


		// hard code some commands for when we're observing
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if( pPlayer && pPlayer->IsObserver() )
		{
			if( m_pLeftButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_RIGHT ) )
			{
				engine->ExecuteClientCmd( "spec_next" );
			}
			if( m_pLeftButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_LEFT ) )
			{
				engine->ExecuteClientCmd( "spec_prev" );
			}
			if( m_pLeftButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_UP ) )
			{
				engine->ExecuteClientCmd( "spec_mode" );
			}
			if( m_pLeftButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_DOWN ) )
			{
				engine->ExecuteClientCmd( "spec_mode" );
			}

			if( m_pRightButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_RIGHT ) )
			{
				engine->ExecuteClientCmd( "spec_next" );
			}
			if( m_pRightButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_LEFT ) )
			{
				engine->ExecuteClientCmd( "spec_prev" );
			}
			if( m_pRightButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_UP ) )
			{
				engine->ExecuteClientCmd( "spec_mode" );
			}
			if( m_pRightButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_DOWN ) )
			{
				engine->ExecuteClientCmd( "spec_mode" );
			}

		}

#ifdef TF_CLIENT_DLL
		// certain tf menus want '0' to cancel, not escape.
		if( gViewPortInterface )
		{
			IViewPortPanel *panel = gViewPortInterface->GetActivePanel();

			if( panel )
			{
				// if the class or team menus are up hide it with start
				if( ( Q_strcmp( panel->GetName(), "class_blue" ) == 0 ) ||
					( Q_strcmp( panel->GetName(), "class_red" ) == 0 ) ||
					( Q_strcmp( panel->GetName(), "team" ) == 0 ) )
				{

					if ( m_pLeftButtonStates->buttonJustPressed( SIXENSE_BUTTON_START ) )
					{
						panel->ShowPanel( false );
					}

				}
				else
				{
					// otherwise just do esc like normal
					if ( m_pLeftButtonStates->buttonJustPressed( SIXENSE_BUTTON_START ) )
					{
						::sendKeyState( 0x01, 1, 0); // 0x01 == esc
					}
					if ( m_pLeftButtonStates->buttonJustReleased( SIXENSE_BUTTON_START ) )
					{
						::sendKeyState( 0x01, 0, 1); // 0x01 == esc
					}
				}
			}
		}
#else
		// Press escape when the menu is up as well so we can exit
		if ( m_pLeftButtonStates->buttonJustPressed( SIXENSE_BUTTON_START ) )
		{
			::sendKeyState( 0x01, 1, 0); // 0x01 == esc
		}
		if ( m_pLeftButtonStates->buttonJustReleased( SIXENSE_BUTTON_START ) )
		{
			::sendKeyState( 0x01, 0, 1); // 0x01 == esc
		}
#endif

		return;
	}

#ifdef PORTAL2
	if ( sixense_features_enabled.GetInt() )
	{
		SwitchViewModes( pCmd );
	}

	// If the radial menu is open, all we care about is the release of the radial menu buttons.
	if ( engine->IsLocalPlayerResolvable() && IsRadialMenuOpen() )
	{
		if ( m_pLeftButtonStates->justReleased( SIXENSE_BUTTON_2 ) || m_pLeftButtonStates->justReleased( SIXENSE_BUTTON_4 ) )
		{
			engine->ClientCmd_Unrestricted( "-mouse_menu" );
		}

		if ( m_pLeftButtonStates->justReleased( SIXENSE_BUTTON_1 ) || m_pLeftButtonStates->justReleased( SIXENSE_BUTTON_3 ) )
		{
			engine->ClientCmd_Unrestricted( "-mouse_menu_taunt" );
		}

		pCmd->buttons = last_buttons;
		pCmd->forwardmove = last_forwardmove;
		pCmd->sidemove = last_sidemove;
		pCmd->sixense_flags = last_sixense_flags;
		return;
	}
#endif

	if( pCmd )
	{
		float forward = m_pFPSPlayerMovement->getWalkDir()[1] * cl_forwardspeed.GetFloat();
		float side = m_pFPSPlayerMovement->getWalkDir()[0] * cl_sidespeed.GetFloat();

		float angle = -GetViewAngleOffset()[YAW] * M_PI / 180.0f;

		pCmd->forwardmove = forward * cosf( angle ) - side * sinf( angle );
		pCmd->sidemove = forward * sinf( angle ) + side * cosf( angle );
	}

#ifdef CSTRIKE15
	if ( m_pLeftButtonStates->buttonJustPressed( SIXENSE_BUTTON_START ) )
	{
		BasePanel()->ShowScaleformPauseMenu( true );
	}
#else
	if ( m_pLeftButtonStates->buttonJustPressed( SIXENSE_BUTTON_START ) )
	{
		::sendKeyState( 0x01, 1, 0); // 0x01 == esc scancode
	}
	if ( m_pLeftButtonStates->buttonJustReleased( SIXENSE_BUTTON_START ) )
	{
		::sendKeyState( 0x01, 0, 1); // 0x01 == esc scancode
	}
#endif

#ifdef TF_CLIENT_DLL
	CHudMenuSpyDisguise *pSpyMenu = ( CHudMenuSpyDisguise * )GET_HUDELEMENT( CHudMenuSpyDisguise );
	if( pSpyMenu->IsVisible() ) 
	{
		if( m_pRightButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_LEFT ) )
		{
			pSpyMenu->HudElementKeyInput( 1, KEY_1, "slot1" );
		}
		if( m_pRightButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_UP ) )
		{
			pSpyMenu->HudElementKeyInput( 1, KEY_2, "slot2" );
		}
		if( m_pRightButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_RIGHT ) )
		{
			pSpyMenu->HudElementKeyInput( 1, KEY_3, "slot3" );
		}
		if( m_pRightButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_DOWN ) )
		{
			engine->ExecuteClientCmd( "lastinv" );
		}
		if( m_pRightButtonStates->buttonJustPressed( SIXENSE_BUTTON_JOYSTICK ) )
		{
			pSpyMenu->HudElementKeyInput( 1, KEY_3, "disguiseteam" );
		}
	}

	CHudMenuEngyBuild *pEngBuildMenu = ( CHudMenuEngyBuild * )GET_HUDELEMENT( CHudMenuEngyBuild );
	if( pEngBuildMenu->IsVisible() )
	{
		if( m_pRightButtonStates->buttonJustPressed( SIXENSE_BUTTON_3 ) )
		{
			pEngBuildMenu->HudElementKeyInput( 1, KEY_1, "slot1" );
		}
		if( m_pRightButtonStates->buttonJustPressed( SIXENSE_BUTTON_1 ) )
		{
			pEngBuildMenu->HudElementKeyInput( 1, KEY_2, "slot2" );
		}
		if( m_pRightButtonStates->buttonJustPressed( SIXENSE_BUTTON_2 ) )
		{
			pEngBuildMenu->HudElementKeyInput( 1, KEY_3, "slot3" );
		}
		if( m_pRightButtonStates->buttonJustPressed( SIXENSE_BUTTON_4 ) )
		{
			pEngBuildMenu->HudElementKeyInput( 1, KEY_4, "slot4" );
		}
		if( m_pRightButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_DOWN ) )
		{
			engine->ExecuteClientCmd( "lastinv" );
		}
	}

	CHudMenuEngyDestroy *pEngDestroyMenu = ( CHudMenuEngyDestroy * )GET_HUDELEMENT( CHudMenuEngyDestroy );
	if( pEngDestroyMenu->IsVisible() )
	{
		if( m_pRightButtonStates->buttonJustPressed( SIXENSE_BUTTON_3 ) )
		{
			pEngDestroyMenu->HudElementKeyInput( 1, KEY_1, "slot1" );
		}
		if( m_pRightButtonStates->buttonJustPressed( SIXENSE_BUTTON_1 ) )
		{
			pEngDestroyMenu->HudElementKeyInput( 1, KEY_2, "slot2" );
		}
		if( m_pRightButtonStates->buttonJustPressed( SIXENSE_BUTTON_2 ) )
		{
			pEngDestroyMenu->HudElementKeyInput( 1, KEY_3, "slot3" );
		}
		if( m_pRightButtonStates->buttonJustPressed( SIXENSE_BUTTON_4 ) )
		{
			pEngDestroyMenu->HudElementKeyInput( 1, KEY_4, "slot4" );
		}
		if( m_pRightButtonStates->stickJustPressed( sixenseUtils::IButtonStates::DIR_DOWN ) )
		{
			engine->ExecuteClientCmd( "lastinv" );
		}
	}

	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		if( m_pLeftButtonStates->absoluteTiltJustStarted( sixenseUtils::IButtonStates::DIR_UP ) )
		{
			engine->ClientCmd_Unrestricted( "training_continue" );
		}
	}

#endif

#ifdef PORTAL2
	// coop ping
	if ( m_pLeftButtonStates->justPressed( SIXENSE_BUTTON_1 ) || m_pLeftButtonStates->justPressed( SIXENSE_BUTTON_2 ) )
	{
		engine->ClientCmd_Unrestricted( "+mouse_menu" );
	}

	if ( m_pLeftButtonStates->justReleased( SIXENSE_BUTTON_1 ) || m_pLeftButtonStates->justReleased( SIXENSE_BUTTON_2 ) )
	{
		engine->ClientCmd_Unrestricted( "-mouse_menu" );
	}

	// coop remote view "tab"
	if ( m_pRightButtonStates->justPressed( SIXENSE_BUTTON_START ) )
	{
		engine->ClientCmd_Unrestricted( "+remote_view" );
		new_buttons |= IN_REMOTE_VIEW;
	}

	if ( m_pRightButtonStates->justReleased( SIXENSE_BUTTON_START ) )
	{
		engine->ClientCmd_Unrestricted( "-remote_view" );
		new_buttons &= ~IN_REMOTE_VIEW;
	}

	// walk
	//if ( m_pLeftButtonStates->justPressed( SIXENSE_BUTTON_2 ) )
	//{
	//	new_buttons |= IN_WALK;
	//}

	//if ( m_pLeftButtonStates->justReleased( SIXENSE_BUTTON_2 ) )
	//{
	//	new_buttons &= ~IN_WALK;
	//}

	// coop gesture
	if ( m_pLeftButtonStates->justPressed( SIXENSE_BUTTON_3 ) || m_pLeftButtonStates->justPressed( SIXENSE_BUTTON_4 ) )
	{
		engine->ClientCmd_Unrestricted( "+mouse_menu_taunt" );
	}

	if ( m_pLeftButtonStates->justReleased( SIXENSE_BUTTON_3 ) || m_pLeftButtonStates->justReleased( SIXENSE_BUTTON_4 ) )
	{
		engine->ClientCmd_Unrestricted( "-mouse_menu_taunt" );
	}

	// zooom
	{
		static bool momentary_zoom = false;
		static int momentary_zoom_release_count = 0;
		static double zoom_button_press_time = 0.0;

		// button just pressed and we're not already zooming, start zoom
		if ( m_pRightButtonStates->buttonJustPressed( SIXENSE_BUTTON_JOYSTICK ) )
		{
			new_buttons |= IN_ZOOM;
			momentary_zoom_release_count = 30;
			momentary_zoom = false;
			zoom_button_press_time = sixenseUtils::Time::getTimeInMilliseconds();
		}

		// check to see how long the zoom button was pressed. If it was really short, we should lock the zoom
		if ( ( m_pACD->controllers[m_nRightIndex].buttons & SIXENSE_BUTTON_JOYSTICK ) &&
			!momentary_zoom &&
			( C_BasePlayer::GetLocalPlayer() && C_BasePlayer::GetLocalPlayer()->IsZoomed() ) )
		{
			double current_time = sixenseUtils::Time::getTimeInMilliseconds();
			double button_held_time = current_time - zoom_button_press_time;
			const double momentary_thresh_time = sixense_zoom_momentary_time.GetFloat();
			if ( button_held_time > momentary_thresh_time )
			{
				momentary_zoom = true;
			}
		}

		// button released and we're zooming
		if ( momentary_zoom && m_pRightButtonStates->buttonJustReleased( SIXENSE_BUTTON_JOYSTICK ) )
		{
			new_buttons |= IN_ZOOM;
			momentary_zoom_release_count = 30;
		}

		// delay release
		if ( momentary_zoom_release_count )
		{
			momentary_zoom_release_count--;
			if ( momentary_zoom_release_count == 0 )
			{
				new_buttons &= ~IN_ZOOM;
			}
		}
	}

	if ( sixense_features_enabled.GetInt() )
	{
		// 1d cube scaling on L4 button, use IN_SCALE1D for button mapping
		if ( m_pACD->controllers[m_nLeftIndex].buttons & SIXENSE_BUTTON_BUMPER )
		{
			new_buttons |= IN_SCALE1D;
		}
		else
		{
			new_buttons &= ~IN_SCALE1D;
		}

		// Button one-to-one
		if ( !sixense_dist_one_to_one_enabled.GetInt() && sixense_auto_one_to_one_enabled.GetInt() &&
			IsHoldingObject() &&
			!m_bIs1to1ModeScaling &&
			( C_Portal_Player::GetLocalPortalPlayer() &&
			!C_Portal_Player::GetLocalPortalPlayer()->IsScalingUseItem() &&
			!C_Portal_Player::GetLocalPortalPlayer()->IsScalingUseItemTurret() ) )
		{
			// one to one
			if ( m_pFPSEvents->eventStarted( sixenseUtils::IFPSEvents::ONE_TO_ONE_CARRY ) )
			{
				SetOneToOneMode( true );
			}

			if ( m_pFPSEvents->eventStopped( sixenseUtils::IFPSEvents::ONE_TO_ONE_CARRY ) )
			{
				SetOneToOneMode( false );
			}
		}

		// dist one-to-one
		if ( sixense_dist_one_to_one_enabled.GetInt() &&
			IsHoldingObject() &&
			!m_bIs1to1ModeScaling &&
			!m_bScalingLockedOneToOne &&
			( C_Portal_Player::GetLocalPortalPlayer() &&
			!C_Portal_Player::GetLocalPortalPlayer()->IsScalingUseItem() &&
			!C_Portal_Player::GetLocalPortalPlayer()->IsScalingUseItemTurret() ) )
		{

			Vector3 cur_pos = Vector3( m_pACD->controllers[m_nRightIndex].pos );

			Vector3 delta_vec = cur_pos - m_GrabPos;

			static float fEndOneToOneRatchetDelayTime = 0.0f;
			if ( !IsInOneToOneMode() &&
				( C_Portal_Player::GetLocalPortalPlayer() && C_Portal_Player::GetLocalPortalPlayer()->GetActiveWeapon() ) &&
				( ( delta_vec[2] < -sixense_dist_one_to_one_dist.GetFloat() ) || // start one to one if you reach forwards
				( delta_vec[1] > 1.5f * sixense_dist_one_to_one_dist.GetFloat() ) ) ) // start one to one if you raise up 1.5 times the dist too...
			{
				SetOneToOneMode( true );
				fEndOneToOneRatchetDelayTime = 0.0f;
			}
			else if ( IsInOneToOneMode() &&
				( delta_vec[2] > -sixense_dist_one_to_one_dist.GetFloat() ) &&
				( delta_vec[1] < 1.5f * sixense_dist_one_to_one_dist.GetFloat() ) &&
				( delta_vec.length() < 2.0f*sixense_dist_one_to_one_dist.GetFloat() ) &&
				IsAimingForwards() )
			{
				// if ratcheting when ending 1to1 mode, first end ratchet, then end 1to1 after a short delay so end ratchet can propogate to server
				if ( m_bIs1to1ModeRatcheting )
				{
					m_pFPSViewAngles->setRatcheting( false );
					m_bIs1to1ModeRatcheting = false;
					fEndOneToOneRatchetDelayTime = gpGlobals->curtime;
				}
				else if ( ( fEndOneToOneRatchetDelayTime == 0.0f ) ||
					( ( gpGlobals->curtime - fEndOneToOneRatchetDelayTime ) > sixense_dist_one_to_one_end_ratchet_delay.GetFloat() ) )
				{
					SetOneToOneMode( false );
					fEndOneToOneRatchetDelayTime = 0.0f;
				}
			}
			else
			{
				fEndOneToOneRatchetDelayTime = 0.0f;
			}

		}

		// portal tweaking
		if ( sixense_portal_tweaking_enabled.GetInt() )
		{
			// portal 1 tweaking on left trigger
			if ( m_bIsLeftTriggerDown )
			{
				new_buttons |= IN_TWEAK2;
			}
			else
			{
				new_buttons &= ~IN_TWEAK2;
			}

			// portal 2 tweaking on right trigger
			if ( m_bIsRightTriggerDown )
			{
				new_buttons |= IN_TWEAK1;
			}
			else
			{
				new_buttons &= ~IN_TWEAK1;
			}
		}
	}

	bool jump_disabled = false;

	if ( m_pLeftButtonStates->justReleased( SIXENSE_BUTTON_BUMPER ) )
	{
		m_fDisableJumpUntil = ( sixenseUtils::Time::getTimeInMilliseconds() ) + sixense_scale_disables_jump_timer.GetFloat() * 1000.0f;
	}


	// If the disable jump timer is set
	if ( m_fDisableJumpUntil != 0.0 && ( m_fDisableJumpUntil > ( sixenseUtils::Time::getTimeInMilliseconds() ) ) )
	{
		jump_disabled = true;
	}

	// Or if we're actively scaling
	if ( new_buttons & IN_SCALE1D )
	{
		jump_disabled = true;
	}

	// jump
	if ( !jump_disabled &&
		( m_pFPSEvents->eventStarted( sixenseUtils::IFPSEvents::JUMP ) ||
		( m_pRightButtonStates->justPressed( SIXENSE_BUTTON_3 ) || m_pRightButtonStates->justPressed( SIXENSE_BUTTON_4 ) ) ) )
	{
		new_buttons |= IN_JUMP;
	}

	if ( m_pFPSEvents->eventStopped( sixenseUtils::IFPSEvents::JUMP ) ||
		( m_pRightButtonStates->justReleased( SIXENSE_BUTTON_3 ) || m_pRightButtonStates->justReleased( SIXENSE_BUTTON_4 ) ) )
	{
		new_buttons &= ~IN_JUMP;
	}

	{
		static bool momentary_hold = false;
		static bool holding_object = false;
		static int momentary_hold_release_count = 0;
		static double hold_button_press_time = 0.0;

		if( m_pRightButtonStates->justPressed( SIXENSE_BUTTON_1 ) )
		{
			// button just pressed and we're not already holding, start holding
			// This is a hack to center wheatley. Basically recenter whenever the 1 button is pressed. It's ok to recenter on drop.
			SetBaseOffset();
			m_GrabPos = Vector3( m_pACD->controllers[m_nRightIndex].pos );
		}

		if ( ( m_pRightButtonStates->justPressed( SIXENSE_BUTTON_1 ) || m_pRightButtonStates->justPressed( SIXENSE_BUTTON_2 ) ) && !( last_buttons & IN_USE ) )
		{
			new_buttons |= IN_USE;
			momentary_hold_release_count = 30;
			momentary_hold = false;

			hold_button_press_time = sixenseUtils::Time::getTimeInMilliseconds();

			if( !IsHoldingObject() ) {

				// If distance one to one is enabled, reset the hold offset on pickup
				if ( sixense_dist_one_to_one_enabled.GetInt() )
				{
					SetBaseOffset();
				}

				m_GrabPos = Vector3( m_pACD->controllers[m_nRightIndex].pos );
			}


		}

		if ( ( ( m_pACD->controllers[m_nRightIndex].buttons & SIXENSE_BUTTON_1 ) || ( m_pACD->controllers[m_nRightIndex].buttons & SIXENSE_BUTTON_2 ) ) && !momentary_hold )
		{
			// Check to see how long the hold button was pressed. If it was really short, we should
			// lock the hold
			double current_time = sixenseUtils::Time::getTimeInMilliseconds();

			double button_held_time = current_time - hold_button_press_time;

			const double momentary_thresh_time = sixense_pickup_momentary_time.GetFloat();

			if ( button_held_time > momentary_thresh_time )
			{
				momentary_hold = true;
			}
		}

		// button released and we're holding, drop it
		if ( momentary_hold && ( m_pRightButtonStates->justReleased( SIXENSE_BUTTON_1 ) || m_pRightButtonStates->justReleased( SIXENSE_BUTTON_2 ) ) && IsHoldingObject() )
		{
			new_buttons |= IN_USE;
			momentary_hold_release_count = 30;
		}

		if ( momentary_hold_release_count )
		{
			momentary_hold_release_count--;

			if ( momentary_hold_release_count == 0 )
			{
				new_buttons &= ~IN_USE;
			}
		}
	}

	unsigned char ratchet_button = SIXENSE_BUTTON_BUMPER;

	if ( m_pRightButtonStates->buttonJustPressed( ratchet_button ) )
	{
		m_pFPSViewAngles->setRatcheting( true );

		if ( m_bIsIn1to1Mode )
		{
			m_bIs1to1ModeRatcheting = true;
		}
	}

	if ( m_pRightButtonStates->buttonJustReleased( ratchet_button ) )
	{
		m_pFPSViewAngles->setRatcheting( false );
		m_bIs1to1ModeRatcheting = false;
	}
#endif

	if( pCmd )
	{
		pCmd->buttons = new_buttons;

		last_forwardmove = pCmd->forwardmove;
		last_sidemove = pCmd->sidemove;
	}

	last_buttons = new_buttons;

#ifdef SIXENSE_PLAYER_DATA
	last_sixense_flags = pCmd->sixense_flags;
#endif
}

#ifdef PORTAL2
bool SixenseInput::SendKeyToActiveWindow(ButtonCode_t key)
{
	KeyValues *kv = new KeyValues( "KeyCodePressed", "code", key );
	vgui::ivgui()->PostMessage(vgui::input()->GetFocus(), kv, NULL);
	return true;
}
#endif

void SixenseInput::SixenseUpdateMouseCursor()
{

#if defined( HL2_CLIENT_DLL ) || defined( TF_CLIENT_DLL )
	// If there's no mouse cursor visible, don't ever move the mouse or click here.
	// Gesture bindings can still call 'sixense_left_click' for times when a click is
	// necessary but no mouse cursor.
	if( vgui::surface() && !vgui::surface()->IsCursorVisible() )
	{
		return;
	}
#endif

	if( !sixense_left_handed.GetInt() ) {
		m_nLeftIndex = m_pControllerManager->getIndex( sixenseUtils::IControllerManager::P1L );
		m_nRightIndex = m_pControllerManager->getIndex( sixenseUtils::IControllerManager::P1R );
	} else {
		m_nLeftIndex = m_pControllerManager->getIndex( sixenseUtils::IControllerManager::P1R );
		m_nRightIndex = m_pControllerManager->getIndex( sixenseUtils::IControllerManager::P1L );
	}

#if 0
	sixenseUtils::mouseAndKeyboardWin32::processQueue();
#endif

	// Keep track of when the left button is down so we can turn it off if we leave mouse mode
	static bool left_clicked = false;

	if ( m_nRightIndex == -1 ) return;

	m_pSixenseAPI->sixenseGetAllNewestData( m_pACD );

	static unsigned char last_seq = 0;
	unsigned char seq = m_pACD->controllers[m_nRightIndex].sequence_number;

	if ( last_seq == seq )
	{
		return;
	}

	last_seq = seq;

	if ( !InMenuMode() )
	{

		// If we're not in mouse mode, check to see if we just entered it and need to switch out.
		if ( left_clicked )
		{
			::sendMouseClick( 0, 1 );
			left_clicked = false;
		}

		return;
	}

	//m_pLeftButtonStates->update( &m_pACD->controllers[m_nLeftIndex] );
	//m_pRightButtonStates->update( &m_pACD->controllers[m_nRightIndex] );

	if ( m_pLeftButtonStates->buttonJustPressed( SIXENSE_BUTTON_START ) )
	{
		::sendKeyState( 0x01, 1, 0); // 0x01 == esc scancode
	}
	if ( m_pLeftButtonStates->buttonJustReleased( SIXENSE_BUTTON_START ) )
	{
		::sendKeyState( 0x01, 0, 1); // 0x01 == esc scancode
	}

	if( m_pRightButtonStates->triggerJustPressed() || m_pRightButtonStates->buttonJustPressed( SIXENSE_BUTTON_1 ) )
	{
		::sendMouseClick( 1, 0 );
		left_clicked = true;
	}
	else if( m_pRightButtonStates->triggerJustReleased() || m_pRightButtonStates->buttonJustReleased( SIXENSE_BUTTON_1 ) )
	{
		::sendMouseClick( 0, 1 );
		left_clicked = false;
	}

#ifdef WIN32

#ifdef PORTAL2
	const char *window_name = "Portal 2 Sixense MotionPack";
#endif
#ifdef CSTRIKE15
	const char *window_name = "Counter-Strike Source";
#endif
#ifdef HL2_CLIENT_DLL
	const char *window_name = "Half-Life 2";
#endif
#ifdef TF_CLIENT_DLL
	
	const int str_len = 128;
	static char window_name[str_len] = "\0";

	if( window_name[0] == '\0' )
	{
		const char *pGameDir = COM_GetModDirectory();
		if ( FStrEq( pGameDir, "tf_beta" ) )
		{
			Q_strncpy( window_name, "Team Fortress 2 Beta", str_len );
		}
		else
		{
			Q_strncpy( window_name, "Team Fortress 2", str_len );
		}
	}
#endif
#ifdef TERROR
	const char *window_name = "Left 4 Dead 2";
#endif
#if defined( CSTRIKE_DLL ) && !defined( CSTRIKE15 ) && !defined( TERROR )
	const char *window_name = "Counter-Strike Source";
#endif

	// Right now you can't move the mouse if the video hint panel is up.
	if ( sixense_mouse_enabled.GetInt() && InMenuMode() && ( GetActiveWindow() == FindWindow( NULL, window_name ) ) )
#else
	if ( sixense_mouse_enabled.GetInt() && InMenuMode() )
#endif
	{

		if ( m_pLaserPointer )
		{

			static Vector2 filtered_pixel_pos( -999, -999 );


			int hand_index;

#ifdef PORTAL2
			float pixel_scale = 0.6f;
			if ( engine->IsLocalPlayerResolvable() && IsRadialMenuOpen() )
			{
				hand_index = m_nLeftIndex;
				pixel_scale = 0.5f;

			}
			else
			{
#else
			{
#endif
				hand_index = m_nRightIndex;

			}

			static bool radial_menu_up_last_frame = false;

#ifdef PORTAL2
			if ( engine->IsLocalPlayerResolvable() && ( IsRadialMenuOpen() != radial_menu_up_last_frame ) )
			{
				// Switched modes, reset the filters...
				filtered_pixel_pos = Vector2( -999, -999 ); // reset the filter
			}

			if ( engine->IsLocalPlayerResolvable() ) radial_menu_up_last_frame = IsRadialMenuOpen();
#endif

			Vector3 view_angles;

			sixenseMath::Matrix3 mat( m_pACD->controllers[hand_index].rot_mat );


			Vector3 forwards;


			Vector3 controller_forwards( 0, 0, -1 ), controller_up( 0, 1, 0 );
			controller_forwards = mat * controller_forwards;


			sixenseMath::Vector3 xz_projection( controller_forwards[0], 0.0f, controller_forwards[2] );
			xz_projection.normalize();


			// Compute the heading angle
			float heading_dot = xz_projection * sixenseMath::Vector3( 0, 0, -1 );

			float heading_angle = -acosf(heading_dot) * 180.0f/3.1415926f;



			sixenseMath::Vector3 cross = xz_projection ^ sixenseMath::Vector3( 0, 0, -1 );

			if( cross[1] > 0.0f ) {
				heading_angle *= -1.0f;
			}

			// Round to +/- 180
			if( heading_angle > 360.0f ) heading_angle -= 360.0f;
			if( heading_angle < 0.0f ) heading_angle += 360.0f;
			if( heading_angle > 180.0f ) heading_angle -= 360.0f;


			// Compute the pitch
			float pitch_angle = asin( controller_forwards[1] ) * 180.0f/3.1415926f;

			float hfov, vfov;
			GetFOV( &hfov, &vfov );

			heading_angle *= sixense_mouse_sensitivity.GetFloat();
			pitch_angle *= sixense_mouse_sensitivity.GetFloat();


			Vector2 norm_coord( heading_angle/hfov + 0.5f, pitch_angle/vfov + 0.5f );

			norm_coord[0] = clamp<float>( norm_coord[0], 0.0f, 1.0f );
			norm_coord[1] = clamp<float>( norm_coord[1], 0.0f, 1.0f );

#ifdef WIN32
			RECT win_rect;
			GetWindowRect( GetActiveWindow(), &win_rect );

			RECT desktop_rect;
			GetWindowRect( GetDesktopWindow(), &desktop_rect );

			// make top actually reference the top of the screen (windows has y=0 at the top)
			win_rect.top = desktop_rect.bottom - win_rect.top;
			win_rect.bottom = desktop_rect.bottom - win_rect.bottom;


			int title_bar_height = GetSystemMetrics( SM_CYCAPTION ) + 2; // fudge these a little bit so we can't click on the borders
			int border_size = GetSystemMetrics( SM_CYBORDER ) + 2;

			win_rect.left += border_size;
			win_rect.right -= border_size;

			win_rect.bottom += border_size;
			win_rect.top -= border_size + title_bar_height;

			float desktop_width = (float)desktop_rect.right - (float)desktop_rect.left;
			float desktop_height = (float)desktop_rect.bottom - (float)desktop_rect.top;

			float win_norm_left = (float)win_rect.left/desktop_width, win_norm_right = (float)win_rect.right/desktop_width;
			float win_norm_top = (float)win_rect.top/desktop_height, win_norm_bottom = (float)win_rect.bottom/desktop_height;

			Vector2 abs_mouse_pos( win_norm_left + norm_coord[0] * (win_norm_right-win_norm_left), win_norm_bottom + norm_coord[1] * (win_norm_top-win_norm_bottom) );



			static Vector2 last_pixel_pos;

			const float mouse_filter_val = 0.8f;

			if ( filtered_pixel_pos == Vector2( -999, -999 ) )
			{
				filtered_pixel_pos = abs_mouse_pos;
				last_pixel_pos = abs_mouse_pos;
			}
			else
			{
				filtered_pixel_pos = filtered_pixel_pos * mouse_filter_val + abs_mouse_pos * ( 1.0f - mouse_filter_val );
			}

			::sendAbsoluteMouseMove( filtered_pixel_pos[0], filtered_pixel_pos[1] );
#endif

		}
	}
}


QAngle SixenseInput::GetViewAngles()
{

	if ( m_pSixenseAPI->sixenseIsControllerEnabled( 0 ) )
	{
		if ( m_pFPSViewAngles )
		{

			QAngle sixense_aim_angle;

			sixenseMath::Vector3 vec = m_pFPSViewAngles->getViewAngles();

			sixense_aim_angle[YAW] = vec[0];
			sixense_aim_angle[PITCH] = vec[1];
			sixense_aim_angle[ROLL] = vec[2];

			return sixense_aim_angle;
		}
	}

	return QAngle();

}


QAngle SixenseInput::GetViewAngleOffset()
{

	if ( m_pSixenseAPI->sixenseIsControllerEnabled( 0 ) && !UseVR() )
	{
		if ( m_pFPSViewAngles )
		{

			QAngle sixense_aim_angle;

			sixenseMath::Vector3 vec = m_pFPSViewAngles->getViewAngleOffset();

			sixense_aim_angle[YAW] = vec[0];
			sixense_aim_angle[PITCH] = vec[1];
			sixense_aim_angle[ROLL] = vec[2];

#if defined( TF_CLIENT_DLL )
			sixense_aim_angle *= g_fDemoChargeViewOffsetScale;
#endif

			return sixense_aim_angle;
		}
	}

	return QAngle(0.f, 0.f, 0.f );

}

#if 0
static void printmat( std::string name, sixenseMath::Matrix4 mat )
{
	Msg( "%s\n", name.c_str() );
	Msg( "[0][0]=%f  [1][0]=%f  [2][0]=%f  [3][0]=%f \n", mat[0][0], mat[1][0], mat[2][0], mat[3][0] );
	Msg( "[0][1]=%f  [1][1]=%f  [2][1]=%f  [3][1]=%f \n", mat[0][1], mat[1][1], mat[2][1], mat[3][1] );
	Msg( "[0][2]=%f  [1][2]=%f  [2][2]=%f  [3][2]=%f \n", mat[0][2], mat[1][2], mat[2][2], mat[3][2] );
	Msg( "[0][3]=%f  [1][3]=%f  [2][3]=%f  [3][3]=%f \n\n", mat[0][3], mat[1][3], mat[2][3], mat[3][3] );

}
#endif

matrix3x4_t ConvertMatrix( sixenseMath::Matrix4 ss_mat )
{

	sixenseMath::Matrix4 tmp_mat( ss_mat );

	tmp_mat = tmp_mat * sixenseMath::Matrix4::rotation( -3.1415926f / 2.0f, Vector3( 1, 0, 0 ) );
	tmp_mat = tmp_mat * sixenseMath::Matrix4::rotation( -3.1415926f / 2.0f, Vector3( 0, 0, 1 ) );
	tmp_mat = tmp_mat * sixenseMath::Matrix4::rotation( 3.1415926f, Vector3( 0, 1, 0 ) );

	tmp_mat = sixenseMath::Matrix4::rotation( -3.1415926f / 2.0f, Vector3( 1, 0, 0 ) ) * tmp_mat;

	matrix3x4_t retmat;

	MatrixInitialize( retmat, Vector( tmp_mat[3][0], tmp_mat[3][1], tmp_mat[3][2] ),
		Vector( tmp_mat[0][0], tmp_mat[0][1], tmp_mat[0][2] ),
		Vector( tmp_mat[1][0], tmp_mat[1][1], tmp_mat[1][2] ),
		Vector( tmp_mat[2][0], tmp_mat[2][1], tmp_mat[2][2] ) );


	return retmat;

}

void SixenseInput::Rumble( unsigned char index, unsigned char rumbleData, unsigned char rumbleFlags )
{
	m_pSixenseAPI->sixenseTriggerVibration( 0, 10, 0 );
	m_pSixenseAPI->sixenseTriggerVibration( 1, 10, 0 );
}

void SixenseInput::Rumble( unsigned char playerIndex, unsigned char handIndex, unsigned char rumbleData, unsigned char rumbleFlags )
{
	// find controller index based on player and hand indexes
	int controller_index = -1;

	switch ( playerIndex )
	{
	case 0:
		controller_index = m_pControllerManager->getIndex( ( 0 == handIndex ) ?
			sixenseUtils::IControllerManager::P1L :
		sixenseUtils::IControllerManager::P1R );
		break;
	case 1:
		controller_index = m_pControllerManager->getIndex( ( 0 == handIndex ) ?
			sixenseUtils::IControllerManager::P2L :
		sixenseUtils::IControllerManager::P2R );
		break;
	case 2:
		controller_index = m_pControllerManager->getIndex( ( 0 == handIndex ) ?
			sixenseUtils::IControllerManager::P3L :
		sixenseUtils::IControllerManager::P3R );
		break;
	case 3:
		controller_index = m_pControllerManager->getIndex( ( 0 == handIndex ) ?
			sixenseUtils::IControllerManager::P4L :
		sixenseUtils::IControllerManager::P4R );
		break;
	default:
		break;
	}

	// if valid controller index and rumble data, set vibration
	if ( ( controller_index >= 0 ) && ( controller_index <= 3 ) )
	{
		m_pSixenseAPI->sixenseTriggerVibration( controller_index, rumbleData, 0 );
	}
}


void SixenseInput::CreateGUI( vgui::VPANEL parent )
{

	parent = vgui::ipanel()->GetParent( parent );

	m_SixenseFrame = new SixenseGUIFrame( enginevgui->GetPanel( PANEL_ROOT ), "SixenseGUIFrame" );
	m_SixenseFrame->SetVisible( false );
	m_SixenseFrame->MoveToFront();
}

void SixenseInput::LoadDefaultSettings( int level )
{

	if ( level == 0 )
	{
		Msg( "Loading default settings for low sensitivity\n" );

		sixense_sensitivity_level.SetValue( 0 );

		sixense_aim_freeaim_accel_band_exponent.SetValue( 1.0f );
		sixense_aim_freeaim_auto_level_rate.SetValue( 1.0f );
		sixense_aim_freeaim_accel_band_size.SetValue( 30 );
		sixense_aim_freeaim_max_speed.SetValue( 5.0f );
		sixense_aim_freeaim_dead_zone_radius.SetValue( 1.0f );
		sixense_aim_freeaim_heading_multiplier.SetValue( 1.0f );
		sixense_aim_freeaim_pitch_multiplier.SetValue( 1.0f );
		sixense_exit_metroid_blend.SetValue( 0.9f );
		//sixense_aim_freeaim_switch_blend_time_enter.SetValue( 0.8f );
		sixense_aim_1to1_heading_multiplier.SetValue( 2.0f );
		sixense_aim_1to1_pitch_multiplier.SetValue( 2.0f );

		// dual analog (1.0 - 5.0)
		sixense_feet_angles_offset_stick_spin_horiz_multiplier.SetValue( 2.5f );
		sixense_feet_angles_offset_stick_spin_vert_multiplier.SetValue( 1.5f );
		sixense_feet_angles_offset_stick_spin_exponent.SetValue( 1.0f );
	}
	else if ( level == 1 )
	{
		Msg( "Loading default settings for medium sensitivity\n" );

		sixense_sensitivity_level.SetValue( 1 );

		sixense_aim_freeaim_accel_band_exponent.SetValue( 1.0f );
		sixense_aim_freeaim_auto_level_rate.SetValue( 1.0f );
		sixense_aim_freeaim_accel_band_size.SetValue( 20 );
		sixense_aim_freeaim_max_speed.SetValue( 7.0f );
		sixense_aim_freeaim_dead_zone_radius.SetValue( 0.25f );
		sixense_aim_freeaim_heading_multiplier.SetValue( 1.5f );
		sixense_aim_freeaim_pitch_multiplier.SetValue( 1.5f );
		//sixense_aim_freeaim_switch_blend_time_enter.SetValue( 1.5f );
		sixense_exit_metroid_blend.SetValue( 0.925f );
		sixense_aim_1to1_heading_multiplier.SetValue( 2.5f );
		sixense_aim_1to1_pitch_multiplier.SetValue( 2.5f );

		// dual analog (1.0 - 5.0)
		sixense_feet_angles_offset_stick_spin_horiz_multiplier.SetValue( 5.0f );
		sixense_feet_angles_offset_stick_spin_vert_multiplier.SetValue( 3.0f );
		sixense_feet_angles_offset_stick_spin_exponent.SetValue( 1.0f );
	}
	else if ( level == 2 )
	{
		Msg( "Loading default settings for high sensitivity\n" );

		sixense_sensitivity_level.SetValue( 2 );

		sixense_aim_freeaim_accel_band_exponent.SetValue( 1.0f );
		sixense_aim_freeaim_auto_level_rate.SetValue( 1.0f );
		sixense_aim_freeaim_accel_band_size.SetValue( 15 );
		sixense_aim_freeaim_max_speed.SetValue( 12.0f );
		sixense_aim_freeaim_dead_zone_radius.SetValue( 0.0f );
		sixense_aim_freeaim_heading_multiplier.SetValue( 1.75f );
		sixense_aim_freeaim_pitch_multiplier.SetValue( 1.75f );
		//sixense_aim_freeaim_switch_blend_time_enter.SetValue( 2.0f );
		sixense_exit_metroid_blend.SetValue( 0.95f );
		sixense_aim_1to1_heading_multiplier.SetValue( 3.0f );
		sixense_aim_1to1_pitch_multiplier.SetValue( 3.0f );

		// dual analog (1.0 - 5.0)
		sixense_feet_angles_offset_stick_spin_horiz_multiplier.SetValue( 7.5f );
		sixense_feet_angles_offset_stick_spin_vert_multiplier.SetValue( 4.5f );
		sixense_feet_angles_offset_stick_spin_exponent.SetValue( 1.0f );
	}
	else if ( level == 3 )
	{
		Msg( "Loading default settings for custom sensitivity\n" );

		sixense_sensitivity_level.SetValue( 3 );
	}		
	else if ( level == 4 )
	{
		Msg( "Loading default settings for \"static xhair\" sensitivity\n" );

		sixense_sensitivity_level.SetValue( 4 );

		sixense_aim_freeaim_accel_band_exponent.SetValue( 1.0f );
		sixense_aim_freeaim_auto_level_rate.SetValue( 1.0f );
		sixense_aim_freeaim_accel_band_size.SetValue( 20 );
		sixense_aim_freeaim_max_speed.SetValue( 30.0f );
		sixense_aim_freeaim_dead_zone_radius.SetValue( 0.0f );
		sixense_aim_freeaim_heading_multiplier.SetValue( 0.0f );
		sixense_aim_freeaim_pitch_multiplier.SetValue( 0.0f );
		//sixense_aim_freeaim_switch_blend_time_enter.SetValue( 2.0f );
		sixense_exit_metroid_blend.SetValue( 0.95f );
		sixense_aim_1to1_heading_multiplier.SetValue( 3.0f );
		sixense_aim_1to1_pitch_multiplier.SetValue( 3.0f );
	}
	else if ( level == 5 )
	{
		Msg( "Loading default settings for \"quick turn\" sensitivity\n" );

		sixense_sensitivity_level.SetValue( 5 );

		sixense_aim_freeaim_accel_band_exponent.SetValue( 1.5f );
		sixense_aim_freeaim_auto_level_rate.SetValue( 1.0f );
		sixense_aim_freeaim_accel_band_size.SetValue( 20 );
		sixense_aim_freeaim_max_speed.SetValue( 30.0f );
		sixense_aim_freeaim_dead_zone_radius.SetValue( 0.0f );
		sixense_aim_freeaim_heading_multiplier.SetValue( 1.75f );
		sixense_aim_freeaim_pitch_multiplier.SetValue( 1.75f );
		//sixense_aim_freeaim_switch_blend_time_enter.SetValue( 2.0f );
		sixense_exit_metroid_blend.SetValue( 0.95f );
		sixense_aim_1to1_heading_multiplier.SetValue( 3.0f );
		sixense_aim_1to1_pitch_multiplier.SetValue( 3.0f );
	}
}


SixenseGUIFrame::SixenseGUIFrame( vgui::VPANEL parent, char const *panelName ) :
BaseClass( NULL, panelName )
{

	SetMoveable( false );
	SetSizeable( false );
	SetCloseButtonVisible( false );

	SetParent( parent );

	SetAutoDelete( false );


	SetBgColor( Color( 100, 100, 100, 255 ) );
	SetFgColor( Color( 100, 100, 100, 255 ) );
	SetAlpha( 255 );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );


	int img_x = 985, img_y = 565;
	int frame_x = 1000, frame_y = 580;

	SetSize( frame_x, frame_y );

	m_ImagePanel = new vgui::ImagePanel( this, "SixenseControllerManagerImage" );
	m_ImagePanel->SetDrawColor( Color( 255, 255, 255, 255 ) );
	m_ImagePanel->SetShouldScaleImage( true );
	m_ImagePanel->SetSize( img_x, img_y );
	m_ImagePanel->SetPos( ( frame_x - img_x ) / 2, ( frame_y - img_y ) / 2 );

	// Clear the title bar
	SetTitle( "", false );

	SetVisible( false );

	MoveToCenterOfScreen();

}

void SixenseGUIFrame::SetVisible( bool state )
{
	vgui::Frame::SetVisible( state );

	m_ImagePanel->SetVisible( state );

	// Make this panel modal while it's visible.
	vgui::input()->SetAppModalSurface( state ? GetVPanel() : NULL );
}


SixenseGUIFrame::~SixenseGUIFrame()
{
	if ( m_ImagePanel && !m_ImagePanel->IsAutoDeleteSet() )
	{
		delete m_ImagePanel;
		m_ImagePanel = NULL;
	}
}

void SixenseGUIFrame::setImage( CUtlString img_name )
{
	m_ImagePanel->SetImage( img_name.String() );
	SixenseInput::m_SixenseFrame->MoveToCenterOfScreen();

}



void SixenseInput::SetFilterLevel( float near_range, float near_val, float far_range, float far_val )
{
	if ( near_range == 0.0f && far_range == 0.0f )
	{
		m_pSixenseAPI->sixenseSetFilterEnabled( 0 );
	}
	else
	{

		m_pSixenseAPI->sixenseSetFilterEnabled( 1 );
		m_pSixenseAPI->sixenseSetFilterParams( near_range, near_val, far_range, far_val );
	}
}

void SixenseInput::DisableGestures( int disable )
{
	m_nGesturesDisabled = disable;

	// if the player is ducking when we disable gestures do one last unduck to make sure they don't get stuck ducking
	m_nShouldUnduck = true;
}

void SixenseInput::DisableFreeAimSpin( int disable )
{
	m_nFreeaimSpinDisabled = disable;

	if ( disable == 0 )
	{
		BlendView();
	}
}

#ifdef PORTAL2

SixenseBaseWarning::SixenseBaseWarning(vgui::Panel *parent, const char *panelName) :
BaseClass(parent, panelName)
{
	SetTitleBarVisible(false);

	SetDeleteSelfOnClose(true);
	SetProportional(true);

	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	SetMenuButtonVisible(false);
	SetCloseButtonVisible(false);

	SetMoveable(false);
	SetSizeable(false);

	SetVisible(true);
}

void SixenseBaseWarning::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	LoadControlSettings("resource/UI/basemodui/SixenseBaseWarning.res");
}

#endif

//////

static void ToggleFrame()
{
	g_pSixenseInput->CreateGUI( enginevgui->GetPanel( PANEL_TOOLS ) );

}
static ConCommand sixense_show_frame( "sixense_show_frame", ToggleFrame, "Show/hide Sixense UI." );

//////

static void SetBaseOffset()
{
		g_pSixenseInput->SetBaseOffset();
	}

ConCommand sixense_set_base_offset( "sixense_set_base_offset", SetBaseOffset );

//////

static void DisableGestures( const CCommand &args )
{
		if ( args.ArgC() > 1 )
			g_pSixenseInput->DisableGestures( atoi( args[1] ) );
	}

ConCommand sixense_disable_gestures( "sixense_disable_gestures", DisableGestures );

//////

static void DisableFreeAimSpin( const CCommand &args )
{
		if ( args.ArgC() > 1 )
			g_pSixenseInput->DisableFreeAimSpin( atoi( args[1] ) );
	}

ConCommand sixense_aim_freeaim_spin_disabled( "sixense_aim_freeaim_spin_disabled", DisableFreeAimSpin );

//////

void set_filter_params( const CCommand &args )
{

	if ( args.ArgC() < 5 )
	{
		Msg( "Usage: set_filter_params <near_range> <near_val> <far_range> <far_val>\n" );
		return;
	}

		g_pSixenseInput->SetFilterLevel( atof( args[1] ), atof( args[2] ), atof( args[3] ), atof( args[4] ) );
	}

static ConCommand sixense_set_filter_params( "sixense_set_filter_params", set_filter_params );

//////

void SixenseSensitivityLevelChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
		g_pSixenseInput->LoadDefaultSettings( sixense_sensitivity_level.GetInt() );
	}

//////

bool directionFromString( CUtlString dir_str, sixenseUtils::IButtonStates::Direction *dir ) {

	if( dir_str == "up" ) {
		*dir = sixenseUtils::IButtonStates::DIR_UP;
	}
	else if( dir_str == "down" ) 
	{
		*dir = sixenseUtils::IButtonStates::DIR_DOWN;
	}
	else if( dir_str == "left" ) 
	{
		*dir = sixenseUtils::IButtonStates::DIR_LEFT;
	}
	else if( dir_str == "right" ) 
	{
		*dir = sixenseUtils::IButtonStates::DIR_RIGHT;
	}
	else if( dir_str == "cw" ) 
	{
		*dir = sixenseUtils::IButtonStates::DIR_CW;
	}
	else if( dir_str == "ccw" ) 
	{
		*dir = sixenseUtils::IButtonStates::DIR_CCW;
	}
	else
	{
		Msg( "Unknown direction %s, shoud be 'up' 'down' 'left' 'right' 'cw' or 'ccw'\n", dir_str.String() );
		return false;
	}

	return true;
}


bool buttonMaskFromString( CUtlString button, unsigned short *button_token ) {

	if ( button == "1" )
	{
		*button_token = SIXENSE_BUTTON_1;
	}
	else if ( button == "2" )
	{
		*button_token = SIXENSE_BUTTON_2;
	}
	else if ( button == "3" )
	{
		*button_token = SIXENSE_BUTTON_3;
	}
	else if ( button == "4" )
	{
		*button_token = SIXENSE_BUTTON_4;
	}
	else if ( button == "start" )
	{
		*button_token = SIXENSE_BUTTON_START;
	}
	else if ( button == "bumper" )
	{
		*button_token = SIXENSE_BUTTON_BUMPER;
	}
	else if ( button == "joystick" )
	{
		*button_token = SIXENSE_BUTTON_JOYSTICK;
	}
	else
	{
		Msg( "Unknown button %s, shoud be '1' '2' '3' '4' 'start' 'trigger' or 'joystick'\n", button.String() );
		return false;
	}

	return true;
}

bool actionFromString( CUtlString action_str, sixenseUtils::IButtonStates::ActionType *action ) {
	if( action_str == "button_press" ) {
		*action = sixenseUtils::IButtonStates::ACTION_BUTTON_PRESS;
		return true;

	} else if( action_str == "trigger_press" ) {
		*action = sixenseUtils::IButtonStates::ACTION_TRIGGER_PRESS;
		return true;

	} else if( action_str == "tilt_gesture" ) {
		*action = sixenseUtils::IButtonStates::ACTION_TILT_GESTURE;
		return true;

	} else if( action_str == "point_gesture" ) {
		*action = sixenseUtils::IButtonStates::ACTION_POINT_GESTURE;
		return true;

	} else if( action_str == "velocity_gesture" ) {
		*action = sixenseUtils::IButtonStates::ACTION_VELOCITY_GESTURE;
		return true;

	} else if( action_str == "joystick_move" ) {
		*action = sixenseUtils::IButtonStates::ACTION_JOYSTICK_MOVE;
		return true;

	} else {
		Msg( "Unknown action %s, shoud be 'button_press' 'trigger_press' 'tilt_gesture' 'point_gesture' 'velocity_gesture' or 'joystick_move'\n", action_str.String() );
		*action = sixenseUtils::IButtonStates::ACTION_BUTTON_PRESS;
		return false;
	}
}

bool handFromString( CUtlString hand_str, int *hand ) {

	if( hand_str == "left" ) {
		*hand = 0;

	} else if( hand_str == "right" ) {
		*hand = 1;

	} else {
		Msg( "Unknown controller %s, should be 'left' or 'right'\n", hand_str.String() );
		return false;
	}

	return true;
}

bool SixenseInput::AreBindingsDisabled() 
{
	if( InMenuMode() ) 
	{
		return true;
	}

#ifdef TF_CLIENT_DLL
	CHudMenuSpyDisguise *pSpyMenu = ( CHudMenuSpyDisguise * )GET_HUDELEMENT( CHudMenuSpyDisguise );
	if( pSpyMenu->IsVisible() ) 
	{
		return true;
	}

	CHudMenuEngyBuild *pEngBuildMenu = ( CHudMenuEngyBuild * )GET_HUDELEMENT( CHudMenuEngyBuild );
	if( pEngBuildMenu->IsVisible() ) 
	{
		return true;
	}

	CHudMenuEngyDestroy *pEngDestroyMenu = ( CHudMenuEngyDestroy * )GET_HUDELEMENT( CHudMenuEngyDestroy );
	if( pEngDestroyMenu->IsVisible() ) 
	{
		return true;
	}
#endif

	return false;
}

// Console commands for controlling sixense_binds
static void SixenseBind( const CCommand &args )
{
	if ( args.ArgC() != 5 && args.ArgC() != 6 )
	{
		Msg( "Usage: sixense_bind <hand_left_or_right> <action> <argument> <on_press_command> [<on_release_command>]\n" );
		return;
	}

	CUtlString hand_str( args[1] ), action_str( args[2] ), argument_str( args[3] ), press_command( args[4] ), release_command;

	if( args.ArgC() == 6 )
	{
		release_command = args[5];
	}

	
	if( g_pSixenseInput->GetGestureBindings() ) 
	{
		g_pSixenseInput->GetGestureBindings()->AddBinding( hand_str, action_str, argument_str, press_command, release_command );
	}
}

static void SixenseListBindings( const CCommand &args )
{
	if ( args.ArgC() != 1 )
	{
		Msg( "Usage: sixense_list_bindings\n" );
		return;
	}

	if( g_pSixenseInput->GetGestureBindings() ) 
	{
		g_pSixenseInput->GetGestureBindings()->ListBindings();
	}
}

static void SixenseWriteBindings( const CCommand &args )
{
	if ( args.ArgC() != 1 && args.ArgC() != 2 )
	{
		Msg( "Usage: sixense_write_bindings [<filename>]\n" );
		return;
	}

	CUtlString filename;

	if( args.ArgC() == 2 ) 
	{
		filename = args[1];
	}

	if( g_pSixenseInput->GetGestureBindings() ) 
	{
		g_pSixenseInput->GetGestureBindings()->WriteBindings( filename );
	}
}

static void SixenseClearBindings( const CCommand &args )
{
	if( g_pSixenseInput->GetGestureBindings() ) 
	{
		g_pSixenseInput->GetGestureBindings()->ClearBindings();
	}
}

static void SixenseCreateDefaultBindings( const CCommand &args )
{
	if( g_pSixenseInput->GetGestureBindings() ) 
	{
		g_pSixenseInput->GetGestureBindings()->CreateDefaultBindings();
	}
}

static void SixenseDeleteBinding( const CCommand &args )
{
	if ( args.ArgC() != 2 )
	{
		Msg( "Usage: sixense_delete_binding <binding_number>\n" );
		return;
	}

	int num = atoi( args[1] );

	if( g_pSixenseInput->GetGestureBindings() ) 
	{
		g_pSixenseInput->GetGestureBindings()->DeleteBinding( num );
	}
}

static ConCommand sixense_bind_command( "sixense_bind", SixenseBind, "Bind a concommand to a button." );
static ConCommand sixense_list_bindings_cc( "sixense_list_bindings", SixenseListBindings, "List the sixense bindings." );
static ConCommand sixense_write_bindings_cc( "sixense_write_bindings", SixenseWriteBindings, "Save the sixense bindings to a file." );
static ConCommand sixense_clear_bindings_cc( "sixense_clear_bindings", SixenseClearBindings, "Clear all sixense bindings." );
static ConCommand sixense_delete_binding_cc( "sixense_delete_binding", SixenseDeleteBinding, "Delete a single binding by index." );
static ConCommand sixense_create_default_binding_cc( "sixense_create_default_bindings", SixenseCreateDefaultBindings, "Erase all current bindings and load the default bindings for this game." );





//////

void SelectMachinegun() 
{

	GetHudWeaponSelection()->SelectSlot(2);
}

ConCommand sixense_select_machinegun( "sixense_select_machinegun", SelectMachinegun );

//////


void SixenseInput::StartRatchet() 
{
	if( m_pFPSViewAngles )
	{
		m_pFPSViewAngles->setRatcheting( true );
	}
}

void StartRatchet() 
{
		g_pSixenseInput->StartRatchet();
	}

ConCommand sixense_start_ratchet( "+sixense_ratchet", StartRatchet );

//////

void SixenseInput::StopRatchet()
{
	if( m_pFPSViewAngles )
	{
		m_pFPSViewAngles->setRatcheting( false );
	}
}

void StopRatchet() 
{
		g_pSixenseInput->StopRatchet();
	}

ConCommand sixense_stop_ratchet( "-sixense_ratchet", StopRatchet );

//////

void SelectPistol() 
{
	GetHudWeaponSelection()->SelectSlot(1);
}

ConCommand sixense_select_pistol( "sixense_select_pistol", SelectPistol );

//////

void SelectGrenade() 
{
#ifdef CSTRIKE15
	GetHudWeaponSelection()->CycleToNextGrenadeOrBomb();
#endif
}

ConCommand sixense_select_grenade( "sixense_select_grenade", SelectGrenade );

//////

void SelectMelee() 
{
	GetHudWeaponSelection()->SelectSlot(3);
}

ConCommand sixense_select_melee( "sixense_select_melee", SelectMelee );

//////

void SixenseInput::LeftPointGesture( bool start ) 
{
	if( start )
		m_pLeftButtonStates->startPointGesture();
	else
		m_pLeftButtonStates->stopPointGesture();

}

//////

void SixenseInput::RightPointGesture( bool start )
{
	if( start )
		m_pRightButtonStates->startPointGesture();
	else
		m_pRightButtonStates->stopPointGesture();

}

//////

void StartLeftPointGesture() 
{
		g_pSixenseInput->LeftPointGesture( true );
	}

ConCommand sixense_start_left_point_gesture( "+sixense_left_point_gesture", StartLeftPointGesture );

//////

void StopLeftPointGesture() 
{
		g_pSixenseInput->LeftPointGesture( false );
	}

ConCommand sixense_stop_left_point_gesture( "-sixense_left_point_gesture", StopLeftPointGesture );

//////

void StartRightPointGesture() 
{
		g_pSixenseInput->RightPointGesture( true );
	}

ConCommand sixense_start_right_point_gesture( "+sixense_right_point_gesture", StartRightPointGesture );

//////

void StopRightPointGesture() 
{
		g_pSixenseInput->RightPointGesture( false );
	}

ConCommand sixense_stop_right_point_gesture( "-sixense_right_point_gesture", StopRightPointGesture );

#endif //SIXENSE
