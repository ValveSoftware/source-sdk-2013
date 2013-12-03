//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Fizz TE
//-----------------------------------------------------------------------------
class C_TEFizz : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEFizz, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEFizz( void );
	virtual			~C_TEFizz( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	int				m_nEntity;
	int				m_nModelIndex;
	int				m_nDensity;
	int				m_nCurrent;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEFizz::C_TEFizz( void )
{
	m_nEntity		= 0;
	m_nModelIndex	= 0;
	m_nDensity		= 0;
	m_nCurrent		= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEFizz::~C_TEFizz( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEFizz::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEFizz::PostDataUpdate" );

	C_BaseEntity *pEnt = cl_entitylist->GetEnt( m_nEntity );
	if (pEnt != NULL)
	{
		tempents->FizzEffect(pEnt, m_nModelIndex, m_nDensity, m_nCurrent );
	}
}

void TE_Fizz( IRecipientFilter& filter, float delay,
	const C_BaseEntity *ed, int modelindex, int density, int current )
{
	C_BaseEntity *pEnt = (C_BaseEntity *)ed;
	if (pEnt != NULL)
	{
		tempents->FizzEffect(pEnt, modelindex, density, current );
	}
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEFizz, DT_TEFizz, CTEFizz)
	RecvPropInt( RECVINFO(m_nEntity)),
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropInt( RECVINFO(m_nDensity)),
	RecvPropInt( RECVINFO(m_nCurrent)),
END_RECV_TABLE()


