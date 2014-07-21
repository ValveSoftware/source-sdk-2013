//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "basetempentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: create clientside physics prop, as breaks model if needed
//-----------------------------------------------------------------------------
class CTEPhysicsProp : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPhysicsProp, CBaseTempEntity );

					CTEPhysicsProp( const char *name );
	virtual			~CTEPhysicsProp( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	virtual void	Precache( void );

	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkQAngle( m_angRotation );
	CNetworkVector( m_vecVelocity );
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( int, m_nSkin );
	CNetworkVar( int, m_nFlags );
	CNetworkVar( int, m_nEffects );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEPhysicsProp::CTEPhysicsProp( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_angRotation.Init();
	m_vecVelocity.Init();
	m_nModelIndex		= 0;
	m_nSkin				= 0;
	m_nFlags			= 0;
	m_nEffects			= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEPhysicsProp::~CTEPhysicsProp( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTEPhysicsProp::Precache( void )
{
	CBaseEntity::PrecacheModel( "models/gibs/hgibs.mdl" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEPhysicsProp::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nModelIndex = CBaseEntity::PrecacheModel( "models/gibs/hgibs.mdl" );
	m_nSkin = 0;
	m_vecOrigin = current_origin;
	m_angRotation = current_angles;
	
	m_vecVelocity.Init( random->RandomFloat( -10, 10 ), random->RandomFloat( -10, 10 ), random->RandomFloat( 0, 20 ) );
	m_nFlags = 0;
	m_nEffects = 0;
	
	Vector forward, right;

	m_vecOrigin += Vector( 0, 0, 24 );

	AngleVectors( current_angles, &forward, &right, 0 );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );
	VectorMA( m_vecOrigin, 25.0, right, m_vecOrigin.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTEPhysicsProp, DT_TEPhysicsProp)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 0), 13 ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 1), 13 ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 2), 13 ),
	SendPropVector( SENDINFO(m_vecVelocity), -1, SPROP_COORD),
	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropInt( SENDINFO(m_nSkin), ANIMATION_SKIN_BITS),
	SendPropInt( SENDINFO(m_nFlags), 2, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nEffects), EF_MAX_BITS, SPROP_UNSIGNED),
END_SEND_TABLE()

// Singleton to fire TEBreakModel objects
static CTEPhysicsProp s_TEPhysicsProp( "physicsprop" );

void TE_PhysicsProp( IRecipientFilter& filter, float delay,
	int modelindex, int skin, const Vector& pos, const QAngle &angles, const Vector& vel, int flags, int effects )
{
	s_TEPhysicsProp.m_vecOrigin		= pos;
	s_TEPhysicsProp.m_angRotation	= angles;
	s_TEPhysicsProp.m_vecVelocity	= vel;
	s_TEPhysicsProp.m_nModelIndex	= modelindex;	
	s_TEPhysicsProp.m_nSkin			= skin;
	s_TEPhysicsProp.m_nFlags		= flags;
	s_TEPhysicsProp.m_nEffects		= effects;

	// Send it over the wire
	s_TEPhysicsProp.Create( filter, delay );
}