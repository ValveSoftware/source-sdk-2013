//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef LOCALFLEXCONTROLLER_H
#define LOCALFLEXCONTROLLER_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

enum LocalFlexController_t
{
	// this isn't really an enum - its just a typed int. gcc will not accept it as a fwd decl, so we'll define one value
	DUMMY_FLEX_CONTROLLER=0x7fffffff						// make take 32 bits
};

inline LocalFlexController_t &operator++( LocalFlexController_t &a      ) { return a = LocalFlexController_t( int( a ) + 1 ); }
inline LocalFlexController_t &operator--( LocalFlexController_t &a      ) { return a = LocalFlexController_t( int( a ) - 1 ); }
inline LocalFlexController_t  operator++( LocalFlexController_t &a, int ) { LocalFlexController_t t = a; a = LocalFlexController_t( int( a ) + 1 ); return t; }
inline LocalFlexController_t  operator--( LocalFlexController_t &a, int ) { LocalFlexController_t t = a; a = LocalFlexController_t( int( a ) - 1 ); return t; }


#endif	// LOCALFLEXCONTROLLER_H
