//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Client Projectile TE
//-----------------------------------------------------------------------------
class C_TEClientProjectile : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEClientProjectile, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	C_TEClientProjectile( void );
	virtual			~C_TEClientProjectile( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector m_vecOrigin;
	Vector m_vecVelocity;
	int m_nModelIndex;
	int m_nLifeTime;
	EHANDLE m_hOwner;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEClientProjectile::C_TEClientProjectile( void )
{
	m_vecOrigin.Init();
	m_vecVelocity.Init();
	m_nModelIndex = 0;
	m_nLifeTime = 0;
	m_hOwner = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEClientProjectile::~C_TEClientProjectile( void )
{
}

void TE_ClientProjectile( IRecipientFilter& filter, float delay,
					const Vector* vecOrigin, const Vector* vecVelocity, int modelindex, int lifetime, CBaseEntity *pOwner )
{
	tempents->ClientProjectile( *vecOrigin, *vecVelocity, vec3_origin, modelindex, lifetime, pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEClientProjectile::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEClientProjectile::PostDataUpdate" );

	tempents->ClientProjectile( m_vecOrigin, m_vecVelocity, vec3_origin, m_nModelIndex, m_nLifeTime, m_hOwner );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEClientProjectile, DT_TEClientProjectile, CTEClientProjectile)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropVector( RECVINFO(m_vecVelocity)),
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropInt( RECVINFO(m_nLifeTime)),
	RecvPropEHandle( RECVINFO(m_hOwner)),
END_RECV_TABLE()
