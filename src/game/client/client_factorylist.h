//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef CLIENT_FACTORYLIST_H
#define CLIENT_FACTORYLIST_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

struct factorylist_t
{
	CreateInterfaceFn appSystemFactory;
	CreateInterfaceFn physicsFactory;
};

// Store off the factories
void FactoryList_Store( const factorylist_t &sourceData );

// retrieve the stored factories
void FactoryList_Retrieve( factorylist_t &destData );

#endif // CLIENT_FACTORYLIST_H
