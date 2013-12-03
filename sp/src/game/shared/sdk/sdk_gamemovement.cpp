//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "gamemovement.h"
#include "sdk_gamerules.h"
#include "sdk_shareddefs.h"
#include "in_buttons.h"
#include "movevars_shared.h"


#ifdef CLIENT_DLL
	#include "c_sdk_player.h"
#else
	#include "sdk_player.h"
#endif


class CSDKGameMovement : public CGameMovement
{
public:
	DECLARE_CLASS( CSDKGameMovement, CGameMovement );

	CSDKGameMovement();
};


// Expose our interface.
static CSDKGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = ( IGameMovement * )&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement,INTERFACENAME_GAMEMOVEMENT, g_GameMovement );


// ---------------------------------------------------------------------------------------- //
// CSDKGameMovement.
// ---------------------------------------------------------------------------------------- //

CSDKGameMovement::CSDKGameMovement()
{
	//m_vecViewOffsetNormal = SDK_PLAYER_VIEW_OFFSET;
}

