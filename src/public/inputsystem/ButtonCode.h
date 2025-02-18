//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef BUTTONCODE_H
#define BUTTONCODE_H

#ifdef _WIN32
#pragma once
#endif

#include "inputsystem/InputEnums.h"
#include "mathlib/mathlib.h"

//-----------------------------------------------------------------------------
// Button enum. "Buttons" are binary-state input devices (mouse buttons, keyboard keys)
//-----------------------------------------------------------------------------
enum
{
	JOYSTICK_MAX_BUTTON_COUNT = 32,
	JOYSTICK_POV_BUTTON_COUNT = 4,
	JOYSTICK_AXIS_BUTTON_COUNT = MAX_JOYSTICK_AXES * 2,
};

#define JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_BUTTON + ((_joystick) * JOYSTICK_MAX_BUTTON_COUNT) + (_button) )
#define JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_POV_BUTTON + ((_joystick) * JOYSTICK_POV_BUTTON_COUNT) + (_button) )
#define JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_AXIS_BUTTON + ((_joystick) * JOYSTICK_AXIS_BUTTON_COUNT) + (_button) )

#define JOYSTICK_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_POV_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_AXIS_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) )

//-----------------------------------------------------------------------------
// Button enum. "Buttons" are binary-state input devices (mouse buttons, keyboard keys)
//-----------------------------------------------------------------------------
enum
{
	STEAMCONTROLLER_MAX_BUTTON_COUNT = SK_MAX_KEYS - 1,
	STEAMCONTROLLER_AXIS_BUTTON_COUNT = MAX_STEAMPADAXIS * 2,
};

#define STEAMCONTROLLER_BUTTON_INTERNAL( _joystick, _button ) ( STEAMCONTROLLER_FIRST_BUTTON + ((_joystick) * STEAMCONTROLLER_MAX_BUTTON_COUNT) + (_button) )
#define STEAMCONTROLLER_AXIS_BUTTON_INTERNAL( _joystick, _button ) ( STEAMCONTROLLER_FIRST_AXIS_BUTTON + ((_joystick) * STEAMCONTROLLER_AXIS_BUTTON_COUNT) + (_button) )

#define STEAMCONTROLLER_BUTTON( _joystick, _button ) ( (ButtonCode_t)STEAMCONTROLLER_BUTTON_INTERNAL( _joystick, _button ) )
#define STEAMCONTROLLER_AXIS_BUTTON( _joystick, _button ) ( (ButtonCode_t)STEAMCONTROLLER_AXIS_BUTTON_INTERNAL( _joystick, _button ) )

enum ButtonCode_t
{
	BUTTON_CODE_INVALID = -1,
	BUTTON_CODE_NONE = 0,

	KEY_FIRST = 0,

	KEY_NONE = KEY_FIRST,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_PAD_0,
	KEY_PAD_1,
	KEY_PAD_2,
	KEY_PAD_3,
	KEY_PAD_4,
	KEY_PAD_5,
	KEY_PAD_6,
	KEY_PAD_7,
	KEY_PAD_8,
	KEY_PAD_9,
	KEY_PAD_DIVIDE,
	KEY_PAD_MULTIPLY,
	KEY_PAD_MINUS,
	KEY_PAD_PLUS,
	KEY_PAD_ENTER,
	KEY_PAD_DECIMAL,
	KEY_LBRACKET,
	KEY_RBRACKET,
	KEY_SEMICOLON,
	KEY_APOSTROPHE,
	KEY_BACKQUOTE,
	KEY_COMMA,
	KEY_PERIOD,
	KEY_SLASH,
	KEY_BACKSLASH,
	KEY_MINUS,
	KEY_EQUAL,
	KEY_ENTER,
	KEY_SPACE,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_CAPSLOCK,
	KEY_NUMLOCK,
	KEY_ESCAPE,
	KEY_SCROLLLOCK,
	KEY_INSERT,
	KEY_DELETE,
	KEY_HOME,
	KEY_END,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_BREAK,
	KEY_LSHIFT,
	KEY_RSHIFT,
	KEY_LALT,
	KEY_RALT,
	KEY_LCONTROL,
	KEY_RCONTROL,
	KEY_LWIN,
	KEY_RWIN,
	KEY_APP,
	KEY_UP,
	KEY_LEFT,
	KEY_DOWN,
	KEY_RIGHT,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_CAPSLOCKTOGGLE,
	KEY_NUMLOCKTOGGLE,
	KEY_SCROLLLOCKTOGGLE,

