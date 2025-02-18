//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef IGAMECONSOLE_H
#define IGAMECONSOLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"

namespace vgui
{
	typedef uintp VPANEL;
}

//-----------------------------------------------------------------------------
// Purpose: interface to game/dev console
//-----------------------------------------------------------------------------
abstract_class IGameConsole : public IBaseInterface
{
public:
	// activates the console, makes it visible and brings it to the foreground
	virtual void Activate() = 0;

	virtual void Initialize() = 0;

	// hides the console
	virtual void Hide() = 0;

	// clears the console
	virtual void Clear() = 0;

	// return true if the console has focus
	virtual bool IsConsoleVisible() = 0;

	virtual void SetParent( vgui::VPANEL parent ) = 0;
};

#define GAMECONSOLE_INTERFACE_VERSION "GameConsole004"

#endif // IGAMECONSOLE_H
