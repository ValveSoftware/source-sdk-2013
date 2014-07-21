//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: this is a map for virtual key codes
//			virtual key codes may exist outside this range for other languages
// NOTE: Button codes also contain mouse codes, but we won't worry about that
//
// $NoKeywords: $
//===========================================================================//

#ifndef KEYCODE_H
#define KEYCODE_H

#ifdef _WIN32
#pragma once
#endif

#include "inputsystem/ButtonCode.h"

namespace vgui
{
typedef ButtonCode_t KeyCode;
}

#endif // KEYCODE_H
