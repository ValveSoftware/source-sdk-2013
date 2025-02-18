//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Create a muzzle flash temp ent
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basetempentity.h"
#include "coordsize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CTETFBlood : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTETFBlood, CBaseTempEntity );

	DECLARE_SERVERCLASS();

	CTETFBlood( const char *name );

	virtual void Test( const Vector& current_origin, const QAngle& current_angles ) {}

public:
	Vector m_vecOrigin;
	Vector m_vecNormal;
	int m_nEntIndex;
};

// Singleton to fire TEMuzzleFlash objects
static CTETFBlood g_TETFBlood( "TFBlood" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTETFBlood::CTETFBlood( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	m_nEntIndex = 0;
}

IMPLEMENT_SERVERCLASS_ST( CTETFBlood, DT_TETFBlood )
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[0] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[1] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropFloat( SENDINFO_NOCHECK( m_vecOrigin[2] ), -1, SPROP_COORD_MP_INTEGRAL ),
	SendPropVector( SENDINFO_NOCHECK( m_vecNormal ), 6, 0, -1.0f, 1.0f ),
	SendPropInt( SENDINFO_NAME( m_nEntIndex, entindex ), MAX_EDICT_BITS, SPROP_UNSIGNED ),
END_SEND_TABLE()

void TE_TFBlood( IRecipientFilter& filter, float delay,
				const Vector &origin, const Vector &normal, int nEntIndex )
{
	g_TETFBlood.m_vecOrigin		= origin;
	g_TETFBlood.m_vecNormal		= normal;
	g_TETFBlood.m_nEntIndex		= nEntIndex;

	// Send it over the wire
	g_TETFBlood.Create( filter, delay );
}