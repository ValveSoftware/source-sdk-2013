//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2 specific input handling
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "kbutton.h"
#include "input.h"

//-----------------------------------------------------------------------------
// Purpose: TF Input interface
//-----------------------------------------------------------------------------
class CSDKInput : public CInput
{
public:
};

static CSDKInput g_Input;

// Expose this interface
IInput *input = ( IInput * )&g_Input;

