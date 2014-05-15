//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Joystick handling function
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "basehandle.h"
#include "utlvector.h"
#include "cdll_client_int.h"
#include "cdll_util.h"
#include "kbutton.h"
#include "usercmd.h"
#include "iclientvehicle.h"
#include "input.h"
#include "iviewrender.h"
#include "convar.h"
#include "hud.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "vgui/Cursor.h"
#include "tier0/icommandline.h"
#include "inputsystem/iinputsystem.h"
#include "inputsystem/ButtonCode.h"
#include "math.h"
#include "tier1/convar_serverbounded.h"
#include "cam_thirdperson.h"

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#else
#include "../common/xbox/xboxstubs.h"
#endif

#ifdef HL2_CLIENT_DLL
// FIXME: Autoaim support needs to be moved from HL2_DLL to the client dll, so this include should be c_baseplayer.h
#include "c_basehlplayer.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Control like a joystick
#define JOY_ABSOLUTE_AXIS	0x00000000		
// Control like a mouse, spinner, trackball
#define JOY_RELATIVE_AXIS	0x00000010		

// Axis mapping
static ConVar joy_name( "joy_name", "joystick", FCVAR_ARCHIVE );
static ConVar joy_advanced( "joy_advanced", "1", FCVAR_ARCHIVE );
static ConVar joy_advaxisx( "joy_advaxisx", "4", FCVAR_ARCHIVE );
static ConVar joy_advaxisy( "joy_advaxisy", "2", FCVAR_ARCHIVE );
static ConVar joy_advaxisz( "joy_advaxisz", "0", FCVAR_ARCHIVE );
static ConVar joy_advaxisr( "joy_advaxisr", "1", FCVAR_ARCHIVE );
static ConVar joy_advaxisu( "joy_advaxisu", "3", FCVAR_ARCHIVE );
static ConVar joy_advaxisv( "joy_advaxisv", "0", FCVAR_ARCHIVE );

// Basic "dead zone" and sensitivity
static ConVar joy_forwardthreshold( "joy_forwardthreshold", "0.15", FCVAR_ARCHIVE );
static ConVar joy_sidethreshold( "joy_sidethreshold", "0.15", FCVAR_ARCHIVE );
static ConVar joy_pitchthreshold( "joy_pitchthreshold", "0.15", FCVAR_ARCHIVE );
static ConVar joy_yawthreshold( "joy_yawthreshold", "0.15", FCVAR_ARCHIVE );
static ConVar joy_forwardsensitivity( "joy_forwardsensitivity", "-1", FCVAR_ARCHIVE );
static ConVar joy_sidesensitivity( "joy_sidesensitivity", "1", FCVAR_ARCHIVE );
static ConVar joy_pitchsensitivity( "joy_pitchsensitivity", "1", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );
static ConVar joy_yawsensitivity( "joy_yawsensitivity", "-1", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );

// Advanced sensitivity and response
static ConVar joy_response_move( "joy_response_move", "1", FCVAR_ARCHIVE, "'Movement' stick response mode: 0=Linear, 1=quadratic, 2=cubic, 3=quadratic extreme, 4=power function(i.e., pow(x,1/sensitivity)), 5=two-stage" );
ConVar joy_response_move_vehicle("joy_response_move_vehicle", "6");
static ConVar joy_response_look( "joy_response_look", "0", FCVAR_ARCHIVE, "'Look' stick response mode: 0=Default, 1=Acceleration Promotion" );
static ConVar joy_lowend( "joy_lowend", "1", FCVAR_ARCHIVE );
static ConVar joy_lowmap( "joy_lowmap", "1", FCVAR_ARCHIVE );
static ConVar joy_accelscale( "joy_accelscale", "0.6", FCVAR_ARCHIVE);
static ConVar joy_accelmax( "joy_accelmax", "1.0", FCVAR_ARCHIVE);
static ConVar joy_autoaimdampenrange( "joy_autoaimdampenrange", "0", FCVAR_ARCHIVE, "The stick range where autoaim dampening is applied. 0 = off" );
static ConVar joy_autoaimdampen( "joy_autoaimdampen", "0", FCVAR_ARCHIVE, "How much to scale user stick input when the gun is pointing at a valid target." );

