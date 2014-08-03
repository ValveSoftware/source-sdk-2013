//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "dt_send.h"
#include "server_class.h"
#include "te_particlesystem.h"
#include "coordsize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST(CTEParticleSystem, DT_TEParticleSystem)
#if defined( TF_DLL )
	SendPropFloat( SENDINFO_VECTORELEM( m_vecOrigin, 0 ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_VECTORELEM( m_vecOrigin, 1 ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_VECTORELEM( m_vecOrigin, 2 ), -1, SPROP_COORD_MP_INTEGRAL ),
#else
	SendPropFloat( SENDINFO_VECTORELEM(m_vecOrigin, 0), -1, SPROP_COORD),
	SendPropFloat( SENDINFO_VECTORELEM(m_vecOrigin, 1), -1, SPROP_COORD),
	SendPropFloat( SENDINFO_VECTORELEM(m_vecOrigin, 2), -1, SPROP_COORD),
#endif
END_SEND_TABLE()




