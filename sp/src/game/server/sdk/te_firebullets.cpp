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
#include "basetempentity.h"


#define NUM_BULLET_SEED_BITS 8


//-----------------------------------------------------------------------------
// Purpose: Display's a blood sprite
//-----------------------------------------------------------------------------
class CTEFireBullets : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEFireBullets, CBaseTempEntity );
	DECLARE_SERVERCLASS();

					CTEFireBullets( const char *name );
	virtual			~CTEFireBullets( void );

public:
	CNetworkVar( int, m_iPlayer );	// player who fired
	CNetworkVector( m_vecOrigin );	// firing origin
	CNetworkQAngle( m_vecAngles );	// firing angle
	CNetworkVar( int, m_iWeaponID );	// weapon ID
	CNetworkVar( int, m_iMode );	// primary or secondary fire ?
	CNetworkVar( int, m_iSeed );	// shared random seed
	CNetworkVar( float, m_flSpread ); // bullets spread
	
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEFireBullets::CTEFireBullets( const char *name ) :
	CBaseTempEntity( name )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEFireBullets::~CTEFireBullets( void )
{
}

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEFireBullets, DT_TEFireBullets)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD ),
	SendPropAngle( SENDINFO_VECTORELEM( m_vecAngles, 0 ), 13, 0 ),
	SendPropAngle( SENDINFO_VECTORELEM( m_vecAngles, 1 ), 13, 0 ),
	SendPropInt( SENDINFO( m_iWeaponID ), 5, SPROP_UNSIGNED ), // max 31 weapons
	SendPropInt( SENDINFO( m_iMode ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iSeed ), NUM_BULLET_SEED_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iPlayer ), 6, SPROP_UNSIGNED ), 	// max 64 players, see MAX_PLAYERS
	SendPropFloat( SENDINFO( m_flSpread ), 10, 0, 0, 1 ),	
END_SEND_TABLE()


// Singleton
static CTEFireBullets g_TEFireBullets( "Shotgun Shot" );


void TE_FireBullets( 
	int	iPlayerIndex,
	const Vector &vOrigin,
	const QAngle &vAngles,
	int	iWeaponID,
	int	iMode,
	int iSeed,
	float flSpread )
{
	CPASFilter filter( vOrigin );
	filter.UsePredictionRules();

	g_TEFireBullets.m_iPlayer = iPlayerIndex-1;
	g_TEFireBullets.m_vecOrigin = vOrigin;
	g_TEFireBullets.m_vecAngles = vAngles;
	g_TEFireBullets.m_iSeed = iSeed;
	g_TEFireBullets.m_flSpread = flSpread;
	g_TEFireBullets.m_iMode = iMode;
	g_TEFireBullets.m_iWeaponID = iWeaponID;

	Assert( iSeed < (1 << NUM_BULLET_SEED_BITS) );
	
	g_TEFireBullets.Create( filter, 0 );
}
