//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef INPUTENUMS_H
#define INPUTENUMS_H
#ifdef _WIN32
#pragma once
#endif

// Standard maximum +/- value of a joystick axis
#define MAX_BUTTONSAMPLE			32768

#if !defined( _X360 )
#define INVALID_USER_ID		-1
#else
#define INVALID_USER_ID		XBX_INVALID_USER_ID
#endif

//-----------------------------------------------------------------------------
// Forward declarations: 
//-----------------------------------------------------------------------------

enum
{
	MAX_JOYSTICKS = 1,
	MOUSE_BUTTON_COUNT = 5,
	MAX_NOVINT_DEVICES = 2,
};

#if defined( LINUX )
// Linux has a slightly different mapping order on the joystick axes
enum JoystickAxis_t
{
	JOY_AXIS_X = 0,
 	JOY_AXIS_Y,
	JOY_AXIS_Z,
	JOY_AXIS_U,
	JOY_AXIS_R,
	JOY_AXIS_V,
	MAX_JOYSTICK_AXES,
};
#else
enum JoystickAxis_t
{
	JOY_AXIS_X = 0,
 	JOY_AXIS_Y,
	JOY_AXIS_Z,
	JOY_AXIS_R,
	JOY_AXIS_U,
	JOY_AXIS_V,
	MAX_JOYSTICK_AXES,
};
#endif

//-----------------------------------------------------------------------------
// Extra mouse codes
//-----------------------------------------------------------------------------
enum
{
	MS_WM_XBUTTONDOWN	= 0x020B,
	MS_WM_XBUTTONUP		= 0x020C,
	MS_WM_XBUTTONDBLCLK	= 0x020D,
	MS_MK_BUTTON4		= 0x0020,
	MS_MK_BUTTON5		= 0x0040,
};

//-----------------------------------------------------------------------------
// Events
//-----------------------------------------------------------------------------
enum InputEventType_t
{
	IE_ButtonPressed = 0,	// m_nData contains a ButtonCode_t
	IE_ButtonReleased,		// m_nData contains a ButtonCode_t
	IE_ButtonDoubleClicked,	// m_nData contains a ButtonCode_t
	IE_AnalogValueChanged,	// m_nData contains an AnalogCode_t, m_nData2 contains the value

	IE_FirstSystemEvent = 100,
	IE_Quit = IE_FirstSystemEvent,
	IE_ControllerInserted,	// m_nData contains the controller ID
	IE_ControllerUnplugged,	// m_nData contains the controller ID

	IE_FirstVguiEvent = 1000,	// Assign ranges for other systems that post user events here
	IE_FirstAppEvent = 2000,
};

struct InputEvent_t
{
	int m_nType;				// Type of the event (see InputEventType_t)
	int m_nTick;				// Tick on which the event occurred
	int m_nData;				// Generic 32-bit data, what it contains depends on the event
	int m_nData2;				// Generic 32-bit data, what it contains depends on the event
	int m_nData3;				// Generic 32-bit data, what it contains depends on the event
};

//-----------------------------------------------------------------------------
// Steam Controller Enums
//-----------------------------------------------------------------------------

#define MAX_STEAM_CONTROLLERS 8

typedef enum
{
	SK_NULL,
	SK_BUTTON_A,
	SK_BUTTON_B,
	SK_BUTTON_X,
	SK_BUTTON_Y,
	SK_BUTTON_UP,
	SK_BUTTON_RIGHT,
	SK_BUTTON_DOWN,
	SK_BUTTON_LEFT,
	SK_BUTTON_LEFT_BUMPER,
	SK_BUTTON_RIGHT_BUMPER,
	SK_BUTTON_LEFT_TRIGGER,
	SK_BUTTON_RIGHT_TRIGGER,
	SK_BUTTON_LEFT_GRIP,
	SK_BUTTON_RIGHT_GRIP,
	SK_BUTTON_LPAD_TOUCH,
	SK_BUTTON_RPAD_TOUCH,
	SK_BUTTON_LPAD_CLICK,
	SK_BUTTON_RPAD_CLICK,
	SK_BUTTON_LPAD_UP,
	SK_BUTTON_LPAD_RIGHT,
	SK_BUTTON_LPAD_DOWN,
	SK_BUTTON_LPAD_LEFT,
	SK_BUTTON_RPAD_UP,
	SK_BUTTON_RPAD_RIGHT,
	SK_BUTTON_RPAD_DOWN,
	SK_BUTTON_RPAD_LEFT,
	SK_BUTTON_SELECT,
	SK_BUTTON_START,
	SK_BUTTON_STEAM,
	SK_BUTTON_INACTIVE_START,
	SK_VBUTTON_F1,						// These are "virtual" buttons. Useful if you want to have flow that maps an action to button code to be interpreted by some UI that accepts keystrokes, but you
	SK_VBUTTON_F2,						// don't want to map to real button (perhaps because it would be interpreted by UI in a way you don't like). 																																										
	SK_VBUTTON_F3,
	SK_VBUTTON_F4,
	SK_VBUTTON_F5,
	SK_VBUTTON_F6,
	SK_VBUTTON_F7,
	SK_VBUTTON_F8,
	SK_VBUTTON_F9,
	SK_VBUTTON_F10,
	SK_VBUTTON_F11,
	SK_VBUTTON_F12,
	SK_MAX_KEYS
} sKey_t;

enum ESteamPadAxis
{
	LEFTPAD_AXIS_X,
	LEFTPAD_AXIS_Y,
	RIGHTPAD_AXIS_X,
	RIGHTPAD_AXIS_Y,
	LEFT_TRIGGER_AXIS,
	RIGHT_TRIGGER_AXIS,
	GYRO_AXIS_PITCH,
	GYRO_AXIS_ROLL,
	GYRO_AXIS_YAW,
	MAX_STEAMPADAXIS = GYRO_AXIS_YAW
};

enum
{
	LASTINPUT_KBMOUSE = 0,
	LASTINPUT_CONTROLLER = 1,
	LASTINPUT_STEAMCONTROLLER = 2
};

enum GameActionSet_t
{
	GAME_ACTION_SET_NONE = -1,
	GAME_ACTION_SET_MENUCONTROLS = 0,
	GAME_ACTION_SET_FPSCONTROLS,
	GAME_ACTION_SET_IN_GAME_HUD,
	GAME_ACTION_SET_SPECTATOR,
};

enum GameActionSetFlags_t
{
	GAME_ACTION_SET_FLAGS_NONE = 0,
	GAME_ACTION_SET_FLAGS_TAUNTING = (1<<0),
};

enum JoystickType_t
{
	INPUT_TYPE_GENERIC_JOYSTICK = 0,
	INPUT_TYPE_X360,
	INPUT_TYPE_STEAMCONTROLLER,
};

#endif // INPUTENUMS_H
