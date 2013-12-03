//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef USERID_H
#define USERID_H
#ifdef _WIN32
#pragma once
#endif

#if !defined( INCLUDED_STEAM_STEAMUSERIDTYPES_H )
	#include "SteamCommon.h"
#endif

#define IDTYPE_WON		0
#define IDTYPE_STEAM	1
#define IDTYPE_VALVE	2
#define IDTYPE_HLTV		3		
#define IDTYPE_REPLAY	4
typedef struct USERID_s
{
	int			idtype;

	union
	{
		TSteamGlobalUserID	steamid;
	} uid;

//	unsigned int		clientip;
} USERID_t;

#endif // USERID_H
