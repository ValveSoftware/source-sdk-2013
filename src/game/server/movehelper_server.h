//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MOVEHELPER_SERVER_H
#define MOVEHELPER_SERVER_H

#ifdef _WIN32
#pragma once
#endif

#include "imovehelper.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class CBasePlayer;
class CBaseEntity;


//-----------------------------------------------------------------------------
// Implementation of the movehelper on the server
//-----------------------------------------------------------------------------

abstract_class IMoveHelperServer : public IMoveHelper
{
};

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------

IMoveHelperServer* MoveHelperServer();


#endif // MOVEHELPER_SERVER_H
