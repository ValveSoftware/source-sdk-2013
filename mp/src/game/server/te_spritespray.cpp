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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short	g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud

//-----------------------------------------------------------------------------
// Purpose: Dispatches Sprite Spray tempentity
//-----------------------------------------------------------------------------
class CTESpriteSpray : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTESpriteSpray, CBaseTempEntity );

					CTESpriteSpray( const char *name );
	virtual			~CTESpriteSpray( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVector( m_vecDirection );
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( int, m_nSpeed );
	CNetworkVar( float, m_fNoise );
	CNetworkVar( int, m_nCount );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTESpriteSpray::CTESpriteSpray( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_vecDirection.Init();
	m_nModelIndex = 0;
	m_fNoise = 0;
	m_nSpeed = 0;
	m_nCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTESpriteSpray::~CTESpriteSpray( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTESpriteSpray::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nModelIndex = g_sModelIndexSmoke;
	m_fNoise = 0.8;
	m_nCount = 5;
	m_nSpeed = 30;
	m_vecOrigin = current_origin;
	
	Vector forward, right;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward, &right, NULL );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );
	VectorMA( m_vecOrigin, -25.0, right, m_vecOrigin.GetForModify() );

	m_vecDirection.Init( random->RandomInt( -100, 100 ), random->RandomInt( -100, 100 ), random->RandomInt( 0, 100 ) );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTESpriteSpray, DT_TESpriteSpray)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecDirection), -1, SPROP_COORD),
	SendPropModelIndex(SENDINFO(m_nModelIndex)),
	SendPropFloat( SENDINFO(m_fNoise ), 8, SPROP_ROUNDDOWN, 0.0, 2.56 ),
	SendPropInt( SENDINFO(m_nSpeed ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nCount), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TESpriteSpray objects
static CTESpriteSpray g_TESpriteSpray( "Sprite Spray" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*pos - 
//			*dir - 
//			modelindex - 
//			speed - 
//			noise - 
//			count - 
//-----------------------------------------------------------------------------
void TE_SpriteSpray( IRecipientFilter& filter, float delay,
	const Vector *pos, const Vector *dir, int modelindex, int speed, float noise, int count )
{
	g_TESpriteSpray.m_vecOrigin		= *pos;
	g_TESpriteSpray.m_vecDirection	= *dir;
	g_TESpriteSpray.m_nModelIndex	= modelindex;	
	g_TESpriteSpray.m_nSpeed		= speed;
	g_TESpriteSpray.m_fNoise		= noise;
	g_TESpriteSpray.m_nCount		= count;

	// Send it over the wire
	g_TESpriteSpray.Create( filter, delay );
}