	KEY_LAST = KEY_SCROLLLOCKTOGGLE,
	KEY_COUNT = KEY_LAST - KEY_FIRST + 1,

	// Mouse
	MOUSE_FIRST = KEY_LAST + 1,

	MOUSE_LEFT = MOUSE_FIRST,
	MOUSE_RIGHT,
	MOUSE_MIDDLE,
	MOUSE_4,
	MOUSE_5,
	MOUSE_WHEEL_UP,		// A fake button which is 'pressed' and 'released' when the wheel is moved up 
	MOUSE_WHEEL_DOWN,	// A fake button which is 'pressed' and 'released' when the wheel is moved down

	MOUSE_LAST = MOUSE_WHEEL_DOWN,
	MOUSE_COUNT = MOUSE_LAST - MOUSE_FIRST + 1,

	// Joystick
	JOYSTICK_FIRST = MOUSE_LAST + 1,

	JOYSTICK_FIRST_BUTTON = JOYSTICK_FIRST,
	JOYSTICK_LAST_BUTTON = JOYSTICK_BUTTON_INTERNAL( MAX_JOYSTICKS-1, JOYSTICK_MAX_BUTTON_COUNT-1 ),
	JOYSTICK_FIRST_POV_BUTTON,
	JOYSTICK_LAST_POV_BUTTON = JOYSTICK_POV_BUTTON_INTERNAL( MAX_JOYSTICKS-1, JOYSTICK_POV_BUTTON_COUNT-1 ),
	JOYSTICK_FIRST_AXIS_BUTTON,
	JOYSTICK_LAST_AXIS_BUTTON = JOYSTICK_AXIS_BUTTON_INTERNAL( MAX_JOYSTICKS-1, JOYSTICK_AXIS_BUTTON_COUNT-1 ),

	JOYSTICK_LAST = JOYSTICK_LAST_AXIS_BUTTON,

#if !defined ( _X360 )
	NOVINT_FIRST = JOYSTICK_LAST + 2, // plus 1 missing key. +1 seems to cause issues on the first button.
	
	NOVINT_LOGO_0 = NOVINT_FIRST,
	NOVINT_TRIANGLE_0,
	NOVINT_BOLT_0,
	NOVINT_PLUS_0,
	NOVINT_LOGO_1,
	NOVINT_TRIANGLE_1,
	NOVINT_BOLT_1,
	NOVINT_PLUS_1,
	
	NOVINT_LAST = NOVINT_PLUS_1,
	STEAMCONTROLLER_FIRST = NOVINT_LAST + 1,
#else
	STEAMCONTROLLER_FIRST = JOYSTICK_LAST + 1
#endif

	// Steam Controller
	STEAMCONTROLLER_FIRST_BUTTON = STEAMCONTROLLER_FIRST,
	STEAMCONTROLLER_LAST_BUTTON = STEAMCONTROLLER_BUTTON_INTERNAL( MAX_STEAM_CONTROLLERS - 1, STEAMCONTROLLER_MAX_BUTTON_COUNT - 1 ),
	STEAMCONTROLLER_FIRST_AXIS_BUTTON,
	STEAMCONTROLLER_LAST_AXIS_BUTTON = STEAMCONTROLLER_AXIS_BUTTON_INTERNAL( MAX_STEAM_CONTROLLERS - 1, STEAMCONTROLLER_AXIS_BUTTON_COUNT - 1 ),
	STEAMCONTROLLER_LAST = STEAMCONTROLLER_LAST_AXIS_BUTTON,

