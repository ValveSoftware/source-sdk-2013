//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef EP1_GAMESTATS_H
#define EP1_GAMESTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "gamestats.h"

class CEP1GameStats : public CBaseGameStats
{
	typedef CBaseGameStats BaseClass;

public:
	CEP1GameStats( void );

	virtual CBaseGameStats *OnInit( CBaseGameStats *pCurrentGameStats, char const *gamedir );

	virtual bool StatTrackingEnabledForMod( void ) { return true; }
	virtual bool UserPlayedAllTheMaps( void );

	virtual const char *GetStatSaveFileName( void );
	virtual const char *GetStatUploadRegistryKeyName( void );
};

#endif // EP1_GAMESTATS_H