static ConVar joy_vehicle_turn_lowend("joy_vehicle_turn_lowend", "0.7");
static ConVar joy_vehicle_turn_lowmap("joy_vehicle_turn_lowmap", "0.4");


// Misc
static ConVar joy_diagonalpov( "joy_diagonalpov", "0", FCVAR_ARCHIVE, "POV manipulator operates on diagonal axes, too." );
static ConVar joy_display_input("joy_display_input", "0", FCVAR_ARCHIVE);
static ConVar joy_wwhack2( "joy_wingmanwarrior_turnhack", "0", FCVAR_ARCHIVE, "Wingman warrior hack related to turn axes." );
ConVar joy_autosprint("joy_autosprint", "0", 0, "Automatically sprint when moving with an analog joystick" );

static ConVar joy_inverty("joy_inverty", "0", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX, "Whether to invert the Y axis of the joystick for looking." );

// XBox Defaults
static ConVar joy_yawsensitivity_default( "joy_yawsensitivity_default", "-1.25", FCVAR_NONE );
static ConVar joy_pitchsensitivity_default( "joy_pitchsensitivity_default", "-1.0", FCVAR_NONE );
static ConVar option_duck_method_default( "option_duck_method_default", "1.0", FCVAR_NONE );
static ConVar joy_inverty_default( "joy_inverty_default", "0", FCVAR_ARCHIVE_XBOX );				// Extracted & saved from profile
static ConVar joy_movement_stick_default( "joy_movement_stick_default", "0", FCVAR_ARCHIVE_XBOX );	// Extracted & saved from profile
static ConVar sv_stickysprint_default( "sv_stickysprint_default", "0", FCVAR_NONE );

void joy_movement_stick_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	engine->ClientCmd( "joyadvancedupdate" );
}
static ConVar joy_movement_stick("joy_movement_stick", "0", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX, "Which stick controls movement (0 is left stick)", joy_movement_stick_Callback );

static ConVar joy_xcontroller_cfg_loaded( "joy_xcontroller_cfg_loaded", "0", FCVAR_ARCHIVE, "If 0, the 360controller.cfg file will be executed on startup & option changes." );

extern ConVar lookspring;
extern ConVar cl_forwardspeed;
extern ConVar lookstrafe;
extern ConVar in_joystick;
extern ConVar_ServerBounded *m_pitch;
extern ConVar l_pitchspeed;
extern ConVar cl_sidespeed;
extern ConVar cl_yawspeed;
extern ConVar cl_pitchdown;
extern ConVar cl_pitchup;
extern ConVar cl_pitchspeed;

extern ConVar cam_idealpitch;
extern ConVar cam_idealyaw;
extern ConVar thirdperson_platformer;
extern ConVar thirdperson_screenspace;

//-----------------------------------------------------------------
// Purpose: Returns true if there's an active joystick connected.
//-----------------------------------------------------------------
bool CInput::EnableJoystickMode()
{
	return IsConsole() || in_joystick.GetBool();
}


//-----------------------------------------------
// Response curve function for the move axes
//-----------------------------------------------
static float ResponseCurve( int curve, float x, int axis, float sensitivity )
{
	switch ( curve )
	{
	case 1:
		// quadratic
		if ( x < 0 )
			return -(x*x) * sensitivity;
		return x*x * sensitivity;

	case 2:
		// cubic
		return x*x*x*sensitivity;

	case 3:
		{
		// quadratic extreme
		float extreme = 1.0f;
		if ( fabs( x ) >= 0.95f )
		{
			extreme = 1.5f;
		}
		if ( x < 0 )
			return -extreme * x*x*sensitivity;
		return extreme * x*x*sensitivity;
		}
	case 4:
		{
			float flScale = sensitivity < 0.0f ? -1.0f : 1.0f;

			sensitivity = clamp( fabs( sensitivity ), 1.0e-8f, 1000.0f );

			float oneOverSens = 1.0f / sensitivity;
		
			if ( x < 0.0f )
			{
				flScale = -flScale;
			}

			float retval = clamp( powf( fabs( x ), oneOverSens ), 0.0f, 1.0f );
			return retval * flScale;
		}
		break;
	case 5:
		{
			float out = x;

			if( fabs(out) <= 0.6f )
			{
				out *= 0.5f;
			}

			out = out * sensitivity;
			return out;
		}
		break;
	case 6: // Custom for driving a vehicle!
		{
			if( axis == YAW )
			{
				// This code only wants to affect YAW axis (the left and right axis), which 
				// is used for turning in the car. We fall-through and use a linear curve on 
				// the PITCH axis, which is the vehicle's throttle. REALLY, these are the 'forward'
				// and 'side' axes, but we don't have constants for those, so we re-use the same
				// axis convention as the look stick. (sjb)
				float sign = 1;

				if( x  < 0.0 )
					sign = -1;

				x = fabs(x);

				if( x <= joy_vehicle_turn_lowend.GetFloat() )
					x = RemapVal( x, 0.0f, joy_vehicle_turn_lowend.GetFloat(), 0.0f, joy_vehicle_turn_lowmap.GetFloat() );
				else
					x = RemapVal( x, joy_vehicle_turn_lowend.GetFloat(), 1.0f, joy_vehicle_turn_lowmap.GetFloat(), 1.0f );

				return x * sensitivity * sign;
			}
			//else
			//	fall through and just return x*sensitivity below (as if using default curve)
		}
	}

	// linear
	return x*sensitivity;
}


