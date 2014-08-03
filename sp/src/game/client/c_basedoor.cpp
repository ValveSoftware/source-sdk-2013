//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basedoor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CBaseDoor
#undef CBaseDoor
#endif

IMPLEMENT_CLIENTCLASS_DT(C_BaseDoor, DT_BaseDoor, CBaseDoor)
	RecvPropFloat(RECVINFO(m_flWaveHeight)),
END_RECV_TABLE()

C_BaseDoor::C_BaseDoor( void )
{
	m_flWaveHeight = 0.0f;
}

C_BaseDoor::~C_BaseDoor( void )
{
}
