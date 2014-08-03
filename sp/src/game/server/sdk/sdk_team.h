//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//

#ifndef SDK_TEAM_H
#define SDK_TEAM_H

#ifdef _WIN32
#pragma once
#endif


#include "utlvector.h"
#include "team.h"


//-----------------------------------------------------------------------------
// Purpose: Team Manager
//-----------------------------------------------------------------------------
class CSDKTeam : public CTeam
{
	DECLARE_CLASS( CSDKTeam, CTeam );
	DECLARE_SERVERCLASS();

public:

	// Initialization
	virtual void Init( const char *pName, int iNumber );
};


extern CSDKTeam *GetGlobalSDKTeam( int iIndex );


#endif // TF_TEAM_H