//-----------------------------------------------
// If we have a valid autoaim target, dampen the 
// player's stick input if it is moving away from
// the target.
//
// This assists the player staying on target.
//-----------------------------------------------
float AutoAimDampening( float x, int axis, float dist )
{
	// FIXME: Autoaim support needs to be moved from HL2_DLL to the client dll, so all games can use it.
#ifdef HL2_CLIENT_DLL
	// Help the user stay on target if the feature is enabled and the user
	// is not making a gross stick movement.
	if( joy_autoaimdampen.GetFloat() > 0.0f && fabs(x) < joy_autoaimdampenrange.GetFloat() )
	{
		// Get the HL2 player
		C_BaseHLPlayer *pLocalPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();

		if( pLocalPlayer )
		{
			// Get the autoaim target
			if( pLocalPlayer->m_HL2Local.m_bAutoAimTarget )
			{
				return joy_autoaimdampen.GetFloat();
			}
		}
	}
#endif
	return 1.0f;// No dampening.
}


//-----------------------------------------------
// This structure holds persistent information used
// to make decisions about how to modulate analog
// stick input.
//-----------------------------------------------
typedef struct 
{
	float	envelopeScale[2];
	bool	peggedAxis[2];
	bool	axisPeggedDir[2];
} envelope_t;

envelope_t	controlEnvelope;

//-----------------------------------------------
// Response curve function specifically for the 
// 'look' analog stick.
//
// when AXIS == YAW, otherAxisValue contains the 
// value for the pitch of the control stick, and
// vice-versa.
//-----------------------------------------------
ConVar joy_pegged("joy_pegged", "0.75");// Once the stick is pushed this far, it's assumed pegged.
ConVar joy_virtual_peg("joy_virtual_peg", "0");
static float ResponseCurveLookDefault( float x, int axis, float otherAxis, float dist, float frametime )
{
	float input = x;

	bool bStickIsPhysicallyPegged = ( dist >= joy_pegged.GetFloat() );

	// Make X positive to make things easier, just remember whether we have to flip it back!
	bool negative = false;
	if( x < 0.0f )
	{
		negative = true;
		x *= -1;
	}

	if( axis == YAW && joy_virtual_peg.GetBool() )
	{
		if( x >= 0.95f )
		{
			// User has pegged the stick
			controlEnvelope.peggedAxis[axis] = true;
			controlEnvelope.axisPeggedDir[axis] = negative;
		}
		
		if( controlEnvelope.peggedAxis[axis] == true )
		{
			// User doesn't have the stick pegged on this axis, but they used to. 
			// If the stick is physically pegged, pretend this axis is still pegged.
			if( bStickIsPhysicallyPegged && negative == controlEnvelope.axisPeggedDir[axis] )
			{
				// If the user still has the stick physically pegged and hasn't changed direction on
				// this axis, keep pretending they have the stick pegged on this axis.
				x = 1.0f;
			}
			else
			{
				controlEnvelope.peggedAxis[axis] = false;
			}
		}
	}

	// Perform the two-stage mapping.
	if( x > joy_lowend.GetFloat() )
	{
		float highmap = 1.0f - joy_lowmap.GetFloat();
		float xNormal = x - joy_lowend.GetFloat();

		float factor = xNormal / ( 1.0f - joy_lowend.GetFloat() );
		x = joy_lowmap.GetFloat() + (highmap * factor);

		// Accelerate.
		if( controlEnvelope.envelopeScale[axis] < 1.0f )
		{
			controlEnvelope.envelopeScale[axis] += ( frametime * joy_accelscale.GetFloat() );
			if( controlEnvelope.envelopeScale[axis] > 1.0f )
			{
				controlEnvelope.envelopeScale[axis] = 1.0f;
			}
		}

		float delta = x - joy_lowmap.GetFloat();
		x = joy_lowmap.GetFloat() + (delta * controlEnvelope.envelopeScale[axis]);
	}
	else
	{
		// Shut off acceleration
		controlEnvelope.envelopeScale[axis] = 0.0f;
		float factor = x / joy_lowend.GetFloat();
		x = joy_lowmap.GetFloat() * factor;
	}

	x *= AutoAimDampening( input, axis, dist );

	if( axis == YAW && x > 0.0f && joy_display_input.GetBool() )
	{
		Msg("In:%f Out:%f Frametime:%f\n", input, x, frametime );
	}

	if( negative )
	{
		x *= -1;
	}

	return x;
}

