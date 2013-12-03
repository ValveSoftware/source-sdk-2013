//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Create a muzzle flash temp ent
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basetempentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Dispatches user tracer stream tempentity
//-----------------------------------------------------------------------------
class CTEMuzzleFlash : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEMuzzleFlash, CBaseTempEntity );

	DECLARE_SERVERCLASS();

	CTEMuzzleFlash( const char *name );
	virtual			~CTEMuzzleFlash( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );

public:

	CNetworkVector( m_vecOrigin );
	CNetworkQAngle( m_vecAngles );
	CNetworkVar( float, m_flScale );
	CNetworkVar( int, m_nType );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEMuzzleFlash::CTEMuzzleFlash( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_vecAngles.Init();
	
	m_flScale	= 1.0f;
	m_nType		= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEMuzzleFlash::~CTEMuzzleFlash( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEMuzzleFlash::Test( const Vector& current_origin, const QAngle& current_angles )
{
}


IMPLEMENT_SERVERCLASS_ST( CTEMuzzleFlash, DT_TEMuzzleFlash )
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD ),
	SendPropVector( SENDINFO(m_vecAngles), -1, SPROP_COORD ),
	SendPropFloat( SENDINFO(m_flScale), -1, SPROP_NOSCALE ),
	SendPropInt( SENDINFO(m_nType), 32, SPROP_UNSIGNED ),
END_SEND_TABLE()

// Singleton to fire TEMuzzleFlash objects
static CTEMuzzleFlash g_TEMuzzleFlash( "MuzzleFlash" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			origin - 
//			*recipient - 
//			*origin - 
//			*dir - 
//			scale - 
//			type - 
//-----------------------------------------------------------------------------
void TE_MuzzleFlash( IRecipientFilter& filter, float delay,
	const Vector &start, const QAngle &angles, float scale, int type )
{
	g_TEMuzzleFlash.m_vecOrigin		= start;
	g_TEMuzzleFlash.m_vecAngles		= angles;
	g_TEMuzzleFlash.m_flScale		= scale;
	g_TEMuzzleFlash.m_nType			= type;

	// Send it over the wire
	g_TEMuzzleFlash.Create( filter, delay );
}