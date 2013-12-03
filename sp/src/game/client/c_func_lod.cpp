//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "view.h"
#include "iviewrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_Func_LOD : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_Func_LOD, C_BaseEntity );
	DECLARE_CLIENTCLASS();

					C_Func_LOD();

// C_BaseEntity overrides.
public:

	unsigned char	GetClientSideFade();

public:
// Replicated vars from the server.
// These are documented in the server-side entity.
public:
	float			m_fDisappearDist;
};


ConVar lod_TransitionDist("lod_TransitionDist", "800");


// ------------------------------------------------------------------------- //
// Tables.
// ------------------------------------------------------------------------- //

// Datatable..
IMPLEMENT_CLIENTCLASS_DT(C_Func_LOD, DT_Func_LOD, CFunc_LOD)
	RecvPropFloat(RECVINFO(m_fDisappearDist)),
END_RECV_TABLE()



// ------------------------------------------------------------------------- //
// C_Func_LOD implementation.
// ------------------------------------------------------------------------- //

C_Func_LOD::C_Func_LOD()
{
	m_fDisappearDist = 5000.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate a fade.
//-----------------------------------------------------------------------------
unsigned char C_Func_LOD::GetClientSideFade()
{
	return UTIL_ComputeEntityFade( this, m_fDisappearDist, m_fDisappearDist + lod_TransitionDist.GetFloat(), 1.0f );
}