ConVar joy_accel_filter("joy_accel_filter", "0.2");// If the non-accelerated axis is pushed farther than this, then accelerate it, too.
static float ResponseCurveLookAccelerated( float x, int axis, float otherAxis, float dist, float frametime )
{
	float input = x;

	float flJoyDist = ( sqrt(x*x + otherAxis * otherAxis) );
	bool bIsPegged = ( flJoyDist>= joy_pegged.GetFloat() );

	// Make X positive to make arithmetic easier for the rest of this function, and
	// remember whether we have to flip it back!
	bool negative = false;
	if( x < 0.0f )
	{
		negative = true;
		x *= -1;
	}

	// Perform the two-stage mapping.
	bool bDoAcceleration = false;// Assume we won't accelerate the input

	if( bIsPegged && x > joy_accel_filter.GetFloat() )
	{
		// Accelerate this axis, since the stick is pegged and 
		// this axis is pressed farther than the acceleration filter
		// Take the lowmap value, or the input, whichever is higher, since 
		// we don't necesarily know whether this is the axis which is pegged
		x = MAX( joy_lowmap.GetFloat(), x );
		bDoAcceleration = true;
	}
	else
	{
		// Joystick is languishing in the low-end, turn off acceleration.
		controlEnvelope.envelopeScale[axis] = 0.0f;
		float factor = x / joy_lowend.GetFloat();
		x = joy_lowmap.GetFloat() * factor;
	}

	if( bDoAcceleration )
	{
		float flMax = joy_accelmax.GetFloat();
		if( controlEnvelope.envelopeScale[axis] < flMax )
		{
			float delta = x - joy_lowmap.GetFloat();
			x = joy_lowmap.GetFloat() + (delta * controlEnvelope.envelopeScale[axis]);
			controlEnvelope.envelopeScale[axis] += ( frametime * joy_accelscale.GetFloat() );

			if( controlEnvelope.envelopeScale[axis] > flMax )
			{
				controlEnvelope.envelopeScale[axis] = flMax;
			}
		}
	}

	x *= AutoAimDampening( input, axis, dist );

	if( axis == YAW && input != 0.0f && joy_display_input.GetBool() )
	{
		Msg("In:%f Out:%f Frametime:%f\n", input, x, frametime );
	}

	if( negative )
	{
		x *= -1;
	}

	return x;
}