	BUTTON_CODE_LAST,
	BUTTON_CODE_COUNT = BUTTON_CODE_LAST - KEY_FIRST + 1,

	// Helpers for XBox 360
	KEY_XBUTTON_UP = JOYSTICK_FIRST_POV_BUTTON,	// POV buttons
	KEY_XBUTTON_RIGHT,
	KEY_XBUTTON_DOWN,
	KEY_XBUTTON_LEFT,

	KEY_XBUTTON_A = JOYSTICK_FIRST_BUTTON,		// Buttons
	KEY_XBUTTON_B,
	KEY_XBUTTON_X,
	KEY_XBUTTON_Y,
	KEY_XBUTTON_LEFT_SHOULDER,
	KEY_XBUTTON_RIGHT_SHOULDER,
	KEY_XBUTTON_BACK,
	KEY_XBUTTON_START,
	KEY_XBUTTON_STICK1,
	KEY_XBUTTON_STICK2,

	KEY_XSTICK1_RIGHT = JOYSTICK_FIRST_AXIS_BUTTON,	// XAXIS POSITIVE
	KEY_XSTICK1_LEFT,							// XAXIS NEGATIVE
	KEY_XSTICK1_DOWN,							// YAXIS POSITIVE
	KEY_XSTICK1_UP,								// YAXIS NEGATIVE
	KEY_XBUTTON_LTRIGGER,						// ZAXIS POSITIVE
	KEY_XBUTTON_RTRIGGER,						// ZAXIS NEGATIVE
	KEY_XSTICK2_RIGHT,							// UAXIS POSITIVE
	KEY_XSTICK2_LEFT,							// UAXIS NEGATIVE
	KEY_XSTICK2_DOWN,							// VAXIS POSITIVE
	KEY_XSTICK2_UP,								// VAXIS NEGATIVE

	// Helpers for Steam Controller
	STEAMCONTROLLER_A = STEAMCONTROLLER_FIRST_BUTTON,
	STEAMCONTROLLER_B,
	STEAMCONTROLLER_X,
	STEAMCONTROLLER_Y,
	STEAMCONTROLLER_DPAD_UP,
	STEAMCONTROLLER_DPAD_RIGHT,
	STEAMCONTROLLER_DPAD_DOWN,
	STEAMCONTROLLER_DPAD_LEFT,
	STEAMCONTROLLER_LEFT_BUMPER,
	STEAMCONTROLLER_RIGHT_BUMPER,
	STEAMCONTROLLER_LEFT_TRIGGER,
	STEAMCONTROLLER_RIGHT_TRIGGER,
	STEAMCONTROLLER_LEFT_GRIP,
	STEAMCONTROLLER_RIGHT_GRIP,
	STEAMCONTROLLER_LEFT_PAD_FINGERDOWN,
	STEAMCONTROLLER_RIGHT_PAD_FINGERDOWN,
	STEAMCONTROLLER_LEFT_PAD_CLICK,
	STEAMCONTROLLER_RIGHT_PAD_CLICK,
	STEAMCONTROLLER_LEFT_PAD_UP,
	STEAMCONTROLLER_LEFT_PAD_RIGHT,
	STEAMCONTROLLER_LEFT_PAD_DOWN,
	STEAMCONTROLLER_LEFT_PAD_LEFT,
	STEAMCONTROLLER_RIGHT_PAD_UP,
	STEAMCONTROLLER_RIGHT_PAD_RIGHT,
	STEAMCONTROLLER_RIGHT_PAD_DOWN,
	STEAMCONTROLLER_RIGHT_PAD_LEFT,
	STEAMCONTROLLER_SELECT,
	STEAMCONTROLLER_START,
	STEAMCONTROLLER_STEAM,
	STEAMCONTROLLER_INACTIVE_START,
	STEAMCONTROLLER_F1,
	STEAMCONTROLLER_F2,
	STEAMCONTROLLER_F3,
	STEAMCONTROLLER_F4,
	STEAMCONTROLLER_F5,
	STEAMCONTROLLER_F6,
	STEAMCONTROLLER_F7,
	STEAMCONTROLLER_F8,
	STEAMCONTROLLER_F9,
	STEAMCONTROLLER_F10,
	STEAMCONTROLLER_F11,
	STEAMCONTROLLER_F12,
};

