//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "iefx.h"
#include "fx.h"
#include "tier0/vprof.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Footprint Decal TE
//-----------------------------------------------------------------------------

class C_TEFootprintDecal : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEFootprintDecal, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEFootprintDecal( void );
	virtual			~C_TEFootprintDecal( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	virtual void	Precache( void );

public:
	Vector			m_vecOrigin;
	Vector			m_vecDirection;
	Vector			m_vecStart;
	int				m_nEntity;
	int				m_nIndex;
	char			m_chMaterialType;
};

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEFootprintDecal, DT_TEFootprintDecal, CTEFootprintDecal)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropVector( RECVINFO(m_vecDirection)),
	RecvPropInt( RECVINFO(m_nEntity)),
	RecvPropInt( RECVINFO(m_nIndex)),
	RecvPropInt( RECVINFO(m_chMaterialType)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------

C_TEFootprintDecal::C_TEFootprintDecal( void )
{
	m_vecOrigin.Init();
	m_vecStart.Init();
	m_nEntity = 0;
	m_nIndex = 0;
	m_chMaterialType = 'C';
}

C_TEFootprintDecal::~C_TEFootprintDecal( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void C_TEFootprintDecal::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Do stuff when data changes
//-----------------------------------------------------------------------------

void C_TEFootprintDecal::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEFootprintDecal::PostDataUpdate" );

	// FIXME: Make this choose the decal based on material type
	if ( r_decals.GetInt() )
	{
		C_BaseEntity *ent = cl_entitylist->GetEnt( m_nEntity );
		if ( ent )
		{
			effects->DecalShoot( m_nIndex, 
				m_nEntity, ent->GetModel(), ent->GetAbsOrigin(), ent->GetAbsAngles(), m_vecOrigin, &m_vecDirection, 0 );
		}
	}
}

void TE_FootprintDecal( IRecipientFilter& filter, float delay, const Vector *origin, const Vector* right, 
	int entity, int index, unsigned char materialType )
{
	if ( r_decals.GetInt() )
	{
		C_BaseEntity *ent = cl_entitylist->GetEnt( entity );
		if ( ent )
		{
			effects->DecalShoot( index, entity, ent->GetModel(), ent->GetAbsOrigin(), ent->GetAbsAngles(), *origin, right, 0 );
		}
	}
}