//-----------------------------------------------
//-----------------------------------------------
static float ResponseCurveLook( int curve, float x, int axis, float otherAxis, float dist, float frametime )
{
	switch( curve )
	{
	case 1://Promotion of acceleration
		return ResponseCurveLookAccelerated( x, axis, otherAxis, dist, frametime );
		break;

	default:
		return ResponseCurveLookDefault( x, axis, otherAxis, dist, frametime );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Advanced joystick setup
//-----------------------------------------------------------------------------
void CInput::Joystick_Advanced(void)
{
	// called whenever an update is needed
	int	i;
	DWORD dwTemp;

	if ( IsX360() )
	{
		// Xbox always uses a joystick
		in_joystick.SetValue( 1 );
	}

	// Initialize all the maps
	for ( i = 0; i < MAX_JOYSTICK_AXES; i++ )
	{
		m_rgAxes[i].AxisMap = GAME_AXIS_NONE;
		m_rgAxes[i].ControlMap = JOY_ABSOLUTE_AXIS;
	}

	if ( !joy_advanced.GetBool() )
	{
		// default joystick initialization
		// 2 axes only with joystick control
		m_rgAxes[JOY_AXIS_X].AxisMap = GAME_AXIS_YAW;
		m_rgAxes[JOY_AXIS_Y].AxisMap = GAME_AXIS_FORWARD;
	}
	else
	{
		if ( Q_stricmp( joy_name.GetString(), "joystick") != 0 )
		{
			// notify user of advanced controller
			Msg( "Using joystick '%s' configuration\n", joy_name.GetString() );
		}

		// advanced initialization here
		// data supplied by user via joy_axisn cvars
		dwTemp = ( joy_movement_stick.GetBool() ) ? (DWORD)joy_advaxisu.GetInt() : (DWORD)joy_advaxisx.GetInt();
		m_rgAxes[JOY_AXIS_X].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_X].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_X", &m_rgAxes[JOY_AXIS_X] );

		dwTemp = ( joy_movement_stick.GetBool() ) ? (DWORD)joy_advaxisr.GetInt() : (DWORD)joy_advaxisy.GetInt();
		m_rgAxes[JOY_AXIS_Y].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_Y].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_Y", &m_rgAxes[JOY_AXIS_Y] );

		dwTemp = (DWORD)joy_advaxisz.GetInt();
		m_rgAxes[JOY_AXIS_Z].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_Z].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_Z", &m_rgAxes[JOY_AXIS_Z] );

		dwTemp = ( joy_movement_stick.GetBool() ) ? (DWORD)joy_advaxisy.GetInt() : (DWORD)joy_advaxisr.GetInt();
		m_rgAxes[JOY_AXIS_R].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_R].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_R", &m_rgAxes[JOY_AXIS_R] );

		dwTemp = ( joy_movement_stick.GetBool() ) ? (DWORD)joy_advaxisx.GetInt() : (DWORD)joy_advaxisu.GetInt();
		m_rgAxes[JOY_AXIS_U].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_U].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_U", &m_rgAxes[JOY_AXIS_U] );

		dwTemp = (DWORD)joy_advaxisv.GetInt();
		m_rgAxes[JOY_AXIS_V].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_V].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_V", &m_rgAxes[JOY_AXIS_V] );

		Msg( "Advanced Joystick settings initialized\n" );
	}

	// If we have an xcontroller, load the cfg file if it hasn't been loaded.
	static ConVarRef var( "joy_xcontroller_found" );
	if ( var.IsValid() && var.GetBool() && in_joystick.GetBool() )
	{
		if ( joy_xcontroller_cfg_loaded.GetInt() < 2 )
		{
			engine->ClientCmd_Unrestricted( "exec 360controller.cfg" );
			if ( IsLinux () )
			{
				engine->ClientCmd_Unrestricted( "exec 360controller-linux.cfg" );
			}
			joy_xcontroller_cfg_loaded.SetValue( 2 );
		}
	}
	else if ( joy_xcontroller_cfg_loaded.GetInt() > 0 )
	{
		engine->ClientCmd_Unrestricted( "exec undo360controller.cfg" );
		joy_xcontroller_cfg_loaded.SetValue( 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CInput::DescribeAxis( int index )
{
	switch ( index )
	{
	case GAME_AXIS_FORWARD:
		return "Forward";
	case GAME_AXIS_PITCH:
		return "Look";
	case GAME_AXIS_SIDE:
		return "Side";
	case GAME_AXIS_YAW:
		return "Turn";
	case GAME_AXIS_NONE:
	default:
		return "Unknown";
	}

	return "Unknown";
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *axis - 
//			*mapping - 
//-----------------------------------------------------------------------------
void CInput::DescribeJoystickAxis( char const *axis, joy_axis_t *mapping )
{
	if ( !mapping->AxisMap )
	{
		Msg( "%s:  unmapped\n", axis );
	}
	else
	{
		Msg( "%s:  mapped to %s (%s)\n",
			axis, 
			DescribeAxis( mapping->AxisMap ),
			mapping->ControlMap != 0 ? "relative" : "absolute" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Allow joystick to issue key events
// Not currently used - controller button events are pumped through the windprocs. KWD
//-----------------------------------------------------------------------------
void CInput::ControllerCommands( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: Scales the raw analog value to lie withing the axis range (full range - deadzone )
//-----------------------------------------------------------------------------
float CInput::ScaleAxisValue( const float axisValue, const float axisThreshold )
{
	// Xbox scales the range of all axes in the inputsystem. PC can't do that because each axis mapping
	// has a (potentially) unique threshold value.  If all axes were restricted to a single threshold
	// as they are on the Xbox, this function could move to inputsystem and be slightly more optimal.
	float result = 0.f;
	if ( IsPC() )
	{
		if ( axisValue < -axisThreshold )
		{
			result = ( axisValue + axisThreshold ) / ( MAX_BUTTONSAMPLE - axisThreshold );
		}
		else if ( axisValue > axisThreshold )
		{
			result = ( axisValue - axisThreshold ) / ( MAX_BUTTONSAMPLE - axisThreshold );
		}
	}
	else
	{
		// IsXbox
		result =  axisValue * ( 1.f / MAX_BUTTONSAMPLE );
	}

	return result;
}


void CInput::Joystick_SetSampleTime(float frametime)
{
	m_flRemainingJoystickSampleTime = frametime;
}

float CInput::Joystick_GetForward( void )
{
	return m_flPreviousJoystickForward;
}

float CInput::Joystick_GetSide( void )
{
	return m_flPreviousJoystickSide;
}

float CInput::Joystick_GetPitch( void )
{
	return m_flPreviousJoystickPitch;
}

float CInput::Joystick_GetYaw( void )
{
	return m_flPreviousJoystickYaw;
}

//-----------------------------------------------------------------------------
// Purpose: Apply joystick to CUserCmd creation
// Input  : frametime - 
//			*cmd - 
//-----------------------------------------------------------------------------
void CInput::JoyStickMove( float frametime, CUserCmd *cmd )
{
	// complete initialization if first time in ( needed as cvars are not available at initialization time )
	if ( !m_fJoystickAdvancedInit )
	{
		Joystick_Advanced();
		m_fJoystickAdvancedInit = true;
	}

	// Verify that the user wants to use the joystick
	if ( !in_joystick.GetInt() )
		return;

	// Reinitialize the 'advanced joystick' system if hotplugging has caused us toggle between some/none joysticks.
	bool haveJoysticks = ( inputsystem->GetJoystickCount() > 0 );
	if ( haveJoysticks != m_fHadJoysticks )
	{
		Joystick_Advanced();
		m_fHadJoysticks = haveJoysticks;
	}

	// Verify that a joystick is available
	if ( !haveJoysticks )
		return; 

	if ( m_flRemainingJoystickSampleTime <= 0 )
		return;
	frametime = MIN(m_flRemainingJoystickSampleTime, frametime);
	m_flRemainingJoystickSampleTime -= frametime;

	QAngle viewangles;

	// Get starting angles
	engine->GetViewAngles( viewangles );

	struct axis_t
	{
		float	value;
		int		controlType;
	};
	axis_t gameAxes[ MAX_GAME_AXES ];
	memset( &gameAxes, 0, sizeof(gameAxes) );

	// Get each joystick axis value, and normalize the range
	for ( int i = 0; i < MAX_JOYSTICK_AXES; ++i )
	{
		if ( GAME_AXIS_NONE == m_rgAxes[i].AxisMap )
			continue;

		float fAxisValue = inputsystem->GetAnalogValue( (AnalogCode_t)JOYSTICK_AXIS( 0, i ) );

		if (joy_wwhack2.GetInt() != 0 )
		{
			// this is a special formula for the Logitech WingMan Warrior
			// y=ax^b; where a = 300 and b = 1.3
			// also x values are in increments of 800 (so this is factored out)
			// then bounds check result to level out excessively high spin rates
			float fTemp = 300.0 * pow(abs(fAxisValue) / 800.0, 1.3);
			if (fTemp > 14000.0)
				fTemp = 14000.0;
			// restore direction information
			fAxisValue = (fAxisValue > 0.0) ? fTemp : -fTemp;
		}

		unsigned int idx = m_rgAxes[i].AxisMap;
		gameAxes[idx].value = fAxisValue;
		gameAxes[idx].controlType = m_rgAxes[i].ControlMap;
	}

	// Re-map the axis values if necessary, based on the joystick configuration
	if ( (joy_advanced.GetInt() == 0) && (in_jlook.state & 1) )
	{
		// user wants forward control to become pitch control
		gameAxes[GAME_AXIS_PITCH] = gameAxes[GAME_AXIS_FORWARD];
		gameAxes[GAME_AXIS_FORWARD].value = 0;

		// if mouse invert is on, invert the joystick pitch value
		// Note: only absolute control support here - joy_advanced = 0
		if ( m_pitch->GetFloat() < 0.0 )
		{
			gameAxes[GAME_AXIS_PITCH].value *= -1;
		}
	}

	if ( (in_strafe.state & 1) || ( lookstrafe.GetFloat() && (in_jlook.state & 1) ) )
	{
		// user wants yaw control to become side control
		gameAxes[GAME_AXIS_SIDE] = gameAxes[GAME_AXIS_YAW];
		gameAxes[GAME_AXIS_YAW].value = 0;
	}

	m_flPreviousJoystickForward	= ScaleAxisValue( gameAxes[GAME_AXIS_FORWARD].value, MAX_BUTTONSAMPLE * joy_forwardthreshold.GetFloat() );
	m_flPreviousJoystickSide	= ScaleAxisValue( gameAxes[GAME_AXIS_SIDE].value, MAX_BUTTONSAMPLE * joy_sidethreshold.GetFloat()  );
	m_flPreviousJoystickPitch	= ScaleAxisValue( gameAxes[GAME_AXIS_PITCH].value, MAX_BUTTONSAMPLE * joy_pitchthreshold.GetFloat()  );
	m_flPreviousJoystickYaw		= ScaleAxisValue( gameAxes[GAME_AXIS_YAW].value, MAX_BUTTONSAMPLE * joy_yawthreshold.GetFloat()  );

	// Skip out if vgui is active
	if ( vgui::surface()->IsCursorVisible() )
		return;

	// If we're inverting our joystick, do so
	if ( joy_inverty.GetBool() )
	{
		m_flPreviousJoystickPitch *= -1.0f;
	}

	// drive yaw, pitch and move like a screen relative platformer game
	if ( CAM_IsThirdPerson() && thirdperson_platformer.GetInt() )
	{
		if ( m_flPreviousJoystickForward || m_flPreviousJoystickSide )
		{
			// apply turn control [ YAW ]
			// factor in the camera offset, so that the move direction is relative to the thirdperson camera
			viewangles[ YAW ] = RAD2DEG(atan2(-m_flPreviousJoystickSide, -m_flPreviousJoystickForward)) + g_ThirdPersonManager.GetCameraOffsetAngles()[ YAW ];
			engine->SetViewAngles( viewangles );

			// apply movement
			Vector2D moveDir( m_flPreviousJoystickForward, m_flPreviousJoystickSide );
			cmd->forwardmove += moveDir.Length() * cl_forwardspeed.GetFloat();
		}

		if ( m_flPreviousJoystickPitch || m_flPreviousJoystickYaw )
		{
			Vector vTempOffset = g_ThirdPersonManager.GetCameraOffsetAngles();

			// look around with the camera
			vTempOffset[ PITCH ] += m_flPreviousJoystickPitch * joy_pitchsensitivity.GetFloat();
			vTempOffset[ YAW ]   += m_flPreviousJoystickYaw * joy_yawsensitivity.GetFloat();

			g_ThirdPersonManager.SetCameraOffsetAngles( vTempOffset );
		}

		if ( m_flPreviousJoystickForward || m_flPreviousJoystickSide || m_flPreviousJoystickPitch || m_flPreviousJoystickYaw )
		{
			Vector vTempOffset = g_ThirdPersonManager.GetCameraOffsetAngles();

			// update the ideal pitch and yaw
			cam_idealpitch.SetValue( vTempOffset[ PITCH ] - viewangles[ PITCH ] );
			cam_idealyaw.SetValue( vTempOffset[ YAW ] - viewangles[ YAW ] );
		}
		return;
	}

	float	joySideMove = 0.f;
	float	joyForwardMove = 0.f;
	float   aspeed = frametime * gHUD.GetFOVSensitivityAdjust();

	// apply forward and side control
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	
	int iResponseCurve = 0;
	if ( pLocalPlayer && pLocalPlayer->IsInAVehicle() )
	{
		iResponseCurve = pLocalPlayer->GetVehicle() ? pLocalPlayer->GetVehicle()->GetJoystickResponseCurve() : joy_response_move_vehicle.GetInt();
	}
	else
	{
		iResponseCurve = joy_response_move.GetInt();
	}	
	
	float val = ResponseCurve( iResponseCurve, m_flPreviousJoystickForward, PITCH, joy_forwardsensitivity.GetFloat() );
	joyForwardMove	+= val * cl_forwardspeed.GetFloat();
	val = ResponseCurve( iResponseCurve, m_flPreviousJoystickSide, YAW, joy_sidesensitivity.GetFloat() );
	joySideMove		+= val * cl_sidespeed.GetFloat();

	Vector2D move( m_flPreviousJoystickYaw, m_flPreviousJoystickPitch );
	float dist = move.Length();

	// apply turn control
	float angle = 0.f;

	if ( JOY_ABSOLUTE_AXIS == gameAxes[GAME_AXIS_YAW].controlType )
	{
		float fAxisValue = ResponseCurveLook( joy_response_look.GetInt(), m_flPreviousJoystickYaw, YAW, m_flPreviousJoystickPitch, dist, frametime );
		angle = fAxisValue * joy_yawsensitivity.GetFloat() * aspeed * cl_yawspeed.GetFloat();
	}
	else
	{
		angle = m_flPreviousJoystickYaw * joy_yawsensitivity.GetFloat() * aspeed * 180.0;
	}

	angle = JoyStickAdjustYaw( angle );
	viewangles[YAW] += angle;
	cmd->mousedx = angle;

	// apply look control
	if ( IsX360() || in_jlook.state & 1 )
	{
		float angle = 0;
		if ( JOY_ABSOLUTE_AXIS == gameAxes[GAME_AXIS_PITCH].controlType )
		{
			float fAxisValue = ResponseCurveLook( joy_response_look.GetInt(), m_flPreviousJoystickPitch, PITCH, m_flPreviousJoystickYaw, dist, frametime );
			angle = fAxisValue * joy_pitchsensitivity.GetFloat() * aspeed * cl_pitchspeed.GetFloat();
		}
		else
		{
			angle = m_flPreviousJoystickPitch * joy_pitchsensitivity.GetFloat() * aspeed * 180.0;
		}
		viewangles[PITCH] += angle;
		cmd->mousedy = angle;
		view->StopPitchDrift();
		if( m_flPreviousJoystickPitch == 0.f && lookspring.GetFloat() == 0.f )
		{
			// no pitch movement
			// disable pitch return-to-center unless requested by user
			// *** this code can be removed when the lookspring bug is fixed
			// *** the bug always has the lookspring feature on
			view->StopPitchDrift();
		}
	}

	// apply player motion relative to screen space
	if ( CAM_IsThirdPerson() && thirdperson_screenspace.GetInt() )
	{
		float ideal_yaw = cam_idealyaw.GetFloat();
		float ideal_sin = sin(DEG2RAD(ideal_yaw));
		float ideal_cos = cos(DEG2RAD(ideal_yaw));
		float side_movement = ideal_cos*joySideMove - ideal_sin*joyForwardMove;
		float forward_movement = ideal_cos*joyForwardMove + ideal_sin*joySideMove;
		cmd->forwardmove += forward_movement;
		cmd->sidemove += side_movement;
	}
	else
	{
		cmd->forwardmove += joyForwardMove;
		cmd->sidemove += joySideMove;
	}

	if ( IsPC() )
	{
		CCommand tmp;
		if ( FloatMakePositive(joyForwardMove) >= joy_autosprint.GetFloat() || FloatMakePositive(joySideMove) >= joy_autosprint.GetFloat() )
		{
			KeyDown( &in_joyspeed, NULL );
		}
		else
		{
			KeyUp( &in_joyspeed, NULL );
		}
	}

	// Bound pitch
	viewangles[PITCH] = clamp( viewangles[ PITCH ], -cl_pitchup.GetFloat(), cl_pitchdown.GetFloat() );

	engine->SetViewAngles( viewangles );
}