inline bool IsAlpha( ButtonCode_t code )
{
	return ( code >= KEY_A ) && ( code <= KEY_Z );
}

inline bool IsAlphaNumeric( ButtonCode_t code )
{
	return ( code >= KEY_0 ) && ( code <= KEY_Z );
}

inline bool IsSpace( ButtonCode_t code )
{
	return ( code == KEY_ENTER ) || ( code == KEY_TAB ) || ( code == KEY_SPACE );
}

inline bool IsKeypad( ButtonCode_t code )
{
	return ( code >= MOUSE_FIRST ) && ( code <= KEY_PAD_DECIMAL );
}

inline bool IsPunctuation( ButtonCode_t code )
{
	return ( code >= KEY_0 ) && ( code <= KEY_SPACE ) && !IsAlphaNumeric( code ) && !IsSpace( code ) && !IsKeypad( code );
}

inline bool IsKeyCode( ButtonCode_t code )
{
	return ( code >= KEY_FIRST ) && ( code <= KEY_LAST );
}

inline bool IsMouseCode( ButtonCode_t code )
{
	return ( code >= MOUSE_FIRST ) && ( code <= MOUSE_LAST );
}

inline bool IsNovintCode( ButtonCode_t code )
{
#if !defined ( _X360 )
	return ( ( code >= NOVINT_FIRST ) && ( code <= NOVINT_LAST ) );
#else
	return false;
#endif
}

inline bool IsNovintButtonCode( ButtonCode_t code )
{
#if !defined ( _X360 )
	return IsNovintCode( code );
#else
	return false;
#endif
}

inline bool IsJoystickCode( ButtonCode_t code )
{
	return ( (( code >= JOYSTICK_FIRST ) && ( code <= JOYSTICK_LAST )) || ((code >= STEAMCONTROLLER_FIRST_BUTTON) && (code <= STEAMCONTROLLER_LAST_BUTTON)) );
}

inline bool IsJoystickButtonCode( ButtonCode_t code )
{
	return ( ( ( code >= JOYSTICK_FIRST_BUTTON ) && ( code <= JOYSTICK_LAST_BUTTON ) ) || IsNovintButtonCode( code ) );
}

inline bool IsJoystickPOVCode( ButtonCode_t code )
{
	return ( code >= JOYSTICK_FIRST_POV_BUTTON ) && ( code <= JOYSTICK_LAST_POV_BUTTON );
}

inline bool IsJoystickAxisCode( ButtonCode_t code )
{
	return ( code >= JOYSTICK_FIRST_AXIS_BUTTON ) && ( code <= JOYSTICK_LAST_AXIS_BUTTON );
}

inline bool IsSteamControllerCode( ButtonCode_t code )
{
	return ( ( code >= STEAMCONTROLLER_FIRST ) && ( code <= STEAMCONTROLLER_LAST ) );
}

inline bool IsSteamControllerButtonCode( ButtonCode_t code )
{
	return ( code >= STEAMCONTROLLER_FIRST_BUTTON ) && ( code <= STEAMCONTROLLER_LAST_BUTTON );
}

inline bool IsSteamControllerAxisCode( ButtonCode_t code )
{
	return ( code >= STEAMCONTROLLER_FIRST_AXIS_BUTTON ) && ( code <= STEAMCONTROLLER_LAST_AXIS_BUTTON );
}

