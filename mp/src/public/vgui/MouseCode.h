//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: names mouse button inputs
// NOTE: Button codes also contain key codes, but we won't worry about that
//
// $NoKeywords: $
//===========================================================================//

#ifndef MOUSECODE_H
#define MOUSECODE_H

#ifdef _WIN32
#pragma once
#endif

#include "inputsystem/ButtonCode.h"

namespace vgui
{
typedef ButtonCode_t MouseCode;
}

#endif // MOUSECODE_H
