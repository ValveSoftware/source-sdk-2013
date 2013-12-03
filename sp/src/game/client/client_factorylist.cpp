//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "client_factorylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static factorylist_t s_factories;

// Store off the factories
void FactoryList_Store( const factorylist_t &sourceData )
{
	s_factories = sourceData;
}

// retrieve the stored factories
void FactoryList_Retrieve( factorylist_t &destData )
{
	destData = s_factories;
}