inline ButtonCode_t GetBaseButtonCode( ButtonCode_t code )
{
	if ( IsJoystickButtonCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_BUTTON ) % JOYSTICK_MAX_BUTTON_COUNT;
		return (ButtonCode_t)( JOYSTICK_FIRST_BUTTON + offset );
	}

	if ( IsJoystickPOVCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_POV_BUTTON ) % JOYSTICK_POV_BUTTON_COUNT;
		return (ButtonCode_t)( JOYSTICK_FIRST_POV_BUTTON + offset );
	}

	if ( IsJoystickAxisCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_AXIS_BUTTON ) % JOYSTICK_AXIS_BUTTON_COUNT;
		return (ButtonCode_t)( JOYSTICK_FIRST_AXIS_BUTTON + offset );
	}

	if ( IsSteamControllerButtonCode( code ) )
	{
		int offset = ( code - STEAMCONTROLLER_FIRST_BUTTON ) % STEAMCONTROLLER_MAX_BUTTON_COUNT;
		return ( ButtonCode_t )( STEAMCONTROLLER_FIRST_BUTTON + offset );
	}

	if ( IsSteamControllerAxisCode( code ) )
	{
		int offset = ( code - STEAMCONTROLLER_FIRST_AXIS_BUTTON ) % STEAMCONTROLLER_AXIS_BUTTON_COUNT;
		return ( ButtonCode_t )( STEAMCONTROLLER_FIRST_AXIS_BUTTON + offset );
	}

	return code;
}

inline int GetJoystickForCode( ButtonCode_t code )
{
	if ( !IsJoystickCode( code ) )
		return 0;

	if ( IsJoystickButtonCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_BUTTON ) / JOYSTICK_MAX_BUTTON_COUNT;
		return offset;
	}
	if ( IsJoystickPOVCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_POV_BUTTON ) / JOYSTICK_POV_BUTTON_COUNT;
		return offset;
	}
	if ( IsJoystickAxisCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_AXIS_BUTTON ) / JOYSTICK_AXIS_BUTTON_COUNT;
		return offset;
	}
	if ( IsSteamControllerButtonCode( code ) )
	{
		int offset = ( code - STEAMCONTROLLER_FIRST_BUTTON ) / STEAMCONTROLLER_MAX_BUTTON_COUNT;
		return offset;
	}
	if ( IsSteamControllerAxisCode( code ) )
	{
		int offset = ( code - STEAMCONTROLLER_FIRST_AXIS_BUTTON ) / STEAMCONTROLLER_AXIS_BUTTON_COUNT;
		return offset;
	}

	return 0;
}

inline ButtonCode_t ButtonCodeToJoystickButtonCode( ButtonCode_t code, int nDesiredJoystick )
{
	if ( ( !IsJoystickCode( code ) && !IsSteamControllerCode( code ) ) || nDesiredJoystick == 0 )
		return code;

	if ( IsJoystickCode( code ) && !IsSteamControllerCode( code ) )
		nDesiredJoystick = clamp( nDesiredJoystick, 0, MAX_JOYSTICKS - 1 );
	else
		nDesiredJoystick = clamp( nDesiredJoystick, 0, MAX_STEAM_CONTROLLERS - 1 );

	code = GetBaseButtonCode( code );

	// Now upsample it
	if ( IsJoystickButtonCode( code ) )
	{
		int nOffset = code - JOYSTICK_FIRST_BUTTON;
		return JOYSTICK_BUTTON( nDesiredJoystick, nOffset );
	}

	if ( IsJoystickPOVCode( code ) )
	{
		int nOffset = code - JOYSTICK_FIRST_POV_BUTTON;
		return JOYSTICK_POV_BUTTON( nDesiredJoystick, nOffset );
	}

	if ( IsJoystickAxisCode( code ) )
	{
		int nOffset = code - JOYSTICK_FIRST_AXIS_BUTTON;
		return JOYSTICK_AXIS_BUTTON( nDesiredJoystick, nOffset );
	}

	if ( IsSteamControllerButtonCode( code ) )
	{
		int nOffset = code - STEAMCONTROLLER_FIRST_BUTTON;
		return STEAMCONTROLLER_BUTTON( nDesiredJoystick, nOffset );
	}

	if ( IsJoystickAxisCode( code ) )
	{
		int nOffset = code - STEAMCONTROLLER_FIRST_AXIS_BUTTON;
		return STEAMCONTROLLER_AXIS_BUTTON( nDesiredJoystick, nOffset );
	}

	return code;
}

#endif // BUTTONCODE_H
