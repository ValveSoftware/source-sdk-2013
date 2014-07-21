//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ITOOLDICTIONARY_H
#define ITOOLDICTIONARY_H

#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystem.h"

//-----------------------------------------------------------------------------
// Forward declaration
//-----------------------------------------------------------------------------
class IToolSystem;


//-----------------------------------------------------------------------------
// Purpose: Every tool dll sitting in bin\tools must expose this interface
//  The engine will load the .dll, get this interface, and then ask for all
//  tools in the .dll
// The engine will call CreateTools just before querying for the tools, so you 
//  can instance any dynamically instanced tools during that call
//-----------------------------------------------------------------------------
class IToolDictionary : public IAppSystem
{
public:
	virtual void		CreateTools() = 0;
	virtual int			GetToolCount() const = 0;
	virtual IToolSystem	*GetTool( int index ) = 0;
};

#define VTOOLDICTIONARY_INTERFACE_VERSION "VTOOLDICTIONARY002"

#endif // ITOOLDICTIONARY_H
