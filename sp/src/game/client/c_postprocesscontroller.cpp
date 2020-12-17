//====== Copyright © 1996-2008, Valve Corporation, All rights reserved. =======
//
// Purpose: stores map postprocess params
//
//=============================================================================
#include "cbase.h"
#include "c_postprocesscontroller.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_PostProcessController, DT_PostProcessController, CPostProcessController )
	RecvPropArray3( RECVINFO_NAME( m_PostProcessParameters.m_flParameters[0], m_flPostProcessParameters ), POST_PROCESS_PARAMETER_COUNT, RecvPropFloat( RECVINFO_NAME( m_PostProcessParameters.m_flParameters[0], m_flPostProcessParameters[0] ) ) ),
	RecvPropBool( RECVINFO(m_bMaster) )
END_RECV_TABLE()

C_PostProcessController* C_PostProcessController::ms_pMasterController = nullptr;

//-----------------------------------------------------------------------------
C_PostProcessController::C_PostProcessController()
: 	m_bMaster( false )
{
	if ( ms_pMasterController == nullptr)
	{
		ms_pMasterController = this;
	}
}

//-----------------------------------------------------------------------------
C_PostProcessController::~C_PostProcessController()
{
	if ( ms_pMasterController == this )
	{
		ms_pMasterController = nullptr;
	}
}

void C_PostProcessController::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( m_bMaster )
	{
		ms_pMasterController = this;
	}
}

#ifdef MAPBASE
// Prevents parameters from fading after a save/restore
bool g_bPostProcessNeedsRestore = false;

void C_PostProcessController::OnRestore()
{
	BaseClass::OnRestore();

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->GetActivePostProcessController() == this )
	{
		// Tell clientmode this is part of a save/restore
		g_bPostProcessNeedsRestore = true;
	}
}
#endif
