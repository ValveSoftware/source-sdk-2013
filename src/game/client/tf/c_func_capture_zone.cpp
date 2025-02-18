//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_shareddefs.h"
#include "c_func_capture_zone.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_CaptureZone, DT_CaptureZone, CCaptureZone )
	RecvPropInt( RECVINFO( m_bDisabled ) ),
END_RECV_TABLE()

IMPLEMENT_AUTO_LIST( ICaptureZoneAutoList );
