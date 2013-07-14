//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_CLASS_SHARED_H
#define SRCPY_CLASS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include <boost/python.hpp>

//-----------------------------------------------------------------------------
// Shared between server and client class
// When you add a new type:
// 1. Add DECLARE_PYSERVERCLASS and DECLARE_PYCLIENTCLASS to the headers of the class
// 2. Add IMPLEMENT_PYSERVERCLASS and IMPLEMENT_PYCLIENTCLASS to the cpp files of the class
// 3. Link recv and send tables in SetupClientClassRecv and SetupServerClass to the type
// 4. Add a fall back factory in ClientClassFactory.
// 5. Regenerate bindings
//-----------------------------------------------------------------------------
enum PyNetworkTypes
{
	PN_NONE = 0,
	PN_BASEENTITY,
	PN_BASEANIMATING,
	PN_BASEANIMATINGOVERLAY,
	PN_BASEFLEX,
	PN_BASECOMBATCHARACTER,
	PN_BASEPLAYER,
	PN_HL2WARSPLAYER,
	PN_UNITBASE,
	PN_BASEGRENADE,
	PN_SPRITE,
	PN_SMOKETRAIL,
	PN_BEAM,
	PN_BASECOMBATWEAPON,
	PN_WARSWEAPON,
	PN_FUNCUNIT,
	PN_BASETOGGLE,
	PN_BASETRIGGER,
};

boost::python::object CreatePyHandleHelper( const CBaseEntity *pEnt, const char *handlename );

#endif // SRCPY_CLASS_SHARED_H

