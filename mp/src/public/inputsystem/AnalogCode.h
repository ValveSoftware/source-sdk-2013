//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef ANALOGCODE_H
#define ANALOGCODE_H

#ifdef _WIN32
#pragma once
#endif


#include "inputsystem/InputEnums.h"


//-----------------------------------------------------------------------------
// Macro to get at joystick codes
//-----------------------------------------------------------------------------
#define JOYSTICK_AXIS_INTERNAL( _joystick, _axis ) ( JOYSTICK_FIRST_AXIS + ((_joystick) * MAX_JOYSTICK_AXES) + (_axis) )
#define JOYSTICK_AXIS( _joystick, _axis ) ( (AnalogCode_t)JOYSTICK_AXIS_INTERNAL( _joystick, _axis ) )


//-----------------------------------------------------------------------------
// Enumeration for analog input devices. Includes joysticks, mousewheel, mouse
//-----------------------------------------------------------------------------
enum AnalogCode_t
{
	ANALOG_CODE_INVALID = -1,
	MOUSE_X = 0,
	MOUSE_Y,
	MOUSE_XY,		// Invoked when either x or y changes
	MOUSE_WHEEL,

	JOYSTICK_FIRST_AXIS,
	JOYSTICK_LAST_AXIS = JOYSTICK_AXIS_INTERNAL( MAX_JOYSTICKS-1, MAX_JOYSTICK_AXES-1 ),

	ANALOG_CODE_LAST,
};


#endif // ANALOGCODE_H
